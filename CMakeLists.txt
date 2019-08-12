cmake_minimum_required(VERSION 3.10)
project(websocket-engine)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-g -O0 -DBOOST_LOG_DYN_LINK")

include_directories(./include)
include_directories(/usr/local/include)

#message(${CMAKE_CURRENT_SOURCE_DIR})
#set(HTTP_PARSER_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs")
#list(APPEND CMAKE_LIBRARY_PATH ${HTPP_PARSER_LIB_PATH})
#find_library(
#    HTTP_PARSER
#    http_parser.2.9.2
#        HINTS ${HTTP_PARSER_LIB_PATH}
#)

aux_source_directory(./src SOURCE)

# must before add_executable
link_directories(/usr/local/lib)

add_executable(test main.cpp ${SOURCE})

# tests
add_executable(test_buffer test/buffer_test.cpp src/Buffer.cpp)
target_link_libraries(test_buffer gtest gmock)

IF (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_link_libraries(test uv boost_log-mt)
    target_link_libraries(test_buffer boost_log-mt)
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_libraries(test uv boost_log pthread http_parser)
ENDIF ()

