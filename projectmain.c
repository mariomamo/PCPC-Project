#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include <unistd.h>
#define NUM_PROCESSES 3
#define MAX_FILE_LIST_SIZE 100
#define MAX_FILE_NAME_LENGTH 255
#define FILE_NUMBER 3
#define TASK_ARRAY_SIZE 2
#define MIN(a,b) (((a)<(b))?(a):(b))

typedef struct {
    char fileName[MAX_FILE_NAME_LENGTH];
    int fileSize;
} FileInfo;

typedef struct {
    char fileName[MAX_FILE_NAME_LENGTH];
    long startFromBytes;
    long endToBytes;
} FileInfoToRead;

typedef struct {
    FileInfoToRead *filesToRead[MAX_FILE_LIST_SIZE];
} Task;

int getFileSize(char *fileName) {
    char file[MAX_FILE_NAME_LENGTH] = "";
    strcat(file, fileName);
    strcat(file, ".txt");
    FILE *fp = fopen(file, "r");
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
        process_number = elements_number;
    }
    long resto = elements_number % process_number;
    long elements_per_process = elements_number / process_number;
    long *arrayOfElements = malloc(sizeof(int) * process_number);

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
        sprintf(msg, "values[%d]: %ld\n", i, values[i]);
        logMessage(msg, my_rank);
    }
}

int thereAreFilesToSplit(int index, int numFiles, int processIndex) {
    return index < numFiles && processIndex < NUM_PROCESSES ? 1 : 0;
}

int getRemainingBytesToSend(FileInfo fileInfo, int startOffset) {
    return fileInfo.fileSize - startOffset;
}

int fileSizeIsEqualToRemainingBytesToRead(FileInfo fileInfo, int startOffset, int remainingBytesToRead) {
    return fileInfo.fileSize - startOffset == remainingBytesToRead ? 1 : 0;
}

int canProcessReadWholeFile(FileInfo fileInfo, int startOffset, int remainingBytesToRead) {
    return getRemainingBytesToSend(fileInfo, startOffset) < remainingBytesToRead ? 1 : 0;
}

