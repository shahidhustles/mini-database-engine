#include "huffman.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <istream>
#include <map>
#include <memory>
#include <ostream>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

using namespace std;

struct HuffmanNode {
    uint8_t symbol;
    uint32_t frequency;
    bool is_leaf;
    HuffmanNode* left;
    HuffmanNode* right;
};

void delete_tree(HuffmanNode* node) {
    if (node) {
        delete_tree(node->left);
        delete_tree(node->right);
        delete node;
    }
}

HuffmanNode* build_tree(
    const vector<pair<uint8_t, uint32_t>>& frequencies) {
    if (frequencies.empty()) {
        return nullptr;
    }

    vector<HuffmanNode*> nodes;

    for (const auto& entry : frequencies) {
        nodes.push_back(new HuffmanNode{entry.first, entry.second, true, nullptr, nullptr});
    }

    if (nodes.size() == 1) {
        auto only = nodes.front();
        auto parent = new HuffmanNode{0, only->frequency, false, only, nullptr};
        return parent;
    }

    while (nodes.size() > 1) {
        sort(nodes.begin(), nodes.end(), [](HuffmanNode* a, HuffmanNode* b) {
            return a->frequency > b->frequency;
        });

        auto right = nodes.back();
        nodes.pop_back();
        auto left = nodes.back();
        nodes.pop_back();

        nodes.push_back(new HuffmanNode{
            0,
            left->frequency + right->frequency,
            false,
            left,
            right,
        });
    }

    return nodes.front();
}

void build_codes(HuffmanNode* node,
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
    CompressedValue result;
    if (text.empty()) {
        return result;
    }

    array<uint32_t, 256> frequencies{};
    for (unsigned char ch : text) {
        ++frequencies[ch];
    }

    for (size_t i = 0; i < frequencies.size(); ++i) {
        if (frequencies[i] != 0) {
            result.frequencies.push_back(make_pair(static_cast<uint8_t>(i), frequencies[i]));
        }
    }
    sort(result.frequencies.begin(), result.frequencies.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

    auto tree = build_tree(result.frequencies);
    unordered_map<uint8_t, string> codes;
    build_codes(tree, "", codes);

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
    
    delete_tree(tree);

    return result;
}

string HuffmanCodec::decompress(const CompressedValue& value) const {
    if (value.bit_count == 0) {
        return "";
    }

    auto tree = build_tree(value.frequencies);
    if (!tree) {
        throw runtime_error("Cannot decompress without Huffman metadata.");
    }

    if (tree->left && !tree->right && tree->left->is_leaf) {
        const auto repeat_count = static_cast<size_t>(tree->left->frequency);
        string output = string(repeat_count, static_cast<char>(tree->left->symbol));
        delete_tree(tree);
        return output;
    }

    string output;
    output.reserve(value.bit_count);
    auto current = tree;
    for (uint64_t bit_index = 0; bit_index < value.bit_count; ++bit_index) {
        const auto byte = value.compressed_bytes[bit_index / 8];
        const bool is_one = ((byte >> (7 - (bit_index % 8))) & 0x1U) != 0;
        current = is_one ? current->right : current->left;
        if (!current) {
            delete_tree(tree);
            throw runtime_error("Corrupted Huffman bit stream.");
        }
        if (current->is_leaf) {
            output.push_back(static_cast<char>(current->symbol));
            current = tree;
        }
    }
    
    delete_tree(tree);

    return output;
}
