/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019-2021 Kalycito Infotech Private Limited
 * Copyright (c) 2020 Yannick Wallerer, Siemens AG
 * Copyright (c) 2020 Thomas Fischer, Siemens AG
 */

#include <open62541/types.h>
#include "ua_pubsub_ns0.h"

#ifdef UA_ENABLE_PUBSUB_FILE_CONFIG
#include "ua_pubsub_config.h"
#endif

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL /* conditional compilation */

typedef struct{
    UA_NodeId parentNodeId;
    UA_UInt32 parentClassifier;
    UA_UInt32 elementClassiefier;
} UA_NodePropertyContext;

//Prototypes
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
static UA_StatusCode addWriterGroupAction(UA_Server *server,
                                          const UA_NodeId *sessionId, void *sessionHandle,
                                          const UA_NodeId *methodId, void *methodContext,
                                          const UA_NodeId *objectId, void *objectContext,
                                          size_t inputSize, const UA_Variant *input,
                                          size_t outputSize, UA_Variant *output);
static UA_StatusCode removeGroupAction(UA_Server *server,
                                          const UA_NodeId *sessionId, void *sessionHandle,
                                          const UA_NodeId *methodId, void *methodContext,
                                          const UA_NodeId *objectId, void *objectContext,
                                          size_t inputSize, const UA_Variant *input,
                                          size_t outputSize, UA_Variant *output);
static UA_StatusCode addDataSetWriterAction(UA_Server *server,
                                          const UA_NodeId *sessionId, void *sessionHandle,
                                          const UA_NodeId *methodId, void *methodContext,
                                          const UA_NodeId *objectId, void *objectContext,
                                          size_t inputSize, const UA_Variant *input,
                                          size_t outputSize, UA_Variant *output);

#endif

static UA_StatusCode
addPubSubObjectNode(UA_Server *server, char* name, UA_UInt32 objectid,
              UA_UInt32 parentid, UA_UInt32 referenceid, UA_UInt32 type_id) {
    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName = UA_LOCALIZEDTEXT("", name);
    return UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(0, objectid),
                                   UA_NODEID_NUMERIC(0, parentid),
                                   UA_NODEID_NUMERIC(0, referenceid),
                                   UA_QUALIFIEDNAME(0, name),
                                   UA_NODEID_NUMERIC(0, type_id),
                                   object_attr, NULL, NULL);
}

static UA_StatusCode
writePubSubNs0VariableArray(UA_Server *server, UA_UInt32 id, void *v,
                      size_t length, const UA_DataType *type) {
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setArray(&var, v, length, type);
    return UA_Server_writeValue(server, UA_NODEID_NUMERIC(0, id), var);
}

static UA_NodeId
findSingleChildNode(UA_Server *server, UA_QualifiedName targetName,
                    UA_NodeId referenceTypeId, UA_NodeId startingNode){
    UA_NodeId resultNodeId;
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = referenceTypeId;
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    rpe.targetName = targetName;
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = startingNode;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;
    UA_BrowsePathResult bpr =
            UA_Server_translateBrowsePathToNodeIds(server, &bp);
    if(bpr.statusCode != UA_STATUSCODE_GOOD ||
       bpr.targetsSize < 1)
        return UA_NODEID_NULL;
    if(UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, &resultNodeId) != UA_STATUSCODE_GOOD){
        UA_BrowsePathResult_clear(&bpr);
        return UA_NODEID_NULL;
    }
    UA_BrowsePathResult_clear(&bpr);
    return resultNodeId;
}

static void
onRead(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
       const UA_NodeId *nodeid, void *context,
       const UA_NumericRange *range, const UA_DataValue *data) {
    UA_Variant value;
    UA_Variant_init(&value);
    const UA_NodePropertyContext *nodeContext = (const UA_NodePropertyContext*)context;
    const UA_NodeId *myNodeId = &nodeContext->parentNodeId;

    switch(nodeContext->parentClassifier){
    case UA_NS0ID_PUBSUBCONNECTIONTYPE: {
        UA_PubSubConnection *pubSubConnection =
            UA_PubSubConnection_findConnectionbyId(server, *myNodeId);
        switch(nodeContext->elementClassiefier) {
        case UA_NS0ID_PUBSUBCONNECTIONTYPE_PUBLISHERID:
            if(pubSubConnection->config->publisherIdType == UA_PUBSUB_PUBLISHERID_STRING) {
                UA_Variant_setScalar(&value, &pubSubConnection->config->publisherId.numeric,
                                     &UA_TYPES[UA_TYPES_STRING]);
            } else {
                UA_Variant_setScalar(&value, &pubSubConnection->config->publisherId.numeric,
                                     &UA_TYPES[UA_TYPES_UINT32]);
            }
            break;
        default:
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Read error! Unknown property.");
        }
        break;
    }
    case UA_NS0ID_DATASETREADERTYPE: {
        UA_DataSetReader *dataSetReader = UA_ReaderGroup_findDSRbyId(server, *myNodeId);
        if(!dataSetReader)
            return;

        switch(nodeContext->elementClassiefier) {
        case UA_NS0ID_DATASETREADERTYPE_PUBLISHERID:
            UA_Variant_setScalar(&value, dataSetReader->config.publisherId.data,
                                 dataSetReader->config.publisherId.type);
            break;
        default:
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Read error! Unknown property.");
        }
        break;
    }
    case UA_NS0ID_WRITERGROUPTYPE: {
        UA_WriterGroup *writerGroup = UA_WriterGroup_findWGbyId(server, *myNodeId);
        if(!writerGroup)
            return;
        switch(nodeContext->elementClassiefier){
        case UA_NS0ID_WRITERGROUPTYPE_PUBLISHINGINTERVAL:
            UA_Variant_setScalar(&value, &writerGroup->config.publishingInterval,
                                 &UA_TYPES[UA_TYPES_DURATION]);
            break;
        default:
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Read error! Unknown property.");
        }
        break;
    }
    case UA_NS0ID_DATASETWRITERTYPE: {
        UA_DataSetWriter *dataSetWriter = UA_DataSetWriter_findDSWbyId(server, *myNodeId);
        if(!dataSetWriter)
            return;

        switch(nodeContext->elementClassiefier) {
            case UA_NS0ID_DATASETWRITERTYPE_DATASETWRITERID:
                UA_Variant_setScalar(&value, &dataSetWriter->config.dataSetWriterId,
                                     &UA_TYPES[UA_TYPES_UINT16]);
                break;
            default:
                UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                               "Read error! Unknown property.");
        }
        break;
    }
    case UA_NS0ID_PUBLISHEDDATAITEMSTYPE: {
        UA_PublishedDataSet *publishedDataSet = UA_PublishedDataSet_findPDSbyId(server, *myNodeId);
        if(!publishedDataSet)
            return;
        switch(nodeContext->elementClassiefier) {
        case UA_NS0ID_PUBLISHEDDATAITEMSTYPE_PUBLISHEDDATA: {
            UA_PublishedVariableDataType *pvd = (UA_PublishedVariableDataType *)
                UA_calloc(publishedDataSet->fieldSize, sizeof(UA_PublishedVariableDataType));
            size_t counter = 0;
            UA_DataSetField *field;
            TAILQ_FOREACH(field, &publishedDataSet->fields, listEntry) {
                pvd[counter].attributeId = UA_ATTRIBUTEID_VALUE;
                pvd[counter].publishedVariable = field->config.field.variable.publishParameters.publishedVariable;
                //UA_NodeId_copy(&field->config.field.variable.publishParameters.publishedVariable, &pvd[counter].publishedVariable);
                counter++;
            }
            UA_Variant_setArray(&value, pvd, publishedDataSet->fieldSize,
                                &UA_TYPES[UA_TYPES_PUBLISHEDVARIABLEDATATYPE]);
            break;
        }
        case UA_NS0ID_PUBLISHEDDATAITEMSTYPE_DATASETMETADATA: {
            UA_Variant_setScalarCopy(&value, &publishedDataSet->dataSetMetaData, &UA_TYPES[UA_TYPES_DATASETMETADATATYPE]);
            break;
        }
        case UA_NS0ID_PUBLISHEDDATAITEMSTYPE_CONFIGURATIONVERSION: {
            UA_Variant_setScalarCopy(&value, &publishedDataSet->dataSetMetaData.configurationVersion,
                                     &UA_TYPES[UA_TYPES_CONFIGURATIONVERSIONDATATYPE]);
            break;
        }
        default:
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Read error! Unknown property.");
        }
        break;
    }
    default:
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Read error! Unknown parent element.");
    }
    UA_Server_writeValue(server, *nodeid, value);
}

static void
onWrite(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
        const UA_NodeId *nodeId, void *nodeContext,
        const UA_NumericRange *range, const UA_DataValue *data){
    UA_Variant value;
    UA_NodeId myNodeId;
    UA_WriterGroup *writerGroup = NULL;
    switch(((UA_NodePropertyContext *) nodeContext)->parentClassifier){
        case UA_NS0ID_PUBSUBCONNECTIONTYPE:
            //no runtime writable attributes
            break;
        case UA_NS0ID_WRITERGROUPTYPE:
            myNodeId = ((UA_NodePropertyContext *) nodeContext)->parentNodeId;
            writerGroup = UA_WriterGroup_findWGbyId(server, myNodeId);
            UA_WriterGroupConfig writerGroupConfig;
            memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
            if(!writerGroup)
                return;
            switch(((UA_NodePropertyContext *) nodeContext)->elementClassiefier){
                case UA_NS0ID_WRITERGROUPTYPE_PUBLISHINGINTERVAL:
                    UA_Server_getWriterGroupConfig(server, writerGroup->identifier, &writerGroupConfig);
                    writerGroupConfig.publishingInterval = *((UA_Duration *) data->value.data);
                    UA_Server_updateWriterGroupConfig(server, writerGroup->identifier, &writerGroupConfig);
                    UA_Variant_setScalar(&value, data->value.data, &UA_TYPES[UA_TYPES_DURATION]);
                    UA_WriterGroupConfig_clear(&writerGroupConfig);
                    break;
                default:
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                   "Write error! Unknown property element.");
            }
            break;
        default:
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Read error! Unknown parent element.");
    }
}

