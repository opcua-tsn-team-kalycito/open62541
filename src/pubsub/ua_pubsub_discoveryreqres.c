/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#ifdef UA_ENABLE_PUBSUB_DISCOVERY_REQUESTRESPONSE

#include "ua_pubsub_discoveryreqres.h"

#define UA_MAX_STACKBUF 512 /* Max size of network messages on the stack */

/**
 * Process Discovery Response Message and create target variables in information model.
 *
 * @param server
 * @param networkmessage
 * @param datasetreader
 * @return UA_STATUSCODE_GOOD on success
 */
UA_StatusCode
UA_Server_processDiscoveryResponseMessage(UA_Server *server, UA_NetworkMessage *pMsg, UA_DataSetReader* dataSetReader) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if (pMsg->payload.discoveryResponsePayload.discoveryResponseHeader.discoveryResponseType != UA_DATASET_METADATA_MESSAGE)
         return UA_STATUSCODE_BADNOTIMPLEMENTED;

#ifdef UA_ENABLE_PUBSUB_METADATA
     if (dataSetReader->config.dataSetMetaData.configurationVersion.majorVersion ==
             pMsg->payload.discoveryResponsePayload.discoveryResponseMessage.dataSetMetaDataMessage.dataSetMetaData.configurationVersion.majorVersion) {
         UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "Already updated Dataset metadata.");
         return UA_STATUSCODE_GOOD;
     }

     UA_DataSetMetaDataType tmpDataSetMetaData;
     UA_DataSetMetaDataType_copy(&pMsg->payload.discoveryResponsePayload.discoveryResponseMessage.dataSetMetaDataMessage.dataSetMetaData, &tmpDataSetMetaData);

     /* TODO: Dataset metadata message check has to be modified */
     if (!(tmpDataSetMetaData.name.data)) {
         UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "No Dataset metadata found");
         return UA_STATUSCODE_BADBOUNDNOTFOUND;
     }

     /* Delete the old dataset reader metadata members if new one comes */
     if (dataSetReader->config.dataSetMetaData.name.data)
         UA_DataSetMetaDataType_deleteMembers(&dataSetReader->config.dataSetMetaData);

     dataSetReader->config.dataSetMetaData = tmpDataSetMetaData;

     char dsmdName[dataSetReader->config.dataSetMetaData.name.length + 1];
     UA_snprintf(dsmdName, sizeof(dsmdName), "%.*s", (int)dataSetReader->config.dataSetMetaData.name.length, (const char*)dataSetReader->config.dataSetMetaData.name.data);

     retval = UA_Server_deleteNode(server, UA_NODEID_STRING(1, dsmdName), true);
     if (retval != UA_STATUSCODE_GOOD)
         UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "Not possible to delete or empty node");
     else
         UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "Older Dataset metadata has been deleted.");

     UA_NodeId folderId;
     UA_String folderName = dataSetReader->config.dataSetMetaData.name;
     UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
     UA_QualifiedName folderBrowseName;
     if(folderName.length > 0) {
         oAttr.displayName.locale = UA_STRING ("en-US");
         oAttr.displayName.text = folderName;
         folderBrowseName.namespaceIndex = 1;
         folderBrowseName.name = folderName;
     }
     else {
         oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Variables");
         folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");
     }

     /* Node ID value has been given as a metadata name instead of creating in random.
      * By using this, the older objects can be deleted. TODO: Design has to be verified */
     UA_Server_addObjectNode (server, UA_NODEID_STRING(1, dsmdName),
                              UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                              folderBrowseName, UA_NODEID_NUMERIC (0,
                              UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);
     retval |= UA_Server_DataSetReader_addTargetVariables (server, &folderId,
                                                           dataSetReader->identifier,
                                                           UA_PUBSUB_SDS_TARGET);
     /* Delete the members */
     UA_NodeId_deleteMembers(&folderId);

     if (retval != UA_STATUSCODE_GOOD)
         return retval;
#endif

     return retval;
}

/**
 * Generate a DiscoveryResponseMessage for the given writer.
 *
 * @param dataSetWriter ptr to corresponding writer
 * @return ptr to generated DataSetMessage
 */
