#include "memory_layer.hpp"

using namespace std;

void MemoryLayer::upsert(int key, const CompressedValue& value) {
    rb_tree_.upsert(key, value);
    skip_list_.upsert(key, value);
}

optional<CompressedValue> MemoryLayer::find(int key) const {
    return rb_tree_.find(key);
}

vector<pair<int, CompressedValue>> MemoryLayer::range(int start, int end) const {
    return skip_list_.range(start, end);
}

vector<pair<int, CompressedValue>> MemoryLayer::snapshot_sorted() const {
    return skip_list_.snapshot_sorted();
}

void MemoryLayer::clear() {
    rb_tree_.clear();
    skip_list_.clear();
}

size_t MemoryLayer::size() const {
    return rb_tree_.size();
}
