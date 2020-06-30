#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>

#include <stdlib.h>

/**
* Read the value attribute of ReadWriteVariableOne variable node from
* server address space with read service.
*/

static void
readVariableOne(UA_Client *client) {

    // Read the value attribute of the node. UA_Client_readValueAttribute is a
    // wrapper for the raw read service available as UA_Client_Service_read
    UA_Variant value; // Variants can hold scalar values and arrays of any type
    UA_Variant_init(&value);    

    // Node Id of the variable node to be read from the address space of the server
    UA_NodeId nodeId = UA_NODEID_STRING(1, "ReadWriteVariableOne");

    // Read the value stored in the variable node
    UA_StatusCode retval = UA_Client_readValueAttribute(client, nodeId, &value);
    if(retval == UA_STATUSCODE_GOOD) {
        printf("\nThe value of node (ReadWriteVariableOne) read from the server"
               "address space = %d\n",
               *(UA_Int32 *)value.data);
    }
    else {
        printf("\nFailed to read ReadWriteVariableOne\n");
    }

    UA_Variant_clear(&value);
}

/**
* Read the value attribute of ReadWriteVariableTwo variable node from
* server address space with read service.
*/

static void
readVariableTwo(UA_Client *client) {

    // Read the value attribute of the node. UA_Client_readValueAttribute is a
    // wrapper for the raw read service available as UA_Client_Service_read
    UA_Variant value; // Variants can hold scalar values and arrays of any type
    UA_Variant_init(&value);    

    // Node Id of the variable node to be read from the address space of the server
    UA_NodeId nodeId = UA_NODEID_NUMERIC(1, 62541);

    // Read the value stored in the variable node
    UA_StatusCode retval = UA_Client_readValueAttribute(client, nodeId, &value);
    if(retval == UA_STATUSCODE_GOOD) {
        printf("\nThe value of node (ReadWriteVariableTwo) read from the server"
               "address space = %d\n",
               *(UA_Int32 *)value.data);
    }
    else {
        printf("\nFailed to read ReadWriteVariableTwo\n");
    }

    UA_Variant_clear(&value);
}

/**
* Change the value attribute of ReadWriteVariableOne variable node in the
* server address space with write service
*/

static void
writeVariableOne(UA_Client *client) {

    // Variable node to hold the value that is to be stored in variable node
    // ReadWriteVariableOne
    UA_Int32 valueToWrite = 0;

    // Get user input for the new value that will be stored in the
    // ReadWriteVariableOne variable node in the server address space
    printf("\nEnter the integer value to be stored in the variable node ReadWriteVariableOne:\n");
    scanf("%d", &valueToWrite);

    // Write node attribute (using the highlevel API)
    UA_Variant *myVariant = UA_Variant_new();
    UA_Variant_setScalarCopy(myVariant, &valueToWrite, &UA_TYPES[UA_TYPES_INT32]);

    UA_StatusCode retval = UA_Client_writeValueAttribute(client, UA_NODEID_STRING(1, "ReadWriteVariableOne"), myVariant);

    // Check the return value if the OPC UA write is done successfully
    if(retval == UA_STATUSCODE_GOOD){
        printf("\nValue of the ReadWriteVariableOne has been updated\n");
    }
    else {
        printf("\nOPC UA Write service call failed for the variable node ReadWriteVariableOne\n");
    }
}

/**
* Change the value attribute of ReadWriteVariableTwo variable node in the
* server address space with write service
*/

static void
writeVariableTwo(UA_Client *client) {

    // Variable node to hold the value that is to be stored in variable node
    // ReadWriteVariableTwo
    UA_Int32 valueToWrite = 0;

    // Get user input for the new value that will be stored in the
    // ReadWriteVariableTwo variable node in the server address space
    printf("\nEnter the integer value to be stored in the variable node ReadWriteVariableTwo:\n");
    scanf("%d", &valueToWrite);

    // Write node attribute (using the highlevel API)
    UA_Variant *myVariant = UA_Variant_new();
    UA_Variant_setScalarCopy(myVariant, &valueToWrite, &UA_TYPES[UA_TYPES_INT32]);

    UA_StatusCode retval = UA_Client_writeValueAttribute(client, UA_NODEID_NUMERIC(1, 62541), myVariant);

    // Check the return value if the OPC UA write is done successfully
    if(retval == UA_STATUSCODE_GOOD){
        printf("\nValue of the ReadWriteVariableTwo has been updated\n");
    }
    else {
        printf("\nOPC UA Write service call failed for the variable node ReadWriteVariableTwo\n");
    }
    

}

int main(void) {

    UA_StatusCode retval;
    char userInput[10];

    // Create a client instance
    UA_Client *client = UA_Client_new();

    // Set default server configuration
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    // Get user input if they want to proceed further with establishing client
    // server connection
    printf("\nConnect to the server endpoint? (yes or no):\n");
    scanf("%s", userInput);

    // If the user provides input "yes" then establish client server communication
    // else abort the client application
    if(strcmp(userInput, "yes") == 0) {
        retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(retval != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
            return (int)retval;
        }
        printf("\nClient-Server connection established successfully\n");
    }
    else {
        printf("\nClient application aborted\n");
        return EXIT_FAILURE;
    }
 
    // Get user input if they want to proceed further with performing OPC UA
    // Read service
    printf("\nDo you want to read the value stored in both variable nodes? (yes or no):\n");
    scanf("%s", userInput);

    // If the user provides input "yes" then perform OPC UA Read service
    // else skip OPC UA Read
    if(strcmp(userInput, "yes") == 0) {
        // User-defined function to read ReadWriteVariableOne variable node from the
        // the server address space
        readVariableOne(client);

        // User-defined function to read ReadWriteVariableTwo variable node from the
        // the server address space
        readVariableTwo(client);
    }
    else {
        printf("\nSkipped OPC UA Read\n");
    }

    // Get user input if they want to proceed further with performing OPC UA
    // Write service
    printf("\nDo you want to update the value stored in both the variable nodes? (yes or no):\n");
    scanf("%s", userInput);

    // If the user provides input "yes" then perform OPC UA Write service
    // else skip OPC UA Write
    if(strcmp(userInput, "yes") == 0) {

        // User-defined function to change the value stored in ReadWriteVariableOne variable node
        writeVariableOne(client);

        // User-defined function to change the value stored in ReadWriteVariableTwo variable node
        writeVariableTwo(client);
    }
    else {
        printf("\nSkipped OPC UA Write\n");
    }

    UA_Client_delete(client); // Disconnects the client internally
    return EXIT_SUCCESS;
}
