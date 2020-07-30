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
static void
stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

#ifdef UA_ENABLE_SUBSCRIPTIONS

static void
handler_events(UA_Client *client, UA_UInt32 subId, void *subContext, UA_UInt32 monId,
               void *monContext, size_t nEventFields, UA_Variant *eventFields) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Notification");

    /* The context should point to the monId on the stack */
    UA_assert(*(UA_UInt32 *)monContext == monId);

    for(size_t i = 0; i < nEventFields; ++i) {
        if(UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_UINT16])) {
            UA_UInt16 severity = *(UA_UInt16 *)eventFields[i].data;
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Severity: %u", severity);
        } else if(UA_Variant_hasScalarType(&eventFields[i],
                                           &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])) {
            UA_LocalizedText *lt = (UA_LocalizedText *)eventFields[i].data;
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Message: '%.*s'",
                        (int)lt->text.length, lt->text.data);
        } else {
#ifdef UA_ENABLE_TYPEDESCRIPTION
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Don't know how to handle type: '%s'",
                        eventFields[i].type->typeName);
#else
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Don't know how to handle type, enable UA_ENABLE_TYPEDESCRIPTION "
                        "for typename");
#endif
        }
    }
}

static void
handler_events_datachange(UA_Client *client, UA_UInt32 subId, void *subContext,
                          UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    printf("\n The value has changed \n");
}

const size_t nSelectClauses = 2;

static UA_SimpleAttributeOperand *
setupSelectClauses(void) {
    UA_SimpleAttributeOperand *selectClauses = (UA_SimpleAttributeOperand *)UA_Array_new(
        nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    if(!selectClauses)
        return NULL;

    for(size_t i = 0; i < nSelectClauses; ++i) {
        UA_SimpleAttributeOperand_init(&selectClauses[i]);
    }

    selectClauses[0].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    selectClauses[0].browsePathSize = 1;
    selectClauses[0].browsePath = (UA_QualifiedName *)UA_Array_new(
        selectClauses[0].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[0].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[0].attributeId = UA_ATTRIBUTEID_VALUE;
    selectClauses[0].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Message");

    selectClauses[1].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    selectClauses[1].browsePathSize = 1;
    selectClauses[1].browsePath = (UA_QualifiedName *)UA_Array_new(
        selectClauses[1].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[1].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[1].attributeId = UA_ATTRIBUTEID_VALUE;
    selectClauses[1].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Severity");

    return selectClauses;
}

#endif

static void
usage(void) {
    UA_LOG_WARNING(
        UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "Usage:\n"
#ifdef UA_ARCHITECTURE_WIN32
        "Monitor events win32:\n"
        "tutorial_client_events.exe <EndpointURL> MonitorEvents <ParentNodeID> <MethodNodeID>\n"
        "Monitor data change win32:\n"
        "tutorial_client_events.exe <EndpointURL> MonitorDataChange <VariableNodeID>\n"
#endif
#ifdef UA_ARCHITECTURE_POSIX
        "Monitor events posix:\n"
        "./tutorial_client_events <EndpointURL> MonitorEvents <ParentNodeID> <MethodNodeID>\n"
        "Monitor data change posix:\n"
        "./tutorial_client_events <EndpointURL> MonitorDataChange <VariableNodeID>\n"
#endif
    );
}

int
main(int argc, char *argv[]) {
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

    if(argc < 2) {
        printf("Usage: tutorial_client_events <opc.tcp://server-url>\n");
        return EXIT_FAILURE;
    }

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    /* opc.tcp://uademo.prosysopc.com:53530/OPCUA/SimulationServer */
    /* opc.tcp://opcua.demo-this.com:51210/UA/SampleServer */
    UA_StatusCode retval = UA_Client_connect(client, argv[1]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_MonitoredItemCreateResult result;  //
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    UA_UInt32 subId = response.subscriptionId;
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    filter.selectClauses = setupSelectClauses();
    filter.selectClausesSize = nSelectClauses;

    if(strcmp(argv[2], "MonitorEvents") == 0) {
        if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
            UA_Client_disconnect(client);
            UA_Client_delete(client);
            return EXIT_FAILURE;
        }
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Create subscription succeeded, id %u", subId);

        /* Add a MonitoredItem */
        UA_MonitoredItemCreateRequest item;
        UA_MonitoredItemCreateRequest_init(&item);
        item.itemToMonitor.nodeId = UA_NODEID_NUMERIC(2, 3902);  // Root->Objects->Server
        item.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
        item.monitoringMode = UA_MONITORINGMODE_REPORTING;

        item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
        item.requestedParameters.filter.content.decoded.data = &filter;
        item.requestedParameters.filter.content.decoded.type =
            &UA_TYPES[UA_TYPES_EVENTFILTER];

        UA_UInt32 monId = 0;

        result = UA_Client_MonitoredItems_createEvent(client, subId,
                                                      UA_TIMESTAMPSTORETURN_BOTH, item,
                                                      &monId, handler_events, NULL);

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

        monId = result.monitoredItemId;

        size_t outputSize;
        UA_Variant *output;
        retval = UA_Client_call(client, UA_NODEID_NUMERIC(2, (UA_UInt32)atoi(argv[3])),
                                UA_NODEID_NUMERIC(2, (UA_UInt32)atoi(argv[4])), 0, NULL,
                                &outputSize, &output);
        if(retval == UA_STATUSCODE_GOOD) {
            printf("Method call was successful, and %lu returned values available.\n",
                   (unsigned long)outputSize);
            UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
        } else {
            printf("Method call was unsuccessful, and %x returned values available.\n",
                   retval);
        }
    } else if(strcmp(argv[2], "MonitorDataChange") == 0) {
        if(response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
            printf("Create subscription succeeded, id %u\n", subId);

        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(
            UA_NODEID_NUMERIC(2, (UA_UInt32)atoi(argv[3])));

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
    } else {
        printf("None of Monitor Datachange or Monitor Events selected \n");
    }

    while(running)
        retval = UA_Client_run_iterate(client, 100);

    /* Delete the subscription */
cleanup:
    UA_MonitoredItemCreateResult_clear(&result);
    UA_Client_Subscriptions_deleteSingle(client, response.subscriptionId);
    UA_Array_delete(filter.selectClauses, nSelectClauses,
                    &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
#endif

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