static UA_StatusCode
UA_DataSetWriter_generateDiscoveryResponseMessage(UA_Server *server, UA_Byte informationType,
                                                  UA_DataSetWriter *dataSetWriter, UA_DiscoveryResponsePayload *discoveryResponsePayload) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    /* Reset the message */
    memset(discoveryResponsePayload, 0, sizeof(UA_DiscoveryResponsePayload));

    discoveryResponsePayload->discoveryResponseHeader.discoveryResponseType = (UA_DiscoveryResponseType) informationType;
    /* TODO: Sequence number can be provided the same as publisher ID in future. As of now given as a constant */
    discoveryResponsePayload->discoveryResponseHeader.sequenceNumber = 101;

    if(!(discoveryResponsePayload->discoveryResponseHeader.discoveryResponseType & UA_DATASET_METADATA_MESSAGE))
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    /* TODO: Implement other discovery response types. Handle both json or binary encoding */

#ifdef UA_ENABLE_PUBSUB_METADATA
    /* Pass dataset writer ID to the DataSetMetaData message */
    if (dataSetWriter->config.dataSetWriterId != 0) {
        discoveryResponsePayload->discoveryResponseMessage.dataSetMetaDataMessage.dataSetWriterId = dataSetWriter->config.dataSetWriterId;
    }
    else {
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* Copy the metadata to datasetmetadata message from published dataset */
    retval = UA_Server_getPublishedDataSetMetaData(server, dataSetWriter->connectedDataSet,
                                                   &discoveryResponsePayload->discoveryResponseMessage.dataSetMetaDataMessage.dataSetMetaData);
    if (retval != UA_STATUSCODE_GOOD)
        return retval;

    /* TODO: Modification needed in status code value */
    discoveryResponsePayload->discoveryResponseMessage.dataSetMetaDataMessage.statusCode = UA_STATUSCODE_GOOD;
#endif

    return retval;
}


static UA_StatusCode
sendNetworkMessageWithDiscoveryResponse(UA_PubSubConnection *connection, UA_WriterGroup *wg,
                                        UA_DiscoveryResponsePayload *discoveryResponsePayload, UA_Byte drpCount,
                                        UA_ExtensionObject *messageSettings,
                                        UA_ExtensionObject *transportSettings) {

    if(messageSettings->content.decoded.type !=
       &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE])
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UadpWriterGroupMessageDataType *wgm = (UA_UadpWriterGroupMessageDataType*)
        messageSettings->content.decoded.data;

    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(UA_NetworkMessage));

    /* TODO: Handle these IDs without content mask if possible */
    nm.groupHeader.writerGroupIdEnabled =
        ((u64)wgm->networkMessageContentMask & (u64)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID) != 0;
    nm.groupHeader.groupVersionEnabled =
        ((u64)wgm->networkMessageContentMask & (u64)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPVERSION) != 0;
    nm.groupHeader.networkMessageNumberEnabled =
        ((u64)wgm->networkMessageContentMask & (u64)UA_UADPNETWORKMESSAGECONTENTMASK_NETWORKMESSAGENUMBER) != 0;
    nm.groupHeader.sequenceNumberEnabled =
        ((u64)wgm->networkMessageContentMask & (u64)UA_UADPNETWORKMESSAGECONTENTMASK_SEQUENCENUMBER) != 0;
    nm.publisherIdEnabled    = 1;
    nm.groupHeaderEnabled    = 0;
    nm.payloadHeaderEnabled  = 0;
    nm.timestampEnabled      = 0;
    nm.picosecondsEnabled    = 0;
    nm.dataSetClassIdEnabled = 0;
    nm.promotedFieldsEnabled = 0;

    nm.version = 1;
    nm.networkMessageType = UA_NETWORKMESSAGE_DISCOVERY_RESPONSE;
    if(connection->config->publisherIdType == UA_PUBSUB_PUBLISHERID_NUMERIC) {
        nm.publisherIdType = UA_PUBLISHERDATATYPE_UINT16;
        nm.publisherId.publisherIdUInt32 = connection->config->publisherId.numeric;
    } else if(connection->config->publisherIdType == UA_PUBSUB_PUBLISHERID_STRING){
        nm.publisherIdType = UA_PUBLISHERDATATYPE_STRING;
        nm.publisherId.publisherIdString = connection->config->publisherId.string;
    }

    nm.payload.discoveryResponsePayload.discoveryResponseHeader = discoveryResponsePayload->discoveryResponseHeader;
    if(nm.payload.discoveryResponsePayload.discoveryResponseHeader.discoveryResponseType != UA_DATASET_METADATA_MESSAGE)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    /* Pass the discovery response message to network message */
    nm.payload.discoveryResponsePayload.discoveryResponseMessage = discoveryResponsePayload->discoveryResponseMessage;

    /* Allocate the buffer. Allocate on the stack if the buffer is small. */
    UA_ByteString buf;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&nm);
    size_t stackSize = 1;
    if(msgSize <= UA_MAX_STACKBUF)
        stackSize = msgSize;
    UA_STACKARRAY(UA_Byte, stackBuf, stackSize);
    buf.data = stackBuf;
    buf.length = msgSize;
    UA_StatusCode retval;
    if(msgSize > UA_MAX_STACKBUF) {
        retval = UA_ByteString_allocBuffer(&buf, msgSize);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* Encode the message */
    UA_Byte *bufPos = buf.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &buf.data[buf.length];
    retval = UA_NetworkMessage_encodeBinary(&nm, &bufPos, bufEnd);
    if(retval != UA_STATUSCODE_GOOD) {
        if(msgSize > UA_MAX_STACKBUF)
            UA_ByteString_deleteMembers(&buf);
        return retval;
    }

    /* Send the prepared messages */
    retval = connection->channel->send(connection->channel, transportSettings, &buf);
    if(msgSize > UA_MAX_STACKBUF)
        UA_ByteString_deleteMembers(&buf);
    return retval;
}

