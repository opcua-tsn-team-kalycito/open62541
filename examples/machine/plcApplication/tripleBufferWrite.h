/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#ifndef _TRIPLEBUFFERWRITE_H_
#define _TRIPLEBUFFERWRITE_H_

// Structure containing the buffer index and flag bit for indicating new data and
// triple buffer initialisation
typedef struct
{
    unsigned int readBuff;
    unsigned int writeBuff;
    unsigned int cleanBuff;
    unsigned int newData;
    unsigned int initTripleBuffer;
}tripleBufferWrite;

// Function to initialise the value of tripleBufferRead structure and shared
// memory mapping
void tripleBufferInit(void *shmPointer, int size);

// Function to write into the Shared Memory
void writeToBuffer(void* data, int size);

// Pointer to the three buffer addresses
static void *pBuffer[3];

// tripleBufferRead structure instance
static tripleBufferWrite *pStatus;

#endif
