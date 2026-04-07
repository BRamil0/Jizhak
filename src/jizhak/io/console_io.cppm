/// Файл для роботи з консольним вводом/виводом.
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
#include <conio.h>
#include <cerrno>
#include <optional>
#include <memory>
#include <algorithm>
#endif

#include "boost/locale.hpp"

/// Модуль для роботи з консольним вводом/виводом.
export module jizhak.io.console;

#if defined(USE_OF_STD_MODULE)
import std;
#endif


namespace jzh::detail {
    std::string boost_convert_wstring(std::wstring_view sv);
    std::string boost_convert_u8string(std::u8string_view sv);
    std::string boost_convert_u16string(std::u16string_view sv);
    std::string boost_convert_u32string(std::u32string_view sv);

    void internal_vprint(std::string_view fmt_str, std::span<std::string> args, bool newline);
}

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
    /**
     * @brief Базовий клас для роботи з консоллю.
     */
    class ConsoleBaseIO {
    private:
        /// Роздільник між елементами для print_all() та println_all().
        std::string sep = " ";

        /// Закінчення яке додає println() та println_all().
        std::string end = "\n";

    protected:
        /**
         * @brief Низькорівнева функція запису рядків у консоль.
         * @param str Рядок для запису, у форматі UTF-8.
         */
        virtual void write_to_console(std::string_view str);

        /**
         * @brief Перекодовує будь-який Unicode рядок у формат UTF-8 за допомогою boost.locale.
         * @param input Рядок у форматі UTF-8, UTF-16 чи UTF-32.
         * @return std::string у форматі UTF-8.
         */
        [[nodiscard]] static std::string to_utf8(auto const& input) {
            auto sv = std::basic_string_view(input);
            using CharT = typename decltype(sv)::value_type;

            if constexpr (std::is_same_v<CharT, char>) {
                return std::string(sv);
            } else if constexpr (std::is_same_v<CharT, wchar_t>) {
                return detail::boost_convert_wstring(sv);
            } else if constexpr (std::is_same_v<CharT, char8_t>) {
                return detail::boost_convert_u8string(sv);
            } else if constexpr (std::is_same_v<CharT, char16_t>) {
                return detail::boost_convert_u16string(sv);
            } else if constexpr (std::is_same_v<CharT, char32_t>) {
                return detail::boost_convert_u32string(sv);
            } else {
                return {};
            }
        }

        /**
         * @brief Якщо аргумент є рядком у будь-якому Unicode-форматі, перекодовує його в UTF-8.
         * @param arg Аргумент будь-якого типу.
         * @return Аргумент в UTF-8 (якщо це рядок) або без змін.
         */
        [[nodiscard]] static auto convert_format_arg(auto const& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_convertible_v<T, std::wstring_view> ||
                          std::is_convertible_v<T, std::u8string_view>) {
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
            std::string formatted_value = std::format("{}", this->convert_format_arg(std::forward<T>(value)));
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

        /**
         * @brief Друкує форматований рядок в консоль.
         * @tparam Fmt Тип форматного рядка (підтримувані символи).
         * @tparam Args Типи аргументів для форматування.
         * @param fmt Форматний рядок.
         * @param args Аргументи для форматування; якщо це рядки в Unicode, вони перекодовуються в UTF-8. Довільна кількість аргументів.
        */
        template <typename Fmt, typename... Args> requires(concepts::is_supported_string<Fmt>)
        void print(Fmt&& fmt, Args&&... args) {
            std::string fmt_u8 = to_utf8(std::forward<Fmt>(fmt));
            if constexpr (sizeof...(args) == 0) {
                this->write_to_console(fmt_u8);
            } else {
                std::vector<std::string> converted = { (std::to_string(convert_format_arg(args)))... };
                this->write_to_console(std::vformat(fmt_u8, std::make_format_args(convert_format_arg(args)...)));
            }
        }

        /**
         * @brief Друкує форматований рядок в консоль та додає закінчення (@c end).
         * @tparam Fmt Тип форматного рядка (підтримувані символи).
         * @tparam Args Типи аргументів для форматування.
         * @param fmt Форматний рядок.
         * @param args Аргументи для форматування; якщо це рядки в Unicode, вони перекодовуються в UTF-8. Довільна кількість аргументів.
        */
        template<typename Fmt, typename... Args> requires(concepts::is_supported_string<Fmt>)
        void println(Fmt&& fmt, Args&&... args) {
            this->print(std::forward<Fmt>(fmt), std::forward<Args>(args)...);
            this->print(this->end);
        }

        /**
         * @brief Друкує будь-яку кількість елементів додавши роздільник (@c sep) між елементами.
         * @tparam Args Типи аргументів, сумісні з std::format.
         * @param args Будь-яка кількість елементів.
         * @note Якщо аргумент є рядком у Unicode, він автоматично перекодовується у UTF-8.
         */
        template<typename... Args>
        void print_all(Args&&... args) {
            bool first = true;
            auto print_one_with_space = [&first, this]<typename T>(T&& arg) {
                auto&& formatted_arg = this->convert_format_arg(std::forward<T>(arg));

                if (!first) {
                    this->write_to_console(this->sep);
                }

                std::string formatted = std::format("{}", formatted_arg);
                this->write_to_console(formatted);

                first = false;
            };

            (print_one_with_space(std::forward<Args>(args)), ...);
        }

        /**
         * @brief Друкує будь-яку кількість елементів додавши роздільник (@c sep) між елементами, після останнього елементу додає закінчення (@c end).
         * @tparam Args Типи аргументів, сумісні з std::format.
         * @param args Будь-яка кількість елементів.
         * @note Якщо аргумент є рядком у Unicode, він автоматично перекодовується у UTF-8.
        */
        template<typename... Args>
        void println_all(Args&&... args) {
            this->print_all(std::forward<Args>(args)...);
            this->print(this->end);
        }
    };

    /**
     * @brief Основний клас для роботи з консоллю.
     */
    class ConsoleIO : public ConsoleBaseIO {
    protected:
        /**
         * @brief Читання рядків з консолі.
         * @param buffer_size Розмір буфера.
         * @param stop_chars Набір стоп символів.
         * @param echo Якщо true — введені символи одразу відображаються у консолі.
         * @return Рядок у форматі UTF-8 (без символу, що перервав читання).
         *
         */
        [[nodiscard]] virtual std::string read_interactive(std::size_t buffer_size, std::string_view stop_chars, bool echo);

    public:
        template <typename T>
        ConsoleBaseIO& operator>>(T& value) {
            value = this->input<std::decay_t<T>>();
            return *this;
        }

        /**
         * @brief Функція для читання рядків з консолі.
         * @tparam T Тип рядка повернення.
         * @param prompt Рядок що буде надруковано.
         * @param stop_chars Набір стоп символів.
         * @param buffer_size Розмір буфера.
         * @param echo Якщо true — введені символи одразу відображаються у консолі.
         * @return Рядок у потрібному форматі.
         */
        template <typename T = std::string> requires(concepts::is_supported_string<T> || std::is_integral_v<T> || std::is_floating_point_v<T>)
        T input(std::string_view prompt = "", std::string_view stop_chars = "\n", std::size_t buffer_size = 0, bool echo = true) {
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

        /**
         * @brief Читання лінії (до \n) з консолі.
         * @tparam T Тип рядка повернення.
         * @param prompt Рядок що буде надруковано.
         * @return Рядок у потрібному форматі.
         */
        template <typename T = std::string> requires(concepts::is_supported_string<T> || std::is_integral_v<T> || std::is_floating_point_v<T>)
        T input_line(std::string_view prompt = "") {
            return this->input<T>(prompt, "\n", 0, true);
        }
    };

    /**
     * @brief Спеціальний клас для друкування у системний потік помилок.
     */
    class ConsoleErrorIO : public ConsoleBaseIO {
    private:
        /**
         * @brief Низькорівнева функція запису рядків у системний потік помилок.
         * @param str Рядок для запису, у форматі UTF-8.
         */
        void write_to_console(std::string_view str) override;
    };


	/// Глобальний об'єкт ConsoleIO()
    extern ConsoleIO cio;

    /// Глобальний об'єкт ConsoleErrorIO()
    extern ConsoleErrorIO ceio;


    /**
     * @name Функції-обгортки над cio
     * @note Спрощений доступ до функціоналу ConsoleIO без явного виклику cio.
     * @{
     */
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
    T input(std::string_view prompt = "", std::string_view stop_chars = "\n", std::size_t buffer_size = 0, bool echo = true) {
        return cio.input<T>(prompt, stop_chars, buffer_size, echo);
    }
    /** @} */
} // namespace jzh