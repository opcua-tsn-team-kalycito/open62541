/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

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

/* Start Method callback function
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * Call back function to be executed once the client request for executing the 'StartMotorMethod'.
 * Change the 'Executable' attribute of 'StartMotorMethod to 'false' 
 * and 'StopMotorMethod' to 'true'. Also set the value of 'MotorRunningStatus' to 'true'.*/
static UA_StatusCode
startMotorMethodCallback(UA_Server* server, const UA_NodeId* sessionId, void* sessionHandle,
                 const UA_NodeId* methodId, void* methodContext,
                 const UA_NodeId* objectId, void* objectContext, size_t inputSize,
                 const UA_Variant* input, size_t outputSize, UA_Variant* output) {
    // NodeId of MotorRunningStatus node in the server address space
    UA_NodeId motorRunningStatusNodeId = UA_NODEID_NUMERIC(2, 153);
    
    // Value to be stored in MotorRunningStatus node in the address space of the server
    UA_Boolean motorCurrentStatus = true;

    // UA_Variant instance to hold the value to be stored in MotorRunningStatus node
    // in the address space of the server
    UA_Variant motorRunningStatusVar;
    UA_Variant_init(&motorRunningStatusVar);
    UA_Variant_setScalar(&motorRunningStatusVar, &motorCurrentStatus, &UA_TYPES[UA_TYPES_BOOLEAN]);

    // Store true in value attribute of MotorRunningStatus node in the address space of the server
    UA_StatusCode retval = UA_Server_writeValue(server,
                                                motorRunningStatusNodeId,
                                                motorRunningStatusVar);
    printf("\n'Store value attribute of MotorRunningStatus as true after "
           "Start method call' -> StatusCode returned: %s\n",
           UA_StatusCode_name(retval));

    // Initialize 'Executable' attribute status of 'Start' method as 'false'
    retval = UA_Server_writeExecutable(server, UA_NODEID_NUMERIC(2, 23811), false);
        
    printf("\n'Store Executable attribute status of Start method as "       
           "false after Start method call' -> StatusCode returned: %s\n",       
           UA_StatusCode_name(retval));     
        
    // Initialize 'Executable' attribute status of 'Stop' method as 'true'      
    retval = UA_Server_writeExecutable(server, UA_NODEID_NUMERIC(2, 17035), true);
    printf("\n'Store Executable attribute status of Stop method as "
           "true after Start method call' -> StatusCode returned: %s\n",
           UA_StatusCode_name(retval));

    return retval;
}

/* StopMotorMethod callback function
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * Call back function to be executed once the client request for executing the 'StopMotorMethod'.
 * Change the 'Executable' attribute of 'StopMotorMethod to 'false' 
 * and 'StartMotorMethod' to 'true'. Also set the value of 'MotorRunningStatus' to 'false'.*/
static UA_StatusCode
stopMotorMethodCallback(UA_Server *server, const UA_NodeId *sessionId, void *sessionHandle,
                    const UA_NodeId *methodId, void *methodContext,
                    const UA_NodeId *objectId, void *objectContext, size_t inputSize,
                    const UA_Variant *input, size_t outputSize, UA_Variant *output) {
    // NodeId of MotorRunningStatus node in the server address space
    UA_NodeId motorRunningStatus = UA_NODEID_NUMERIC(2, 153);
    
    // Value to be stored in MotorRunningStatus node in the address space of the server     
    UA_Boolean motorCurrentStatus = false;

    // UA_Variant instance to hold the value to be stored in MotorRunningStatus node
    // in the address space of the server
    UA_Variant motorRunningStatusVar;
    UA_Variant_init(&motorRunningStatusVar);
    UA_Variant_setScalar(&motorRunningStatusVar, &motorCurrentStatus, &UA_TYPES[UA_TYPES_BOOLEAN]);

    // Store false in value attribute of MotorRunningStatus node in the address space of the server
    UA_StatusCode retval = UA_Server_writeValue(server,     
                                                motorRunningStatus,     
                                                motorRunningStatusVar);
        
    printf("\n'Store value attribute of MotorRunningStatus as false after "     
           "Stop method call' -> StatusCode returned: %s\n",        
           UA_StatusCode_name(retval));
        
        
    // Initialize 'Executable' attribute status of 'Stop' method as 'false'     
    retval = UA_Server_writeExecutable(server, UA_NODEID_NUMERIC(2, 17035), false);
    printf("\n'Store Executable attribute status of Stop method as "
           "false after Stop method call' -> StatusCode returned: %s\n",
           UA_StatusCode_name(retval));

    // Initialize 'Executable' attribute status of 'Start' method as 'true' 
    retval = UA_Server_writeExecutable(server, UA_NODEID_NUMERIC(2, 23811), true);
    printf("\n'Store Executable attribute status of Start method as "
           "true after Stop method call' -> StatusCode returned: %s\n",
           UA_StatusCode_name(retval));
    
    return retval;
}

