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
    Node* current = head_;
    for (int level = current_level_; level >= 0; --level) {
        while (current->forward[level] != nullptr && current->forward[level]->key < key) {
            current = current->forward[level];
        }
    }

    current = current->forward[0];
    if (current != nullptr && current->key == key) {
        return current->value;
    }
    return nullopt;
}

vector<pair<int, CompressedValue>> SkipList::range(int start, int end) const {
    vector<pair<int, CompressedValue>> results;
    if (start > end) {
        swap(start, end);
    }

    Node* current = head_;
    for (int level = current_level_; level >= 0; --level) {
        while (current->forward[level] != nullptr && current->forward[level]->key < start) {
            current = current->forward[level];
        }
    }

    current = current->forward[0];
    while (current != nullptr && current->key <= end) {
        results.push_back(make_pair(current->key, current->value));
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
