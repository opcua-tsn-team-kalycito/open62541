PRE-REQUISITES:
    Tested in RT enabled kernel (4.19.37-rt19) with lubuntu as OS in nodes (say node1 and node2)
    Install llvm and clang
        apt-get install llvm
        apt-get install clang
    Clone bpf-next (v4.19) using the following steps:
        cd /usr/local/src/
        git clone https://github.com/xdp-project/bpf-next.git
        cd bpf-next/
        git checkout 84df9525b0c27f3ebc2ebb1864fa62a97fdedb7d
        make defconfig
        make headers_install
        cd samples/bpf
        make 

Ensure the nodes are ptp synchronized
PTP SYNCHRONIZATION:
Clone and install linuxptp version 2.0
    git clone https://github.com/richardcochran/linuxptp.git
    cd linuxptp/
    git checkout -f 059269d0cc50f8543b00c3c1f52f33a6c1aa5912
Install application
    make
    make install
    cp configs/gPTP.cfg /etc/linuxptp/
    cd ../

To make node as ptp master run the following command (say in node1):
    sudo daemonize -E BUILD_ID=dontKillMe -o /var/log/ptp4l.log -e /var/log/ptp4l.err.log /usr/bin/taskset -c 1 chrt 90 /usr/local/sbin/ptp4l -i <I210 interface> -2 -mq -f /etc/linuxptp/gPTP.cfg --step_threshold=1 --fault_reset_interval=0 --announceReceiptTimeout=10
To make node as ptp slave run the following command (say in node2):
    sudo daemonize -E BUILD_ID=dontKillMe -o /var/log/ptp4l.log -e /var/log/ptp4l.err.log /usr/bin/taskset -c 1 chrt 90 /usr/local/sbin/ptp4l -i <I210 interface> -2 -mq -s -f /etc/linuxptp/gPTP.cfg --step_threshold=1 --fault_reset_interval=0 --announceReceiptTimeout=10
To ensure phc2sys synchronization (in both nodes, node1 and node2):
    sudo daemonize -E BUILD_ID=dontKillMe -o /var/log/phc2sys.log -e /var/log/phc2sys.err.log /usr/bin/taskset -c 1 chrt 89 /usr/local/sbin/phc2sys -s <I210 interface> -c CLOCK_REALTIME --step_threshold=1 --transportSpecific=1 -w -m 

Check if /etc/modules has entry for 8021q
    cat /etc/modules | grep '8021q'
===========================================================================================================================================================================

For ETF Transmit:
In both nodes:

#Clean up any existing setting
    sudo tc qdisc del dev <I210 interface> root

#Configure ETF
    sudo tc qdisc add dev <I210 interface> parent root mqprio num_tc 3 map 2 2 1 0 2 2 2 2 2 2 2 2 2 2 2 2 queues 1@0 1@1 2@2 hw 0

MQPRIO_NUM=`sudo tc qdisc show | grep mqprio | cut -d ':' -f1 | cut -d ' ' -f3`
    sudo tc qdisc add dev <I210 interface> parent $MQPRIO_NUM:1 etf offload clockid CLOCK_TAI delta 150000
    sudo tc qdisc add dev <I210 interface> parent $MQPRIO_NUM:2 etf offload clockid CLOCK_TAI delta 150000 

============================================================================================================================================================================
For XDP receive:

As per the sample application, XDP listens to Rx_Queue 2; to direct the incoming ethernet packets to the second Rx_Queue, the follow the given steps
    In both nodes:
        #Disable VLAN offload
        ethtool -K <I210 interface> rxvlan off
        tc qdisc delete dev <I210 interface> ingress
        tc qdisc add dev <I210 interface> ingress
    In node 1:
        tc filter add dev <I210 interface> parent ffff: proto 0xb62c flower dst_mac 01:00:5E:00:00:01 hw_tc 2 skip_sw
    In node 2:
        tc filter add dev <I210 interface> parent ffff: proto 0xb62c flower dst_mac 01:00:5E:7F:00:01 hw_tc 2 skip_sw

    To make XDP listen to other RX_Queue, change RECEIVE_QUEUE value in TSN_ETF_XDP_publisher.c and TSN_ETF_XDP_loopback.c applications, also follow the above INGRESS POLICY steps with hw_tc to be the same value as given in receive_queue

NOTE: By default, XDP runs in SKB mode

To run ETF in Publisher and XDP in Subscriber in two nodes connected in peer-to-peer network
    cd open62541/build
    cmake -DUA_BUILD_EXAMPLES=OFF -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING=ON -DUA_ENABLE_PUBSUB_ETH_UADP=ON -DUA_ENABLE_PUBSUB_ETH_UADP_ETF=ON -DUA_ENABLE_PUBSUB_ETH_UADP_XDP=ON ..
    make

