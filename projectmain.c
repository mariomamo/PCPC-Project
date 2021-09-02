#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <mpi.h>
#define MAX_FILE_LIST_SIZE 100
#define MAX_FILE_NAME_LENGTH 255
#define MAX_WORD_SIZE 1024
#define FILE_NUMBER 1
#define TASK_ARRAY_SIZE 200000
#define MAX_HEAP_SIZE 100000
#define TASK_ARRAY_SIZE_2 2
#define MASTER_PROCESS_ID 0
#define TAG 100
#define MAX(a,b) (((a)>(b))?(a):(b))

int num_processes;

typedef struct {
    char fileName[MAX_FILE_NAME_LENGTH];
    int fileSize;
} FileInfo;

typedef struct {
    char fileName[MAX_FILE_NAME_LENGTH];
    long startFromBytes;
    long endToBytes;
} SubTask;

typedef struct {
    SubTask *subTasks;
    int size;
} Task;

typedef struct {
    char word[MAX_WORD_SIZE];
    long occurrences;
} Item;

struct BTreeNode {
    char word[MAX_WORD_SIZE];
    long occurrences;
    struct BTreeNode *left;
    struct BTreeNode *right;
};

struct ListNode {
    Item *item;
    struct ListNode *prev;
    struct ListNode *next;
};

struct BTreeNode* newBTree(struct BTreeNode *bt1, struct BTreeNode *bt2, char *word, int occurrences) {
    struct BTreeNode *node = calloc(1, sizeof(struct BTreeNode));
    // if (btree == NULL) return NULL;
    node -> left = bt1;
    node -> right = bt2;
    strcpy(node -> word, word);
    node -> occurrences = occurrences;
    return node;
}

struct BTreeNode* newEmptyBTree() {
    struct BTreeNode *node = calloc(1, sizeof(struct BTreeNode));
    node -> left = NULL;
    node -> right = NULL;
    strcpy(node -> word, "");
    node -> occurrences = 0;
    return node;
}

Item* newItem() {
    Item *item = calloc(1, sizeof(Item));
    return item;
}

struct ListNode* newListNode() {
    struct ListNode *listNode = calloc(1, sizeof(struct ListNode));
    listNode -> item = newItem();
}

int getFileSize(char *fileName) {
    char subTask[MAX_FILE_NAME_LENGTH] = "";
    strcat(subTask, fileName);
    // strcat(subTask, ".txt");
    FILE *fp = fopen(subTask, "r");
    fseek(fp, 0L, SEEK_END);
    return ftell(fp);
}

FileInfo* getFilesInfos() {
    char *fileNames[MAX_FILE_NAME_LENGTH] = {
        // "files/610_parole_HP.txt",
        // "files/1000_parole_italiane.txt",
        // "files/6000_parole_italiane.txt",
        "files/280000_parole_italiane.txt",
        // "files/test.txt"
    };

    // char *fileNames[MAX_FILE_NAME_LENGTH] = {
    //     "files/test_count.txt",
    //     "files/test.txt",
    //     "files/test2.txt",
    //     "files/test3.txt"
    // };

    FileInfo *filesInfos = malloc(FILE_NUMBER * sizeof(FileInfo));
    int i = 0;
    for (i = 0; i < FILE_NUMBER; i++) {
        FileInfo f;
        strcpy(f.fileName, fileNames[i]);
        f.fileSize = getFileSize(fileNames[i]);
        filesInfos[i] = f;
        // printf("+ [%s]\n|--+ %d\n", f.fileName, f.fileSize);
    }

    for (i = 0; i < FILE_NUMBER; i++) {
        printf("+ [%s]\n|--+ %d\n", filesInfos[i].fileName, filesInfos[i].fileSize);
    }

    return filesInfos;
}