static UA_StatusCode
addVariableValueSource(UA_Server *server, UA_ValueCallback valueCallback,
                       UA_NodeId node, UA_NodePropertyContext *context){
    UA_Server_setNodeContext(server, node, context);
    return UA_Server_setVariableNode_valueCallback(server, node, valueCallback);
}

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
static UA_StatusCode
addPubSubConnectionConfig(UA_Server *server, UA_PubSubConnectionDataType *pubsubConnectionDataType,
                          UA_NodeId *connectionId){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_NetworkAddressUrlDataType networkAddressUrlDataType;
    memset(&networkAddressUrlDataType, 0, sizeof(networkAddressUrlDataType));
    UA_ExtensionObject eo = pubsubConnectionDataType->address;
    if(eo.encoding == UA_EXTENSIONOBJECT_DECODED){
        if(eo.content.decoded.type == &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]){
            if(UA_NetworkAddressUrlDataType_copy((UA_NetworkAddressUrlDataType *) eo.content.decoded.data,
                                                 &networkAddressUrlDataType) != UA_STATUSCODE_GOOD){
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
        }
    }

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.transportProfileUri = pubsubConnectionDataType->transportProfileUri;
    connectionConfig.name = pubsubConnectionDataType->name;
    //TODO set real connection state
    connectionConfig.enabled = pubsubConnectionDataType->enabled;
    //connectionConfig.enabled = pubSubConnectionDataType.enabled;
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrlDataType,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    if(pubsubConnectionDataType->publisherId.type == &UA_TYPES[UA_TYPES_UINT32]){
        connectionConfig.publisherId.numeric = * ((UA_UInt32 *) pubsubConnectionDataType->publisherId.data);
    } else if(pubsubConnectionDataType->publisherId.type == &UA_TYPES[UA_TYPES_STRING]){
        connectionConfig.publisherIdType = UA_PUBSUB_PUBLISHERID_STRING;
        UA_String_copy((UA_String *) pubsubConnectionDataType->publisherId.data, &connectionConfig.publisherId.string);
    } else {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER, "Unsupported PublisherId Type used.");
        //TODO what's the best default behaviour here?
        connectionConfig.publisherId.numeric = 0;
    }

    retVal |= UA_Server_addPubSubConnection(server, &connectionConfig, connectionId);
    UA_NetworkAddressUrlDataType_clear(&networkAddressUrlDataType);
    return retVal;
}

/**
 * **WriterGroup handling**
 *
 * The WriterGroup (WG) is part of the connection and contains the primary
 * configuration parameters for the message creation. */
static UA_StatusCode
addWriterGroupConfig(UA_Server *server, UA_NodeId connectionId,
                     UA_WriterGroupDataType *writerGroupDataType, UA_NodeId *writerGroupId){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    /* Now we create a new WriterGroupConfig and add the group to the existing
     * PubSubConnection. */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = writerGroupDataType->name;
    writerGroupConfig.publishingInterval = writerGroupDataType->publishingInterval;
    writerGroupConfig.enabled = writerGroupDataType->enabled;
    writerGroupConfig.writerGroupId = writerGroupDataType->writerGroupId;
    //TODO remove hard coded UADP
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.priority = writerGroupDataType->priority;

    UA_UadpWriterGroupMessageDataType writerGroupMessage;
    UA_ExtensionObject *eoWG = &writerGroupDataType->messageSettings;
    if(eoWG->encoding == UA_EXTENSIONOBJECT_DECODED){
        writerGroupConfig.messageSettings.encoding  = UA_EXTENSIONOBJECT_DECODED;
        if(eoWG->content.decoded.type == &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]){
            if(UA_UadpWriterGroupMessageDataType_copy((UA_UadpWriterGroupMessageDataType *) eoWG->content.decoded.data,
                                                        &writerGroupMessage) != UA_STATUSCODE_GOOD){
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
            writerGroupConfig.messageSettings.content.decoded.data = &writerGroupMessage;
        }
    }

    retVal |= UA_Server_addWriterGroup(server, connectionId, &writerGroupConfig, writerGroupId);
    return retVal;
}

/**
 * **DataSetWriter handling**
 *
 * A DataSetWriter (DSW) is the glue between the WG and the PDS. The DSW is
 * linked to exactly one PDS and contains additional informations for the
 * message generation. */
static UA_StatusCode
addDataSetWriterConfig(UA_Server *server, UA_NodeId writerGroupId,
                       UA_DataSetWriterDataType *dataSetWriterDataType){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_NodeId publishedDataSetId = UA_NODEID_NULL;
    UA_PublishedDataSet *tmpPDS;

    TAILQ_FOREACH(tmpPDS, &server->pubSubManager.publishedDataSets, listEntry){
        if(UA_String_equal(&dataSetWriterDataType->dataSetName, &tmpPDS->config.name)){
            publishedDataSetId = tmpPDS->identifier;
        }
    }

    if(UA_NodeId_isNull(&publishedDataSetId))
        return UA_STATUSCODE_BADPARENTNODEIDINVALID;

    /* We need now a DataSetWriter within the WriterGroup. This means we must
     * create a new DataSetWriterConfig and add call the addWriterGroup function. */
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = dataSetWriterDataType->name;
    dataSetWriterConfig.dataSetWriterId = dataSetWriterDataType->dataSetWriterId;
    dataSetWriterConfig.keyFrameCount = dataSetWriterDataType->keyFrameCount;
    dataSetWriterConfig.dataSetFieldContentMask =  dataSetWriterDataType->dataSetFieldContentMask;

    UA_NodeId dataSetWriterId;
    retVal |= UA_Server_addDataSetWriter(server, writerGroupId, publishedDataSetId,
                                         &dataSetWriterConfig, &dataSetWriterId);
    return retVal;
}

#endif
/*************************************************/
/*            PubSubConnection                   */
/*************************************************/
UA_StatusCode
addPubSubConnectionRepresentation(UA_Server *server, UA_PubSubConnection *connection){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(connection->config->name.length > 512)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_STACKARRAY(char, connectionName, sizeof(char) * connection->config->name.length +1);
    memcpy(connectionName, connection->config->name.data, connection->config->name.length);
    connectionName[connection->config->name.length] = '\0';
    //This code block must use a lock
    UA_NODESTORE_REMOVE(server, &connection->identifier);
    UA_NodeId pubSubConnectionNodeId;
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("de-DE", connectionName);
    retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT,
                                      UA_NODEID_NUMERIC(0, connection->identifier.identifier.numeric),
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE),
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_HASPUBSUBCONNECTION),
                                      UA_QUALIFIEDNAME(0, connectionName),
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_PUBSUBCONNECTIONTYPE),
                                      (const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                                      NULL, &pubSubConnectionNodeId);
    addPubSubObjectNode(server, "Address", connection->identifier.identifier.numeric+1,
                        pubSubConnectionNodeId.identifier.numeric,  UA_NS0ID_HASCOMPONENT,
                        UA_NS0ID_NETWORKADDRESSURLTYPE);
    UA_Server_addNode_finish(server, pubSubConnectionNodeId);
    //End lock zone

    UA_NodeId addressNode, urlNode, interfaceNode, publisherIdNode, connectionPropertieNode, transportProfileUri;
    addressNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Address"),
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                      UA_NODEID_NUMERIC(0, connection->identifier.identifier.numeric));
    urlNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Url"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), addressNode);
    interfaceNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "NetworkInterface"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), addressNode);
    publisherIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublisherId"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                        UA_NODEID_NUMERIC(0, connection->identifier.identifier.numeric));
    connectionPropertieNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "ConnectionProperties"),
                                                 UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                                 UA_NODEID_NUMERIC(0, connection->identifier.identifier.numeric));
    transportProfileUri = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "TransportProfileUri"),
                                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                          UA_NODEID_NUMERIC(0, connection->identifier.identifier.numeric));

    if(UA_NodeId_equal(&addressNode, &UA_NODEID_NULL) ||
       UA_NodeId_equal(&urlNode, &UA_NODEID_NULL) ||
       UA_NodeId_equal(&interfaceNode, &UA_NODEID_NULL) ||
       UA_NodeId_equal(&publisherIdNode, &UA_NODEID_NULL) ||
       UA_NodeId_equal(&connectionPropertieNode, &UA_NODEID_NULL) ||
       UA_NodeId_equal(&transportProfileUri, &UA_NODEID_NULL)) {
        return UA_STATUSCODE_BADNOTFOUND;
    }

    retVal |= writePubSubNs0VariableArray(server, connectionPropertieNode.identifier.numeric,
                                          connection->config->connectionProperties,
                                          connection->config->connectionPropertiesSize,
                                          &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);

    UA_NetworkAddressUrlDataType *networkAddressUrlDataType = ((UA_NetworkAddressUrlDataType *) connection->config->address.data);
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &networkAddressUrlDataType->url, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_writeValue(server, urlNode, value);
    UA_Variant_setScalar(&value, &networkAddressUrlDataType->networkInterface, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_writeValue(server, interfaceNode, value);
    UA_Variant_setScalar(&value, &connection->config->transportProfileUri, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_writeValue(server, transportProfileUri, value);

    UA_NodePropertyContext *connectionPublisherIdContext = (UA_NodePropertyContext *) UA_malloc(sizeof(UA_NodePropertyContext));
    connectionPublisherIdContext->parentNodeId = connection->identifier;
    connectionPublisherIdContext->parentClassifier = UA_NS0ID_PUBSUBCONNECTIONTYPE;
    connectionPublisherIdContext->elementClassiefier = UA_NS0ID_PUBSUBCONNECTIONTYPE_PUBLISHERID;
    UA_ValueCallback valueCallback;
    valueCallback.onRead = onRead;
    valueCallback.onWrite = NULL;
    retVal |= addVariableValueSource(server, valueCallback, publisherIdNode, connectionPublisherIdContext);

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
    retVal |= UA_Server_addReference(server, connection->identifier,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                     UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PUBSUBCONNECTIONTYPE_ADDWRITERGROUP), true);
    retVal |= UA_Server_addReference(server, connection->identifier,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                     UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PUBSUBCONNECTIONTYPE_ADDREADERGROUP), true);
    retVal |= UA_Server_addReference(server, connection->identifier,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                     UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PUBSUBCONNECTIONTYPE_REMOVEGROUP), true);
