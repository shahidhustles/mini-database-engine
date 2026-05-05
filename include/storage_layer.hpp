#pragma once

#include "bplus_tree.hpp"
#include "compressed_value.hpp"
#include "trace_models.hpp"

#include <fstream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class StorageLayer {
public:
    StorageLayer(std::string index_path = "index.db", std::string values_path = "values.db");
    ~StorageLayer();

    void open();
    void upsert(int key, const CompressedValue& value);
    std::vector<std::uint64_t> upsert_with_trace(int key, const CompressedValue& value);
    std::optional<CompressedValue> find(int key);
    std::optional<CompressedValue> find_with_trace(int key,
                                                   std::vector<std::uint64_t>& node_path,
                                                   std::uint64_t& leaf_id);
    std::vector<std::pair<int, CompressedValue>> range(int start, int end);
    std::vector<std::pair<int, CompressedValue>> range_with_trace(int start,
                                                                  int end,
                                                                  std::vector<std::uint64_t>& node_path,
                                                                  std::vector<std::uint64_t>& scanned_leaf_ids);
    GraphSnapshot tree_snapshot(const std::vector<std::uint64_t>& active_node_ids = {},
                                const std::vector<int>& active_keys = {}) const;
    void close();

private:
    std::string index_path_;
    std::string values_path_;
    bool is_open_;
    BPlusTree tree_;
    std::fstream* values_stream_;

    BPlusTree::ValuePointer append_value(const CompressedValue& value);
    CompressedValue read_value(const BPlusTree::ValuePointer& pointer);
};
