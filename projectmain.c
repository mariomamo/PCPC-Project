#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include <unistd.h>
#define NUM_PROCESSES 300
#define MAX_FILE_LIST_SIZE 100
#define MAX_FILE_NAME_LENGTH 255
#define FILE_NUMBER 3
#define TASK_ARRAY_SIZE 200000
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
    // char *fileNames[MAX_FILE_NAME_LENGTH] = {
    //     "files/610_parole_HP",
    //     "files/1000_parole_italiane",
    //     "files/6000_parole_italiane",
    //     "files/280000_parole_italiane",
    //     "files/test"
    // };

    char *fileNames[MAX_FILE_NAME_LENGTH] = {
        "files/test",
        "files/test2",
        "files/test3"
    };

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


// int getWordsNumber(int fileSizeInByte) {
//     int wordSizeAverage = 7;
//     int wordNumberWithCarriage = (fileSizeInByte / wordSizeAverage);
//     int wordNumber = (fileSizeInByte - (wordNumberWithCarriage * 2)) / wordSizeAverage;
//     // Add rest of subdivision
//     wordNumber += ((fileSizeInByte - (wordNumberWithCarriage * 2)) % wordSizeAverage != 0);
//     return wordNumber;
// }

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
        // Errore divisione
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
    // printf("%d - %ld\n", values[0].size, arraySize);
    int i;
    char msg[1000];
    for (int i = 0; i < arraySize; i++) {
        // printf("%d\n", values[i].size);
        printSubTaskArray(values[i].subTasks, i, values[i].size);
    }
}

int thereAreFilesToSplit(int index, int numFiles, int processIndex) {
    return index < numFiles && processIndex < NUM_PROCESSES ? 1 : 0;
}

int fileSizeIsEqualToRemainingBytesToRead(FileInfo fileInfo, int startOffset, int remainingBytesToRead) {
    return fileInfo.fileSize - startOffset == remainingBytesToRead ? 1 : 0;
}

int getRemainingBytesToSend(FileInfo fileInfo, int startOffset) {
    return fileInfo.fileSize - startOffset;
}

int canProcessReadWholeFile(FileInfo fileInfo, int startOffset, int remainingBytesToRead) {
    return getRemainingBytesToSend(fileInfo, startOffset) < remainingBytesToRead ? 1 : 0;
}

void setFileToReadInfo(SubTask *fileInfoToRead, FileInfo fileInfo, int startOffset, int endToBytes) {
    strcpy(fileInfoToRead -> fileName, fileInfo.fileName);
    fileInfoToRead -> startFromBytes = startOffset;
    fileInfoToRead -> endToBytes = endToBytes - 1;
}

int isFileEnded(int sendedBytes, FileInfo fileInfo, int startOffset) {
    return sendedBytes == (fileInfo.fileSize - startOffset) ? 1 : 0;
}

int getSendedBytes(int endToBytes, int startFromBytes) {
    return endToBytes - startFromBytes;
}

long addSubTask(SubTask *subTaskArray, long subTaskArrayMaximumSize, SubTask newSubTask, int taskArrayIndex) {
    // printf("Addo %s: %ld - %ld\n", newSubtask -> fileName, newSubtask -> startFromBytes, newSubtask -> endToBytes);
    // printf("subTaskArrayMaximumSize: %ld, taskArrayIndex: %d\n", subTaskArrayMaximumSize, taskArrayIndex);
    if (subTaskArrayMaximumSize <= taskArrayIndex) {
        subTaskArrayMaximumSize += TASK_ARRAY_SIZE;
        subTaskArray = realloc(subTaskArray, subTaskArrayMaximumSize * sizeof(SubTask));
        // printf("REALLOC\n");
        // printf("NOW IS subTaskArrayMaximumSize: %ld, taskArrayIndex: %d\n", subTaskArrayMaximumSize, taskArrayIndex);
    }
    SubTask f;
    subTaskArray[taskArrayIndex] = newSubTask;
    return subTaskArrayMaximumSize;
}

