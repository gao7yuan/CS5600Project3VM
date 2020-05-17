#include "FrameTable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * Variables
 */

// size of a page
extern const int PAGE_SIZE;
// total number of pages
extern const int NUM_PAGES;
// physical memory space
extern char *MEMORY;
// page number that user start to use
const int USER_BASE_PAGE_NUM = 256;

/*
 * Helper Functions
 */

/**
 * Given a thread, a VPN and its matching PFN, update the corresponding
 * information in its page table and frame table.
 * @param thread - thread to update information with.
 * @param virtualPageNumber - VPN.
 * @param physicalFrameNumber - PFN.
 * @param frameTable - frame table to update information with.
 */
void updateTables(Thread *thread, int virtualPageNumber,
                  int physicalFrameNumber, FrameTable *frameTable);

/**
 * Given a frame table, apply clock algorithm to find a frame to evict page.
 * @param frameTable - frame table to apply clock algorithm.
 * @return the frame number to evict page.
 */
int applyClockAlgorithm(FrameTable *frameTable);

/**
 * Given a frame and its PFN, cache its information in memory to disk.
 * @param frame - frame to cache to disk.
 * @param physicalFrameNumber - physical frame number related to the frame.
 */
void cacheToDisk(FrameTableEntry *frame, int physicalFrameNumber);

/**
 * Given a thread, a VPN and a PFN, load data from disk related to the thread
 * and VPN to corresponding PFN on memory.
 * @param thread - thread to load data from.
 * @param virtualPageNumber - VPN to load data from.
 * @param physicalFrameNumber - PFN to load data to.
 */
void loadFromDisk(Thread *thread, int virtualPageNumber,
                  int physicalFrameNumber);

/*
 * Implementations
 */

// create a frame table
// init with a frame table entry array with size 2k
// init index
FrameTable *createFrameTable() {
  FrameTable *ret = (FrameTable *)malloc(sizeof(FrameTable));
  ret->index = USER_BASE_PAGE_NUM; // initial starting index
  ret->frameTableEntryArray =
      (FrameTableEntry *)malloc(sizeof(FrameTableEntry) * NUM_PAGES);
  for (int i = 0; i < NUM_PAGES; i++) {
    ret->frameTableEntryArray[i].threadId = -1;
    ret->frameTableEntryArray[i].thread = NULL;
    ret->frameTableEntryArray[i].virtualPageNumber = -1;
    ret->frameTableEntryArray[i].accessed = 0;
    pthread_mutex_init(&(ret->frameTableEntryArray[i].lock), NULL);
  }
  pthread_mutex_init(&(ret->lock), NULL);
  return ret;
}

void destroyFrameTable(FrameTable *frameTable) {
  for (int i = 0; i < NUM_PAGES; i++) {
    pthread_mutex_destroy(&(frameTable->frameTableEntryArray[i].lock));
  }
  pthread_mutex_destroy(&(frameTable->lock));
  free(frameTable->frameTableEntryArray);
  free(frameTable);
}

// called only when this vpn of thread has not been translated
int translate(Thread *thread, int virtualPageNumber, FrameTable *frameTable) {
  // check frame table for available frame from USER_BASE_PAGE_NUM to avoid
  // accessing kernel
  int physicalFrameNumber = -1;
  // lock whole frame table to scan
  pthread_mutex_lock(&(frameTable->lock));
  for (int i = USER_BASE_PAGE_NUM; i < NUM_PAGES; i++) {
    // lock current frame to check
    pthread_mutex_lock(&(frameTable->frameTableEntryArray[i].lock));
    if (frameTable->frameTableEntryArray[i].threadId == -1) {
      // found an available frame!
      physicalFrameNumber = i;
      break; // without unlocking it, because we need to use it
    }
    // unlock current frame
    pthread_mutex_unlock(&(frameTable->frameTableEntryArray[i].lock));
  }
  // if there's no available frame, apply clock algorithm and evict an old frame
  if (physicalFrameNumber == -1) {
    physicalFrameNumber = applyClockAlgorithm(frameTable);
    // evict old frame
    swapPageOut(
        frameTable,
        physicalFrameNumber); // at this time we still need its frame lock
  }
  updateTables(thread, virtualPageNumber, physicalFrameNumber, frameTable);
  // unlock the whole frame
  pthread_mutex_unlock(&(frameTable->lock));
  return physicalFrameNumber;
}