int getTotalBytesFromFiles(FileInfo *fileInfo, int fileNumber) {
    int totalBytes = 0;
    int i = 0;
    for (i = 0; i < fileNumber; i++) {
        totalBytes += fileInfo[i].fileSize;
    }
    return totalBytes;
}

long* getNumberOfElementsPerProcess(int process_number, int elements_number) {
    if (process_number > elements_number) {
        // Division error
        printf("THERE ARE TOO MANY PROCESSES\nPROCESSES: %d, TOTAL BYTES: %d\n", process_number, elements_number);
        exit(0);
        process_number = elements_number;
    }
    long resto = elements_number % process_number;
    long elements_per_process = elements_number / process_number;
    long *arrayOfElements = malloc(sizeof(long) * process_number);

    int i;
    for (i = 0; i < process_number; i++) {
        arrayOfElements[i] = i < resto ? elements_per_process + 1 : elements_per_process;
    }
    
    return arrayOfElements;   
}

void logMessage(char *message, int rank) {
    char msg[1024];
    sprintf(msg, "[Processo %d] >> %s", rank, message);
    write(1, &msg, strlen(msg)+1);
    fflush(stdin);
}

void printArray(long *values, int my_rank, int size) {
    int i;
    char msg[50];

    for (i = 0; i < size; i++) {
        sprintf(msg, "########## values[%d]: %ld\n", i, values[i]);
        logMessage(msg, my_rank);
    }
}

void printSubTaskArray(SubTask *values, int process, int size) {
    int i;
    char msg[1000];

    for (i = 0; i < size; i++) {
        sprintf(msg, "[>>>>>> process[%d]: %s, %ld, %ld\n", process, values[i].fileName, values[i].startFromBytes, values[i].endToBytes);
        write(1, &msg, strlen(msg)+1);
    }
}

void printTaskArray(Task *values, long arraySize) {
    printf("ARRAY SIZE: %ld\n", arraySize);
    int i;
    char msg[1000];
    for (int i = 0; i < arraySize; i++) {
        printSubTaskArray(values[i].subTasks, i, values[i].size);
    }
}

int thereAreFilesToSplit(int index, int numFiles, int processIndex) {
    return index < numFiles && processIndex < num_processes ? 1 : 0;
}

int getRemainingBytesToSend(FileInfo fileInfo, int startOffset) {
    return fileInfo.fileSize - startOffset;
}

int canProcessReadWholeFile(FileInfo fileInfo, int startOffset, int remainingBytesToRead) {
    return getRemainingBytesToSend(fileInfo, startOffset) <= remainingBytesToRead ? 1 : 0;
}

void setSubTask(SubTask *subTask, FileInfo fileInfo, int startFromBytes, int endToBytes) {
    strcpy(subTask -> fileName, fileInfo.fileName);
    subTask -> startFromBytes = startFromBytes;
    subTask -> endToBytes = endToBytes - 1;
}

int isFileEnded(int sendedBytes, FileInfo fileInfo, int startOffset) {
    return sendedBytes == (fileInfo.fileSize - startOffset) ? 1 : 0;
}

int getSendedBytes(int endToBytes, int startFromBytes) {
    return endToBytes - startFromBytes;
}

long addSubTask(Task *task, long subTaskArrayMaximumSize, SubTask newSubTask) {
    // We want to do this: task -> subTasks[task -> size++] = subTask;
    SubTask *tempArray = task -> subTasks;
    // printf("<<< %zu - %zu\n", sizeof(tempArray), sizeof(Task));
    // printf("Addo %s: %ld - %ld\n", newSubTask.fileName, newSubTask.startFromBytes, newSubTask.endToBytes);
    // printf("subTaskArrayMaximumSize: %ld, taskArrayIndex: %d\n", subTaskArrayMaximumSize, task -> size);
    if (subTaskArrayMaximumSize <= task -> size) {
        subTaskArrayMaximumSize += TASK_ARRAY_SIZE_2;
        tempArray = realloc(tempArray, subTaskArrayMaximumSize * sizeof(SubTask));
        // printf(">>> %zu - %zu\n", sizeof(tempArray), sizeof(Task));
        // printf("REALLOC\n");
        // printf("NOW IS subTaskArrayMaximumSize: %ld, taskArrayIndex: %d\n", subTaskArrayMaximumSize, task -> size);
    }
    tempArray[task -> size] = newSubTask;
    task -> subTasks = tempArray;
    task -> size++;
    return subTaskArrayMaximumSize;
}

