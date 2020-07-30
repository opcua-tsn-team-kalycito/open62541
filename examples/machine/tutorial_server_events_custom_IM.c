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

 /* Setting up an event
 * ^^^^^^^^^^^^^^^^^^^
 * In order to set up the event, we can first use ``UA_Server_createEvent`` to give us a node representation of the event.
 * All we need for this is our `EventType`. Once we have our event node, which is saved internally as an `ObjectNode`,
 * we can define the attributes the event has the same way we would define the attributes of an object node. It is not
 * necessary to define the attributes `EventId`, `ReceiveTime`, `SourceNode` or `EventType` since these are set
 * automatically by the server. In this example, we will be setting the fields 'Message', 'Severity' and `Time`.
 */
static UA_StatusCode
setUpEvent(UA_Server *server, UA_NodeId *outId, UA_NodeId methodNodeId) {

    // NodeId of SimpleEventType ObjectType created under BaseEventType
    UA_NodeId eventType = UA_NODEID_NUMERIC(2, 2001);


    // Create an event with server instance and event type input and output the event id
    UA_StatusCode retval = UA_Server_createEvent(server, eventType, outId);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                       "createEvent failed. StatusCode %s", UA_StatusCode_name(retval));
        return retval;
    }
	
	else {
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "\n Event created in the server with nodeid : %d \n", outId->identifier.numeric); 
	}

    // Set the Event Properties
    // Setting the time in which the event has occured. An instance for UA_DateTime structure
    // is created and it is assigned to UA_DateTime_now function which returns the current
    // system time

    UA_DateTime eventTime = UA_DateTime_now();
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Time"),
                                         &eventTime, &UA_TYPES[UA_TYPES_DATETIME]);

    // Severity value for an event can be set as a number between 1 to 1000
    // Since we are creating a sample event, assigning a value of 100 to severity

    UA_UInt16 eventSeverity = 100;
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Severity"),
                                         &eventSeverity, &UA_TYPES[UA_TYPES_UINT16]);
    
    // Set the event message
    UA_LocalizedText eventMessage =
    UA_LOCALIZEDTEXT("en-US", "The motor has Started.");
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Message"),
                                    	&eventMessage, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);

    // SourceName indicates the Node which has caused the event to trigger
    // Read the display name of the method node to be set as SourceName

    UA_LocalizedText displayName;
    retval = UA_Server_readDisplayName(server, methodNodeId, &displayName);
    UA_String eventSourceName = displayName.text;
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
static UA_StatusCode
generateEvent(UA_Server *server, UA_NodeId methodNodeId) {
    UA_StatusCode retval;
 
    // Set up event
    UA_NodeId eventNodeId;
    retval = setUpEvent(server, &eventNodeId, methodNodeId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Creating event failed. StatusCode %s", UA_StatusCode_name(retval));
    }
	
	//Triger the event to send event notification
    retval = UA_Server_triggerEvent(server, eventNodeId,
                                    UA_NODEID_NUMERIC(2, 3902),
                                    NULL, UA_TRUE);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Triggering event failed. StatusCode %s", UA_StatusCode_name(retval));
    }
	else {
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "\n Event has been triggered \n");
    } 
    return retval;
}

/* Start Method callback function
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * Call back function to be executed once the client request for executing the 'StartMotorMethod'*/
static UA_StatusCode
startMotorMethodCallback(UA_Server* server, const UA_NodeId* sessionId, void* sessionHandle,
                 const UA_NodeId* methodId, void* methodContext,
                 const UA_NodeId* objectId, void* objectContext, size_t inputSize,
                 const UA_Variant* input, size_t outputSize, UA_Variant* output) {

    UA_StatusCode retval;
    UA_NodeId startMotorMethodNodeId = UA_NODEID_NUMERIC(2, 23811);

    // Pass the server instance and method nodeID to the user defined function
    retval = generateEvent(server, startMotorMethodNodeId);

    return retval;
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    UA_StatusCode retval = namespace_machineim_generated(server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Adding the MachineIM namespace failed. Please check previous error output.");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    // Set callback for the 'StartMotorMethod' call
    retval = UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(2, 23811),
                                              startMotorMethodCallback);
    printf("\nSet callback for the 'StartMotorMethod' call statuscode returned: %s\n",
           UA_StatusCode_name(retval));

    retval = UA_Server_run(server, &running);
    UA_Server_delete(server);

    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
