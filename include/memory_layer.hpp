#pragma once

#include "red_black_tree.hpp"
#include "skip_list.hpp"
#include "trace_models.hpp"

#include <optional>
#include <utility>
#include <vector>

class MemoryLayer {
public:
    void upsert(int key, const CompressedValue& value);
    std::optional<CompressedValue> find(int key) const;
    std::optional<CompressedValue> find_with_trace(int key, std::vector<int>& rb_path) const;
    std::vector<std::pair<int, CompressedValue>> range(int start, int end) const;
    std::vector<std::pair<int, CompressedValue>> range_with_trace(int start,
                                                                  int end,
                                                                  std::vector<int>& skip_visited,
                                                                  std::vector<int>& included_keys) const;
    std::vector<std::pair<int, CompressedValue>> snapshot_sorted() const;
    GraphSnapshot rb_tree_snapshot(const std::vector<int>& active_keys = {},
                                   std::optional<int> highlighted_key = std::nullopt) const;
    GraphSnapshot skip_list_snapshot(const std::vector<int>& active_keys = {}) const;
    void clear();
    std::size_t size() const;

private:
    RedBlackTree rb_tree_;
    SkipList skip_list_;
};
