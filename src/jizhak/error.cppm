export module jizhak.error;

import std;

template <typename T>
concept is_supported_enum = std::is_enum_v<T> && requires {{ T::OK } -> std::convertible_to<T>;};

template<typename T>
inline constexpr bool dependent_false = false;

export namespace jzh {
    enum struct JizhakErrorID {
        OK = 0,
        buffer_is_empty,
        incomplete_character,
        max_size_file,
    };

    template <typename T> requires is_supported_enum<T>
    constexpr std::string_view default_message_for(T code_id) {
        static_assert(dependent_false<T>, "You forgot to overload the function");
        return {};
    };

    template <>
    constexpr std::string_view default_message_for(JizhakErrorID code_id) {
        switch (code_id) {
        case JizhakErrorID::OK:
            return "OK";
        default:
            return "Unknown error";
        }
    }

    template <typename T> requires is_supported_enum<T>
    struct Error {
        T id = T::OK;
        std::string message{default_message_for(id)};

        Error() = default;

        Error(const T new_id)
            : id(new_id), message(default_message_for(new_id)) {}

        Error(const T new_id, std::string_view const& new_message)
            : id(new_id), message(new_message) {}

        bool operator==(const T& other_id) const {
            return id == other_id;
        }

        auto operator<=>(const Error& other) const {
            return static_cast<int>(this->id) <=> static_cast<int>(other.id);
        }

        explicit operator bool() const {
            return id != T::OK;
        }

        [[nodiscard]] std::string_view what() const {
            return message;
        }
    };

    using JizhakError = Error<JizhakErrorID>;
} // namespace izh