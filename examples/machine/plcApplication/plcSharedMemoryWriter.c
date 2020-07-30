/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Creating a shared memory with a size to accodomate triple buffer to hold
 * the data. Then continuosly write a randam double data into the shared memory. */

// Include triple buffer and shared memory headers
#include "tripleBufferWrite.h"
#include "sharedMemoryWrite.h"

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

/* Creating Shared Memory
 * ^^^^^^^^^^^^^^^^^^^^^^
 * This function either creates or opens a shared memory with the key 
 * "PLCSharedMemory" and truncates its size to hold triple buffers of 100 bytes
 * each. This further invokes the triple buffer initialisation function to 
 * set values for read, write and clean buffer indexes. */
void createSharedMemory()
{
    int shmFd;
    void *pshmAddr;
    shmFd = shm_open(sharedMemoryObj , O_CREAT | O_RDWR, 0666);
    ftruncate(shmFd, (3 * sharedMemorySize) + sizeof(tripleBufferWrite));
    pshmAddr = mmap(0, (3 * sharedMemorySize) + sizeof(tripleBufferWrite), PROT_WRITE | PROT_READ, MAP_SHARED, shmFd, 0);
    tripleBufferInit(pshmAddr, sharedMemorySize);
}

int main ()
{
    double dataToSend;
    createSharedMemory();

    while(1)
    {
        // Random data generation
	    dataToSend = (double)(rand()%(1000-1)+ (double)rand() / (double)((unsigned)RAND_MAX + 1));
        
        // Function call to write the randomly generated data into the shared memory
        writeToBuffer(&dataToSend, sizeof(dataToSend));

        printf("Data written into Shared memory: %lf\n", dataToSend);

        // Sleep for 5 seconds
        sleep(5);
    }

    return 0;

}
