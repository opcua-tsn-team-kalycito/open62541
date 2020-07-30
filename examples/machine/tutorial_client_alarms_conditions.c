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
#include <string.h>

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

UA_ByteString eventIdentifier ;

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
        else if (UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_BYTESTRING])) {
             UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "EventID: ");
             UA_ByteString buffer = *(UA_ByteString *)eventFields[i].data;
             UA_ByteString_allocBuffer(&eventIdentifier, 16);
             memcpy(eventIdentifier.data,buffer.data, buffer.length);
             for(int iteration = 0; iteration < 16; iteration++){
             printf("%.2x", eventIdentifier.data[iteration] );
             }
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
    
const size_t nSelectClauses = 5;
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

    selectClauses[0].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    selectClauses[0].browsePathSize = 1;
    selectClauses[0].browsePath = (UA_QualifiedName*)
        UA_Array_new(selectClauses[0].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[0].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[0].attributeId = UA_ATTRIBUTEID_VALUE;
    selectClauses[0].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Message");

    selectClauses[1].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    selectClauses[1].browsePathSize = 1;
    selectClauses[1].browsePath = (UA_QualifiedName*)
        UA_Array_new(selectClauses[1].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[1].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[1].attributeId = UA_ATTRIBUTEID_VALUE;
    selectClauses[1].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Severity");

    selectClauses[2].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    selectClauses[2].browsePathSize = 1;
    selectClauses[2].browsePath = (UA_QualifiedName*)
        UA_Array_new(selectClauses[1].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[2].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[2].attributeId = UA_ATTRIBUTEID_VALUE;
    selectClauses[2].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "SourceName");

    selectClauses[3].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    selectClauses[3].browsePathSize = 1;
    selectClauses[3].browsePath = (UA_QualifiedName*)
        UA_Array_new(selectClauses[1].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[3].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[3].attributeId = UA_ATTRIBUTEID_VALUE;
    selectClauses[3].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Time");

    selectClauses[4].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    selectClauses[4].browsePathSize = 1;
    selectClauses[4].browsePath = (UA_QualifiedName*)
        UA_Array_new(selectClauses[1].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[4].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[4].attributeId = UA_ATTRIBUTEID_VALUE;
    selectClauses[4].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "EventId");
    return selectClauses;
}
#endif

/* Acknowledge Method */
UA_StatusCode callAcknowledgeMethod(UA_Client *client, UA_NodeId objectId, UA_ByteString eventId,
                                            UA_LocalizedText comment) {
    UA_Variant input[2];
    size_t inputSize = 2;
    UA_Variant_init(&input[0]);
    UA_Variant_setScalarCopy(&input[0], &eventId, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_init(&input[1]);
    UA_Variant_setScalarCopy(&input[1], &comment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    size_t outputSize = 0;
    UA_Variant *output = NULL;
    UA_StatusCode retval = UA_Client_call(client, objectId, UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE_ACKNOWLEDGE),
                                    inputSize, &input[0], &outputSize, &output);
    return retval;
}

/* Confirm Method */
UA_StatusCode callConfirmMethod(UA_Client *client, UA_NodeId objectId, UA_ByteString eventId,
                                            UA_LocalizedText comment) {
    UA_Variant input[2];
    size_t inputSize = 2;
    UA_Variant_init(&input[0]);
    UA_Variant_setScalarCopy(&input[0], &eventId, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_init(&input[1]);
    UA_Variant_setScalarCopy(&input[1], &comment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    size_t outputSize = 0;
    UA_Variant *output = NULL;
    UA_StatusCode retval = UA_Client_call(client, objectId, UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE_CONFIRM),
                                    inputSize, &input[0], &outputSize, &output);
    return retval;
}


/* Method call
 * ^^^^^^^^^^^
 * Based on the option choose by the user, the nodeId of 'enable' call or
 * 'activate' call will be provided as an input to this function. The respective
 * method will be called from the client */
static void methodCall(UA_Client *client, UA_NodeId nodeId, UA_NodeId ObjId) {
    // Set up the request
    UA_CallRequest request;
    UA_CallRequest_init(&request);
    UA_CallMethodRequest item;
    UA_CallMethodRequest_init(&item);

    // Assign the nodeId of th respective method call
    item.methodId = nodeId;

    // Assign the object nodeId to which the method belongs to
    // In our case it is the nodeId of condition1
    item.objectId = ObjId;

    request.methodsToCall = &item;
    request.methodsToCallSize = 1;

    UA_CallResponse response;

    // Call the service
    response = UA_Client_Service_call(client, request);
    if(response.results->statusCode != UA_STATUSCODE_GOOD) {
        printf("\nMethod call is unsuccessful: %s\n", UA_StatusCode_name(response.results->statusCode));
    }
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
    UA_UInt16 objMonitorNamespace = 0;
    UA_UInt32 objMonitorNodeId = 0;
    printf("\nCreating a Monitored Item under this Subscription to monitor events generated in object\n");
    printf("\nInput the namespace and node ID of the Object you wish to monitor for event notification\n");
    scanf("%hd %d", &objMonitorNamespace, &objMonitorNodeId);
    // UA_MonitoredItemCreateResult is a structure holding the monitoredItem Id, sampling interval,
    // queue size and filter
    // Create a instance to that structure
    UA_MonitoredItemCreateResult result;

    /* Add a MonitoredItem */
    UA_MonitoredItemCreateRequest monItem;
    UA_MonitoredItemCreateRequest_init(&monItem);
    monItem.itemToMonitor.nodeId = UA_NODEID_NUMERIC(objMonitorNamespace, objMonitorNodeId);
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
                    "Monitoring 'Root->Objects->ConditionSourceObject', id %u", subResponse.subscriptionId);
    }

cleanup:
    // Delete the subscription
    UA_CreateSubscriptionResponse_clear(&subResponse);
    UA_MonitoredItemCreateResult_clear(&result);
#endif
}
static void readAlarmStatus(UA_Client *client)
{
    UA_Variant ActiveStatevalue; 
    UA_Variant_init(&ActiveStatevalue);    
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, 53914);
    // Read the value stored in the variable node
    UA_StatusCode retval = UA_Client_readValueAttribute(client, nodeId, &ActiveStatevalue);
    UA_LocalizedText ActiveStateVar = *(UA_LocalizedText*)ActiveStatevalue.data;

    UA_Variant AckedStatevalue; 
    UA_Variant_init(&AckedStatevalue);    
    nodeId = UA_NODEID_NUMERIC(0, 53918);
    // Read the value stored in the variable node
    retval |= UA_Client_readValueAttribute(client, nodeId, &AckedStatevalue);
    UA_LocalizedText AckedStateVar = *(UA_LocalizedText*)AckedStatevalue.data;

    UA_Variant ConfirmedStatevalue; 
    UA_Variant_init(&ConfirmedStatevalue);    
    nodeId = UA_NODEID_NUMERIC(0, 53940);
    // Read the value stored in the variable node
    retval |= UA_Client_readValueAttribute(client, nodeId, &ConfirmedStatevalue);
    UA_LocalizedText ConfirmedStateVar = *(UA_LocalizedText*)ConfirmedStatevalue.data;
    
    if(retval == UA_STATUSCODE_GOOD) {
        printf("\n\nAlarms active state is: %s \n", 
        ActiveStateVar.text.data);
        printf("\nAlarms acknowledged state is: %s \n", 
        AckedStateVar.text.data);
        printf("\nAlarms confirmed state is: %s \n", 
        ConfirmedStateVar.text.data);
}
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    char userOption[5];
    char userOptionMethodCall[5];

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
    
    while(1)
    {
       // Get the users interest on executing the method calls or exiting the application
       printf("\nDo you want perform 'enable', 'activate', 'acknowledge', 'confirm' or exit the application (enable/activate/acknowledge/confirm/exit):\n");
       scanf("%s", userOptionMethodCall);
       if(strcmp(userOptionMethodCall, "enable") == 0) {
          printf("\nEnabling the alarm with enable method call, Input the namespace index and nodeID of the method call \n");
          UA_UInt16 methodCallNamespace = 0;
          UA_UInt32 methodCallNodeId = 0;
          scanf("%hd %d", &methodCallNamespace, &methodCallNodeId);
          printf("\n Input the namespace index and nodeID of the Object containing the method call \n");
          UA_UInt16 ObjNamespace = 0;
          UA_UInt32 ObjNodeId = 0;
          scanf("%hd %d", &ObjNamespace, &ObjNodeId);
          methodCall(client, UA_NODEID_NUMERIC(methodCallNamespace, methodCallNodeId), UA_NODEID_NUMERIC(ObjNamespace, ObjNodeId));
       }

        else if(strcmp(userOptionMethodCall, "activate") == 0) {
           printf("\n Writing true value into the variable Activate Condition 1 to activate the alarm \n");
           UA_Boolean valueToWrite = true;
           UA_Variant *myVariant = UA_Variant_new();
           UA_Variant_setScalarCopy(myVariant, &valueToWrite, &UA_TYPES[UA_TYPES_BOOLEAN]);
           printf("\n Input the namespace index and node ID of 'Activate Condition 1' variable\n");
           UA_UInt16 VarNamespace = 0;
           UA_UInt32 VarNodeId = 0;
           scanf("%hd %d", &VarNamespace, &VarNodeId);
           retval = UA_Client_writeValueAttribute(client, UA_NODEID_NUMERIC(VarNamespace, VarNodeId), myVariant);
        }

        else if(strcmp(userOptionMethodCall, "acknowledge") == 0) {
           printf("\n Input the namespace index and nodeID of the Object containing the method call\n");
           UA_UInt16 ObjNamespace = 0;
           UA_UInt32 ObjNodeId = 0;
           scanf("%hd %d", &ObjNamespace, &ObjNodeId);
           UA_NodeId objectId = UA_NODEID_NUMERIC(ObjNamespace, ObjNodeId);
           printf("\nInput the comment that you wish to provide for the acknowledge method\n");
           char userComment[100];
           scanf("%s", userComment);
           UA_LocalizedText comment = UA_LOCALIZEDTEXT("en-US", userComment);
           callAcknowledgeMethod(client, objectId, eventIdentifier, comment);
        }

        else if(strcmp(userOptionMethodCall, "confirm") == 0) {
           printf("\n Input the namespace index and nodeID of the Object containing the method call\n");
           UA_UInt16 ObjNamespace = 0;
           UA_UInt32 ObjNodeId = 0;
           scanf("%hd %d", &ObjNamespace, &ObjNodeId);
           UA_NodeId objectId = UA_NODEID_NUMERIC(ObjNamespace, ObjNodeId);
           printf("\nInput the comment that you wish to provide for the confirm method\n");
           char userComment[100];
           scanf("%s", userComment);
           UA_LocalizedText comment = UA_LOCALIZEDTEXT("en-US", userComment);
           callConfirmMethod(client, objectId, eventIdentifier, comment);
        }
    
        else {
           printf("\nExiting the application\n");
           goto cleanup;
        }

        sleep(1);
        retval = UA_Client_run_iterate(client, 100);
        readAlarmStatus(client);
    }

cleanup:
    // Disconnect the client from the server
    UA_Client_disconnect(client);

    // Delete the client instance
    UA_Client_delete(client);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

