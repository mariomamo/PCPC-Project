#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#define MASTER_PROCESS_ID 0

int num_processes;

int main(int argc, char **argv) {
    int rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    num_processes -= 1;


    if (rank == MASTER_PROCESS_ID){
        // FileInfo *filesInfos = getFilesInfos();
        // int totalBytes = getTotalBytesFromFiles(filesInfos, FILE_NUMBER);
        // Task *taskArray = calloc(TASK_ARRAY_SIZE, sizeof(Task));
        // long *taskArrayCurrentSize = malloc(1 * sizeof(long));
        // long *arrayBytesPerProcess = getNumberOfElementsPerProcess(num_processes, totalBytes);
        // printArray(arrayBytesPerProcess, 0, num_processes);
        // divideFilesBetweenProcesses(taskArray, taskArrayCurrentSize, arrayBytesPerProcess, filesInfos, FILE_NUMBER);
        // printf("%ld\n", *taskArrayCurrentSize);
        // printTaskArray(taskArray, *taskArrayCurrentSize);
        printf("Cià o biò\n");

        // scatterTasks(taskArray, *taskArrayCurrentSize, subTaskType);

        // char c[100] = {'À', 'Á', 'Â', 'Ã', 'Ä', 'Å', 'Æ', 'Ç', 'È', 'É', 'Ê', 'Ë', 'Ì', 'Í', 'Î', 'Ï', 'Ð', 'Ñ', 'Ò', 'Ó', 'Ô', 'Õ', 'Ö', 'Ø','Ù', 'Ú', 'Û', 'Ü', 'Ý', 'Þ', 'ß', 'à', 'á', 'â', 'ã', 'ä', 'å', 'æ', 'ç', 'è', 'é', 'ê', 'ë', 'ì', 'í', 'î', 'ï', 'ð', 'ñ', 'ò', 'ó', 'ô', 'õ', 'ö', 'ø', 'ù', 'ú', 'û', 'ü', 'ý', 'þ', 'ÿ', 0};
        // int i = 0;
        // while (c[i] != 0)
        // printf("%c = %d\n", c[i], c[i++]);

        // MPI_Gather(wordsList, TASK_ARRAY_SIZE, itemType, wordsList, TASK_ARRAY_SIZE, itemType, MASTER_PROCESS_ID, MPI_COMM_WORLD);
        
    } else {
        int numerOfTasks = 0;
        // MPI_Recv(&numerOfTasks, 1, MPI_INT, MASTER_PROCESS_ID, TAG, MPI_COMM_WORLD, &status);
        printf("Processo [%d] cià statti buon\n", rank);
        // printf("I'm the process number %d and I'm waiting for %d tasks\n", rank, numerOfTasks);
        int currentTask = 0;
        for (currentTask = 0; currentTask < numerOfTasks; currentTask++) {
            // MPI_Recv(&subTask, 1, subTaskType, MASTER_PROCESS_ID, TAG, MPI_COMM_WORLD, &status);
            // printf("Processo [%d] I received %s - % ld - %ld\n", rank, subTask.fileName, subTask.startFromBytes, subTask.endToBytes);
        }
    }

    MPI_Finalize();
    return 0;
}