void setFileToReadInfo(FileInfoToRead *fileInfoToRead, FileInfo fileInfo, int startOffset, int endToBytes) {
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

long addTask(FileInfoToRead *taskArray, long taskArraySize, FileInfoToRead newTask, int taskArrayIndex) {
    printf("Addo %s\n", newTask.fileName);
    if (taskArraySize <= taskArrayIndex) {
        taskArraySize += TASK_ARRAY_SIZE;
        taskArray = realloc(taskArray, taskArraySize * sizeof(FileInfoToRead));
    }
    taskArray[taskArrayIndex] = newTask;
    return taskArraySize;
}

// void printFileInfoToReadArray(FileInfoToRead *fileInfoToRead, long fileInfoToReadArraySize) {
//     int i = 0;
//     for (i = 0; i < fileInfoToReadArraySize; i++) {
//         printf("FILE NAME: %s\n", fileInfoToRead[i].fileName);
//     }
// }

// void printTaskArray(Task* taskArray, long taskArraySize) {
//     int i = 0;
//     for (i = 0; i < taskArraySize; i++) {
//         printFileInfoToReadArray(taskArray[i].filesToRead, 1);
//     }
// }

void divideFilesBetweenProcesses(FileInfo *filesInfos, int bytesNumber, int numProcesses, int numFiles) {
    long taskArraySize = TASK_ARRAY_SIZE;
    Task *taskArray = calloc(sizeof(FileInfoToRead), TASK_ARRAY_SIZE);
    Task t;
    // printTaskArray(taskArray, taskArraySize);
    long *arrayBytesPerProcess = getNumberOfElementsPerProcess(numProcesses, bytesNumber);
    printArray(arrayBytesPerProcess, 0, numProcesses);
    // printf("Each process will read %ld bytes\n", bytesPerProcess);
    long remainingBytesToRead = arrayBytesPerProcess[0];
    int i = 0;
    long startOffset = 0;
    int process = 0;
    int sendedBytes = 0;
    long taskArrayIndex = 0;
    long FileInfoToReadArrayIndex = 0;
    
    while (thereAreFilesToSplit(i, numFiles, process) == 1) {
        FileInfoToRead fileInfoToRead;
        Task task;
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
            if (isFileEnded(sendedBytes, filesInfos[i], startOffset)) {
                // printf("FILE FINITO 3 \n");
                i++;
                // remainingBytesToRead = arrayBytesPerProcess[process];
            }
            startOffset = 0;
            printf("Processo %d -> {%s} - from %ld to %ld\n", process, fileInfoToRead.fileName, fileInfoToRead.startFromBytes, fileInfoToRead.endToBytes);
            process++;
            remainingBytesToRead = arrayBytesPerProcess[process];
            // taskArraySize = addTask(taskArray, taskArraySize, fileInfoToRead, taskArrayIndex++);
            // printf(">>> INVIATI %ld - %ld = %d\n", endToBytes, startFromBytes, sendedBytes);
            // printf(">>> REMAINING BYTES TO READ = %ld - %d = %ld\n", remainingBytesToRead, sendedBytes, remainingBytesToRead - sendedBytes);
        } else if (canProcessReadWholeFile(filesInfos[i], startOffset, remainingBytesToRead) == 1) {
            // CI VA TUTTO
            endToBytes = filesInfos[i].fileSize;
            setFileToReadInfo(&fileInfoToRead, filesInfos[i], startOffset, endToBytes);
            sendedBytes = getSendedBytes(endToBytes, startFromBytes);
            // SE IL FILE è TERMINATO
            printf("Processo %d -> {%s} - from %ld to %ld\n", process, fileInfoToRead.fileName, fileInfoToRead.startFromBytes, fileInfoToRead.endToBytes);
            // printf(">>> INVIATI %ld - %ld = %d\n", endToBytes, startFromBytes, sendedBytes);
            // printf(">>> REMAINING BYTES TO READ = %ld - %d = %ld\n", remainingBytesToRead, sendedBytes, remainingBytesToRead - sendedBytes);
            remainingBytesToRead -= sendedBytes;
            // printf(">>> REMAINING BYTES %ld\n\n", remainingBytesToRead);
            if (isFileEnded(sendedBytes, filesInfos[i], startOffset)) {
                // printf("FILE FINITO 1 \n");
                i++;
                startOffset = 0;
            }
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
            printf("Processo %d -> {%s} - from %ld to %ld\n", process, fileInfoToRead.fileName, fileInfoToRead.startFromBytes, fileInfoToRead.endToBytes);
            // printf(">>> [2] INVIATI %ld - %ld = %d\n", endToBytes, startFromBytes, sendedBytes);
            // printf(">>> [2] REMAINING BYTES %ld - %ld = %ld\n", remainingBytesToRead, sendedBytes, remainingBytesToRead - sendedBytes);
            // printf(">>> [2] REMAINING BYTES TO READ = %ld - %d = %ld\n", remainingBytesToRead, sendedBytes, remainingBytesToRead - sendedBytes);
            remainingBytesToRead -= sendedBytes;
            // printf(">>> [2] REMAINING BYTES %ld\n\n", remainingBytesToRead);
            if (isFileEnded(sendedBytes, filesInfos[i], startOffset)) {
                // printf("FILE FINITO 2: %d - %ld\n", inviati, filesInfos[i].fileSize - startOffset);
                i++;
                startOffset = 0;
            } else {
                startOffset = endToBytes;
            }
            process++;
            remainingBytesToRead = arrayBytesPerProcess[process];
            // taskArraySize = addTask(taskArray, taskArraySize, fileInfoToRead, taskArrayIndex++);
        }
        // printf("%d - %d\n", i < numFiles, process < NUM_PROCESSES);
        // task.filesToRead[FileInfoToReadArrayIndex++] = fileInfoToRead;
    }
    // printTaskArray(taskArray, taskArraySize);
    // task.startFromBytes = 0;
    // task.endToBytes = 100;
    printf("SIZE: %d\n", filesInfos[i].fileSize);
}

int main(int argc, char **argv) {
    FileInfo *filesInfos = getFilesInfos();
    int totalBytes = getTotalBytesFromFiles(filesInfos, FILE_NUMBER);


    printf("Total bytes: %d\n", totalBytes);

    divideFilesBetweenProcesses(filesInfos, totalBytes, NUM_PROCESSES, FILE_NUMBER);

    return 0;
}
