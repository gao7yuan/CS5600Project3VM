#include "memory.h"
#include "FrameTable.h"
#include "thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tests/utils.h>

/*
 * Variables
 */

FrameTable *frameTable;
char *MEMORY;

// 4k is the size of a page
const int PAGE_SIZE = 4 * 1024;
// A total of 8M exists in memory
const int ALL_MEM_SIZE = 8 * 1024 * 1024;
// USER Space starts at 1M
const int USER_BASE_ADDR = 1024 * 1024;
// Stack starts at 8M and goes down to 6M
const int STACK_END_ADDR = 6 * 1024 * 1024;
// There are total of 2048 pages
const int NUM_PAGES = 2 * 1024;

/*
 * Helper Functions
 */

/**
 * Given a thread, an address, a size and a pointer to output data, read from
 * desired heap address for desired size.
 * @param thread - thread to read data from.
 * @param addr - address to read data from.
 * @param size - size of data to read.
 * @param outData - pointer to output data.
 */
void readFromHeapAddr(Thread *thread, int addr, int size, void *outData);

/**
 * Given a thread, an address, a size and a pointer to output data, read from
 * desired stack address for desired size.
 * @param thread - thread to read data from.
 * @param addr - address to read data from.
 * @param size - size of data to read.
 * @param outData - pointer to output data.
 */
void readFromStackAddr(Thread *thread, int addr, int size, void *outData);

/**
 * Given a thread and a VPN, read for a certain size from the page starting from
 * an offset and store to output data.
 * @param thread - thread to read data from.
 * @param virtualPageNumber - page number to read data from.
 * @param startOffset - offset to start reading in the page.
 * @param sizeToRead - bytes to read.
 * @param goPositive - 1 for heap and 0 for stack.
 * @param outData - output data.
 * @param offsetOutData - starting position to copy data in output data.
 */
void readFromPage(Thread *thread, int virtualPageNumber, int startOffset,
                  int sizeToRead, int goPositive, void *outData,
                  int offsetOutData);

/**
 * Given a thread, an address, a size and a pointer to data to write, write data
 * to the heap address for desired size.
 * @param thread - thread to write.
 * @param addr - address to write.
 * @param size - size to write.
 * @param data - data to write.
 */
void writeToHeapAddr(Thread *thread, int addr, int size, const void *data);

/**
 * Given a thread, an address, a size and a pointer to data to write, write data
 * to the stack address for desired size.
 * @param thread - thread to write.
 * @param addr - address to write.
 * @param size - size to write.
 * @param data - data to write.
 */
void writeToStackAddr(Thread *thread, int addr, int size, const void *data);

/**
 * Given a thread and VPN, write to this page starting from an offset with given
 * data.
 * @param thread - thread to write.
 * @param virtualPageNumber - VPN to write.
 * @param startOffset - starting offset to write in the page.
 * @param sizeToWrite - bytes to write.
 * @param goPositive - 1 for heap and 0 for stack.
 * @param data - data to write.
 * @param offsetData - starting position in data to write.
 */
void writeToPage(Thread *thread, int virtualPageNumber, int startOffset,
                 int sizeToWrite, int goPositive, const void *data,
                 int offsetData);

/*
 * Implementations
 */

