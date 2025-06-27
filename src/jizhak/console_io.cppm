module;
#if defined(_WIN32)
#include <windows.h>
#undef max
#undef min
#else
#include <unistd.h>
#endif

export module jizhak.io.console;
import std;


template <typename T>
concept is_supported_string = requires(T t) {
    { std::basic_string_view(t) } -> std::same_as<std::basic_string_view<typename decltype(std::basic_string_view(t))::value_type>>;
    requires std::same_as<typename decltype(std::basic_string_view(t))::value_type, char> ||
             std::same_as<typename decltype(std::basic_string_view(t))::value_type, char8_t>;
};

export namespace jzh {
    class ConsoleIO {
    private:
        std::string sep = " ";
        std::string end = "\n";

        [[nodiscard]] static std::optional<std::error_code> write_to_console(std::string_view str) {
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

        [[nodiscard]] static std::string to_utf8(auto const& input) {
            auto sv = std::basic_string_view(input);
            using CharT = typename decltype(sv)::value_type;

            if constexpr (std::is_same_v<CharT, char>) {
                return std::string(sv);
            }
            else if constexpr (std::is_same_v<CharT, char8_t>) {
                return std::string(reinterpret_cast<const char*>(sv.data()), sv.size());
            }
            else {
                return "";
            }
        }

        [[nodiscard]] static auto convert_format_arg(auto const& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, const char8_t*>)
                return reinterpret_cast<const char*>(arg);
            else if constexpr (std::is_convertible_v<T, std::u8string_view>)
                return std::string_view(reinterpret_cast<const char*>(std::u8string_view(arg).data()), std::u8string_view(arg).size());
            else
                return arg;
        }

    public:
        ConsoleIO() = default;
        explicit ConsoleIO(std::string new_sep, std::string new_end)
            : sep(std::move(new_sep)), end(std::move(new_end)) {}

        ConsoleIO(const ConsoleIO&) = default;
        ConsoleIO(ConsoleIO&&) = default;
        ConsoleIO& operator=(const ConsoleIO&) = default;
        ConsoleIO& operator=(ConsoleIO&&) = default;

        ~ConsoleIO() = default;

        template <typename T>
        ConsoleIO& operator<<(T&& value) {
            std::string formatted_value = std::format("{}", this->convert_format_arg(std::forward<T>(value)));

            if (auto error = this->write_to_console(formatted_value); error.has_value()) {
                throw std::system_error(error.value());
            }
            return *this;
        }

        [[nodiscard]] std::string get_sep() const { return sep; }
        [[nodiscard]] std::string get_separator() const { return sep; }
        [[nodiscard]] std::string get_end() const { return end; }

        void set_sep(std::string new_sep) { sep = std::move(new_sep); }
        void set_separator(std::string new_sep) { sep = std::move(new_sep); }
        void set_end(std::string new_end) { end = std::move(new_end); }

        template <typename Fmt, typename... Args> requires(is_supported_string<Fmt>)
        void print(Fmt&& fmt, Args&&... args) {
            std::string fmt_as_utf8 = this->to_utf8(std::forward<Fmt>(fmt));

            if constexpr (sizeof...(args) == 0) {
                if (auto error = this->write_to_console(fmt_as_utf8); error.has_value())
                    throw std::system_error(error.value());
            }
            else {
                std::string formatted_string = std::vformat(
                    fmt_as_utf8,
                    std::make_format_args(this->convert_format_arg(args)...)
                );
                if (auto error = this->write_to_console(formatted_string); error.has_value())
                    throw std::system_error(error.value());
            }
        }

        template<typename Fmt, typename... Args> requires(is_supported_string<Fmt>)
        void println(Fmt&& fmt, Args&&... args) {
            this->print(std::forward<Fmt>(fmt), std::forward<Args>(args)...);
            this->print(this->end);
        }

        template<typename... Args>
        void print_all(Args&&... args) {
            bool first = true;
            auto print_one_with_space = [&](const auto& arg) {
                if (!first) {
                    if (auto error = this->write_to_console(this->sep); error.has_value())
                        throw std::system_error(error.value());
                }
                std::string formatted_arg = std::format("{}", this->convert_format_arg(arg));

                if (auto error = this->write_to_console(formatted_arg); error.has_value())
                    throw std::system_error(error.value());

                first = false;
            };
            (print_one_with_space(args), ...);
        }

        template<typename... Args>
        void println_all(Args&&... args) {
            this->print_all(std::forward<Args>(args)...);
            this->print(this->end);
        }
    };

    ConsoleIO cio = ConsoleIO();

    template<typename FormatString, typename... Args>
    void print(const FormatString& fmt, Args&&... args) {
        cio.print(std::basic_string_view(fmt), std::forward<Args>(args)...);
    }

    template<typename Fmt, typename... Args>
    void println(Fmt&& fmt, Args&&... args) {
        cio.println(std::forward<Fmt>(fmt), std::forward<Args>(args)...);
    }

    template<typename... Args>
    void print_all(Args&&... args) {
        cio.print_all(std::forward<Args>(args)...);
    }
    template<typename... Args>
    void println_all(Args&&... args) {
        cio.println_all(std::forward<Args>(args)...);
    }
} // namespace jzh