Execute the following gcc command to build publisher and subscriber applications
gcc -O2 ../examples/pubsub_realtime/TSN_ETF_XDP_publisher.c -I../include -I../plugins/include -Isrc_generated -I../arch/posix -I../arch -I../plugins/networking -I../deps/ -I../src/server -I../src -I../src/pubsub bin/libopen62541.a /usr/local/src/bpf-next/tools/lib/bpf/libbpf.a -lrt -lpthread -lelf -D_GNU_SOURCE -o ./bin/TSN_ETF_XDP_publisher
gcc -O2 ../examples/pubsub_realtime/TSN_ETF_XDP_loopback.c -I../include -I../plugins/include -Isrc_generated -I../arch/posix -I../arch -I../plugins/networking -I../deps/ -I../src/server -I../src -I../src/pubsub bin/libopen62541.a /usr/local/src/bpf-next/tools/lib/bpf/libbpf.a -lrt -lpthread -lelf -D_GNU_SOURCE -o ./bin/TSN_ETF_XDP_loopback

The generated binaries are generated in build/bin/ folder
    ./bin/TSN_ETF_XDP_publisher <I210 interface> - Run in node 1
    ./bin/TSN_ETF_XDP_loopback <I210 interface>  - Run in node 2
Eg: ./bin/TSN_ETF_XDP_publisher enp2s0
    ./bin/TSN_ETF_XDP_loopback enp2s0

NOTE: It is always recommended to run TSN_ETF_XDP_loopback application first to avoid missing the initial data published by TSN_ETF_XDP_publisher
====================================================================================================================================================================
NOTE:
To change CYCLE_TIME for applications, change CYCLE_TIME macro value in the following files:
    examples/pubsub_realtime/TSN_ETF_XDP_loopback.c
    examples/pubsub_realtime/TSN_ETF_XDP_publisher.c

Eg: To run applications in 100us cycletime, modify CYCLE_TIME value as 0.10 in TSN_ETF_XDP_publisher.c and TSN_ETF_XDP_loopback.c applications

To modify the existing multicast MAC address with physical MAC address, modify
    PUBLISHING_MAC_ADDRESS to the MAC address of the other node
    SUBSCRIBING_MAC_ADDRESS to the MAC address of the same node
NOTE: 
    1. If VLAN tag is used:
            Ensure the MAC address is given along with VLAN ID and PCP
            Eg: "opc.eth://01-00-5E-00-00-01:8.3" where 8 is the VLAN ID and 3 is the PCP
    2. If VLAN tag is not used:
            Ensure the MAC address is not given along with VLAN ID and PCP
            Eg: "opc.eth://01-00-5E-00-00-01"
    3. If MAC address is changed, follow INGRESS POLICY steps with dst_mac address to be the same as SUBSCRIBING_MAC_ADDRESS for both nodes

To write the published counter data along with timestamp in csv uncomment #define UPDATE_MEASUREMENTS in TSN_ETF_XDP_publisher.c and TSN_ETF_XDP_loopback.c applications

NOTE: The CSV files will be generated in the build directory containing the timestamp and counter values for benchmarking only after terminating both the applications using “Ctrl + C”

To increase the payload size, change the REPEATED_NODECOUNTS macro in TSN_ETF_XDP_publisher.c and TSN_ETF_XDP_loopback.c applications (for 1 REPEATED_NODECOUNTS, 8 bytes of data will increase in payload)
Eg: To increase the payload to 64 bytes, change REPEATED_NODECOUNTS to 4
=====================================================================================================================================================================

TO RUN THE UNIT TEST CASES FOR ETHERNET FUNCTIONALITY:

Change the ethernet interface in #define ETHERNET_INTERFACE macro in
    check_pubsub_connection_ethernet.c
    check_pubsub_publish_ethernet.c
    check_pubsub_connection_ethernet_etf.c
    check_pubsub_publish_ethernet_etf.c

1. To test ethernet connection creation and ethernet publish unit tests(without realtime)
        cd open62541/build
        make clean
        cmake -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_ETH_UADP=ON -DUA_BUILD_UNIT_TESTS=ON ..
        make
        The following binaries are generated in build/bin/tests folder
          ./bin/tests/check_pubsub_connection_ethernet - To check ethernet connection creation
          ./bin/tests/check_pubsub_publish_ethernet    - To check ethernet send functionality

2. To test ethernet connection creation and ethernet publish unit tests(with realtime)
        cd open62541/build
        make clean
        cmake -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING=OFF -DUA_ENABLE_PUBSUB_ETH_UADP=ON -DUA_ENABLE_PUBSUB_ETH_UADP_ETF=ON -DUA_BUILD_UNIT_TESTS=ON ..
        make
        The following binaries are generated in build/bin/tests folder
          ./bin/tests/check_pubsub_connection_ethernet_etf - To check ethernet connection creation with ETF
          ./bin/tests/check_pubsub_publish_ethernet_etf - To check ethernet send functionality with ETF


TO RUN THE UNIT TEST CASES FOR XDP FUNCTIONALITY:

Change the ethernet interface in #define ETHERNET_INTERFACE macro in
    check_pubsub_connection_xdp.c

1. To test connection creation using XDP transport layer
        cd open62541/build
        make clean
        cmake -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_ETH_UADP=ON -DUA_BUILD_UNIT_TESTS=ON -DUA_ENABLE_PUBSUB_ETH_UADP_ETF=ON -DUA_ENABLE_PUBSUB_ETH_UADP_XDP=ON ..
        make
        The following binary is generated in build/bin/tests folder
          ./bin/tests/check_pubsub_connection_xdp - To check connection creation with XDP