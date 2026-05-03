#pragma once

#include "bplus_tree.hpp"
#include "compressed_value.hpp"

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
    std::optional<CompressedValue> find(int key);
    std::vector<std::pair<int, CompressedValue>> range(int start, int end);
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
