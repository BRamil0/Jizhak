module;
#include "boost/locale.hpp"

export module jizhak.file;

import std;
export import jizhak.io.file;
export import jizhak.error;

template <typename T>
concept is_supported_string = requires(T t) {
    { std::basic_string_view(t) } -> std::same_as<std::basic_string_view<typename decltype(std::basic_string_view(t))::value_type>>;
    requires std::same_as<typename decltype(std::basic_string_view(t))::value_type, char> ||
             std::same_as<typename decltype(std::basic_string_view(t))::value_type, wchar_t> ||
             std::same_as<typename decltype(std::basic_string_view(t))::value_type, char8_t> ||
             std::same_as<typename decltype(std::basic_string_view(t))::value_type, char16_t> ||
             std::same_as<typename decltype(std::basic_string_view(t))::value_type, char32_t>;
};

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

        void write_byts(const std::vector<std::byte> &bytes_to_write) {
            this->file_io.write(bytes_to_write);
        }

        struct UtfAnalysisResult {
            size_t incomplete_bytes_at_end = 0; // Скільки байт в кінці буфера є частиною неповного символу
            size_t needed_bytes_to_complete = 0; // Скільки ще байт потрібно дочитати
        };

        UtfAnalysisResult analyze_buffer_completeness() const {
            if (byte_buffer.empty()) {
                return {0, 0};
            }

            if (encoding == "UTF-8") {
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

        File(const File&) = delete;
        File(File&&) {}

        File& operator=(const File&) = delete;
        File& operator=(File&&) {}

        virtual ~File() = default;


        template <typename T> requires is_supported_string<T>
        T read(size_t chars_to_read) {
            T result_buffer;

            while (boost::locale::conv::utf_to_utf<char32_t>(text_buffer).length() < chars_to_read) {
                if (file_io.tell() < file_io.size()) {
                    byte_buffer = file_io.read(4096);
                }

                UtfAnalysisResult analysis = analyze_buffer_completeness();

                // Перевіряємо, чи можемо дочитати необхідні байти
                if (analysis.needed_bytes_to_complete > 0) {
                    size_t bytes_available_in_file = file_io.size() - file_io.tell();

                    if (bytes_available_in_file >= analysis.needed_bytes_to_complete) {
                        // Так, можемо. Дочитуємо рівно стільки, скільки треба.
                        byte_buffer = file_io.read(analysis.needed_bytes_to_complete);
                        analysis = {0, 0};
                    } else {
                        // Ні, не можемо (кінець файлу). Відрізаємо хвіст.
                    }
                }

                decode_to_internal_buffer(byte_buffer.size() - analysis.incomplete_bytes_at_end);

                if (file_io.tell() == file_io.size() && byte_buffer.empty()) {
                    break;
                }
            }

            size_t available_chars = boost::locale::conv::utf_to_utf<char32_t>(text_buffer).length();
            size_t chars_to_process = std::min(available_chars, chars_to_read);

            size_t byte_pos_to_cut = boost::locale::boundary::character(text_buffer.begin(), text_buffer.end(), chars_to_process).base() - text_buffer.begin();

            std::string_view view_to_convert(text_buffer.data(), byte_pos_to_cut);

            if constexpr (std::is_same_v<T, std::string>) {
                result_buffer.assign(view_to_convert);
            } else {
                result_buffer = boost::locale::conv::utf_to_utf<typename T::value_type>(view_to_convert);
            }

            text_buffer.erase(0, byte_pos_to_cut);

            return result_buffer;
        }

        template <typename T> requires is_supported_string<T>
        T read(size_t text_to_read, long long offset, om offset_mode = om::current) {
            byte_buffer.clear();
            text_buffer.clear();
            file_io.seek(offset, offset_mode);
            return read<T>(text_to_read);
        }

        template <typename T> requires is_supported_string<T>
        std::optional<JizhakError> write(std::basic_string_view<T> text) {
            return JizhakError(JizhakErrorID::pass);
        }

        [[nodiscard]] bool is_open() const {return file_io.is_open(); }
        [[nodiscard]] bool is_unicode() const {return unicode; }

        [[nodiscard]] size_t size_buffer() const {return byte_buffer.size() + text_buffer.size(); }
        [[nodiscard]] size_t size_file() const {return file_io.size(); }
    };
}