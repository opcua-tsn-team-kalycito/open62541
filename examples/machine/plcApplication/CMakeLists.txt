#########################################
# PLC Shared Memory Writter Application #
#########################################
cmake_minimum_required (VERSION 3.10)

#search paths for included headers
#include_directories("${PROJECT_BINARY_DIR}")

#set executable archiving path
set(EXECUTABLE_OUTPUT_PATH bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY build)

#library files to be included
set(LIBS ${LIBS} tripleBufferWrite)

#generate executable and link it with the libraries
add_executable(plcSharedMemoryWriter plcSharedMemoryWriter.c)
target_link_libraries(plcSharedMemoryWriter ${LIBS})
target_link_libraries(plcSharedMemoryWriter rt)

#generate library files
add_library(tripleBufferWrite tripleBufferWrite.c)
