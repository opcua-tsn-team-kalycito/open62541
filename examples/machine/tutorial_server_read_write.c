#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

static volatile UA_Boolean running = true;

/**
* Add a variable node having datatype UA_Int32 to the server address space.
* NodeId of this variable node will be of type STRING
*/

static void addVariableNodeOne(UA_Server *server) {
    // Initial value to be set for the 'ReadWriteVariableOne' variable node 
    // in the address space of the server
    UA_Int32 variableNodeValue = 0;
        
    // Default attributes for a variable node
    UA_VariableAttributes variableNodeAttr = UA_VariableAttributes_default;

    // Edit the default attributes for the 'ReadWriteVariableOne' variable node
    UA_Variant_setScalar(&variableNodeAttr.value,
                         &variableNodeValue,
                         &UA_TYPES[UA_TYPES_INT32]);
    variableNodeAttr.description = UA_LOCALIZEDTEXT("en-US","A variable that has NodeId of type STRING");
    variableNodeAttr.displayName = UA_LOCALIZEDTEXT("en-US","ReadWriteVariableOne");
    variableNodeAttr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    variableNodeAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    // Define the references of the 'ReadWriteVariableOne' variable node
    UA_NodeId requestedNodeId = UA_NODEID_STRING(1, "ReadWriteVariableOne");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "ReadWriteVariableOne");

    // Add the 'ReadWriteVariableOne' variable node to the address space of the server
    UA_StatusCode retVal = UA_Server_addVariableNode(server,
                                                     requestedNodeId,
                                                     parentNodeId,
                                                     referenceTypeId,
                                                     browseName,
                                                     UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                     variableNodeAttr, NULL, NULL);
    
    if(retVal != UA_STATUSCODE_GOOD) {
        printf("\nFailed to add the 'ReadWriteVariableOne' variable node to the address space of the server\n");
    }
    else
    {
        printf("\n The 'ReadWriteVariableOne' variable node has been added to the address space of the server\n");
    }
    
}

/**
* Add a variable having datatype UA_Int32 to the server address space.
* NodeId of this variable node will be of type NUMERIC
*/

static void addVariableNodeTwo(UA_Server *server) {
    // Initial value to be set for the 'ReadWriteVariableTwo' variable node 
    // in the address space of the server
    UA_Int32 variableNodeValue = 0;
        
    // Default attributes for a variable node
    UA_VariableAttributes variableNodeAttr = UA_VariableAttributes_default;

    // Edit the default attributes for the 'ReadWriteVariableTwo' variable node
    UA_Variant_setScalar(&variableNodeAttr.value,
                         &variableNodeValue,
                         &UA_TYPES[UA_TYPES_INT32]);
    variableNodeAttr.description = UA_LOCALIZEDTEXT("en-US","A variable that has NodeId of type NUMERIC");
    variableNodeAttr.displayName = UA_LOCALIZEDTEXT("en-US","ReadWriteVariableTwo");
    variableNodeAttr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    variableNodeAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    // Define the references of the 'ReadWriteVariableTwo' variable node
    UA_NodeId requestedNodeId = UA_NODEID_NUMERIC(1, 62541);
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "ReadWriteVariableTwo");

    // Add the 'ReadWriteVariableTwo' variable node to the address space of the server
    UA_StatusCode retVal = UA_Server_addVariableNode(server,
                                                     requestedNodeId,
                                                     parentNodeId,
                                                     referenceTypeId,
                                                     browseName,
                                                     UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                     variableNodeAttr, NULL, NULL);

    if(retVal != UA_STATUSCODE_GOOD) {
        printf("\nFailed to add the 'ReadWriteVariableTwo' variable node to the address space of the server\n");
    }
    else
    {
        printf("\n The 'ReadWriteVariableTwo' variable node has been added to the address space of the server\n");
    }

}

/**
* Read the value of the variable added to the server address space with
* read service.
*/
static void
readVariableOne(UA_Server *server, void *data) {

    // Node Id of the variable node to be read from the address space of the server
    UA_NodeId variableNodeNodeId = UA_NODEID_STRING(1, "ReadWriteVariableOne");

    // UA_Variant instance to hold the value of variable node read from the address
    // space of the server
    UA_Variant variableNodeValue;
    UA_Variant_init(&variableNodeValue);

    // Read the value stored in the variable node
    UA_StatusCode retVal = UA_Server_readValue(server, variableNodeNodeId, &variableNodeValue);

    if(retVal != UA_STATUSCODE_GOOD) {
        printf("\nFailed to complete OPC UA read service\n");
    }
    else{
        printf("\nThe value of node (ReadWriteVariableOne) read from the server address space = %d\n", *(UA_Int32 *)variableNodeValue.data);
    }

}

