// hello_plugin.cpp
// A minimal IPlugin implementation. This file is compiled into a shared
// library (.so on Linux, .dll on Windows). The host loads it at runtime.
//
// PLUGIN_BUILDING is defined by the plugin's CMake target, which flips
// PLUGIN_API to dllexport / visibility("default") so create_plugin
// becomes a visible exported symbol.

#include "plugin_api.h"

#include <iostream>

namespace {

// Anonymous namespace = internal linkage for these symbols.
// Combined with -fvisibility=hidden, this class is completely invisible
// outside this translation unit. Only create_plugin() crosses the boundary.

class HelloPlugin : public IPlugin {
public:
    const char* name() const noexcept override {
        return "hello";
    }

    int execute(const char* input) noexcept override {
        if (input == nullptr) {
            std::cerr << "[hello plugin] error: null input\n";
            return 1;
        }
        std::cout << "[hello plugin] hello, " << input << "!\n";
        return 0;
    }
};

} // namespace

// The single exported symbol. extern "C" to avoid name mangling so that
// dlsym("create_plugin") / GetProcAddress(..., "create_plugin") finds it.
// Returns a raw pointer; the host wraps it in unique_ptr in Step 5.

extern "C" IPlugin* create_plugin() 
{
    try {
        return new HelloPlugin();
    } catch (...) {
        return nullptr;
    }
}