#endif
    return retVal;
}

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
static UA_StatusCode
addPubSubConnectionAction(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionHandle,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_PubSubConnectionDataType pubSubConnectionDataType = *((UA_PubSubConnectionDataType *) input[0].data);

    //call API function and create the connection
    UA_NodeId connectionId;
    retVal |= addPubSubConnectionConfig(server, &pubSubConnectionDataType, &connectionId);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "addPubSubConnection failed");
        return retVal;
    }

    for(size_t i = 0; i < pubSubConnectionDataType.writerGroupsSize; i++){
        UA_NodeId writerGroupId;
        UA_WriterGroupDataType *writerGroupDataType = &pubSubConnectionDataType.writerGroups[i];
        retVal |= addWriterGroupConfig(server, connectionId, writerGroupDataType, &writerGroupId);
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "addWriterGroup failed");
            return retVal;
        }

        for (size_t j = 0; j < pubSubConnectionDataType.writerGroups[i].dataSetWritersSize; j++){
            UA_DataSetWriterDataType *dataSetWriterDataType = &writerGroupDataType->dataSetWriters[j];
            retVal |= addDataSetWriterConfig(server, writerGroupId, dataSetWriterDataType);
            if(retVal != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "addDataSetWriter failed");
                return retVal;
            }
        }
        UA_Server_setWriterGroupOperational(server, writerGroupId);
    }

    for(size_t i = 0; i < pubSubConnectionDataType.readerGroupsSize; i++){
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
        readerGroupConfig.name = pubSubConnectionDataType.readerGroups[i].name;
        UA_NodeId readerGroupId;
        retVal |= UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig,
                                           &readerGroupId);
        if(retVal != UA_STATUSCODE_GOOD)
           return retVal;

        UA_Server_setReaderGroupOperational(server, readerGroupId);

        for (size_t j = 0; j < pubSubConnectionDataType.readerGroups[i].dataSetReadersSize; j++){
            UA_DataSetReaderConfig readerConfig;
            memset (&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
            readerConfig.name = pubSubConnectionDataType.readerGroups[i].dataSetReaders[j].name;
            /* Parameters to filter which DataSetMessage has to be processed
             * by the DataSetReader */
            UA_UInt16 publisherIdentifier = *(UA_UInt16*)pubSubConnectionDataType.readerGroups[i].dataSetReaders[j].publisherId.data;
            readerConfig.publisherId.type = &UA_TYPES[UA_TYPES_UINT16];
            readerConfig.publisherId.data = &publisherIdentifier;
            readerConfig.writerGroupId    = pubSubConnectionDataType.readerGroups[i].dataSetReaders[j].writerGroupId;
            readerConfig.dataSetWriterId  = pubSubConnectionDataType.readerGroups[i].dataSetReaders[j].dataSetWriterId;

            /* Setting up Meta data configuration in DataSetReader */
            UA_DataSetMetaDataType *pMetaData;
            pMetaData = &readerConfig.dataSetMetaData;
            UA_DataSetMetaDataType_init (pMetaData);
            pMetaData->name =  pubSubConnectionDataType.readerGroups[i].dataSetReaders[j].dataSetMetaData.name;
            pMetaData->fieldsSize = pubSubConnectionDataType.readerGroups[i].dataSetReaders[j].dataSetMetaData.fieldsSize;
            pMetaData->fields = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                                &UA_TYPES[UA_TYPES_FIELDMETADATA]);
            for (size_t k = 0; k < pMetaData->fieldsSize; k++){
                UA_FieldMetaData_init (&pMetaData->fields[k]);
                UA_NodeId_copy (&pubSubConnectionDataType.readerGroups[i].dataSetReaders[j].dataSetMetaData.fields[k].dataType,
                                &pMetaData->fields[k].dataType);
                pMetaData->fields[k].builtInType = pubSubConnectionDataType.readerGroups[i].dataSetReaders[j].dataSetMetaData.fields[k].builtInType;
                pMetaData->fields[k].name = pubSubConnectionDataType.readerGroups[i].dataSetReaders[j].dataSetMetaData.fields[k].name;
                pMetaData->fields[k].valueRank = pubSubConnectionDataType.readerGroups[i].dataSetReaders[j].dataSetMetaData.fields[k].valueRank;
            }
            UA_NodeId dataSetReaderId;
            retVal |= UA_Server_addDataSetReader(server, readerGroupId, &readerConfig,
                                                 &dataSetReaderId);
            if(retVal != UA_STATUSCODE_GOOD)
                return retVal;

            UA_TargetVariablesDataType targetVars;
            UA_ExtensionObject *eoTargetVar = &pubSubConnectionDataType.readerGroups[i].dataSetReaders[j].subscribedDataSet;
            if(eoTargetVar->encoding == UA_EXTENSIONOBJECT_DECODED){
                if(eoTargetVar->content.decoded.type == &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE]){
                    if(UA_TargetVariablesDataType_copy((UA_TargetVariablesDataType *) eoTargetVar->content.decoded.data,
                                                        &targetVars) != UA_STATUSCODE_GOOD){
                        return UA_STATUSCODE_BADOUTOFMEMORY;
                    }
                    UA_NodeId folderId;
                    UA_String folderName = pMetaData->name;
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

                    UA_Server_addObjectNode (server, UA_NODEID_NULL,
                                            UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                                            UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                                            folderBrowseName, UA_NODEID_NUMERIC (0,
                                            UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);
                    /**
                    * **TargetVariables**
                    *
                    * The SubscribedDataSet option TargetVariables defines a list of Variable mappings between
                    * received DataSet fields and target Variables in the Subscriber AddressSpace.
                    * The values subscribed from the Publisher are updated in the value field of these variables */
                    /* Create the TargetVariables with respect to DataSetMetaData fields */
                    UA_FieldTargetVariable *targetVarsData = (UA_FieldTargetVariable *)
                            UA_calloc(targetVars.targetVariablesSize, sizeof(UA_FieldTargetVariable));
                    for(size_t l = 0; l < targetVars.targetVariablesSize; l++) {
                        /* Variable to subscribe data */
                        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
                        UA_LocalizedText_copy(&pMetaData->fields[l].description,
                                            &vAttr.description);
                        vAttr.displayName.locale = UA_STRING("en-US");
                        vAttr.displayName.text = pMetaData->fields[l].name;
                        vAttr.dataType = pMetaData->fields[l].dataType;

                        UA_NodeId newNode;
                        retVal |= UA_Server_addVariableNode(server, targetVars.targetVariables[l].targetNodeId,
                                                            folderId,
                                                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                                            UA_QUALIFIEDNAME(1, (char *)pMetaData->fields[l].name.data),
                                                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                            vAttr, NULL, &newNode);

                        /* For creating Targetvariables */
                        UA_FieldTargetDataType_init(&targetVarsData[l].targetVariable);
                        targetVarsData[l].targetVariable.attributeId  = targetVars.targetVariables[l].attributeId;
                        targetVarsData[l].targetVariable.targetNodeId = newNode;
                    }
                    retVal = UA_Server_DataSetReader_createTargetVariables(server, dataSetReaderId,
                                                                           targetVars.targetVariablesSize, targetVarsData);
                    for(size_t z = 0; z < targetVars.targetVariablesSize; z++)
                        UA_FieldTargetDataType_clear(&targetVarsData[z].targetVariable);
                    UA_free(targetVarsData);
                    UA_free(pMetaData->fields);
                }
            }
        }
    }
    //set ouput value
    UA_Variant_setScalarCopy(output, &connectionId, &UA_TYPES[UA_TYPES_NODEID]);
    return UA_STATUSCODE_GOOD;
}
#endif

UA_StatusCode
removePubSubConnectionRepresentation(UA_Server *server, UA_PubSubConnection *connection){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
    retVal |= UA_Server_deleteReference(server, connection->identifier, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), true,
                                        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PUBSUBCONNECTIONTYPE_ADDWRITERGROUP),
                                        false);
    retVal |= UA_Server_deleteReference(server, connection->identifier, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), true,
                                        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PUBSUBCONNECTIONTYPE_ADDREADERGROUP),
                                        false);
    retVal |= UA_Server_deleteReference(server, connection->identifier, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), true,
                                        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PUBSUBCONNECTIONTYPE_REMOVEGROUP),
                                        false);