Task* newTask() {
    Task *task = malloc(1 * sizeof(Task));
    task -> subTasks = malloc(TASK_ARRAY_SIZE_2 * sizeof(SubTask));
    task -> size = 0;
    return task;
}

SubTask* newSubTask(char *fileName, int startFromBytes, int endToBytes) {
    SubTask *subTask = malloc(1 * sizeof(SubTask));
    strcpy(subTask -> fileName, fileName);
    subTask -> startFromBytes = startFromBytes;
    subTask -> endToBytes = endToBytes;
    return subTask;
}

int addTask(Task *taskArray, long taskArrayCurrentSize, Task *task) {
    // printf("%ld - %ld\n", sizeof(task -> subTasks), sizeof(Task)s);
    // TODO: CONTROLLARE SE FUNZIONA
    SubTask *temp = task -> subTasks;
    task -> subTasks = realloc(task -> subTasks, task -> size * sizeof(SubTask));
    // printf("%ld - %ld\n", sizeof(task -> subTasks), sizeof(Task));
    taskArray[taskArrayCurrentSize] = *task;
    task -> subTasks = calloc(TASK_ARRAY_SIZE_2, sizeof(SubTask));
    task -> size = 0;
    return taskArrayCurrentSize + 1;
}

Task *divideFilesBetweenProcesses(Task *taskArray, long *taskArrayCurrentSize, long *arrayBytesPerProcess, FileInfo *filesInfos, int numFiles) {
    *taskArrayCurrentSize = 0;
    long remainingBytesToRead = arrayBytesPerProcess[0];
    int currentFileInfo = 0;
    long startOffset = 0;
    int currentProcess = 0;
    int sendedBytes = 0;
    Task *task = newTask();
    long startFromBytes;
    long endToBytes;
    long subTaskArrayMaximumSize = TASK_ARRAY_SIZE_2;
    SubTask subTask = *newSubTask(subTask.fileName, subTask.startFromBytes, subTask.endToBytes);
    
    while (thereAreFilesToSplit(currentFileInfo, numFiles,currentProcess)) {
        startFromBytes = startOffset;
        if (canProcessReadWholeFile(filesInfos[currentFileInfo], startOffset, remainingBytesToRead)) {
            endToBytes = filesInfos[currentFileInfo].fileSize;
            setSubTask(&subTask, filesInfos[currentFileInfo], startOffset, endToBytes);
            sendedBytes = getSendedBytes(endToBytes, startFromBytes);
            remainingBytesToRead -= sendedBytes;
            // subTaskArrayMaximumSize = addSubTask(subTaskArray, subTaskArrayMaximumSize, subTask, subTaskArrayCurrentSize++);
            subTaskArrayMaximumSize = addSubTask(task, subTaskArrayMaximumSize, subTask);
            
            // task -> subTasks[task -> size++] = subTask;
            // task -> subTasks[task -> size++] = *newSubTask(subTask.fileName, subTask.startFromBytes, subTask.endToBytes);
            currentFileInfo++;
            startOffset = 0;
            
            if (remainingBytesToRead == 0) {
                remainingBytesToRead = arrayBytesPerProcess[currentProcess];
                currentProcess++;
                *taskArrayCurrentSize = addTask(taskArray, *taskArrayCurrentSize, task);
                subTaskArrayMaximumSize = 0;
            }

        } else {
            endToBytes = startOffset + remainingBytesToRead;
            setSubTask(&subTask, filesInfos[currentFileInfo], startOffset, endToBytes);
            sendedBytes = getSendedBytes(endToBytes, startFromBytes);
            remainingBytesToRead -= sendedBytes;

            // task -> subTasks[task -> size++] = *newSubTask(subTask.fileName, subTask.startFromBytes, subTask.endToBytes);
            // task -> subTasks[task -> size++] = subTask;
            subTaskArrayMaximumSize = addSubTask(task, subTaskArrayMaximumSize, subTask);

            if (isFileEnded(sendedBytes, filesInfos[currentFileInfo], startOffset)) {
                // printf("FILE FINITO 2: %d - %ld\n", sendedBytes, filesInfos[i].fileSize - startOffset);
                currentFileInfo++;
                startOffset = 0;
            } else {
                startOffset = endToBytes;
            }

            if (remainingBytesToRead == 0) {
                *taskArrayCurrentSize = addTask(taskArray, *taskArrayCurrentSize, task);
                subTaskArrayMaximumSize = 0;
            }

            currentProcess++;
            remainingBytesToRead = arrayBytesPerProcess[currentProcess];
        }
    }

    return taskArray;
}

