#include "bplus_tree.hpp"
#include "command_parser.hpp"
#include "db_interface.hpp"
#include "huffman.hpp"
#include "memory_layer.hpp"
#include "red_black_tree.hpp"
#include "skip_list.hpp"
#include "storage_layer.hpp"

#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct TestContext {
    int passed = 0;
    int failed = 0;
};

void assert_true(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void run_test(TestContext& context, const std::string& name, const std::function<void()>& test) {
    try {
        test();
        ++context.passed;
        std::cout << "[PASS] " << name << '\n';
    } catch (const std::exception& ex) {
        ++context.failed;
        std::cout << "[FAIL] " << name << ": " << ex.what() << '\n';
    }
}

std::filesystem::path temp_path(const std::string& name) {
    auto path = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove(path);
    return path;
}

void test_huffman_round_trip() {
    HuffmanCodec codec;
    for (const auto& text : std::vector<std::string>{"hello world", "aaaaabbbbcc", ""}) {
        const auto compressed = codec.compress(text);
        const auto decompressed = codec.decompress(compressed);
        assert_true(decompressed == text, "Huffman round-trip mismatch.");
    }
}

void test_red_black_tree_upsert_and_find() {
    HuffmanCodec codec;
    RedBlackTree tree;
    tree.upsert(10, codec.compress("ten"));
    tree.upsert(5, codec.compress("five"));
    tree.upsert(15, codec.compress("fifteen"));
    tree.upsert(10, codec.compress("TEN"));
    const auto found = tree.find(10);
    assert_true(found.has_value(), "Expected RB tree hit.");
    assert_true(codec.decompress(found.value()) == "TEN", "Expected overwrite in RB tree.");
    assert_true(tree.size() == 3, "Expected RB tree unique-key size.");
}

void test_skip_list_range_and_order() {
    HuffmanCodec codec;
    SkipList list;
    list.upsert(8, codec.compress("eight"));
    list.upsert(3, codec.compress("three"));
    list.upsert(5, codec.compress("five"));
    list.upsert(5, codec.compress("FIVE"));

    const auto results = list.range(3, 8);
    assert_true(results.size() == 3, "Expected three skip-list results.");
    assert_true(results[0].first == 3 && results[1].first == 5 && results[2].first == 8,
                "Expected sorted skip-list range.");
    assert_true(codec.decompress(results[1].second) == "FIVE", "Expected skip-list overwrite.");
}

void test_memory_layer_snapshot_sorted() {
    HuffmanCodec codec;
    MemoryLayer layer;
    layer.upsert(7, codec.compress("seven"));
    layer.upsert(2, codec.compress("two"));
    layer.upsert(9, codec.compress("nine"));
    const auto snapshot = layer.snapshot_sorted();
    assert_true(snapshot.size() == 3, "Expected memory snapshot size.");
    assert_true(snapshot[0].first == 2 && snapshot[1].first == 7 && snapshot[2].first == 9,
                "Expected sorted memory snapshot.");
}

void test_bplus_tree_split_and_range() {
    BPlusTree tree(4);
    for (int key = 1; key <= 12; ++key) {
        tree.upsert(key, BPlusTree::ValuePointer{static_cast<std::uint64_t>(key * 100), 8});
    }

    assert_true(tree.nodes().size() > 1, "Expected B+ tree split.");
    assert_true(!tree.nodes()[tree.root_id()].is_leaf, "Expected internal root after split.");

    const auto found = tree.find(9);
    assert_true(found.has_value() && found->offset == 900, "Expected B+ tree lookup.");

    const auto results = tree.range(4, 7);
    assert_true(results.size() == 4, "Expected four B+ tree range records.");
    assert_true(results.front().first == 4 && results.back().first == 7, "Expected inclusive B+ range.");
}

void test_storage_layer_persistence() {
    HuffmanCodec codec;
    const auto index = temp_path("mini_db_index_test.db");
    const auto values = temp_path("mini_db_values_test.db");

    {
        StorageLayer storage(index.string(), values.string());
        storage.open();
        storage.upsert(1, codec.compress("one"));
        storage.upsert(2, codec.compress("two"));
        storage.close();
    }

    {
        StorageLayer storage(index.string(), values.string());
        storage.open();
        const auto value = storage.find(2);
        assert_true(value.has_value(), "Expected persisted storage value.");
        assert_true(codec.decompress(value.value()) == "two", "Expected persisted storage payload.");
        const auto results = storage.range(1, 2);
        assert_true(results.size() == 2, "Expected persisted storage range.");
        storage.close();
    }

    std::filesystem::remove(index);
    std::filesystem::remove(values);
}

void test_database_interface_memory_disk_merge() {
    const auto index = temp_path("mini_db_interface_index.db");
    const auto values = temp_path("mini_db_interface_values.db");

    {
        DatabaseInterface database(index.string(), values.string());
        database.insert(1, "one");
        database.insert(2, "two");
        assert_true(database.get(1).value() == "one", "Expected get from memory.");
        database.flush();
        database.insert(2, "two-updated");
        database.insert(3, "three");

        const auto range = database.range(1, 3);
        assert_true(range.size() == 3, "Expected merged range size.");
        assert_true(range[1].first == 2 && range[1].second == "two-updated", "Expected memory to win on merge.");
        database.shutdown();
    }

    {
        DatabaseInterface database(index.string(), values.string());
        assert_true(database.get(1).value() == "one", "Expected reopened disk value.");
        assert_true(database.get(2).value() == "two-updated", "Expected flush on shutdown.");
        assert_true(database.get(3).value() == "three", "Expected shutdown flush.");
        database.shutdown();
    }

    std::filesystem::remove(index);
    std::filesystem::remove(values);
}

void test_command_parser() {
    std::string error;
    auto insert = parse_command("INSERT 5 \"hello world\"", error);
    assert_true(insert.has_value() && insert->type == ParsedCommand::Type::Insert, "Expected INSERT parse.");
    assert_true(insert->first == 5 && insert->text == "hello world", "Expected parsed INSERT payload.");

    auto range = parse_command("RANGE 3 7", error);
    assert_true(range.has_value() && range->type == ParsedCommand::Type::Range, "Expected RANGE parse.");
    assert_true(range->first == 3 && range->second == 7, "Expected RANGE bounds.");

    auto bad = parse_command("INSERT nope", error);
    assert_true(!bad.has_value() && !error.empty(), "Expected parse failure.");
}

int main() {
    TestContext context;

    run_test(context, "Huffman round-trip", test_huffman_round_trip);
    run_test(context, "Red-Black Tree upsert/find", test_red_black_tree_upsert_and_find);
    run_test(context, "Skip List range/order", test_skip_list_range_and_order);
    run_test(context, "Memory layer snapshot", test_memory_layer_snapshot_sorted);
    run_test(context, "B+ Tree split/range", test_bplus_tree_split_and_range);
    run_test(context, "Storage persistence", test_storage_layer_persistence);
    run_test(context, "Database interface integration", test_database_interface_memory_disk_merge);
    run_test(context, "Command parser", test_command_parser);

    std::cout << "Passed: " << context.passed << ", Failed: " << context.failed << '\n';
    return context.failed == 0 ? 0 : 1;
}
