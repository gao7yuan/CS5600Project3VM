#include "AllocationList.h"
#include <stdlib.h>

/*
 * Variables
 */

// total memory
extern const int ALL_MEM_SIZE;
// USER Space starting address
extern const int USER_BASE_ADDR;
// Stack starts at 8M and goes down to 6M
extern const int STACK_END_ADDR;

/*
 * Implementations
 */

// space allowed for stack: [ALL_MEM_SIZE - 1 -> STACK_END_ADDR]
// stack grows negatively
AllocationList *createStackAllocationList() {
  AllocationList *stackList = (AllocationList *)malloc(sizeof(AllocationList));
  stackList->head = NULL;
  stackList->tail = NULL;
  stackList->availableMemory = ALL_MEM_SIZE - STACK_END_ADDR;
  stackList->START_ADDRESS = ALL_MEM_SIZE - 1;
  stackList->END_ADDRESS = STACK_END_ADDR;
  return stackList;
}

// space allowed for heap: [USER_BASE_ADDR -> STACK_END_ADDR - 1]
// heap grows positively
AllocationList *createHeapAllocationList() {
  AllocationList *heapList = (AllocationList *)malloc(sizeof(AllocationList));
  heapList->head = NULL;
  heapList->tail = NULL;
  heapList->availableMemory = STACK_END_ADDR - USER_BASE_ADDR;
  heapList->START_ADDRESS = USER_BASE_ADDR;
  heapList->END_ADDRESS = STACK_END_ADDR - 1;
  return heapList;
}

void destroyAllocationList(AllocationList *allocationList) {
  AllocationListNode *cur = allocationList->head;
  AllocationListNode *next = NULL;
  while (cur != NULL) {
    next = cur->next;
    free(cur);
    cur = next;
  }
  free(allocationList);
}
