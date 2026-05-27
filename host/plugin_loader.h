// plugin_loader.h
// RAII wrapper around the OS dynamic loader. Loads a shared library,
// looks up the plugin factory, and returns a unique_ptr<IPlugin> that
// keeps the library mapped for as long as the plugin object lives.

#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H

#include "plugin_api.h"

#include <memory>
#include <string>

namespace pls {  // pls = plugin system

// Opaque handle to a loaded library. We hide the platform type behind
// a forward-declared struct so the header has no #ifdef _WIN32 leaks.
struct LibraryHandle;

// A loaded plugin = the IPlugin instance + a shared library handle that
// must outlive it. We expose this as a single unique_ptr by stuffing the
// handle into a custom deleter that gets destroyed AFTER the IPlugin*.
using PluginPtr = std::unique_ptr<IPlugin, void(*)(IPlugin*)>;

class PluginLoader {
public:
    // Loads the shared library at `path`. Throws std::runtime_error on
    // failure (file missing, symbol not found, etc.).
    explicit PluginLoader(const std::string& path);

    // Non-copyable: a library handle is unique. Moveable.
    PluginLoader(const PluginLoader&) = delete;
    PluginLoader& operator=(const PluginLoader&) = delete;
    PluginLoader(PluginLoader&&) noexcept = default;
    PluginLoader& operator=(PluginLoader&&) noexcept = default;

    ~PluginLoader();

    // Calls the plugin's create_plugin() factory. The returned smart
    // pointer keeps the library loaded for its entire lifetime — even
    // if this PluginLoader is destroyed first.
    PluginPtr create();

private:
    // shared_ptr because the deleter inside PluginPtr will hold a copy.
    // That second owner is what guarantees the library outlives the plugin.
    std::shared_ptr<LibraryHandle> lib_;
    create_plugin_fn factory_;
};

} // namespace pls

#endif // PLUGIN_LOADER_H