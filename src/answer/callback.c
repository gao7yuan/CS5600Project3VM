#include "callback.h"
#include <stdio.h>
#include <stdlib.h>
#include <tests/utils.h>
#include <unistd.h>

/*
 * Variables
 */

// total memory
const int ALL_MEM_SIZE;
char *MEMORY;
FrameTable *frameTable; // information of frames on physical memory

/*
 * Implementations
 */

void startupCallback() {
  char buffer[1000];
  sprintf(buffer, "Starting...\n");
  logData(buffer);
  flushLog();

  // create directory for caching
  if (access("cache", F_OK) != 0) {
    system("mkdir ./cache");
  }
  // create global data structures and allocate memory
  frameTable = createFrameTable();
  MEMORY = (char *)malloc(ALL_MEM_SIZE);

  sprintf(buffer, "Ready to test!\n");
  logData(buffer);
  flushLog();
}
void shutdownCallback() {
  char buffer[1000];
  sprintf(buffer, "Shutting down.\n");
  logData(buffer);
  flushLog();

  // clear cache
  if (access("cache", F_OK) == 0) {
    system("rm -r ./cache");
  }
  // clean memory
  free(MEMORY);
  destroyFrameTable(frameTable);

  sprintf(buffer, "Shut down already.\n");
  logData(buffer);
  flushLog();
}