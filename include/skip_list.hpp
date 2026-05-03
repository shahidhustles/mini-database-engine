#pragma once

#include "compressed_value.hpp"

#include <cstddef>
#include <optional>
#include <random>
#include <utility>
#include <vector>

class SkipList {
public:
    SkipList();
    ~SkipList();

    void upsert(int key, const CompressedValue& value);
    std::optional<CompressedValue> find(int key) const;
    std::vector<std::pair<int, CompressedValue>> range(int start, int end) const;
    std::vector<std::pair<int, CompressedValue>> snapshot_sorted() const;
    void clear();
    std::size_t size() const;

private:
    struct Node {
        int key;
        CompressedValue value;
        std::vector<Node*> forward;

        Node(int node_key, const CompressedValue& node_value, int level);
    };

    static constexpr int kMaxLevel = 8;
    static constexpr double kProbability = 0.5;

    Node* head_;
    int current_level_;
    std::size_t size_;
    std::mt19937 generator_;
    std::uniform_real_distribution<double> distribution_;

    int random_level();
};
