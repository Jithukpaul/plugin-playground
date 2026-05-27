#ifndef PLUGIN_API_H
#define PLUGIN_API_H

class IPlugin 
{
public:
    virtual ~IPlugin() = default;
    virtual const char* name() const noexcept = 0;
    virtual int execute(const char* input) noexcept = 0;
};

// --- Factory function ----------------------------------------------------
// Every plugin DLL/SO must export exactly this symbol with C linkage.
// The host looks it up via dlsym / GetProcAddress.

extern "C" IPlugin* create_plugin();


// Convenience: a typedef for the function pointer the host will cast to.
typedef IPlugin* (*create_plugin_fn)();

#endif // PLUGIN_API_H