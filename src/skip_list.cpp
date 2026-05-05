#include "skip_list.hpp"

#include <algorithm>

using namespace std;

SkipList::Node::Node(int node_key, const CompressedValue& node_value, int level)
    : key(node_key), value(node_value), forward(level + 1, nullptr) {}

SkipList::SkipList()
    : head_(new Node(0, CompressedValue{}, kMaxLevel)),
      current_level_(0),
      size_(0),
      generator_(random_device{}()),
      distribution_(0.0, 1.0) {}

SkipList::~SkipList() {
    clear();
    delete head_;
}

void SkipList::upsert(int key, const CompressedValue& value) {
    vector<Node*> update(kMaxLevel + 1, nullptr);
    Node* current = head_;

    for (int level = current_level_; level >= 0; --level) {
        while (current->forward[level] != nullptr && current->forward[level]->key < key) {
            current = current->forward[level];
        }
        update[level] = current;
    }

    current = current->forward[0];
    if (current != nullptr && current->key == key) {
        current->value = value;
        return;
    }

    const int node_level = random_level();
    if (node_level > current_level_) {
        for (int level = current_level_ + 1; level <= node_level; ++level) {
            update[level] = head_;
        }
        current_level_ = node_level;
    }

    Node* node = new Node(key, value, node_level);
    for (int level = 0; level <= node_level; ++level) {
        node->forward[level] = update[level]->forward[level];
        update[level]->forward[level] = node;
    }
    ++size_;
}

optional<CompressedValue> SkipList::find(int key) const {
    vector<int> ignored;
    return find_with_trace(key, ignored);
}

optional<CompressedValue> SkipList::find_with_trace(int key, vector<int>& visited_keys) const {
    visited_keys.clear();
    Node* current = head_;
    for (int level = current_level_; level >= 0; --level) {
        while (current->forward[level] != nullptr && current->forward[level]->key < key) {
            current = current->forward[level];
            visited_keys.push_back(current->key);
        }
    }

    current = current->forward[0];
    if (current != nullptr) {
        visited_keys.push_back(current->key);
    }
    if (current != nullptr && current->key == key) {
        return current->value;
    }
    return nullopt;
}

vector<pair<int, CompressedValue>> SkipList::range(int start, int end) const {
    vector<int> ignored;
    vector<int> included;
    return range_with_trace(start, end, ignored, included);
}

vector<pair<int, CompressedValue>> SkipList::range_with_trace(int start,
                                                              int end,
                                                              vector<int>& visited_keys,
                                                              vector<int>& included_keys) const {
    vector<pair<int, CompressedValue>> results;
    visited_keys.clear();
    included_keys.clear();
    if (start > end) {
        swap(start, end);
    }

    Node* current = head_;
    for (int level = current_level_; level >= 0; --level) {
        while (current->forward[level] != nullptr && current->forward[level]->key < start) {
            current = current->forward[level];
            visited_keys.push_back(current->key);
        }
    }

    current = current->forward[0];
    while (current != nullptr && current->key <= end) {
        results.push_back(make_pair(current->key, current->value));
        included_keys.push_back(current->key);
        current = current->forward[0];
    }
    return results;
}

vector<pair<int, CompressedValue>> SkipList::snapshot_sorted() const {
    vector<pair<int, CompressedValue>> results;
    results.reserve(size_);
    Node* current = head_->forward[0];
    while (current != nullptr) {
        results.push_back(make_pair(current->key, current->value));
        current = current->forward[0];
    }
    return results;
}

GraphSnapshot SkipList::snapshot(const vector<int>& active_keys) const {
    GraphSnapshot snapshot;
    snapshot.layout = "preset";

    vector<Node*> base_nodes;
    Node* current = head_->forward[0];
    while (current != nullptr) {
        base_nodes.push_back(current);
        current = current->forward[0];
    }

    for (int level = current_level_; level >= 0; --level) {
        double x = 0.0;
        for (Node* walker = head_->forward[level]; walker != nullptr; walker = walker->forward[level]) {
            GraphNode node;
            node.id = "skip-" + to_string(walker->key) + "-L" + to_string(level);
            node.label = to_string(walker->key);
            node.group = "skip-node";
            node.x = x * 120.0;
            node.y = (current_level_ - level) * 72.0;
            node.highlighted = std::find(active_keys.begin(), active_keys.end(), walker->key) != active_keys.end();
            node.active = node.highlighted;
            node.note = "L" + to_string(level);
            snapshot.nodes.push_back(node);

            if (walker->forward[level] != nullptr) {
                GraphEdge edge;
                edge.id = node.id + "-next";
                edge.source = node.id;
                edge.target = "skip-" + to_string(walker->forward[level]->key) + "-L" + to_string(level);
                edge.highlighted = node.highlighted &&
                                   std::find(active_keys.begin(), active_keys.end(), walker->forward[level]->key) != active_keys.end();
                snapshot.edges.push_back(edge);
            }
            x += 1.0;
        }
    }

    for (Node* walker : base_nodes) {
        for (size_t level = 1; level < walker->forward.size(); ++level) {
            GraphEdge edge;
            edge.id = "skip-" + to_string(walker->key) + "-tower-" + to_string(level);
            edge.source = "skip-" + to_string(walker->key) + "-L" + to_string(static_cast<int>(level - 1));
            edge.target = "skip-" + to_string(walker->key) + "-L" + to_string(static_cast<int>(level));
            edge.dashed = true;
            edge.highlighted = std::find(active_keys.begin(), active_keys.end(), walker->key) != active_keys.end();
            snapshot.edges.push_back(edge);
        }
    }

    return snapshot;
}

void SkipList::clear() {
    Node* current = head_->forward[0];
    while (current != nullptr) {
        Node* next = current->forward[0];
        delete current;
        current = next;
    }
    for (size_t i = 0; i < head_->forward.size(); ++i) {
        head_->forward[i] = nullptr;
    }
    current_level_ = 0;
    size_ = 0;
}

size_t SkipList::size() const {
    return size_;
}

int SkipList::random_level() {
    int level = 0;
    while (level < kMaxLevel && distribution_(generator_) < kProbability) {
        ++level;
    }
    return level;
}
