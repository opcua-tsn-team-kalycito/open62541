#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include "open62541/namespace_machineim_generated.h"

static volatile UA_Boolean running = true;

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    // Create a new server instance
    UA_Server *server = UA_Server_new();

    // Creates a server config on the default port 4840 with no server certificate
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    // Function call to add the custom machine information model into the server
    // address space
    UA_StatusCode retval = namespace_machineim_generated(server);
    if(retval != UA_STATUSCODE_GOOD) {
	UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Adding the MachineIM namespace failed. Please check previous error output.");
	UA_Server_delete(server);
	return EXIT_FAILURE;
    }

    // Function to keep the server continously running
    retval = UA_Server_run(server, &running);

    // Delete the created server instance
    UA_Server_delete(server);

    return EXIT_SUCCESS;
}
