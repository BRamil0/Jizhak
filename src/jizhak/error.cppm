module;

#if !defined(USE_OF_STD_MODULE)
#include <compare>
#include <concepts>
#include <exception>
#include <string>
#include <string_view>
#include <type_traits>
#endif

export module jizhak.error;

#if defined(USE_OF_STD_MODULE)
import std;
#endif

export namespace jzh::concepts {
    template <typename TErrorID>
    concept is_supported_enum = std::is_enum_v<TErrorID> && requires {
        { TErrorID::OK } -> std::convertible_to<TErrorID>;
        { TErrorID::null } -> std::convertible_to<TErrorID>;
        { TErrorID::none } -> std::convertible_to<TErrorID>;
        { TErrorID::pass } -> std::convertible_to<TErrorID>;
    };
} // namespace jzh::concepts

template<typename T>
inline constexpr bool dependent_false = false;

export namespace jzh {
    enum struct JizhakErrorID {
        OK = 0,
        pass,
        null,
        none,
        empty,
        buffer_is_empty,
        incomplete_character,
        max_size_file,
        conversion_error,
        generic_error,
        function_is_empty,
        cannot_steal_task,
        worker_not_found,
        task_not_found,
        zero_transferred,
        timeout_expired,
        index_overrun,
        no_workers_available,
        identifiers_are_different,
        failed_start_in_stream,
        internal_error,
        shutting,
        thread_already_registered,
        the_established_thread_is_not_this_tpm,
        file_already_open,
        file_not_open,
    };

    template <typename T> requires concepts::is_supported_enum<T>
    constexpr std::string_view default_message_for([[maybe_unused]] T code_id) {
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

    template <typename TErrorID> requires concepts::is_supported_enum<TErrorID>
    struct Error : public std::exception {
        TErrorID id = TErrorID::none;
        std::string message_{default_message_for(id)};

        Error() = default;

        Error(const TErrorID new_id)
            : id(new_id), message_(default_message_for(new_id)) {}

        Error(const TErrorID new_id, std::string_view const& new_message)
            : id(new_id), message_(new_message) {}

        Error(const Error& other) = default;
        Error(Error&& other) noexcept = default;

        Error& operator=(const Error& other) = default;
        Error& operator=(Error&& other) noexcept = default;

        ~Error() override = default;

        bool operator==(const TErrorID& other_id) const {
            return id == other_id;
        }

        auto operator<=>(const Error& other) const {
            return static_cast<int>(this->id) <=> static_cast<int>(other.id);
        }

        explicit operator bool() const {
            return (id != TErrorID::OK) and (id != TErrorID::none) and (id != TErrorID::null) and (id != TErrorID::pass);
        }

        [[nodiscard]] const char* what() const noexcept override {
            return message_.c_str();
        }

        [[nodiscard]] std::string_view message() const noexcept {
            return message_;
        }
    };

    using JizhakError = Error<JizhakErrorID>;
} // namespace izh