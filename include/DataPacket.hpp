#ifndef WORKFLOW_ENGINE_DATA_PACKET_HPP
#define WORKFLOW_ENGINE_DATA_PACKET_HPP

#include "Result.hpp"

#include <any>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace workflow {

/**
 * @brief Generic key-value data container flowing through the pipeline.
 *
 * DataPacket is the universal currency of the Workflow Engine.  Each
 * ICommand receives a DataPacket, operates on it (possibly reading
 * from and writing to a DataBus for cross-command communication),
 * and returns a Result<DataPacket>.
 *
 * Internally backed by std::unordered_map<std::string, std::any> for
 * maximum flexibility.  Type-safety is enforced at retrieval via
 * Result<T> — if the type doesn't match, an error is returned.
 */
class DataPacket {
public:
    DataPacket() = default;
    ~DataPacket() = default;


    DataPacket(DataPacket&&) noexcept = default;
    DataPacket& operator=(DataPacket&&) noexcept = default;


    DataPacket(const DataPacket&) = default;
    DataPacket& operator=(const DataPacket&) = default;



    /**
     * @brief Store a value under the given key.
     * Overwrites any existing entry for that key.
     */
    template <typename T>
    void set(const std::string& key, T value) {
        data_[key] = std::any(std::move(value));
    }



    /**
     * @brief Retrieve a typed value.
     * @return Result<T>::ok(value) when key exists and type matches.
     * @return Result<T>::error when key is missing or type mismatches.
     */
    template <typename T>
    Result<T> get(const std::string& key) const {
        auto it = data_.find(key);
        if (it == data_.end()) {
            return Result<T>::error(
                "DataPacket::get: key '" + key + "' not found");
        }
        try {
            return Result<T>::ok(std::any_cast<T>(it->second));
        } catch (const std::bad_any_cast&) {
            return Result<T>::error(
                "DataPacket::get: type mismatch for key '" + key +
                "', expected " + typeid(T).name() +
                " but stored " + it->second.type().name());
        }
    }

    /**
     * @brief Check whether a key exists.
     */
    bool has(const std::string& key) const noexcept {
        return data_.find(key) != data_.end();
    }

    /**
     * @brief Return all stored keys (order is unspecified).
     */
    std::vector<std::string> keys() const {
        std::vector<std::string> result;
        result.reserve(data_.size());
        for (const auto& [key, _] : data_) {
            result.push_back(key);
        }
        return result;
    }

    /**
     * @brief Remove an entry by key.
     */
    void remove(const std::string& key) {
        data_.erase(key);
    }

    /**
     * @brief Number of entries.
     */
    size_t size() const noexcept {
        return data_.size();
    }

    /**
     * @brief Remove all entries.
     */
    void clear() noexcept {
        data_.clear();
    }

    /**
     * @brief Merge another DataPacket into this one.
     * Keys from `other` overwrite existing keys in `this`.
     */
    void merge(const DataPacket& other) {
        for (const auto& [key, value] : other.data_) {
            data_[key] = value;
        }
    }




    /**
     * @brief Serialize this DataPacket to a JSON object.
     *
     * Supported value types: int, double, bool, std::string,
     * and nlohmann::json itself (for nested structures).
     * Unsupported types are serialized as the string "<unsupported-type>".
     */
    nlohmann::json to_json() const;

    /**
     * @brief Deserialize a DataPacket from a JSON object.
     *
     * JSON numbers are stored as int (integral) or double (floating).
     * JSON booleans → bool, JSON strings → std::string.
     * Nested JSON objects/arrays are NOT unwrapped — they are stored
     * as nlohmann::json values directly.
     *
     * @return Result<DataPacket>::ok on success.
     * @return Result<DataPacket>::error if `j` is not a JSON object.
     */
    static Result<DataPacket> from_json(const nlohmann::json& j);

private:
    std::unordered_map<std::string, std::any> data_;
};

}

#endif