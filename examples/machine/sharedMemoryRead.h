/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#ifdef UA_ARCHITECTURE_POSIX

#ifndef SHAREDMEMORYREAD_H
#define SHAREDMEMORYREAD_H

// Size of each buffers in the shared memory
// TODO: The size can be modified based on the data type
static int sharedMemorySize = 100;

// Shared memory object name - a unique identifier
static char sharedMemoryObj[] = "PLCSharedMemory";

// Function to create the shared memory
void createSharedMemory(void);

// Function to delete the shared memory
void sharedMemoryDelete(void);

#endif

#endif

