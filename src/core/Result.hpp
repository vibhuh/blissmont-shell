// core/Result.hpp — a tiny Qt-free Result<T> (value-or-error) for shell logic.
// Lives in core/ so it is testable with no Qt, no display. Errors are explicit and
// total; we do not use exceptions for expected failure paths (parse, validation).
#pragma once

#include <string>
#include <utility>
#include <variant>

namespace blissmont::core {

struct Error {
    std::string code;     // machine-readable, e.g. "money.parse"
    std::string message;  // human-readable detail
};

// Result<T> holds either a T or an Error. Construct via Ok()/Err() below.
template <typename T>
class Result {
public:
    static Result Ok(T value) { return Result(std::move(value)); }
    static Result Err(Error error) { return Result(std::move(error)); }
    static Result Err(std::string code, std::string message) {
        return Result(Error{std::move(code), std::move(message)});
    }

    [[nodiscard]] bool ok() const noexcept { return std::holds_alternative<T>(slot_); }
    explicit operator bool() const noexcept { return ok(); }

    // Precondition: ok(). The value accessors.
    [[nodiscard]] const T& value() const& { return std::get<T>(slot_); }
    [[nodiscard]] T& value() & { return std::get<T>(slot_); }
    [[nodiscard]] T&& value() && { return std::get<T>(std::move(slot_)); }

    // Precondition: !ok().
    [[nodiscard]] const Error& error() const { return std::get<Error>(slot_); }

    [[nodiscard]] T value_or(T fallback) const {
        return ok() ? std::get<T>(slot_) : std::move(fallback);
    }

private:
    explicit Result(T value) : slot_(std::move(value)) {}
    explicit Result(Error error) : slot_(std::move(error)) {}
    std::variant<T, Error> slot_;
};

}  // namespace blissmont::core
