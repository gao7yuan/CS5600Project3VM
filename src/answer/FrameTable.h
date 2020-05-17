#ifndef VIRTUALMEMFRAMEWORKC_FRAMETABLE_H
#define VIRTUALMEMFRAMEWORKC_FRAMETABLE_H

#include "thread.h"

/**
 * Represents a frame table entry.
 */
typedef struct FrameTableEntry {
  int threadId;          // thread id the frame corresponds to
  Thread *thread;        // thread the frame corresponds to
  int virtualPageNumber; // virtual page number the frame matches
  int accessed;          // whether it was accessed recently
  pthread_mutex_t lock;  // frame lock
} FrameTableEntry;

/**
 * Represents a frame table that contains a frame table entry array and a
 * starting index for clock algorithm.
 */
typedef struct FrameTable {
  FrameTableEntry *frameTableEntryArray;
  int index;            // index to start with, for clock algorithm
  pthread_mutex_t lock; // frame table lock
} FrameTable;

/**
 * Creates a frame table.
 * @return a frame table.
 */
FrameTable *createFrameTable();

/**
 * Given a frame table, destroy it and free all memory allocated to it.
 * @param frameTable - frame table to destroy.
 */
void destroyFrameTable(FrameTable *frameTable);

/**
 * Given a thread id and a virtual page number that has not been translated, use
 * clock algorithm to translate virtual page to a physical frame.
 * @param thread - thread to translate.
 * @param virtualPageNumber - virtual page number to translate.
 * @param frameTable - frame table to update.
 * @return physical frame number.
 */
int translate(Thread *thread, int virtualPageNumber, FrameTable *frameTable);

/**
 * Given a frame table and physical frame number of a frame to swap out to disk,
 * update related frame table and page table information.
 * @param frameTable - frame table that stores all frame information.
 * @param physicalFrameNumber - physical frame number of the frame to page out.
 */
void swapPageOut(FrameTable *frameTable, int physicalFrameNumber);

/**
 * Given a frame table, the thread and virtual page number, find the location on
 * disk and swap back to physical memory.
 * @param thread - thread to access.
 * @param frameTable - frame table.
 * @param virtualPageNumber - virtual page number of the page to access.
 * @return the new physical frame number of the page.
 */
int swapPageIn(Thread *thread, FrameTable *frameTable, int virtualPageNumber);

#endif // VIRTUALMEMFRAMEWORKC_FRAMETABLE_H
