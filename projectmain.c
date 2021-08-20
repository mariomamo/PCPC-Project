#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#define NUM_PROCESSES 13
#define MAX_FILE_LIST_SIZE 100
#define MAX_FILE_NAME_LENGTH 255
#define FILE_NUMBER 5
#define MIN(a,b) (((a)<(b))?(a):(b))

typedef struct {
    char fileName[MAX_FILE_NAME_LENGTH];
    int fileSize;
} FileInfo;

typedef struct {
    char fileName[MAX_FILE_NAME_LENGTH];
    long startFromBytes;
    long endToBytes;
} FileToReadInfo;

int getFileSize(char *fileName) {
    char file[MAX_FILE_NAME_LENGTH] = "";
    strcat(file, fileName);
    strcat(file, ".txt");
    FILE *fp = fopen(file, "r");
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

// void divideFilesBetweenProcesses(FileInfo *filesInfos, int bytesNumber, int numProcesses, int numFiles) {
//     int bytesPerProcess = bytesNumber / numProcesses; // TODO: Attento al resto
//     printf("Each process will read %d bytes\n", bytesPerProcess);
//     FileToReadInfo fileToReadInfoList[MAX_FILE_LIST_SIZE];
//     int partialBytesPerProcess = 0;
//     int i = 0;
//     int process = 0;
//     int startFrom = 0;
//     int remaining = 0;
//     int end = 0;
//     while ((partialBytesPerProcess <= bytesPerProcess) && i <= numFiles && process < NUM_PROCESSES) {
//         printf("%d\n", (partialBytesPerProcess <= bytesPerProcess) && i <= numFiles && process < NUM_PROCESSES);
//         FileToReadInfo fileToReadInfo;
//         // printf(">>> %s\n", filesInfos[i].fileName);
//         strcpy(fileToReadInfo.fileName, filesInfos[i].fileName);
//         if (partialBytesPerProcess + (filesInfos[i].fileSize - startFrom) <= bytesPerProcess) {
//             printf("START FROM %d\n", startFrom);
//             partialBytesPerProcess += filesInfos[i].fileSize;
//             fileToReadInfo.startFromBytes = startFrom;
//             startFrom = 0;
//             fileToReadInfo.endToBytes = filesInfos[i].fileSize;
//             i++;
//             printf("%s CI VA TUTTO\n", fileToReadInfo.fileName);
//             printf("Process %d read from %ld to %ld\n", process, fileToReadInfo.startFromBytes, fileToReadInfo.endToBytes);
//         }
//         else {
//             fileToReadInfo.startFromBytes = startFrom;
//             startFrom = bytesPerProcess - partialBytesPerProcess + 1;
//             fileToReadInfo.endToBytes = bytesPerProcess - partialBytesPerProcess;
//             remaining = filesInfos[i].fileSize - fileToReadInfo.endToBytes;
//             printf(">>> REMAINING 1 <<< %d\n", remaining);
//             printf(">>> END TO BYTES <<< %ld\n", fileToReadInfo.endToBytes);
//             printf("%s CI VA SOLO %d\n", fileToReadInfo.fileName, bytesPerProcess - partialBytesPerProcess);
//             printf("%s RESTA %d\n", fileToReadInfo.fileName, remaining);
//             printf("Process %d read from %ld to %ld\n", process, fileToReadInfo.startFromBytes, fileToReadInfo.endToBytes);
//             if (partialBytesPerProcess == bytesPerProcess) {
//                 printf("PROCESS FULL\n");
//                 partialBytesPerProcess = 0;
//                 // printf(">>>> PARTIAL BYTES PER PROCESS = %d\n", partialBytesPerProcess);
//                 process++;
//             } else { // QUA NON ENTRA MAI
//                 printf(">>> NAME <<< %s\n", filesInfos[i].fileName);
//                 printf(">>> REMAINING 2 <<< %d\n", remaining);
//                 partialBytesPerProcess += (bytesPerProcess - partialBytesPerProcess);
//                 partialBytesPerProcess = partialBytesPerProcess % bytesPerProcess;
//                 printf(">>> NAME <<< %s\n", filesInfos[i].fileName);
//                 end++;
//                 if (end == 2) {
//                     i++;
//                 }
//                 if (remaining == 0) {
//                     i++; //QUANDO DEVO FARE i++? SEMPRE? NON CREDO
//                 }
//                 printf("===== %d - %d\n", partialBytesPerProcess, bytesPerProcess);
//                 printf("<<>>>> PARTIAL BYTES PER PROCESS = %d\n", partialBytesPerProcess);
//                 startFrom = 0;
//             }
//         }
//     }

void divideFilesBetweenProcesses(FileInfo *filesInfos, int bytesNumber, int numProcesses, int numFiles) {
    long bytesPerProcess = bytesNumber / numProcesses; // ATTENTO AL RESTO
    printf("Each process will read %ld bytes\n", bytesPerProcess);
    long remainingBytesToRead = bytesPerProcess;
    int i = 0;
    long startOffset = 0;
    int process = 0;
    
    FileToReadInfo fileToReadInfo;
    while (i < numFiles && process < NUM_PROCESSES) {
        long startFromBytes;
        long endToBytes;
        if ((filesInfos[i].fileSize - startOffset) < remainingBytesToRead) {
            // CI VA TUTTO
            startFromBytes = startOffset;
            endToBytes = filesInfos[i].fileSize;
            strcpy(fileToReadInfo.fileName, filesInfos[i].fileName);
            fileToReadInfo.startFromBytes = startFromBytes;
            fileToReadInfo.endToBytes = endToBytes - 1;
            int inviati = endToBytes - startFromBytes;
            remainingBytesToRead -= inviati;
            // SE IL FILE è TERMINATO
            if (inviati == (filesInfos[i].fileSize - startOffset)) {
                // printf("FILE FINITO 1 \n");
                i++;
                startOffset = 0;
            }
            printf("Processo %d -> {%s} - from %ld to %ld\n", process, fileToReadInfo.fileName, fileToReadInfo.startFromBytes, fileToReadInfo.endToBytes);
            // printf(">>> INVIATI %ld - %ld = %d\n", endToBytes, startFromBytes, inviati);
            // printf("\n>>> REMAINING BYTES %ld - %ld = %ld\n", remainingBytesToRead, inviati, r§emainingBytesToRead - inviati);
            // printf(">>> REMAINING BYTES %ld\n\n", remainingBytesToRead);
        } else if (filesInfos[i].fileSize > remainingBytesToRead) {
            // QUA RESTA IN LOOP
            // printf("FILE SIZE %d, REMAINING %ld\n", filesInfos[i].fileSize, remainingBytesToRead);
            // CI VA SOLO UNA PARTE
            startFromBytes = startOffset;
            // printf("\n>>> END TO BYTES %ld + %ld = %ld\n", remainingBytesToRead, startOffset, remainingBytesToRead + startOffset);
            endToBytes = remainingBytesToRead + startOffset;
            strcpy(fileToReadInfo.fileName, filesInfos[i].fileName);
            fileToReadInfo.startFromBytes = startFromBytes;
            fileToReadInfo.endToBytes = endToBytes - 1;
            // startOffset = filesInfos[i].fileSize - remainingBytesToRead;
            int inviati = endToBytes - startFromBytes;
            remainingBytesToRead -= inviati;
            // SE IL FILE è TERMINATO
            // SE IL FILE è TERMINATO
            if (inviati == (filesInfos[i].fileSize - startOffset)) {
                // printf("FILE FINITO 2: %d - %d\n", inviati, filesInfos[i].fileSize - startOffset);
                i++;
                startOffset = 0;
            } else {
                startOffset = endToBytes;
            }
            remainingBytesToRead = bytesPerProcess;
            printf("Processo %d -> {%s} - from %ld to %ld\n", process, fileToReadInfo.fileName, fileToReadInfo.startFromBytes, fileToReadInfo.endToBytes);
            // printf(">>> [2] INVIATI %ld - %ld = %d\n", endToBytes, startFromBytes, inviati);
            // printf(">>> [2] REMAINING BYTES %ld - %ld = %ld\n", remainingBytesToRead, inviati, remainingBytesToRead - inviati);
            // printf(">>> [2] REMAINING BYTES %ld\n\n", remainingBytesToRead);
            process++;
        } else if (filesInfos[i].fileSize == remainingBytesToRead) {
            startFromBytes = startOffset;
            endToBytes = remainingBytesToRead;
            strcpy(fileToReadInfo.fileName, filesInfos[i].fileName);
            fileToReadInfo.startFromBytes = startOffset;
            fileToReadInfo.endToBytes = endToBytes - 1;
            int inviati = endToBytes - startFromBytes;
            remainingBytesToRead -= inviati;
            // SE IL FILE è TERMINATO
            if (inviati == (filesInfos[i].fileSize - startOffset)) {
                // printf("FILE FINITO 3 \n");
                i++;
            }
            remainingBytesToRead = bytesPerProcess;
            printf("Processo %d -> {%s} - from %ld to %ld\n", process, fileToReadInfo.fileName, fileToReadInfo.startFromBytes, fileToReadInfo.endToBytes);
        }
        // printf("%d - %d\n", i < numFiles, process < NUM_PROCESSES);
    }
    fileToReadInfo.startFromBytes = 0;
    fileToReadInfo.endToBytes = 100;
    printf("SIZE: %d\n", filesInfos[i].fileSize);
}

int main(int argc, char **argv) {
    FileInfo *filesInfos = getFilesInfos();
    int totalBytes = getTotalBytesFromFiles(filesInfos, FILE_NUMBER);

    printf("Total bytes: %d\n", totalBytes);

    divideFilesBetweenProcesses(filesInfos, totalBytes, NUM_PROCESSES, FILE_NUMBER);

    return 0;
}