#endif
    retVal |= UA_Server_deleteNode(server, connection->identifier, true);
    return retVal;
}

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
static UA_StatusCode
removeConnectionAction(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionHandle,
                       const UA_NodeId *methodId, void *methodContext,
                       const UA_NodeId *objectId, void *objectContext,
                       size_t inputSize, const UA_Variant *input,
                       size_t outputSize, UA_Variant *output){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_NodeId nodeToRemove = *((UA_NodeId *) input[0].data);
    retVal |= UA_Server_removePubSubConnection(server, nodeToRemove);
    if(retVal == UA_STATUSCODE_BADNOTFOUND)
        retVal = UA_STATUSCODE_BADNODEIDUNKNOWN;
    return retVal;
}
#endif

/**********************************************/
/*               DataSetReader                */
/**********************************************/
UA_StatusCode
addDataSetReaderRepresentation(UA_Server *server, UA_DataSetReader *dataSetReader){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_NodeId publisherIdNode, writerGroupIdNode, dataSetwriterIdNode;

    /* Display DataSetReaderName */
    if(dataSetReader->config.name.length > 512)
        return UA_STATUSCODE_BADCONFIGURATIONERROR;

    UA_STACKARRAY(char, dsrName, sizeof(char) * dataSetReader->config.name.length + 1);
    memcpy(dsrName, dataSetReader->config.name.data, dataSetReader->config.name.length);
    dsrName[dataSetReader->config.name.length] = '\0';
    //This code block must use a lock
    UA_NODESTORE_REMOVE(server, &dataSetReader->identifier);
    retVal |= addPubSubObjectNode(server, dsrName, dataSetReader->identifier.identifier.numeric,
                                  dataSetReader->linkedReaderGroup.identifier.numeric,
                                  UA_NS0ID_HASDATASETREADER, UA_NS0ID_DATASETREADERTYPE);
    //End lock zone

    /* Add childNodes such as PublisherId, WriterGroupId and DataSetWriterId in DataSetReader object */
    publisherIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublisherId"),
                                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                          UA_NODEID_NUMERIC(0, dataSetReader->identifier.identifier.numeric));
    writerGroupIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "WriterGroupId"),
                                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                            UA_NODEID_NUMERIC(0, dataSetReader->identifier.identifier.numeric));
    dataSetwriterIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetWriterId"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                              UA_NODEID_NUMERIC(0, dataSetReader->identifier.identifier.numeric));

    if(UA_NodeId_equal(&publisherIdNode, &UA_NODEID_NULL)
        || UA_NodeId_equal(&writerGroupIdNode, &UA_NODEID_NULL)
        || UA_NodeId_equal(&dataSetwriterIdNode, &UA_NODEID_NULL)){
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_NodePropertyContext *dataSetReaderPublisherIdContext = (UA_NodePropertyContext *) UA_malloc(sizeof(UA_NodePropertyContext));
    dataSetReaderPublisherIdContext->parentNodeId = dataSetReader->identifier;
    dataSetReaderPublisherIdContext->parentClassifier = UA_NS0ID_DATASETREADERTYPE;
    dataSetReaderPublisherIdContext->elementClassiefier = UA_NS0ID_DATASETREADERTYPE_PUBLISHERID;
    UA_ValueCallback valueCallback;
    valueCallback.onRead = onRead;
    valueCallback.onWrite = NULL;
    retVal |= addVariableValueSource(server, valueCallback, publisherIdNode, dataSetReaderPublisherIdContext);

    /* Update childNode with values from Publisher */
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &dataSetReader->config.writerGroupId, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_writeValue(server, writerGroupIdNode, value);
    UA_Variant_setScalar(&value, &dataSetReader->config.dataSetWriterId, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_writeValue(server, dataSetwriterIdNode, value);
    return retVal;
}

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
static UA_StatusCode
addDataSetReaderAction(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionHandle,
                       const UA_NodeId *methodId, void *methodContext,
                       const UA_NodeId *objectId, void *objectContext,
                       size_t inputSize, const UA_Variant *input,
                       size_t outputSize, UA_Variant *output){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_DataSetReaderDataType *dataSetReaderDataType = (UA_DataSetReaderDataType *) input[0].data;
    UA_DataSetReaderConfig readerConfig;
    memset (&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name = dataSetReaderDataType->name;
    /* Parameters to filter which DataSetMessage has to be processed
     * by the DataSetReader */
    if(dataSetReaderDataType->publisherId.type == &UA_TYPES[UA_TYPES_UINT32]){
        UA_UInt32 publisherIdentifier = * ((UA_UInt32 *) dataSetReaderDataType->publisherId.data);
        readerConfig.publisherId.type = &UA_TYPES[UA_TYPES_UINT32];
        readerConfig.publisherId.data = &publisherIdentifier;
    } else if(dataSetReaderDataType->publisherId.type == &UA_TYPES[UA_TYPES_STRING]){
        UA_String publisherIdentifier;
        readerConfig.publisherId.type = &UA_TYPES[UA_TYPES_STRING];
        UA_String_copy((UA_String *) dataSetReaderDataType->publisherId.data, &publisherIdentifier);
        readerConfig.publisherId.data = &publisherIdentifier;
    } else {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER, "Unsupported PublisherId Type used.");
        //TODO what's the best default behaviour here?
    }
    
    readerConfig.writerGroupId    = dataSetReaderDataType->writerGroupId;
    readerConfig.dataSetWriterId  = dataSetReaderDataType->dataSetWriterId;

    /* Setting up Meta data configuration in DataSetReader */
    UA_DataSetMetaDataType *pMetaData;
    pMetaData = &readerConfig.dataSetMetaData;
    UA_DataSetMetaDataType_init (pMetaData);
    pMetaData->name =  dataSetReaderDataType->name;
    pMetaData->fieldsSize = dataSetReaderDataType->dataSetMetaData.fieldsSize;
    pMetaData->fields = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                        &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    for (size_t i = 0; i < pMetaData->fieldsSize; i++){
        UA_FieldMetaData_init (&pMetaData->fields[i]);
        UA_NodeId_copy (&dataSetReaderDataType->dataSetMetaData.fields[i].dataType,
                        &pMetaData->fields[i].dataType);
        pMetaData->fields[i].builtInType = dataSetReaderDataType->dataSetMetaData.fields[i].builtInType;
        pMetaData->fields[i].name = dataSetReaderDataType->dataSetMetaData.fields[i].name;
        pMetaData->fields[i].valueRank = dataSetReaderDataType->dataSetMetaData.fields[i].valueRank;
    }

    UA_NodeId dataSetReaderId;
    retVal |= UA_Server_addDataSetReader(server, *objectId, &readerConfig,
                                         &dataSetReaderId);
    if(retVal != UA_STATUSCODE_GOOD){
        return retVal;
    }
    UA_TargetVariablesDataType targetVars;
    UA_ExtensionObject *eoTargetVar = &dataSetReaderDataType->subscribedDataSet;
    if(eoTargetVar->encoding == UA_EXTENSIONOBJECT_DECODED){
        if(eoTargetVar->content.decoded.type == &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE]){
            if(UA_TargetVariablesDataType_copy((UA_TargetVariablesDataType *) eoTargetVar->content.decoded.data,
                                            &targetVars) != UA_STATUSCODE_GOOD){
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            UA_NodeId folderId;
            UA_String folderName = pMetaData->name;
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

            UA_Server_addObjectNode (server, UA_NODEID_NULL,
                                    UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                                    folderBrowseName, UA_NODEID_NUMERIC (0,
                                    UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);
            /**
            * **TargetVariables**
            *
            * The SubscribedDataSet option TargetVariables defines a list of Variable mappings between
            * received DataSet fields and target Variables in the Subscriber AddressSpace.
            * The values subscribed from the Publisher are updated in the value field of these variables */
            /* Create the TargetVariables with respect to DataSetMetaData fields */
            UA_FieldTargetVariable *targetVarsData = (UA_FieldTargetVariable *)
                    UA_calloc(targetVars.targetVariablesSize, sizeof(UA_FieldTargetVariable));
            for(size_t j = 0; j < targetVars.targetVariablesSize; j++) {
                /* Variable to subscribe data */
                UA_VariableAttributes vAttr = UA_VariableAttributes_default;
                UA_LocalizedText_copy(&pMetaData->fields[j].description,
                                    &vAttr.description);
                vAttr.displayName.locale = UA_STRING("en-US");
                vAttr.displayName.text = pMetaData->fields[j].name;
                vAttr.dataType = pMetaData->fields[j].dataType;

                UA_NodeId newNode;
                retVal |= UA_Server_addVariableNode(server, targetVars.targetVariables[j].targetNodeId,
                                                    folderId,
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                                    UA_QUALIFIEDNAME(1, (char *)pMetaData->fields[j].name.data),
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                    vAttr, NULL, &newNode);

                /* For creating Targetvariables */
                UA_FieldTargetDataType_init(&targetVarsData[j].targetVariable);
                targetVarsData[j].targetVariable.attributeId  = targetVars.targetVariables[j].attributeId;
                targetVarsData[j].targetVariable.targetNodeId = newNode;
            }

            retVal = UA_Server_DataSetReader_createTargetVariables(server, dataSetReaderId,
                                                                    targetVars.targetVariablesSize, targetVarsData);
            for(size_t z = 0; z < targetVars.targetVariablesSize; z++)
                UA_FieldTargetDataType_clear(&targetVarsData[z].targetVariable);
            UA_free(targetVarsData);
            UA_free(pMetaData->fields);
        }
    }
    UA_Variant_setScalarCopy(output, &dataSetReaderId, &UA_TYPES[UA_TYPES_NODEID]);
    return retVal;
}
#endif

UA_StatusCode
removeDataSetReaderRepresentation(UA_Server *server, UA_DataSetReader* dataSetReader){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    retVal |= UA_Server_deleteNode(server, dataSetReader->identifier, false);
    return retVal;
}

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
static UA_StatusCode
removeDataSetReaderAction(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionHandle,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_NodeId nodeToRemove = *((UA_NodeId *) input[0].data);
    retVal |= UA_Server_removeDataSetReader(server, nodeToRemove);
    return retVal;
}
#endif

/*************************************************/
/*                PublishedDataSet               */
/*************************************************/
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
static UA_StatusCode
addDataSetFolderAction(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionHandle,
                       const UA_NodeId *methodId, void *methodContext,
                       const UA_NodeId *objectId, void *objectContext,
                       size_t inputSize, const UA_Variant *input,
                       size_t outputSize, UA_Variant *output){
    /* defined in R 1.04 9.1.4.5.7 */
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_String newFolderName = *((UA_String *) input[0].data);
    UA_NodeId generatedId;
    UA_ObjectAttributes objectAttributes = UA_ObjectAttributes_default;
    UA_LocalizedText name = {UA_STRING("en-US"), newFolderName};
    objectAttributes.displayName = name;
    retVal |= UA_Server_addObjectNode(server, UA_NODEID_NULL, *objectId, UA_NODEID_NUMERIC(0,UA_NS0ID_ORGANIZES),
                                      UA_QUALIFIEDNAME(0, "DataSetFolder"), UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE),
                                      objectAttributes, NULL, &generatedId);
    UA_Variant_setScalarCopy(output, &generatedId, &UA_TYPES[UA_TYPES_NODEID]);
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
    retVal |= UA_Server_addReference(server, generatedId,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                     UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_ADDPUBLISHEDDATAITEMS), true);
    retVal |= UA_Server_addReference(server, generatedId,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                     UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_REMOVEPUBLISHEDDATASET), true);
    retVal |= UA_Server_addReference(server, generatedId,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                     UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_ADDDATASETFOLDER), true);
    retVal |= UA_Server_addReference(server, generatedId,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                     UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_REMOVEDATASETFOLDER), true);