void createSubTaskMPIStruct(MPI_Datatype *subTaskType) {
    int subTaskBlockCounts[3];
    int taskBlockCounts[2];
    MPI_Aint lb, mpi_long_extent, mpi_char_extent, sub_task_extent;
    MPI_Aint subTaskOffsets[3], taskOffsets[2];
    MPI_Datatype subTaskArrayTypes[3];
    MPI_Datatype taskArrayTypes[2];

    MPI_Type_get_extent(MPI_LONG, &lb, &mpi_long_extent);
    MPI_Type_get_extent(MPI_CHAR, &lb, &mpi_char_extent);

    subTaskOffsets[0] = 0;
    subTaskArrayTypes[0] = MPI_CHAR;
    subTaskBlockCounts[0] = MAX_FILE_NAME_LENGTH;

    subTaskOffsets[1] = MAX_FILE_NAME_LENGTH + mpi_char_extent;
    subTaskArrayTypes[1] = MPI_LONG;
    subTaskBlockCounts[1] = 1;

    subTaskOffsets[2] = subTaskOffsets[1] + mpi_long_extent;
    subTaskArrayTypes[2] = MPI_LONG;
    subTaskBlockCounts[2] = 1;

    MPI_Type_create_struct(3, subTaskBlockCounts, subTaskOffsets, subTaskArrayTypes, subTaskType);
    MPI_Type_commit(subTaskType);
}

void scatterTasks(Task *taskArray, int taskArrayCurrentSize, MPI_Datatype subTaskType) {
    int taskIndex = 0;
    for (taskIndex = 0; taskIndex < taskArrayCurrentSize; taskIndex++) {
        Task task = taskArray[taskIndex];
        long subTaskIndex = 0;
        MPI_Send(&task.size, 1, MPI_INT, taskIndex + 1, TAG, MPI_COMM_WORLD);
        for (subTaskIndex = 0; subTaskIndex < task.size; subTaskIndex++) {
            SubTask subTask = task.subTasks[subTaskIndex];
            MPI_Send(&subTask, 1, subTaskType, taskIndex + 1, TAG, MPI_COMM_WORLD);
        }
    }
}

