#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace sentinel::indexer {

// Posting list: sorted list of event IDs that contain a given term
using PostingList = std::vector<uint64_t>;

// In-memory inverted index for a segment
class InvertedIndex {
public:
    InvertedIndex() = default;

    // Add a term occurrence for an event
    void add_term(std::string_view term, uint64_t event_id);

    // Index all terms from a raw event string
    void index_event(uint64_t event_id, std::string_view raw_text);

    // Search for events containing a term
    const PostingList& search(const std::string& term) const;

    // Search with AND semantics (intersection of posting lists)
    PostingList search_and(const std::vector<std::string>& terms) const;

    // Search with OR semantics (union of posting lists)
    PostingList search_or(const std::vector<std::string>& terms) const;

    // Number of unique terms
    size_t term_count() const { return index_.size(); }

    // Total posting list entries
    size_t total_postings() const;

    // Serialize to binary format
    std::vector<uint8_t> serialize() const;

    // Deserialize from binary format
    static InvertedIndex deserialize(const uint8_t* data, size_t size);

    // Get all terms (for bloom filter construction)
    std::vector<std::string> all_terms() const;

private:
    std::unordered_map<std::string, PostingList> index_;
    static const PostingList EMPTY_LIST;

    // Tokenize raw text into terms
    static std::vector<std::string_view> tokenize(std::string_view text);

    // Intersect two sorted posting lists
    static PostingList intersect(const PostingList& a, const PostingList& b);

    // Union two sorted posting lists
    static PostingList merge(const PostingList& a, const PostingList& b);
};

} // namespace sentinel::indexer
