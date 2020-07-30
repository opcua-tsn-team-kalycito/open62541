####################
# Machine Examples #
####################

###################
# Custom XML      #
###################
if(NOT CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # needed or cmake doesn't recognize dependencies of generated files
    set(PROJECT_BINARY_DIR ${CMAKE_BINARY_DIR})
endif()

set(CMAKE_C_FLAGS "-std=c99")
set(LIBS ${LIBS} sharedMemoryRead)
set(LIBS ${LIBS} tripleBufferRead)

if(UA_NAMESPACE_ZERO STREQUAL "FULL" OR UA_NAMESPACE_ZERO STREQUAL "REDUCED")

    # generate machine IM namespace
    ua_generate_nodeset_and_datatypes(
        NAME "machineim"
        FILE_NS "${PROJECT_SOURCE_DIR}/machine/MachineIM.xml"
        DEPENDS "${CMAKE_SOURCE_DIR}/deps/ua-nodeset/Schema/Opc.Ua.NodeSet2.xml"
        INTERNAL
    )
if(UA_ENABLE_SUBSCRIPTIONS_EVENTS)
if(UA_ENABLE_HISTORIZING)
    add_example(tutorial_server tutorial_server.c
                ${UA_NODESET_MACHINEIM_SOURCES})
    target_link_libraries(tutorial_server ${LIBS})
    add_dependencies(tutorial_server open62541-generator-ns-machineim)
endif()
endif()
if(UA_ENABLE_HISTORIZING)
    add_example(tutorial_server_historical_data_access tutorial_server_historical_data_access.c
    ${UA_NODESET_MACHINEIM_SOURCES})
    target_link_libraries(tutorial_server_historical_data_access ${LIBS})
    add_dependencies(tutorial_server_historical_data_access open62541-generator-ns-machineim)
    add_example(tutorial_client_historical_data_access tutorial_client_historical_data_access.c)
endif()
    add_library(sharedMemoryRead sharedMemoryRead.c)
    add_library(tripleBufferRead tripleBufferRead.c)

    add_example(tutorial_client tutorial_client.c
                ${UA_NODESET_MACHINEIM_SOURCES})
    add_dependencies(tutorial_client open62541-generator-ns-machineim)
if(UA_ENABLE_SUBSCRIPTIONS_EVENTS)
    add_example(tutorial_server_information_model tutorial_server_information_model.c
                ${UA_NODESET_MACHINEIM_SOURCES})
    target_link_libraries(tutorial_server_information_model ${LIBS})
    add_dependencies(tutorial_server_information_model open62541-generator-ns-machineim)
    add_example(tutorial_server_events_custom_IM tutorial_server_events_custom_IM.c
                ${UA_NODESET_MACHINEIM_SOURCES})
    add_dependencies(tutorial_server_events_custom_IM open62541-generator-ns-machineim)
    add_example(tutorial_client_events_custom_IM tutorial_client_events_custom_IM.c
                ${UA_NODESET_MACHINEIM_SOURCES})
    add_dependencies(tutorial_client_events_custom_IM open62541-generator-ns-machineim)
if(UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS)
    add_example(tutorial_server_alarms_conditions_simplified tutorial_server_alarms_conditions_simplified.c)
    add_example(tutorial_client_alarms_conditions tutorial_client_alarms_conditions.c)
endif()
endif()
endif()
