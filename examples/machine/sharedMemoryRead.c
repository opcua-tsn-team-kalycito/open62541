/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Creating a shared memory with a size to accodomate triple buffer to hold
 * the data. This application can be used in tutorial server to create and
 * read the shared memory data. */

// Include triple buffer and shared memory headers
#include "tripleBufferRead.h"
#include "sharedMemoryRead.h"

#define _POSIX_C_SOURCE 200809L

// Standard headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>

int shmFd;
void *pshmAddr;

/* Creating Shared Memory
 * ^^^^^^^^^^^^^^^^^^^^^^
 * This function either creates or opens a shared memory with the key 
 * "PLCSharedMemory" and truncates its size to hold triple buffers of 100 bytes
 * each. This further invokes the triple buffer initialisation function to 
 * set values for read, write and clean buffer indexes. */
void createSharedMemory(void)
{
    int result;
    
    // Open the shared memory with read and write permission
    shmFd = shm_open(sharedMemoryObj , O_CREAT | O_RDWR, 0666);

    // Truncate the size of shared memory file descriptor
    result = ftruncate(shmFd, ((3 * sharedMemorySize) + (int)sizeof(tripleBufferRead)));

    // Map the file descriptor into the memory
    pshmAddr = mmap(0, (size_t)(3 * sharedMemorySize) + sizeof(tripleBufferRead), PROT_WRITE | PROT_READ, MAP_SHARED, shmFd, 0);

    // Initialise the triple buffer
    tripleBufferInit(pshmAddr, sharedMemorySize);
}

/* Shared memory delete
 * ^^^^^^^^^^^^^^^^^^^^
 * This function deletes the shared memory by unmapping the memory, deallocating
 * the file descriptor and removing the shared memory object. */
void sharedMemoryDelete()
{
    // Remove mapping for the pages that contains the address space
    munmap(pshmAddr, (size_t)(3 * sharedMemorySize) + sizeof(tripleBufferRead));
    
    // Deallocate the file descriptor
    close(shmFd);

    // Remove the name of the shared memory object
    shm_unlink(sharedMemoryObj);
}








