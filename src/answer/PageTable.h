#ifndef VIRTUALMEMFRAMEWORKC_PAGETABLE_H
#define VIRTUALMEMFRAMEWORKC_PAGETABLE_H

/**
 * Represents a page table entry.
 */
typedef struct PageTableEntry {
  int physicalFrameNumber; // corresponding physical frame number that matches
                           // with this page
  int present; // whether the frame related to this page is available on
               // physical memory or not
} PageTableEntry;

#endif // VIRTUALMEMFRAMEWORKC_PAGETABLE_H
