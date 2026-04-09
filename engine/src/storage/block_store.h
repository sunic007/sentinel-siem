#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

namespace sentinel::storage {

// Block-level I/O abstraction for segment files
class BlockStore {
public:
    explicit BlockStore(const std::filesystem::path& base_dir);

    // Write a block of data, returns block ID
    uint64_t write_block(const std::string& index_name,
                         const void* data, size_t size);

    // Read a block by ID
    std::vector<uint8_t> read_block(const std::string& index_name,
                                     uint64_t block_id);

    // Delete a block
    bool delete_block(const std::string& index_name, uint64_t block_id);

    // Get total disk usage for an index
    size_t disk_usage(const std::string& index_name) const;

    // Ensure index directory exists
    void ensure_index_dir(const std::string& index_name);

    const std::filesystem::path& base_dir() const { return base_dir_; }

private:
    std::filesystem::path base_dir_;
    std::filesystem::path block_path(const std::string& index_name,
                                      uint64_t block_id) const;
};

} // namespace sentinel::storage