/* MotorRunningStatus variable
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * Initialize the value for 'MotorRunningStatus' variable to 'false' to indicate the motor
 * is not yet started during the bootup. */
static void initMotorRunningStatus(UA_Server *server) {
    // NodeId of MotorRunningStatus
    UA_NodeId motorRunningStatus = UA_NODEID_NUMERIC(2, 153);
    UA_Variant myWriteVar;
    UA_Boolean motorCurrentStatus = false;
    UA_Variant_init(&myWriteVar);
    UA_Variant_setScalar(&myWriteVar, &motorCurrentStatus, &UA_TYPES[UA_TYPES_BOOLEAN]);

    // Store MotorRunningStatus as false at boot up of server
    UA_StatusCode retval = UA_Server_writeValue(server, motorRunningStatus, myWriteVar);
    printf("\nStore MotorRunningStatus as 'false' at boot up of server "
           "statuscode returned: %s\n", UA_StatusCode_name(retval));
}

/* Executable attribute
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * Initialize the 'Executable' attribute of 'StartMotorMethod' as 'true' to help the client
 * understand that the 'StartMotorMethod' can be executed and 'StopMotorMethod' as 'false' 
 * to help the client understand that the 'StopMotorMethod' cannot be executed */
static void initExecutableAttribute(UA_Server *server) {
    // Initialize 'Exectable' attribute of 'StartMotorMethod' as 'true' at boot up of server
    UA_StatusCode retval = UA_Server_writeExecutable(server, UA_NODEID_NUMERIC(2, 23811), true);
    if(retval == UA_STATUSCODE_GOOD) {
        printf("\nInitialized 'Executable' attribute of 'StartMotorMethod' as 'true' "
               "at boot up of server statuscode returned\n");
    }
    else {
        printf("\n'Executable' attribute of 'StartMotorMethod' is not set to true\n");
    }

    // Initialize 'Exectable' attribute of 'StopMotorMethod' as 'false' at boot up of server
    retval = UA_Server_writeExecutable(server, UA_NODEID_NUMERIC(2, 17035), false);
    if(retval == UA_STATUSCODE_GOOD) {
        printf("\nInitialized 'Executable' attribute of 'StopMotorMethod' as 'false' "
               "at boot up of server statuscode returned\n");
    }
    else {
        printf("\n'Executable' attribute of 'StopMotorMethod' is not set to false\n");
    }

}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    UA_StatusCode retval = namespace_machineim_generated(server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Adding the MachineIM namespace failed. Please check previous error output.");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    // Initialise the 'MotorRunningStatus' variable to false during the bootup
    initMotorRunningStatus(server);

    // Initialise the 'Executable' attribute of 'StartMotorMethod' to true and
    // 'StopMotorMethod' to false
    initExecutableAttribute(server);

    // Set callback for the 'StartMotorMethod' call
    retval = UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(2, 23811), startMotorMethodCallback);
    printf("\nSet callback for the 'StartMotorMethod' call statuscode returned: %s\n",
           UA_StatusCode_name(retval));

    // Set callback for the 'StopMotorMethod' call
    retval = UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(2, 17035), stopMotorMethodCallback);
    printf("\nSet callback for the 'StopMotorMethod' call statuscode returned: %s\n", UA_StatusCode_name(retval));

    retval = UA_Server_run(server, &running);
    UA_Server_delete(server);

    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
