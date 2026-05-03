#pragma once

#include "compressed_value.hpp"

#include <cstddef>
#include <optional>

class RedBlackTree {
public:
    RedBlackTree();
    ~RedBlackTree();

    void upsert(int key, const CompressedValue& value);
    std::optional<CompressedValue> find(int key) const;
    void clear();
    std::size_t size() const;

private:
    enum class Color { Red, Black };

    struct Node {
        int key;
        CompressedValue value;
        Color color;
        Node* parent;
        Node* left;
        Node* right;

        Node(int node_key, const CompressedValue& node_value);
    };

    Node* root_;
    std::size_t size_;

    Node* find_node(int key) const;
    void rotate_left(Node* node);
    void rotate_right(Node* node);
    void insert_fixup(Node* node);
    void clear_subtree(Node* node);
};
