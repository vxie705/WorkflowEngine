#ifndef WORKFLOW_ENGINE_COMMAND_REGISTRY_HPP
#define WORKFLOW_ENGINE_COMMAND_REGISTRY_HPP

#include "DataBus.hpp"
#include "DataPacket.hpp"
#include "ICommand.hpp"
#include "ILogger.hpp"
#include "Result.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace workflow {

/**
 * @brief Signature for a command factory function (same as WorkflowEngine).
 */
using CommandFactory = std::function<std::unique_ptr<ICommand>(
    const std::string &instance_name, const nlohmann::json &params)>;

/**
 * @brief Thread-safe global registry for self-registering ICommand factories.
 *
 * ## Design — Self-Registration Pattern
 *
 * Each plugin translation unit calls:
 *
 *   REGISTER_COMMAND("EchoCommand", [] { return
 * std::make_unique<EchoCommand>(); })
 *
 * via a static global variable whose constructor runs before main().
 * The WorkflowEngine reads this registry on demand, so **no manual
 * register_command calls are needed** in main.cpp.
 *
 * ## Thread Safety
 *
 * All access is guarded by std::mutex. Plugins may register from multiple
 * threads during static init (though usually single-threaded in practice).
 *
 * ## Aislamiento Total
 *
 * Commands MUST NOT access global variables at runtime. The registry only
 * stores factory LAMBDAS — zero global state leaks into command execution.
 * At execute() time, commands receive DataPacket, DataBus, and ILogger via
 * dependency injection exclusively.
 */
class CommandRegistry {
public:
  /** @brief Thread-safe singleton accessor (Meyer's Singleton). */
  static CommandRegistry &instance() {
    static CommandRegistry registry;
    return registry;
  }

  /**
   * @brief Register a factory for a command type.
   * @param type_name  The string key used in JSON config (e.g., "EchoCommand").
   * @param factory    The factory function.
   * @return true if newly registered, false if the type was already registered.
   */
  bool register_factory(const std::string &type_name, CommandFactory factory) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (factories_.find(type_name) != factories_.end()) {
      return false; // Already registered — no double-registration
    }
    factories_[type_name] = std::move(factory);
    return true;
  }

  /**
   * @brief Look up a factory by type name.
   * @return Pointer to the factory, or nullptr if not found.
   */
  const CommandFactory *find(const std::string &type_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = factories_.find(type_name);
    if (it != factories_.end()) {
      return &it->second;
    }
    return nullptr;
  }

  /**
   * @brief Export all registered factories to another map (for the engine).
   */
  void
  export_to(std::unordered_map<std::string, CommandFactory> &target) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &[name, factory] : factories_) {
      target[name] = factory;
    }
  }

  /**
   * @brief Number of registered command types.
   */
  size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return factories_.size();
  }

  /** @brief List all registered type names. */
  std::vector<std::string> type_names() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    names.reserve(factories_.size());
    for (const auto &[name, _] : factories_) {
      names.push_back(name);
    }
    return names;
  }

private:
  CommandRegistry() = default;
  mutable std::mutex mutex_;
  std::unordered_map<std::string, CommandFactory> factories_;
};

}

/**
 * @brief Macro to self-register a command factory into the global registry.
 *
 * Usage at file scope in a single .cpp file:
 *
 *   REGISTER_COMMAND(EchoCommand, "EchoCommand",
 *       [](const std::string& instance_name, const nlohmann::json& params) {
 *           return std::make_unique<EchoCommand>(instance_name, params);
 *       });
 *
 * @param type_id         A valid C++ identifier (e.g., EchoCommand)
 * @param type_name_str   The string key used in JSON config (e.g.,
 * "EchoCommand")
 * @param factory_lambda  Lambda returning std::unique_ptr<ICommand>
 *
 * The static variable assures the code runs before main().
 */
#define REGISTER_COMMAND(type_id, type_name_str, factory_lambda)               \
  namespace {                                                                  \
  static const bool __registered_##type_id##__ = []() -> bool {                \
    return ::workflow::CommandRegistry::instance().register_factory(           \
        type_name_str, (factory_lambda));                                      \
  }();                                                                         \
  }

#endif