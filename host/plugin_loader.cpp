// plugin_loader.cpp
// All platform-specific code lives here, behind #ifdef _WIN32.

#include "plugin_loader.h"

#include <cstring>     // memcpy
#include <stdexcept>
#include <string>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <dlfcn.h>
#endif

namespace pls 
{

// --- LibraryHandle: opaque type wrapping the OS handle -------------------
//
// Defined in the .cpp so the header never sees HMODULE / void*. The
// destructor calls the right OS function. shared_ptr<LibraryHandle>
// gives us reference-counted lifetime.

struct LibraryHandle 
{
#if defined(_WIN32)
    HMODULE h;
#else
    void* h;
#endif

    ~LibraryHandle() noexcept {
        if (h == nullptr) return;
#if defined(_WIN32)
        ::FreeLibrary(h);
#else
        ::dlclose(h);
#endif
    }
};

// --- Platform helpers ----------------------------------------------------

namespace 
{
// Open a shared library. Returns the OS handle or throws.
#if defined(_WIN32)
HMODULE open_library(const std::string& path) {
    HMODULE h = ::LoadLibraryA(path.c_str());
    if (h == nullptr) {
        DWORD err = ::GetLastError();
        throw std::runtime_error(
            "LoadLibrary failed for '" + path +
            "' (Windows error " + std::to_string(err) + ")");
    }
    return h;
}
void* find_symbol(HMODULE h, const char* name) {
    FARPROC p = ::GetProcAddress(h, name);
    return reinterpret_cast<void*>(p);
}
const char* last_error_string() {
    static thread_local std::string msg;
    msg = "GetLastError=" + std::to_string(::GetLastError());
    return msg.c_str();
}
#else
void* open_library(const std::string& path) 
{
    // RTLD_NOW: resolve all symbols immediately. We'd rather fail at
    //   load time than crash later on first use of a missing symbol.
    // RTLD_LOCAL: don't make this library's symbols available for
    //   resolution by subsequently-loaded libraries. Prevents one
    //   plugin's symbols from satisfying another plugin's lookups.
    ::dlerror();  // clear any prior error
    void* h = ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (h == nullptr) 
    {
        const char* err = ::dlerror();
        throw std::runtime_error(
            "dlopen failed for '" + path + "': " +
            (err ? err : "unknown error"));
    }
    return h;
}

void* find_symbol(void* h, const char* name) 
{
    ::dlerror();
    void* sym = ::dlsym(h, name);
    // Note: a valid symbol can legitimately be nullptr, so dlsym
    // signals errors via dlerror, not the return value. For our
    // case (function pointers) null always means "not found".
    return sym;
}
const char* last_error_string() 
{
    const char* e = ::dlerror();
    return e ? e : "unknown error";
}
#endif

} // namespace

// --- PluginLoader implementation -----------------------------------------

PluginLoader::PluginLoader(const std::string& path)
    : lib_(std::make_shared<LibraryHandle>())
    , factory_(nullptr)
{
    // 1. Open the library.
    lib_->h = open_library(path);

    // 2. Look up create_plugin.
    void* raw = find_symbol(lib_->h, "create_plugin");
    if (raw == nullptr) {
        // ~LibraryHandle will run when lib_ goes out of scope (here,
        // via the throw). dlclose / FreeLibrary unmap the library.
        throw std::runtime_error(
            "Symbol 'create_plugin' not found in '" + path + "': " +
            last_error_string());
    }

    // 3. Convert void* to function pointer portably.
    // reinterpret_cast between object pointers and function pointers
    // is conditionally supported in C++ (works on POSIX/Windows but
    // is technically UB by the standard). memcpy avoids the issue
    // entirely and compiles to the same single move instruction.
    std::memcpy(&factory_, &raw, sizeof(factory_));
}

PluginLoader::~PluginLoader() = default;
// = default is correct here. shared_ptr<LibraryHandle> handles release.
// If this PluginLoader holds the LAST reference, ~LibraryHandle runs
// and unloads. If a PluginPtr deleter still holds a reference, the
// library stays mapped until that PluginPtr is destroyed.

PluginPtr PluginLoader::create() 
{
    if (factory_ == nullptr) 
    {
        // Shouldn't happen — constructor throws if factory missing.
        throw std::runtime_error("PluginLoader has no factory");
    }

    IPlugin* raw = factory_();
    if (raw == nullptr)
    {
        // Plugin's create_plugin returned null (e.g. caught bad_alloc).
        throw std::runtime_error("create_plugin returned nullptr");
    }

    // The deleter captures a shared_ptr<LibraryHandle> by VALUE. This
    // copy lives inside the unique_ptr's deleter slot. As long as the
    // unique_ptr is alive, this copy keeps the library mapped — even
    // if the PluginLoader that produced it has long since been destroyed.
    //
    // C++14 generalized lambda capture lets us move/copy the shared_ptr
    // into the lambda. The lambda is then converted to a function
    // pointer ONLY if it has no captures — but it does have one. So
    // we can't use the void(*)(IPlugin*) deleter type with a capturing
    // lambda directly.
    //
    // Two C++14-compatible options:
    //  (a) Use std::function<void(IPlugin*)> as the deleter type. Works
    //      but adds heap allocation and type erasure overhead per plugin.
    //  (b) Make a small struct deleter that holds the shared_ptr.
    //      Zero overhead, more typing. We use this.
    //
    // But our PluginPtr alias uses void(*)(IPlugin*) for simplicity in
    // the public API. So we need a different approach:
    // store the shared_ptr keepalive elsewhere. The cleanest: use a
    // custom deleter type. Let's switch the PluginPtr alias.
    //
    // (See header — we'll fix this in a moment below.)

    // For now, capture the lib_ in a deleter struct:
    struct Deleter 
    {
        std::shared_ptr<LibraryHandle> keepalive;
        void operator()(IPlugin* p) const noexcept 
        {
            // 1. delete p — virtual destructor dispatches into plugin code.
            //    Plugin's operator delete runs against plugin's heap.
            delete p;
            // 2. ~Deleter runs after this returns. keepalive's refcount
            //    drops; if it hits zero, ~LibraryHandle runs FreeLibrary.
            //    Order matters: plugin destruction MUST happen before
            //    library unload.
        }
    };

    // We can't build a unique_ptr<IPlugin, void(*)(IPlugin*)> from a
    // stateful deleter. The PluginPtr alias needs to change.
    (void)Deleter{};   // silence unused warning for the explanation
    (void)raw;
    throw std::logic_error("placeholder — see corrected version below");
}

} // namespace pls