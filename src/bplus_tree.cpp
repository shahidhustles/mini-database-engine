#include "bplus_tree.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

using namespace std;

constexpr char kMagic[8] = {'M', 'D', 'B', 'P', 'L', 'U', 'S', '1'};

template <typename T>
void write_pod(ostream& out, T value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(T));
    if (!out) {
        throw runtime_error("Failed to write B+ tree data.");
    }
}

template <typename T>
T read_pod(istream& in) {
    T value{};
    in.read(reinterpret_cast<char*>(&value), sizeof(T));
    if (!in) {
        throw runtime_error("Failed to read B+ tree data.");
    }
    return value;
}

namespace {

string join_keys(const vector<int>& keys) {
    ostringstream out;
    for (size_t i = 0; i < keys.size(); ++i) {
        if (i != 0) {
            out << " | ";
        }
        out << keys[i];
    }
    return out.str();
}

bool contains_key(const vector<int>& haystack, int key) {
    return find(haystack.begin(), haystack.end(), key) != haystack.end();
}

}  // namespace

BPlusTree::BPlusTree(size_t max_keys) : max_keys_(max_keys), root_id_(0) {
    clear();
}

void BPlusTree::clear() {
    nodes_.clear();
    nodes_.push_back(Node{});
    root_id_ = 0;
}

void BPlusTree::upsert(int key, const ValuePointer& value, vector<uint64_t>* touched_node_ids) {
    const auto leaf_id = find_leaf_id_with_trace(key, touched_node_ids);
    if (touched_node_ids != nullptr) {
        touched_node_ids->push_back(leaf_id);
    }

    auto& leaf = nodes_[leaf_id];
    auto it = lower_bound(leaf.keys.begin(), leaf.keys.end(), key);
    const auto index = static_cast<size_t>(distance(leaf.keys.begin(), it));

    if (it != leaf.keys.end() && *it == key) {
        leaf.values[index] = value;
        return;
    }

    leaf.keys.insert(it, key);
    leaf.values.insert(leaf.values.begin() + static_cast<ptrdiff_t>(index), value);

    if (leaf.keys.size() > max_keys_) {
        split_leaf(leaf_id, touched_node_ids);
    }
}

optional<BPlusTree::ValuePointer> BPlusTree::find(int key) const {
    vector<uint64_t> ignored;
    uint64_t leaf_id = Node::invalid_id();
    return find_with_trace(key, ignored, leaf_id);
}

optional<BPlusTree::ValuePointer> BPlusTree::find_with_trace(int key,
                                                             vector<uint64_t>& node_path,
                                                             uint64_t& leaf_id) const {
    node_path.clear();
    leaf_id = find_leaf_id_with_trace(key, &node_path);
    const auto& leaf = nodes_[leaf_id];
    auto it = lower_bound(leaf.keys.begin(), leaf.keys.end(), key);
    if (it == leaf.keys.end() || *it != key) {
        return nullopt;
    }
    const auto index = static_cast<size_t>(distance(leaf.keys.begin(), it));
    return leaf.values[index];
}

vector<pair<int, BPlusTree::ValuePointer>> BPlusTree::range(int start, int end) const {
    vector<uint64_t> ignored_path;
    vector<uint64_t> ignored_leaves;
    return range_with_trace(start, end, ignored_path, ignored_leaves);
}

vector<pair<int, BPlusTree::ValuePointer>> BPlusTree::range_with_trace(
    int start,
    int end,
    vector<uint64_t>& node_path,
    vector<uint64_t>& scanned_leaf_ids) const {
    vector<pair<int, ValuePointer>> results;
    node_path.clear();
    scanned_leaf_ids.clear();

    if (start > end) {
        swap(start, end);
    }
    if (nodes_.empty()) {
        return results;
    }

    uint64_t leaf_id = find_leaf_id_with_trace(start, &node_path);
    while (leaf_id != Node::invalid_id()) {
        scanned_leaf_ids.push_back(leaf_id);
        const auto& leaf = nodes_[leaf_id];
        for (size_t i = 0; i < leaf.keys.size(); ++i) {
            const int key = leaf.keys[i];
            if (key < start) {
                continue;
            }
            if (key > end) {
                return results;
            }
            results.emplace_back(key, leaf.values[i]);
        }
        leaf_id = leaf.next_leaf;
    }
    return results;
}

