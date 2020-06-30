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

#include <signal.h>
#include <stdlib.h>

#ifdef _MSC_VER 
#pragma warning(                                                                         \
    disable : 4996)  // warning C4996: 'UA_Client_Subscriptions_addMonitoredEvent': was
                     // declared deprecated
#endif

#ifdef __clang__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

static UA_Boolean running = true;
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

#ifdef UA_ENABLE_SUBSCRIPTIONS
static void
handler_events_datachange(UA_Client *client, UA_UInt32 subId, void *subContext,
                          UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    printf("\n The value has changed. Updated value as of now is %lf \n",
           *((UA_Double *)(value->value.data)));
}
#endif

static void usage(void) {
    UA_LOG_WARNING(
        UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "Usage:\n"
#ifdef UA_ARCHITECTURE_WIN32
        "tutorial_client_events.exe <EndpointURL> <VariableNodeID>\n"
#endif
#ifdef UA_ARCHITECTURE_POSIX
        "./tutorial_client_events <EndpointURL> <VariableNodeID>\n"
#endif
    );
}

int main(int argc, char *argv[]) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    if(argc == 1) {
        usage();
        return EXIT_SUCCESS;
    }

    if(strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)  {
        usage();
        return EXIT_SUCCESS;
    }

    // Create a new client instance
    UA_Client *client = UA_Client_new();

    // Create a client configuration with default setting
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    // Connect to the server (server URL is provided as an argument to establish connection)
    // First a SecureChannel is opened, then a Session
    UA_StatusCode retval = UA_Client_connect(client, argv[1]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

#ifdef UA_ENABLE_SUBSCRIPTIONS
    // UA_MonitoredItemCreateResult is a structure holding the monitoredItem Id, sampling interval,
    // queue size and filter
    // Create a instance to that structure
    UA_MonitoredItemCreateResult result;

    // An instance of the response structure is created to store the reponse contaning the 
    // revised publishing interval and subscription Id
    UA_CreateSubscriptionResponse response;

    // Create a subscription request with the default publishing interval 500ms
    // This value can be changed by using the UA_CreateSubscriptionRequest_init function
    // and assigning the required value to the UA_CreateSubscriptionRequest structure instance
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();

    // This functions sends a request to the server to create subscription. 
    response = UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    
    UA_UInt32 subId = response.subscriptionId;

    if(response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
        printf("Create subscription succeeded, id %u\n", subId);

    // Send a request to create monitored item. A request with Node Id of the variable
    // to be monitored is send
    UA_MonitoredItemCreateRequest monRequest = 
              UA_MonitoredItemCreateRequest_default(
                        UA_NODEID_NUMERIC(2, (UA_UInt32)atoi(argv[2])));

    // Send a request to create monitored item. A request with subscription Id,
    // default parameters of monitored item, callback function to be executed for
    // each and every publishing interval is send to the server
    result = UA_Client_MonitoredItems_createDataChange(
        client, response.subscriptionId, UA_TIMESTAMPSTORETURN_BOTH, monRequest, NULL,
        handler_events_datachange, NULL);

    if(result.statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Could not add the MonitoredItem with %s",
                    UA_StatusCode_name(retval));
        goto cleanup;
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Monitoring 'Root->Objects->Server', id %u",
                    response.subscriptionId);
    }
 
    // While loop to keep the client alive untill a signal handler is detected
    while(running)
        retval = UA_Client_run_iterate(client, 100);

cleanup:
    // Delete the subscription
    UA_CreateSubscriptionResponse_clear(&response);
    UA_MonitoredItemCreateResult_clear(&result);
#endif

    // Disconnect the client from the server
    UA_Client_disconnect(client);

    // Delete the client instance
    UA_Client_delete(client);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
