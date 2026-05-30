#include "IPlugin.hpp"
#include "ILogger.hpp"

#include <filesystem>
#include <string>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <dlfcn.h>
#endif

namespace workflow {

namespace {

/**
 * @brief Platform-agnostic wrapper to load a shared library.
 * @param path  Filesystem path to .dll/.so.
 * @return PluginHandle on success, with handle=nullptr on failure.
 */
PluginHandle load_library(const std::string& path) {
    PluginHandle h;
#ifdef _WIN32
    h.hmodule = static_cast<void*>(LoadLibraryA(path.c_str()));
#else
    h.handle = dlopen(path.c_str(), RTLD_NOW);
#endif
    return h;
}

/**
 * @brief Platform-agnostic wrapper to get a symbol from a dynamic library.
 * @tparam FuncSig  Function pointer type.
 * @param handle  Loaded library handle.
 * @param name    Symbol name (e.g., "wf_plugin_register").
 * @return Function pointer, or nullptr if not found.
 */
template <typename FuncSig>
FuncSig get_symbol(const PluginHandle& handle, const char* name) {
#ifdef _WIN32
    return reinterpret_cast<FuncSig>(
        GetProcAddress(static_cast<HMODULE>(handle.hmodule), name));
#else
    return reinterpret_cast<FuncSig>(dlsym(handle.handle, name));
#endif
}

/**
 * @brief Unload a shared library.
 */
void unload_library(PluginHandle& handle) {
#ifdef _WIN32
    if (handle.hmodule) {
        FreeLibrary(static_cast<HMODULE>(handle.hmodule));
        handle.hmodule = nullptr;
    }
#else
    if (handle.handle) {
        dlclose(handle.handle);
        handle.handle = nullptr;
    }
#endif
}

}

std::vector<LoadedPlugin> PluginLoader::load_from_directory(const std::string& plugin_dir) {
    std::vector<LoadedPlugin> result;

    namespace fs = std::filesystem;


#ifdef _WIN32
    const std::string extension = ".dll";
#else
    const std::string extension = ".so";
#endif


    for (const auto& entry : fs::directory_iterator(plugin_dir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension().string() != extension) continue;

        std::string full_path = entry.path().string();


        PluginHandle handle = load_library(full_path);
        if (handle.hmodule == nullptr
#ifndef _WIN32
            && handle.handle == nullptr
#endif
        ) {
            continue;
        }


        using NameFunc     = const char* (*)();
        using RegisterFunc = int (*)();

        auto name_fn     = get_symbol<NameFunc>(handle, "wf_plugin_name");
        auto register_fn = get_symbol<RegisterFunc>(handle, "wf_plugin_register");

        if (!name_fn || !register_fn) {

            unload_library(handle);
            continue;
        }


        int registered_count = register_fn();

        LoadedPlugin plugin;
        plugin.name   = name_fn();
        plugin.path   = full_path;
        plugin.handle = handle;

        loaded_.push_back(plugin);
        result.push_back(plugin);
    }

    return result;
}

void PluginLoader::unload(LoadedPlugin& plugin) {
    unload_library(plugin.handle);

    auto it = std::find_if(loaded_.begin(), loaded_.end(),
        [&plugin](const LoadedPlugin& p) { return p.path == plugin.path; });
    if (it != loaded_.end()) {
        loaded_.erase(it);
    }
}

void PluginLoader::unload_all() {
    for (auto& plugin : loaded_) {
        unload_library(plugin.handle);
    }
    loaded_.clear();
}

}