#include "db_interface.hpp"

#include "httplib.h"
#include "json.hpp"

#include <cstdlib>
#include <exception>
#include <optional>
#include <string>

using json = nlohmann::json;
using namespace std;

namespace {

json to_json_value(const GraphNode& node) {
    return {
        {"id", node.id},
        {"label", node.label},
        {"group", node.group},
        {"x", node.x},
        {"y", node.y},
        {"highlighted", node.highlighted},
        {"active", node.active},
        {"dimmed", node.dimmed},
        {"note", node.note},
    };
}

json to_json_value(const GraphEdge& edge) {
    return {
        {"id", edge.id},
        {"source", edge.source},
        {"target", edge.target},
        {"label", edge.label},
        {"highlighted", edge.highlighted},
        {"dashed", edge.dashed},
    };
}

json to_json_value(const GraphSnapshot& snapshot) {
    json nodes = json::array();
    for (const auto& node : snapshot.nodes) {
        nodes.push_back(to_json_value(node));
    }

    json edges = json::array();
    for (const auto& edge : snapshot.edges) {
        edges.push_back(to_json_value(edge));
    }

    return {
        {"layout", snapshot.layout},
        {"nodes", nodes},
        {"edges", edges},
    };
}

json to_json_value(const TableRow& row) {
    return {
        {"cells", row.cells},
        {"highlighted", row.highlighted},
    };
}

json to_json_value(const TableSnapshot& table) {
    json rows = json::array();
    for (const auto& row : table.rows) {
        rows.push_back(to_json_value(row));
    }
    return {
        {"columns", table.columns},
        {"rows", rows},
    };
}

json to_json_value(const CompressionTrace& compression) {
    return {
        {"frequencyTable", to_json_value(compression.frequency_table)},
        {"heapTable", to_json_value(compression.heap_table)},
        {"treeSnapshot", to_json_value(compression.tree_snapshot)},
        {"codeTable", to_json_value(compression.code_table)},
        {"bitString", compression.bit_string},
        {"compressedBytesHex", compression.compressed_bytes_hex},
        {"bitCount", compression.bit_count},
    };
}

json to_json_value(const RangeMergeItem& item) {
    return {
        {"key", item.key},
        {"value", item.value},
        {"source", item.source},
    };
}

json to_json_value(const RangeMergeTrace& trace) {
    json items = json::array();
    for (const auto& item : trace.items) {
        items.push_back(to_json_value(item));
    }
    return {{"items", items}};
}

json to_json_value(const TracePanel& panel) {
    json output = {
        {"id", panel.id},
        {"title", panel.title},
        {"kind", panel.kind},
        {"description", panel.description},
        {"bullets", panel.bullets},
    };
    if (panel.graph.has_value()) {
        output["graph"] = to_json_value(panel.graph.value());
    }
    if (panel.table.has_value()) {
        output["table"] = to_json_value(panel.table.value());
    }
    if (panel.compression.has_value()) {
        output["compression"] = to_json_value(panel.compression.value());
    }
    if (panel.range_merge.has_value()) {
        output["rangeMerge"] = to_json_value(panel.range_merge.value());
    }
    return output;
}

json to_json_value(const ResultRow& row) {
    return {
        {"key", row.key},
        {"value", row.value},
        {"source", row.source},
    };
}

json to_json_value(const OperationTrace& trace) {
    json rows = json::array();
    for (const auto& row : trace.rows) {
        rows.push_back(to_json_value(row));
    }

    json panels = json::array();
    for (const auto& panel : trace.panels) {
        panels.push_back(to_json_value(panel));
    }

    return {
        {"command", trace.command},
        {"commandType", trace.command_type},
        {"success", trace.success},
        {"statusMessage", trace.status_message},
        {"resultSource", trace.result_source},
        {"rows", rows},
        {"panels", panels},
    };
}

}  // namespace

int main(int argc, char* argv[]) {
    int port = 8080;
    string index_path = "browser_index.db";
    string values_path = "browser_values.db";
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0) {
            port = 8080;
        }
    }
    if (argc > 2) {
        index_path = argv[2];
    }
    if (argc > 3) {
        values_path = argv[3];
    }

    try {
        DatabaseInterface database(index_path, values_path);
        httplib::Server server;

        server.Get("/api/health", [](const httplib::Request&, httplib::Response& response) {
            response.set_content(json({{"ok", true}}).dump(), "application/json");
        });

        server.Post("/api/execute", [&](const httplib::Request& request, httplib::Response& response) {
            try {
                const auto body = json::parse(request.body);
                if (!body.contains("command") || !body["command"].is_string()) {
                    response.status = 400;
                    response.set_content(
                        json({{"success", false}, {"statusMessage", "Request must include a string command."}}).dump(),
                        "application/json");
                    return;
                }

                auto trace = database.execute_with_trace(body["command"].get<string>());
                response.set_content(to_json_value(trace).dump(), "application/json");
            } catch (const exception& ex) {
                response.status = 500;
                response.set_content(json({{"success", false}, {"statusMessage", ex.what()}}).dump(),
                                     "application/json");
            }
        });

        if (!server.listen("127.0.0.1", port)) {
            return 1;
        }
    } catch (const exception& ex) {
        return 1;
    }

    return 0;
}
