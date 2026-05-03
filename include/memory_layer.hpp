#pragma once

#include "red_black_tree.hpp"
#include "skip_list.hpp"

#include <optional>
#include <utility>
#include <vector>

class MemoryLayer {
public:
    void upsert(int key, const CompressedValue& value);
    std::optional<CompressedValue> find(int key) const;
    std::vector<std::pair<int, CompressedValue>> range(int start, int end) const;
    std::vector<std::pair<int, CompressedValue>> snapshot_sorted() const;
    void clear();
    std::size_t size() const;

private:
    RedBlackTree rb_tree_;
    SkipList skip_list_;
};
