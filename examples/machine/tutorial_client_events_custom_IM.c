/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Building a Simple Client
 * ------------------------
 * You should already have a basic server from the previous tutorials. open62541
 * provides both a server- and clientside API, so creating a client is as easy as
 * creating a server. Copy the following into a file `myClient.c`: */

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

static UA_Boolean running = true;
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}


#ifdef UA_ENABLE_SUBSCRIPTIONS

static void
handler_events(UA_Client *client, UA_UInt32 subId, void *subContext,
               UA_UInt32 monId, void *monContext,
               size_t nEventFields, UA_Variant *eventFields) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Notification");

    /* The context should point to the monId on the stack */
    UA_assert(*(UA_UInt32*)monContext == monId);

    for(size_t i = 0; i < nEventFields; ++i) {
        if(UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_UINT16])) {
            UA_UInt16 severity = *(UA_UInt16 *)eventFields[i].data;
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Severity: %u", severity);
        } else if (UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])) {
            UA_LocalizedText *lt = (UA_LocalizedText *)eventFields[i].data;
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Message: '%.*s'", (int)lt->text.length, lt->text.data);
        } else if (UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_STRING])) {
            UA_String text = *(UA_String *)eventFields[i].data;
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "SourceName: '%s'", (char *)(text.data));
        } else if (UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_DATETIME])) {
            UA_DateTime text = *(UA_DateTime *)eventFields[i].data;
            UA_Int64 tOffset = UA_DateTime_localTimeUtcOffset();
            UA_DateTimeStruct dts = UA_DateTime_toStruct(text + tOffset);
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Time: '%04u-%02u-%02u %02u:%02u:%02u.%03u'", dts.year, dts.month,
                        dts.day, dts.hour, dts.min, dts.sec, dts.milliSec);
        }
        else {
#ifdef UA_ENABLE_TYPEDESCRIPTION
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Don't know how to handle type: '%s'", eventFields[i].type->typeName);
#else
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Don't know how to handle type, enable UA_ENABLE_TYPEDESCRIPTION "
                        "for typename");
#endif
        }
    }
}
    
const size_t nSelectClauses = 4;
static UA_UInt32 monId = 0;