void addToBTree(struct BTreeNode *btree, char *word) {
    // printf("%d\n", strcmp(btree -> word, ""));
    char p1[MAX_WORD_SIZE], p2[MAX_WORD_SIZE];
    strcpy(p1, btree -> word);
    strcpy(p2, word);
    int compare = strcmp(btree -> word, word);
    if (strcmp(btree -> word, "") == 0) {
        // printf("COMPARE: %s - %s = %d\n", p1, p2, compare);
        strcpy(btree -> word, word);
        btree -> occurrences = 1;
        btree -> left = NULL;
        btree -> right = NULL;
        // printf("WORD: %s", btree -> word);
        return;
    }
    while (1) {
        compare = strcmp(btree -> word, word);
        strcpy(p1, btree -> word);
        strcpy(p2, word);
        // printf("COMPARE: %s - %s = %d\n", p1, p2, compare);
        if (compare == 0) {
            // printf("UGUALI\n");
            btree -> occurrences += 1;
            // printf("[UGUALE] Aggiunto %s", word);
            // printf("OCCURRENCES: %ld\n", btree -> occurrences);
            return;
        } if (compare < 0) {
            // printf("MINORE\n");
            if (btree -> left != NULL) {
                // printf("VADO A SINISTRA\n");
                btree = btree -> left;
            } else {
                // printf("WORD: %s", btree -> left -> word);
                btree -> left = newBTree(NULL, NULL, word, 1);
                // printf("[MINORE] Aggiunto %s", word);
                return;
                // printf("WORD: %s", btree -> left -> word);
                // VADO NEL SOTTOALBERO DI SINISTRA
            }
        } else {
            // printf("MAGGIORE\n");
            if (btree -> right != NULL) {
                // printf("VADO A DESTRA\n");
                btree = btree -> right;
            } else {
                // printf("[MAGGIORE] Aggiunto %s", word);
                btree -> right = newBTree(NULL, NULL, word, 1);
                return;
            }
            // VADO NEL SOTTOALBERO DI DESTRA
        }
    }
}

void byLevel(struct BTreeNode *t) {
    int maxNodes = 10000;
    struct BTreeNode *nodesToPrint[maxNodes];
    int actualIndex = 0;
    int lastIndex = 1;
    int maximumIndex = 1;
    if (t == NULL) return;
    nodesToPrint[actualIndex] = t;
    for (actualIndex = 0 ; actualIndex < maximumIndex; actualIndex++) {
        struct BTreeNode *currentNode = nodesToPrint[actualIndex];
        printf("%s: %ld\n", currentNode -> word, currentNode -> occurrences);
        if (currentNode -> left != NULL) {
            // printf("ADD LEFT: %s", currentNode -> left -> word);
            nodesToPrint[lastIndex++] = currentNode -> left;
            maximumIndex++;
        }
        if (currentNode -> right != NULL) {
            // printf("ADD RIGHT: %s", currentNode -> right -> word);
            nodesToPrint[lastIndex++] = currentNode -> right;
            maximumIndex++;
        }
        // return;
    }

    // printf("FINITO\n");
}

void toLowerString(char *str) {
    int i = 0;
    int length = strlen(str);
    for (i = 0; i < length - 2; i++) {
        str[i] = tolower(str[i]);
    }
    if (str[length - 1] == '\n')
        str[length - 2] = 0;
}

struct BTreeNode* countWords(SubTask *subTask, int rank) {
    rank = rank - 1;
    struct BTreeNode *btree = newEmptyBTree();
    // printf("[%d] Ho ricevuto %s - %ld - %ld\n", rank, subTask -> fileName, subTask -> startFromBytes, subTask -> endToBytes);
    FILE *file = fopen(subTask -> fileName, "r");
    long start = subTask -> startFromBytes;
    int remainingBytesToRead = subTask -> endToBytes + 1 - subTask -> startFromBytes;
    int lineSize = 1000;
    int parole = 0;
    char line[lineSize];
    // printf("Processo %d, leggo %d\n", rank, remainingBytesToRead);
    if (start != 0) {
        fseek(file, start-1, SEEK_SET);
        char c = fgetc(file);
        if (c != EOF) {
            if (c != '\n') {
                // printf("NO BUENO: %c\n", c);
                // fgets(line, lineSize, file);
                // printf("la parola è: %s", line);
                while ((c = fgetc(file)) != '\n') {
                    remainingBytesToRead--;
                }
                remainingBytesToRead --;
                // printf("Processo %d, in realtà devo leggere %d\n", rank, remainingBytesToRead);
            } else {
                // printf("BUENO\n");
            }
        }
    }
    // int remainingBytesToRead = 100;
    int chread = 0;
    char *read;
    while (remainingBytesToRead > 0 && (read = fgets(line, lineSize, file))) {
        int readedBytes = strlen(line);
        // if (rank == 1)
        //     printf("REMAINING: %d - %d = %d\n", remainingBytesToRead, readedBytes, remainingBytesToRead - readedBytes);
        remainingBytesToRead -= readedBytes;
        parole++;
        // printf("LINE: %s", line);
        // printf("PRIMA: %s", line);
        toLowerString(line);
        // printf("[%d] DOPO: %s\n", rank, line);
        addToBTree(btree, line);
        chread += readedBytes;
    }
    // printf("[%d] last word: %s\n", rank, line);
    fclose(file);
    // printf("\n[%d] Ho letto %d bytes\nParole: %d\n", rank, chread, parole);
    return btree;
}

