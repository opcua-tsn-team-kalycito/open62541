/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#ifdef UA_ARCHITECTURE_POSIX

#ifndef TRIPLEBUFFERREAD_H
#define TRIPLEBUFFERREAD_H

// Structure containing the buffer index and flag bit for indicating new data and
// triple buffer initialisation
typedef struct
{
    unsigned int readBuff;
    unsigned int writeBuff;
    unsigned int cleanBuff;
    unsigned int newData;
    unsigned int initTripleBuffer;
}tripleBufferRead;

// Function to initialise the value of tripleBufferRead structure and shared
// memory mapping
void tripleBufferInit(void *shmPointer, int size);

// Function to read the data from Shared Memory
void readFromBuffer(void *data, int size);

// Pointer to the three buffer addresses
void *pBuffer[3];

// tripleBufferRead structure instance
tripleBufferRead *pStatus;

#endif

#endif
