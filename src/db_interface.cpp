#include "db_interface.hpp"

#include "command_parser.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

using namespace std;

namespace {

string command_type_name(ParsedCommand::Type type) {
    switch (type) {
        case ParsedCommand::Type::Insert:
            return "INSERT";
        case ParsedCommand::Type::Get:
            return "GET";
        case ParsedCommand::Type::Range:
            return "RANGE";
        case ParsedCommand::Type::Flush:
            return "FLUSH";
        case ParsedCommand::Type::Exit:
            return "EXIT";
    }
    return "UNKNOWN";
}

TracePanel make_graph_panel(const string& id,
                            const string& title,
                            const string& description,
                            GraphSnapshot snapshot,
                            vector<string> bullets = {}) {
    TracePanel panel;
    panel.id = id;
    panel.title = title;
    panel.kind = "graph";
    panel.description = description;
    panel.graph = move(snapshot);
    panel.bullets = move(bullets);
    return panel;
}

TracePanel make_table_panel(const string& id,
                            const string& title,
                            const string& description,
                            TableSnapshot table,
                            vector<string> bullets = {}) {
    TracePanel panel;
    panel.id = id;
    panel.title = title;
    panel.kind = "table";
    panel.description = description;
    panel.table = move(table);
    panel.bullets = move(bullets);
    return panel;
}

TracePanel make_compression_panel(const CompressionTrace& compression) {
    TracePanel panel;
    panel.id = "compression";
    panel.title = "Compression";
    panel.kind = "compression";
    panel.description = "Huffman frequency summary, min-heap merges, code table, and final bit payload.";
    panel.compression = compression;
    return panel;
}

TracePanel make_range_merge_panel(vector<RangeMergeItem> items) {
    TracePanel panel;
    panel.id = "range-merge";
    panel.title = "Range Merge";
    panel.kind = "range-merge";
    panel.description = "Merged range rows with memory records taking precedence over disk records.";
    RangeMergeTrace trace;
    trace.items = move(items);
    panel.range_merge = move(trace);
    return panel;
}

}  // namespace

DatabaseInterface::DatabaseInterface(string index_path, string values_path)
    : storage_layer_(move(index_path), move(values_path)), shutdown_called_(false) {
    storage_layer_.open();
}

DatabaseInterface::~DatabaseInterface() {
    if (!shutdown_called_) {
        try {
            shutdown();
        } catch (...) {
        }
    }
}

void DatabaseInterface::insert(int key, const string& value) {
    memory_layer_.upsert(key, codec_.compress(value));
}

optional<string> DatabaseInterface::get(int key) {
    auto memory_value = memory_layer_.find(key);
    if (memory_value.has_value()) {
        return codec_.decompress(memory_value.value());
    }

    auto storage_value = storage_layer_.find(key);
    if (!storage_value.has_value()) {
        return nullopt;
    }
    return codec_.decompress(storage_value.value());
}

vector<pair<int, string>> DatabaseInterface::range(int start, int end) {
    auto memory_records = memory_layer_.range(start, end);
    auto storage_records = storage_layer_.range(start, end);

    vector<pair<int, string>> merged;
    size_t memory_index = 0;
    size_t storage_index = 0;

    while (memory_index < memory_records.size() || storage_index < storage_records.size()) {
        if (storage_index >= storage_records.size() ||
            (memory_index < memory_records.size() &&
             memory_records[memory_index].first < storage_records[storage_index].first)) {
            merged.push_back(make_pair(memory_records[memory_index].first,
                                       codec_.decompress(memory_records[memory_index].second)));
            ++memory_index;
        } else if (memory_index >= memory_records.size() ||
                   storage_records[storage_index].first < memory_records[memory_index].first) {
            merged.push_back(make_pair(storage_records[storage_index].first,
                                       codec_.decompress(storage_records[storage_index].second)));
            ++storage_index;
        } else {
            merged.push_back(make_pair(memory_records[memory_index].first,
                                       codec_.decompress(memory_records[memory_index].second)));
            ++memory_index;
            ++storage_index;
        }
    }

    return merged;
}

void DatabaseInterface::flush() {
    const auto snapshot = memory_layer_.snapshot_sorted();
    for (const auto& item : snapshot) {
        storage_layer_.upsert(item.first, item.second);
    }
    memory_layer_.clear();
}

