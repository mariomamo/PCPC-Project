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
#define TASK_ARRAY_SIZE 400000
#define MAX_HEAP_SIZE 4000
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
    int height;
    struct BTreeNode *left;
    struct BTreeNode *right;
};

struct ListNode {
    Item *item;
    struct ListNode *prev;
    struct ListNode *next;
};

struct BTreeNode* newBTree(struct BTreeNode *bt1, struct BTreeNode *bt2, char *word, long occurrences) {
    struct BTreeNode *node = calloc(1, sizeof(struct BTreeNode));
    // if (btree == NULL) return NULL;
    node -> left = bt1;
    node -> right = bt2;
    strcpy(node -> word, word);
    node -> occurrences = occurrences;
    node -> height = 1;
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

Item* newItemWithValues(char *word, int occurrences) {
    Item *item = calloc(1, sizeof(Item));
    strcpy(item -> word, word);
    item -> occurrences = occurrences;
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
        // "files/280000_parole_italiane.txt",
        "files/test.txt",
        // "files/altri/commedia.txt",
        // "files/altri/bible.txt",
        // "files/altri/03-The_Return_Of_The_King.txt",
        // "files/altri/02-The_Two_Towers.txt",
    };

    // char *fileNames[MAX_FILE_NAME_LENGTH] = {
        // "files/minicommedia.txt",
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
        sprintf(msg, ">>>>>> process[%d]: %s, %ld, %ld\n", process, values[i].fileName, values[i].startFromBytes, values[i].endToBytes);
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

// void createSubTaskMPIStruct(MPI_Datatype *subTaskType) {
//     int subTaskBlockCounts[3];
//     MPI_Aint lb, mpi_long_extent, mpi_char_extent;
//     MPI_Aint subTaskOffsets[3];
//     MPI_Datatype subTaskArrayTypes[3];

//     MPI_Type_get_extent(MPI_LONG, &lb, &mpi_long_extent);
//     MPI_Type_get_extent(MPI_CHAR, &lb, &mpi_char_extent);

//     subTaskOffsets[0] = 0;
//     subTaskArrayTypes[0] = MPI_CHAR;
//     subTaskBlockCounts[0] = MAX_FILE_NAME_LENGTH;

//     subTaskOffsets[1] = MAX_FILE_NAME_LENGTH + mpi_char_extent;
//     subTaskArrayTypes[1] = MPI_LONG;
//     subTaskBlockCounts[1] = 1;

//     subTaskOffsets[2] = subTaskOffsets[1] + mpi_long_extent;
//     subTaskArrayTypes[2] = MPI_LONG;
//     subTaskBlockCounts[2] = 1;

//     MPI_Type_create_struct(3, subTaskBlockCounts, subTaskOffsets, subTaskArrayTypes, subTaskType);
//     MPI_Type_commit(subTaskType);
// }

void createSubTaskMPIStruct(MPI_Datatype *subTaskType) {
    int subTaskBlockCounts[3];
    MPI_Aint subTaskOffsets[3], base_address;
    MPI_Datatype subTaskArrayTypes[3];

    SubTask st;

    MPI_Get_address(&st, &base_address);
    
    MPI_Get_address(&st.fileName, &subTaskOffsets[0]);
    subTaskOffsets[0] = MPI_Aint_diff(subTaskOffsets[0], base_address);
    subTaskArrayTypes[0] = MPI_CHAR;
    subTaskBlockCounts[0] = MAX_FILE_NAME_LENGTH;

    MPI_Get_address(&st.startFromBytes, &subTaskOffsets[1]);
    subTaskOffsets[1] = MPI_Aint_diff(subTaskOffsets[1], base_address);
    subTaskArrayTypes[1] = MPI_LONG;
    subTaskBlockCounts[1] = 1;

    MPI_Get_address(&st.endToBytes, &subTaskOffsets[2]);
    subTaskOffsets[2] = MPI_Aint_diff(subTaskOffsets[2], base_address);
    subTaskArrayTypes[2] = MPI_LONG;
    subTaskBlockCounts[2] = 1;

    MPI_Type_create_struct(3, subTaskBlockCounts, subTaskOffsets, subTaskArrayTypes, subTaskType);
    MPI_Type_commit(subTaskType);
}

void createItemMPIStruct(MPI_Datatype *itemType) {
    int itemBlockCounts[2];
    MPI_Aint itemOffsets[2], base_address;
    MPI_Datatype itemsArrayTypes[2];

    Item it;

    MPI_Get_address(&it, &base_address);

    MPI_Get_address(&it.word, &itemOffsets[0]);
    itemOffsets[0] = MPI_Aint_diff(itemOffsets[0], base_address);
    itemsArrayTypes[0] = MPI_CHAR;
    itemBlockCounts[0] = MAX_WORD_SIZE;

    MPI_Get_address(&it.occurrences, &itemOffsets[1]);
    itemOffsets[1] = MPI_Aint_diff(itemOffsets[1], base_address);
    itemsArrayTypes[1] = MPI_LONG;
    itemBlockCounts[1] = 1;

    MPI_Type_create_struct(2, itemBlockCounts, itemOffsets, itemsArrayTypes, itemType);
    MPI_Type_commit(itemType);
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

// void addToBTree(struct BTreeNode *btree, char *word) {
//     // printf("%d\n", strcmp(btree -> word, ""));
//     char p1[MAX_WORD_SIZE], p2[MAX_WORD_SIZE];
//     strcpy(p1, btree -> word);
//     strcpy(p2, word);
//     int compare = strcmp(btree -> word, word);
//     if (strcmp(btree -> word, "") == 0) {
//         // printf("COMPARE: %s - %s = %d\n", p1, p2, compare);
//         strcpy(btree -> word, word);
//         btree -> occurrences = 1;
//         btree -> left = NULL;
//         btree -> right = NULL;
//         // printf("WORD: %s", btree -> word);
//         return;
//     }
//     while (1) {
//         compare = strcmp(btree -> word, word);
//         strcpy(p1, btree -> word);
//         strcpy(p2, word);
//         // printf("COMPARE: %s - %s = %d\n", p1, p2, compare);
//         if (compare == 0) {
//             // printf("UGUALI\n");
//             btree -> occurrences += 1;
//             // printf("[UGUALE] Aggiunto %s", word);
//             // printf("OCCURRENCES: %ld\n", btree -> occurrences);
//             return;
//         } if (compare < 0) {
//             // printf("MINORE\n");
//             if (btree -> left != NULL) {
//                 // printf("VADO A SINISTRA\n");
//                 btree = btree -> left;
//             } else {
//                 // printf("WORD: %s", btree -> left -> word);
//                 btree -> left = newBTree(NULL, NULL, word, 1);
//                 // printf("[MINORE] Aggiunto %s", word);
//                 return;
//                 // printf("WORD: %s", btree -> left -> word);
//                 // VADO NEL SOTTOALBERO DI SINISTRA
//             }
//         } else {
//             // printf("MAGGIORE\n");
//             if (btree -> right != NULL) {
//                 // printf("VADO A DESTRA\n");
//                 btree = btree -> right;
//             } else {
//                 // printf("[MAGGIORE] Aggiunto %s", word);
//                 btree -> right = newBTree(NULL, NULL, word, 1);
//                 return;
//             }
//             // VADO NEL SOTTOALBERO DI DESTRA
//         }
//     }
// }

int getTreeHeight(struct BTreeNode *node) {
    if (node == NULL) return 0;
    return node -> height;
}

int getBalance(struct BTreeNode *node) {
    if (node == NULL) return 0;
    return getTreeHeight(node -> left) - getTreeHeight(node -> right);
}

struct BTreeNode* rightRotate(struct BTreeNode *node) {
    struct BTreeNode *leftChild = node -> left;
    struct BTreeNode *rightChildOfLeftChild = leftChild -> right;
 
    // Perform rotation
    leftChild -> right = node;
    node -> left = rightChildOfLeftChild;
 
    // Update heights
    node -> height = MAX(getTreeHeight(node -> left), getTreeHeight(node -> right)) + 1;
    leftChild -> height = MAX(getTreeHeight(leftChild -> left), getTreeHeight(leftChild -> right)) + 1;

    // Return new root
    return leftChild;
}

struct BTreeNode* leftRotate(struct BTreeNode *node) {
    struct BTreeNode *rightChild = node -> right;
    struct BTreeNode *leftChildOfRightChild = rightChild -> left;
 
    // Perform rotation
    rightChild -> left = node;
    node -> right = leftChildOfRightChild;
 
    //  Update heights
    node -> height = MAX(getTreeHeight(node -> left), getTreeHeight(node -> right)) + 1;
    rightChild -> height = MAX(getTreeHeight(rightChild -> left), getTreeHeight(rightChild -> right)) + 1;

    // Return new root
    return rightChild;
}

int compareByName(Item *item, struct BTreeNode *btreeNode) {
    return strcmp(item -> word, btreeNode -> word);
}

int compareByOccurrences(Item *item, struct BTreeNode *btreeNode) {
    if (item -> occurrences > btreeNode -> occurrences) return -1;
    if (item -> occurrences < btreeNode -> occurrences) return 1;
    return compareByName(item, btreeNode);
}

struct BTreeNode* addToAVL(struct BTreeNode *btree, Item item, int (*compareFunction)()) {
    if (btree == NULL || strcmp(btree -> word, "") == 0) {
        return newBTree(NULL, NULL, item.word, item.occurrences);
    }

    int compare = compareFunction(&item, btree);
    if (compare < 0)
        btree -> left  = addToAVL(btree -> left, item, compareFunction);
    else if (compare > 0)
        btree -> right = addToAVL(btree -> right, item, compareFunction);
    else {
        btree -> occurrences += item.occurrences;
        return btree;
    }

    // printf("%d\n", getTreeHeight(btree -> left));
    btree -> height = MAX(getTreeHeight(btree -> left), getTreeHeight(btree -> right)) + 1;

    int balance = getBalance(btree);
    
    // printf("Balanced: %d\n", balance);

    int leftCompare = btree -> left != NULL ? compareFunction(&item, btree -> left) : 0;
    int rightCompare = btree -> right != NULL ? compareFunction(&item, btree -> right) : 0;

    // Left Left Case
    if (balance > 1 && leftCompare < 0)
        return rightRotate(btree);
 
    // Right Right Case
    if (balance < -1 && rightCompare > 0)
        return leftRotate(btree);
 
    // Left Right Case
    if (balance > 1 && leftCompare > 0) {
        btree -> left =  leftRotate(btree -> left);
        return rightRotate(btree);
    }

    // Right Left Case
    if (balance < -1 && rightCompare < 0) {
        btree -> right = rightRotate(btree -> right);
        return leftRotate(btree);
    }

    return btree;
}

struct BTreeNode* orderAVLByOccurrences(struct BTreeNode *oldTree, struct BTreeNode *newTree) {
    if (oldTree != NULL) {
        newTree = orderAVLByOccurrences(oldTree -> left, newTree);
        newTree = addToAVL(newTree, *newItemWithValues(oldTree -> word, oldTree -> occurrences), compareByOccurrences);
        newTree = orderAVLByOccurrences(oldTree -> right, newTree);
    }
    return newTree;
}

void preOrder(struct BTreeNode *rootNode) {
    if(rootNode != NULL) {
        printf("%s: %ld\n", rootNode -> word, rootNode -> occurrences);
        preOrder(rootNode -> left);
        preOrder(rootNode -> right);
    }
}

void inOrder(struct BTreeNode *rootNode, int rank){
    if (rootNode != NULL) {
        inOrder(rootNode -> left, rank);
        printf("[%d] %s: %ld\n", rank, rootNode -> word, rootNode -> occurrences);
        inOrder(rootNode -> right, rank);
    }
}

long createArrayFromAVL(Item *wordsList, struct BTreeNode *rootNode, long index) {
    if (rootNode != NULL) {
        index = createArrayFromAVL(wordsList, rootNode -> left, index);
        wordsList[index++] = *newItemWithValues(rootNode -> word, rootNode -> occurrences);
        // printf("INDEX: %ld\n", index);
        return createArrayFromAVL(wordsList, rootNode -> right, index);
    }
    return index;
}

void byLevel(struct BTreeNode *t) {
    int maxNodes = 10000;
    struct BTreeNode *nodesToPrint[maxNodes];
    int actualIndex = 0;
    int lastIndex = 1;
    int maximumIndex = 1;
    if (t == NULL) return;
    nodesToPrint[actualIndex] = t;
    for (actualIndex = 0; actualIndex < maximumIndex; actualIndex++) {
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
    for (i = 0; i < length; i++) {
        str[i] = tolower(str[i]);
    }
    str[length] = 0;
}

/*
* A-Z = 65 - 90
* a-z = 97 - 122
* Accented letters = '-65' - '-128'
* 192 -> 255 (215 and 247 excluded)
*/
int isCharacter(char ch) {
    // -105 = ×
    // -73 = ÷
    // printf("%d\n", 'è');
    return ch >= 65 && ch <= 90 || ch >= 97 && ch <= 122;
}

struct BTreeNode* countWords(SubTask *subTask, int rank) {
    rank = rank - 1;
    // struct BTreeNode *btree = newEmptyBTree();
    struct BTreeNode *btree = NULL;
    // printf("[%d] Ho ricevuto %s - %ld - %ld\n", rank, subTask -> fileName, subTask -> startFromBytes, subTask -> endToBytes);
    FILE *file = fopen(subTask -> fileName, "r");
    long start = subTask -> startFromBytes;
    // long start = 50;
    int remainingBytesToRead = subTask -> endToBytes + 1 - subTask -> startFromBytes;
    // int remainingBytesToRead = 50;
    int lineSize = 1000;
    int parole = 0;
    char c;
    int chread = 0;
    // printf("Processo %d, leggo %d\n", rank, remainingBytesToRead);
    if (start != 0) {
        fseek(file, start-1, SEEK_SET);
        c = fgetc(file);
        // printf("Pred char: %c\n", c);
        if (c != EOF && c != '\n' && c != ' ') {
            // printf("NO BUENO: %c\n", c);
            // fgets(line, lineSize, file);
            // printf("la parola è: %s", line);
            while (remainingBytesToRead > 0 && (c = fgetc(file)) != EOF && c != '\n' && c != ' ') {
                // printf("CHAR: %c\n", c);
                remainingBytesToRead--;
                chread++;
            }
            remainingBytesToRead--;
            // printf("Processo %d, in realtà devo leggere %d\n", rank, remainingBytesToRead);
        } else {
            // printf("BUENO\n");
        }
    }
    // int remainingBytesToRead = 100;
    int readedBytes;
    char line[lineSize];
    while (remainingBytesToRead > 0) {
        readedBytes = 0;
        while (remainingBytesToRead > 0 && (c = fgetc(file)) != EOF && (c == ' ' || c == '\r' || c == '\n' || !isCharacter(c))) {
            remainingBytesToRead--;
            // printf("Remaining: %d\n", remainingBytesToRead);
            // printf("Scarto questo: %c: remaining: %d\n", c, remainingBytesToRead);
            chread++;
        }
        if (remainingBytesToRead <= 0) break;
        while (c != EOF && c != ' ' && c != '\r' && c != '\n' && isCharacter(c)) {
            // printf("[%d] Reading %c\n", rank, c);
            line[readedBytes++] = c;
            c = fgetc(file);
            // printf("PROX: %d\n", c);
        }
        // printf("[%d] Readed %c\n", rank, 'è');
        line[readedBytes] = 0;
        if (c != EOF) readedBytes++;
        remainingBytesToRead -= readedBytes;
        parole++;
        chread += readedBytes;
        // printf("[%d] Readed: %d, Remaining: %d\n", rank, readedBytes, remainingBytesToRead);

        toLowerString(line);
        // printf("[%d] DOPO: %s\n", rank, line);
        // addToBTree(btree, line);
        btree = addToAVL(btree, *newItemWithValues(line, 1), compareByName);

        // printf("\n[%d] Ho letto %d bytes\nParole: %d\n", rank, chread, parole);
        // if (c == EOF) {
        //     chread--;
        //     break;
        // }
        // printf("\n[%d] Ho letto %d bytes\n", rank, readedBytes);
    }
    // printf("[%d] last word: %s\n", rank, line);
    fclose(file);
    // printf("[%d] Ho letto %d bytes\nParole: %d\n", rank, chread, parole);
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
    for (i = 0; i <= size; i++) {
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

    // free(temp);
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

    strcpy(heap[0].word, heap[size].word);
    heap[0].occurrences = heap[size].occurrences;

    strcpy(heap[size].word, "");
    heap[size].occurrences = -1;

    // free(heap[size]);
    // heap[size].word = NULL;
    // printf("%s %ld\n", heap[size].word, heasp[size].occurrences);

    // BALANCE HEAP
    int leftChildIndex = 0;
    int rightChildIndex = 0;
    // printf("%s - %ld\n", maxChild -> word, maxChild -> occurrences);

    int pos = 0;
    while (pos <= size) {
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

void writeTree(struct BTreeNode *wordsTree, FILE *outputFile) {
    if (wordsTree != NULL) {
        writeTree(wordsTree -> left, outputFile);
        fprintf(outputFile, "%s,%ld\n", wordsTree -> word, wordsTree -> occurrences);
        writeTree(wordsTree -> right, outputFile);
    }
}

void createCSV(struct BTreeNode *wordsTree, long size, int rank) {
    if (size < 0) return;

    char fileName[MAX_FILE_NAME_LENGTH];
    sprintf(fileName, "files/output_%d.txt", rank);
    FILE *output = fopen(fileName, "w");
    fprintf(output, "WORD,COUNT\n");

    writeTree(wordsTree, output);

    fclose(output);
}

void printInfoArray(Item *wordsList, long size) {
    int currentIndex = 0;
    for (currentIndex = 0; currentIndex < size; currentIndex++) {
        printf("%s: %ld\n", wordsList[currentIndex].word, wordsList[currentIndex].occurrences);
    }
}

Item* initWordsListDisplsAndRecvCount(int *wordsListDispls, int *wordsListRecvCounts, int *wordsListSizes, long *size) {
    *size = 0;
    wordsListRecvCounts[0] = 0;
    wordsListDispls[0] = 0;
    // printf("Displs[%d] = %d\n", 0, displs[0]);
    int i;
    for (i = 1; i <= num_processes; i++) {
        wordsListRecvCounts[i] = wordsListSizes[i];
        wordsListDispls[i] = wordsListDispls[i - 1] + wordsListRecvCounts[i - 1];
        *size += wordsListSizes[i];
        // printf("wordsListRecvCounts[%d] = %d\n", i, wordsListRecvCounts[i]);
    }

    Item *recv = calloc(num_processes * *size, sizeof(Item));
    return recv;
}

struct BTreeNode* mergeOrderedLists(Item *receivedWordsList, int size) {
    int i;
    struct BTreeNode *avl;
    for (i = 0; i < size; i++) {
        avl = addToAVL(avl, receivedWordsList[i], compareByName);
    }

    return avl;
    // printf("%ld\n", receivedWordsList[0].occurrences);
    // printf("%ld\n", receivedWordsList[wordsListSizes[1]].occurrences);
    // printf("%ld\n", receivedWordsList[wordsListSizes[1] + wordsListSizes[2]].occurrences);
}

int main(int argc, char **argv) {
    int rank;

    MPI_Datatype subTaskType, itemType;
    MPI_Status status;

    SubTask subTask;
    Item *receivedData = malloc(sizeof(Item) * TASK_ARRAY_SIZE * num_processes);
    // Item wordsList[TASK_ARRAY_SIZE];
    Item *wordsList = calloc(TASK_ARRAY_SIZE, sizeof(Item));
    struct BTreeNode *avl;
    long size = TASK_ARRAY_SIZE;
    double startTime;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    num_processes -= 1;
    
    createSubTaskMPIStruct(&subTaskType);
    createItemMPIStruct(&itemType);

    Item *recv;

    if (rank == MASTER_PROCESS_ID) {
        // receivedData = malloc(sizeof(Item) * TASK_ARRAY_SIZE * num_processes);
        FileInfo *filesInfos = getFilesInfos();
        int totalBytes = getTotalBytesFromFiles(filesInfos, FILE_NUMBER);
        Task *taskArray = calloc(TASK_ARRAY_SIZE, sizeof(Task));
        long *taskArrayCurrentSize = malloc(1 * sizeof(long));
        long *arrayBytesPerProcess = getNumberOfElementsPerProcess(num_processes, totalBytes);
        printArray(arrayBytesPerProcess, 0, num_processes);
        divideFilesBetweenProcesses(taskArray, taskArrayCurrentSize, arrayBytesPerProcess, filesInfos, FILE_NUMBER);
        printf("%ld\n", *taskArrayCurrentSize);
        // printTaskArray(taskArray, *taskArrayCurrentSize);

        startTime = MPI_Wtime();

        scatterTasks(taskArray, *taskArrayCurrentSize, subTaskType);
        
    } else {
        int numerOfTasks = 0;
        MPI_Recv(&numerOfTasks, 1, MPI_INT, MASTER_PROCESS_ID, TAG, MPI_COMM_WORLD, &status);
        // printf("I'm the process number %d and I'm waiting for %d tasks\n", rank, numerOfTasks);
        int currentTask = 0;
        for (currentTask = 0; currentTask < numerOfTasks; currentTask++) {
            MPI_Recv(&subTask, 1, subTaskType, MASTER_PROCESS_ID, TAG, MPI_COMM_WORLD, &status);
            // printf("Processo [%d] I received %s - %ld - %ld\n", rank, subTask.fileName, subTask.startFromBytes, subTask.endToBytes);
            printf("[%d] Counting words...\n", rank);
            avl = countWords(&subTask, rank);
            printf("[%d] Word counted!\n", rank);
            printf("[%d] Sending data to master process...\n", rank);
            size = createArrayFromAVL(wordsList, avl, 0);
        }
    }

    int *wordsListSizes = calloc(num_processes + 1, sizeof(int));
    MPI_Gather(&size, 1, MPI_INT, &wordsListSizes[rank], 1, MPI_INT, MASTER_PROCESS_ID, MPI_COMM_WORLD);

    int wordsListRecvCounts[num_processes];
    
    int wordsListDispls[num_processes];
    if (rank == MASTER_PROCESS_ID) {
        recv = initWordsListDisplsAndRecvCount(wordsListDispls, wordsListRecvCounts, wordsListSizes, &size);
    }

    MPI_Gatherv(wordsList, size, itemType, recv, wordsListRecvCounts, wordsListDispls, itemType, MASTER_PROCESS_ID, MPI_COMM_WORLD);
    if (rank != MASTER_PROCESS_ID) {
        printf("[%d] Data sended!\n", rank);
    } else {
        struct BTreeNode *avl = mergeOrderedLists(recv, size);
        struct BTreeNode *orderedByOccurrencesAVL = NULL;
        orderedByOccurrencesAVL = orderAVLByOccurrences(avl, orderedByOccurrencesAVL);
        // inOrder(orderedByOccurrencesAVL, MASTER_PROCESS_ID);

        printf("[%d] Creating CSV...\n", rank);
        createCSV(orderedByOccurrencesAVL, size, rank);
        double endTime = MPI_Wtime();
        printf("[%d] CSV created!\n", rank);
        double totalTime = endTime - startTime;
        printf("[%d] Execution time: %f\n", rank, totalTime);
    }

    MPI_Finalize();
    return 0;
}