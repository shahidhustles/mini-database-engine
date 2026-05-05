#include "huffman.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <istream>
#include <memory>
#include <ostream>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

using namespace std;

namespace {

struct HuffmanNode {
    string id;
    uint8_t symbol = 0;
    uint32_t frequency = 0;
    bool is_leaf = false;
    shared_ptr<HuffmanNode> left;
    shared_ptr<HuffmanNode> right;
};

struct NodeCompare {
    bool operator()(const shared_ptr<HuffmanNode>& lhs, const shared_ptr<HuffmanNode>& rhs) const {
        if (lhs->frequency == rhs->frequency) {
            return lhs->id > rhs->id;
        }
        return lhs->frequency > rhs->frequency;
    }
};

template <typename T>
void write_pod(ostream& out, T value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(T));
    if (!out) {
        throw runtime_error("Failed to write binary data.");
    }
}

template <typename T>
T read_pod(istream& in) {
    T value{};
    in.read(reinterpret_cast<char*>(&value), sizeof(T));
    if (!in) {
        throw runtime_error("Failed to read binary data.");
    }
    return value;
}

string describe_symbol(uint8_t symbol) {
    if (symbol == ' ') {
        return "space";
    }
    if (symbol == '\n') {
        return "\\n";
    }
    if (symbol >= 32 && symbol <= 126) {
        return string(1, static_cast<char>(symbol));
    }
    return "0x" + string(1, "0123456789ABCDEF"[symbol >> 4]) +
           string(1, "0123456789ABCDEF"[symbol & 0x0F]);
}

void build_codes(const shared_ptr<HuffmanNode>& node,
                 const string& prefix,
                 unordered_map<uint8_t, string>& codes) {
    if (!node) {
        return;
    }
    if (node->is_leaf) {
        codes[node->symbol] = prefix.empty() ? "0" : prefix;
        return;
    }
    build_codes(node->left, prefix + "0", codes);
    build_codes(node->right, prefix + "1", codes);
}

GraphSnapshot build_tree_snapshot(const shared_ptr<HuffmanNode>& root) {
    GraphSnapshot snapshot;
    snapshot.layout = "breadthfirst";
    if (!root) {
        return snapshot;
    }

    queue<pair<shared_ptr<HuffmanNode>, int>> pending;
    pending.push({root, 0});

    while (!pending.empty()) {
        auto current = pending.front();
        pending.pop();

        GraphNode node;
        node.id = current.first->id;
        node.group = current.first->is_leaf ? "huffman-leaf" : "huffman-merge";
        if (current.first->is_leaf) {
            node.label = describe_symbol(current.first->symbol) + " : " + to_string(current.first->frequency);
        } else {
            node.label = to_string(current.first->frequency);
        }
        snapshot.nodes.push_back(node);

        if (current.first->left) {
            GraphEdge edge;
            edge.id = current.first->id + "-L";
            edge.source = current.first->id;
            edge.target = current.first->left->id;
            edge.label = "0";
            snapshot.edges.push_back(edge);
            pending.push({current.first->left, current.second + 1});
        }
        if (current.first->right) {
            GraphEdge edge;
            edge.id = current.first->id + "-R";
            edge.source = current.first->id;
            edge.target = current.first->right->id;
            edge.label = "1";
            snapshot.edges.push_back(edge);
            pending.push({current.first->right, current.second + 1});
        }
    }

    return snapshot;
}

string bytes_to_hex(const vector<uint8_t>& bytes) {
    if (bytes.empty()) {
        return "";
    }
    ostringstream out;
    out << hex << uppercase << setfill('0');
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (i != 0) {
            out << ' ';
        }
        out << setw(2) << static_cast<int>(bytes[i]);
    }
    return out.str();
}

pair<shared_ptr<HuffmanNode>, TableSnapshot> build_tree_with_trace(
    const vector<pair<uint8_t, uint32_t>>& frequencies) {
    TableSnapshot merge_table;
    merge_table.columns = {"Step", "Left", "Right", "Merged"};

    if (frequencies.empty()) {
        return {nullptr, merge_table};
    }

    priority_queue<shared_ptr<HuffmanNode>,
                   vector<shared_ptr<HuffmanNode>>,
                   NodeCompare>
        min_heap;

    int next_id = 0;
    for (const auto& item : frequencies) {
        auto node = make_shared<HuffmanNode>();
        node->id = "huff-" + to_string(next_id++);
        node->symbol = item.first;
        node->frequency = item.second;
        node->is_leaf = true;
        min_heap.push(node);
    }

    if (min_heap.size() == 1) {
        auto only = min_heap.top();
        min_heap.pop();

        auto parent = make_shared<HuffmanNode>();
        parent->id = "huff-" + to_string(next_id++);
        parent->frequency = only->frequency;
        parent->left = only;
        parent->is_leaf = false;

        TableRow row;
        row.cells = {"1", describe_symbol(only->symbol) + " (" + to_string(only->frequency) + ")", "-", to_string(parent->frequency)};
        merge_table.rows.push_back(row);
        return {parent, merge_table};
    }

    int step = 1;
    while (min_heap.size() > 1) {
        auto left = min_heap.top();
        min_heap.pop();
        auto right = min_heap.top();
        min_heap.pop();

        auto parent = make_shared<HuffmanNode>();
        parent->id = "huff-" + to_string(next_id++);
        parent->frequency = left->frequency + right->frequency;
        parent->left = left;
        parent->right = right;
        parent->is_leaf = false;

        TableRow row;
        row.cells = {to_string(step++),
                     (left->is_leaf ? describe_symbol(left->symbol) : string("merge")) + " (" +
                         to_string(left->frequency) + ")",
                     (right->is_leaf ? describe_symbol(right->symbol) : string("merge")) + " (" +
                         to_string(right->frequency) + ")",
                     to_string(parent->frequency)};
        merge_table.rows.push_back(row);

        min_heap.push(parent);
    }

    return {min_heap.top(), merge_table};
}

}  // namespace

