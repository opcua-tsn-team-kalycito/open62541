TO RUN ETF APPLICATIONS:

To run ETF applications in two nodes connected in peer-to-peer network - 
    cd open62541/build
    ccmake ..
    Enable the following macros
          UA_BUILD_EXAMPLES
          UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING
          UA_ENABLE_PUBSUB
          UA_ENABLE_PUBSUB_ETH_UADP
    make

To change CYCLE_TIME for applications, change CYCLE_TIME value in the following files:
    open62541/plugins/ua_pubsub_realtime.c
    examples/pubsub_realtime/TSN_ETF_loopback.c
    examples/pubsub_realtime/TSN_ETF_publisher.c
Eg: To run applications in 250us cycletime, modify CYCLE_TIME value as 250 * 1000 in all three above mentioned files


The following binaries are generated in build/bin/examples
    ./bin/examples/TSN_ETF_publisher "opc.eth://MAC of node2" "opc.eth://MAC of node 1" interface_name - Run in node 1
    ./bin/examples/TSN_ETF_loopback "opc.eth://MAC of node1" "opc.eth://MAC of node 2" interface_name - Run in node 2

    Eg.: ./bin/examples/TSN_ETF_publisher "opc.eth://a0-36-9f-04-5b-11" "opc.eth://a0-36-9f-2d-01-bf" enp4s0
         ./bin/examples/TSN_ETF_loopback "opc.eth://a0-36-9f-2d-01-bf" "opc.eth://a0-36-9f-04-5b-11" enp4s0

To write the published counter data along with timestamp in csv uncomment #define UPDATE_MEASUREMENTS in TSN_ETF_publisher.c and TSN_ETF_loopback.c applications

=====================================================================================================================================================================

TO RUN THE UNIT TEST CASES FOR ETHERNET FUNCTIONALITY:

Change the ethernet interface in #define ETHERNET_INTERFACE macro in
    check_pubsub_connection_ethernet.c
    check_pubsub_publish_ethernet.c
    check_pubsub_publish_realtime_ethernet.c

1. To test ethernet connection creation and ethernet publish unit tests(without realtime)
        cd open62541/build
        ccmake ..
        Enable the following macros
          UA_ENABLE_UNIT_TESTS
          UA_ENABLE_PUBSUB
          UA_ENABLE_PUBSUB_ETH_UADP
        make
        The following binaries are generated in build/bin/tests folder
          ./bin/tests/check_pubsub_connection_ethernet - To check ethernet connection creation
          ./bin/tests/check_pubsub_publish_ethernet - To check ethernet send functionality

2. To test ethernet publish unit test(with realtime)
        cd open62541/build
        ccmake ..
        Enable the following macros
          UA_ENABLE_UNIT_TESTS
          UA_ENABLE_PUBSUB
          UA_ENABLE_PUBSUB_ETH_UADP
          UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING
        make
        The following binaries are generated in build/bin/tests folder
          ./bin/tests/check_pubsub_publish_realtime_ethernet - To check ethernet send functionality with realtime


