module;

#if !defined(USE_OF_STD_MODULE)
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <system_error>
#include <concepts>
#include <limits>
#include <tuple>
#include <utility>
#include <cwchar>
#include <type_traits>
#include <charconv>
#include <stdexcept>
#include <conio.h>
#include <cerrno>
#include <optional>
#include <memory>
#include <algorithm>
#endif

#if defined(_WIN32)
#include <windows.h>
#undef max
#undef min
#else
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#endif

#include "boost/locale.hpp"
#include "fmt/format.h"

#if defined(USE_OF_STD_MODULE)
import std;
#endif

export module jizhak.io.console;

export namespace jzh::concepts {
    template <typename T>
    concept is_supported_string = requires(T t) {
        { std::basic_string_view(t) } -> std::same_as<std::basic_string_view<typename decltype(std::basic_string_view(t))::value_type>>;
        requires std::same_as<typename decltype(std::basic_string_view(t))::value_type, char> ||
                 std::same_as<typename decltype(std::basic_string_view(t))::value_type, wchar_t> ||
                 std::same_as<typename decltype(std::basic_string_view(t))::value_type, char8_t> ||
                 std::same_as<typename decltype(std::basic_string_view(t))::value_type, char16_t> ||
                 std::same_as<typename decltype(std::basic_string_view(t))::value_type, char32_t>;
    };
} // namespace jzh::concepts

export namespace jzh {
    class ConsoleBaseIO {
    private:
        std::string sep = " ";
        std::string end = "\n";

    protected:
        virtual void write_to_console(std::string_view str) {
            if (str.empty()) {
                return;
            }

            #if defined(_WIN32)
                if (str.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
                    throw std::length_error("String size is too large for Windows API");


                HANDLE h_std_err = GetStdHandle(STD_OUTPUT_HANDLE);
                if (h_std_err == INVALID_HANDLE_VALUE)
                    throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "Failed to get standard error handle");


                DWORD mode;
                if (GetConsoleMode(h_std_err, &mode)) {
                    const int str_size_as_int = static_cast<int>(str.size());
                    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), str_size_as_int, nullptr, 0);
                    if (size_needed == 0)
                        throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "MultiByteToWideChar failed to calculate size");


                    std::wstring w_str(static_cast<size_t>(size_needed), 0);
                    MultiByteToWideChar(CP_UTF8, 0, str.data(), str_size_as_int, &w_str[0], size_needed);

                    if (w_str.size() > std::numeric_limits<DWORD>::max())
                        throw std::length_error("String is too large for WriteConsoleW");


