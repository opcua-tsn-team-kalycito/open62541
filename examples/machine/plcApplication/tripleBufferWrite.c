/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Initialise the tripleBufferRead structure and map the shared memory addresses
 * for the triple buffers. Implementation to read the data from the shared memory */

#include "tripleBufferWrite.h"

#include <stdio.h>
#include <string.h>

/* Triple Buffer intialisation
 * ^^^^^^^^^^^^^^^^^^^^^^
 * This function maps the three buffers to the shared memory address. The start address
 * of the shared memory is to hold the read, write and clean buffer index value along 
 * with the flag bit to indicate new data. Then three buffer pointers are assigned by
 * incrementing the start address with shared memory size. Then initialise the triple
 * buffer index values */
void tripleBufferInit(void *shmPointer, int size)
{
    // Triple buffers to point some address value in the shared memory region
    pStatus = (tripleBufferWrite *) shmPointer;
    pBuffer[0] = (char *) pStatus + sizeof(tripleBufferWrite);
    pBuffer[1] = (char *) pBuffer[0] + size;
    pBuffer[2] = (char *) pBuffer[1] +size;

    // Condition to check the triple buffer initialisation
    if (pStatus->initTripleBuffer == 0)
    {
        // Index of read, write and clean buffer
        pStatus->readBuff = 0;
        pStatus->writeBuff = 1;
        pStatus->cleanBuff = 2;

        // Set the flag bit to indicate the availability of new data
        pStatus->newData = 0;

        // Set the flag bit to indicate the triple buffer initialisation
        pStatus->initTripleBuffer = 1;
    }
}

/* Write into Shared Memory
 * ^^^^^^^^^^^^^^^^^^^^^^
 * This function writes the data into shared memory. It copies the data into write buffer
 * index of shared memory and swaps the clean and write buffer index. Sets the newData flag
 * to 1 to indicate fresh data */
void writeToBuffer(void* data, int size)
{
    // Temporary variable for swapping
    unsigned int writeBuffer;

    // Copy the data into the shared memory
    memcpy((char *)pBuffer[pStatus->writeBuff], data, size);

    // Swap the write and clean buffer
    writeBuffer = pStatus->writeBuff;
    pStatus->writeBuff = pStatus->cleanBuff;
    pStatus->cleanBuff = writeBuffer;

    // Set the flag bit to indicate new data
    pStatus->newData = 1;
}
