#include "storage/block_store.h"
#include <fstream>
#include <spdlog/spdlog.h>
#include <atomic>

namespace sentinel::storage {

static std::atomic<uint64_t> g_next_block_id{1};

BlockStore::BlockStore(const std::filesystem::path& base_dir)
    : base_dir_(base_dir) {
    std::filesystem::create_directories(base_dir_);
}

void BlockStore::ensure_index_dir(const std::string& index_name) {
    auto dir = base_dir_ / index_name;
    std::filesystem::create_directories(dir);
}

uint64_t BlockStore::write_block(const std::string& index_name,
                                  const void* data, size_t size) {
    ensure_index_dir(index_name);
    uint64_t block_id = g_next_block_id.fetch_add(1);

    auto path = block_path(index_name, block_id);
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        spdlog::error("Failed to create block file: {}", path.string());
        return 0;
    }

    file.write(static_cast<const char*>(data), size);
    file.flush();

    spdlog::debug("Wrote block {} ({} bytes) to {}", block_id, size, index_name);
    return block_id;
}

std::vector<uint8_t> BlockStore::read_block(const std::string& index_name,
                                             uint64_t block_id) {
    auto path = block_path(index_name, block_id);
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        spdlog::error("Block file not found: {}", path.string());
        return {};
    }

    auto size = file.tellg();
    file.seekg(0);

    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

bool BlockStore::delete_block(const std::string& index_name, uint64_t block_id) {
    auto path = block_path(index_name, block_id);
    return std::filesystem::remove(path);
}

size_t BlockStore::disk_usage(const std::string& index_name) const {
    auto dir = base_dir_ / index_name;
    if (!std::filesystem::exists(dir)) return 0;

    size_t total = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            total += entry.file_size();
        }
    }
    return total;
}

std::filesystem::path BlockStore::block_path(const std::string& index_name,
                                              uint64_t block_id) const {
    return base_dir_ / index_name / ("block_" + std::to_string(block_id) + ".dat");
}

} // namespace sentinel::storage
