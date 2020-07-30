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

#include <open62541/plugin/historydata/history_data_backend_memory.h>
#include <open62541/plugin/historydata/history_data_gathering_default.h>
#include <open62541/plugin/historydata/history_database_default.h>
#include <open62541/plugin/historydatabase.h>

#include <signal.h>
#include <stdlib.h>

static volatile UA_Boolean running = true;

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

// Function to generate random double values for the data source variables
// To be replaced with actual data from shared memory
static UA_StatusCode
readRandomDoubleData (UA_Server *server,
             const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *nodeId, void *nodeContext,
             UA_Boolean sourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_Double toggle = (UA_Double)UA_UInt32_random();
    UA_Variant_setScalarCopy(&value->value, &toggle, &UA_TYPES[UA_TYPES_DOUBLE]);
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }

   return UA_STATUSCODE_GOOD;
}

// Function to generate random double values for the data source variables
// To be replaced with actual data from shared memory
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

#ifdef UA_ARCHITECTURE_POSIX
    //Create a double variable and point to the shared memory location to read data
	UA_Double dataRead = 0;
	readFromBuffer(&dataRead, sizeof(dataRead));
    printf("Data from Shared Memory: %f\n", dataRead);
#else
    UA_Double dataRead = 0;
#endif
    UA_Variant_setScalarCopy(&value->value, &dataRead, &UA_TYPES[UA_TYPES_DOUBLE]);
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }

   return UA_STATUSCODE_GOOD;
}

// Set 'PressureSensor' and 'TemperatureSensor' variables as Datasource variables
static void
addSensorDataSourceVariable(UA_Server *server) {
    UA_DataSource randomDataSource;
    randomDataSource.read = readRandomDoubleData;
    randomDataSource.write = NULL;
    // Machine-1, PressureSensor Datasource variable
    setVariableNode_dataSource(server, UA_NODEID_NUMERIC(2, 14771), randomDataSource);


}

 /* Setting up an event
 * ^^^^^^^^^^^^^^^^^^^
 * In order to set up the event, we can first use ``UA_Server_createEvent`` to give us a node representation of the event.
 * All we need for this is our `EventType`. Once we have our event node, which is saved internally as an `ObjectNode`,
 * we can define the attributes the event has the same way we would define the attributes of an object node. It is not
 * necessary to define the attributes `EventId`, `ReceiveTime`, `SourceNode` or `EventType` since these are set
 * automatically by the server. In this example, we will be setting the fields 'Message' and 'Severity' in addition
 * to `Time` which is needed to make the example UaExpert compliant.
 */
static UA_StatusCode
setUpEvent(UA_Server *server, UA_NodeId *outId) {
    // NodeId of SimpleEventType ObjectType created under BaseEventType
    UA_NodeId eventType = UA_NODEID_NUMERIC(2, 2001);
    UA_StatusCode retval = UA_Server_createEvent(server, eventType, outId);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                       "createEvent failed. StatusCode %s", UA_StatusCode_name(retval));
        return retval;
    }

    /* Set the Event Attributes */
    /* Setting the Time is required or else the event will not show up in UAExpert! */
    UA_DateTime eventTime = UA_DateTime_now();
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Time"),
                                         &eventTime, &UA_TYPES[UA_TYPES_DATETIME]);

    UA_UInt16 eventSeverity = 100;
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Severity"),
                                         &eventSeverity, &UA_TYPES[UA_TYPES_UINT16]);
    
    UA_NodeId motorRunningStatus = UA_NODEID_NUMERIC(2, 153);
    UA_Variant myReadVar;
    UA_Variant_init(&myReadVar);

    // The below 'UA_Server_readValue' API should be replaced with code to read value from
    // a shared memory where the PLC data gets updated
    UA_Server_readValue(server, motorRunningStatus, &myReadVar);

    // Store event message equivalent to the MotorRunningStatus
    if(*(UA_Boolean *)myReadVar.data == false)
    {
        UA_LocalizedText eventMessage =
            UA_LOCALIZEDTEXT("en-US", "Motor Stopped.");
        UA_Server_writeObjectProperty_scalar(
            server, *outId, UA_QUALIFIEDNAME(0, "Message"), &eventMessage,
            &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    }
    else if(*(UA_Boolean *)myReadVar.data == true)
    {
        UA_LocalizedText eventMessage = UA_LOCALIZEDTEXT("en-US", "Motor Started.");
        UA_Server_writeObjectProperty_scalar(
            server, *outId, UA_QUALIFIEDNAME(0, "Message"), &eventMessage,
            &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    }
    else
    {
        UA_LocalizedText eventMessage = UA_LOCALIZEDTEXT("en-US", "Unkown Motor State.");
        UA_Server_writeObjectProperty_scalar(
            server, *outId, UA_QUALIFIEDNAME(0, "Message"), &eventMessage,
            &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    }

    UA_String eventSourceName = UA_STRING("Machine-1");
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "SourceName"),
                                         &eventSourceName, &UA_TYPES[UA_TYPES_STRING]);

    return UA_STATUSCODE_GOOD;
}

