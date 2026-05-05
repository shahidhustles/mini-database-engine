#include "memory_layer.hpp"

using namespace std;

void MemoryLayer::upsert(int key, const CompressedValue& value) {
    rb_tree_.upsert(key, value);
    skip_list_.upsert(key, value);
}

optional<CompressedValue> MemoryLayer::find(int key) const {
    return rb_tree_.find(key);
}

optional<CompressedValue> MemoryLayer::find_with_trace(int key, vector<int>& rb_path) const {
    return rb_tree_.find_with_path(key, rb_path);
}

vector<pair<int, CompressedValue>> MemoryLayer::range(int start, int end) const {
    return skip_list_.range(start, end);
}

vector<pair<int, CompressedValue>> MemoryLayer::range_with_trace(int start,
                                                                 int end,
                                                                 vector<int>& skip_visited,
                                                                 vector<int>& included_keys) const {
    return skip_list_.range_with_trace(start, end, skip_visited, included_keys);
}

vector<pair<int, CompressedValue>> MemoryLayer::snapshot_sorted() const {
    return skip_list_.snapshot_sorted();
}

GraphSnapshot MemoryLayer::rb_tree_snapshot(const vector<int>& active_keys,
                                            optional<int> highlighted_key) const {
    return rb_tree_.snapshot(active_keys, highlighted_key);
}

GraphSnapshot MemoryLayer::skip_list_snapshot(const vector<int>& active_keys) const {
    return skip_list_.snapshot(active_keys);
}

void MemoryLayer::clear() {
    rb_tree_.clear();
    skip_list_.clear();
}

size_t MemoryLayer::size() const {
    return rb_tree_.size();
}