#endif
    return retVal;
}
#endif

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
static UA_StatusCode
removeDataSetFolderAction(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionHandle,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_NodeId nodeToRemove = *((UA_NodeId *) input[0].data);
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
    retVal |= UA_Server_deleteReference(server, nodeToRemove, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), true,
                                        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_ADDPUBLISHEDDATAITEMS),
                                        false);
    retVal |= UA_Server_deleteReference(server, nodeToRemove, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), true,
                                        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_REMOVEPUBLISHEDDATASET),
                                        false);
    retVal |= UA_Server_deleteReference(server, nodeToRemove, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), true,
                                        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_ADDDATASETFOLDER),
                                        false);
    retVal |= UA_Server_deleteReference(server, nodeToRemove, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), true,
                                        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_REMOVEDATASETFOLDER),
                                        false);
#endif
    retVal |= UA_Server_deleteNode(server, nodeToRemove, false);
    return retVal;
}
#endif

UA_StatusCode
addPublishedDataItemsRepresentation(UA_Server *server, UA_PublishedDataSet *publishedDataSet) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(publishedDataSet->config.name.length > 512)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_STACKARRAY(char, pdsName, sizeof(char) * publishedDataSet->config.name.length +1);
    memcpy(pdsName, publishedDataSet->config.name.data, publishedDataSet->config.name.length);
    pdsName[publishedDataSet->config.name.length] = '\0';
    //This code block must use a lock
    UA_NODESTORE_REMOVE(server, &publishedDataSet->identifier);
    retVal |= addPubSubObjectNode(server, pdsName, publishedDataSet->identifier.identifier.numeric,
                                  UA_NS0ID_PUBLISHSUBSCRIBE_PUBLISHEDDATASETS,
                                  UA_NS0ID_HASPROPERTY, UA_NS0ID_PUBLISHEDDATAITEMSTYPE);
    //End lock zone

    UA_ValueCallback valueCallback;
    valueCallback.onRead = onRead;
    valueCallback.onWrite = NULL;

    UA_NodeId configurationVersionNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "ConfigurationVersion"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                            UA_NODEID_NUMERIC(0, publishedDataSet->identifier.identifier.numeric));
    if(UA_NodeId_equal(&configurationVersionNode, &UA_NODEID_NULL))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodePropertyContext * configurationVersionContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    configurationVersionContext->parentNodeId = publishedDataSet->identifier;
    configurationVersionContext->parentClassifier = UA_NS0ID_PUBLISHEDDATAITEMSTYPE;
    configurationVersionContext->elementClassiefier =
        UA_NS0ID_PUBLISHEDDATAITEMSTYPE_CONFIGURATIONVERSION;
    retVal |= addVariableValueSource(server, valueCallback, configurationVersionNode,
                                     configurationVersionContext);

    UA_NodeId publishedDataNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublishedData"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                            UA_NODEID_NUMERIC(0, publishedDataSet->identifier.identifier.numeric));
    if(UA_NodeId_equal(&publishedDataNode, &UA_NODEID_NULL))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodePropertyContext * publishingIntervalContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    publishingIntervalContext->parentNodeId = publishedDataSet->identifier;
    publishingIntervalContext->parentClassifier = UA_NS0ID_PUBLISHEDDATAITEMSTYPE;
    publishingIntervalContext->elementClassiefier = UA_NS0ID_PUBLISHEDDATAITEMSTYPE_PUBLISHEDDATA;
    retVal |= addVariableValueSource(server, valueCallback, publishedDataNode,
                                     publishingIntervalContext);

    UA_NodeId dataSetMetaDataNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetMetaData"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                            UA_NODEID_NUMERIC(0, publishedDataSet->identifier.identifier.numeric));
    if(UA_NodeId_equal(&dataSetMetaDataNode, &UA_NODEID_NULL))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodePropertyContext *metaDataContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    metaDataContext->parentNodeId = publishedDataSet->identifier;
    metaDataContext->parentClassifier = UA_NS0ID_PUBLISHEDDATAITEMSTYPE;
    metaDataContext->elementClassiefier = UA_NS0ID_PUBLISHEDDATAITEMSTYPE_DATASETMETADATA;
    retVal |= addVariableValueSource(server, valueCallback, dataSetMetaDataNode, metaDataContext);

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
    retVal |= UA_Server_addReference(server, publishedDataSet->identifier,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                     UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PUBLISHEDDATAITEMSTYPE_ADDVARIABLES), true);
    retVal |= UA_Server_addReference(server, publishedDataSet->identifier,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                     UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PUBLISHEDDATAITEMSTYPE_REMOVEVARIABLES), true);
#endif
    return retVal;
}

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
static UA_StatusCode
addPublishedDataItemsAction(UA_Server *server,
                            const UA_NodeId *sessionId, void *sessionHandle,
                            const UA_NodeId *methodId, void *methodContext,
                            const UA_NodeId *objectId, void *objectContext,
                            size_t inputSize, const UA_Variant *input,
                            size_t outputSize, UA_Variant *output){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    size_t fieldNameAliasesSize = input[1].arrayLength;
    UA_String * fieldNameAliases = (UA_String *) input[1].data;
    size_t fieldFlagsSize = input[2].arrayLength;
    UA_DataSetFieldFlags * fieldFlags = (UA_DataSetFieldFlags *) input[2].data;
    size_t variablesToAddSize = input[3].arrayLength;
    UA_ExtensionObject *eoAddVar = (UA_ExtensionObject *)input[3].data;

    if(!(fieldNameAliasesSize == fieldFlagsSize || fieldFlagsSize == variablesToAddSize))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(publishedDataSetConfig));
    publishedDataSetConfig.name = *((UA_String *) input[0].data);
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;

    UA_NodeId dataSetItemsNodeId;
    retVal |= UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &dataSetItemsNodeId).addResult;
    if (retVal != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "addPublishedDataset failed");
        return retVal;
    }

    UA_DataSetFieldConfig dataSetFieldConfig;
    for(size_t j = 0; j < variablesToAddSize; ++j) {
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.fieldNameAlias = fieldNameAliases[j];
        if(fieldFlags[j] == UA_DATASETFIELDFLAGS_PROMOTEDFIELD)
            dataSetFieldConfig.field.variable.promotedField = UA_TRUE;

        UA_PublishedVariableDataType variablesToAddField;
        UA_PublishedVariableDataType_init(&variablesToAddField);
        if(eoAddVar[j].encoding == UA_EXTENSIONOBJECT_DECODED){
            if(eoAddVar[j].content.decoded.type == &UA_TYPES[UA_TYPES_PUBLISHEDVARIABLEDATATYPE]){
                if(UA_PublishedVariableDataType_copy((UA_PublishedVariableDataType *) eoAddVar[j].content.decoded.data,
                                                      &variablesToAddField) != UA_STATUSCODE_GOOD){
                    return UA_STATUSCODE_BADOUTOFMEMORY;
                }
            }
        }
        dataSetFieldConfig.field.variable.publishParameters = variablesToAddField;
        retVal |= UA_Server_addDataSetField(server, dataSetItemsNodeId, &dataSetFieldConfig, NULL).result;
        if (retVal != UA_STATUSCODE_GOOD){
           UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "addDataSetField failed");
           return retVal;
        }

        UA_PublishedVariableDataType_clear(&variablesToAddField);
    }

    UA_Variant_setScalarCopy(output, &dataSetItemsNodeId, &UA_TYPES[UA_TYPES_NODEID]);
    return retVal;
}
#endif

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
static UA_StatusCode
addVariablesAction(UA_Server *server,
                   const UA_NodeId *sessionId, void *sessionHandle,
                   const UA_NodeId *methodId, void *methodContext,
                   const UA_NodeId *objectId, void *objectContext,
                   size_t inputSize, const UA_Variant *input,
                   size_t outputSize, UA_Variant *output){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;

    return retVal;
}