/**
* Read the value of the variable added to the server address space with
* read service.
*/
static void
readVariableTwo(UA_Server *server, void *data) {

    // Node Id of the variable node to be read from the address space of the server
    UA_NodeId variableNodeNodeId = UA_NODEID_NUMERIC(1, 62541);

    // UA_Variant instance to hold the value of variable node read from the address
    // space of the server
    UA_Variant variableNodeValue;
    UA_Variant_init(&variableNodeValue);

    // Read the value stored in the variable node
    UA_StatusCode retVal = UA_Server_readValue(server, variableNodeNodeId, &variableNodeValue);

    if(retVal != UA_STATUSCODE_GOOD) {
        printf("\nFailed to complete OPC UA read service\n");
    }
    else{
        printf("\nThe value of node (ReadWriteVariableTwo) read from the server address space = %d\n", *(UA_Int32 *)variableNodeValue.data);
    }

}

/**
* Now we change the value with the write service. This uses the same service
* implementation that can also be reached over the network by an OPC UA client.
*/

static void
writeVariableOne(UA_Server *server, void *data) {

    // Node Id of the variable node to be read from the address space of the server
    UA_NodeId variableNodeNodeId = UA_NODEID_STRING(1, "ReadWriteVariableOne");

    // UA_Variant instance to hold the value of variable node read from the address
    // space of the server
    UA_Variant variableNodeValueRead;
    UA_Variant_init(&variableNodeValueRead);

    // Read the value stored in the variable node
    UA_StatusCode retVal = UA_Server_readValue(server, variableNodeNodeId, &variableNodeValueRead);

    // Variable to hold the updated value of the variable node in the server address space
    UA_Int32 ReadWriteVariable = *(UA_Int32 *)variableNodeValueRead.data;

    ReadWriteVariable += 1;

    UA_Variant variableNodeValueWrite;
    UA_Variant_init(&variableNodeValueWrite);
    UA_Variant_setScalar(&variableNodeValueWrite, &ReadWriteVariable, &UA_TYPES[UA_TYPES_INT32]);

    // Write the updated value to 
    retVal = UA_Server_writeValue(server, variableNodeNodeId, variableNodeValueWrite);

    if(retVal != UA_STATUSCODE_GOOD) {
        printf("\nFailed to complete OPC UA write service\n");
    }
    else{
        printf("\nValue of the ReadWriteVariableOne has been updated\n");
    }
}

/**
* Now we change the value with the write service. This uses the same service
* implementation that can also be reached over the network by an OPC UA client.
*/

static void
writeVariableTwo(UA_Server *server, void *data) {

    // Node Id of the variable node to be read from the address space of the server
    UA_NodeId variableNodeNodeId = UA_NODEID_NUMERIC(1, 62541);

    // UA_Variant instance to hold the value of variable node read from the address
    // space of the server
    UA_Variant variableNodeValueRead;
    UA_Variant_init(&variableNodeValueRead);

    // Read the value stored in the variable node
    UA_StatusCode retVal = UA_Server_readValue(server, variableNodeNodeId, &variableNodeValueRead);

    // Variable to hold the updated value of the variable node in the server address space
    UA_Int32 ReadWriteVariable = *(UA_Int32 *)variableNodeValueRead.data;

    ReadWriteVariable += 1;

    UA_Variant variableNodeValueWrite;
    UA_Variant_init(&variableNodeValueWrite);
    UA_Variant_setScalar(&variableNodeValueWrite, &ReadWriteVariable, &UA_TYPES[UA_TYPES_INT32]);

    // Write the updated value to 
    retVal = UA_Server_writeValue(server, variableNodeNodeId, variableNodeValueWrite);

    if(retVal != UA_STATUSCODE_GOOD) {
        printf("\nFailed to complete OPC UA write service\n");
    }
    else{
        printf("\nValue of the ReadWriteVariableTwo has been updated\n");
    }
}

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    // Create a server object and set the server configuration as default configuration
    UA_Server *serverObj = UA_Server_new();

    // Set default server configuration
    UA_ServerConfig_setDefault(UA_Server_getConfig(serverObj));

    // User-defined function to add a ReadWriteVariableOne variable node to the server address space
    addVariableNodeOne(serverObj);

    // User-defined function to add a ReadWriteVariableTwo variable node to the server address space
    addVariableNodeTwo(serverObj);

    // User-defined callback to read the value stored in ReadWriteVariableOne variable node
    UA_Server_addRepeatedCallback(serverObj, readVariableOne, NULL, 5000, NULL);

    // User-defined callback to read the value stored in ReadWriteVariableTwo variable node
    UA_Server_addRepeatedCallback(serverObj, readVariableTwo, NULL, 5000, NULL);

    // User-defined callback to change the value stored in ReadWriteVariableOne variable node
    UA_Server_addRepeatedCallback(serverObj, writeVariableOne, NULL, 5000, NULL);

    // User-defined callback to change the value stored in ReadWriteVariableTwo variable node
    UA_Server_addRepeatedCallback(serverObj, writeVariableTwo, NULL, 5000, NULL);

    // Start the server
    UA_StatusCode retval = UA_Server_run(serverObj, &running);

    UA_Server_delete(serverObj);

    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
