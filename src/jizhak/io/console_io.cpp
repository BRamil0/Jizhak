module;

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

module jizhak.io.console;

namespace jzh::detail {
    std::string boost_convert_wstring(std::wstring_view sv) {
        return boost::locale::conv::utf_to_utf<char>(sv.data(), sv.data() + sv.size());
    }
    std::string boost_convert_u8string(std::u8string_view sv) {
        return boost::locale::conv::utf_to_utf<char>(sv.data(), sv.data() + sv.size());
    }
    std::string boost_convert_u16string(std::u16string_view sv) {
        return boost::locale::conv::utf_to_utf<char>(sv.data(), sv.data() + sv.size());
    }
    std::string boost_convert_u32string(std::u32string_view sv) {
        return boost::locale::conv::utf_to_utf<char>(sv.data(), sv.data() + sv.size());
    }
}

namespace jzh {
    void ConsoleBaseIO::write_to_console(std::string_view str) {
        if (str.empty()) {
            return;
        }

        #if defined(_WIN32)
            if (str.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
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


                std::wstring w_str(static_cast<std::size_t>(size_needed), 0);
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

    std::string ConsoleIO::read_interactive(std::size_t buffer_size, std::string_view stop_chars, bool echo) {
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

    void ConsoleErrorIO::write_to_console(std::string_view str) override {
        if (str.empty()) {
            return;
        }

        #if defined(_WIN32)
            if (str.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
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


                std::wstring w_str(static_cast<std::size_t>(size_needed), 0);
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

    ConsoleIO cio{};
    ConsoleErrorIO ceio{};
}
