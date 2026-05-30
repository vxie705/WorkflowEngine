#ifndef WORKFLOW_ENGINE_IPLUGIN_HPP
#define WORKFLOW_ENGINE_IPLUGIN_HPP

#include "CommandRegistry.hpp"

#include <string>
#include <vector>
#include <nlohmann/json.hpp>































#ifndef WORKFLOW_PLUGIN_API
  #ifdef _WIN32
    #define WORKFLOW_PLUGIN_API extern "C" __declspec(dllexport)
  #else
    #define WORKFLOW_PLUGIN_API extern "C" __attribute__((visibility("default")))
  #endif
#endif



namespace workflow {

/**
 * @brief Opaque handle to a dynamically loaded shared library.
 *
 * Platform-specific: HMODULE on Windows, void* (from dlopen) on Linux.
 * Wrapped in a struct to avoid leaking platform types into the public API.
 */
struct PluginHandle {
#ifdef _WIN32
    void* hmodule = nullptr;
#else
    void* handle = nullptr;
#endif
};

/**
 * @brief Represents a loaded plugin and its exported symbols.
 */
struct LoadedPlugin {
    std::string name;
    std::string path;
    PluginHandle handle;
};

/**
 * @brief Discovers, loads, and unloads dynamic plugin libraries.
 *
 * ## Design
 * PluginLoader scans a directory for shared libraries (.dll/.so),
 * loads each one, and calls `wf_plugin_register()` so the commands
 * self-register into the global CommandRegistry.
 *
 * After loading, the engine reads the CommandRegistry to obtain all
 * available factories.
 */
class PluginLoader {
public:
    /**
     * @brief Discover and load all plugins in a directory.
     * @param plugin_dir  Path to the directory containing .dll/.so files.
     * @return List of successfully loaded plugins.
     */
    std::vector<LoadedPlugin> load_from_directory(const std::string& plugin_dir);

    /**
     * @brief Unload a specific plugin.
     */
    void unload(LoadedPlugin& plugin);

    /**
     * @brief Unload all loaded plugins.
     */
    void unload_all();

private:
    std::vector<LoadedPlugin> loaded_;
};

}

#endif