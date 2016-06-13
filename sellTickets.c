/**
 * sell-tickets.c
 * --------------
 * A very simple example of a critical section that is protected by a
 * semaphore lock. There is a global variable numTickets which tracks the
 * number of tickets remaining to sel. We will create many threads that all
 * will attempt to sell tickets until they are all gone. However, we must
 * control access to this global variable lest we sell more tickets than
 * really exist. We have a semaphore lock that will only allow one seller
 * thread to access the numTickets variable at a time. Before attempting to
 * sell a ticket, the thread must acquire the lock by waiting on the semaphore
 * and then release the lock when through by signalling the semaphore.
 */

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define NUM_TICKETS 35
#define NUM_SELLERS 4
#define BUFFER_SIZE 32

typedef struct {
    char* name;
    struct random_data* buffer;
} threadData;

static void* SellTickets(void* threadArgs);

/**
 * The ticket counter and its associated lock will be accessed
 * by all threads, so made global for easy access
 */

static int numTickets = NUM_TICKETS;
static sem_t ticketsLock;

/**
 * Our main creates the initial semaphore lock in an unlocked state
 * (one thread can immediately acquire it) and sets up all of the
 * ticket seller threads, and lets them run to completion. They should
 * all finish when all tickets have been sold.
 */

void main(int argc, char **argv)
{
    pthread_t* threads = (pthread_t*) calloc(NUM_SELLERS, sizeof(pthread_t));
    threadData* threadArgs = (threadData*) calloc(NUM_SELLERS, sizeof(threadData));
    struct random_data* rand_states = (struct random_data*) calloc(NUM_SELLERS, sizeof(struct random_data));
    char* rand_statebufs = (char*) calloc(NUM_SELLERS, BUFFER_SIZE);
    char nameBuffer[32];
    int i;
    int rc;

    void* status;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    sem_init(&ticketsLock, 0, 1);
    for (i = 0; i < NUM_SELLERS; i++) {
        initstate_r(random(), rand_statebufs + i, BUFFER_SIZE, rand_states + i);
        sprintf(nameBuffer, "Seller #%d", i + 1);
        (threadArgs + i)->name = strdup(nameBuffer);
        (threadArgs + i)->buffer = rand_states + i;
        rc = pthread_create(threads + i, NULL, SellTickets, (void *) (threadArgs + i));
        if (rc != 0) {
            printf("ERROR: pthread_create() failed with return code %d\n", rc);
            exit(-1);
        }
    }

    pthread_attr_destroy(&attr);
    for (i = 0; i < NUM_SELLERS; i++) {
        rc = pthread_join(threads[i], &status);
        if (rc != 0) {
            printf("ERROR: pthread_join() failed with a return code %d\n", rc);
            exit(-1);
        }
    }

    sem_destroy(&ticketsLock);
    for (i = 0; i < NUM_SELLERS; i++) free((threadArgs + i)->name);
    free(rand_states);
    free(rand_statebufs);    
    free(threadArgs);
    free(threads);
    printf("All done!\n");
    pthread_exit(NULL);
}

/**
 * SellTickets
 * -----------
 * This is the routine forked by each of the ticket-selling threads. It
 * will loop selling tickets until there are no more tickets left to
 * sell. Before access the global numTickets variable, it acquires the
 * ticketsLock to ensure that our threads don't step on one another and
 * oversell on the number of tickets
 */

static void* SellTickets(void* threadArgs)
{
    // loval vars are unique to each thread
    bool done = false;
    int numSoldByThisThread = 0;
    threadData* threadInfo = (threadData*) threadArgs;

    while (!done) {
        /**
         * imagine some code here which does something independent of
         * the other hreads such as working with a customer to determine
         * which tickets they want. Simulate with a small random delay
         * to get random variations in output patters.
         */
        int result;
        random_r(threadInfo->buffer, &result);
        usleep(500000 + result % 1500000);

        // ENTER CRITICAL SECTION
        sem_wait(&ticketsLock);
        if (numTickets == 0) {
            done = true;
        } else {
            numTickets--;
            numSoldByThisThread++;
            printf("%s sold one (%d left)\n", threadInfo->name, numTickets);
        }
        // LEAVE CRITICAL SECTION
        sem_post(&ticketsLock);
    }

    printf("%s noticed all tickets sold! (I sold %d myself)\n", threadInfo->name, numSoldByThisThread);
    pthread_exit((void*) threadArgs);
}