static UA_StatusCode
removeVariablesAction(UA_Server *server,
                      const UA_NodeId *sessionId, void *sessionHandle,
                      const UA_NodeId *methodId, void *methodContext,
                      const UA_NodeId *objectId, void *objectContext,
                      size_t inputSize, const UA_Variant *input,
                      size_t outputSize, UA_Variant *output){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;

    return retVal;
}
#endif


UA_StatusCode
removePublishedDataSetRepresentation(UA_Server *server, UA_PublishedDataSet *publishedDataSet){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    retVal |= UA_Server_deleteNode(server, publishedDataSet->identifier, false);

    return retVal;
}

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
static UA_StatusCode
removePublishedDataSetAction(UA_Server *server,
                             const UA_NodeId *sessionId, void *sessionHandle,
                             const UA_NodeId *methodId, void *methodContext,
                             const UA_NodeId *objectId, void *objectContext,
                             size_t inputSize, const UA_Variant *input,
                             size_t outputSize, UA_Variant *output){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_NodeId nodeToRemove = *((UA_NodeId *) input[0].data);
    retVal |= UA_Server_removePublishedDataSet(server, nodeToRemove);
    return retVal;
}
#endif

/**********************************************/
/*               WriterGroup                  */
/**********************************************/

static UA_StatusCode
readContentMask(UA_Server *server, const UA_NodeId *sessionId,
                void *sessionContext, const UA_NodeId *nodeId,
                void *nodeContext, UA_Boolean includeSourceTimeStamp,
                const UA_NumericRange *range, UA_DataValue *value) {
    UA_WriterGroup *writerGroup = (UA_WriterGroup*)nodeContext;
    if((writerGroup->config.messageSettings.encoding != UA_EXTENSIONOBJECT_DECODED &&
        writerGroup->config.messageSettings.encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) ||
       writerGroup->config.messageSettings.content.decoded.type !=
       &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE])
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UadpWriterGroupMessageDataType *wgm = (UA_UadpWriterGroupMessageDataType*)
        writerGroup->config.messageSettings.content.decoded.data;

    UA_Variant_setScalarCopy(&value->value, &wgm->networkMessageContentMask,
                             &UA_TYPES[UA_TYPES_UADPNETWORKMESSAGECONTENTMASK]);
    value->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeContentMask(UA_Server *server, const UA_NodeId *sessionId,
                 void *sessionContext, const UA_NodeId *nodeId,
                 void *nodeContext, const UA_NumericRange *range,
                 const UA_DataValue *value) {
    UA_WriterGroup *writerGroup = (UA_WriterGroup*)nodeContext;
    if((writerGroup->config.messageSettings.encoding != UA_EXTENSIONOBJECT_DECODED &&
        writerGroup->config.messageSettings.encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) ||
       writerGroup->config.messageSettings.content.decoded.type !=
       &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE])
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UadpWriterGroupMessageDataType *wgm = (UA_UadpWriterGroupMessageDataType*)
        writerGroup->config.messageSettings.content.decoded.data;

    if(!value->value.type)
        return UA_STATUSCODE_BADTYPEMISMATCH;
    if(value->value.type->typeKind != UA_DATATYPEKIND_ENUM &&
       value->value.type->typeKind != UA_DATATYPEKIND_INT32)
        return UA_STATUSCODE_BADTYPEMISMATCH;

    wgm->networkMessageContentMask = *(UA_UadpNetworkMessageContentMask*)value->value.data;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
addWriterGroupRepresentation(UA_Server *server, UA_WriterGroup *writerGroup){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(writerGroup->config.name.length > 512)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_STACKARRAY(char, wgName, sizeof(char) * writerGroup->config.name.length + 1);
    memcpy(wgName, writerGroup->config.name.data, writerGroup->config.name.length);
    wgName[writerGroup->config.name.length] = '\0';
    //This code block must use a lock
    UA_NODESTORE_REMOVE(server, &writerGroup->identifier);
    retVal |= addPubSubObjectNode(server, wgName, writerGroup->identifier.identifier.numeric,
                                  writerGroup->linkedConnection->identifier.identifier.numeric,
                                  UA_NS0ID_HASCOMPONENT, UA_NS0ID_WRITERGROUPTYPE);
    //End lock zone
    UA_NodeId keepAliveNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "KeepAliveTime"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                            UA_NODEID_NUMERIC(0, writerGroup->identifier.identifier.numeric));
    UA_NodeId publishingIntervalNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublishingInterval"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                            UA_NODEID_NUMERIC(0, writerGroup->identifier.identifier.numeric));
    if(UA_NodeId_equal(&keepAliveNode, &UA_NODEID_NULL) ||
       UA_NodeId_equal(&publishingIntervalNode, &UA_NODEID_NULL))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodePropertyContext * publishingIntervalContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    publishingIntervalContext->parentNodeId = writerGroup->identifier;
    publishingIntervalContext->parentClassifier = UA_NS0ID_WRITERGROUPTYPE;
    publishingIntervalContext->elementClassiefier = UA_NS0ID_WRITERGROUPTYPE_PUBLISHINGINTERVAL;
    UA_ValueCallback valueCallback;
    valueCallback.onRead = onRead;
    valueCallback.onWrite = onWrite;
    retVal |= addVariableValueSource(server, valueCallback,
                                     publishingIntervalNode, publishingIntervalContext);
    UA_Server_writeAccessLevel(server, publishingIntervalNode,
                               UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE);

    UA_NodeId priorityNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Priority"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                            UA_NODEID_NUMERIC(0, writerGroup->identifier.identifier.numeric));
    UA_NodeId writerGroupIdNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "WriterGroupId"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                            UA_NODEID_NUMERIC(0, writerGroup->identifier.identifier.numeric));
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &writerGroup->config.publishingInterval, &UA_TYPES[UA_TYPES_DURATION]);
    UA_Server_writeValue(server, publishingIntervalNode, value);
    UA_Variant_setScalar(&value, &writerGroup->config.keepAliveTime, &UA_TYPES[UA_TYPES_DURATION]);
    UA_Server_writeValue(server, keepAliveNode, value);
    UA_Variant_setScalar(&value, &writerGroup->config.priority, &UA_TYPES[UA_TYPES_BYTE]);
    UA_Server_writeValue(server, priorityNode, value);
    UA_Variant_setScalar(&value, &writerGroup->config.writerGroupId, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_writeValue(server, writerGroupIdNode, value);

    retVal |= addPubSubObjectNode(server, "MessageSettings", 0,
                                  writerGroup->identifier.identifier.numeric,
                                  UA_NS0ID_HASCOMPONENT, UA_NS0ID_UADPWRITERGROUPMESSAGETYPE);

    /* Find the variable with the content mask */

    UA_NodeId messageSettingsId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "MessageSettings"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_NODEID_NUMERIC(0, writerGroup->identifier.identifier.numeric));
    UA_NodeId contentMaskId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "NetworkMessageContentMask"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), messageSettingsId);
    if(UA_NodeId_equal(&messageSettingsId, &UA_NODEID_NULL) ||
       UA_NodeId_equal(&contentMaskId, &UA_NODEID_NULL)) {
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* Set the callback */
    UA_DataSource ds;
    ds.read = readContentMask;
    ds.write = writeContentMask;
    UA_Server_setVariableNode_dataSource(server, contentMaskId, ds);
    UA_Server_setNodeContext(server, contentMaskId, writerGroup);

    /* Make writable */
    UA_Server_writeAccessLevel(server, contentMaskId,
                               UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_READ);

    return retVal;
}

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
static UA_StatusCode
addWriterGroupAction(UA_Server *server,
                             const UA_NodeId *sessionId, void *sessionHandle,
                             const UA_NodeId *methodId, void *methodContext,
                             const UA_NodeId *objectId, void *objectContext,
                             size_t inputSize, const UA_Variant *input,
                             size_t outputSize, UA_Variant *output){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_WriterGroupDataType *writerGroupDataType = ((UA_WriterGroupDataType *) input[0].data);
    UA_NodeId writerGroupId;
    retVal |= addWriterGroupConfig(server, *objectId, writerGroupDataType, &writerGroupId);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "addWriterGroup failed");
        return retVal;
    }

    UA_Variant_setScalarCopy(output, &writerGroupId, &UA_TYPES[UA_TYPES_NODEID]);
    return retVal;
}
#endif

UA_StatusCode
removeGroupRepresentation(UA_Server *server, UA_WriterGroup *writerGroup) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    retVal |= UA_Server_deleteNode(server, writerGroup->identifier, false);
    return retVal;
}

UA_StatusCode
removeReaderGroupRepresentation(UA_Server *server, UA_ReaderGroup *readerGroup) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    retVal |= UA_Server_deleteNode(server, readerGroup->identifier, false);
    return retVal;
}

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
static UA_StatusCode
removeGroupAction(UA_Server *server,
                             const UA_NodeId *sessionId, void *sessionHandle,
                             const UA_NodeId *methodId, void *methodContext,
                             const UA_NodeId *objectId, void *objectContext,
                             size_t inputSize, const UA_Variant *input,
                             size_t outputSize, UA_Variant *output){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_NodeId nodeToRemove = *((UA_NodeId *) input[0].data);
    if(UA_WriterGroup_findWGbyId(server, nodeToRemove) != NULL)
        retVal |= UA_Server_removeWriterGroup(server, nodeToRemove);
    else
        retVal |= UA_Server_removeReaderGroup(server, nodeToRemove);
    return retVal;
}
#endif

