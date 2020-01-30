/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#include <open62541/plugin/pubsub_ethernet.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <check.h>
#include <time.h>

#include "open62541/types_generated_encoding_binary.h"
#include "ua_pubsub.h"
#include "ua_server_internal.h"
#include "ua_pubsub_networkmessage.h"

#define UA_SUBSCRIBER_PORT                4801    /* Port for Subscriber*/
#define PUBLISH_INTERVAL                  5       /* Publish interval*/
#define PUBLISHER_ID                      2234    /* Publisher Id*/
#define DATASET_WRITER_ID                 62541   /* DataSet Writer Id*/
#define WRITER_GROUP_ID                   100     /* Writer group Id  */
#define PUBLISHER_DATA                    42      /* Published data */
#define PUBLISHVARIABLE_NODEID            1000    /* Published data nodeId */
#define UA_MAX_STACKBUF                   512     /* Max size of network messages on the stack */
#define ETHERNET_INTERFACE                "enp4s0"
#define PUBLISHING_MULTICAST_MAC_ADDRESS  "opc.eth://01-00-5E-7F-00-01"

/* Global declaration for test cases  */
UA_Server *server = NULL;
UA_ServerConfig *config = NULL;
UA_NodeId connection_test;
UA_NodeId readerGroupTest;
UA_NodeId publishedDataSetTest;
UA_WriterGroup      *currentWriterGroupCallback;
UA_StatusCode res = UA_STATUSCODE_GOOD;

/* setup() is to create an environment for test cases */
static void setup(void) {
    /*Add setup by creating new server with valid configuration */
    server = UA_Server_new();
    config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, UA_SUBSCRIBER_PORT, NULL);
    UA_Server_run_startup(server);
    config->pubsubTransportLayers = (UA_PubSubTransportLayer *) UA_malloc(sizeof(UA_PubSubTransportLayer));
    if(!config->pubsubTransportLayers) {
        UA_ServerConfig_clean(config);
    }

    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerEthernet();
    config->pubsubTransportLayersSize++;

    /* Add connection to the server */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("Ethernet Test Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING(ETHERNET_INTERFACE), UA_STRING(PUBLISHING_MULTICAST_MAC_ADDRESS)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
    connectionConfig.publisherId.numeric = PUBLISHER_ID;
    UA_Server_addPubSubConnection(server, &connectionConfig, &connection_test);
}

/* teardown() is to delete the environment set for test cases */
static void teardown(void) {
    /*Call server delete functions */
   UA_Server_run_shutdown(server);
   UA_Server_delete(server);
}

