#include "avl_tree.h"

#include <cstdint>
#include <algorithm>

// ============================================================
// Construction / destruction
// ============================================================

AVLTree::AVLTree() = default;

AVLTree::~AVLTree() {
    destroy(root_);
}

void AVLTree::destroy(Node* node) {
    if (!node) return;
    destroy(node->left);
    destroy(node->right);
    delete node;
}

// ============================================================
// Height / balance helpers
// ============================================================

int AVLTree::height(Node* n) {
    return n ? n->height : 0;
}

int AVLTree::balance_factor(Node* n) {
    return n ? height(n->left) - height(n->right) : 0;
}

void AVLTree::update_height(Node* n) {
    n->height = 1 + std::max(height(n->left), height(n->right));
}

// ============================================================
// Rotations
// ============================================================

AVLTree::Node* AVLTree::rotate_right(Node* y) {
    Node* x  = y->left;
    Node* t2 = x->right;

    x->right = y;
    y->left  = t2;

    update_height(y);
    update_height(x);
    return x;
}

AVLTree::Node* AVLTree::rotate_left(Node* x) {
    Node* y  = x->right;
    Node* t2 = y->left;

    y->left  = x;
    x->right = t2;

    update_height(x);
    update_height(y);
    return y;
}

AVLTree::Node* AVLTree::rebalance(Node* n) {
    update_height(n);
    int bf = balance_factor(n);

    // Left-heavy
    if (bf > 1) {
        if (balance_factor(n->left) < 0) {      // Left-Right
            n->left = rotate_left(n->left);
        }
        return rotate_right(n);                  // Left-Left
    }
    // Right-heavy
    if (bf < -1) {
        if (balance_factor(n->right) > 0) {     // Right-Left
            n->right = rotate_right(n->right);
        }
        return rotate_left(n);                   // Right-Right
    }
    return n;
}

// ============================================================
// Insert
// ============================================================

AVLTree::Node* AVLTree::insert_node(Node* node, const std::string& key,
                                    const std::string& value, bool& inserted) {
    if (!node) {
        inserted = true;
        return new Node(key, value);
    }
    if (key < node->key) {
        node->left  = insert_node(node->left,  key, value, inserted);
    } else if (key > node->key) {
        node->right = insert_node(node->right, key, value, inserted);
    } else {
        // key already exists → update value
        node->value = value;
        inserted = false;
        return node;
    }
    return rebalance(node);
}

void AVLTree::insert(const std::string& key, const std::string& value) {
    bool inserted = false;
    root_ = insert_node(root_, key, value, inserted);
    if (inserted) count_++;
}

// ============================================================
// Remove
// ============================================================

AVLTree::Node* AVLTree::min_node(Node* n) {
    while (n && n->left) n = n->left;
    return n;
}

AVLTree::Node* AVLTree::remove_node(Node* node, const std::string& key,
                                    bool& removed) {
    if (!node) return nullptr;

    if (key < node->key) {
        node->left  = remove_node(node->left,  key, removed);
    } else if (key > node->key) {
        node->right = remove_node(node->right, key, removed);
    } else {
        // found the node to delete
        removed = true;
        if (!node->left || !node->right) {
            Node* child = node->left ? node->left : node->right;
            delete node;
            return child;
        }
        // two children: copy successor's key/value, then delete successor
        Node* succ = min_node(node->right);
        node->key   = succ->key;
        node->value = succ->value;
        bool dummy  = false;
        node->right = remove_node(node->right, succ->key, dummy);
    }
    return rebalance(node);
}

bool AVLTree::remove(const std::string& key) {
    bool removed = false;
    root_ = remove_node(root_, key, removed);
    if (removed) count_--;
    return removed;
}

// ============================================================
// Search
// ============================================================

bool AVLTree::search(const std::string& key, std::string* out) const {
    Node* cur = root_;
    while (cur) {
        if (key < cur->key)      cur = cur->left;
        else if (key > cur->key) cur = cur->right;
        else {
            if (out) *out = cur->value;
            return true;
        }
    }
    return false;
}

bool AVLTree::contains(const std::string& key) const {
    return search(key, nullptr);
}

// ============================================================
// Utilities
// ============================================================

bool AVLTree::empty() const { return count_ == 0; }
size_t AVLTree::size() const { return count_; }

void AVLTree::clear() {
    destroy(root_);
    root_  = nullptr;
    count_ = 0;
}

void AVLTree::inorder_rec(Node* n,
                          const std::function<void(const std::string&, const std::string&)>& cb) const {
    if (!n) return;
    inorder_rec(n->left, cb);
    cb(n->key, n->value);
    inorder_rec(n->right, cb);
}

void AVLTree::inorder(const std::function<void(const std::string&, const std::string&)>& cb) const {
    inorder_rec(root_, cb);
}

std::vector<std::string> AVLTree::keys_in_order() const {
    std::vector<std::string> result;
    result.reserve(count_);
    inorder([&result](const std::string& k, const std::string&) {
        result.push_back(k);
    });
    return result;
}

// ============================================================
// Serialization
// ============================================================

void AVLTree::write_str(std::ostream& os, const std::string& s) {
    uint32_t len = static_cast<uint32_t>(s.size());
    os.write(reinterpret_cast<const char*>(&len), sizeof(len));
    os.write(s.data(), s.size());
}

bool AVLTree::read_str(std::istream& is, std::string& s) {
    uint32_t len = 0;
    is.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (!is || len > (1u << 28)) return false;   // sanity cap: 256 MB
    s.resize(len);
    if (len > 0) is.read(&s[0], len);
    return static_cast<bool>(is);
}

void AVLTree::collect_pairs_rec(Node* n,
                                std::vector<std::pair<std::string,std::string>>& out) const {
    if (!n) return;
    collect_pairs_rec(n->left, out);
    out.emplace_back(n->key, n->value);
    collect_pairs_rec(n->right, out);
}

bool AVLTree::save(const std::string& path) const {
    // Refuse to serialize if node count exceeds what the uint32_t header
    // field can represent — writing a truncated count would corrupt the file.
    if (count_ > UINT32_MAX) return false;

    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs) return false;

    uint32_t n = static_cast<uint32_t>(count_);
    ofs.write(reinterpret_cast<const char*>(&n), sizeof(n));

    std::vector<std::pair<std::string,std::string>> pairs;
    pairs.reserve(count_);
    collect_pairs_rec(root_, pairs);
    for (auto& [k, v] : pairs) {
        write_str(ofs, k);
        write_str(ofs, v);
    }
    return static_cast<bool>(ofs);
}

bool AVLTree::load(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;

    uint32_t n = 0;
    ifs.read(reinterpret_cast<char*>(&n), sizeof(n));
    if (!ifs) return false;

    // Pre-flight sanity check: each entry requires at least 8 bytes
    // (two uint32_t length fields, even for empty strings). If the
    // remaining file is too small for the claimed entry count, the
    // header is lying — abort before driving allocations.
    std::streampos hdr_end = ifs.tellg();
    ifs.seekg(0, std::ios::end);
    std::streampos file_end = ifs.tellg();
    ifs.seekg(hdr_end, std::ios::beg);

    if (file_end < hdr_end) return false;
    uint64_t remaining = static_cast<uint64_t>(file_end - hdr_end);
    if (static_cast<uint64_t>(n) * 8 > remaining) return false;

    clear();
    for (uint32_t i = 0; i < n; ++i) {
        std::string k, v;
        if (!read_str(ifs, k) || !read_str(ifs, v)) {
            clear();
            return false;
        }
        insert(k, v);
    }
    return true;
}
