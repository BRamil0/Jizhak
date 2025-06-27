export module jizhak.io;

export import jizhak.io.console;


/*
namespace jzh::details {
    [[nodiscard]] std::string read_line_from_console() {
        #if defined(_WIN32)
            HANDLE h_std_in = GetStdHandle(STD_INPUT_HANDLE);
            if (h_std_in == INVALID_HANDLE_VALUE) {
                throw std::system_error(GetLastError(), std::system_category(), "GetStdHandle for stdin failed");
            }

            // Читаємо в UTF-16 буфер. Для простоти візьмемо фіксований розмір.
            // У реальному світі для дуже довгого вводу може знадобитися цикл та динамічний буфер.
            wchar_t buffer[512];
            DWORD chars_read = 0;

            if (!ReadConsoleW(h_std_in, buffer, sizeof(buffer)/sizeof(wchar_t), &chars_read, nullptr)) {
                throw std::system_error(GetLastError(), std::system_category(), "ReadConsoleW failed");
            }

            // ReadConsoleW включає \r\n, їх треба прибрати
            if (chars_read >= 2 && buffer[chars_read - 2] == L'\r' && buffer[chars_read - 1] == L'\n') {
                chars_read -= 2;
            } else if (chars_read >= 1 && buffer[chars_read - 1] == L'\n') {
                chars_read -= 1;
            }

            // Конвертуємо результат з UTF-16 (wchar_t) в UTF-8 (char/string)
            if (chars_read == 0) {
                return "";
            }

            int size_needed = WideCharToMultiByte(CP_UTF8, 0, buffer, chars_read, nullptr, 0, nullptr, nullptr);
            if (size_needed == 0) {
                throw std::system_error(GetLastError(), std::system_category(), "WideCharToMultiByte failed to get size");
            }

            std::string result(size_needed, 0);
            WideCharToMultiByte(CP_UTF8, 0, buffer, chars_read, &result[0], size_needed, nullptr, nullptr);

            return result;

        #else // POSIX
            char buffer[512];
            ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));

            if (bytes_read == -1) {
                throw std::system_error(errno, std::system_category(), "read from stdin failed");
            }

            // `read` включає \n, прибираємо його
            if (bytes_read > 0 && buffer[bytes_read - 1] == '\n') {
                bytes_read -= 1;
            }

            return std::string(buffer, bytes_read);
        #endif
    }
} // namespace jzh::details
*/