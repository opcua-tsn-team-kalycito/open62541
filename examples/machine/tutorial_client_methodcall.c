/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/util.h>

#include <stdlib.h>

#ifdef _MSC_VER
#pragma warning(disable:4996) // warning C4996: 'UA_Client_Subscriptions_addMonitoredEvent': was declared deprecated
#endif

#ifdef __clang__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

/* Read Executable Attribute
 * ^^^^^^^^^^^^^^^^^^^^^^^^^
 * Read the executable attribute of 'StartMotorMethod' and 'StopMotorMethod' and 
 * display the same. */
static void readExecutableAttribute(UA_Client *client) {
    UA_Boolean start;

    // Node Id of the method node whose 'Executable' attribute is to be read 
    // from the address space of the server
    UA_NodeId nodeIdStart = UA_NODEID_NUMERIC(2, 23811);

    // Read the value stored in the variable node
    UA_StatusCode retval = UA_Client_readExecutableAttribute(client, nodeIdStart, &start);
    if(retval == UA_STATUSCODE_GOOD) {
        printf("\n The value of 'Executable' attribute of 'StartMotorMethod' is: %d \n", start);
    }
    else {
        printf("\nFailed to read 'Executable' attribute of 'StartMotorMethod'\n");
    }

    UA_Boolean stop;

    // Node Id of the method node whose 'Executable' attribute is to be read 
    // from the address space of the server
    UA_NodeId nodeIdStop = UA_NODEID_NUMERIC(2, 17035);

    // Read the value stored in the variable node
    retval = UA_Client_readExecutableAttribute(client, nodeIdStop, &stop);
    if(retval == UA_STATUSCODE_GOOD) {
        printf("\n The value of 'Executable' attribute of 'StopMotorMethod' is: %d \n", stop);
    }
    else {
        printf("\nFailed to read 'Executable' attribute of 'StopMotorMethod'\n");
    }
}

/* Method call
 * ^^^^^^^^^^^
 * Based on the option choose by the user, the nodeId of 'StartMotorMethod' call or
 * 'StopMotorMethod' call will be provided as an input to this function. The respective
 * method will be called from the client */
static void methodCall(UA_Client *client, UA_UInt32 nodeId) {
    // Set up the request
    UA_CallRequest request;
    UA_CallRequest_init(&request);
    UA_CallMethodRequest item;
    UA_CallMethodRequest_init(&item);

    // Assign the nodeId of th respective method call
    item.methodId = UA_NODEID_NUMERIC(2, nodeId);

    // Assign the object nodeId to which the method belongs to
    // In our case it is the nodeId of the Machine-1 object 
    item.objectId = UA_NODEID_NUMERIC(2, 3902);

    request.methodsToCall = &item;
    request.methodsToCallSize = 1;

    UA_CallResponse response;

    // Call the service
    response = UA_Client_Service_call(client, request);
    if(response.results->statusCode != UA_STATUSCODE_GOOD) {
        printf("\nMethod call is unsuccessful: %s\n", UA_StatusCode_name(response.results->statusCode));
    }

    UA_CallResponse_deleteMembers(&response);
}

int main(void) {
    char userOptionMethodCall[10];
 
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
    }
    
    while(1) {
        // Read the 'Executable' attribute of the method call to understand how it value
        // changes after each method call is performed
        readExecutableAttribute(client);

        // Get the users interest on executing the method calls or exiting the application
        printf("\nDo you want perform 'StartMethodCall' or 'StopMethodCall' or exit the application (start/stop/exit):\n");
        scanf("%s", userOptionMethodCall);
        if(strcmp(userOptionMethodCall, "start") == 0) {
            methodCall(client, 23811);
        }
        else if(strcmp(userOptionMethodCall, "stop") == 0) {
            methodCall(client, 17035);
        }
        else {
            printf("\nExiting the application\n");
            goto cleanup;
        }
    }

cleanup:
    // Disconnect the client from the server
    UA_Client_disconnect(client);

    // Delete the client instance
    UA_Client_delete(client);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
