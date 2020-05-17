#include "thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Variables
 */

// There are total of 2048 pages
const int NUM_PAGES;

int currentThreadId = 1;

/*
 * Implementations
 */

Thread *createThread() {
  Thread *ret = malloc(sizeof(Thread));
  bzero(ret, sizeof(Thread));

  ret->threadId = currentThreadId;
  currentThreadId++;

  // init page table
  ret->pageTable = (PageTableEntry *)malloc(sizeof(PageTableEntry) * NUM_PAGES);
  for (int i = 0; i < NUM_PAGES; i++) {
    ret->pageTable[i].physicalFrameNumber = -1; // default: not assigned
    ret->pageTable[i].present = 0; // default: not present on physical memory
  }

  // init allocation lists
  ret->stackAllocationList = createStackAllocationList();
  ret->heapAllocationList = createHeapAllocationList();

  return ret;
}

void destroyThread(Thread *thread) {
  // This is line is ABSOLUTELY REQUIRED for the tests to run properly. This
  // allows the thread to finish its work DO NOT REMOVE.
  if (thread->thread)
    pthread_join(thread->thread, NULL);

  free(thread->pageTable);
  destroyAllocationList(thread->stackAllocationList);
  destroyAllocationList(thread->heapAllocationList);
  free(thread);
}
