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

    add_library(sharedMemoryRead sharedMemoryRead.c)
    add_library(tripleBufferRead tripleBufferRead.c)

    add_example(tutorial_server_read_write tutorial_server_read_write.c)
    add_example(tutorial_client_read_write tutorial_client_read_write.c)

    add_example(tutorial_server_information_model tutorial_server_information_model.c
                ${UA_NODESET_MACHINEIM_SOURCES})
    target_link_libraries(tutorial_server_information_model ${LIBS})
    add_dependencies(tutorial_server_information_model open62541-generator-ns-machineim)

    add_example(tutorial_server_methodcall tutorial_server_methodcall.c
                ${UA_NODESET_MACHINEIM_SOURCES})
    target_link_libraries(tutorial_server_methodcall ${LIBS})
    add_dependencies(tutorial_server_methodcall open62541-generator-ns-machineim)

    add_example(tutorial_client_methodcall tutorial_client_methodcall.c
                ${UA_NODESET_MACHINEIM_SOURCES})
    add_dependencies(tutorial_client_methodcall open62541-generator-ns-machineim)

    add_example(tutorial_server_subscription_datachange tutorial_server_subscription_datachange.c
	        ${UA_NODESET_MACHINEIM_SOURCES})
    target_link_libraries(tutorial_server_subscription_datachange ${LIBS})
    add_dependencies(tutorial_server_subscription_datachange open62541-generator-ns-machineim)

    add_example(tutorial_client_subscription_datachange tutorial_client_subscription_datachange.c
            ${UA_NODESET_MACHINEIM_SOURCES})
    add_dependencies(tutorial_client_subscription_datachange open62541-generator-ns-machineim)

endif()
