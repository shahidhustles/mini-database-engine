#include "db_interface.hpp"

#include <utility>

using namespace std;

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
