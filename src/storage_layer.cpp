#include "storage_layer.hpp"

#include <filesystem>
#include <stdexcept>

using namespace std;

StorageLayer::StorageLayer(string index_path, string values_path)
    : index_path_(move(index_path)),
      values_path_(move(values_path)),
      is_open_(false),
      tree_(4),
      values_stream_(nullptr) {}

StorageLayer::~StorageLayer() {
    close();
}

void StorageLayer::open() {
    if (is_open_) {
        return;
    }

    tree_.load(index_path_);

    if (!filesystem::exists(values_path_)) {
        ofstream create(values_path_, ios::binary);
        if (!create) {
            throw runtime_error("Failed to create values file.");
        }
    }

    values_stream_ = new fstream(values_path_, ios::binary | ios::in | ios::out);
    if (!values_stream_ || !(*values_stream_)) {
        delete values_stream_;
        values_stream_ = nullptr;
        throw runtime_error("Failed to open values file.");
    }

    is_open_ = true;
}

void StorageLayer::upsert(int key, const CompressedValue& value) {
    if (!is_open_) {
        throw runtime_error("Storage layer is not open.");
    }

    const auto pointer = append_value(value);
    tree_.upsert(key, pointer);
    tree_.save(index_path_);
}

optional<CompressedValue> StorageLayer::find(int key) {
    if (!is_open_) {
        throw runtime_error("Storage layer is not open.");
    }

    const auto pointer = tree_.find(key);
    if (!pointer.has_value()) {
        return nullopt;
    }
    return read_value(pointer.value());
}

vector<pair<int, CompressedValue>> StorageLayer::range(int start, int end) {
    if (!is_open_) {
        throw runtime_error("Storage layer is not open.");
    }

    vector<pair<int, CompressedValue>> results;
    for (const auto& item : tree_.range(start, end)) {
        results.push_back(make_pair(item.first, read_value(item.second)));
    }
    return results;
}

void StorageLayer::close() {
    if (values_stream_ != nullptr) {
        values_stream_->close();
        delete values_stream_;
        values_stream_ = nullptr;
    }
    is_open_ = false;
}

BPlusTree::ValuePointer StorageLayer::append_value(const CompressedValue& value) {
    values_stream_->clear();
    values_stream_->seekp(0, ios::end);
    const auto offset = static_cast<uint64_t>(values_stream_->tellp());
    value.write(*values_stream_);
    values_stream_->flush();

    BPlusTree::ValuePointer pointer;
    pointer.offset = offset;
    pointer.length = value.serialized_size();
    return pointer;
}

CompressedValue StorageLayer::read_value(const BPlusTree::ValuePointer& pointer) {
    values_stream_->clear();
    values_stream_->seekg(static_cast<streamoff>(pointer.offset), ios::beg);
    return CompressedValue::read(*values_stream_);
}
