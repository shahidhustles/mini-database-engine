#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct GraphNode {
    std::string id;
    std::string label;
    std::string group;
    double x = 0.0;
    double y = 0.0;
    bool highlighted = false;
    bool active = false;
    bool dimmed = false;
    std::string note;
};

struct GraphEdge {
    std::string id;
    std::string source;
    std::string target;
    std::string label;
    bool highlighted = false;
    bool dashed = false;
};

struct GraphSnapshot {
    std::string layout = "breadthfirst";
    std::vector<GraphNode> nodes;
    std::vector<GraphEdge> edges;
};

struct TableRow {
    std::vector<std::string> cells;
    bool highlighted = false;
};

struct TableSnapshot {
    std::vector<std::string> columns;
    std::vector<TableRow> rows;
};

struct CompressionTrace {
    TableSnapshot frequency_table;
    TableSnapshot heap_table;
    GraphSnapshot tree_snapshot;
    TableSnapshot code_table;
    std::string bit_string;
    std::string compressed_bytes_hex;
    std::uint64_t bit_count = 0;
};

struct RangeMergeItem {
    int key = 0;
    std::string value;
    std::string source;
};

struct RangeMergeTrace {
    std::vector<RangeMergeItem> items;
};

struct TracePanel {
    std::string id;
    std::string title;
    std::string kind;
    std::string description;
    std::optional<GraphSnapshot> graph;
    std::optional<TableSnapshot> table;
    std::optional<CompressionTrace> compression;
    std::optional<RangeMergeTrace> range_merge;
    std::vector<std::string> bullets;
};

struct ResultRow {
    int key = 0;
    std::string value;
    std::string source;
};

struct OperationTrace {
    std::string command;
    std::string command_type;
    bool success = true;
    std::string status_message;
    std::string result_source;
    std::vector<ResultRow> rows;
    std::vector<TracePanel> panels;
};
