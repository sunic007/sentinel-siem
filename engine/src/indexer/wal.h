#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <cstdint>

namespace sentinel::indexer {

// Write-Ahead Log entry
struct WalEntry {
    uint64_t sequence_number;
    uint64_t timestamp_us;       // Microsecond timestamp
    std::string index_name;
    std::string raw_event;
    std::string host;
    std::string source;
    std::string sourcetype;
};

// Write-Ahead Log for durability before indexing
class Wal {
public:
    explicit Wal(const std::filesystem::path& dir, size_t max_size = 16 * 1024 * 1024);
    ~Wal();

    // Append an entry to the WAL
    uint64_t append(const WalEntry& entry);

    // Read all entries from the current WAL
    std::vector<WalEntry> read_all() const;

    // Mark entries as committed (safe to remove)
    void commit(uint64_t up_to_sequence);

    // Rotate to a new WAL file
    void rotate();

    // Get current WAL size
    size_t current_size() const;

    // Recovery: replay uncommitted entries
    std::vector<WalEntry> recover();

private:
    std::filesystem::path dir_;
    std::filesystem::path current_path_;
    size_t max_size_;
    uint64_t next_sequence_ = 1;
    mutable std::mutex mutex_;
    std::ofstream writer_;

    void open_writer();
    void serialize_entry(std::ostream& out, const WalEntry& entry) const;
    WalEntry deserialize_entry(std::istream& in) const;
};

} // namespace sentinel::indexer
