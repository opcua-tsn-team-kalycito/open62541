/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#ifndef _SHAREDMEMORYWRITE_H_
#define _SHAREDMEMORYWRITE_H_

// Size of each buffers in the shared memory
// TODO: The size can be modified based on the data type
static int sharedMemorySize = 100;

// Shared memory object name - a unique identifier
static char sharedMemoryObj[] = "PLCSharedMemory";

// Function to create the shared memory
void createSharedMemory(void);

#endif