START_TEST(EthernetPublisher) {
        /* To check status after running both publisher and subscriber */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_PublishedDataSetConfig pdsConfig;
        UA_NodeId dataSetWriter;
        UA_NodeId writerGroup;

        /* Published DataSet */
        memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
        pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
        pdsConfig.name = UA_STRING("PublishedDataSet Test");
        UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetTest);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Create variable to publish integer data */
        UA_NodeId publisherNode;
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.description           = UA_LOCALIZEDTEXT("en-US","Published Int32");
        attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Int32");
        attr.dataType              = UA_TYPES[UA_TYPES_INT32].typeId;
        UA_Int32 publisherData     = PUBLISHER_DATA;
        UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_INT32]);
        retVal                     = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                               UA_QUALIFIEDNAME(1, "Published Int32"),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                               attr, NULL, &publisherNode);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Data Set Field */
        UA_NodeId dataSetFieldIdent;
        UA_DataSetFieldConfig dataSetFieldConfig;
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType              = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Published Int32");
        dataSetFieldConfig.field.variable.promotedField  = UA_FALSE;
        dataSetFieldConfig.field.variable.publishParameters.publishedVariable = publisherNode;
        dataSetFieldConfig.field.variable.publishParameters.attributeId       = UA_ATTRIBUTEID_VALUE;
        UA_Server_addDataSetField (server, publishedDataSetTest, &dataSetFieldConfig, &dataSetFieldIdent);

        /* Writer group */
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name               = UA_STRING("WriterGroup Test");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.enabled            = UA_FALSE;
        writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
        writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
        /* Message settings in WriterGroup to include necessary headers */
        writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
        writerGroupMessage->networkMessageContentMask          = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                                  (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                                  (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                                  (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
        writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
        retVal |= UA_Server_addWriterGroup(server, connection_test, &writerGroupConfig, &writerGroup);
        UA_Server_setWriterGroupOperational(server, writerGroup);
        UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* DataSetWriter */
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name            = UA_STRING("DataSetWriter Test");
        dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
        dataSetWriterConfig.keyFrameCount   = 10;
        retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetTest, &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        UA_WriterGroup *writerGroupTest =  UA_WriterGroup_findWGbyId(server, writerGroup);
        UA_PubSubConnection *connection = writerGroupTest->linkedConnection;
        size_t dsmCount = 0;
        UA_DataSetWriter *dataSetWriter_test;
        UA_STACKARRAY(UA_DataSetMessage, dsmStore, writerGroupTest->writersCount);

        LIST_FOREACH(dataSetWriter_test, &writerGroupTest->writers, listEntry) {
        /* Find the dataset */
        UA_PublishedDataSet *publishedDataSet = UA_PublishedDataSet_findPDSbyId(server, dataSetWriter_test->connectedDataSet);
        if(!publishedDataSet) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "PubSub Publish: PublishedDataSet not found");
            continue;
        }

        /* Generate the DSM */
       res = UA_DataSetWriter_generateDataSetMessage(server, &dsmStore[dsmCount], dataSetWriter_test);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "PubSub Publish: DataSetMessage creation failed");
            continue;
        }

         UA_NetworkMessage nm;
         memset(&nm, 0, sizeof(UA_NetworkMessage));
         generateNetworkMessage(connection, writerGroupTest , &dsmStore[dsmCount], &dataSetWriter_test->config.dataSetWriterId, 1, &writerGroupTest->config.messageSettings, &writerGroupTest->config.transportSettings, &nm);
         /* Allocate the buffer. Allocate on the stack if the buffer is small. */
         UA_ByteString buf;
         size_t msgSize = UA_NetworkMessage_calcSizeBinary(&nm, NULL);
         size_t stackSize = 1;
         if(msgSize <= UA_MAX_STACKBUF)
            stackSize = msgSize;
         UA_STACKARRAY(UA_Byte, stackBuf, stackSize);
         buf.data = stackBuf;
         buf.length = msgSize;
         UA_StatusCode retval;
         UA_Byte *bufPos = buf.data;
         memset(bufPos, 0, msgSize);
         const UA_Byte *bufEnd = &buf.data[buf.length];
         retval = UA_NetworkMessage_encodeBinary(&nm, &bufPos, bufEnd);
         if(retval != UA_STATUSCODE_GOOD) {
            if(msgSize > UA_MAX_STACKBUF)
                UA_ByteString_clear(&buf);
            return;
         }
         retval = connection->channel->send(connection->channel, &writerGroupTest->config.transportSettings, &buf);
         if(msgSize > UA_MAX_STACKBUF)
             UA_ByteString_clear(&buf);
         ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
         UA_DataSetMessage_free(&dsmStore[dsmCount]);
    }

} END_TEST

int main(void) {
    /*Test case to run both publisher*/
    TCase *tc_pubsub_ethernet_publish = tcase_create("Publisher publishing Ethernet packets");
    tcase_add_checked_fixture(tc_pubsub_ethernet_publish, setup, teardown);
    tcase_add_test(tc_pubsub_ethernet_publish, EthernetPublisher);

    Suite *suite = suite_create("Ethernet Publisher");
    suite_add_tcase(suite, tc_pubsub_ethernet_publish);

    SRunner *suiteRunner = srunner_create(suite);
    srunner_set_fork_status(suiteRunner, CK_NOFORK);
    srunner_run_all(suiteRunner,CK_NORMAL);
    int number_failed = srunner_ntests_failed(suiteRunner);
    srunner_free(suiteRunner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

