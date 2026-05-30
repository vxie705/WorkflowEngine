#ifndef WORKFLOW_ENGINE_RESULT_HPP
#define WORKFLOW_ENGINE_RESULT_HPP

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace workflow {

/**
 * @brief A Rust-like Result type for explicit error handling.
 *
 * Commands MUST return Result<DataPacket>. The engine inspects
 * is_ok() after each filter step. If is_error(), the pipeline
 * halts and propagates the error upward.
 *
 * @tparam T The success value type.
 */
template <typename T>
class Result {
public:


    /** Construct a success Result containing a value. */
    static Result<T> ok(T value) {
        Result<T> r;
        r.ok_ = true;
        r.value_ = std::move(value);
        return r;
    }

    /** Construct an error Result with a message and optional code. */
    static Result<T> error(std::string message, int code = -1) {
        Result<T> r;
        r.ok_ = false;
        r.error_message_ = std::move(message);
        r.error_code_ = code;
        return r;
    }



    bool is_ok() const noexcept { return ok_; }
    bool is_error() const noexcept { return !ok_; }

    /** Retrieve the error message (undefined if is_ok()). */
    const std::string& error_message() const noexcept { return error_message_; }

    /** Retrieve the error code (undefined if is_ok()). */
    int error_code() const noexcept { return error_code_; }



    /**
     * @brief Access the contained value.
     * @throws std::runtime_error if this Result holds an error.
     */
    T& value() {
        if (!ok_) {
            throw std::runtime_error(
                "Result::value() called on error: " + error_message_);
        }
        return value_.value();
    }

    const T& value() const {
        if (!ok_) {
            throw std::runtime_error(
                "Result::value() called on error: " + error_message_);
        }
        return value_.value();
    }

    /**
     * @brief Get the value or return a default.
     */
    T value_or(T default_value) const noexcept {
        return ok_ ? value_.value() : std::move(default_value);
    }

private:
    Result() = default;

    bool ok_ = false;
    std::optional<T> value_;
    std::string error_message_;
    int error_code_ = 0;
};

/**
 * @brief Specialisation for commands that produce no data on success.
 */
template <>
class Result<void> {
public:
    static Result<void> ok() {
        Result<void> r;
        r.ok_ = true;
        return r;
    }

    static Result<void> error(std::string message, int code = -1) {
        Result<void> r;
        r.ok_ = false;
        r.error_message_ = std::move(message);
        r.error_code_ = code;
        return r;
    }

    bool is_ok() const noexcept { return ok_; }
    bool is_error() const noexcept { return !ok_; }

    const std::string& error_message() const noexcept { return error_message_; }
    int error_code() const noexcept { return error_code_; }

private:
    Result<void>() = default;

    bool ok_ = false;
    std::string error_message_;
    int error_code_ = 0;
};

}

#endif