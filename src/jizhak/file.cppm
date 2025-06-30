export module jizhak.file;

import std;
export import jizhak.io.file;

namespace jzh {
    class File {
    public:
        using fm = FileIO::FileMode;
    private:
        FileIO file_io{};
        std::vector<std::byte> byte_buffer{};
        std::string text_buffer {};

    protected:
        void read() {}
        void write() {}
    public:
        class Iterator {

        };


        explicit File(const std::filesystem::path& path, fm file_mode = fm::read_write)
            : file_io(path, file_mode) {}

        [[nodiscard]] bool is_open() const {
            return file_io.is_open();
        }

        [[nodiscard]] size_t size_buffer() const {return byte_buffer.size() + text_buffer.size();}
        [[nodiscard]] size_t size_file() const {return file_io.size(); }
    };
}