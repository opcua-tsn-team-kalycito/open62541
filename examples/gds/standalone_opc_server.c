#include "open62541.h"

UA_Boolean running = true;

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_ServerConfig *config = UA_ServerConfig_new_minimal(4842, NULL);

    UA_Server *server = UA_Server_new(config);
    UA_Server_run(server, &running);

    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return 0;
}
