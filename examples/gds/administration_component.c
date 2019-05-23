/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */
/*
 * A simple server instance which registers with the discovery server (see server_lds.c).
 * Before shutdown it has to unregister itself.
 */

#include "open62541.h"

UA_Boolean running = true;

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int main(int argc, char **argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    // Initialize the client and connect to the GDS
    UA_ClientConfig config = UA_ClientConfig_default;
    UA_String applicationUri = UA_String_fromChars("urn:open62541.example.server_register");
    UA_Client *client = UA_Client_new(config);
    /* Change the localhost to the IP running GDS if needed */
    UA_StatusCode retval = UA_Client_connect_username(client, "opc.tcp://localhost:4841", "user1", "password");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        UA_String_deleteMembers(&applicationUri);
        return (int)retval;
    }

    /* A client to connect to the OPC UA server for pushing the certificate */
    UA_Client *client_push = UA_Client_new(UA_ClientConfig_default);
    retval = UA_Client_connect(client_push, "opc.tcp://localhost:4842");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client_push);
        return (int)retval;
    }

    /* Every ApplicationURI shall be unique.
     * Therefore the client should be sure that the application is not registered yet. */
    size_t length = 0;
    UA_ApplicationRecordDataType *records = NULL;
    UA_GDS_call_findApplication(client, applicationUri, &length, records);

    if (!length) {
        // Register Application
        UA_NodeId appId = UA_NODEID_NULL;
        UA_ApplicationRecordDataType record;
        memset(&record, 0, sizeof(UA_ApplicationRecordDataType));
        record.applicationUri = applicationUri;
        record.applicationType = UA_APPLICATIONTYPE_SERVER;
        record.productUri = UA_STRING("urn:open62541.example.server_register");
        record.applicationNamesSize++;
        UA_LocalizedText applicationName = UA_LOCALIZEDTEXT("en-US", "open62541_Server");
        record.applicationNames = &applicationName;
        record.discoveryUrlsSize++;
        UA_String discoveryUrl = UA_STRING("opc.tcp://localhost:4840");
        record.discoveryUrls = &discoveryUrl;
        record.serverCapabilitiesSize++;
        UA_String serverCap = UA_STRING("LDS");
        record.serverCapabilities = &serverCap;

        UA_GDS_call_registerApplication(client, &record, &appId);

        //Request a new application instance certificate (with the associated private key)
        UA_NodeId requestId;
        UA_ByteString certificaterequest;
        UA_String  subjectName  = UA_STRING("C=DE,O=open62541,CN=open62541@localhost");
        UA_Boolean regenPrivKey = 1;

        UA_GDS_call_createSigningRequest(client_push, &UA_NODEID_NULL, &UA_NODEID_NULL, &subjectName,
                                         &regenPrivKey, &UA_BYTESTRING_NULL, &certificaterequest);

        UA_GDS_call_startSigningRequest(client, &appId, &UA_NODEID_NULL, &UA_NODEID_NULL,
                                        &certificaterequest, &requestId);

        //Fetch the certificate and private key
        UA_ByteString certificate;
        UA_ByteString privateKey;
        UA_ByteString issuerCertificate;
        if (!UA_NodeId_isNull(&requestId)){
            retval = UA_GDS_call_finishRequest(client, &appId, &requestId,
                                               &certificate, &privateKey, &issuerCertificate);

            /* To Do: Update Certificate */
        }
    }

    UA_String_deleteMembers(&applicationUri);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return (int)retval;
}
