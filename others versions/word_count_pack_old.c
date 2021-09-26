#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <mpi.h>
#define MAX_FILE_LIST_SIZE 100
#define MAX_FILE_NAME_LENGTH 255
#define MAX_WORD_SIZE 1024
#define TASK_ARRAY_SIZE 400000
#define MAX_HEAP_SIZE 4000
#define TASK_ARRAY_SIZE_2 1000
#define MASTER_PROCESS_ID 0
#define TAG 100
#define PACK_SIZE 1000000
#define MAX(a,b) (((a)>(b))?(a):(b))

int num_processes;
char message[1024];
int FILE_NUMBER;

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

void logMessage(char *message, int rank) {
    char msg[1024];
    sprintf(msg, "[Processo %d] >> %s", rank, message);
    write(1, &msg, strlen(msg)+1);
    fflush(stdin);
}

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

int getFileSize(char *fileName) {
    FILE *fp = fopen(fileName, "r");
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    fclose(fp);
    // free(fp);
    return size;
}

int readFileNamesFromFile(char fileNames[MAX_FILE_LIST_SIZE][MAX_FILE_NAME_LENGTH]) {
    FILE *input = fopen("input.txt", "r");
    char *line;
    size_t len = 0;
    int read = 0;
    int index = 0;
    while((read = getline(&line, &len, input)) != -1) {
        strcpy(fileNames[index], line);
        if (fileNames[index][read - 1] == '\n' || fileNames[index][read - 1] == '\r') {
            fileNames[index][read - 1] = '\0';
        }
        if (fileNames[index][read - 2] == '\n' || fileNames[index][read - 2] == '\r') {
            fileNames[index][read - 2] = '\0';
        }

        index++;
    }

    fclose(input);
    // free(input);
    return index;
}

