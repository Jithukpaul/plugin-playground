# MODULE library: a shared library that's only ever loaded via dlopen /
# LoadLibrary, never linked against directly. CMake won't try to add it
# to anyone's link line. This is exactly what plugins are.
#
# Alternative: SHARED. Functionally similar on Linux but on Windows would
# generate an import .lib that we don't need. MODULE is the precise tool.
add_library(plugin_hello MODULE hello_plugin.cpp)

# Inherit include path + C++14 from the contract target.
target_link_libraries(plugin_hello PRIVATE plugin_api)

# Note: currently this feature is not implemented in the example
# # This flips PLUGIN_API to dllexport / visibility("default") inside the
# # plugin's TU. PRIVATE so it doesn't leak to consumers.
# target_compile_definitions(plugin_hello PRIVATE PLUGIN_BUILDING)

# Strip the "lib" prefix on Linux so we get plugin_hello.so, not
# libplugin_hello.so. Cosmetic, but it makes the discovery code
# in Step 4 simpler — no per-platform prefix handling needed.
set_target_properties(plugin_hello PROPERTIES PREFIX "")