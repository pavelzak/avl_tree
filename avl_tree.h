#ifndef AVL_TREE_H
#define AVL_TREE_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <functional>

// AVL tree storing string values (keyed by string comparison).
// Supports insert, remove, search, in-order traversal, and
// binary serialization to / from disk.
class AVLTree {
public:
    AVLTree();
    ~AVLTree();

    AVLTree(const AVLTree&) = delete;
    AVLTree& operator=(const AVLTree&) = delete;

    // --- Core operations ---
    void insert(const std::string& key, const std::string& value);
    // Returns true if the key existed and was removed.
    bool remove(const std::string& key);
    // Returns true if found; writes value to *out (if out != nullptr).
    bool search(const std::string& key, std::string* out) const;
    bool contains(const std::string& key) const;

    // --- Utilities ---
    bool   empty() const;
    size_t size() const;
    void   clear();

    // In-order traversal: calls cb(key, value) for every node in sorted order.
    void inorder(const std::function<void(const std::string&, const std::string&)>& cb) const;

    // Returns keys in sorted order (in-order key sequence).
    std::vector<std::string> keys_in_order() const;

    // --- Serialization ---
    // Binary format:
    //   uint32_t node_count
    //   repeat node_count times:
    //     uint32_t key_len, key bytes,
    //     uint32_t val_len, val bytes
    // Tree is rebuilt by inserting in the recorded order; since this is
    // an AVL tree the final structure is deterministic regardless of order.
    bool save(const std::string& path) const;
    bool load(const std::string& path);

private:
    struct Node {
        std::string key;
        std::string value;
        Node* left   = nullptr;
        Node* right  = nullptr;
        int   height = 1;

        Node(const std::string& k, const std::string& v)
            : key(k), value(v) {}
    };

    Node*   root_  = nullptr;
    size_t  count_ = 0;

    // --- internal helpers ---
    static int  height(Node* n);
    static int  balance_factor(Node* n);
    static void update_height(Node* n);
    static Node* rotate_right(Node* y);
    static Node* rotate_left(Node* x);
    static Node* rebalance(Node* n);

    static Node* insert_node(Node* node, const std::string& key,
                             const std::string& value, bool& inserted);
    static Node* min_node(Node* n);
    static Node* remove_node(Node* node, const std::string& key, bool& removed);
    static void  destroy(Node* node);

    void  inorder_rec(Node* n,
                      const std::function<void(const std::string&, const std::string&)>& cb) const;

    // serialization helpers
    static void write_str(std::ostream& os, const std::string& s);
    static bool read_str(std::istream& is, std::string& s);
    void collect_pairs_rec(Node* n, std::vector<std::pair<std::string,std::string>>& out) const;
};

#endif // AVL_TREE_H
