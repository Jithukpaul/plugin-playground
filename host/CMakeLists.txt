# host/CMakeLists.txt
add_executable(host main.cpp)
target_link_libraries(host PRIVATE plugin_api)