void addToList(struct ListNode *head, struct BTreeNode *btreeNode) {
    struct ListNode *currentNode = head;
    printf("STO AGGIUNGENDO: %s - %ld\n", btreeNode -> word, btreeNode -> occurrences);

    if (strcmp(currentNode -> item -> word, "") == 0) {
        // printf("OH YES\n");
        Item *item = newItem();
        strcpy(item -> word, btreeNode -> word);
        item -> occurrences = btreeNode -> occurrences;
        currentNode -> item = item;
        return;
    }


    if (btreeNode -> occurrences > head -> item -> occurrences) {
        printf("O CHIù GRUOSS\n");
        struct ListNode *tempNode = newListNode();
        strcpy(tempNode -> item -> word, head -> item -> word);
        tempNode -> item -> occurrences = head -> item -> occurrences;
        // struct ListNode *prevNode = head -> next;
        printf("%s", head -> next -> item -> word);
        // prevNode -> prev = tempNode;
        // tempNode -> next = prevNode;
        printf("CHECCè\n");
        tempNode -> prev = head;

        strcpy(head -> item -> word, btreeNode -> word);
        head -> item -> occurrences = btreeNode -> occurrences;
        head -> next = tempNode;
        head -> prev = NULL;

        return;
    }

    // printf("I'm here\n");
    // while (currentNode -> next != NULL && btreeNode -> occurrences <= currentNode -> item -> occurrences) {
    //     // printf("OCCURRENCES: %s - %ld\n", currentNode -> item -> word, currentNode -> item -> occurrences);
    //     currentNode = currentNode -> next;
    // }

    // printf("STO QUA\n");

    struct ListNode *newNode = newListNode();
    Item *item = newItem();
    strcpy(item -> word, btreeNode -> word);
    item -> occurrences = btreeNode -> occurrences;
    newNode -> item = item;
    newNode -> next = currentNode;
    newNode -> prev = currentNode -> prev;
    // currentNode -> prev -> next = newNode;
    // currentNode -> prev = newNode;
    // newNode -> next = currentNode;
    // struct ListNode *prevNode = currentNode -> prev;
    // if (prevNode != NULL) {
    //     prevNode -> next = newNode;
    // }
}

void printList(struct ListNode *list) {
    // printf("WHAT: %d\n", list -> next -> item -> occurrences);
    while (list != NULL) {
        printf("PRINT LIST >>> %s: %ld\n", list -> item -> word, list -> item -> occurrences);
        list = list -> next;
    }
}

void printHeap(Item *heap, long size) {
    int i = 0;
    for (i = 0; i < size; i++) {
        printf("%s: %ld\n", heap[i].word, heap[i].occurrences);
        // printf("SIZE: %d\n", i);
    }
    printf("=== END\n");
}