FileInfo* getFilesInfos() {
    char fileNames[MAX_FILE_LIST_SIZE][MAX_FILE_NAME_LENGTH] = {""};
    FILE_NUMBER = readFileNamesFromFile(fileNames);

    FileInfo *filesInfos = malloc(FILE_NUMBER * sizeof(FileInfo));
    int i = 0;
    for (i = 0; i < FILE_NUMBER; i++) {
        FileInfo f;
        strcpy(f.fileName, fileNames[i]);
        f.fileSize = getFileSize(fileNames[i]);
        filesInfos[i] = f;
    }

    char message[2014];

    for (i = 0; i < FILE_NUMBER; i++) {
        sprintf(message, "+ [%s]\n|--+ SIZE: %d\n", filesInfos[i].fileName, filesInfos[i].fileSize);
        logMessage(message, MASTER_PROCESS_ID);
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
        sprintf(message, "THERE ARE TOO MANY PROCESSES\nPROCESSES: %d, TOTAL BYTES: %d\n", process_number, elements_number);
        logMessage(message, MASTER_PROCESS_ID);
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

void printArray(long *values, int my_rank, int size) {
    int i;
    char msg[50];

    for (i = 0; i < size; i++) {
        sprintf(msg, "########## values[%d]: %ld\n", i, values[i]);
        
    }
}

void printSubTaskArray(SubTask *values, int process, int size) {
    int i;
    char msg[1000];

    for (i = 0; i < size; i++) {
        sprintf(msg, ">>>>>> process[%d]: %s, %ld, %ld\n", process, values[i].fileName, values[i].startFromBytes, values[i].endToBytes);
        logMessage(msg, MASTER_PROCESS_ID);
    }
}

void printTaskArray(Task *values, long arraySize) {
    sprintf(message, "ARRAY SIZE: %ld\n", arraySize);
    logMessage(message, MASTER_PROCESS_ID);
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
    SubTask *tempArray = task -> subTasks;
    if (subTaskArrayMaximumSize <= task -> size) {
        subTaskArrayMaximumSize += TASK_ARRAY_SIZE_2;
        tempArray = realloc(tempArray, subTaskArrayMaximumSize * sizeof(SubTask));
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
    // TODO: CONTROLLARE SE FUNZIONA
    // SubTask *temp = task -> subTasks;
    task -> subTasks = realloc(task -> subTasks, task -> size * sizeof(SubTask));
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
            subTaskArrayMaximumSize = addSubTask(task, subTaskArrayMaximumSize, subTask);

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

            subTaskArrayMaximumSize = addSubTask(task, subTaskArrayMaximumSize, subTask);

            if (isFileEnded(sendedBytes, filesInfos[currentFileInfo], startOffset)) {
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
    int position = 0;
    char message[PACK_SIZE];
    MPI_Request *requests = calloc(num_processes * 2, sizeof(MPI_Request));
    for (taskIndex = 0; taskIndex < taskArrayCurrentSize; taskIndex++) {
        Task task = taskArray[taskIndex];
        long subTaskIndex = 0;
        position = 0;
        MPI_Pack(&task.size, 1, MPI_INT, message, PACK_SIZE, &position, MPI_COMM_WORLD);
        MPI_Pack(task.subTasks, task.size, subTaskType, message, PACK_SIZE, &position, MPI_COMM_WORLD);
        MPI_Isend(message, PACK_SIZE, MPI_PACKED, taskIndex + 1, TAG, MPI_COMM_WORLD, &requests[taskIndex]);
    }
    MPI_Status status;
    for (taskIndex = 0; taskIndex < taskArrayCurrentSize; taskIndex++) {
        MPI_Wait(&requests[taskIndex], &status);
    }
    free(requests);
}

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

    btree -> height = MAX(getTreeHeight(btree -> left), getTreeHeight(btree -> right)) + 1;

    int balance = getBalance(btree);

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

long createArrayFromAVL(Item *wordsList, struct BTreeNode *rootNode, long index) {
    if (rootNode != NULL) {
        index = createArrayFromAVL(wordsList, rootNode -> left, index);
        wordsList[index++] = *newItemWithValues(rootNode -> word, rootNode -> occurrences);
        return createArrayFromAVL(wordsList, rootNode -> right, index);
    }
    return index;
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
    return ch >= 65 && ch <= 90 || ch >= 97 && ch <= 122;
}

struct BTreeNode* countWords(SubTask *subTask, int rank) {
    rank = rank - 1;
    struct BTreeNode *btree = NULL;
    FILE *file = fopen(subTask -> fileName, "r");
    long start = subTask -> startFromBytes;
    int remainingBytesToRead = subTask -> endToBytes + 1 - subTask -> startFromBytes;
    int lineSize = 1000;
    int parole = 0;
    char c;
    int chread = 0;
    if (start != 0) {
        fseek(file, start-1, SEEK_SET);
        c = fgetc(file);
        if (c != EOF && c != '\n' && c != ' ') {
            while (remainingBytesToRead > 0 && (c = fgetc(file)) != EOF && c != '\n' && c != ' ') {
                remainingBytesToRead--;
                chread++;
            }
            remainingBytesToRead--;
        }
    }
    int readedBytes;
    char line[lineSize];
    while (remainingBytesToRead > 0) {
        readedBytes = 0;
        while (remainingBytesToRead > 0 && (c = fgetc(file)) != EOF && (c == ' ' || c == '\r' || c == '\n' || !isCharacter(c))) {
            remainingBytesToRead--;
            chread++;
        }
        if (remainingBytesToRead <= 0) break;
        while (c != EOF && c != ' ' && c != '\r' && c != '\n' && isCharacter(c)) {
            line[readedBytes++] = c;
            c = fgetc(file);
        }
        line[readedBytes] = 0;
        if (c != EOF) readedBytes++;
        remainingBytesToRead -= readedBytes;
        parole++;
        chread += readedBytes;

        toLowerString(line);
        btree = addToAVL(btree, *newItemWithValues(line, 1), compareByName);
    }
    fclose(file);
    // free(file);
    return btree;
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

void writeTree(struct BTreeNode *wordsTree, FILE *outputFile, long *wordsNumber) {
    if (wordsTree != NULL) {
        writeTree(wordsTree -> left, outputFile, wordsNumber);
        (*wordsNumber)++;
        fprintf(outputFile, "%s,%ld\n", wordsTree -> word, wordsTree -> occurrences);
        writeTree(wordsTree -> right, outputFile, wordsNumber);
    }
}

void createCSV(struct BTreeNode *wordsTree, long size, int rank, long *wordsNumber) {
    if (size < 0) return;

    char fileName[MAX_FILE_NAME_LENGTH];
    sprintf(fileName, "files/output.csv");
    FILE *output = fopen(fileName, "w");
    fprintf(output, "WORD,COUNT\n");

    writeTree(wordsTree, output, wordsNumber);

    fclose(output);
    // free(output);
}

Item* initWordsListDisplsAndRecvCount(int *wordsListDispls, int *wordsListRecvCounts, int *wordsListSizes, long *size) {
    *size = 0;
    wordsListRecvCounts[0] = 0;
    wordsListDispls[0] = 0;
    int i;
    for (i = 1; i <= num_processes; i++) {
        wordsListRecvCounts[i] = wordsListSizes[i];
        wordsListDispls[i] = wordsListDispls[i - 1] + wordsListRecvCounts[i - 1];
        *size += wordsListSizes[i];
    }

    Item *recv = calloc(*size, sizeof(Item));
    return recv;
}

struct BTreeNode* mergeOrderedLists(Item *receivedWordsList, int size) {
    int i;
    struct BTreeNode *avl;
    for (i = 0; i < size; i++) {
        avl = addToAVL(avl, receivedWordsList[i], compareByName);
    }

    return avl;
}

void processTasks(Task *task, SubTask *subTask, struct BTreeNode *avl, Item *wordsList, long *size, int *rank) {
    int currentTask;
    for (currentTask = 0; currentTask < task -> size; currentTask++) {
        *subTask = task -> subTasks[currentTask];
        strcpy(message, "Counting words...\n");
        logMessage(message, *rank);
        avl = countWords(subTask, *rank);
        *size = createArrayFromAVL(wordsList, avl, 0);
    }
}

int mergeData(struct BTreeNode *avl, long *size, double *startTime, int *rank) {
    struct BTreeNode *orderedByOccurrencesAVL = NULL;
    orderedByOccurrencesAVL = orderAVLByOccurrences(avl, orderedByOccurrencesAVL);
    free(avl);
    // inOrder(orderedByOccurrencesAVL, MASTER_PROCESS_ID);
    strcpy(message, "Creating CSV...\n");
    logMessage(message, *rank);
    long wordsNumber = 0;
    createCSV(orderedByOccurrencesAVL, *size, *rank, &wordsNumber);
    free(orderedByOccurrencesAVL);
    strcpy(message, "CSV created!\n");
    logMessage(message, *rank);
    return wordsNumber;
}

int main(int argc, char **argv) {
    int rank;

    MPI_Datatype subTaskType, itemType;
    MPI_Status status;

    SubTask subTask;
    Item *receivedData = malloc(sizeof(Item) * TASK_ARRAY_SIZE * num_processes);
    Item *wordsList = calloc(TASK_ARRAY_SIZE, sizeof(Item));
    struct BTreeNode *avl;
    long size = TASK_ARRAY_SIZE;
    double startTime;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (num_processes == 1) {
        strcpy(message, "THERE ARE TOO FEW PROCESSES! PLEASE RUN WITH 2 OR MORE PROCESSES\n");
        logMessage(message, rank);
        exit(0);
    }

    num_processes -= 1;
    
    createSubTaskMPIStruct(&subTaskType);
    createItemMPIStruct(&itemType);

    Item *recv;

    if (rank == MASTER_PROCESS_ID) {
        FileInfo *filesInfos = getFilesInfos();
        int totalBytes = getTotalBytesFromFiles(filesInfos, FILE_NUMBER);
        Task *taskArray = calloc(TASK_ARRAY_SIZE, sizeof(Task));
        long *taskArrayCurrentSize = malloc(1 * sizeof(long));
        long *arrayBytesPerProcess = getNumberOfElementsPerProcess(num_processes, totalBytes);
        printArray(arrayBytesPerProcess, 0, num_processes);
        divideFilesBetweenProcesses(taskArray, taskArrayCurrentSize, arrayBytesPerProcess, filesInfos, FILE_NUMBER);
        printTaskArray(taskArray, *taskArrayCurrentSize);

        startTime = MPI_Wtime();
        scatterTasks(taskArray, *taskArrayCurrentSize, subTaskType);
    } else {
        char message[PACK_SIZE];
        Task *task = newTask();
        int position = 0;
        MPI_Recv(message, PACK_SIZE, MPI_PACKED, MASTER_PROCESS_ID, TAG, MPI_COMM_WORLD, &status);
        MPI_Unpack(message, PACK_SIZE, &position, &task -> size, 1, MPI_INT, MPI_COMM_WORLD);
        task -> subTasks = calloc(task -> size, sizeof(SubTask));
        MPI_Unpack(message, PACK_SIZE, &position, task -> subTasks, task -> size, subTaskType, MPI_COMM_WORLD);
        processTasks(task, &subTask, avl, wordsList, &size, &rank);
        MPI_Type_free(&subTaskType);
        strcpy(message, "Word counted! Sending data to master process...\n");
        logMessage(message, rank);
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
        strcpy(message, "Data sended!\n");
        logMessage(message, rank);
    } else {
        avl = mergeOrderedLists(recv, size);
        free(recv);
        long wordsNumber = mergeData(avl, &size, &startTime, &rank);
        double endTime = MPI_Wtime();
        double totalTime = endTime - startTime;
        sprintf(message, "Processed %ld words in %f seconds\n", wordsNumber, totalTime);
        logMessage(message, rank);
    }

    MPI_Finalize();
    return 0;
}