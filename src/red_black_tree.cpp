#include "red_black_tree.hpp"

#include <algorithm>
#include <stdexcept>

RedBlackTree::Node::Node(int node_key, const CompressedValue& node_value)
    : key(node_key),
      value(node_value),
      color(Color::Red),
      parent(nullptr),
      left(nullptr),
      right(nullptr) {}

RedBlackTree::RedBlackTree() : root_(nullptr), size_(0) {}

RedBlackTree::~RedBlackTree() {
    clear();
}

void RedBlackTree::upsert(int key, const CompressedValue& value) {
    Node* parent = nullptr;
    Node* current = root_;

    while (current != nullptr) {
        parent = current;
        if (key == current->key) {
            current->value = value;
            return;
        }
        current = key < current->key ? current->left : current->right;
    }

    auto* node = new Node(key, value);
    node->parent = parent;

    if (parent == nullptr) {
        root_ = node;
    } else if (key < parent->key) {
        parent->left = node;
    } else {
        parent->right = node;
    }

    ++size_;
    insert_fixup(node);
}

std::optional<CompressedValue> RedBlackTree::find(int key) const {
    Node* node = find_node(key);
    if (node == nullptr) {
        return std::nullopt;
    }
    return node->value;
}

std::optional<CompressedValue> RedBlackTree::find_with_path(int key, std::vector<int>& path) const {
    path.clear();
    Node* node = find_node_with_path(key, &path);
    if (node == nullptr) {
        return std::nullopt;
    }
    return node->value;
}

GraphSnapshot RedBlackTree::snapshot(const std::vector<int>& active_keys,
                                     std::optional<int> highlighted_key) const {
    GraphSnapshot snapshot;
    snapshot.layout = "preset";
    double next_x = 0.0;
    snapshot_subtree(root_, snapshot, active_keys, highlighted_key, 0, next_x);
    return snapshot;
}

void RedBlackTree::clear() {
    clear_subtree(root_);
    root_ = nullptr;
    size_ = 0;
}

std::size_t RedBlackTree::size() const {
    return size_;
}

RedBlackTree::Node* RedBlackTree::find_node(int key) const {
    return find_node_with_path(key, nullptr);
}

RedBlackTree::Node* RedBlackTree::find_node_with_path(int key, std::vector<int>* path) const {
    Node* current = root_;
    while (current != nullptr) {
        if (path != nullptr) {
            path->push_back(current->key);
        }
        if (key == current->key) {
            return current;
        }
        current = key < current->key ? current->left : current->right;
    }
    return nullptr;
}

void RedBlackTree::rotate_left(Node* node) {
    Node* pivot = node->right;
    if (pivot == nullptr) {
        throw std::runtime_error("Invalid left rotation.");
    }

    node->right = pivot->left;
    if (pivot->left != nullptr) {
        pivot->left->parent = node;
    }

    pivot->parent = node->parent;
    if (node->parent == nullptr) {
        root_ = pivot;
    } else if (node == node->parent->left) {
        node->parent->left = pivot;
    } else {
        node->parent->right = pivot;
    }

    pivot->left = node;
    node->parent = pivot;
}

void RedBlackTree::rotate_right(Node* node) {
    Node* pivot = node->left;
    if (pivot == nullptr) {
        throw std::runtime_error("Invalid right rotation.");
    }

    node->left = pivot->right;
    if (pivot->right != nullptr) {
        pivot->right->parent = node;
    }

    pivot->parent = node->parent;
    if (node->parent == nullptr) {
        root_ = pivot;
    } else if (node == node->parent->right) {
        node->parent->right = pivot;
    } else {
        node->parent->left = pivot;
    }

    pivot->right = node;
    node->parent = pivot;
}

void RedBlackTree::insert_fixup(Node* node) {
    while (node != root_ && node->parent->color == Color::Red) {
        Node* parent = node->parent;
        Node* grandparent = parent->parent;
        if (grandparent == nullptr) {
            break;
        }

        if (parent == grandparent->left) {
            Node* uncle = grandparent->right;
            if (uncle != nullptr && uncle->color == Color::Red) {
                parent->color = Color::Black;
                uncle->color = Color::Black;
                grandparent->color = Color::Red;
                node = grandparent;
            } else {
                if (node == parent->right) {
                    node = parent;
                    rotate_left(node);
                    parent = node->parent;
                    grandparent = parent != nullptr ? parent->parent : nullptr;
                }
                if (parent != nullptr) {
                    parent->color = Color::Black;
                }
                if (grandparent != nullptr) {
                    grandparent->color = Color::Red;
                    rotate_right(grandparent);
                }
            }
        } else {
            Node* uncle = grandparent->left;
            if (uncle != nullptr && uncle->color == Color::Red) {
                parent->color = Color::Black;
                uncle->color = Color::Black;
                grandparent->color = Color::Red;
                node = grandparent;
            } else {
                if (node == parent->left) {
                    node = parent;
                    rotate_right(node);
                    parent = node->parent;
                    grandparent = parent != nullptr ? parent->parent : nullptr;
                }
                if (parent != nullptr) {
                    parent->color = Color::Black;
                }
                if (grandparent != nullptr) {
                    grandparent->color = Color::Red;
                    rotate_left(grandparent);
                }
            }
        }
    }

    if (root_ != nullptr) {
        root_->color = Color::Black;
    }
}

void RedBlackTree::clear_subtree(Node* node) {
    if (node == nullptr) {
        return;
    }
    clear_subtree(node->left);
    clear_subtree(node->right);
    delete node;
}

void RedBlackTree::snapshot_subtree(Node* node,
                                    GraphSnapshot& snapshot,
                                    const std::vector<int>& active_keys,
                                    std::optional<int> highlighted_key,
                                    int depth,
                                    double& next_x) const {
    if (node == nullptr) {
        return;
    }

    snapshot_subtree(node->left, snapshot, active_keys, highlighted_key, depth + 1, next_x);

    GraphNode graph_node;
    graph_node.id = "rb-" + std::to_string(node->key);
    graph_node.label = std::to_string(node->key) + (node->color == Color::Red ? " (R)" : " (B)");
    graph_node.group = node->color == Color::Red ? "rb-red" : "rb-black";
    graph_node.x = next_x * 120.0;
    graph_node.y = depth * 96.0;
    graph_node.highlighted = highlighted_key.has_value() && highlighted_key.value() == node->key;
    graph_node.active =
        std::find(active_keys.begin(), active_keys.end(), node->key) != active_keys.end();
    snapshot.nodes.push_back(graph_node);

    if (node->left != nullptr) {
        GraphEdge edge;
        edge.id = graph_node.id + "-L";
        edge.source = graph_node.id;
        edge.target = "rb-" + std::to_string(node->left->key);
        edge.label = "L";
        edge.highlighted = graph_node.active &&
                           std::find(active_keys.begin(), active_keys.end(), node->left->key) != active_keys.end();
        snapshot.edges.push_back(edge);
    }
    if (node->right != nullptr) {
        GraphEdge edge;
        edge.id = graph_node.id + "-R";
        edge.source = graph_node.id;
        edge.target = "rb-" + std::to_string(node->right->key);
        edge.label = "R";
        edge.highlighted = graph_node.active &&
                           std::find(active_keys.begin(), active_keys.end(), node->right->key) != active_keys.end();
        snapshot.edges.push_back(edge);
    }

    next_x += 1.0;
    snapshot_subtree(node->right, snapshot, active_keys, highlighted_key, depth + 1, next_x);
}
