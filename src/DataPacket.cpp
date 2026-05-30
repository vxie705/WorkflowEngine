#include "DataPacket.hpp"
#include <nlohmann/json.hpp>

namespace workflow {

nlohmann::json DataPacket::to_json() const {
    nlohmann::json obj = nlohmann::json::object();

    for (const auto& [key, value] : data_) {

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

Result<DataPacket> DataPacket::from_json(const nlohmann::json& j) {
    if (!j.is_object()) {
        return Result<DataPacket>::error(
            "DataPacket::from_json: expected JSON object, received non-object");
    }

    DataPacket packet;

    for (auto it = j.begin(); it != j.end(); ++it) {
        const std::string& key = it.key();
        const nlohmann::json& val = it.value();

        switch (val.type()) {
        case nlohmann::json::value_t::boolean:
            packet.set(key, val.get<bool>());
            break;
        case nlohmann::json::value_t::number_integer:
        case nlohmann::json::value_t::number_unsigned:


            if (val.is_number_integer()) {
                auto n = val.get<std::int64_t>();
                if (n >= static_cast<std::int64_t>(std::numeric_limits<int>::min()) &&
                    n <= static_cast<std::int64_t>(std::numeric_limits<int>::max())) {
                    packet.set(key, static_cast<int>(n));
                } else {
                    packet.set(key, n);
                }
            } else {

                packet.set(key, val.get<std::uint64_t>());
            }
            break;
        case nlohmann::json::value_t::number_float:
            packet.set(key, val.get<double>());
            break;
        case nlohmann::json::value_t::string:
            packet.set(key, val.get<std::string>());
            break;
        case nlohmann::json::value_t::object:
        case nlohmann::json::value_t::array:

            packet.set(key, val);
            break;
        case nlohmann::json::value_t::null:
        case nlohmann::json::value_t::binary:
        case nlohmann::json::value_t::discarded:
        default:

            break;
        }
    }

    return Result<DataPacket>::ok(std::move(packet));
}

}