void DatabaseInterface::shutdown() {
    flush();
    storage_layer_.close();
    shutdown_called_ = true;
}

OperationTrace DatabaseInterface::execute_with_trace(const string& raw_command) {
    OperationTrace trace;
    trace.command = raw_command;

    string error;
    const auto parsed = parse_command(raw_command, error);
    if (!parsed.has_value()) {
        trace.success = false;
        trace.command_type = "INVALID";
        trace.status_message = error;
        return trace;
    }

    trace.command_type = command_type_name(parsed->type);

    switch (parsed->type) {
        case ParsedCommand::Type::Insert: {
            auto compression = codec_.compress_with_trace(parsed->text);
            memory_layer_.upsert(parsed->first, compression.first);
            trace.status_message = "Inserted into memory layer.";
            trace.result_source = "memory";
            trace.rows.push_back({parsed->first, parsed->text, "memory"});
            trace.panels.push_back(make_compression_panel(compression.second));
            trace.panels.push_back(make_graph_panel("rbtree",
                                                    "Red-Black Tree",
                                                    "Memory lookup structure after the insert/update.",
                                                    memory_layer_.rb_tree_snapshot({}, parsed->first)));
            trace.panels.push_back(make_graph_panel("skiplist",
                                                    "Skip List",
                                                    "Ordered memory view after the insert/update.",
                                                    memory_layer_.skip_list_snapshot({parsed->first})));
            break;
        }

        case ParsedCommand::Type::Get: {
            vector<int> rb_path;
            auto memory_value = memory_layer_.find_with_trace(parsed->first, rb_path);
            if (memory_value.has_value()) {
                const auto value = codec_.decompress(memory_value.value());
                trace.status_message = "Key found in memory.";
                trace.result_source = "memory";
                trace.rows.push_back({parsed->first, value, "memory"});
                trace.panels.push_back(make_graph_panel("rbtree-search",
                                                        "Red-Black Tree Search",
                                                        "Visited nodes during in-memory lookup.",
                                                        memory_layer_.rb_tree_snapshot(rb_path, parsed->first),
                                                        {"Read served directly from memory layer."}));
            } else {
                vector<uint64_t> node_path;
                uint64_t leaf_id = BPlusTree::Node::invalid_id();
                auto storage_value = storage_layer_.find_with_trace(parsed->first, node_path, leaf_id);
                if (!storage_value.has_value()) {
                    trace.status_message = "Key not found.";
                    trace.result_source = "none";
                    trace.panels.push_back(make_graph_panel("rbtree-miss",
                                                            "Red-Black Tree Search",
                                                            "Memory lookup missed, then storage lookup ran.",
                                                            memory_layer_.rb_tree_snapshot(rb_path)));
                    trace.panels.push_back(make_graph_panel("bplus-search",
                                                            "B+ Tree Search",
                                                            "Visited internal nodes and target leaf on disk lookup.",
                                                            storage_layer_.tree_snapshot(node_path)));
                } else {
                    const auto value = codec_.decompress(storage_value.value());
                    trace.status_message = "Key found on disk.";
                    trace.result_source = "disk";
                    trace.rows.push_back({parsed->first, value, "disk"});
                    trace.panels.push_back(make_graph_panel("rbtree-miss",
                                                            "Red-Black Tree Search",
                                                            "Memory lookup missed before falling back to disk.",
                                                            memory_layer_.rb_tree_snapshot(rb_path)));
                    trace.panels.push_back(make_graph_panel("bplus-search",
                                                            "B+ Tree Search",
                                                            "Visited internal nodes and target leaf on disk lookup.",
                                                            storage_layer_.tree_snapshot(node_path, {parsed->first}),
                                                            {"Read served from storage layer."}));
                }
            }
            break;
        }

        case ParsedCommand::Type::Range: {
            vector<int> skip_visited;
            vector<int> memory_keys;
            auto memory_records =
                memory_layer_.range_with_trace(parsed->first, parsed->second, skip_visited, memory_keys);

            vector<uint64_t> node_path;
            vector<uint64_t> scanned_leaves;
            auto storage_records =
                storage_layer_.range_with_trace(parsed->first, parsed->second, node_path, scanned_leaves);

            vector<RangeMergeItem> merge_items;
            size_t memory_index = 0;
            size_t storage_index = 0;
            while (memory_index < memory_records.size() || storage_index < storage_records.size()) {
                if (storage_index >= storage_records.size() ||
                    (memory_index < memory_records.size() &&
                     memory_records[memory_index].first < storage_records[storage_index].first)) {
                    const auto value = codec_.decompress(memory_records[memory_index].second);
                    trace.rows.push_back({memory_records[memory_index].first, value, "memory"});
                    merge_items.push_back({memory_records[memory_index].first, value, "memory"});
                    ++memory_index;
                } else if (memory_index >= memory_records.size() ||
                           storage_records[storage_index].first < memory_records[memory_index].first) {
                    const auto value = codec_.decompress(storage_records[storage_index].second);
                    trace.rows.push_back({storage_records[storage_index].first, value, "disk"});
                    merge_items.push_back({storage_records[storage_index].first, value, "disk"});
                    ++storage_index;
                } else {
                    const auto value = codec_.decompress(memory_records[memory_index].second);
                    trace.rows.push_back({memory_records[memory_index].first, value, "memory"});
                    merge_items.push_back({memory_records[memory_index].first, value, "memory"});
                    ++memory_index;
                    ++storage_index;
                }
            }

            ostringstream message;
            message << "Returned " << trace.rows.size() << " row";
            if (trace.rows.size() != 1) {
                message << "s";
            }
            message << " from the inclusive range.";
            trace.status_message = message.str();
            trace.result_source = trace.rows.empty() ? "none" : "mixed";

            if (!memory_keys.empty()) {
                trace.panels.push_back(make_graph_panel("skiplist-range",
                                                        "Skip List Range",
                                                        "Memory-side ordered scan for the requested range.",
                                                        memory_layer_.skip_list_snapshot(memory_keys)));
            }
            vector<uint64_t> active_nodes = node_path;
            active_nodes.insert(active_nodes.end(), scanned_leaves.begin(), scanned_leaves.end());
            trace.panels.push_back(make_graph_panel("bplus-range",
                                                    "B+ Tree Range",
                                                    "Disk-side descent plus linked-leaf scan.",
                                                    storage_layer_.tree_snapshot(active_nodes),
                                                    {"Leaf chain traversal is highlighted on disk results."}));
            trace.panels.push_back(make_range_merge_panel(move(merge_items)));
            break;
        }

        case ParsedCommand::Type::Flush: {
            const auto snapshot = memory_layer_.snapshot_sorted();
            vector<int> flushed_keys;
            vector<uint64_t> touched_nodes;
            for (const auto& item : snapshot) {
                flushed_keys.push_back(item.first);
                auto touched = storage_layer_.upsert_with_trace(item.first, item.second);
                touched_nodes.insert(touched_nodes.end(), touched.begin(), touched.end());
            }
            GraphSnapshot skip_snapshot = memory_layer_.skip_list_snapshot(flushed_keys);
            memory_layer_.clear();

            ostringstream message;
            message << "Flushed " << flushed_keys.size() << " record";
            if (flushed_keys.size() != 1) {
                message << "s";
            }
            message << " to disk.";
            trace.status_message = message.str();
            trace.result_source = "disk";

            if (!flushed_keys.empty()) {
                trace.panels.push_back(make_graph_panel("skiplist-flush",
                                                        "Skip List Flush Order",
                                                        "Memory records are flushed in sorted order.",
                                                        move(skip_snapshot)));
            }
            trace.panels.push_back(make_graph_panel("bplus-flush",
                                                    "B+ Tree After Flush",
                                                    "Touched nodes and leaves after storage upserts.",
                                                    storage_layer_.tree_snapshot(touched_nodes, flushed_keys)));
            TableSnapshot summary;
            summary.columns = {"Metric", "Value"};
            summary.rows.push_back({{"Persisted records", to_string(flushed_keys.size())}, true});
            trace.panels.push_back(make_table_panel("flush-summary",
                                                    "Flush Summary",
                                                    "Persistence outcome for the current flush.",
                                                    move(summary)));
            break;
        }

        case ParsedCommand::Type::Exit: {
            flush();
            storage_layer_.close();
            shutdown_called_ = true;
            storage_layer_.open();
            shutdown_called_ = false;
            trace.status_message = "Storage flushed and reopened; browser session remains available.";
            trace.result_source = "disk";
            break;
        }
    }

    return trace;
}
