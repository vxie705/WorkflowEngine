#include "DataBus.hpp"
#include <nlohmann/json.hpp>

namespace workflow {

nlohmann::json DataBus::to_json() const {
    nlohmann::json obj = nlohmann::json::object();

    for (const auto& [key, value] : shared_state_) {
        const auto& type = value.type();

        if (type == typeid(int)) {
            obj[key] = std::any_cast<int>(value);
        } else if (type == typeid(double)) {
            obj[key] = std::any_cast<double>(value);
        } else if (type == typeid(float)) {
            obj[key] = static_cast<double>(std::any_cast<float>(value));
        } else if (type == typeid(bool)) {
            obj[key] = std::any_cast<bool>(value);
        } else if (type == typeid(std::string)) {
            obj[key] = std::any_cast<std::string>(value);
        } else if (type == typeid(const char*)) {
            obj[key] = std::string(std::any_cast<const char*>(value));
        } else if (type == typeid(nlohmann::json)) {
            obj[key] = std::any_cast<nlohmann::json>(value);
        } else if (type == typeid(long)) {
            obj[key] = static_cast<std::int64_t>(std::any_cast<long>(value));
        } else if (type == typeid(long long)) {
            obj[key] = static_cast<std::int64_t>(std::any_cast<long long>(value));
        } else if (type == typeid(unsigned int)) {
            obj[key] = static_cast<std::uint64_t>(std::any_cast<unsigned int>(value));
        } else if (type == typeid(unsigned long)) {
            obj[key] = static_cast<std::uint64_t>(std::any_cast<unsigned long>(value));
        } else if (type == typeid(unsigned long long)) {
            obj[key] = static_cast<std::uint64_t>(std::any_cast<unsigned long long>(value));
        } else if (type == typeid(std::int64_t)) {
            obj[key] = std::any_cast<std::int64_t>(value);
        } else if (type == typeid(std::uint64_t)) {
            obj[key] = std::any_cast<std::uint64_t>(value);
        } else {
            obj[key] = std::string("<unsupported-type>");
        }
    }

    return obj;
}

}