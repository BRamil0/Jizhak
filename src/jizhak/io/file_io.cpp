module;
#include <memory>
#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

module jizhak.io.file;

namespace jzh {
    struct FileIO::System {
        #if defined(_WIN32)
        HANDLE handle_ = INVALID_HANDLE_VALUE;
        #else
        int descriptor_ = -1;
        #endif
    };


    void FileIO::write_to_file(const void* buffer, std::size_t size) const {
        if (!is_open()) throw std::runtime_error("File is not open for writing.");

#if defined(_WIN32)
        DWORD bytes_written = 0;
        if (!WriteFile(system_->handle_, buffer, static_cast<DWORD>(size), &bytes_written, nullptr) || bytes_written != size) {
            throw std::system_error(GetLastError(), std::system_category(), "Failed to write to file");
        }
#else
        ssize_t result = ::write(system_->descriptor_, buffer, size);
        if (result == -1 || static_cast<std::size_t>(result) != size) {
            throw std::system_error(errno, std::system_category(), "Failed to write to file");
        }
#endif
    }

    std::size_t FileIO::read_from_file(void* buffer, std::size_t size) const {
        if (!is_open()) throw std::runtime_error("File is not open for reading.");

#if defined(_WIN32)
        DWORD bytes_read = 0;
        if (!ReadFile(system_->handle_, buffer, static_cast<DWORD>(size), &bytes_read, nullptr)) {
            throw std::system_error(GetLastError(), std::system_category(), "Failed to read from file");
        }
        return bytes_read;
#else
        ssize_t result = ::read(system_->descriptor_, buffer, size);
        if (result == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to read from file");
        }
        return result;
#endif
    }

    FileIO::FileIO() : system_(std::make_unique<System>()) {}

    FileIO::FileIO(const std::filesystem::path &file_path, const FileMode mode)
    : system_(std::make_unique<System>())
    {
        open(file_path, mode);
    }

    FileIO::FileIO(FileIO&& other) noexcept {
        file_path_ = std::move(other.file_path_);
        mode_ = other.mode_;

            #if defined(_WIN32)
        system_->handle_ = other.system_->handle_;
        other.system_->handle_ = INVALID_HANDLE_VALUE;
            #else
        system_->descriptor_ = other.system_->descriptor_;
        other.system_->descriptor_ = -1;
            #endif
    }

    FileIO& FileIO::operator=(FileIO&& other) noexcept {
        if (this == &other) return *this;
        this->~FileIO();

        system_->handle_ = other.system_->handle_;
        file_path_ = std::move(other.file_path_);
        mode_ = other.mode_;
            #if defined(_WIN32)
        other.system_->handle_ = INVALID_HANDLE_VALUE;
            #else
        other.system_->descriptor_ = -1;
            #endif
        return *this;
    }

    FileIO::~FileIO() noexcept { // NOLINT(modernize-use-equals-default)
        if (is_open())
            close();
    }


    std::optional<JizhakError> FileIO::open(const std::filesystem::path &file_path, const FileMode mode = FileMode::read_write) {
    if (is_open())
        return JizhakError(JizhakErrorID::file_already_open, "File is already open. Close it before opening a new one.");

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

        system_->handle_ = CreateFileW(file_path_.c_str(), dwDesiredAccess, FILE_SHARE_READ, nullptr, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (system_->handle_ == INVALID_HANDLE_VALUE) {
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

        system_->descriptor_ = ::open(file_path_.c_str(), flags, 0664);
        if (system_->descriptor_ == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to open file: " + file_path_.string());
        }
    #endif
    return std::nullopt;
}

    std::optional<JizhakError> FileIO::close() noexcept {
        if (!is_open())
            return JizhakError(JizhakErrorID::file_not_open,"File is not open.");

        #if defined(_WIN32)
            CloseHandle(system_->handle_);
        #else
            ::close(system_->descriptor_);
        #endif
        return std::nullopt;
    }


    void FileIO::seek(long long offset, OffsetMode offset_mode) {
        #if defined(_WIN32)
            DWORD moveMethod = FILE_CURRENT;
            switch(offset_mode) {
                case OffsetMode::begin: moveMethod = FILE_BEGIN; break;
                case OffsetMode::current: moveMethod = FILE_CURRENT; break;
                case OffsetMode::end: moveMethod = FILE_END; break;
            }
            LARGE_INTEGER li;
            li.QuadPart = offset;
            if (!SetFilePointerEx(system_->handle_, li, nullptr, moveMethod)) {
                throw std::system_error(GetLastError(), std::system_category(), "Failed to seek in file");
            }
        #else
            int whence = SEEK_CUR;
            switch(offset_mode) {
                case OffsetMode::begin: whence = SEEK_SET; break;
                case OffsetMode::current: whence = SEEK_CUR; break;
                case OffsetMode::end: whence = SEEK_END; break;
            }
            if (::lseek(system_->descriptor_, offset, whence) == -1) {
                 throw std::system_error(errno, std::system_category(), "Failed to seek in file");
            }
        #endif
        }

    [[nodiscard]] long long FileIO::tell() const {
        #if defined(_WIN32)
            LARGE_INTEGER li;
            li.QuadPart = 0;
            if (!SetFilePointerEx(system_->handle_, li, &li, FILE_CURRENT)) {
                throw std::system_error(GetLastError(), std::system_category(), "Failed to tell file position");
            }
            return li.QuadPart;
        #else
            long long pos = ::lseek(system_->descriptor_, 0, SEEK_CUR);
            if (pos == -1) {
                throw std::system_error(errno, std::system_category(), "Failed to tell file position");
            }
            return pos;
        #endif
    }

    [[nodiscard]] std::size_t FileIO::size() const {
        #if defined(_WIN32)
            LARGE_INTEGER li;
            if (!GetFileSizeEx(system_->handle_, &li)) {
                 throw std::system_error(GetLastError(), std::system_category(), "Failed to get file size");
            }
            return li.QuadPart;
        #else
            long long current_pos = tell();
            long long size = ::lseek(system_->descriptor_, 0, SEEK_END);
            // Повертаємо курсор на місце
            ::lseek(system_->descriptor_, current_pos, SEEK_SET);
            if (size == -1) {
                throw std::system_error(errno, std::system_category(), "Failed to get file size");
            }
            return size;
        #endif
    }

    [[nodiscard]] bool FileIO::is_open() const {
        #if defined(_WIN32)
            return system_->handle_ != INVALID_HANDLE_VALUE;
        #else
            return system_->descriptor_ != -1;
        #endif
    }

}