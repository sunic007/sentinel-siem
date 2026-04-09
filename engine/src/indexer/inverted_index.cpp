#include "indexer/inverted_index.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace sentinel::indexer {

const PostingList InvertedIndex::EMPTY_LIST;

void InvertedIndex::add_term(std::string_view term, uint64_t event_id) {
    std::string lower_term(term);
    std::transform(lower_term.begin(), lower_term.end(), lower_term.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    auto& list = index_[lower_term];
    if (list.empty() || list.back() != event_id) {
        list.push_back(event_id);
    }
}

void InvertedIndex::index_event(uint64_t event_id, std::string_view raw_text) {
    auto tokens = tokenize(raw_text);
    for (auto& token : tokens) {
        add_term(token, event_id);
    }
}

const PostingList& InvertedIndex::search(const std::string& term) const {
    std::string lower_term(term);
    std::transform(lower_term.begin(), lower_term.end(), lower_term.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    auto it = index_.find(lower_term);
    if (it == index_.end()) return EMPTY_LIST;
    return it->second;
}

PostingList InvertedIndex::search_and(const std::vector<std::string>& terms) const {
    if (terms.empty()) return {};

    PostingList result = search(terms[0]);
    for (size_t i = 1; i < terms.size() && !result.empty(); ++i) {
        result = intersect(result, search(terms[i]));
    }
    return result;
}

PostingList InvertedIndex::search_or(const std::vector<std::string>& terms) const {
    PostingList result;
    for (const auto& term : terms) {
        result = merge(result, search(term));
    }
    return result;
}

size_t InvertedIndex::total_postings() const {
    size_t total = 0;
    for (const auto& [term, list] : index_) {
        total += list.size();
    }
    return total;
}

std::vector<std::string> InvertedIndex::all_terms() const {
    std::vector<std::string> terms;
    terms.reserve(index_.size());
    for (const auto& [term, _] : index_) {
        terms.push_back(term);
    }
    return terms;
}

std::vector<uint8_t> InvertedIndex::serialize() const {
    std::vector<uint8_t> buffer;

    // Format: [term_count][term_len][term_data][posting_count][posting_ids...]
    uint32_t term_count = static_cast<uint32_t>(index_.size());
    buffer.insert(buffer.end(),
        reinterpret_cast<uint8_t*>(&term_count),
        reinterpret_cast<uint8_t*>(&term_count) + sizeof(uint32_t));

    for (const auto& [term, postings] : index_) {
        uint32_t term_len = static_cast<uint32_t>(term.size());
        buffer.insert(buffer.end(),
            reinterpret_cast<uint8_t*>(&term_len),
            reinterpret_cast<uint8_t*>(&term_len) + sizeof(uint32_t));
        buffer.insert(buffer.end(), term.begin(), term.end());

        uint32_t posting_count = static_cast<uint32_t>(postings.size());
        buffer.insert(buffer.end(),
            reinterpret_cast<uint8_t*>(&posting_count),
            reinterpret_cast<uint8_t*>(&posting_count) + sizeof(uint32_t));

        for (uint64_t id : postings) {
            buffer.insert(buffer.end(),
                reinterpret_cast<const uint8_t*>(&id),
                reinterpret_cast<const uint8_t*>(&id) + sizeof(uint64_t));
        }
    }

    return buffer;
}

InvertedIndex InvertedIndex::deserialize(const uint8_t* data, size_t size) {
    InvertedIndex index;
    size_t offset = 0;

    auto read_u32 = [&]() -> uint32_t {
        uint32_t v;
        std::memcpy(&v, data + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        return v;
    };

    auto read_u64 = [&]() -> uint64_t {
        uint64_t v;
        std::memcpy(&v, data + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);
        return v;
    };

    uint32_t term_count = read_u32();
    for (uint32_t i = 0; i < term_count && offset < size; ++i) {
        uint32_t term_len = read_u32();
        std::string term(reinterpret_cast<const char*>(data + offset), term_len);
        offset += term_len;

        uint32_t posting_count = read_u32();
        PostingList postings(posting_count);
        for (uint32_t j = 0; j < posting_count; ++j) {
            postings[j] = read_u64();
        }

        index.index_[std::move(term)] = std::move(postings);
    }

    return index;
}

std::vector<std::string_view> InvertedIndex::tokenize(std::string_view text) {
    std::vector<std::string_view> tokens;
    size_t start = 0;
    bool in_token = false;

    for (size_t i = 0; i <= text.size(); ++i) {
        bool is_delim = (i == text.size()) ||
            std::isspace(static_cast<unsigned char>(text[i])) ||
            text[i] == '=' || text[i] == ',' || text[i] == ';' ||
            text[i] == ':' || text[i] == '"' || text[i] == '\'' ||
            text[i] == '[' || text[i] == ']' || text[i] == '(' ||
            text[i] == ')' || text[i] == '{' || text[i] == '}' ||
            text[i] == '<' || text[i] == '>';

        if (is_delim) {
            if (in_token && i - start >= 2) {  // Min token length = 2
                tokens.push_back(text.substr(start, i - start));
            }
            in_token = false;
        } else if (!in_token) {
            start = i;
            in_token = true;
        }
    }

    return tokens;
}

PostingList InvertedIndex::intersect(const PostingList& a, const PostingList& b) {
    PostingList result;
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] == b[j]) {
            result.push_back(a[i]);
            ++i; ++j;
        } else if (a[i] < b[j]) {
            ++i;
        } else {
            ++j;
        }
    }
    return result;
}

PostingList InvertedIndex::merge(const PostingList& a, const PostingList& b) {
    PostingList result;
    std::merge(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(result));
    auto last = std::unique(result.begin(), result.end());
    result.erase(last, result.end());
    return result;
}

} // namespace sentinel::indexer