int allocateHeapMem(Thread *thread, int size) {
  char buffer[1000];
  sprintf(buffer, "Allocating heap memory with size %d.\n", size);
  logData(buffer);
  flushLog();

  // check heap allocation list for free space available
  AllocationList *heapAllocationList = thread->heapAllocationList;
  if (heapAllocationList->availableMemory < size) {
    sprintf(buffer,
            "Attempting to allocate %d bytes with only %d available memory.",
            size, heapAllocationList->availableMemory);
    logData(buffer);
    flushLog();
    return -1;
  }
  // create new allocation list node for this allocation
  AllocationListNode *allocationListNode =
      (AllocationListNode *)malloc(sizeof(AllocationListNode));
  if (heapAllocationList->tail == NULL) {
    // if no previous allocation, start from starting address of heap
    allocationListNode->startAddress = heapAllocationList->START_ADDRESS;
    // make this head
    heapAllocationList->head = allocationListNode;
  } else {
    // if there is previous allocation, start from end address of last
    // allocation
    allocationListNode->startAddress = heapAllocationList->tail->endAddress + 1;
    // let previous tail point to this node
    heapAllocationList->tail->next = allocationListNode;
  }
  // fill in other fields
  allocationListNode->endAddress = allocationListNode->startAddress + size - 1;
  allocationListNode->allocationSize = size;
  allocationListNode->next = NULL;
  // make this node tail
  heapAllocationList->tail = allocationListNode;
  // update heap allocation list info
  heapAllocationList->availableMemory -= size;

  sprintf(buffer,
          "Finished allocating. For this heap allocation, start: %d, end: %d, "
          "size: %d.\n",
          allocationListNode->startAddress, allocationListNode->endAddress,
          allocationListNode->allocationSize);
  logData(buffer);
  flushLog();

  return allocationListNode->startAddress;
}

int allocateStackMem(Thread *thread, int size) {
  char buffer[1000];
  sprintf(buffer, "Allocating stack memory with size %d.\n", size);
  logData(buffer);
  flushLog();

  // check stack allocation list for free space available
  AllocationList *stackAllocationList = thread->stackAllocationList;
  if (stackAllocationList->availableMemory < size) {
    sprintf(buffer,
            "Attempting to allocate %d bytes with only %d available memory.\n",
            size, stackAllocationList->availableMemory);
    logData(buffer);
    flushLog();
    return -1;
  }
  // create new allocation list node for this allocation
  AllocationListNode *allocationListNode =
      (AllocationListNode *)malloc(sizeof(AllocationListNode));
  if (stackAllocationList->tail == NULL) {
    // if no previous allocation, start from starting address of stack
    allocationListNode->startAddress = stackAllocationList->START_ADDRESS;
    // make this head
    stackAllocationList->head = allocationListNode;
  } else {
    // if there is previous allocation, start from end address of last
    // allocation
    allocationListNode->startAddress =
        stackAllocationList->tail->endAddress - 1;
    // let previous tail point to this node
    stackAllocationList->tail->next = allocationListNode;
  }
  // fill in other fields
  allocationListNode->endAddress =
      allocationListNode->startAddress - size + 1; // stack grows negatively
  allocationListNode->allocationSize = size;
  allocationListNode->next = NULL;
  // make this node tail
  stackAllocationList->tail = allocationListNode;
  // update heap allocation list info
  stackAllocationList->availableMemory -= size;

  sprintf(buffer,
          "Finished allocating. For this stack allocation, start: %d, end: %d, "
          "size: %d.\n",
          allocationListNode->startAddress, allocationListNode->endAddress,
          allocationListNode->allocationSize);
  logData(buffer);
  flushLog();

  return allocationListNode->startAddress;
}

void writeToAddr(Thread *thread, int addr, int size, const void *data) {
  char buffer[1000];
  if (addr < USER_BASE_ADDR || addr >= ALL_MEM_SIZE) {
    sprintf(buffer, "Writing to un-accessible addr %d.\n", addr);
    logData(buffer);
    flushLog();
    kernelPanic(thread, addr);
    return;
  }

  if (addr >= USER_BASE_ADDR && addr < STACK_END_ADDR) {
    // write to heap
    writeToHeapAddr(thread, addr, size, data);
  } else if (addr >= STACK_END_ADDR && addr < ALL_MEM_SIZE) {
    // write to stack
    writeToStackAddr(thread, addr, size, data);
  } else {
    // addr out of range
    kernelPanic(thread, addr);
  }
}