/**
 * Triggering an event
 * ^^^^^^^^^^^^^^^^^^^
 * First a node representing an event is generated using ``setUpEvent``. Once our event is good to go, we specify
 * a node which emits the event - in this case the server node. We can use ``UA_Server_triggerEvent`` to trigger our
 * event onto said node. Passing ``NULL`` as the second-last argument means we will not receive the `EventId`.
 * The last boolean argument states whether the node should be deleted. */
static void
generateEventCallback(UA_Server *server, void *data) {
    UA_NodeId motorRunningStatus = UA_NODEID_NUMERIC(2, 153);
    static UA_Boolean motorPreviousStatus = false;
    UA_Boolean motorCurrentStatus;

    UA_Variant myReadVar;
    UA_Variant_init(&myReadVar);

    // The below 'UA_Server_readValue' API should be replaced with code to read value from
    // a shared memory where the PLC data gets updated
    UA_Server_readValue(server, motorRunningStatus, &myReadVar);

    motorCurrentStatus = *(UA_Boolean *)myReadVar.data;

    if(motorPreviousStatus != motorCurrentStatus)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Creating event");
        
        /* set up event */
        UA_NodeId eventNodeId;
        UA_StatusCode retval = setUpEvent(server, &eventNodeId);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                           "Creating event failed. StatusCode %s", UA_StatusCode_name(retval));
        }

        retval = UA_Server_triggerEvent(server, eventNodeId,
                                        UA_NODEID_NUMERIC(2, 3902),
                                        NULL, UA_TRUE);
        if(retval != UA_STATUSCODE_GOOD)
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                           "Triggering event failed. StatusCode %s", UA_StatusCode_name(retval));
    }

    motorPreviousStatus = motorCurrentStatus;
}

// Change
// 'Executable' attribute of 'Start' method to 'false'
// 'Executable' attribute of 'Stop' method to 'true'
// Change MotorRunning Status to 'true'
static UA_StatusCode
startMethodCallback(UA_Server* server, const UA_NodeId* sessionId, void* sessionHandle,
                 const UA_NodeId* methodId, void* methodContext,
                 const UA_NodeId* objectId, void* objectContext, size_t inputSize,
                 const UA_Variant* input, size_t outputSize, UA_Variant* output) {

    UA_NodeId motorRunningStatus = UA_NODEID_NUMERIC(2, 153);
    UA_Boolean motorCurrentStatus = true;

    UA_Variant myWriteVar;
    UA_Variant_init(&myWriteVar);
    UA_Variant_setScalar(&myWriteVar, &motorCurrentStatus, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_StatusCode retval = UA_Server_writeValue(server, motorRunningStatus, myWriteVar);
    printf("Writing a bool returned statuscode %s\n", UA_StatusCode_name(retval));

    retval = UA_Server_writeExecutable(server, UA_NODEID_NUMERIC(2, 23811),
                                                  false);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Start Method call failed. StatusCode %s",
                       UA_StatusCode_name(retval));
        return retval;
    }

    retval = UA_Server_writeExecutable(server, UA_NODEID_NUMERIC(2, 17035), true);
    return retval;
}

// Change 
// 'Executable' attribute of 'Stop' method to 'false'
// 'Executable' attribute of 'Start' method to 'true'
// Change MotorRunning Status to 'false'
static UA_StatusCode
stopMethodCallback(UA_Server *server, const UA_NodeId *sessionId, void *sessionHandle,
                    const UA_NodeId *methodId, void *methodContext,
                    const UA_NodeId *objectId, void *objectContext, size_t inputSize,
                    const UA_Variant *input, size_t outputSize, UA_Variant *output) {

    UA_NodeId motorRunningStatus = UA_NODEID_NUMERIC(2, 153);
    UA_Boolean motorCurrentStatus = false;

    UA_Variant myWriteVar;
    UA_Variant_init(&myWriteVar);
    UA_Variant_setScalar(&myWriteVar, &motorCurrentStatus, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_StatusCode retval = UA_Server_writeValue(server, motorRunningStatus, myWriteVar);
    printf("Writing a bool returned statuscode %s\n", UA_StatusCode_name(retval));

    retval = UA_Server_writeExecutable(server, UA_NODEID_NUMERIC(2, 17035), false);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Stop Method call failed. StatusCode %s",
                       UA_StatusCode_name(retval));
        return retval;
    }

    retval = UA_Server_writeExecutable(server, UA_NODEID_NUMERIC(2, 23811), true);
    return retval;
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

