module;
#if defined(_WIN32)
    #include <windows.h>
#else
    #include <unistd.h>
    #include <fcntl.h>
#endif

export module jizhak.io.file;
import std;

export namespace jzh {

    class FileIO final {
    public:
        enum struct FileMode {
            read,
            write,
            append,
            read_write
        };
        enum struct Encoding {
            UTF8, UTF8_bom, UTF16_le, UTF16_be, UFT32_le, UTF32_be,
            ascii, other, binary
        };
        enum struct OffsetMode {
            begin,
            current,
            end
        };

        using fm = FileMode;
        using enc = Encoding;
        using om = OffsetMode;

    private:
        #if defined(_WIN32)
            HANDLE handle_ = INVALID_HANDLE_VALUE;
        #else
            int descriptor_ = -1;
        #endif

        std::filesystem::path file_path_;
        FileMode mode_;
        Encoding encoding_;

    protected:
        void write_to_file(const void* buffer, size_t size) const {
            if (!is_open()) throw std::runtime_error("File is not open for writing.");

            #if defined(_WIN32)
                DWORD bytes_written = 0;
                if (!WriteFile(handle_, buffer, static_cast<DWORD>(size), &bytes_written, nullptr) || bytes_written != size) {
                    throw std::system_error(GetLastError(), std::system_category(), "Failed to write to file");
                }
            #else
                ssize_t result = ::write(descriptor_, buffer, size);
                if (result == -1 || static_cast<size_t>(result) != size) {
                    throw std::system_error(errno, std::system_category(), "Failed to write to file");
                }
            #endif
        }

        size_t read_from_file(void* buffer, size_t size) const {
            if (!is_open()) throw std::runtime_error("File is not open for reading.");

            #if defined(_WIN32)
                DWORD bytes_read = 0;
                if (!ReadFile(handle_, buffer, static_cast<DWORD>(size), &bytes_read, nullptr)) {
                     throw std::system_error(GetLastError(), std::system_category(), "Failed to read from file");
                }
                return bytes_read;
            #else
                ssize_t result = ::read(descriptor_, buffer, size);
                if (result == -1) {
                    throw std::system_error(errno, std::system_category(), "Failed to read from file");
                }
                return result;
            #endif
        }

