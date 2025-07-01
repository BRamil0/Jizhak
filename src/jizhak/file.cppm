module;
#include "boost/locale.hpp"

export module jizhak.file;

import std;
export import jizhak.io.file;
export import jizhak.error;

namespace jzh {
    class File {
    public:
        using fm = FileIO::FileMode;
        using om = FileIO::OffsetMode;
    private:
        FileIO file_io{};
        std::vector<std::byte> byte_buffer {};
        std::string text_buffer {};
        std::string encoding {};
        bool unicode = false;

    protected:
        std::optional<JizhakError> read_byts(const size_t bytes_to_read = 4) {
            if (file_io.tell() == file_io.size())
                return JizhakError(JizhakErrorID::max_size_file);
            this->byte_buffer.append_range(file_io.read(bytes_to_read));
            return std::nullopt;
        }

        void write_byts() {}

        struct UtfAnalysisResult {
            size_t incomplete_bytes_at_end = 0; // Скільки байт в кінці буфера є частиною неповного символу
            size_t needed_bytes_to_complete = 0; // Скільки ще байт потрібно дочитати
        };

        // Крок 2: Функція аналізу буфера
        UtfAnalysisResult analyze_buffer_completeness() const {
            if (byte_buffer.empty()) {
                return {0, 0};
            }

            if (encoding == "UTF-8") {
                // Логіка для UTF-8
                size_t look_behind = 1;
                while (look_behind <= byte_buffer.size() && look_behind <= 4) {
                    const auto b = static_cast<unsigned char>(byte_buffer[byte_buffer.size() - look_behind]);
                    if ((b & 0xC0) != 0x80) { // Знайшли початковий байт (не 10xxxxxx)
                        size_t expected_length = 0;
                        if ((b & 0x80) == 0) expected_length = 1;       // 0xxxxxxx
                        else if ((b & 0xE0) == 0xC0) expected_length = 2; // 110xxxxx
                        else if ((b & 0xF0) == 0xE0) expected_length = 3; // 1110xxxx
                        else if ((b & 0xF8) == 0xF0) expected_length = 4; // 11110xxx
                        else return {look_behind, 0}; // Некоректний початковий байт, вважаємо його помилкою

                        if (look_behind < expected_length) {
                            // Ми знайшли початок, але послідовність неповна
                            return {look_behind, expected_length - look_behind};
                        }
                        return {0, 0}; // Все добре, останній символ повний
                    }
                    look_behind++;
                }
                return {look_behind - 1, 0}; // Всі байти - продовження, це помилка
            }


            if (encoding == "UTF-16") {
                size_t incomplete = byte_buffer.size() % 2;
                return {incomplete, incomplete > 0 ? size_t{1} : size_t{0}};
            }

            if (encoding == "UTF-32") {
                size_t incomplete = byte_buffer.size() % 4;
                return {incomplete, incomplete > 0 ? (4 - incomplete) : size_t{0}};
            }

            // Для інших кодувань кожен байт вважається повним
            return {0, 0};
        }

        // Крок 4: Декодування буфера байтів у внутрішній текстовий буфер (UTF-8)
        void decode_to_internal_buffer(size_t bytes_to_decode) {
            if (bytes_to_decode == 0) return;

            std::string_view byte_view(
                reinterpret_cast<const char*>(byte_buffer.data()),
                bytes_to_decode
            );

            try {
                if (encoding == "UTF-8") {
                    text_buffer.append(byte_view);
                } else if (encoding == "UTF-16" || encoding == "UTF-32") {
                    text_buffer += boost::locale::conv::utf_to_utf<char>(byte_view.data(), byte_view.data() + byte_view.size());
                }
                else {
                    boost::locale::generator gen;
                    std::locale file_locale = gen("." + encoding);
                    text_buffer += boost::locale::conv::to_utf<char>(byte_view.data(), byte_view.data() + byte_view.size(), file_locale);
                }
                // Видаляємо з буфера байти, які успішно сконвертували
                byte_buffer.erase(byte_buffer.begin(), byte_buffer.begin() + bytes_to_decode);
            } catch(...) {
                // Якщо декодування не вдалося, це помилка в даних файлу
                // Можна кинути виняток або повернути помилку
                throw std::runtime_error("Failed to decode file content, data may be corrupted.");
            }
        }
    public:
        class Iterator {

        };