uint32_t CompressedValue::metadata_size() const {
    return static_cast<uint32_t>(frequencies.size() * (sizeof(uint8_t) + sizeof(uint32_t)));
}

uint32_t CompressedValue::serialized_size() const {
    return static_cast<uint32_t>(sizeof(uint32_t) + metadata_size() + sizeof(uint32_t) +
                                 compressed_bytes.size() + sizeof(uint64_t));
}

void CompressedValue::write(ostream& out) const {
    write_pod<uint32_t>(out, metadata_size());
    for (const auto& item : frequencies) {
        write_pod<uint8_t>(out, item.first);
        write_pod<uint32_t>(out, item.second);
    }
    write_pod<uint32_t>(out, static_cast<uint32_t>(compressed_bytes.size()));
    if (!compressed_bytes.empty()) {
        out.write(reinterpret_cast<const char*>(compressed_bytes.data()),
                  static_cast<streamsize>(compressed_bytes.size()));
        if (!out) {
            throw runtime_error("Failed to write compressed payload.");
        }
    }
    write_pod<uint64_t>(out, bit_count);
}

CompressedValue CompressedValue::read(istream& in) {
    CompressedValue value;

    const auto metadata_bytes = read_pod<uint32_t>(in);
    if (metadata_bytes % (sizeof(uint8_t) + sizeof(uint32_t)) != 0) {
        throw runtime_error("Corrupted compressed metadata.");
    }

    const uint32_t entry_count =
        metadata_bytes / static_cast<uint32_t>(sizeof(uint8_t) + sizeof(uint32_t));
    value.frequencies.reserve(entry_count);
    for (uint32_t i = 0; i < entry_count; ++i) {
        const auto symbol = read_pod<uint8_t>(in);
        const auto frequency = read_pod<uint32_t>(in);
        value.frequencies.push_back(make_pair(symbol, frequency));
    }

    const auto payload_size = read_pod<uint32_t>(in);
    value.compressed_bytes.resize(payload_size);
    if (payload_size > 0) {
        in.read(reinterpret_cast<char*>(value.compressed_bytes.data()), payload_size);
        if (!in) {
            throw runtime_error("Failed to read compressed payload.");
        }
    }
    value.bit_count = read_pod<uint64_t>(in);
    return value;
}

CompressedValue HuffmanCodec::compress(const string& text) const {
    return compress_with_trace(text).first;
}

pair<CompressedValue, CompressionTrace> HuffmanCodec::compress_with_trace(const string& text) const {
    CompressedValue result;
    CompressionTrace trace;
    trace.frequency_table.columns = {"Symbol", "Frequency"};
    trace.code_table.columns = {"Symbol", "Code"};

    if (text.empty()) {
        return {result, trace};
    }

    array<uint32_t, 256> frequencies{};
    for (unsigned char ch : text) {
        ++frequencies[ch];
    }

    for (size_t i = 0; i < frequencies.size(); ++i) {
        if (frequencies[i] != 0) {
            result.frequencies.push_back(make_pair(static_cast<uint8_t>(i), frequencies[i]));
            TableRow row;
            row.cells = {describe_symbol(static_cast<uint8_t>(i)), to_string(frequencies[i])};
            trace.frequency_table.rows.push_back(row);
        }
    }

    sort(result.frequencies.begin(),
         result.frequencies.end(),
         [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

    auto tree_and_trace = build_tree_with_trace(result.frequencies);
    auto tree = tree_and_trace.first;
    trace.heap_table = tree_and_trace.second;

    unordered_map<uint8_t, string> codes;
    build_codes(tree, "", codes);
    for (const auto& item : result.frequencies) {
        TableRow row;
        row.cells = {describe_symbol(item.first), codes[item.first]};
        trace.code_table.rows.push_back(row);
    }

    string bits;
    bits.reserve(text.size() * 2);
    for (unsigned char ch : text) {
        bits += codes[ch];
    }

    result.bit_count = bits.size();
    result.compressed_bytes.assign((bits.size() + 7) / 8, 0);
    for (size_t i = 0; i < bits.size(); ++i) {
        if (bits[i] == '1') {
            result.compressed_bytes[i / 8] |= static_cast<uint8_t>(1U << (7 - (i % 8)));
        }
    }

    trace.tree_snapshot = build_tree_snapshot(tree);
    trace.bit_string = bits;
    trace.compressed_bytes_hex = bytes_to_hex(result.compressed_bytes);
    trace.bit_count = result.bit_count;

    return {result, trace};
}

string HuffmanCodec::decompress(const CompressedValue& value) const {
    if (value.bit_count == 0) {
        return "";
    }

    auto tree = build_tree_with_trace(value.frequencies).first;
    if (!tree) {
        throw runtime_error("Cannot decompress without Huffman metadata.");
    }

    if (tree->left && !tree->right && tree->left->is_leaf) {
        return string(static_cast<size_t>(tree->left->frequency), static_cast<char>(tree->left->symbol));
    }

    string output;
    output.reserve(value.bit_count);
    auto current = tree;
    for (uint64_t bit_index = 0; bit_index < value.bit_count; ++bit_index) {
        const auto byte = value.compressed_bytes[bit_index / 8];
        const bool is_one = ((byte >> (7 - (bit_index % 8))) & 0x1U) != 0;
        current = is_one ? current->right : current->left;
        if (!current) {
            throw runtime_error("Corrupted Huffman bit stream.");
        }
        if (current->is_leaf) {
            output.push_back(static_cast<char>(current->symbol));
            current = tree;
        }
    }

    return output;
}