void writeToHeapAddr(Thread *thread, int addr, int size, const void *data) {
  char buffer[1000];
  sprintf(buffer, "Attempting to write to heap addr %d with size %d.\n", addr,
          size);
  logData(buffer);
  flushLog();

  if (addr < USER_BASE_ADDR || addr >= STACK_END_ADDR) {
    kernelPanic(thread, addr);
    return;
  }
  AllocationList *heapAllocationList = thread->heapAllocationList;
  // if no heap mem allocated, or addr to read is outside of region of heap mem
  // allocation, kernel panic
  if (heapAllocationList->head == NULL || heapAllocationList->tail == NULL ||
      addr < heapAllocationList->head->startAddress ||
      addr > heapAllocationList->tail->endAddress) {
    sprintf(buffer, "Address at %d out of allocated heap range.\n", addr);
    logData(buffer);
    flushLog();
    kernelPanic(thread, addr);
    return;
  }
  // check if size is within allocated mem
  if (heapAllocationList->tail->endAddress < addr + size - 1) {
    sprintf(buffer,
            "Writing beyond end of heap allocation at address %d, allocated "
            "heap end address is %d.\n",
            addr + size - 1, heapAllocationList->tail->endAddress);
    logData(buffer);
    flushLog();
    kernelPanic(thread, addr);
    // below never executed
    sprintf(buffer, "I just raised a kernel panic.\n");
    logData(buffer);
    flushLog();
    return;
  }

  // write page by page
  int pageNumber = addr / PAGE_SIZE;  // current page number
  int startOffset = addr % PAGE_SIZE; // starting offset in page in bytes
  int sizeToWrite =
      PAGE_SIZE -
      startOffset; // bytes to read in a page, init to full page - offset
  int bytesWritten = 0;
  while (bytesWritten < size) {
    // if size to write in a page greater than remaining bytes to read, change
    // to remaining bytes to read
    sizeToWrite =
        sizeToWrite > size - bytesWritten ? size - bytesWritten : sizeToWrite;
    writeToPage(thread, pageNumber, startOffset, sizeToWrite, 1, data,
                bytesWritten);
    bytesWritten += sizeToWrite;
    pageNumber++;
    startOffset = 0;
    sizeToWrite = PAGE_SIZE;
  }
}

void writeToStackAddr(Thread *thread, int addr, int size, const void *data) {
  char buffer[4096];
  sprintf(buffer, "Attempting to write to stack addr %d with size %d.\n", addr,
          size);
  logData(buffer);
  flushLog();

  if (addr < STACK_END_ADDR || addr >= ALL_MEM_SIZE) {
    sprintf(buffer, "Address at %d out of allocated stack range.\n", addr);
    logData(buffer);
    flushLog();
    kernelPanic(thread, addr);
    return;
  }
  AllocationList *stackAllocationList = thread->stackAllocationList;
  // if no stack mem allocated, or addr to read is outside of region of stack
  // mem allocation, kernel panic
  if (stackAllocationList->head == NULL || stackAllocationList->tail == NULL ||
      addr > stackAllocationList->head->startAddress ||
      addr < stackAllocationList->tail->endAddress) {
    kernelPanic(thread, addr);
    return;
  }
  // check if size is within allocated mem
  if (stackAllocationList->tail->endAddress > addr - size + 1) {
    sprintf(buffer,
            "Writing beyond end of stack allocation at address %d, allocated "
            "stack end address is %d.\n",
            addr - size + 1, stackAllocationList->tail->endAddress);
    logData(buffer);
    flushLog();
    kernelPanic(thread, addr);
    // below never executed
    sprintf(buffer, "I just raised a kernel panic.\n");
    logData(buffer);
    flushLog();
    return;
  }

  // write page by page
  int pageNumber = addr / PAGE_SIZE; // current page number
  int startOffset =
      PAGE_SIZE - addr % PAGE_SIZE; // starting offset in page in bytes
  int sizeToWrite =
      addr % PAGE_SIZE; // bytes to write in a page, init to full page - offset
  int bytesWritten = 0;
  while (bytesWritten < size) {
    sizeToWrite =
        sizeToWrite > size - bytesWritten ? size - bytesWritten : sizeToWrite;
    writeToPage(thread, pageNumber, startOffset, sizeToWrite, 0, data,
                bytesWritten);
    bytesWritten += sizeToWrite;
    pageNumber--;
    startOffset = 1;
    sizeToWrite = PAGE_SIZE;
  }
  sprintf(buffer, "Successfully written to addr %d with size %d.\n", addr,
          size);
  logData(buffer);
  flushLog();
}

