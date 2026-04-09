#include "indexer/wal.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace sentinel::indexer {

Wal::Wal(const std::filesystem::path& dir, size_t max_size)
    : dir_(dir), max_size_(max_size) {
    std::filesystem::create_directories(dir_);
    current_path_ = dir_ / "wal_current.log";
    open_writer();

    // Recover sequence number from existing WAL
    auto entries = read_all();
    if (!entries.empty()) {
        next_sequence_ = entries.back().sequence_number + 1;
    }

    spdlog::info("WAL initialized at {}, next sequence: {}", dir_.string(), next_sequence_);
}

Wal::~Wal() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (writer_.is_open()) {
        writer_.flush();
        writer_.close();
    }
}

void Wal::open_writer() {
    writer_.open(current_path_, std::ios::binary | std::ios::app);
    if (!writer_) {
        spdlog::error("Failed to open WAL file: {}", current_path_.string());
    }
}

uint64_t Wal::append(const WalEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);

    WalEntry e = entry;
    e.sequence_number = next_sequence_++;

    serialize_entry(writer_, e);
    writer_.flush();

    // Check if rotation is needed
    if (current_size() >= max_size_) {
        rotate();
    }

    return e.sequence_number;
}

void Wal::serialize_entry(std::ostream& out, const WalEntry& entry) const {
    // Simple binary format: [seq][timestamp][len+data for each string field]
    out.write(reinterpret_cast<const char*>(&entry.sequence_number), sizeof(uint64_t));
    out.write(reinterpret_cast<const char*>(&entry.timestamp_us), sizeof(uint64_t));

    auto write_string = [&out](const std::string& s) {
        uint32_t len = static_cast<uint32_t>(s.size());
        out.write(reinterpret_cast<const char*>(&len), sizeof(uint32_t));
        out.write(s.data(), len);
    };

    write_string(entry.index_name);
    write_string(entry.raw_event);
    write_string(entry.host);
    write_string(entry.source);
    write_string(entry.sourcetype);
}

WalEntry Wal::deserialize_entry(std::istream& in) const {
    WalEntry entry;
    in.read(reinterpret_cast<char*>(&entry.sequence_number), sizeof(uint64_t));
    in.read(reinterpret_cast<char*>(&entry.timestamp_us), sizeof(uint64_t));

    auto read_string = [&in]() -> std::string {
        uint32_t len;
        in.read(reinterpret_cast<char*>(&len), sizeof(uint32_t));
        std::string s(len, '\0');
        in.read(s.data(), len);
        return s;
    };

    entry.index_name = read_string();
    entry.raw_event = read_string();
    entry.host = read_string();
    entry.source = read_string();
    entry.sourcetype = read_string();

    return entry;
}

std::vector<WalEntry> Wal::read_all() const {
    std::vector<WalEntry> entries;

    std::ifstream reader(current_path_, std::ios::binary);
    if (!reader) return entries;

    while (reader.peek() != EOF) {
        try {
            entries.push_back(deserialize_entry(reader));
        } catch (...) {
            spdlog::warn("WAL read error, stopping at {} entries", entries.size());
            break;
        }
    }

    return entries;
}

void Wal::commit(uint64_t up_to_sequence) {
    // In a full implementation, this would mark entries as committed
    // and allow truncation. For now, we track the committed sequence.
    spdlog::debug("WAL committed up to sequence {}", up_to_sequence);
}

void Wal::rotate() {
    if (writer_.is_open()) {
        writer_.flush();
        writer_.close();
    }

    // Archive old WAL
    auto archive_path = dir_ / ("wal_" + std::to_string(next_sequence_) + ".log");
    std::filesystem::rename(current_path_, archive_path);

    // Open new WAL
    open_writer();
    spdlog::info("WAL rotated, archived to {}", archive_path.string());
}

size_t Wal::current_size() const {
    if (std::filesystem::exists(current_path_)) {
        return std::filesystem::file_size(current_path_);
    }
    return 0;
}

std::vector<WalEntry> Wal::recover() {
    spdlog::info("Starting WAL recovery...");
    auto entries = read_all();
    spdlog::info("Recovered {} entries from WAL", entries.size());
    return entries;
}

} // namespace sentinel::indexer
