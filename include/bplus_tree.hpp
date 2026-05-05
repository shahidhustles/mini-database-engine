#pragma once

#include "trace_models.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class BPlusTree {
public:
    struct ValuePointer {
        std::uint64_t offset = 0;
        std::uint32_t length = 0;
    };

    struct Node {
        bool is_leaf = true;
        std::uint64_t parent = invalid_id();
        std::uint64_t next_leaf = invalid_id();
        std::vector<int> keys;
        std::vector<std::uint64_t> children;
        std::vector<ValuePointer> values;

        static std::uint64_t invalid_id() { return UINT64_MAX; }
    };

    explicit BPlusTree(std::size_t max_keys = 4);

    void clear();
    void upsert(int key, const ValuePointer& value, std::vector<std::uint64_t>* touched_node_ids = nullptr);
    std::optional<ValuePointer> find(int key) const;
    std::optional<ValuePointer> find_with_trace(int key,
                                                std::vector<std::uint64_t>& node_path,
                                                std::uint64_t& leaf_id) const;
    std::vector<std::pair<int, ValuePointer>> range(int start, int end) const;
    std::vector<std::pair<int, ValuePointer>> range_with_trace(int start,
                                                               int end,
                                                               std::vector<std::uint64_t>& node_path,
                                                               std::vector<std::uint64_t>& scanned_leaf_ids) const;
    GraphSnapshot snapshot(const std::vector<std::uint64_t>& active_node_ids = {},
                           const std::vector<int>& active_keys = {}) const;

    void save(const std::string& path) const;
    void load(const std::string& path);

    std::size_t max_keys() const;
    const std::vector<Node>& nodes() const;
    std::uint64_t root_id() const;

private:
    std::size_t max_keys_;
    std::vector<Node> nodes_;
    std::uint64_t root_id_;

    std::uint64_t create_node(bool is_leaf);
    std::uint64_t find_leaf_id(int key) const;
    std::uint64_t find_leaf_id_with_trace(int key, std::vector<std::uint64_t>* node_path) const;
    void insert_into_parent(std::uint64_t left_id,
                            int separator_key,
                            std::uint64_t right_id,
                            std::vector<std::uint64_t>* touched_node_ids);
    void split_leaf(std::uint64_t leaf_id, std::vector<std::uint64_t>* touched_node_ids);
    void split_internal(std::uint64_t node_id, std::vector<std::uint64_t>* touched_node_ids);
};