void swapItems(Item *item1, Item *item2) {
    Item *temp = newItem();
    temp -> occurrences = item1 -> occurrences;
    strcpy(temp -> word, item1 -> word);

    strcpy(item1 -> word, item2 -> word);
    item1 -> occurrences = item2 -> occurrences;

    strcpy(item2 -> word, temp -> word);
    item2 -> occurrences = temp -> occurrences;

    free(temp);
}

void maxHeapify(Item *heap, long currPos) {
    int padre = (currPos - 1) / 2;
    int itemPos = currPos;
    while (padre >= 0 && heap[padre].occurrences < heap[itemPos].occurrences) {
        swapItems(&heap[padre], &heap[itemPos]);
        itemPos = padre;
        padre = (padre - 1) / 2;
    }
}

int createOrderedArrayFromBtree(Item *heap, struct BTreeNode *btree) {
    // Item *heap = calloc(MAX_HEAP_SIZE, sizeof(Item));

    int maxNodes = MAX_HEAP_SIZE;
    struct BTreeNode *nodesToScan[maxNodes];
    int actualIndex = 0;
    int lastIndex = 1;
    int maximumIndex = 1;
    nodesToScan[actualIndex] = btree;
    for (actualIndex = 0; actualIndex < maximumIndex; actualIndex++) {
        struct BTreeNode *currentNode = nodesToScan[actualIndex];
        // printf("%s: %ld\n", currentNode -> word, currentNode -> occurrences);

        Item *item = newItem();
        strcpy(item -> word, currentNode -> word);
        item -> occurrences = currentNode -> occurrences;
        heap[actualIndex] = *item;
        // printf(">> %s: %ld\n", heap[actualIndex].word, heap[actualIndex].occurrences);
        maxHeapify(heap, actualIndex);

        if (currentNode -> left != NULL) {
            // printf("ADD LEFT: %s\n", currentNode -> left -> word);
            nodesToScan[lastIndex++] = currentNode -> left;
            maximumIndex++;
        }
        if (currentNode -> right != NULL) {
            // printf("ADD RIGHT: %s\n", currentNode -> right -> word);
            nodesToScan[lastIndex++] = currentNode -> right;
            maximumIndex++;
        }
    }
    return actualIndex - 1;
}

Item getItemWithValue(char *name, int occurrences) {
    Item item = *newItem();
    strcpy(item.word, name);
    item.occurrences = occurrences;
    return item;
}

Item* getMaximumFromHeap(Item *heap, long size) {
    Item *max = newItem();
    strcpy(max -> word, heap[0].word);
    max -> occurrences = heap[0].occurrences;

    strcpy(heap[0].word, heap[size - 1].word);
    heap[0].occurrences = heap[size - 1].occurrences;

    strcpy(heap[size - 1].word, "");
    heap[size - 1].occurrences = -1;

    // free(heap[size]);
    // heap[size].word = NULL;
    // printf("%s %ld\n", heap[size].word, heasp[size].occurrences);

    // BALANCE HEAP
    int leftChildIndex = 0;
    int rightChildIndex = 0;
    // printf("%s - %ld\n", maxChild -> word, maxChild -> occurrences);

    int pos = 0;
    while (pos <= size - 1) {
        leftChildIndex = (2 * pos) + 1;
        rightChildIndex = (2 * pos) + 2;
        int maxChildIndex = -1;
        if (leftChildIndex < size && heap[leftChildIndex].occurrences > -1) {
            maxChildIndex = leftChildIndex;
        }
        if (rightChildIndex < size && heap[rightChildIndex].occurrences > - 1) {
            if (maxChildIndex == -1) maxChildIndex = rightChildIndex;
            else if (heap[rightChildIndex].occurrences > heap[maxChildIndex].occurrences) maxChildIndex = rightChildIndex;
        }
        if (maxChildIndex == -1) break;
        // printf("LEFT: %s - %ld\n", heap[leftChildIndex].word, heap[leftChildIndex].occurrences);
        // printf("RIGHT: %s - %ld\n", heap[rightChildIndex].word, heap[rightChildIndex].occurrences);
        // printf("MAX: %s - %ld\n\n", heap[maxChildIndex].word, heap[maxChildIndex].occurrences);
        swapItems(&heap[maxChildIndex], &heap[pos]);
        pos = maxChildIndex;
        // break;
    }    

    return max;
}

