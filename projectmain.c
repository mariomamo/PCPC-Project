#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include <unistd.h>
#define NUM_PROCESSES 5
#define MAX_FILE_LIST_SIZE 100
#define MAX_FILE_NAME_LENGTH 255
#define FILE_NUMBER 5
#define TASK_ARRAY_SIZE 200000
#define TASK_ARRAY_SIZE_2 2
#define MIN(a,b) (((a)<(b))?(a):(b))

typedef struct {
    char fileName[MAX_FILE_NAME_LENGTH];
    int fileSize;
} FileInfo;

typedef struct {
    char fileName[MAX_FILE_NAME_LENGTH];
    long startFromBytes;
    long endToBytes;
    int size;
} SubTask;

typedef struct {
    SubTask *subTasks;
    int size;
} Task;

int getFileSize(char *fileName) {
    char subTask[MAX_FILE_NAME_LENGTH] = "";
    strcat(subTask, fileName);
    strcat(subTask, ".txt");
    FILE *fp = fopen(subTask, "r");
    fseek(fp, 0L, SEEK_END);
    return ftell(fp);
}

FileInfo* getFilesInfos() {
    char *fileNames[MAX_FILE_NAME_LENGTH] = {
        "files/610_parole_HP",
        "files/1000_parole_italiane",
        "files/6000_parole_italiane",
        "files/280000_parole_italiane",
        "files/test"
    };

    // char *fileNames[MAX_FILE_NAME_LENGTH] = {
    //     "files/test",
    //     "files/test2",
    //     "files/test3"
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
    return index < numFiles && processIndex < NUM_PROCESSES ? 1 : 0;
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

// long addSubTask(SubTask *subTaskArray, long subTaskArrayMaximumSize, SubTask newSubTask, int taskArrayIndex) {
//     // printf("Addo %s: %ld - %ld\n", newSubtask -> fileName, newSubtask -> startFromBytes, newSubtask -> endToBytes);
//     // printf("subTaskArrayMaximumSize: %ld, taskArrayIndex: %d\n", subTaskArrayMaximumSize, taskArrayIndex);
//     if (subTaskArrayMaximumSize <= taskArrayIndex) {
//         subTaskArrayMaximumSize += TASK_ARRAY_SIZE;
//         subTaskArray = realloc(subTaskArray, subTaskArrayMaximumSize * sizeof(SubTask));
//         // printf("REALLOC\n");
//         // printf("NOW IS subTaskArrayMaximumSize: %ld, taskArrayIndex: %d\n", subTaskArrayMaximumSize, taskArrayIndex);
//     }
//     SubTask f;
//     subTaskArray[taskArrayIndex] = newSubTask;
//     return subTaskArrayMaximumSize;
// }

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
    task -> subTasks = calloc(TASK_ARRAY_SIZE, sizeof(SubTask));
    task -> size = 0;
    return taskArrayCurrentSize + 1;
}

Task *divideFilesBetweenProcesses(Task *taskArray, long *taskArrayCurrentSize, FileInfo *filesInfos, int bytesNumber, int numProcesses, int numFiles) {
    *taskArrayCurrentSize = 0;
    long *arrayBytesPerProcess = getNumberOfElementsPerProcess(numProcesses, bytesNumber);
    printArray(arrayBytesPerProcess, 0, numProcesses);
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

int main(int argc, char **argv) {
    FileInfo *filesInfos = getFilesInfos();
    int totalBytes = getTotalBytesFromFiles(filesInfos, FILE_NUMBER);

    Task *taskArray = calloc(TASK_ARRAY_SIZE, sizeof(Task));
    long *taskArrayCurrentSize = malloc(1 * sizeof(long));
    divideFilesBetweenProcesses(taskArray, taskArrayCurrentSize, filesInfos, totalBytes, NUM_PROCESSES, FILE_NUMBER);
    // printf("%d\n", *taskArrayCurrentSize);
    printTaskArray(taskArray, *taskArrayCurrentSize);
    //  char *fileNames[MAX_FILE_NAME_LENGTH] = {
    //     "files/610_parole_HP",
    //     "files/1000_parole_italiane",
    //     "files/6000_parole_italiane",
    //     "files/280000_parole_italiane",
    //     "files/test",
    //     "files/test2"
    // };
    
    // SubTask *subTaskArray = calloc(sizeof(SubTask), TASK_ARRAY_SIZE);
    // int i = 0, size = 0, totalSize = 0;
    // for (i = 0; i < 6; i++) {
    //     SubTask subTask;
    //     strcpy(subTask.fileName, fileNames[i]);
    //     subTask.startFromBytes = 0;
    //     subTask.endToBytes = i;
    //     totalSize = addSubTask(subTaskArray, totalSize, subTask, size++);
    // }
    // printSubTaskArray(subTaskArray, size);
    return 0;
}
