#pragma once

#include <string>
#include <cstdint>
#include <filesystem>

namespace sentinel::storage {

// Memory-mapped file reader for fast segment access
class MmapReader {
public:
    MmapReader();
    ~MmapReader();

    MmapReader(const MmapReader&) = delete;
    MmapReader& operator=(const MmapReader&) = delete;
    MmapReader(MmapReader&& other) noexcept;
    MmapReader& operator=(MmapReader&& other) noexcept;

    // Open a file for memory-mapped reading
    bool open(const std::filesystem::path& path);

    // Close the mapping
    void close();

    // Access the mapped data
    const uint8_t* data() const { return data_; }
    size_t size() const { return size_; }
    bool is_open() const { return data_ != nullptr; }

    // Read a range within the mapped file
    const uint8_t* read_at(size_t offset, size_t length) const;

private:
    uint8_t* data_ = nullptr;
    size_t size_ = 0;
#ifdef _WIN32
    void* file_handle_ = nullptr;
    void* mapping_handle_ = nullptr;
#else
    int fd_ = -1;
#endif
};

} // namespace sentinel::storage
