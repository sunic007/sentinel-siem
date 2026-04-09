#include "storage/mmap_reader.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace sentinel::storage {

MmapReader::MmapReader() = default;

MmapReader::~MmapReader() {
    close();
}

MmapReader::MmapReader(MmapReader&& other) noexcept
    : data_(other.data_), size_(other.size_) {
#ifdef _WIN32
    file_handle_ = other.file_handle_;
    mapping_handle_ = other.mapping_handle_;
    other.file_handle_ = nullptr;
    other.mapping_handle_ = nullptr;
#else
    fd_ = other.fd_;
    other.fd_ = -1;
#endif
    other.data_ = nullptr;
    other.size_ = 0;
}

MmapReader& MmapReader::operator=(MmapReader&& other) noexcept {
    if (this != &other) {
        close();
        data_ = other.data_;
        size_ = other.size_;
#ifdef _WIN32
        file_handle_ = other.file_handle_;
        mapping_handle_ = other.mapping_handle_;
        other.file_handle_ = nullptr;
        other.mapping_handle_ = nullptr;
#else
        fd_ = other.fd_;
        other.fd_ = -1;
#endif
        other.data_ = nullptr;
        other.size_ = 0;
    }
    return *this;
}

bool MmapReader::open(const std::filesystem::path& path) {
    close();

#ifdef _WIN32
    file_handle_ = CreateFileW(
        path.c_str(), GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr
    );
    if (file_handle_ == INVALID_HANDLE_VALUE) {
        spdlog::error("Failed to open file: {}", path.string());
        file_handle_ = nullptr;
        return false;
    }

    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file_handle_, &file_size)) {
        CloseHandle(file_handle_);
        file_handle_ = nullptr;
        return false;
    }
    size_ = static_cast<size_t>(file_size.QuadPart);

    if (size_ == 0) {
        CloseHandle(file_handle_);
        file_handle_ = nullptr;
        return true;  // Empty file
    }

    mapping_handle_ = CreateFileMappingW(
        file_handle_, nullptr, PAGE_READONLY, 0, 0, nullptr
    );
    if (!mapping_handle_) {
        CloseHandle(file_handle_);
        file_handle_ = nullptr;
        return false;
    }

    data_ = static_cast<uint8_t*>(
        MapViewOfFile(mapping_handle_, FILE_MAP_READ, 0, 0, 0)
    );
    if (!data_) {
        CloseHandle(mapping_handle_);
        CloseHandle(file_handle_);
        mapping_handle_ = nullptr;
        file_handle_ = nullptr;
        return false;
    }
#else
    fd_ = ::open(path.c_str(), O_RDONLY);
    if (fd_ < 0) {
        spdlog::error("Failed to open file: {}", path.string());
        return false;
    }

    struct stat st;
    if (fstat(fd_, &st) < 0) {
        ::close(fd_);
        fd_ = -1;
        return false;
    }
    size_ = st.st_size;

    if (size_ == 0) {
        ::close(fd_);
        fd_ = -1;
        return true;
    }

    data_ = static_cast<uint8_t*>(
        mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0)
    );
    if (data_ == MAP_FAILED) {
        data_ = nullptr;
        ::close(fd_);
        fd_ = -1;
        return false;
    }
#endif

    spdlog::debug("Memory-mapped file: {} ({} bytes)", path.string(), size_);
    return true;
}

void MmapReader::close() {
    if (!data_) return;

#ifdef _WIN32
    UnmapViewOfFile(data_);
    if (mapping_handle_) CloseHandle(mapping_handle_);
    if (file_handle_) CloseHandle(file_handle_);
    mapping_handle_ = nullptr;
    file_handle_ = nullptr;
#else
    munmap(data_, size_);
    if (fd_ >= 0) ::close(fd_);
    fd_ = -1;
#endif

    data_ = nullptr;
    size_ = 0;
}

const uint8_t* MmapReader::read_at(size_t offset, size_t length) const {
    if (!data_ || offset + length > size_) {
        return nullptr;
    }
    return data_ + offset;
}

} // namespace sentinel::storage
