module;

import jizhak.std;
// #include <filesystem>
// #include <stdexcept>
// #include <system_error>
// #include <string>
// #include <optional>
// #include <vector>
// #include <cstddef>
// #include <utility>
// #include <span>
// #include <cerrno>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

export module jizhak.io.file;

export namespace jzh {

    class FileIO final {
    public:
        enum struct FileMode {
            read,
            write,
            append,
            read_write
        };
        enum struct OffsetMode {
            begin,
            current,
            end
        };

        using fm = FileMode;
        using om = OffsetMode;

    private:
        #if defined(_WIN32)
            HANDLE handle_ = INVALID_HANDLE_VALUE;
        #else
            int descriptor_ = -1;
        #endif

        std::filesystem::path file_path_ = std::filesystem::path();
        FileMode mode_ = FileMode::read_write;

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
        FileIO() = default;

        explicit FileIO(const std::filesystem::path &file_path, const FileMode mode = FileMode::read_write) {
            open(file_path, mode);
        }

        FileIO(const FileIO&) = delete;
        FileIO& operator=(const FileIO&) = delete;

        FileIO(FileIO&& other) noexcept {
            file_path_ = std::move(other.file_path_);
            mode_ = other.mode_;

            #if defined(_WIN32)
                handle_ = other.handle_;
                other.handle_ = INVALID_HANDLE_VALUE;
            #else
                descriptor_ = other.descriptor_;
                other.descriptor_ = -1;
            #endif
        }

        ~FileIO() noexcept { // NOLINT(modernize-use-equals-default)
            if (is_open())
                close();
        }

        FileIO& operator=(FileIO&& other) noexcept {
            if (this == &other) return *this;
            this->~FileIO();

            handle_ = other.handle_;
            file_path_ = std::move(other.file_path_);
            mode_ = other.mode_;
            #if defined(_WIN32)
                other.handle_ = INVALID_HANDLE_VALUE;
            #else
                other.descriptor_ = -1;
            #endif
            return *this;
        }

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

        std::optional<std::string> open(const std::filesystem::path &file_path, const FileMode mode = FileMode::read_write) {
            if (is_open()) {
                return std::string("File is already open. Close it before opening a new one.");
            }
            this->file_path_ = file_path;
            this->mode_ = mode;

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
            return std::nullopt;
        }

        std::optional<std::string> close() noexcept {
            if (!is_open()) {
                return std::string("File is not open.");
            };
            #if defined(_WIN32)
                CloseHandle(handle_);
            #else
                ::close(descriptor_);
            #endif
            return std::nullopt;
        }

        void write(const std::byte& data) {
            write_to_file(&data, 1);
        }

        void write(std::span<const std::byte> const data) {
            write_to_file(data.data(), data.size());
        }

        [[nodiscard]] std::byte read() { // NOLINT(readability-convert-member-functions-to-static)
            std::byte buffer{};
            if (const size_t bytes_read = read_from_file(&buffer, 1); bytes_read == 0) throw std::runtime_error("End of file reached");
            return buffer;
        }

        [[nodiscard]] std::vector<std::byte> read(const size_t bytes_to_read) { // NOLINT(readability-convert-member-functions-to-static)
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