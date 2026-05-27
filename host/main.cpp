// main.cpp
// Step 3 placeholder: this host doesn't load anything yet. We just want
// to verify the build graph compiles and links. Step 4 replaces this
// with real dlopen/LoadLibrary logic.

#include "plugin_api.h"

#include <iostream>

int main() {
    std::cout << "Host built successfully. Plugin loading comes in Step 4.\n";
    // Reference the typedef so the header is meaningfully used.
    create_plugin_fn fn = nullptr;
    (void)fn;
    
    return 0;
}