void UA_DicoveryResponseMessage_free(UA_DiscoveryResponsePayload* p) {
    if(p->discoveryResponseHeader.discoveryResponseType == UA_DATASET_METADATA_MESSAGE) {
        UA_DataSetMetaDataType_deleteMembers(&p->discoveryResponseMessage.dataSetMetaDataMessage.dataSetMetaData);
    }
    else {
        //not implemented
    }
}

UA_StatusCode
sendDiscoveryResponse(UA_Server *server, UA_Byte informationType, UA_UInt16 *dataSetWriterId) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(!(informationType & UA_DATASET_METADATA_MESSAGE))
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    /* TODO: Handling multiple datasetwriter when traffic reduction comes into place - Part 14 spec Pg.No: 74 */

    UA_DataSetWriter *dataSetWriter = UA_DataSetWriter_findDSWbyDSWId(server, *dataSetWriterId);
    if(!dataSetWriter) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Metadata publish failed. Dataset writer not found");
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_NodeId writerGroupId = dataSetWriter->linkedWriterGroup;
    UA_WriterGroup *writerGroup = UA_WriterGroup_findWGbyId(server, writerGroupId);
    if(!writerGroup) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Metadata publish failed. WriterGroup not found");
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* Binary or Json encoding */
    if(writerGroup->config.encodingMimeType != UA_PUBSUB_ENCODING_UADP &&
       writerGroup->config.encodingMimeType != UA_PUBSUB_ENCODING_JSON) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Metadata publish failed: Unknown encoding type.");
        return UA_STATUSCODE_BADDATAENCODINGINVALID;
    }

    /* Find the connection associated with the writer */
    UA_PubSubConnection *connection =
        UA_PubSubConnection_findConnectionbyId(server, writerGroup->linkedConnection);
    if(!connection) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Metadata publish failed. PubSubConnection invalid.");
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* TODO: Handling multiple messages in discovery response payload in other response types and considering traffic reduction.
     * As of now handling single datasetmetadatamessage with a datasetwriter in discovery response payload. PubSub Spec Part 14 (Pg.No:76)
     */
    UA_DiscoveryResponsePayload drp;

    /* Find the dataset */
    UA_PublishedDataSet *pds = UA_PublishedDataSet_findPDSbyId(server, dataSetWriter->connectedDataSet);
    if(!pds) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "Metadata Publish: PublishedDataSet not found");
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* Generate the DSMDM */
    retval = UA_DataSetWriter_generateDiscoveryResponseMessage(server, informationType, dataSetWriter, &drp);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "Metadata Publish: DataSetMetaDataMessage creation failed");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* TODO: Handle for both UADP and JSON encoding */
    /* Send network message with discovery response */
    retval = sendNetworkMessageWithDiscoveryResponse(connection, writerGroup, &drp, 1,
                                                     &writerGroup->config.messageSettings,
                                                     &writerGroup->config.transportSettings);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "Metadata Publish: Sending metatadata failed");
    return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* TODO: Clean up the Discovery Response Message for other discovery response type */
    UA_DicoveryResponseMessage_free(&drp);

    return retval;
}

#endif /* UA_ENABLE_PUBSUB_DISCOVERY_REQUESTRESPONSE */

#endif /* UA_ENABLE_PUBSUB */