// apply clock algorithm to find a frame number to evict
int applyClockAlgorithm(FrameTable *frameTable) {
  int toEvict = -1;
  int start = frameTable->index;
  do {
    // lock current frame to check
    pthread_mutex_lock(
        &(frameTable->frameTableEntryArray[frameTable->index].lock));
    if (frameTable->frameTableEntryArray[frameTable->index].accessed == 0) {
      // if current frame was not accessed recently, decide to evict it
      toEvict = frameTable->index;
      break; // without unlocking because we still need it
    }
    frameTable->frameTableEntryArray[frameTable->index].accessed =
        0; // reset to not accessed
    // unlock current frame because we are to check next
    pthread_mutex_unlock(
        &(frameTable->frameTableEntryArray[frameTable->index].lock));
    frameTable->index++;
    if (frameTable->index >= NUM_PAGES) {
      frameTable->index = USER_BASE_PAGE_NUM;
    }
  } while (frameTable->index != start);
  if (toEvict == -1) {
    // since we want to evict start, we lock the start frame
    pthread_mutex_lock(&(frameTable->frameTableEntryArray[start].lock));
    toEvict = start;
  }
  return toEvict;
}

// update page table and frame table with given VPN, PFN and thread
void updateTables(Thread *thread, int virtualPageNumber,
                  int physicalFrameNumber, FrameTable *frameTable) {
  // update frame info
  frameTable->frameTableEntryArray[physicalFrameNumber].threadId =
      thread->threadId;
  frameTable->frameTableEntryArray[physicalFrameNumber].thread = thread;
  frameTable->frameTableEntryArray[physicalFrameNumber].virtualPageNumber =
      virtualPageNumber;
  frameTable->frameTableEntryArray[physicalFrameNumber].accessed = 0;
  // update page info
  thread->pageTable[virtualPageNumber].present = 1;
  thread->pageTable[virtualPageNumber].physicalFrameNumber =
      physicalFrameNumber;
}

// swap page out from desired PFN
void swapPageOut(FrameTable *frameTable, int physicalFrameNumber) {
  FrameTableEntry frame =
      frameTable
          ->frameTableEntryArray[physicalFrameNumber]; // find frame to swap out
  int virtualPageNumber = frame.virtualPageNumber; // find virtual page to cache
  cacheToDisk(&frame, physicalFrameNumber);
  // update page table
  PageTableEntry *pageTable =
      frameTable->frameTableEntryArray[physicalFrameNumber].thread->pageTable;
  pageTable[virtualPageNumber].present =
      0; // indicate this page not on physical memory now
}

void cacheToDisk(FrameTableEntry *frame, int physicalFrameNumber) {
  // default file name: ./cache/threadId_vpn
  char fileName[100];
  bzero(fileName, 100);
  sprintf(fileName, "./cache/%d_%d", frame->threadId, frame->virtualPageNumber);

  // read from memory to buffer
  char buffer[PAGE_SIZE];
  bzero(buffer, PAGE_SIZE);
  memcpy(buffer, &(MEMORY[physicalFrameNumber * PAGE_SIZE]), PAGE_SIZE);

  // write to file
  FILE *file = fopen(fileName, "w+");
  fwrite(buffer, sizeof(char), PAGE_SIZE, file);
  fclose(file);
}

// swap page in, return updated physical frame number
int swapPageIn(Thread *thread, FrameTable *frameTable, int virtualPageNumber) {
  // lock the whole frame table so that we can scan it and apply clock algorithm
  pthread_mutex_lock(&(frameTable->lock));
  // apply clock algorithm to find the frame to evict
  int frameNumberToSwap = applyClockAlgorithm(
      frameTable); // find the frame to swap out, it has the frame lock!
  // swap page out
  swapPageOut(frameTable, frameNumberToSwap);
  // load from disk to memory
  loadFromDisk(thread, virtualPageNumber, frameNumberToSwap);
  // update tables
  updateTables(thread, virtualPageNumber, frameNumberToSwap, frameTable);
  // unlock the whole frame table since we are done scanning
  pthread_mutex_unlock(&(frameTable->lock));
  return frameNumberToSwap;
}

void loadFromDisk(Thread *thread, int virtualPageNumber,
                  int physicalFrameNumber) {
  char fileName[100];
  bzero(fileName, 100);
  sprintf(fileName, "./cache/%d_%d", thread->threadId, virtualPageNumber);

  char buffer[PAGE_SIZE];
  bzero(buffer, PAGE_SIZE);

  // read from file to buffer
  FILE *file;
  if ((file = fopen(fileName, "r"))) {
    fseek(file, 0, SEEK_SET);
    fread(buffer, sizeof(char), PAGE_SIZE, file);
    fclose(file);
  }

  // write to MEMORY
  memcpy(&MEMORY[physicalFrameNumber * PAGE_SIZE], buffer, PAGE_SIZE);
}