/**********************************************/
/*               ReaderGroup                  */
/**********************************************/
UA_StatusCode
addReaderGroupRepresentation(UA_Server *server, UA_ReaderGroup *readerGroup){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;

    /* Display ReaderGroupName */
    if(readerGroup->config.name.length > 512) {
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }
    else {
    UA_STACKARRAY(char, rgName, sizeof(char) * readerGroup->config.name.length + 1);
    memcpy(rgName, readerGroup->config.name.data, readerGroup->config.name.length);
    rgName[readerGroup->config.name.length] = '\0';
    //This code block must use a lock
    UA_NODESTORE_REMOVE(server, &readerGroup->identifier);

    /* Add object ReaderGroup under PubSubConnectionType object */
    retVal |= addPubSubObjectNode(server, rgName, readerGroup->identifier.identifier.numeric,
                                  readerGroup->linkedConnection.identifier.numeric,
                                  UA_NS0ID_HASCOMPONENT, UA_NS0ID_READERGROUPTYPE);
    }

    //End lock zone
    return retVal;
}

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
static UA_StatusCode
addReaderGroupAction(UA_Server *server,
                     const UA_NodeId *sessionId, void *sessionHandle,
                     const UA_NodeId *methodId, void *methodContext,
                     const UA_NodeId *objectId, void *objectContext,
                     size_t inputSize, const UA_Variant *input,
                     size_t outputSize, UA_Variant *output){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ReaderGroupDataType *readerGroupDataType = ((UA_ReaderGroupDataType *) input[0].data);
    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = readerGroupDataType->name;
    UA_NodeId readerGroupId;
    retVal |= UA_Server_addReaderGroup(server, *objectId, &readerGroupConfig,
                                        &readerGroupId);
    if(retVal != UA_STATUSCODE_GOOD){
        return retVal;
    }

    UA_Variant_setScalarCopy(output, &readerGroupId, &UA_TYPES[UA_TYPES_NODEID]);
    return retVal;
}
#endif

/**********************************************/
/*               DataSetWriter                */
/**********************************************/
UA_StatusCode
addDataSetWriterRepresentation(UA_Server *server, UA_DataSetWriter *dataSetWriter){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(dataSetWriter->config.name.length > 512)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_STACKARRAY(char, dswName, sizeof(char) * dataSetWriter->config.name.length + 1);
    memcpy(dswName, dataSetWriter->config.name.data, dataSetWriter->config.name.length);
    dswName[dataSetWriter->config.name.length] = '\0';
    //This code block must use a lock
    UA_NODESTORE_REMOVE(server, &dataSetWriter->identifier);
    retVal |= addPubSubObjectNode(server, dswName, dataSetWriter->identifier.identifier.numeric,
                                  dataSetWriter->linkedWriterGroup.identifier.numeric,
                                  UA_NS0ID_HASDATASETWRITER, UA_NS0ID_DATASETWRITERTYPE);
    //End lock zone
    retVal |= UA_Server_addReference(server, dataSetWriter->connectedDataSet,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETTOWRITER),
                                     UA_EXPANDEDNODEID_NUMERIC(0, dataSetWriter->identifier.identifier.numeric), true);

    UA_NodeId dataSetWriterIdNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetWriterId"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                            UA_NODEID_NUMERIC(0, dataSetWriter->identifier.identifier.numeric));
    UA_NodeId keyFrameNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "KeyFrameCount"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                            UA_NODEID_NUMERIC(0, dataSetWriter->identifier.identifier.numeric));
    UA_NodeId dataSetFieldContentMaskNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetFieldContentMask"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                            UA_NODEID_NUMERIC(0, dataSetWriter->identifier.identifier.numeric));

    UA_NodePropertyContext *dataSetWriterIdContext = (UA_NodePropertyContext *) UA_malloc(sizeof(UA_NodePropertyContext));
    dataSetWriterIdContext->parentNodeId = dataSetWriter->identifier;
    dataSetWriterIdContext->parentClassifier = UA_NS0ID_DATASETWRITERTYPE;
    dataSetWriterIdContext->elementClassiefier = UA_NS0ID_DATASETWRITERTYPE_DATASETWRITERID;
    UA_ValueCallback valueCallback;
    valueCallback.onRead = onRead;
    valueCallback.onWrite = NULL;
    retVal |= addVariableValueSource(server, valueCallback,
                                     dataSetWriterIdNode, dataSetWriterIdContext);

    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &dataSetWriter->config.dataSetWriterId, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_writeValue(server, dataSetWriterIdNode, value);
    UA_Variant_setScalar(&value, &dataSetWriter->config.keyFrameCount, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Server_writeValue(server, keyFrameNode, value);
    UA_Variant_setScalar(&value, &dataSetWriter->config.dataSetFieldContentMask, &UA_TYPES[UA_TYPES_DATASETFIELDCONTENTMASK]);
    UA_Server_writeValue(server, dataSetFieldContentMaskNode, value);

    retVal |= addPubSubObjectNode(server, "MessageSettings", 0,
                                  dataSetWriter->identifier.identifier.numeric,
                                  UA_NS0ID_HASCOMPONENT, UA_NS0ID_UADPDATASETWRITERMESSAGETYPE);
    return retVal;
}

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
static UA_StatusCode
addDataSetWriterAction(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionHandle,
                       const UA_NodeId *methodId, void *methodContext,
                       const UA_NodeId *objectId, void *objectContext,
                       size_t inputSize, const UA_Variant *input,
                       size_t outputSize, UA_Variant *output){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_NodeId dataSetWriterId;
    UA_DataSetWriterDataType *dataSetWriterDataType = (UA_DataSetWriterDataType *) input[0].data;
    retVal |= addDataSetWriterConfig(server, *objectId, dataSetWriterDataType);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "addDataSetWriter failed");
        return retVal;
    }

    UA_Variant_setScalarCopy(output, &dataSetWriterId, &UA_TYPES[UA_TYPES_NODEID]);
    return UA_STATUSCODE_GOOD;
}
#endif


UA_StatusCode
removeDataSetWriterRepresentation(UA_Server *server, UA_DataSetWriter *dataSetWriter) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    retVal |= UA_Server_deleteNode(server, dataSetWriter->identifier, false);
    return retVal;
}

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
static UA_StatusCode
removeDataSetWriterAction(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionHandle,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_NodeId nodeToRemove = *((UA_NodeId *) input[0].data);
    retVal |= UA_Server_removeDataSetWriter(server, nodeToRemove);
    return retVal;
}
#endif

/**********************************************/
/*                Destructors                 */
/**********************************************/

static void
connectionTypeDestructor(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionContext,
                         const UA_NodeId *typeId, void *typeContext,
                         const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_USERLAND, "Connection destructor called!");
    UA_NodeId publisherIdNode;
    publisherIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublisherId"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), *nodeId);
    UA_NodePropertyContext *internalConnectionContext;
    UA_Server_getNodeContext(server, publisherIdNode, (void **) &internalConnectionContext);
    if(!UA_NodeId_equal(&UA_NODEID_NULL , &publisherIdNode)){
        UA_free(internalConnectionContext);
    }

}

static void
writerGroupTypeDestructor(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *typeId, void *typeContext,
                          const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_USERLAND, "WriterGroup destructor called!");
    UA_NodeId intervalNode;
    intervalNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublishingInterval"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), *nodeId);
    UA_NodePropertyContext *internalConnectionContext;
    UA_Server_getNodeContext(server, intervalNode, (void **) &internalConnectionContext);
    if(!UA_NodeId_equal(&UA_NODEID_NULL , &intervalNode)){
        UA_free(internalConnectionContext);
    }
}

static void
readerGroupTypeDestructor(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *typeId, void *typeContext,
                          const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_USERLAND, "ReaderGroup destructor called!");
}

static void
dataSetWriterTypeDestructor(UA_Server *server,
                            const UA_NodeId *sessionId, void *sessionContext,
                            const UA_NodeId *typeId, void *typeContext,
                            const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_USERLAND, "DataSetWriter destructor called!");
    UA_NodeId dataSetWriterIdNode;
    dataSetWriterIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetWriterId"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), *nodeId);
    UA_NodePropertyContext *internalDSWContext;
    UA_Server_getNodeContext(server, dataSetWriterIdNode, (void **) &internalDSWContext);
    if(!UA_NodeId_equal(&UA_NODEID_NULL, &dataSetWriterIdNode))
        UA_free(internalDSWContext);
}

static void
dataSetReaderTypeDestructor(UA_Server *server,
                            const UA_NodeId *sessionId, void *sessionContext,
                            const UA_NodeId *typeId, void *typeContext,
                            const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_USERLAND, "DataSetReader destructor called!");

    /* Deallocate the memory allocated for publisherId */
    UA_NodeId publisherIdNode;
    publisherIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublisherId"),
                                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), *nodeId);
    UA_NodePropertyContext *internalDSRContext;
    UA_Server_getNodeContext(server, publisherIdNode, (void **) &internalDSRContext);
    if(!UA_NodeId_equal(&UA_NODEID_NULL , &publisherIdNode)){
        UA_free(internalDSRContext);
    }
}

