#pragma once

#include "huffman.hpp"
#include "memory_layer.hpp"
#include "storage_layer.hpp"
#include "trace_models.hpp"

#include <optional>
#include <string>
#include <utility>
#include <vector>

class DatabaseInterface {
public:
    DatabaseInterface(std::string index_path = "index.db", std::string values_path = "values.db");
    ~DatabaseInterface();

    void insert(int key, const std::string& value);
    std::optional<std::string> get(int key);
    std::vector<std::pair<int, std::string>> range(int start, int end);
    void flush();
    void shutdown();
    OperationTrace execute_with_trace(const std::string& raw_command);

private:
    HuffmanCodec codec_;
    MemoryLayer memory_layer_;
    StorageLayer storage_layer_;
    bool shutdown_called_;
};