        explicit File(const std::filesystem::path& path, fm file_mode = fm::read_write)
            : file_io(path, file_mode) {}

template <typename T>
        T read(size_t chars_to_read) {
            // Тимчасовий буфер для результату, який буде повернуто у форматі T
            T result_buffer;

            // Основний цикл: працюємо, доки не накопичимо потрібну кількість символів
            while (boost::locale::conv::utf_to_utf<char32_t>(text_buffer).length() < chars_to_read) {
                // Крок 1: Читаємо порцію байтів
                if (file_io.tell() < file_io.size()) {
                    byte_buffer = file_io.read(4096); // Читаємо в кінець буфера
                }

                // Крок 2: Аналізуємо, чи повний останній символ
                UtfAnalysisResult analysis = analyze_buffer_completeness();

                // Крок 3: Перевіряємо, чи можемо дочитати необхідні байти
                if (analysis.needed_bytes_to_complete > 0) {
                    size_t bytes_available_in_file = file_io.size() - file_io.tell();

                    if (bytes_available_in_file >= analysis.needed_bytes_to_complete) {
                        // Крок 3.2: Так, можемо. Дочитуємо рівно стільки, скільки треба.
                        byte_buffer = file_io.read(analysis.needed_bytes_to_complete);
                        analysis = {0, 0}; // Тепер буфер гарантовано повний
                    } else {
                        // Крок 3.1: Ні, не можемо (кінець файлу). Відрізаємо хвіст.
                        // Ці байти будуть проігноровані, бо утворюють неповний символ в кінці файлу.
                    }
                }

                // Декодуємо все, окрім неповного хвоста
                decode_to_internal_buffer(byte_buffer.size() - analysis.incomplete_bytes_at_end);

                // Якщо ми в кінці файлу і більше нічого не можемо прочитати, виходимо
                if (file_io.tell() == file_io.size() && byte_buffer.empty()) {
                    break;
                }
            }

            // Крок 5: Перекодовуємо з внутрішнього UTF-8 у потрібний користувачу формат T
            size_t available_chars = boost::locale::conv::utf_to_utf<char32_t>(text_buffer).length();
            size_t chars_to_process = std::min(available_chars, chars_to_read);

            // Знаходимо позицію в байтах, яка відповідає потрібній кількості символів
            size_t byte_pos_to_cut = boost::locale::boundary::character(text_buffer.begin(), text_buffer.end(), chars_to_process).base() - text_buffer.begin();

            std::string_view view_to_convert(text_buffer.data(), byte_pos_to_cut);

            if constexpr (std::is_same_v<T, std::string>) {
                result_buffer.assign(view_to_convert);
            } else {
                // Для std::wstring, std::u16string etc.
                result_buffer = boost::locale::conv::utf_to_utf<typename T::value_type>(view_to_convert);
            }

            text_buffer.erase(0, byte_pos_to_cut);

            return result_buffer;
        }

        template <typename T>
        T read(size_t text_to_read, long long offset, om offset_mode = om::current) {
            byte_buffer.clear();
            text_buffer.clear();
            file_io.seek(offset, offset_mode);
            return read<T>(text_to_read);
        }

        [[nodiscard]] bool is_open() const {
            return file_io.is_open();
        }
        [[nodiscard]] bool is_unicode() const {
            return unicode;
        }

        [[nodiscard]] size_t size_buffer() const {return byte_buffer.size() + text_buffer.size();}
        [[nodiscard]] size_t size_file() const {return file_io.size(); }
    };
}