void writeToPage(Thread *thread, int virtualPageNumber, int startOffset,
                 int sizeToWrite, int goPositive, const void *data,
                 int offsetData) {
  // find PFN
  int physicalFrameNumber;
  if (thread->pageTable[virtualPageNumber].physicalFrameNumber == -1) {
    // if not translated
    physicalFrameNumber = translate(thread, virtualPageNumber, frameTable);
  } else {
    // if already translated, check current pfn and whether it is present
    physicalFrameNumber =
        thread->pageTable[virtualPageNumber].physicalFrameNumber;
    // lock the frame when checking
    pthread_mutex_lock(
        &(frameTable->frameTableEntryArray[physicalFrameNumber].lock));
    if (thread->pageTable[virtualPageNumber].present == 0) {
      // if not present, unlock this frame because we don't need it
      pthread_mutex_unlock(
          &(frameTable->frameTableEntryArray[physicalFrameNumber].lock));
      // swap in
      physicalFrameNumber = swapPageIn(thread, frameTable, virtualPageNumber);
    }
  }

  // now the frame regarding physicalFrameNumber is locked, we can process it
  frameTable->frameTableEntryArray[physicalFrameNumber].accessed = 1;
  // write to memory
  for (int i = 0; i < sizeToWrite; i++) {
    if (goPositive) {
      memcpy(&(MEMORY[PAGE_SIZE * physicalFrameNumber + startOffset + i]),
             &(data[offsetData + i]), 1);
    } else {
      memcpy(&(MEMORY[PAGE_SIZE * (physicalFrameNumber + 1) - startOffset - i]),
             &(data[offsetData + i]), 1);
    }
  }
  // unlock the frame when done
  pthread_mutex_unlock(
      &(frameTable->frameTableEntryArray[physicalFrameNumber].lock));
}

void readFromAddr(Thread *thread, int addr, int size, void *outData) {
  if (addr < USER_BASE_ADDR || addr >= ALL_MEM_SIZE) {
    kernelPanic(thread, addr);
    return;
  }

  if (addr >= USER_BASE_ADDR && addr < STACK_END_ADDR) {
    // read from heap
    readFromHeapAddr(thread, addr, size, outData);
  } else if (addr > STACK_END_ADDR && addr <= ALL_MEM_SIZE) {
    // read from stack
    readFromStackAddr(thread, addr, size, outData);
  } else {
    // addr out of range
    kernelPanic(thread, addr);
  }
}

void readFromHeapAddr(Thread *thread, int addr, int size, void *outData) {
  if (addr < USER_BASE_ADDR || addr >= STACK_END_ADDR) {
    kernelPanic(thread, addr);
    return;
  }
  AllocationList *heapAllocationList = thread->heapAllocationList;
  // if no heap mem allocated, or addr to read is outside of region of heap mem
  // allocation, kernel panic
  if (heapAllocationList->head == NULL || heapAllocationList->tail == NULL ||
      addr < heapAllocationList->head->startAddress ||
      addr > heapAllocationList->tail->endAddress) {
    kernelPanic(thread, addr);
    return;
  }
  // check if size is within allocated mem
  if (heapAllocationList->tail->endAddress < addr + size - 1) {
    kernelPanic(thread, addr);
    return;
  }

  // read page by page
  int pageNumber = addr / PAGE_SIZE;  // current page number
  int startOffset = addr % PAGE_SIZE; // starting offset in page in bytes
  int sizeToRead =
      PAGE_SIZE -
      startOffset; // bytes to read in a page, init to full page - offset
  int bytesRead = 0;
  while (bytesRead < size) {
    // if size to read in a page greater than remaining bytes to read, change to
    // remaining bytes to read
    sizeToRead = sizeToRead > size - bytesRead ? size - bytesRead : sizeToRead;
    readFromPage(thread, pageNumber, startOffset, sizeToRead, 1, outData,
                 bytesRead);
    bytesRead += sizeToRead;
    pageNumber++;
    startOffset = 0;
    sizeToRead = PAGE_SIZE;
  }
}

