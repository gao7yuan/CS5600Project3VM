#ifndef VIRTUALMEMFRAMEWORKC_ALLOCATIONLIST_H
#define VIRTUALMEMFRAMEWORKC_ALLOCATIONLIST_H

/**
 * Represents information of one allocation. Includes starting address and end
 * address, allocation size, and pointer to next allocation.
 */
typedef struct AllocationListNode {
  int startAddress;   // starting address for this allocation
  int endAddress;     // ending address for this allocation
  int allocationSize; // size of this allocation
  struct AllocationListNode *next;
} AllocationListNode;

/**
 * Represents a list of allocations on memory. Includes head (first allocation)
 * and tail (last allocation), allowed starting and ending addresses for
 * allocation, and memory available.
 */
typedef struct AllocationList {
  AllocationListNode *head; // first allocation
  AllocationListNode *tail; // last allocation
  int START_ADDRESS;        // total start address for allocation
  int END_ADDRESS;          // total end address for allocation
  int availableMemory;      // memory available for allocation
} AllocationList;

/**
 * Initiate an allocation list for stack.
 * @return an allocation list for stack.
 */
AllocationList *createStackAllocationList();

/**
 * Initiate an allocation list for heap.
 * @return an allocation list for heap.
 */
AllocationList *createHeapAllocationList();

/**
 * Given an allocation list, free all memory allocated for it.
 * @param allocationList - allocation list to destroy.
 */
void destroyAllocationList(AllocationList *allocationList);

#endif // VIRTUALMEMFRAMEWORKC_ALLOCATIONLIST_H