#ifdef UA_ARCHITECTURE_POSIX
    createSharedMemory();
#endif

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    UA_StatusCode retval = namespace_machineim_generated(server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Adding the MachineIM namespace failed. Please check previous error output.");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    UA_HistoryDataGathering gathering = UA_HistoryDataGathering_Default(1);
    config->historyDatabase = UA_HistoryDatabase_default(gathering);

    //NodeID of Temperature sensor 
    UA_NodeId outNodeId = UA_NODEID_NUMERIC(2, 17421);
    
    //Assign TemperatureSensor as Data source variable and input shared memory data for read 
    UA_DataSource sharedMemDataSource;
    sharedMemDataSource.read = readSharedMemoryData;
    sharedMemDataSource.write = NULL;
    //Set Temperature sensor as data source variable
    setVariableNode_dataSource(server, outNodeId, sharedMemDataSource);
    UA_HistorizingNodeIdSettings setting;
    //Setup the backend memory to store data from Temperature sensor
    setting.historizingBackend = UA_HistoryDataBackend_Memory(1,0);
    //Setup the maximum response size
    setting.maxHistoryDataResponseSize = 100;
    //Select update strategy as POLL suitable for dataSource variable
    setting.historizingUpdateStrategy = UA_HISTORIZINGUPDATESTRATEGY_POLL;
    //Configure the polling interval
    setting.pollingInterval = 100;
    //Registed temperatureSensor nodeID to save data in the database
    retval = gathering.registerNodeId(server, gathering.context, &outNodeId, setting);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "registerNodeId %s", UA_StatusCode_name(retval));
     //Start pollingto gather data into the database
    retval = gathering.startPoll(server, gathering.context, &outNodeId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "startPoll %s", UA_StatusCode_name(retval));
    // Assign PressureSensor value with random generated DataSource variable
    addSensorDataSourceVariable(server);
    // User-defined callback to trigger event for every MotorRunningStatus change
    UA_Server_addRepeatedCallback(server, generateEventCallback, NULL, 10000, NULL);

    // NodeId of MotorRunningStatus
    UA_NodeId motorRunningStatus = UA_NODEID_NUMERIC(2, 153);
    UA_Variant myWriteVar;
    UA_Boolean motorCurrentStatus = false;
    UA_Variant_init(&myWriteVar);
    UA_Variant_setScalar(&myWriteVar, &motorCurrentStatus, &UA_TYPES[UA_TYPES_BOOLEAN]);
    // Store MotorRunningStatus as false at boot up of server
    retval = UA_Server_writeValue(server, motorRunningStatus, myWriteVar);
    printf("Store MotorRunningStatus as false at boot up of server statuscode returned: %s\n", UA_StatusCode_name(retval));

    // Initialize 'Exectable' attribute status of 'Stop' method as 'false' at boot up of server
    retval = UA_Server_writeExecutable(server, UA_NODEID_NUMERIC(2, 17035), false);
    printf("Initialize 'Exectable' attribute status of 'Stop' method as 'false' at boot up of server statuscode returned: %s\n", UA_StatusCode_name(retval));

    // Set callback for the 'Start' method call
    retval = UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(2, 23811), startMethodCallback);
    printf("Set callback for the 'Start' method call statuscode returned: %s\n",
           UA_StatusCode_name(retval));

    // Set callback for the 'Stop' method call
    retval = UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(2, 17035), stopMethodCallback);
    printf("Set callback for the 'Stop' method call statuscode returned: %s\n", UA_StatusCode_name(retval));

    retval = UA_Server_run(server, &running);
    //Stop polling historical data as the server is stopped
    retval = gathering.stopPoll(server, gathering.context, &outNodeId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "stopPoll %s", UA_StatusCode_name(retval));
    UA_Server_delete(server);

    // Delete the shared memory
    sharedMemoryDelete();
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
