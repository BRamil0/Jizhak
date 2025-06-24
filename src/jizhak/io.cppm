module;
#if defined(_WIN32)
#include <windows.h>
#undef max
#undef min
#else
#include <unistd.h>
#endif

export module jizhak.io;

import std;
import jizhak.platform_info;

template<typename T>
concept is_char_based = std::is_same_v<T, char> || std::is_same_v<T, char8_t>;

namespace jzh::details {
    [[nodiscard]] std::optional<std::error_code> write_to_console(std::string_view str) {
        #if defined(_WIN32)
            if (str.empty())
                return std::nullopt;

            if (str.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
                return std::make_error_code(std::errc::message_size);

            const int str_size_as_int = static_cast<int>(str.size());

            int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), str_size_as_int, nullptr, 0);
            if (size_needed == 0)
                throw std::system_error(GetLastError(), std::system_category(), "Failed to get standard output handle");

            std::wstring w_str(size_needed, 0);

            MultiByteToWideChar(CP_UTF8, 0, str.data(), str_size_as_int, &w_str[0], size_needed);

            HANDLE h_std_out = GetStdHandle(STD_OUTPUT_HANDLE);
            const DWORD w_str_size_as_dword = static_cast<DWORD>(w_str.size());
            WriteConsoleW(h_std_out, w_str.c_str(), w_str_size_as_dword, nullptr, nullptr);
        #else // POSIX
            if (str.empty()) {
                return std::nullopt;
            }
            if (write(STDOUT_FILENO, str.data(), str.size()) == -1) {
                // Системна помилка на POSIX
                throw std::system_error(errno, std::system_category(), "Failed to write to console");
            }
        #endif
        return std::nullopt;
    }
} // namespace jzh::details

export namespace jzh {
    template <typename... Args>
    void print(std::format_string<Args...> fmt, Args&&... args) {
        auto result = details::write_to_console(std::format(fmt, std::forward<Args>(args)...));
        if (result.has_value()) {
            throw std::system_error(result.value());
        }
    }

    template <typename... Args>
    void println(std::format_string<Args...> fmt, Args&&... args) {
        std::string formatted_str = std::format(fmt, std::forward<Args>(args)...);

        auto result1 = details::write_to_console(formatted_str);
        if (result1.has_value())
            throw std::system_error(result1.value());

        auto result2 = details::write_to_console("\n");
        if (result2.has_value())
            throw std::system_error(result2.value());
    }

    inline void print(std::string_view sv) {
        auto result = details::write_to_console(sv);
        if (result.has_value())
            throw std::system_error(result.value());
    }

    inline void println(std::string_view sv) {
        auto result1 = details::write_to_console(sv);
        if (result1.has_value())
            throw std::system_error(result1.value());

        auto result2 = details::write_to_console("\n");
        if (result2.has_value())
            throw std::system_error(result2.value());
    }

    template <typename... Args>
    void print(const std::u8string_view fmt_u8, Args&&... args) {
        std::string_view fmt_sv(reinterpret_cast<const char*>(fmt_u8.data()), fmt_u8.size());
        std::string fmt_str(fmt_sv);

        auto result = details::write_to_console(std::vformat(fmt_str, std::make_format_args(args...)));
        if (result.has_value()) {
            throw std::system_error(result.value());
        }
    }

    template <typename... Args>
    void println(const std::u8string_view fmt_u8, Args&&... args) {
        std::string_view fmt_sv(reinterpret_cast<const char*>(fmt_u8.data()), fmt_u8.size());
        std::string fmt_str(fmt_sv);

        std::string formatted_str = std::vformat(fmt_str, std::make_format_args(args...));

        auto result1 = details::write_to_console(formatted_str);
        if (result1.has_value())
            throw std::system_error(result1.value());

        auto result2 = details::write_to_console("\n");
        if (result2.has_value())
            throw std::system_error(result2.value());
    }

    inline void print(std::u8string_view u8sv) {
        std::string_view sv(reinterpret_cast<const char*>(u8sv.data()), u8sv.size());

        auto result = details::write_to_console(sv);
        if (result.has_value())
            throw std::system_error(result.value());
    }

    inline void println(std::u8string_view u8sv) {
        std::string_view sv(reinterpret_cast<const char*>(u8sv.data()), u8sv.size());

        auto result1 = details::write_to_console(sv);
        if (result1.has_value())
            throw std::system_error(result1.value());

        auto result2 = details::write_to_console("\n");
        if (result2.has_value())
            throw std::system_error(result2.value());
    }
} // namespace jzh