static UA_SimpleAttributeOperand *
setupSelectClauses(void) {
    UA_SimpleAttributeOperand *selectClauses = (UA_SimpleAttributeOperand*)
        UA_Array_new(nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    if(!selectClauses)
        return NULL;

    for(size_t i =0; i<nSelectClauses; ++i) {
        UA_SimpleAttributeOperand_init(&selectClauses[i]);
    }
    //select the event with qualifier name "message" to filter message property of event
    selectClauses[0].typeDefinitionId = UA_NODEID_NUMERIC(2, 2001); //simple event type
    selectClauses[0].browsePathSize = 1;
    selectClauses[0].browsePath = (UA_QualifiedName*)
        UA_Array_new(selectClauses[0].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[0].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[0].attributeId = UA_ATTRIBUTEID_VALUE; //filter out attributeID value present in message
    selectClauses[0].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Message");
    //select the event with qualifier name "severity" to filter severity property of event 
    selectClauses[1].typeDefinitionId = UA_NODEID_NUMERIC(2, 2001);  //simple event type
    selectClauses[1].browsePathSize = 1;
    selectClauses[1].browsePath = (UA_QualifiedName*)
        UA_Array_new(selectClauses[1].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[1].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[1].attributeId = UA_ATTRIBUTEID_VALUE;  //filter out attributeID value present in Severity
    selectClauses[1].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Severity");
    //select the event with qualifier name "SourceName" to filter SourceName property of event 
    selectClauses[2].typeDefinitionId = UA_NODEID_NUMERIC(2, 2001);  //simple event type
    selectClauses[2].browsePathSize = 1;
    selectClauses[2].browsePath = (UA_QualifiedName*)
        UA_Array_new(selectClauses[1].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[2].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[2].attributeId = UA_ATTRIBUTEID_VALUE;  //filter out attributeID value present in SourceName
    selectClauses[2].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "SourceName");
    //select the event with qualifier name "Time" to filter Time property of event 
    selectClauses[3].typeDefinitionId = UA_NODEID_NUMERIC(2, 2001);  //simple event type
    selectClauses[3].browsePathSize = 1;
    selectClauses[3].browsePath = (UA_QualifiedName*)
        UA_Array_new(selectClauses[1].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[3].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[3].attributeId = UA_ATTRIBUTEID_VALUE;   //filter out attributeID value present in Time
    selectClauses[3].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Time");

    return selectClauses;
}
#endif

/* Method call
 * ^^^^^^^^^^^
 * Based on the option choose by the user, the nodeId of 'StartMotorMethod' call or
 * 'StopMotorMethod' call will be provided as an input to this function. The respective
 * method will be called from the client */
static void methodCall(UA_Client *client, UA_NodeId nodeId) {
    // Set up the request
    UA_CallRequest request;
    UA_CallRequest_init(&request);
    UA_CallMethodRequest item;
    UA_CallMethodRequest_init(&item);

    // Assign the nodeId of th respective method call
    item.methodId = nodeId;

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

static void createSubscription(UA_Client *client) {
#ifdef UA_ENABLE_SUBSCRIPTIONS

    printf("\nCreating Subscription with default Publishing interval of 500ms\n");  
    // An instance of the response structure is created to store the reponse contaning the 
    // revised publishing interval and subscription Id
    UA_CreateSubscriptionResponse subResponse;
    // Create a subscription request with the default publishing interval 500ms
    // This value can be changed by using the UA_CreateSubscriptionRequest_init function
    // and assigning the required value to the UA_CreateSubscriptionRequest structure instance
    UA_CreateSubscriptionRequest subRequest = UA_CreateSubscriptionRequest_default();

    // This functions sends a request to the server to create subscription. 
    subResponse = UA_Client_Subscriptions_create(client, subRequest, NULL, NULL, NULL);

    UA_UInt32 subId; 

    if(subResponse.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
    {
        subId = subResponse.subscriptionId;
        printf("\nSubscription created with Subscription Id - %u\n", subId);
    }
    else
    {
       printf("\nCreation of subscription failed, exiting the application\n");
       goto cleanup;
    }
    
    printf("\nWould you like to add an object as a monitored item to monitor event (yes/no):\n");
    char userOption[5];
    scanf("%s", userOption);

    if (strcmp(userOption, "yes") != 0) {
       printf("\nMonitoring item is not created. Hence terminating the application\n");
       goto cleanup;
    }
    UA_UInt32 objMonitorNodeId = 0;
    printf("\nCreating a Monitored Item under this Subscription to monitor events generated in object\n");
    printf("\nInput the node ID of the Object you wish to monitor for event notification\n");
    scanf("%d", &objMonitorNodeId);
    // UA_MonitoredItemCreateResult is a structure holding the monitoredItem Id, sampling interval,
    // queue size and filter
    // Create a instance to that structure
    UA_MonitoredItemCreateResult result;

    /* Add a MonitoredItem */
    UA_MonitoredItemCreateRequest monItem;
    UA_MonitoredItemCreateRequest_init(&monItem);
    monItem.itemToMonitor.nodeId = UA_NODEID_NUMERIC(2, objMonitorNodeId); // Root->Objects->Machine-1
    monItem.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    monItem.monitoringMode = UA_MONITORINGMODE_REPORTING;

    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    filter.selectClauses = setupSelectClauses();
    filter.selectClausesSize = nSelectClauses;

    monItem.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    monItem.requestedParameters.filter.content.decoded.data = &filter;
    monItem.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_EVENTFILTER];

    result = UA_Client_MonitoredItems_createEvent(client, subId,
                                             UA_TIMESTAMPSTORETURN_BOTH, monItem,
                                             &monId, handler_events, NULL);

    if(result.statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Could not add the MonitoredItem with %s", UA_StatusCode_name(result.statusCode));
        goto cleanup;
    } else {
        monId = result.monitoredItemId;
        printf("\nMonitoring item created successfully with monitoredItemId %d\n", monId);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Monitoring 'Root->Objects->Machine-1', id %u", subResponse.subscriptionId);
    }

cleanup:
    // Delete the subscription
    UA_CreateSubscriptionResponse_clear(&subResponse);
    UA_MonitoredItemCreateResult_clear(&result);
#endif
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    char userOption[5];

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
    }

    printf("\nDo you want to create subscription in the server (yes/no):\n");
    scanf("%s", userOption);

    if (strcmp(userOption, "yes") == 0) {
        createSubscription(client);
    }
    else {
        printf("\nSubscription is not created. Hence terminating the application\n");
        goto cleanup;
    }

    printf("\nEvents in server will be generated once 'StartMotorMethod'is called from the client\n");

    // Get the users interest on executing the method calls or exiting the application
    printf("\nDo you want to perform 'StartMotorMethod' (yes/no):\n");
    scanf("%s", userOption);
    if(strcmp(userOption, "yes") == 0) {
        //Execute start method call
        methodCall(client, UA_NODEID_NUMERIC(2, 23811));
    }
    else {
        printf("\nExiting the application\n");
        goto cleanup;
    }
    //Sleep to keep the connection alive until we receive event notificaiton
    sleep(1);
    retval = UA_Client_run_iterate(client, 100);

cleanup:
    // Disconnect the client from the server
    UA_Client_disconnect(client);

    // Delete the client instance
    UA_Client_delete(client);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