GraphSnapshot BPlusTree::snapshot(const vector<uint64_t>& active_node_ids,
                                  const vector<int>& active_keys) const {
    GraphSnapshot snapshot;
    snapshot.layout = "preset";
    if (nodes_.empty()) {
        return snapshot;
    }

    unordered_set<uint64_t> active_nodes(active_node_ids.begin(), active_node_ids.end());
    unordered_map<uint64_t, int> depth_map;
    unordered_map<int, int> order_per_depth;
    queue<uint64_t> pending;
    pending.push(root_id_);
    depth_map[root_id_] = 0;

    while (!pending.empty()) {
        const auto node_id = pending.front();
        pending.pop();
        const auto& node = nodes_[node_id];
        if (!node.is_leaf) {
            for (auto child : node.children) {
                depth_map[child] = depth_map[node_id] + 1;
                pending.push(child);
            }
        }
    }

    for (size_t node_id = 0; node_id < nodes_.size(); ++node_id) {
        const auto& node = nodes_[node_id];
        const int depth = depth_map.count(node_id) != 0 ? depth_map[node_id] : 0;
        const int order = order_per_depth[depth]++;

        GraphNode graph_node;
        graph_node.id = "bplus-" + to_string(node_id);
        graph_node.label = node.keys.empty() ? "(empty)" : join_keys(node.keys);
        graph_node.group = node.is_leaf ? "bplus-leaf" : "bplus-internal";
        graph_node.x = order * 180.0;
        graph_node.y = depth * 120.0;
        graph_node.active = active_nodes.count(node_id) != 0;
        graph_node.highlighted = any_of(node.keys.begin(),
                                        node.keys.end(),
                                        [&](int key) { return contains_key(active_keys, key); });
        if (node.is_leaf && node.next_leaf != Node::invalid_id()) {
            graph_node.note = "leaf";
        }
        snapshot.nodes.push_back(graph_node);
    }

    for (size_t node_id = 0; node_id < nodes_.size(); ++node_id) {
        const auto& node = nodes_[node_id];
        if (!node.is_leaf) {
            for (auto child : node.children) {
                GraphEdge edge;
                edge.id = "bplus-edge-" + to_string(node_id) + "-" + to_string(child);
                edge.source = "bplus-" + to_string(node_id);
                edge.target = "bplus-" + to_string(child);
                edge.highlighted = active_nodes.count(node_id) != 0 && active_nodes.count(child) != 0;
                snapshot.edges.push_back(edge);
            }
        } else if (node.next_leaf != Node::invalid_id()) {
            GraphEdge edge;
            edge.id = "bplus-leaf-" + to_string(node_id) + "-next";
            edge.source = "bplus-" + to_string(node_id);
            edge.target = "bplus-" + to_string(node.next_leaf);
            edge.label = "next";
            edge.dashed = true;
            edge.highlighted = active_nodes.count(node_id) != 0 &&
                               active_nodes.count(node.next_leaf) != 0;
            snapshot.edges.push_back(edge);
        }
    }

    return snapshot;
}

void BPlusTree::save(const string& path) const {
    ofstream out(path, ios::binary | ios::trunc);
    if (!out) {
        throw runtime_error("Failed to open index file for writing.");
    }

    out.write(kMagic, sizeof(kMagic));
    write_pod<uint32_t>(out, 1);
    write_pod<uint32_t>(out, static_cast<uint32_t>(max_keys_));
    write_pod<uint64_t>(out, root_id_);
    write_pod<uint64_t>(out, static_cast<uint64_t>(nodes_.size()));

    for (const auto& node : nodes_) {
        write_pod<uint8_t>(out, node.is_leaf ? 1 : 0);
        write_pod<uint64_t>(out, node.parent);
        write_pod<uint64_t>(out, node.next_leaf);
        write_pod<uint32_t>(out, static_cast<uint32_t>(node.keys.size()));
        for (int key : node.keys) {
            write_pod<int32_t>(out, key);
        }

        if (node.is_leaf) {
            write_pod<uint32_t>(out, static_cast<uint32_t>(node.values.size()));
            for (const auto& value : node.values) {
                write_pod<uint64_t>(out, value.offset);
                write_pod<uint32_t>(out, value.length);
            }
        } else {
            write_pod<uint32_t>(out, static_cast<uint32_t>(node.children.size()));
            for (uint64_t child : node.children) {
                write_pod<uint64_t>(out, child);
            }
        }
    }
}

