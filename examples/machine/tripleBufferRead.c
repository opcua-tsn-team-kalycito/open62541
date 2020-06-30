/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Initialise the tripleBufferRead structure and map the shared memory addresses
 * for the triple buffers. Implementation to read the data from the shared memory */

#include "tripleBufferRead.h"

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
    pStatus = (tripleBufferRead *) shmPointer;
    pBuffer[0] = (char *) pStatus + sizeof(tripleBufferRead);
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

/* Read from Shared Memory
 * ^^^^^^^^^^^^^^^^^^^^^^
 * This function reads from the shared memory. If the newData bit is set to 1, it swaps 
 * the read and clean buffer and reads the data. Or else, it reads the old data  */
void readFromBuffer(void *data, int size)
{
    // Temporary variable for swapping
    unsigned int readBuffer;

    // Check if newData flag is set and swap the clean and read buff
    if(pStatus->newData == 1)
    {
        readBuffer = pStatus->readBuff;
        pStatus->readBuff = pStatus->cleanBuff;
        pStatus->cleanBuff = readBuffer;
        pStatus->newData = 0;
    }

    // Copy the data present in the shared memory location
    memcpy(data, (char *)pBuffer[pStatus->readBuff],(size_t)size);
}
