/**
 * readerWriter.c
 * --------------
 * The canonical consumer-producer example. This version has hust one reader
 * and just one writer (although it could be generalized to multiple readers/writers)
 * communicating information through a shared buffer. There are two generalized semaphores
 * used, one to track the num of empty buffers, another to track full buffers. Each is used
 * to count, as well as control access.
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#define NUM_TOTAL_BUFFERS 5
#define DATA_LENGTH 20

typedef struct {
    const char* name;
    char* sharedBuffer;
    sem_t* pEmptyBuffers;
    sem_t* pFullBuffers;
    struct random_data* randBuffer;
} threadData;

static void* Writer(void* writerData);
static void* Reader(void* readerData);
static void ProcessData(void* readerData);
static char PrepareData(void* writerData);

/**
 * Initially, all buffers are empty, so our empty buffer semaphore starts
 * with a count equal to the total number of buffers, while our full buffer
 * semaphore begins at zero. We create two threads: one to read and one
 * to write, and then start them off running. They will finish after all data
 * has been written and read.
 */

void main(int argc, char **argv)
{
    pthread_t writer, reader;
    sem_t emptyBuffers, fullBuffers;
    char buffers[NUM_TOTAL_BUFFERS];
    int rc;
    struct random_data* randStates;
    char* randStateBuffers;
    void* status;

    randStates = (struct random_data*) calloc(2, sizeof(struct random_data));
    randStateBuffers = (char*) calloc(2, 32);
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    sem_init(&emptyBuffers, 0, NUM_TOTAL_BUFFERS);
    sem_init(&fullBuffers, 0, 0);

    initstate_r(random(), randStateBuffers, 16, randStates);
    initstate_r(random(), randStateBuffers + 1, 16, randStates + 1);

    threadData writerData = {"Writer", buffers, &emptyBuffers, &fullBuffers, randStates};
    threadData readerData = {"Reader", buffers, &emptyBuffers, &fullBuffers, randStates + 1};
    
    rc = pthread_create(&writer, NULL, Writer, (void*) &writerData);
    if (rc != 0) {
        printf("ERROR: pthread_create(&writer,...) failed with return code of %d\n.", rc);
        exit(1);
    }
    rc = pthread_create(&reader, NULL, Reader, (void*) &readerData);
    if (rc != 0) {
        printf("ERROR: pthread_create(&reader,...) failed with return code of %d\n.", rc);
        exit(1);
    }

    pthread_attr_destroy(&attr);

    rc = pthread_join(writer, &status);
    if (rc != 0) {
        printf("ERROR: pthread_join(&writer,...) failed with return code of %d\n.", rc);
        exit(1);
    }
    rc = pthread_join(reader, &status);
    if (rc != 0) {
        printf("ERROR: pthread_join(&reader,...) failed with return code of %d\n.", rc);
        exit(1);
    }

    sem_destroy(&emptyBuffers);
    sem_destroy(&fullBuffers);
    free(randStateBuffers);
    free(randStates);
    printf("All Done!\n");
}

/**
 * Writer
 * ------
 * This is the routine forked by the Writer thread. It will loop until
 * all data is written. It prepares the data to be written, the waits
 * for an empty buffer to be available to write the data to, after which
 * it signals that a full buffer is ready.
 */

static void* Writer(void* writerData)
{
    int i, writePt;
    char dataToWrite;

    threadData* data = (threadData*) writerData;

    for (i = 0; i < DATA_LENGTH; i++) {
        dataToWrite = PrepareData(writerData);
        sem_wait(data->pEmptyBuffers);
        data->sharedBuffer[writePt] = dataToWrite;
        printf("%s: buffer[%d] = %c\n", data->name, writePt, dataToWrite);
        writePt = (writePt + 1) % NUM_TOTAL_BUFFERS;
        sem_post(data->pFullBuffers);
    }

    pthread_exit((void*) writerData);
}

static void* Reader(void* readerData)
{
    int i, readPt = 0;
    char dataToRead;

    threadData* data = (threadData*) readerData;

    for (i = 0; i < DATA_LENGTH; i++) {
        sem_wait(data->pFullBuffers);
        dataToRead = data->sharedBuffer[readPt];
        printf("\t\t\t\t%s: buffer[%d] = %c\n", data->name, readPt, dataToRead);
        readPt = (readPt + 1) % NUM_TOTAL_BUFFERS;
        sem_post(data->pEmptyBuffers);
        ProcessData(readerData);
    }

    pthread_exit((void*) readerData);
}

static void ProcessData(void* readerData)
{
    int result;
    random_r(((threadData*) readerData)->randBuffer, &result);
    usleep(500000 + (result % 1500000));
}

static char PrepareData(void* writerData)
{
    int result;
    random_r(((threadData*) writerData)->randBuffer, &result);
    usleep(500000 + (result % 1500000));
    return ((char)(65 + (result % 25)));
}
