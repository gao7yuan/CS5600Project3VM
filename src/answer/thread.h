#ifndef VIRTUALMEMFRAMEWORKC_THREAD_H
#define VIRTUALMEMFRAMEWORKC_THREAD_H

#include "AllocationList.h"
#include "PageTable.h"
#include <pthread.h>

/**
 * Represents a thread. Includes a pthread_t, thread id, an array for page
 * table, an allocation list for stack and one for heap.
 */
typedef struct Thread {
  pthread_t thread;
  int threadId;
  PageTableEntry
      *pageTable; // an array of page table entries, size should be 2k
  AllocationList *stackAllocationList; // info about all stack allocations
  AllocationList *heapAllocationList;  // info about all heap allocations
} Thread;

/**
 * This function should create and return a thread that can be started anytime
 * after the return of the call. A caller may execute the pthread contained
 * within the thread struct anytime after returning from this call.
 * @return A fully functional thread object that has been initialized and is
 * ready to run.
 */
Thread *createThread();

/**
 * Destroys a thread object and cleans up any allocated memory. This function
 * will also do a pthread_join so that the called thread can finish completing.
 * Removing the pthread_join will cause bad behavior.
 * @param thread
 */
void destroyThread(Thread *thread);

#endif // VIRTUALMEMFRAMEWORKC_THREAD_H
