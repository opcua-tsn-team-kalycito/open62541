
/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>

#include <stdio.h>
#include <stdlib.h>

//Uncomment thid define to enable history data log
#define HISTORY_DATA_LOG 

#ifdef HISTORY_DATA_LOG
//File pointer initialization 
FILE               *historicalDataFilePointer;
char               *fileHistoricalDB = "HistoricalDB.csv";
#endif

static void
printTimestamp(char *name, UA_DateTime date) {
    UA_DateTimeStruct dts = UA_DateTime_toStruct(date);
    if (name)
        printf("%s: %02u-%02u-%04u %02u:%02u:%02u.%03u, ", name,
               dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
    else
        printf("%02u-%02u-%04u %02u:%02u:%02u.%03u, ",
               dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
}

static void
printDataValue(UA_DataValue *value) {
    /* Print status and timestamps */
    if (value->hasServerTimestamp)
        printTimestamp("ServerTime", value->serverTimestamp);

    if (value->hasSourceTimestamp)
        printTimestamp("SourceTime", value->sourceTimestamp);

    if (value->hasStatus)
        printf("Status 0x%08x, ", value->status);

    if (value->value.type == &UA_TYPES[UA_TYPES_UINT32]) {
        UA_UInt32 hrValue = *(UA_UInt32 *)value->value.data;
        printf("Uint32Value %u\n", hrValue);
    }

    if (value->value.type == &UA_TYPES[UA_TYPES_DOUBLE]) {
        UA_Double hrValue = *(UA_Double *)value->value.data;
        printf("DoubleValue %f\n", hrValue);
    }
#ifdef HISTORY_DATA_LOG
    //File write section
    UA_DateTimeStruct serverTimestamp = UA_DateTime_toStruct(value->serverTimestamp);
    UA_Double hrValue = *(UA_Double *)value->value.data;
    historicalDataFilePointer = fopen(fileHistoricalDB, "a");
    fprintf(historicalDataFilePointer, "%02u-%02u-%04u %02u:%02u:%02u.%03u, %f\n",serverTimestamp.day, serverTimestamp.month,
            serverTimestamp.year, serverTimestamp.hour, serverTimestamp.min, serverTimestamp.sec, serverTimestamp.milliSec, hrValue);
    fclose(historicalDataFilePointer);
#endif
}

static UA_Boolean
readRaw(const UA_HistoryData *data) {
    printf("readRaw Value count: %lu\n", (long unsigned)data->dataValuesSize);

    /* Iterate over all values */
    for (UA_UInt32 i = 0; i < data->dataValuesSize; ++i)
    {
        printDataValue(&data->dataValues[i]);
    }

    /* We want more data! */
    return true;
}

static UA_Boolean
readHist(UA_Client *client, const UA_NodeId *nodeId,
         UA_Boolean moreDataAvailable,
         const UA_ExtensionObject *data, void *unused) {
    printf("\nRead historical callback:\n");
    printf("\tHas more data:\t%d\n\n", moreDataAvailable);
    if (data->content.decoded.type == &UA_TYPES[UA_TYPES_HISTORYDATA]) {
        return readRaw((UA_HistoryData*)data->content.decoded.data);
    }
    return true;
}

int main(int argc, char *argv[]) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    /* Connect to the Unified Automation demo server */
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

    /* Read historical values */
    printf("\nStart historical read of temperature sensor\n");
    UA_NodeId temperatureSensorNodeId = UA_NODEID_NUMERIC(2, 17421);
	
	/* Perform historical data read of values present in temperaturSensor */
    retval = UA_Client_HistoryRead_raw(client, &temperatureSensorNodeId, readHist,
                                       UA_DateTime_fromUnixTime(0), UA_DateTime_now(),
                                       UA_STRING_NULL, false, 100, UA_TIMESTAMPSTORETURN_BOTH, (void *)UA_FALSE);

    if (retval != UA_STATUSCODE_GOOD) {
        printf("Failed. %s\n", UA_StatusCode_name(retval));
    }
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
