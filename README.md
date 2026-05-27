# C++ Plugin Playground

A minimal, cross-platform C++ plugin system built with CMake. The project demonstrates how to load shared libraries at runtime using `dlopen`/`LoadLibrary`, call into a plugin through a pure-virtual interface, and manage library lifetime safely with RAII smart pointers.

## Architecture

```
include/
  plugin_api.h          # IPlugin interface + create_plugin factory declaration
host/
  plugin_loader.h/.cpp  # RAII wrapper around dlopen/LoadLibrary
  main.cpp              # Host executable entry point
plugins/
  hello/
    hello_plugin.cpp    # Example plugin: implements IPlugin, exports create_plugin
```

**Contract (`IPlugin`)** — a pure-virtual interface in `include/plugin_api.h`. Both the host and every plugin depend on this header; nothing else is shared.

**Plugin** — compiled as a `MODULE` shared library (`.so`/`.dll`). Exports a single C-linkage symbol `create_plugin()` that returns a heap-allocated `IPlugin*`.

**Host** — loads a plugin with `PluginLoader`, which calls `dlopen`/`LoadLibrary`, resolves `create_plugin`, and returns a `unique_ptr<IPlugin>` whose custom deleter keeps the library mapped for the lifetime of the plugin object.

## Requirements

- CMake 3.14+
- A C++14-capable compiler (GCC, Clang, or MSVC)

## Build

```bash
cmake -B build
cmake --build build
```

Output lands in `build/bin/`:
- `host` — the host executable
- `plugin_hello.so` (Linux) / `plugin_hello.dll` (Windows)

## Run

```bash
./build/bin/host
```

## Key design decisions

| Decision | Reason |
|---|---|
| `-fvisibility=hidden` globally | Only symbols explicitly exported cross the ABI boundary — matches Windows DLL behavior |
| `MODULE` vs `SHARED` for plugins | `MODULE` signals dlopen-only loading; avoids generating an import `.lib` on Windows |
| `RTLD_NOW | RTLD_LOCAL` | Fail fast on missing symbols at load time; prevent symbol collisions between plugins |
| `memcpy` for `void*` → fn-ptr cast | Avoids technically-UB `reinterpret_cast` between object and function pointers; compiles identically |
| `shared_ptr<LibraryHandle>` in deleter | Plugin `unique_ptr` holds a copy of the handle, so the library stays mapped even after `PluginLoader` is destroyed |

## Adding a plugin

1. Create `plugins/<name>/<name>_plugin.cpp` implementing `IPlugin` and exporting `create_plugin`.
2. Add a `CMakeLists.txt` next to it:
   ```cmake
   add_library(plugin_<name> MODULE <name>_plugin.cpp)
   target_link_libraries(plugin_<name> PRIVATE plugin_api)
   set_target_properties(plugin_<name> PROPERTIES PREFIX "")
   ```
3. Add `add_subdirectory(plugins/<name>)` to the root `CMakeLists.txt`.
