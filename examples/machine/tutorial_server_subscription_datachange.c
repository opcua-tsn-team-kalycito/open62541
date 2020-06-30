/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Building a Simple Server
 * ------------------------
 *
 * This series of tutorial guide you through your first steps with open62541.
 * For compiling the examples, you need a compiler (MS Visual Studio 2015 or
 * newer, GCC, Clang and MinGW32 are all known to be working). The compilation
 * instructions are given for GCC but should be straightforward to adapt.
 *
 * It will also be very helpful to install an OPC UA Client with a graphical
 * frontend, such as UAExpert by Unified Automation. That will enable you to
 * examine the information model of any OPC UA server.
 *
 * To get started, downdload the open62541 single-file release from
 * http://open62541.org or generate it according to the :ref:`build instructions
 * <building>` with the "amalgamation" option enabled. From now on, we assume
 * you have the ``open62541.c/.h`` files in the current folder. Now create a new
 * C source-file called ``myServer.c`` with the following content: */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include "open62541/namespace_machineim_generated.h"
#include "../src/server/ua_server_internal.h"
#include "sharedMemoryRead.h"
#include "tripleBufferRead.h"

#include <signal.h>
#include <stdlib.h>

static volatile UA_Boolean running = true;

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

/* Read data from shared memory
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * This function reads the data from shared memory and sets the value to
 * temperature variable present in the custom machine information model. */
static UA_StatusCode
readSharedMemoryData (UA_Server *server,
             const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *nodeId, void *nodeContext,
             UA_Boolean sourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
// If it is POSIX architecture we can read the value from shared memory created
// between the PLC application and open62541 tutorial or else will assign a 
// value of zero
#ifdef UA_ARCHITECTURE_POSIX
    // Create a double variable to store the data read from the shared memory
        UA_Double dataRead = 0;

    // Function call to read the data from the shared memory
    // It reads the data and stores it in the dataRead variable
        readFromBuffer(&dataRead, sizeof(dataRead));

    // Prints the data read from the shared memory
    printf("Data from Shared Memory: %f\n", dataRead);
#else
    // Assigns Zero is it is not read from Shared memory
    UA_Double dataRead = 0;
#endif
    // Sets the value read from the shared memory to the value field of temperature variable
    UA_Variant_setScalarCopy(&value->value, &dataRead, &UA_TYPES[UA_TYPES_DOUBLE]);

    // Sets this value to true, indicating a updated value to the variable
    value->hasValue = true;

    // If this is set true, it sets the source timestamp value in the returned value
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }

   return UA_STATUSCODE_GOOD;
}

/* Add Temperature sensor as a data source variable
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * This function sets temperature sensor variable as a data source variable. It also
 * registers the read and write function callback which has to be executed whenever
 * there is a request from the client to read or write the data.  */
static void
setTemperatureSensorDataSourceVariable(UA_Server *server) {
    // Create a instance of UA_DataSource structure
    UA_DataSource sharedMemDataSource;

    // Assign the read and write callback functions to the created instance
    sharedMemDataSource.read = readSharedMemoryData;
    sharedMemDataSource.write = NULL;

    // Function used to set a variable already available in the information model
    // as a data source variable
    setVariableNode_dataSource(server, UA_NODEID_NUMERIC(2, 17421), sharedMemDataSource);
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

#ifdef UA_ARCHITECTURE_POSIX
    // Function call the create a shared memory region
    createSharedMemory();
#endif

    // Create a new server instance
    UA_Server *server = UA_Server_new();

    // Creates a server config on the default port 4840 with no server certificate
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    // Function call to add the custom machine information model into the server
    // address space
    UA_StatusCode retval = namespace_machineim_generated(server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Adding the MachineIM namespace failed. Please check previous error output.");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    // Set Temperature sensor as a Data source variable
    setTemperatureSensorDataSourceVariable(server);

    // Function to keep the server continously running
    retval = UA_Server_run(server, &running);

    // Delete the created server instance
    UA_Server_delete(server);

    // Delete the shared memory
    sharedMemoryDelete();
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