    public:
        explicit FileIO(std::filesystem::path file_path, const FileMode mode = FileMode::read_write)
            : file_path_(std::move(file_path)), mode_(mode), encoding_(Encoding::binary) {

            #if defined(_WIN32)
                DWORD dwDesiredAccess = 0;
                DWORD dwCreationDisposition = 0;

                switch (mode_) {
                    case FileMode::read:
                        dwDesiredAccess = GENERIC_READ;
                        dwCreationDisposition = OPEN_EXISTING;
                        break;
                    case FileMode::write:
                        dwDesiredAccess = GENERIC_WRITE;
                        dwCreationDisposition = CREATE_ALWAYS;
                        break;
                    case FileMode::append:
                        dwDesiredAccess = FILE_APPEND_DATA;
                        dwCreationDisposition = OPEN_ALWAYS;
                        break;
                    case FileMode::read_write:
                        dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
                        dwCreationDisposition = OPEN_ALWAYS;
                        break;
                }

                handle_ = CreateFileW(file_path_.c_str(), dwDesiredAccess, FILE_SHARE_READ, nullptr, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (handle_ == INVALID_HANDLE_VALUE) {
                    throw std::system_error(GetLastError(), std::system_category(), "Failed to open file: " + file_path_.string());
                }

            #else // POSIX
                int flags = 0;
                switch (mode_) {
                    case FileMode::read:       flags = O_RDONLY; break;
                    case FileMode::write:      flags = O_WRONLY | O_CREAT | O_TRUNC; break;
                    case FileMode::append:     flags = O_WRONLY | O_CREAT | O_APPEND; break;
                    case FileMode::read_write: flags = O_RDWR | O_CREAT; break;
                }

                descriptor_ = ::open(file_path_.c_str(), flags, 0664);
                if (descriptor_ == -1) {
                    throw std::system_error(errno, std::system_category(), "Failed to open file: " + file_path_.string());
                }
            #endif
        }

        ~FileIO() {
            if (!is_open()) return;
            #if defined(_WIN32)
                CloseHandle(handle_);
            #else
                ::close(descriptor_);
            #endif
        }

        FileIO(const FileIO&) = delete;
        FileIO& operator=(const FileIO&) = delete;

        FileIO(FileIO&& other) noexcept {
            handle_ = other.handle_;
            file_path_ = std::move(other.file_path_);
            mode_ = other.mode_;
            encoding_ = other.encoding_;

            #if defined(_WIN32)
                other.handle_ = INVALID_HANDLE_VALUE;
            #else
                other.descriptor_ = -1;
            #endif
        }

        FileIO& operator=(FileIO&& other) noexcept {
            if (this == &other) return *this;
            this->~FileIO();

            handle_ = other.handle_;
            file_path_ = std::move(other.file_path_);
            mode_ = other.mode_;
            encoding_ = other.encoding_;
            #if defined(_WIN32)
                other.handle_ = INVALID_HANDLE_VALUE;
            #else
                other.descriptor_ = -1;
            #endif
            return *this;
        }

        void write(const std::byte& data) {
            write_to_file(&data, 1);
        }

        void write(std::span<const std::byte> const data) {
            write_to_file(data.data(), data.size());
        }

        [[nodiscard]] std::byte read() {
            std::byte buffer{};
            if (const size_t bytes_read = read_from_file(&buffer, 1); bytes_read == 0) throw std::runtime_error("End of file reached");
            return buffer;
        }

        [[nodiscard]] std::vector<std::byte> read(const size_t bytes_to_read) {
            std::vector<std::byte> buffer(bytes_to_read);
            const size_t actual_bytes_read = read_from_file(buffer.data(), buffer.size());
            buffer.resize(actual_bytes_read);
            return buffer;
        }

        void seek(long long offset, OffsetMode offset_mode = OffsetMode::current) {
            #if defined(_WIN32)
                DWORD moveMethod = FILE_CURRENT;
                switch(offset_mode) {
                    case OffsetMode::begin: moveMethod = FILE_BEGIN; break;
                    case OffsetMode::current: moveMethod = FILE_CURRENT; break;
                    case OffsetMode::end: moveMethod = FILE_END; break;
                }
                LARGE_INTEGER li;
                li.QuadPart = offset;
                if (!SetFilePointerEx(handle_, li, nullptr, moveMethod)) {
                    throw std::system_error(GetLastError(), std::system_category(), "Failed to seek in file");
                }
            #else
                int whence = SEEK_CUR;
                switch(offset_mode) {
                    case OffsetMode::begin: whence = SEEK_SET; break;
                    case OffsetMode::current: whence = SEEK_CUR; break;
                    case OffsetMode::end: whence = SEEK_END; break;
                }
                if (::lseek(descriptor_, offset, whence) == -1) {
                     throw std::system_error(errno, std::system_category(), "Failed to seek in file");
                }
            #endif
        }

        [[nodiscard]] long long tell() const {
            #if defined(_WIN32)
                LARGE_INTEGER li;
                li.QuadPart = 0;
                if (!SetFilePointerEx(handle_, li, &li, FILE_CURRENT)) {
                    throw std::system_error(GetLastError(), std::system_category(), "Failed to tell file position");
                }
                return li.QuadPart;
            #else
                long long pos = ::lseek(descriptor_, 0, SEEK_CUR);
                if (pos == -1) {
                    throw std::system_error(errno, std::system_category(), "Failed to tell file position");
                }
                return pos;
            #endif
        }

        [[nodiscard]] size_t size() const {
            #if defined(_WIN32)
                LARGE_INTEGER li;
                if (!GetFileSizeEx(handle_, &li)) {
                     throw std::system_error(GetLastError(), std::system_category(), "Failed to get file size");
                }
                return li.QuadPart;
            #else
                long long current_pos = tell();
                long long size = ::lseek(descriptor_, 0, SEEK_END);
                // Повертаємо курсор на місце
                ::lseek(descriptor_, current_pos, SEEK_SET);
                if (size == -1) {
                    throw std::system_error(errno, std::system_category(), "Failed to get file size");
                }
                return size;
            #endif
        }

        [[nodiscard]] bool is_open() const {
            #if defined(_WIN32)
                return handle_ != INVALID_HANDLE_VALUE;
            #else
                return descriptor_ != -1;
            #endif
        }
    };
} // namespace jzh