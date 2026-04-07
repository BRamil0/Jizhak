module;

#if !defined(USE_OF_STD_MODULE)
#include <filesystem>
#include <stdexcept>
#include <system_error>
#include <string>
#include <optional>
#include <vector>
#include <cstddef>
#include <utility>
#include <span>
#include <cerrno>
#endif

/// Модуль для роботи з файлами з використанням байтів.
export module jizhak.io.file;
export import jizhak.error;

#if defined(USE_OF_STD_MODULE)
import std;
#endif

export namespace jzh {
    /// Основний клас
    class FileIO final {
    public:
        /// Режим відкриття файлу.
        enum struct FileMode {
            read,
            write,
            append,
            read_write
        };
        /// Вибір точки відліку для функції seek().
        enum struct OffsetMode {
            begin,
            current,
            end
        };

        using fm = FileMode;
        using om = OffsetMode;

    private:
        /// Змінна що зберігає файл.
        struct System;
        std::unique_ptr<System> system_;

        std::filesystem::path file_path_ = std::filesystem::path();
        FileMode mode_ = FileMode::read_write;

    protected:
        /**
         * Читає байти до буфера з файлу.
         * @param buffer Посилання на буфер.
         * @param size Розмір буфера.
         * @note Це низькорівнева функція та призначення тільки для внутрішнього використання.
         */
        void write_to_file(const void* buffer, std::size_t size) const;

        /**
         * Записує байти з буфера до файлу
         * @param buffer Посилання на буфер.
         * @param size Розмір буфера.
         * @return Скільки було записано.
         * @note Це низькорівнева функція та призначення тільки для внутрішнього використання.
         */
        std::size_t read_from_file(void* buffer, std::size_t size) const;

    public:
        FileIO();

        explicit FileIO(const std::filesystem::path &file_path, const FileMode mode = FileMode::read_write);

        FileIO(const FileIO&) = delete;
        FileIO& operator=(const FileIO&) = delete;

        FileIO(FileIO&& other) noexcept;

        ~FileIO() noexcept;

        FileIO& operator=(FileIO&& other) noexcept;

        [[nodiscard]] FileMode get_mode() const noexcept {
            return mode_;
        }
        [[nodiscard]] std::filesystem::path get_file_path() const noexcept {
            return file_path_;
        }

        [[nodiscard]] bool is_readable() const noexcept {
            return mode_ == FileMode::read || mode_ == FileMode::read_write;
        }

        [[nodiscard]] bool is_writable() const noexcept {
            return mode_ == FileMode::write || mode_ == FileMode::append || mode_ == FileMode::read_write;
        }

        /**
         * Відкриває файл, якщо файл вже було відкрито, то повертає JizhakErrorID::file_already_open.
         * @param file_path Шлях до файлу.
         * @param mode Режим відкриття, за замовченням FileMode::read_write.
         * @return Опціонально повертає помилку з JizhakError.
         */
        std::optional<JizhakError> open(const std::filesystem::path &file_path, const FileMode mode = FileMode::read_write);

        std::optional<JizhakError> close() noexcept;

        /**
         * Записує один байт у файл.
         * @param data Байт.
         */
        void write(const std::byte& data) {
            write_to_file(&data, 1);
        }

        /**
         * Записує багато байтів.
         * @param data Масив байтів.
         */
        void write(std::span<const std::byte> const data) {
            write_to_file(data.data(), data.size());
        }

        /**
         * Читає один байт та змішує вказівник.
         * @return Повертає байт.
         */
        [[nodiscard]] std::byte read() { // NOLINT(readability-convert-member-functions-to-static)
            std::byte buffer{};
            if (const std::size_t bytes_read = read_from_file(&buffer, 1); bytes_read == 0) throw std::runtime_error("End of file reached");
            return buffer;
        }

        /**
         * Читає скільки скаже bytes_to_read.
         * @param bytes_to_read Кількість байтів що треба прочитати.
         * @return Повертає вектор байтів.
         */
        [[nodiscard]] std::vector<std::byte> read(const std::size_t bytes_to_read) { // NOLINT(readability-convert-member-functions-to-static)
            std::vector<std::byte> buffer(bytes_to_read);
            const std::size_t actual_bytes_read = read_from_file(buffer.data(), buffer.size());
            buffer.resize(actual_bytes_read);
            return buffer;
        }

        /**
         * Змінює положення вказівника.
         * @param offset Відносне зміщення від offset_mode.
         * @param offset_mode Точка рахування, задається через OffsetMode.
         */
        void seek(long long offset, OffsetMode offset_mode = OffsetMode::current);

        [[nodiscard]] long long tell() const;

        [[nodiscard]] std::size_t size() const;

        [[nodiscard]] bool is_open() const;
    };
} // namespace jzh