void BPlusTree::load(const string& path) {
    if (!filesystem::exists(path) || filesystem::is_empty(path)) {
        clear();
        return;
    }

    ifstream in(path, ios::binary);
    if (!in) {
        throw runtime_error("Failed to open index file for reading.");
    }

    char magic[sizeof(kMagic)]{};
    in.read(magic, sizeof(magic));
    if (!in) {
        throw runtime_error("Invalid B+ tree index header.");
    }
    for (size_t i = 0; i < sizeof(kMagic); ++i) {
        if (magic[i] != kMagic[i]) {
            throw runtime_error("Invalid B+ tree index header.");
        }
    }

    const auto version = read_pod<uint32_t>(in);
    if (version != 1) {
        throw runtime_error("Unsupported B+ tree index version.");
    }

    max_keys_ = read_pod<uint32_t>(in);
    root_id_ = read_pod<uint64_t>(in);
    const auto node_count = read_pod<uint64_t>(in);

    nodes_.clear();
    nodes_.reserve(node_count);

    for (uint64_t i = 0; i < node_count; ++i) {
        Node node;
        node.is_leaf = read_pod<uint8_t>(in) != 0;
        node.parent = read_pod<uint64_t>(in);
        node.next_leaf = read_pod<uint64_t>(in);
        const auto key_count = read_pod<uint32_t>(in);
        node.keys.reserve(key_count);
        for (uint32_t j = 0; j < key_count; ++j) {
            node.keys.push_back(read_pod<int32_t>(in));
        }

        if (node.is_leaf) {
            const auto value_count = read_pod<uint32_t>(in);
            node.values.reserve(value_count);
            for (uint32_t j = 0; j < value_count; ++j) {
                ValuePointer pointer;
                pointer.offset = read_pod<uint64_t>(in);
                pointer.length = read_pod<uint32_t>(in);
                node.values.push_back(pointer);
            }
        } else {
            const auto child_count = read_pod<uint32_t>(in);
            node.children.reserve(child_count);
            for (uint32_t j = 0; j < child_count; ++j) {
                node.children.push_back(read_pod<uint64_t>(in));
            }
        }

        nodes_.push_back(move(node));
    }

    if (nodes_.empty()) {
        clear();
    }
}

size_t BPlusTree::max_keys() const {
    return max_keys_;
}

const vector<BPlusTree::Node>& BPlusTree::nodes() const {
    return nodes_;
}

uint64_t BPlusTree::root_id() const {
    return root_id_;
}

uint64_t BPlusTree::create_node(bool is_leaf) {
    Node node;
    node.is_leaf = is_leaf;
    nodes_.push_back(node);
    return nodes_.size() - 1;
}

uint64_t BPlusTree::find_leaf_id(int key) const {
    return find_leaf_id_with_trace(key, nullptr);
}

uint64_t BPlusTree::find_leaf_id_with_trace(int key, vector<uint64_t>* node_path) const {
    uint64_t current_id = root_id_;
    while (true) {
        if (node_path != nullptr) {
            node_path->push_back(current_id);
        }
        if (nodes_[current_id].is_leaf) {
            return current_id;
        }

        const auto& node = nodes_[current_id];
        size_t child_index = 0;
        while (child_index < node.keys.size() && key >= node.keys[child_index]) {
            ++child_index;
        }
        current_id = node.children[child_index];
    }
}

