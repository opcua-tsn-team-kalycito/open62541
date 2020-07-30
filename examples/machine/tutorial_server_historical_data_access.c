/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Building a Simple Server
 * ------------------------
 *
 * This series of tutorial guide you through your first steps with open62541.
 * For compiling the examples, you need a compiler (MS Visual Studio 2015 or
 * newer, GCC, Clang and MinGW32 are all known to be working). The compilation
 * instructions are given for GCC but should be straightforward to adapt.
 *
 * It will also be very helpful to install an OPC UA Client with a graphical
 * frontend, such as UAExpert by Unified Automation. That will enable you to
 * examine the information model of any OPC UA server.
 *
 * To get started, downdload the open62541 single-file release from
 * http://open62541.org or generate it according to the :ref:`build instructions
 * <building>` with the "amalgamation" option enabled. From now on, we assume
 * you have the ``open62541.c/.h`` files in the current folder. Now create a new
 * C source-file called ``myServer.c`` with the following content: */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include "open62541/namespace_machineim_generated.h"
#include "../src/server/ua_server_internal.h"
#include "sharedMemoryRead.h"
#include "tripleBufferRead.h"

#include <open62541/plugin/historydata/history_data_backend_memory.h>
#include <open62541/plugin/historydata/history_data_gathering_default.h>
#include <open62541/plugin/historydata/history_database_default.h>
#include <open62541/plugin/historydatabase.h>

#include <signal.h>
#include <stdlib.h>

static volatile UA_Boolean running = true;

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}


// Function to generate random double values for the data source variables
// To be replaced with actual data from shared memory
static UA_StatusCode
readSharedMemoryData (UA_Server *server,
             const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *nodeId, void *nodeContext,
             UA_Boolean sourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

#ifdef UA_ARCHITECTURE_POSIX
    //Create a double variable and point to the shared memory location to read data
	UA_Double dataRead = 0;
	readFromBuffer(&dataRead, sizeof(dataRead));
    //printf("Data from Shared Memory: %f\n", dataRead);
#else
    UA_Double dataRead = 0;
#endif
    UA_Variant_setScalarCopy(&value->value, &dataRead, &UA_TYPES[UA_TYPES_DOUBLE]);
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }

   return UA_STATUSCODE_GOOD;
}


int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

#ifdef UA_ARCHITECTURE_POSIX
    createSharedMemory();
#endif

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    UA_StatusCode retval = namespace_machineim_generated(server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Adding the MachineIM namespace failed. Please check previous error output.");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    //NodeID of Temperature sensor 
    UA_NodeId outNodeId = UA_NODEID_NUMERIC(2, 17421);
    
    //Assign TemperatureSensor as Data source variable and input shared memory data for read 
    UA_DataSource sharedMemDataSource;
    sharedMemDataSource.read = readSharedMemoryData;
	
    //Set Temperature sensor as data source variable
    setVariableNode_dataSource(server, outNodeId, sharedMemDataSource);
	
	//We need a gathering for the plugin to constuct.
    //The UA_HistoryDataGathering is responsible to collect data and store it to the database.
    //We will use this gathering for one node, only. initialNodeIdStoreSize = 1
    //The store will grow if you register more than one node, but this is expensive.
    UA_HistoryDataGathering gathering = UA_HistoryDataGathering_Default(1);
	
	// We set the responsible plugin in the configuration. UA_HistoryDatabase is
    // the main plugin which handles the historical data service. 
    config->historyDatabase = UA_HistoryDatabase_default(gathering);

	// Now we define the settings for our node 
    UA_HistorizingNodeIdSettings setting;
	
	// There is a memory based database plugin. We will use that. We just
    // reserve space for 1 node. This will also
    // automaticaly grow if needed, but that is expensive, because all data must
    // be copied.
    //Setup the backend memory to store data from Temperature sensor
    setting.historizingBackend = UA_HistoryDataBackend_Memory(1,100);
	
	//We want the server to serve a maximum of 100 values per request. This
    //value depend on the plattform you are running the server. A big server
    //can serve more values, smaller ones less.
    //Setup the maximum response size
    setting.maxHistoryDataResponseSize = 100;
	
    //Select update strategy as POLL suitable for dataSource variable
    setting.historizingUpdateStrategy = UA_HISTORIZINGUPDATESTRATEGY_POLL;
	
	//If we have a sensor which do not report updates
    //and need to be polled we change the setting like that.
    //The polling interval in ms.
    //Configure the polling interval
    setting.pollingInterval = 100;
	
    //Registed temperatureSensor nodeID to save data in the database
    retval = gathering.registerNodeId(server, gathering.context, &outNodeId, setting);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "registerNodeId %s", UA_StatusCode_name(retval));
	
     //Start polling to gather data into the database
    retval = gathering.startPoll(server, gathering.context, &outNodeId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "startPoll %s", UA_StatusCode_name(retval));

    retval = UA_Server_run(server, &running);

    //Stop polling historical data as the server is stopped
    retval = gathering.stopPoll(server, gathering.context, &outNodeId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "stopPoll %s", UA_StatusCode_name(retval));
    UA_Server_delete(server);

    // Delete the shared memory
    sharedMemoryDelete();
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
