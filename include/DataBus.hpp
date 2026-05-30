#ifndef WORKFLOW_ENGINE_DATA_BUS_HPP
#define WORKFLOW_ENGINE_DATA_BUS_HPP

#include "Result.hpp"

#include <any>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace workflow {

/**
 * @brief Shared communication channel between commands during pipeline
 * execution.
 *
 * DataBus enables PIPE & FILTER decoupling: commands never reference
 * each other directly. Instead, Command A publishes a value (e.g.,
 * "user_id"), and Command B consumes it. The bus is created per
 * pipeline execution and injected into every command.
 *
 * @note  DataBus is NOT persisted between executions. Each call to
 *        WorkflowEngine::execute() creates a fresh DataBus.
 *        For cross-pipeline persistence, use an external store.
 */
class DataBus {
public:
    DataBus() = default;
    ~DataBus() = default;


    DataBus(const DataBus&) = delete;
    DataBus& operator=(const DataBus&) = delete;
    DataBus(DataBus&&) noexcept = default;
    DataBus& operator=(DataBus&&) noexcept = default;

    /**
     * @brief Publish a value onto the bus.
     * Overwrites any existing value for the same key.
     */
    template <typename T>
    void publish(const std::string& key, T value) {
        shared_state_[key] = std::any(std::move(value));
    }

    /**
     * @brief Consume a typed value from the bus.
     * @return Result<T>::ok(value) if key exists and type matches.
     * @return Result<T>::error on missing key or type mismatch.
     */
    template <typename T>
    Result<T> consume(const std::string& key) const {
        auto it = shared_state_.find(key);
        if (it == shared_state_.end()) {
            return Result<T>::error(
                "DataBus::consume: key '" + key + "' not found");
        }
        try {
            return Result<T>::ok(std::any_cast<T>(it->second));
        } catch (const std::bad_any_cast&) {
            return Result<T>::error(
                "DataBus::consume: type mismatch for key '" + key +
                "', expected " + typeid(T).name() +
                " but stored " + it->second.type().name());
        }
    }

    /**
     * @brief Check whether a key exists on the bus.
     */
    bool has_key(const std::string& key) const noexcept {
        return shared_state_.find(key) != shared_state_.end();
    }

    /**
     * @brief Remove a key from the bus.
     */
    void remove(const std::string& key) {
        shared_state_.erase(key);
    }

    /**
     * @brief Clear all state from the bus.
     */
    void clear() noexcept {
        shared_state_.clear();
    }

    /**
     * @brief Serialize the current bus state to a JSON object.
     *
     * Supported types: int, double, bool, std::string, nlohmann::json.
     * Unsupported types serialize as "<unsupported-type>".
     */
    nlohmann::json to_json() const;

private:
    std::unordered_map<std::string, std::any> shared_state_;
};

}

#endif