void readFromStackAddr(Thread *thread, int addr, int size, void *outData) {
  if (addr < STACK_END_ADDR || addr >= ALL_MEM_SIZE) {
    kernelPanic(thread, addr);
    return;
  }
  AllocationList *stackAllocationList = thread->stackAllocationList;
  // if no stack mem allocated, or addr to read is outside of region of stack
  // mem allocation, kernel panic
  if (stackAllocationList->head == NULL || stackAllocationList->tail == NULL ||
      addr > stackAllocationList->head->startAddress ||
      addr < stackAllocationList->tail->endAddress) {
    kernelPanic(thread, addr);
    return;
  }
  // check if size is within allocated mem
  if (stackAllocationList->tail->endAddress > addr - size + 1) {
    kernelPanic(thread, addr);
    return;
  }

  // read page by page
  int pageNumber = addr / PAGE_SIZE; // current page number
  int startOffset =
      PAGE_SIZE - addr % PAGE_SIZE; // starting offset in page in bytes
  int sizeToRead =
      addr % PAGE_SIZE; // bytes to read in a page, init to full page - offset
  int bytesRead = 0;
  while (bytesRead < size) {
    // if size to read in a page greater than remaining bytes to read, change to
    // remaining bytes to read
    sizeToRead = sizeToRead > size - bytesRead ? size - bytesRead : sizeToRead;
    readFromPage(thread, pageNumber, startOffset, sizeToRead, 0, outData,
                 bytesRead);
    bytesRead += sizeToRead;
    pageNumber--;
    startOffset = 1;
    sizeToRead = PAGE_SIZE;
  }
}

void readFromPage(Thread *thread, int virtualPageNumber, int startOffset,
                  int sizeToRead, int goPositive, void *outData,
                  int offsetOutData) {
  // find PFN
  int physicalFrameNumber;

  // check current pfn and whether it is present
  physicalFrameNumber =
      thread->pageTable[virtualPageNumber].physicalFrameNumber;
  // lock the frame when checking
  pthread_mutex_lock(
      &(frameTable->frameTableEntryArray[physicalFrameNumber].lock));
  if (thread->pageTable[virtualPageNumber].present == 0) {
    // if not present, unlock this frame because we don't need it
    pthread_mutex_unlock(
        &(frameTable->frameTableEntryArray[physicalFrameNumber].lock));
    // swap in
    physicalFrameNumber = swapPageIn(thread, frameTable, virtualPageNumber);
  }

  // now this frame is locked we can process it
  frameTable->frameTableEntryArray[physicalFrameNumber].accessed = 1;
  // read from memory
  for (int i = 0; i < sizeToRead; i++) {
    if (goPositive) {
      memcpy(&(outData[offsetOutData + i]),
             &(MEMORY[PAGE_SIZE * physicalFrameNumber + startOffset + i]), 1);
    } else {
      memcpy(&(outData[offsetOutData + i]),
             &(MEMORY[PAGE_SIZE * (physicalFrameNumber + 1) - startOffset - i]),
             1);
    }
  }
  // unlock this frame
  pthread_mutex_unlock(
      &(frameTable->frameTableEntryArray[physicalFrameNumber].lock));
}

char *getCacheFileName(Thread *thread, int addr) {
  if (addr < USER_BASE_ADDR || addr >= ALL_MEM_SIZE) {
    kernelPanic(thread, addr);
    return NULL;
  }
  int pageNumber = addr / PAGE_SIZE;
  char fileName[100];
  sprintf(fileName, "./cache/%d_%d", thread->threadId, pageNumber);
  char *ret = (char *)malloc(strlen(fileName) + 1);
  strcpy(ret, fileName);
  return ret;
}