#set(CMAKE_CXX_FLAGS "-fno-elide-constructors")
add_executable(test_buffer buffer_test.cpp ../src/Buffer.cpp)
target_link_libraries(test_buffer gtest gmock)
target_link_libraries(test_buffer boost_log-mt)

add_executable(test_string test_string.cpp)
target_link_libraries(test_string boost_log-mt)