void createCSV(Item *heap, long size, int rank) {
    Item *maximum;
    char fileName[MAX_FILE_NAME_LENGTH];
    sprintf(fileName, "files/output_%d.txt", rank);

    FILE *output = fopen(fileName, "w");
    fprintf(output, "WORD,COUNT\n");

    int s = size;
    while (s > 0) {
        maximum = getMaximumFromHeap(heap, size);
        fprintf(output, "%s,%ld\n", maximum -> word, maximum -> occurrences);
        s--;
    }

	fclose(output);
}

void testHeap() {
    Item heap[12];
    char *names[12] = {"Mario", "Nicola", "Guido", "Gilberto", "PieroAngela", "GoogleMerda", "Antonio", "Pidocchio", "Geppetto", "Orecchiali", "Trottola", "Trottolino"};
    int numbers[12] = {1, 4, 5, 9, 8, 7, 6, 10, 11, 15, 12, 13};

    int i = 0;
    for (i = 0; i < 12; i++) {
        heap[i] = getItemWithValue(names[i], numbers[i]);
        maxHeapify(heap, i);
    }

    // printHeap(heap, 11);
    createCSV(heap, 12, 0);
}

int main(int argc, char **argv) {
    int rank;

    MPI_Datatype subTaskType;
    MPI_Status status;

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    num_processes -= 1;

    createSubTaskMPIStruct(&subTaskType);

    SubTask subTask;

    if (rank == MASTER_PROCESS_ID){
        FileInfo *filesInfos = getFilesInfos();
        int totalBytes = getTotalBytesFromFiles(filesInfos, FILE_NUMBER);
        Task *taskArray = calloc(TASK_ARRAY_SIZE, sizeof(Task));
        long *taskArrayCurrentSize = malloc(1 * sizeof(long));
        long *arrayBytesPerProcess = getNumberOfElementsPerProcess(num_processes, totalBytes);
        printArray(arrayBytesPerProcess, 0, num_processes);
        divideFilesBetweenProcesses(taskArray, taskArrayCurrentSize, arrayBytesPerProcess, filesInfos, FILE_NUMBER);
        // printf("%d\n", *taskArrayCurrentSize);
        // printTaskArray(taskArray, *taskArrayCurrentSize);

        scatterTasks(taskArray, *taskArrayCurrentSize, subTaskType);
        
    } else {
        int numerOfTasks = 0;
        MPI_Recv(&numerOfTasks, 1, MPI_INT, MASTER_PROCESS_ID, TAG, MPI_COMM_WORLD, &status);
        // printf("I'm the process number %d and I'm waiting for %d tasks\n", rank, numerOfTasks);
        int currentTask = 0;
        for (currentTask = 0; currentTask < numerOfTasks; currentTask++) {
            MPI_Recv(&subTask, 1, subTaskType, MASTER_PROCESS_ID, TAG, MPI_COMM_WORLD, &status);
            // printf("Processo [%d] ho ricevuto %s - % ld - %ld\n", rank, subTask.fileName, subTask.startFromBytes, subTask.endToBytes);
            struct BTreeNode *btree = countWords(&subTask, rank);
            // byLevel(btree);
            // printf("\n\n");
            Item *heap = calloc(MAX_HEAP_SIZE, sizeof(Item));
            int size = createOrderedArrayFromBtree(heap, btree);
            // printHeap(heap, size);
            createCSV(heap, size, rank);

            // testHeap();

        }
    }

    MPI_Finalize();
    return 0;
}