Task* newTask() {
    Task *task = malloc(1 * sizeof(Task));
    task -> subTasks = malloc(MAX_FILE_LIST_SIZE * sizeof(SubTask));
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

// void printFileInfoToReadArray(SubTask *fileInfoToRead, long fileInfoToReadArraySize) {
//     int i = 0;
//     for (i = 0; i < fileInfoToReadArraySize; i++) {
//         printf("FILE NAME: %s\n", fileInfoToRead[i].fileName);
//     }
// }

// void printTaskArray(Task* subTaskArray, long subTaskArrayMaximumSize) {
//     int i = 0;
//     for (i = 0; i < subTaskArrayMaximumSize; i++) {
//         printFileInfoToReadArray(subTaskArray[i].subTasks, 1);
//     }
// }

void divideFilesBetweenProcesses(FileInfo *filesInfos, int bytesNumber, int numProcesses, int numFiles) {
    // long subTaskArrayMaximumSize = TASK_ARRAY_SIZE;
    // long subTaskArrayCurrentSize = 0;
    // SubTask *subTaskArray = calloc(TASK_ARRAY_SIZE, sizeof(SubTask));
    Task *taskArray = calloc(TASK_ARRAY_SIZE, sizeof(Task));
    long taskArrayCurrentSize = 0;
    // printTaskArray(subTaskArray, subTaskArrayMaximumSize);
    long *arrayBytesPerProcess = getNumberOfElementsPerProcess(numProcesses, bytesNumber);
    printArray(arrayBytesPerProcess, 0, numProcesses);
    // printf("Each process will read %ld bytes\n", bytesPerProcess);
    long remainingBytesToRead = arrayBytesPerProcess[0];
    int i = 0;
    long startOffset = 0;
    int process = 0;
    int sendedBytes = 0;
    long FileInfoToReadArrayIndex = 0;
    // Task *task = newTask();
    Task *task = newTask();
    
    while (thereAreFilesToSplit(i, numFiles, process) == 1) {
        SubTask fileInfoToRead;
        long startFromBytes;
        long endToBytes;
        startFromBytes = startOffset;
        if (fileSizeIsEqualToRemainingBytesToRead(filesInfos[i], startOffset, remainingBytesToRead)) {
            // NON SO SE CI VUOLE - START OFFSET
            // printf("File info size: %d - Start offset: %d - Remaining bytes to read: %d\n", filesInfos[i].fileSize, startOffset, remainingBytesToRead);
            endToBytes = startOffset + remainingBytesToRead;
            setFileToReadInfo(&fileInfoToRead, filesInfos[i], startOffset, endToBytes);
            sendedBytes = getSendedBytes(endToBytes, startFromBytes);
            remainingBytesToRead -= sendedBytes;
            // SE IL FILE è TERMINATO
            // subTaskArrayMaximumSize = addSubTask(subTaskArray, subTaskArrayMaximumSize, fileInfoToRead, subTaskArrayCurrentSize++);
            // printf("MMH [3] :%s\n", subTaskArray[subTaskArrayCurrentSize - 1].fileName);
            
            // SubTask subTask;
            // strcpy(subtask -> fileName, fileInfoToRead.fileName);
            // subtask -> startFromBytes = fileInfoToRead.startFromBytes;
            // subtask -> endToBytes = subtask -> endToBytes;
            // printf(">> TASK-SIZE: %d\n", task -> size);
            task -> subTasks[task -> size++] = *newSubTask(fileInfoToRead.fileName, fileInfoToRead.startFromBytes, fileInfoToRead.endToBytes);
            // printSubTaskArray(task -> subTasks, process, task -> size);
            // printf("FILE MEM: %p\n", &subTask);
            // int i;
            // char msg[1000];
            // for (i = 0; i < task -> size; i++) {
            //     sprintf(msg, "[>>>>>> process[%d]: %s, %ld, %ld\n", process, task -> subTasks[i].fileName, task -> subTasks[i].startFromBytes, task -> subTasks[i].endToBytes);
            //     write(1, &msg, strlen(msg)+1);
            // }
            // i++;
            process++;

            if (isFileEnded(sendedBytes, filesInfos[i], startOffset)) {
                // FORSE L'IF SI PUO' TOGLIERE
                // printf("FILE FINITO 3 \n");
                i++;
                // subTaskArrayMaximumSize = TASK_ARRAY_SIZE;
                // subTaskArrayCurrentSize = 0;
                // Task task = newTask();
                remainingBytesToRead = arrayBytesPerProcess[process];
            }
            startOffset = 0;
            // printf("Processo [3] %d -> {%s} - from %ld to %ld\n", process, fileInfoToRead.fileName, fileInfoToRead.startFromBytes, fileInfoToRead.endToBytes);
            // remainingBytesToRead = arrayBytesPerProcess[process];
            // printf("[3] MO CANCELLO TUTTO\n");
            // printSubTaskArray(task -> subTasks, process, task -> size);
            // subTaskArrayCurrentSize = 0;
            // subTaskArray = calloc(TASK_ARRAY_SIZE, sizeof(SubTask));
            // subTaskArrayMaximumSize = TASK_ARRAY_SIZE;

            // task -> subTasks[task -> size] = subTaskArray;
            // task -> size++;
            taskArray[taskArrayCurrentSize] = *task;
            taskArrayCurrentSize++;
            // task = newTask();
            task -> subTasks = calloc(TASK_ARRAY_SIZE, sizeof(SubTask));
            task -> size = 0;

            // printf("NEW SIZE: %d\n", task -> size);
            // subTaskArrayMaximumSize = addSubTask(subTaskArray, subTaskArrayMaximumSize, fileInfoToRead, taskArrayIndex++);
            // printf(">>> INVIATI %ld - %ld = %d\n", endToBytes, startFromBytes, sendedBytes);
            // printf(">>> REMAINING BYTES TO READ = %ld - %d = %ld\n", remainingBytesToRead, sendedBytes, remainingBytesToRead - sendedBytes);
        } else if (canProcessReadWholeFile(filesInfos[i], startOffset, remainingBytesToRead) == 1) {
            // CI VA TUTTO
            endToBytes = filesInfos[i].fileSize;
            setFileToReadInfo(&fileInfoToRead, filesInfos[i], startOffset, endToBytes);
            sendedBytes = getSendedBytes(endToBytes, startFromBytes);
            // SE IL FILE è TERMINATO
            // printf("Processo [1] %d -> {%s} - from %ld to %ld\n", process, fileInfoToRead.fileName, fileInfoToRead.startFromBytes, fileInfoToRead.endToBytes);
            // printf(">>> INVIATI %ld - %ld = %d\n", endToBytes, startFromBytes, sendedBytes);
            // printf(">>> REMAINING BYTES TO READ = %ld - %d = %ld\n", remainingBytesToRead, sendedBytes, remainingBytesToRead - sendedBytes);
            remainingBytesToRead -= sendedBytes;
            // printf(">>> REMAINING BYTES %ld\n\n", remainingBytesToRead);
            // subTaskArrayMaximumSize = addSubTask(subTaskArray, subTaskArrayMaximumSize, fileInfoToRead, subTaskArrayCurrentSize++);
            // printf("MMH [1] :%s\n", subTaskArray[subTaskArrayCurrentSize - 1].fileName);
            // printf(">> TASK-SIZE: %d\n", task -> size);
            task -> subTasks[task -> size++] = *newSubTask(fileInfoToRead.fileName, fileInfoToRead.startFromBytes, fileInfoToRead.endToBytes);

            if (isFileEnded(sendedBytes, filesInfos[i], startOffset)) {
                // FORSE QUESTO SI PUO' TOGLIERE
                // printf("FILE FINITO 1, CI VA TUTTO\n");
                i++;
                startOffset = 0;
                // subTaskArrayCurrentSize = 0;
                // subTaskArray = calloc(TASK_ARRAY_SIZE, sizeof(SubTask));
                // subTaskArrayMaximumSize = TASK_ARRAY_SIZE;
                // subTaskArrayMaximumSize = TASK_ARRAY_SIZE;
            }

            if (remainingBytesToRead == 0) {
                // printf("[1] MO CANCELLO TUTTO\n");
                // printSubTaskArray(subTaskArray, process, subTaskArrayCurrentSize);
                // printSubTaskArray(task -> subTasks, process, task -> size);
                // int i;
                // char msg[1000];
                // for (i = 0; i < task -> size; i++) {
                //     sprintf(msg, "[>>>>>> process[%d]: %s, %ld, %ld\n", process, task -> subTasks[i].fileName, task -> subTasks[i].startFromBytes, task -> subTasks[i].endToBytes);
                //     write(1, &msg, strlen(msg)+1);
                // }
                // task -> subTasks[task -> size] = subTaskArray;
                // task -> size++;
                taskArray[taskArrayCurrentSize] = *task;
                taskArrayCurrentSize++;
                // task = newTask();
                task -> subTasks = calloc(TASK_ARRAY_SIZE, sizeof(SubTask));
                task -> size = 0;
            }

            // printf("NEW SIZE: %d\n", task -> size);
            // printf("\n>>> REMAINING BYTES %ld - %ld = %ld\n", remainingBytesToRead, inviati, r§emainingBytesToRead - inviati);
        } else {
            // NON SO SE CI VUOLE - START OFFSET
            // printf("FILE SIZE %d, REMAINING %ld\n", filesInfos[i].fileSize, remainingBytesToRead);
            // CI VA SOLO UNA PARTE
            // printf("\n>>> END TO BYTES %ld + %ld = %ld\n", remainingBytesToRead, startOffset, remainingBytesToRead + startOffset);
            endToBytes = startOffset + remainingBytesToRead;
            setFileToReadInfo(&fileInfoToRead, filesInfos[i], startOffset, endToBytes);
            // startOffset = filesInfos[i].fileSize - remainingBytesToRead;
            sendedBytes = getSendedBytes(endToBytes, startFromBytes);
            // printf("\n>>> REMAINING BYTES TO READ = %ld - BYTES PER PROCESS = %ld - START OFFSET %ld\n", remainingBytesToRead, arrayBytesPerProcess[i], startOffset);
            // printf("Processo [2] %d -> {%s} - from %ld to %ld\n", process, fileInfoToRead.fileName, fileInfoToRead.startFromBytes, fileInfoToRead.endToBytes);
            // printf(">>> [2] INVIATI %ld - %ld = %d\n", endToBytes, startFromBytes, sendedBytes);
            // printf(">>> [2] REMAINING BYTES TO READ = %ld - %d = %ld\n", remainingBytesToRead, sendedBytes, remainingBytesToRead - sendedBytes);
            remainingBytesToRead -= sendedBytes;
            // printf(">>> [2] REMAINING BYTES %ld\n\n", remainingBytesToRead);
            // printf("PRE-SIZEONA: %ld\n", subTaskArrayCurrentSize);
            // printSubTaskArray(subTaskArray, subTaskArrayCurrentSize);
            // subTaskArrayMaximumSize = addSubTask(subTaskArray, subTaskArrayMaximumSize, fileInfoToRead, subTaskArrayCurrentSize++);
            // printf("POST-SIZEONA: %ld\n", subTaskArrayCurrentSize);
            // printf("MMH [2] :%s\n", subTaskArray[0].fileName);
            // printf("MMH [2] :%s\n", subTaskArray[1].fileName);
            // printf("MMH [2] :%s\n", subTaskArray[2].fileName);
            // printf(">> TASK-SIZE: %d\n", task -> size);
            task -> subTasks[task -> size++] = *newSubTask(fileInfoToRead.fileName, fileInfoToRead.startFromBytes, fileInfoToRead.endToBytes);

            if (isFileEnded(sendedBytes, filesInfos[i], startOffset)) {
                // printf("FILE FINITO 2: %d - %ld\n", sendedBytes, filesInfos[i].fileSize - startOffset);
                i++;
                startOffset = 0;
                // subTaskArrayCurrentSize = 0;
                // subTaskArray = calloc(TASK_ARRAY_SIZE, sizeof(SubTask));
                // subTaskArrayMaximumSizse = TASK_ARRAY_SIZE;
                // subTaskArrayMaximumSize = TASK_ARRAY_SIZE;
            } else {
                startOffset = endToBytes;
            }

            if (remainingBytesToRead == 0) {
                // printf("[2] MO CANCELLO TUTTO\n");
                // printSubTaskArray(subTaskArray, process, subTaskArrayCurrentSize);
                // printSubTaskArray(task -> subTasks, process, task -> size);
                // int i;
                // char msg[1000];
                // for (i = 0; i < task -> size; i++) {
                //     sprintf(msg, "[>>>>>> process[%d]: %s, %ld, %ld\n", process, task -> subTasks[i].fileName, task -> subTasks[i].startFromBytes, task -> subTasks[i].endToBytes);
                //     write(1, &msg, strlen(msg)+1);
                // }
                // task -> subTasks[task -> size] = subTaskArray;
                // task -> size++;
                taskArray[taskArrayCurrentSize] = *task;
                taskArrayCurrentSize++;
                // task = newTask();
                task -> subTasks = calloc(TASK_ARRAY_SIZE, sizeof(SubTask));
                task -> size = 0;
            }

            process++;
            remainingBytesToRead = arrayBytesPerProcess[process];
            // printf(">>> REMAINING BYTES TO READ arrayBytesPerProcess[%d] = %ld\n", process, remainingBytesToRead);
            // subTaskArrayMaximumSize = addSubTask(subTaskArray, subTaskArrayMaximumSize, fileInfoToRead, taskArrayIndex++);
        }
        // printf("%d - %d\n", i < numFiles, process < NUM_PROCESSES);
        // task -> subTasks[FileInfoToReadArrayIndex++] = fileInfoToRead;
    }
    // printTaskArray(subTaskArray, subTaskArrayMaximumSize);
    // task -> startFromBytes = 0;
    // task -> endToBytes = 100;
    // printf("SIZE: %d\n", filesInfos[i].fileSize);

    printTaskArray(taskArray, taskArrayCurrentSize);
}

int main(int argc, char **argv) {
    FileInfo *filesInfos = getFilesInfos();
    int totalBytes = getTotalBytesFromFiles(filesInfos, FILE_NUMBER);


    // printf("Total bytes: %d\n", totalBytes);

    divideFilesBetweenProcesses(filesInfos, totalBytes, NUM_PROCESSES, FILE_NUMBER);
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
    //     SubTask fileInfoToRead;
    //     strcpy(fileInfoToRead.fileName, fileNames[i]);
    //     fileInfoToRead.startFromBytes = 0;
    //     fileInfoToRead.endToBytes = i;
    //     totalSize = addSubTask(subTaskArray, totalSize, fileInfoToRead, size++);
    // }
    // printSubTaskArray(subTaskArray, size);



    return 0;
}