static void
publishedDataItemsTypeDestructor(UA_Server *server,
                            const UA_NodeId *sessionId, void *sessionContext,
                            const UA_NodeId *typeId, void *typeContext,
                            const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                "PublishedDataItems destructor called!");
    void *childContext;
    UA_NodeId node = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublishedData"),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), *nodeId);
    UA_Server_getNodeContext(server, node, (void**)&childContext);
    if(!UA_NodeId_equal(&UA_NODEID_NULL , &node))
        UA_free(childContext);

    node = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "ConfigurationVersion"),
                               UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                               *nodeId);
    UA_Server_getNodeContext(server, node, (void**)&childContext);
    if(!UA_NodeId_equal(&UA_NODEID_NULL , &node))
        UA_free(childContext);

    node = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetMetaData"),
                               UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), *nodeId);
    UA_Server_getNodeContext(server, node, (void**)&childContext);
    if(!UA_NodeId_equal(&node, &UA_NODEID_NULL))
        UA_free(childContext);
}

/*************************************/
/*         PubSub configurator       */
/*************************************/

/* UA_loadPubSubConfigMethodCallback() */
/**
 * @brief   callback function that will be executed when the method "PubSub configurator (replace config)" is called.
 */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS 
#ifdef UA_ENABLE_PUBSUB_FILE_CONFIG
static UA_StatusCode
UA_loadPubSubConfigMethodCallback(UA_Server *server,
                                  const UA_NodeId *sessionId, void *sessionHandle,
                                  const UA_NodeId *methodId, void *methodContext,
                                  const UA_NodeId *objectId, void *objectContext,
                                  size_t inputSize, const UA_Variant *input,
                                  size_t outputSize, UA_Variant *output) {
    if(inputSize == 1) {
        UA_ByteString *inputStr = (UA_ByteString*)input->data;
        return UA_PubSubManager_loadPubSubConfigFromByteString(server, *inputStr);
    } else if(inputSize > 1) {
        return UA_STATUSCODE_BADTOOMANYARGUMENTS;
    } else {
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    }
}
#endif
#endif /*UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS*/


/* UA_addLoadPubSubConfigMethod() */
/**
 * @brief       Adds method node to server. This method is used to load binary files for PubSub 
 *              configuration and delete / replace old PubSub configurations.
 * 
 * @param       server      [bi]    UA_Server object that shall contain the method.
 * 
 * @return      UA_STATUSCODE_GOOD on success
 */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
#ifdef UA_ENABLE_PUBSUB_FILE_CONFIG
static UA_StatusCode 
UA_addLoadPubSubConfigMethod(UA_Server *server) {
    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    inputArgument.description = UA_LOCALIZEDTEXT("en-US", "PubSub config binfile");
    inputArgument.name = UA_STRING("BinFile");
    inputArgument.dataType = UA_TYPES[UA_TYPES_BYTESTRING].typeId;
    inputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes configAttr = UA_MethodAttributes_default;
    configAttr.description = UA_LOCALIZEDTEXT("en-US","Load binary configuration file");
    configAttr.displayName = UA_LOCALIZEDTEXT("en-US","LoadPubSubConfigurationFile");
    configAttr.executable = true;
    configAttr.userExecutable = true;
    UA_StatusCode retVal = UA_Server_addMethodNode(server, UA_NODEID_NULL,
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE),
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                                                   UA_QUALIFIEDNAME(1, "PubSub configuration"),
                                                   configAttr, &UA_loadPubSubConfigMethodCallback,
                                                   1, &inputArgument, 0, NULL, NULL, NULL);
    return retVal;
}
#endif
#endif /*UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS*/


/* UA_deletePubSubConfigMethodCallback() */
/**
 * @brief   callback function that will be executed when the method "PubSub configurator (delete config)" is called.
 */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
#ifdef UA_ENABLE_PUBSUB_FILE_CONFIG
static UA_StatusCode
UA_deletePubSubConfigMethodCallback(UA_Server *server,
                                    const UA_NodeId *sessionId, void *sessionHandle,
                                    const UA_NodeId *methodId, void *methodContext,
                                    const UA_NodeId *objectId, void *objectContext,
                                    size_t inputSize, const UA_Variant *input,
                                    size_t outputSize, UA_Variant *output) {
    UA_PubSubManager_delete(server, &(server->pubSubManager));
    
    return UA_STATUSCODE_GOOD;
}
#endif
#endif /*UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS*/


/* UA_addDeletePubSubConfigMethod() */
/**
 * @brief       Adds method node to server. This method is used to delete the current PubSub configuration.
 * 
 * @param       server      [bi]    UA_Server object that shall contain the method.
 * 
 * @return      UA_STATUSCODE_GOOD on success
 */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
#ifdef UA_ENABLE_PUBSUB_FILE_CONFIG
static UA_StatusCode 
UA_addDeletePubSubConfigMethod(UA_Server *server) {
    UA_MethodAttributes configAttr = UA_MethodAttributes_default;
    configAttr.description = UA_LOCALIZEDTEXT("en-US","Delete current PubSub configuration");
    configAttr.displayName = UA_LOCALIZEDTEXT("en-US","DeletePubSubConfiguration");
    configAttr.executable = true;
    configAttr.userExecutable = true;
    UA_StatusCode retVal = UA_Server_addMethodNode(server, UA_NODEID_NULL,
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE),
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                                                   UA_QUALIFIEDNAME(1, "Delete PubSub config"),
                                                   configAttr, &UA_deletePubSubConfigMethodCallback,
                                                   0, NULL, 0, NULL, NULL, NULL);
    return retVal;
}
#endif
#endif /*UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS*/



UA_StatusCode
UA_Server_initPubSubNS0(UA_Server *server) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_String profileArray[1];
    profileArray[0] = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

    retVal |= writePubSubNs0VariableArray(server, UA_NS0ID_PUBLISHSUBSCRIBE_SUPPORTEDTRANSPORTPROFILES,
                                    profileArray,
                                    1, &UA_TYPES[UA_TYPES_STRING]);

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL_METHODS
    retVal |= UA_Server_setMethodNode_callback(server,
                                               UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_ADDCONNECTION), addPubSubConnectionAction);
    retVal |= UA_Server_setMethodNode_callback(server,
                                               UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_REMOVECONNECTION), removeConnectionAction);
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_PUBLISHEDDATASETS),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                     UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_ADDDATASETFOLDER), true);
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_PUBLISHEDDATASETS),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                     UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_ADDPUBLISHEDDATAITEMS), true);
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_PUBLISHEDDATASETS),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                     UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_REMOVEPUBLISHEDDATASET), true);
    retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_PUBLISHEDDATASETS),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                     UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_REMOVEDATASETFOLDER), true);
    retVal |= UA_Server_setMethodNode_callback(server,
                                               UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_ADDDATASETFOLDER), addDataSetFolderAction);
    retVal |= UA_Server_setMethodNode_callback(server,
                                               UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_REMOVEDATASETFOLDER), removeDataSetFolderAction);
    retVal |= UA_Server_setMethodNode_callback(server,
                                               UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_ADDPUBLISHEDDATAITEMS), addPublishedDataItemsAction);
    retVal |= UA_Server_setMethodNode_callback(server,
                                               UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_REMOVEPUBLISHEDDATASET), removePublishedDataSetAction);
    retVal |= UA_Server_setMethodNode_callback(server,
                                               UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHEDDATAITEMSTYPE_ADDVARIABLES), addVariablesAction);
    retVal |= UA_Server_setMethodNode_callback(server,
                                               UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHEDDATAITEMSTYPE_REMOVEVARIABLES), removeVariablesAction);
    retVal |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBSUBCONNECTIONTYPE_ADDWRITERGROUP), addWriterGroupAction);
    retVal |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBSUBCONNECTIONTYPE_ADDREADERGROUP), addReaderGroupAction);
    retVal |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBSUBCONNECTIONTYPE_REMOVEGROUP), removeGroupAction);
    retVal |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WRITERGROUPTYPE_ADDDATASETWRITER), addDataSetWriterAction);
    retVal |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WRITERGROUPTYPE_REMOVEDATASETWRITER), removeDataSetWriterAction);
    retVal |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_READERGROUPTYPE_ADDDATASETREADER), addDataSetReaderAction);
    retVal |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_READERGROUPTYPE_REMOVEDATASETREADER), removeDataSetReaderAction);

#ifdef UA_ENABLE_PUBSUB_FILE_CONFIG
    retVal |= UA_addLoadPubSubConfigMethod(server);
    retVal |= UA_addDeletePubSubConfigMethod(server);
#endif

#else
    retVal |= UA_Server_deleteReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE), UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), true,
                                        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_ADDCONNECTION),
                                        false);
    retVal |= UA_Server_deleteReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE), UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), true,
                                        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_REMOVECONNECTION),
                                        false);
#endif
    UA_NodeTypeLifecycle lifeCycle;
    lifeCycle.constructor = NULL;
    lifeCycle.destructor = connectionTypeDestructor;
    UA_Server_setNodeTypeLifecycle(server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBSUBCONNECTIONTYPE), lifeCycle);
    lifeCycle.destructor = writerGroupTypeDestructor;
    UA_Server_setNodeTypeLifecycle(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WRITERGROUPTYPE), lifeCycle);
    lifeCycle.destructor = readerGroupTypeDestructor;
    UA_Server_setNodeTypeLifecycle(server, UA_NODEID_NUMERIC(0, UA_NS0ID_READERGROUPTYPE), lifeCycle);
    lifeCycle.destructor = dataSetWriterTypeDestructor;
    UA_Server_setNodeTypeLifecycle(server, UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETWRITERTYPE), lifeCycle);
    lifeCycle.destructor = publishedDataItemsTypeDestructor;
    UA_Server_setNodeTypeLifecycle(server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHEDDATAITEMSTYPE), lifeCycle);
    lifeCycle.destructor = dataSetReaderTypeDestructor;
    UA_Server_setNodeTypeLifecycle(server, UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETREADERTYPE), lifeCycle);

    return retVal;
}

#endif /* UA_ENABLE_PUBSUB_INFORMATIONMODEL */
