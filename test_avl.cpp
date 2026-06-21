// Comprehensive test suite for AVLTree.
// Compile: g++ -std=c++17 -O2 -Wall -Wextra -g avl_tree.cpp test_avl.cpp -o test_avl
// Run:     ./test_avl

#include "avl_tree.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Tiny test framework
// ---------------------------------------------------------------------------
static int g_tests_run    = 0;
static int g_tests_passed = 0;

#define RUN_TEST(fn) do {                              \
    ++g_tests_run;                                     \
    std::printf("  [ RUN  ] %s\n", #fn);               \
    fn();                                              \
    ++g_tests_passed;                                  \
    std::printf("  [ PASS ] %s\n", #fn);               \
} while (0)

#define CHECK(cond) do {                               \
    if (!(cond)) {                                     \
        std::fprintf(stderr,                            \
            "\n  CHECK FAILED at %s:%d: %s\n",         \
            __FILE__, __LINE__, #cond);                \
        std::abort();                                   \
    }                                                  \
} while (0)

// Helper: verify tree's in-order keys match expected vector
static void check_order(const AVLTree& t, const std::vector<std::string>& expected) {
    auto k = t.keys_in_order();
    CHECK(k.size() == expected.size());
    for (size_t i = 0; i < k.size(); ++i) {
        CHECK(k[i] == expected[i]);
    }
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------

// 1. Basic insert + search
static void test_insert_search_basic() {
    AVLTree t;
    CHECK(t.empty());
    CHECK(t.size() == 0);

    t.insert("banana", "yellow");
    t.insert("apple", "red");
    t.insert("cherry", "dark red");

    CHECK(t.size() == 3);
    CHECK(!t.empty());

    std::string v;
    CHECK(t.search("apple", &v));   CHECK(v == "red");
    CHECK(t.search("banana", &v));  CHECK(v == "yellow");
    CHECK(t.search("cherry", &v));  CHECK(v == "dark red");

    CHECK(!t.search("durian", &v));
    CHECK(t.contains("apple"));
    CHECK(!t.contains("zzz"));

    check_order(t, {"apple", "banana", "cherry"});
}

// 2. Inserting duplicate key updates value (no size change)
static void test_insert_duplicate_updates() {
    AVLTree t;
    t.insert("key1", "val1");
    CHECK(t.size() == 1);
    t.insert("key1", "val2");      // duplicate
    CHECK(t.size() == 1);

    std::string v;
    CHECK(t.search("key1", &v));
    CHECK(v == "val2");
}

// 3. In-order traversal is sorted after many random-ish inserts
static void test_inorder_sorted() {
    AVLTree t;
    std::set<std::string> reference;
    std::vector<std::string> keys = {
        "z", "a", "m", "b", "y", "c", "x", "d",
        "w", "e", "v", "f", "u", "g", "t", "h",
        "s", "i", "r", "j", "q", "k", "p", "l",
        "o", "n"
    };
    for (size_t i = 0; i < keys.size(); ++i) {
        t.insert(keys[i], std::to_string(i));
        reference.insert(keys[i]);
    }
    CHECK(t.size() == reference.size());
    check_order(t, std::vector<std::string>(reference.begin(), reference.end()));
}

// 4. Delete leaf, node with one child, node with two children
static void test_remove_basic() {
    AVLTree t;
    t.insert("e", "5");
    t.insert("b", "2");
    t.insert("h", "8");
    t.insert("a", "1");
    t.insert("d", "4");
    t.insert("f", "6");
    t.insert("g", "7");
    //        e
    //      /   |
    //     b     h
    //    / \   /
    //   a   d f
    //          |
    //           g

    CHECK(t.remove("a"));      // leaf
    CHECK(t.size() == 6);
    CHECK(!t.contains("a"));
    check_order(t, {"b","d","e","f","g","h"});

    CHECK(t.remove("h"));      // node with one child (f-g subtree)
    CHECK(t.size() == 5);
    CHECK(!t.contains("h"));
    check_order(t, {"b","d","e","f","g"});

    CHECK(t.remove("e"));      // node with two children (root)
    CHECK(t.size() == 4);
    CHECK(!t.contains("e"));
    check_order(t, {"b","d","f","g"});
}

// 5. Deleting nonexistent key returns false
static void test_remove_nonexistent() {
    AVLTree t;
    t.insert("x", "1");
    CHECK(!t.remove("y"));
    CHECK(t.size() == 1);
}

// 6. Stress insert/delete with balance verification
static void test_stress_balance() {
    AVLTree t;

    // Insert 1..500 as keys
    for (int i = 1; i <= 500; ++i) {
        t.insert("k" + std::to_string(i), "v" + std::to_string(i));
    }
    CHECK(t.size() == 500);

    // All present
    for (int i = 1; i <= 500; ++i) {
        CHECK(t.contains("k" + std::to_string(i)));
    }

    // In-order must be sorted
    auto ks = t.keys_in_order();
    for (size_t i = 1; i < ks.size(); ++i) {
        CHECK(ks[i - 1] < ks[i]);
    }

    // Delete every other key
    for (int i = 1; i <= 500; i += 2) {
        CHECK(t.remove("k" + std::to_string(i)));
    }
    CHECK(t.size() == 250);

    // Odd keys gone, even keys present
    for (int i = 1; i <= 500; i += 2) {
        CHECK(!t.contains("k" + std::to_string(i)));
    }
    for (int i = 2; i <= 500; i += 2) {
        CHECK(t.contains("k" + std::to_string(i)));
    }

    // Still sorted
    ks = t.keys_in_order();
    for (size_t i = 1; i < ks.size(); ++i) {
        CHECK(ks[i - 1] < ks[i]);
    }
}

// 7. Save to disk and load back — data preserved
static void test_save_load() {
    const std::string path = "/tmp/avl_test_data.bin";

    AVLTree t1;
    t1.insert("gamma",   "3");
    t1.insert("alpha",   "1");
    t1.insert("beta",    "2");
    t1.insert("delta",   "4");
    t1.insert("epsilon", "5");

    CHECK(t1.save(path));

    AVLTree t2;
    CHECK(t2.load(path));
    CHECK(t2.size() == t1.size());

    // Same keys in same order
    auto k1 = t1.keys_in_order();
    auto k2 = t2.keys_in_order();
    CHECK(k1 == k2);

    // Same values
    std::string v1, v2;
    for (auto& k : k1) {
        CHECK(t1.search(k, &v1));
        CHECK(t2.search(k, &v2));
        CHECK(v1 == v2);
    }

    std::remove(path.c_str());
}

// 8. Load into a non-empty tree replaces contents
static void test_load_replaces() {
    const std::string path = "/tmp/avl_test_replace.bin";

    AVLTree t1;
    t1.insert("a", "1");
    t1.insert("b", "2");
    t1.save(path);

    AVLTree t2;
    t2.insert("zzz", "999");
    t2.insert("yyy", "888");
    CHECK(t2.size() == 2);

    CHECK(t2.load(path));
    CHECK(t2.size() == 2);
    CHECK(t2.contains("a"));
    CHECK(t2.contains("b"));
    CHECK(!t2.contains("zzz"));
    CHECK(!t2.contains("yyy"));

    std::remove(path.c_str());
}

// 9. Empty tree save/load round-trips correctly
static void test_save_load_empty() {
    const std::string path = "/tmp/avl_test_empty.bin";

    AVLTree t1;
    CHECK(t1.save(path));

    AVLTree t2;
    t2.insert("x", "1");
    CHECK(t2.load(path));
    CHECK(t2.size() == 0);
    CHECK(t2.empty());

    std::remove(path.c_str());
}

// 10. Handles empty strings as keys and values
static void test_empty_strings() {
    AVLTree t;
    t.insert("", "");             // empty key, empty value
    t.insert("a", "");
    t.insert("", "updated");      // update empty key

    CHECK(t.size() == 2);

    std::string v;
    CHECK(t.search("", &v));
    CHECK(v == "updated");
    CHECK(t.search("a", &v));
    CHECK(v.empty());
}

// 11. Large save/load stress
static void test_save_load_large() {
    const std::string path = "/tmp/avl_test_large.bin";

    AVLTree t1;
    for (int i = 0; i < 1000; ++i) {
        t1.insert("node_" + std::to_string(i), "val_" + std::to_string(i));
    }
    CHECK(t1.size() == 1000);
    CHECK(t1.save(path));

    AVLTree t2;
    CHECK(t2.load(path));
    CHECK(t2.size() == 1000);

    std::string v;
    for (int i = 0; i < 1000; ++i) {
        std::string key = "node_" + std::to_string(i);
        CHECK(t2.search(key, &v));
        CHECK(v == ("val_" + std::to_string(i)));
    }

    // Order preserved
    auto k1 = t1.keys_in_order();
    auto k2 = t2.keys_in_order();
    CHECK(k1 == k2);

    std::remove(path.c_str());
}

// 12. Load from a nonexistent file fails gracefully
static void test_load_nonexistent() {
    AVLTree t;
    CHECK(!t.load("/tmp/avl_does_not_exist_xyz123.bin"));
    CHECK(t.empty());
}

// 13. Delete all elements one by one
static void test_delete_all() {
    AVLTree t;
    for (int i = 0; i < 20; ++i) {
        t.insert("k" + std::to_string(i), "v");
    }
    CHECK(t.size() == 20);

    for (int i = 0; i < 20; ++i) {
        CHECK(t.remove("k" + std::to_string(i)));
    }
    CHECK(t.size() == 0);
    CHECK(t.empty());
    CHECK(t.keys_in_order().empty());
}

// 14. Clear works
static void test_clear() {
    AVLTree t;
    t.insert("a", "1");
    t.insert("b", "2");
    t.insert("c", "3");
    t.clear();
    CHECK(t.empty());
    CHECK(t.size() == 0);
    CHECK(!t.contains("a"));
}

// 15. Load rejects a truncated/crafted file whose header claims more
//     entries than the file body can possibly contain.
static void test_load_truncated_file() {
    const std::string path = "/tmp/avl_test_truncated.bin";

    // Craft a file: header says 1000 entries, but body has only 1 entry.
    // The pre-flight check should reject it without allocating 999 nodes.
    std::ofstream ofs(path, std::ios::binary);

    uint32_t fake_count = 1000;                         // lies
    ofs.write(reinterpret_cast<const char*>(&fake_count), sizeof(fake_count));

    // Write one valid entry: key="a", val="1"
    auto write_u32 = [&](uint32_t v) {
        ofs.write(reinterpret_cast<const char*>(&v), sizeof(v));
    };
    const std::string key = "a", val = "1";
    write_u32(static_cast<uint32_t>(key.size()));
    ofs.write(key.data(), key.size());
    write_u32(static_cast<uint32_t>(val.size()));
    ofs.write(val.data(), val.size());
    ofs.close();

    // Load must fail, tree must remain empty
    AVLTree t;
    CHECK(!t.load(path));
    CHECK(t.empty());
    CHECK(t.size() == 0);

    // Extra: also test a header-only file (count > 0, no body at all)
    std::ofstream ofs2(path, std::ios::binary);
    fake_count = 10;
    ofs2.write(reinterpret_cast<const char*>(&fake_count), sizeof(fake_count));
    ofs2.close();

    CHECK(!t.load(path));
    CHECK(t.empty());

    std::remove(path.c_str());
}

// 16. Load rejects a file with a single zero-length body where the
//     header claims 2+ entries — tests the boundary of the size check.
static void test_load_size_check_boundary() {
    const std::string path = "/tmp/avl_test_boundary.bin";

    // Header claims 2 entries (needs >= 16 bytes of body), but body
    // is only 8 bytes (exactly one empty/empty entry).
    std::ofstream ofs(path, std::ios::binary);
    uint32_t count = 2;
    ofs.write(reinterpret_cast<const char*>(&count), sizeof(count));
    // One entry: key_len=0, val_len=0  (8 bytes total)
    uint32_t zero = 0;
    ofs.write(reinterpret_cast<const char*>(&zero), sizeof(zero));  // key_len
    ofs.write(reinterpret_cast<const char*>(&zero), sizeof(zero));  // val_len
    ofs.close();

    AVLTree t;
    CHECK(!t.load(path));
    CHECK(t.empty());

    // But if header claims 1 entry with 8 bytes of body — that's valid
    std::ofstream ofs2(path, std::ios::binary);
    count = 1;
    ofs2.write(reinterpret_cast<const char*>(&count), sizeof(count));
    ofs2.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
    ofs2.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
    ofs2.close();

    CHECK(t.load(path));
    CHECK(t.size() == 1);
    CHECK(t.contains(""));

    std::remove(path.c_str());
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    std::printf("========================================\n");
    std::printf(" AVL Tree Test Suite\n");
    std::printf("========================================\n");

    RUN_TEST(test_insert_search_basic);
    RUN_TEST(test_insert_duplicate_updates);
    RUN_TEST(test_inorder_sorted);
    RUN_TEST(test_remove_basic);
    RUN_TEST(test_remove_nonexistent);
    RUN_TEST(test_stress_balance);
    RUN_TEST(test_save_load);
    RUN_TEST(test_load_replaces);
    RUN_TEST(test_save_load_empty);
    RUN_TEST(test_empty_strings);
    RUN_TEST(test_save_load_large);
    RUN_TEST(test_load_nonexistent);
    RUN_TEST(test_delete_all);
    RUN_TEST(test_clear);
    RUN_TEST(test_load_truncated_file);
    RUN_TEST(test_load_size_check_boundary);

    std::printf("========================================\n");
    std::printf(" Results: %d/%d passed\n", g_tests_passed, g_tests_run);
    std::printf("========================================\n");

    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