void BPlusTree::insert_into_parent(uint64_t left_id,
                                   int separator_key,
                                   uint64_t right_id,
                                   vector<uint64_t>* touched_node_ids) {
    const auto parent_id = nodes_[left_id].parent;
    if (parent_id == Node::invalid_id()) {
        const auto new_root_id = create_node(false);
        auto& root = nodes_[new_root_id];
        root.keys.push_back(separator_key);
        root.children.push_back(left_id);
        root.children.push_back(right_id);
        nodes_[left_id].parent = new_root_id;
        nodes_[right_id].parent = new_root_id;
        root_id_ = new_root_id;
        if (touched_node_ids != nullptr) {
            touched_node_ids->push_back(new_root_id);
            touched_node_ids->push_back(left_id);
            touched_node_ids->push_back(right_id);
        }
        return;
    }

    auto& parent = nodes_[parent_id];
    size_t child_index = 0;
    for (size_t i = 0; i < parent.children.size(); ++i) {
        if (parent.children[i] == left_id) {
            child_index = i;
            break;
        }
    }
    parent.keys.insert(parent.keys.begin() + static_cast<ptrdiff_t>(child_index), separator_key);
    parent.children.insert(parent.children.begin() + static_cast<ptrdiff_t>(child_index + 1), right_id);
    nodes_[right_id].parent = parent_id;
    if (touched_node_ids != nullptr) {
        touched_node_ids->push_back(parent_id);
        touched_node_ids->push_back(right_id);
    }

    if (parent.keys.size() > max_keys_) {
        split_internal(parent_id, touched_node_ids);
    }
}

void BPlusTree::split_leaf(uint64_t leaf_id, vector<uint64_t>* touched_node_ids) {
    const size_t split_index = (nodes_[leaf_id].keys.size() + 1) / 2;
    const auto parent_id = nodes_[leaf_id].parent;
    const auto old_next_leaf = nodes_[leaf_id].next_leaf;
    vector<int> right_keys(nodes_[leaf_id].keys.begin() + static_cast<ptrdiff_t>(split_index),
                           nodes_[leaf_id].keys.end());
    vector<ValuePointer> right_values(
        nodes_[leaf_id].values.begin() + static_cast<ptrdiff_t>(split_index), nodes_[leaf_id].values.end());

    const auto new_leaf_id = create_node(true);
    auto& leaf = nodes_[leaf_id];
    leaf.keys.erase(leaf.keys.begin() + static_cast<ptrdiff_t>(split_index), leaf.keys.end());
    leaf.values.erase(leaf.values.begin() + static_cast<ptrdiff_t>(split_index), leaf.values.end());
    leaf.next_leaf = new_leaf_id;

    auto& new_leaf = nodes_[new_leaf_id];
    new_leaf.parent = parent_id;
    new_leaf.next_leaf = old_next_leaf;
    new_leaf.keys = move(right_keys);
    new_leaf.values = move(right_values);

    if (touched_node_ids != nullptr) {
        touched_node_ids->push_back(leaf_id);
        touched_node_ids->push_back(new_leaf_id);
    }
    insert_into_parent(leaf_id, new_leaf.keys.front(), new_leaf_id, touched_node_ids);
}

void BPlusTree::split_internal(uint64_t node_id, vector<uint64_t>* touched_node_ids) {
    const auto parent_id = nodes_[node_id].parent;
    const size_t middle_index = nodes_[node_id].keys.size() / 2;
    const int promoted_key = nodes_[node_id].keys[middle_index];

    vector<int> right_keys(nodes_[node_id].keys.begin() + static_cast<ptrdiff_t>(middle_index + 1),
                           nodes_[node_id].keys.end());
    vector<uint64_t> right_children(nodes_[node_id].children.begin() + static_cast<ptrdiff_t>(middle_index + 1),
                                    nodes_[node_id].children.end());

    const auto new_node_id = create_node(false);
    auto& node = nodes_[node_id];
    node.keys.erase(node.keys.begin() + static_cast<ptrdiff_t>(middle_index), node.keys.end());
    node.children.erase(node.children.begin() + static_cast<ptrdiff_t>(middle_index + 1), node.children.end());

    auto& new_node = nodes_[new_node_id];
    new_node.is_leaf = false;
    new_node.parent = parent_id;
    new_node.keys = move(right_keys);
    new_node.children = move(right_children);

    for (auto child_id : new_node.children) {
        nodes_[child_id].parent = new_node_id;
    }
    if (touched_node_ids != nullptr) {
        touched_node_ids->push_back(node_id);
        touched_node_ids->push_back(new_node_id);
        for (auto child_id : new_node.children) {
            touched_node_ids->push_back(child_id);
        }
    }

    insert_into_parent(node_id, promoted_key, new_node_id, touched_node_ids);
}