                    DWORD chars_written = 0;
                    if (!WriteConsoleW(h_std_err, w_str.c_str(), static_cast<DWORD>(w_str.size()), &chars_written, nullptr))
                         throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "Failed to write to console");

                } else {
                    if (str.size() > std::numeric_limits<DWORD>::max())
                        throw std::length_error("String is too large for WriteFile");

                    DWORD bytes_written = 0;
                    if (!WriteFile(h_std_err, str.data(), static_cast<DWORD>(str.size()), &bytes_written, nullptr) || bytes_written != str.size())
                        throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "Failed to write to redirected error stream");

                }

            #else // POSIX
                ssize_t result = ::write(STDOUT_FILENO, str.data(), str.size());
                if (result == -1)
                    throw std::system_error(errno, std::system_category(), "Failed to write to error stream");
            #endif
        }
        [[nodiscard]] static std::string to_utf8(auto const& input) {
            auto sv = std::basic_string_view(input);
            using CharT = typename decltype(sv)::value_type;

            if constexpr (std::is_same_v<CharT, char>) {
                return std::string(sv);
            } else if constexpr (std::is_same_v<CharT, wchar_t> ||
                               std::is_same_v<CharT, char8_t> ||
                               std::is_same_v<CharT, char16_t> ||
                               std::is_same_v<CharT, char32_t>) {
                return boost::locale::conv::utf_to_utf<char>(sv.data(), sv.data() + sv.size());
            } else if constexpr (requires { std::begin(input); std::end(input); }) {
                return std::string(std::begin(sv), std::end(sv));
            } else {
                static_assert(std::is_same_v<CharT, void>, "Unsupported character type for conversion to UTF-8.");
                return {};
            }
        }
        [[nodiscard]] static auto convert_format_arg(auto const& arg) {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_convertible_v<T, std::wstring_view> ||
                          std::is_convertible_v<T, std::u8string_view> ||
                          std::is_convertible_v<T, std::u16string_view> ||
                          std::is_convertible_v<T, std::u32string_view>) {
                return to_utf8(arg);
            } else {
                return arg;
            }
        }

    public:
        ConsoleBaseIO() = default;
        explicit ConsoleBaseIO(std::string new_sep, std::string new_end)
            : sep(std::move(new_sep)), end(std::move(new_end)) {}

        ConsoleBaseIO(const ConsoleBaseIO& other) = default;
        ConsoleBaseIO(ConsoleBaseIO&& other) = default;

        ConsoleBaseIO& operator=(const ConsoleBaseIO& other) = default;
        ConsoleBaseIO& operator=(ConsoleBaseIO&& other) = default;

        virtual ~ConsoleBaseIO() = default;


        template <typename T>
        ConsoleBaseIO& operator<<(T&& value) {
            std::string formatted_value = fmt::format("{}", this->convert_format_arg(std::forward<T>(value)));
            this->write_to_console(formatted_value);

            return *this;
        }
        ConsoleBaseIO& operator<<(const ConsoleBaseIO& other) {
            this->write_to_console(other.sep);
            return *this;
        }

        [[nodiscard]] std::string get_sep() const { return sep; }
        [[nodiscard]] std::string get_separator() const { return sep; }
        [[nodiscard]] std::string get_end() const { return end; }

        void set_sep(std::string &new_sep) { sep = std::move(new_sep); }
        void set_separator(std::string &new_sep) { sep = std::move(new_sep); }
        void set_end(std::string &new_end) { end = std::move(new_end); }

        template <typename Fmt, typename... Args> requires(concepts::is_supported_string<Fmt>)
        void print(Fmt&& fmt, Args&&... args) {
            std::string fmt_as_utf8 = this->to_utf8(std::forward<Fmt>(fmt));

            if constexpr (sizeof...(args) == 0) {
                this->write_to_console(fmt_as_utf8);
            } else {
                auto converted_args = std::make_tuple(this->convert_format_arg(std::forward<Args>(args))...);

                std::string formatted_string = fmt::vformat(
                    fmt::string_view(fmt_as_utf8.data(), fmt_as_utf8.size()),
                    std::apply([](auto&&... unpacked_args) {
                        return fmt::make_format_args(unpacked_args...);
                    }, converted_args)
                );
                this->write_to_console(formatted_string);
            }
        }

        template<typename Fmt, typename... Args> requires(concepts::is_supported_string<Fmt>)
        void println(Fmt&& fmt, Args&&... args) {
            this->print(std::forward<Fmt>(fmt), std::forward<Args>(args)...);
            this->print(this->end);
        }

        template<typename... Args>
        void print_all(Args&&... args) {
            bool first = true;
            auto print_one_with_space = [&first, this]<typename T>(T&& arg) {
                auto&& formatted_arg = this->convert_format_arg(std::forward<T>(arg));

                if (!first) {
                    this->write_to_console(this->sep);
                }

                std::string formatted = fmt::format("{}", formatted_arg);
                this->write_to_console(formatted);

                first = false;
            };

            (print_one_with_space(std::forward<Args>(args)), ...);
        }

        template<typename... Args>
        void println_all(Args&&... args) {
            this->print_all(std::forward<Args>(args)...);
            this->print(this->end);
        }
    };

    class ConsoleIO : public ConsoleBaseIO {
    protected:
        [[nodiscard]] virtual std::string read_interactive(size_t buffer_size, std::string_view stop_chars, bool echo) {
            #if defined(_WIN32)
                std::wstring result_w; // Збираємо результат у wstring для коректної роботи з Unicode
                if (buffer_size > 0) result_w.reserve(buffer_size);

                while (true) {
                    if (buffer_size > 0 && result_w.length() >= buffer_size) {
                        break;
                    }

                    wchar_t wch = _getwch();
                    if (wch == L'\r' || wch == L'\n') {
                        if (echo) {
                            write_to_console("\n");
                        }
                        break;
                    }
                    if (wch == L'\b') {
                        if (!result_w.empty()) {
                            result_w.pop_back();
                            if (echo) {
                                write_to_console("\b \b");
                            }
                        }
                        continue;
                    }
                    if (wch == 3) { // Ctrl+C
                        throw std::runtime_error("Input interrupted by user (Ctrl+C).");
                    }

                    if (!stop_chars.empty() && stop_chars.find(static_cast<char>(wch)) != std::string_view::npos) {
                        if (echo) {
                             write_to_console(to_utf8(std::wstring_view(&wch, 1)));
                        }
                        break;
                    }

                    result_w += wch;

                    if (echo) {
                        write_to_console(to_utf8(std::wstring_view(&wch, 1)));
                    }
                }
                return to_utf8(result_w);

            #else // POSIX
                class TerminalModeGuard {
                public:
                    TerminalModeGuard(bool enable_echo) {
                        tcgetattr(STDIN_FILENO, &oldt_);
                        newt_ = oldt_;
                        // Вимикаємо канонічний режим І системне відлуння
                        newt_.c_lflag &= ~(ICANON | ECHO);
                        tcsetattr(STDIN_FILENO, TCSANOW, &newt_);
                    }
                    ~TerminalModeGuard() { tcsetattr(STDIN_FILENO, TCSANOW, &oldt_); }
                private:
                    struct termios oldt_{}, newt_{};
                };

                TerminalModeGuard guard(echo);
                std::string result;
                if (buffer_size > 0) result.reserve(buffer_size);

                char ch;
                while (read(STDIN_FILENO, &ch, 1) > 0) {
                    if (buffer_size > 0 && result.length() >= buffer_size) break;

                    if (ch == '\n' || ch == '\r') {
                        if (echo) write_to_console("\n");
                        break;
                    }
                    if (ch == 127 || ch == 8) {
                        if (!result.empty()) {
                            result.pop_back();
                            if (echo) write_to_console("\b \b");
                        }
                        continue;
                    }
                    if (ch == 3) throw std::runtime_error("Input interrupted by user (Ctrl+C).");

                    if (!stop_chars.empty() && stop_chars.find(ch) != std::string_view::npos) {
                        if (echo) write_to_console({&ch, 1});
                        break;
                    }

                    result += ch;
                    if (echo) write_to_console({&ch, 1});
                }
                return result;
            #endif
        }

    public:
        template <typename T>
        ConsoleBaseIO& operator>>(T& value) {
            value = this->input<std::decay_t<T>>();
            return *this;
        }

        template <typename T = std::string> requires(concepts::is_supported_string<T> || std::is_integral_v<T> || std::is_floating_point_v<T>)
        T input(std::string_view prompt = "", std::string_view stop_chars = "\n", size_t buffer_size = 0, bool echo = true) {
            if (!prompt.empty()) {
                print(prompt);
            }

            std::string line = this->read_interactive(buffer_size, stop_chars, echo);

            if constexpr (std::is_same_v<T, std::string>)
                return boost::locale::conv::utf_to_utf<char>(line);
            else if constexpr (std::is_same_v<T, std::u8string>)
                return boost::locale::conv::utf_to_utf<char8_t>(line);
            else if constexpr (std::is_same_v<T, std::wstring>)
                return boost::locale::conv::utf_to_utf<wchar_t>(line);
            else if constexpr (std::is_same_v<T, std::u16string>)
                return boost::locale::conv::utf_to_utf<char16_t>(line);
            else if constexpr (std::is_same_v<T, std::u32string>)
                return boost::locale::conv::utf_to_utf<char32_t>(line);

            else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
                T value{};
                auto [ptr, ec] = std::from_chars(line.data(), line.data() + line.size(), value);
                if (ec == std::errc()) return value;
                else throw std::invalid_argument("Failed to convert input to the requested numeric type.");
            } else {
                static_assert(std::is_same_v<T, void>, "Unsupported type for input<T>().");
                return {};
            }
        }

        template <typename T = std::string> requires(concepts::is_supported_string<T> || std::is_integral_v<T> || std::is_floating_point_v<T>)
        T input_line(std::string_view prompt = "") {
            return this->input<T>(prompt, "\n", 0, true);
        }
    };

    class ConsoleErrorIO : public ConsoleBaseIO {
    private:
        void write_to_console(std::string_view str) override {
            if (str.empty()) {
                return;
            }

            #if defined(_WIN32)
                if (str.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
                    throw std::length_error("String size is too large for Windows API");


                HANDLE h_std_err = GetStdHandle(STD_ERROR_HANDLE);
                if (h_std_err == INVALID_HANDLE_VALUE)
                    throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "Failed to get standard error handle");


                DWORD mode;
                if (GetConsoleMode(h_std_err, &mode)) {
                    const int str_size_as_int = static_cast<int>(str.size());
                    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), str_size_as_int, nullptr, 0);
                    if (size_needed == 0)
                        throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "MultiByteToWideChar failed to calculate size");


                    std::wstring w_str(static_cast<size_t>(size_needed), 0);
                    MultiByteToWideChar(CP_UTF8, 0, str.data(), str_size_as_int, &w_str[0], size_needed);

                    if (str.size() > std::numeric_limits<DWORD>::max())
                        throw std::length_error("String is too large for WriteFile");

                    DWORD chars_written = 0;
                    if (!WriteConsoleW(h_std_err, w_str.c_str(), static_cast<DWORD>(w_str.size()), &chars_written, nullptr))
                         throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "Failed to write to console");

                } else {
                    if (str.size() > std::numeric_limits<DWORD>::max())
                        throw std::length_error("String is too large for WriteFile");

                    DWORD bytes_written = 0;
                    if (!WriteFile(h_std_err, str.data(), static_cast<DWORD>(str.size()), &bytes_written, nullptr) || bytes_written != str.size())
                        throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "Failed to write to redirected error stream");

                }

            #else // POSIX
                ssize_t result = ::write(STDERR_FILENO, str.data(), str.size());
                if (result == -1)
                    throw std::system_error(errno, std::system_category(), "Failed to write to error stream");
            #endif
        }
    };

    ConsoleIO cio = ConsoleIO();
    ConsoleErrorIO ceio = ConsoleErrorIO();

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

    template <typename T = std::string>
    T input(std::string_view prompt = "", std::string_view stop_chars = "\n", size_t buffer_size = 0, bool echo = true) {
        return cio.input<T>(prompt, stop_chars, buffer_size, echo);
    }
} // namespace jzh