#ifndef ZHUZIBINARYTREE_H
#define ZHUZIBINARYTREE_H

#include <functional>
#include <queue>
#include <vector>
#include <stack>
#include <stdexcept>
#include <utility>

template<typename T>
class zhuziBinaryTree {
public:
    // ---------- 公开节点结构 ----------
    struct Node {
        T data;
        Node* left;
        Node* right;
        Node(const T& val) : data(val), left(nullptr), right(nullptr) {}
    };

    // ---------- 中序遍历迭代器 ----------
    class iterator {
    public:
        using value_type = T;
        using pointer = T*;
        using reference = T&;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

        iterator() : m_stack() {}
        explicit iterator(Node* root) {
            if (root) pushLeft(root);
        }

        reference operator*() const {
            if (m_stack.empty()) throw std::out_of_range("Dereferencing end iterator");
            return m_stack.top()->data;
        }
        pointer operator->() const {
            return &(operator*());
        }
        iterator& operator++() {
            if (m_stack.empty()) return *this;
            Node* node = m_stack.top();
            m_stack.pop();
            if (node->right) pushLeft(node->right);
            return *this;
        }
        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        bool operator==(const iterator& other) const {
            if (m_stack.empty() && other.m_stack.empty()) return true;
            if (m_stack.empty() || other.m_stack.empty()) return false;
            return m_stack.top() == other.m_stack.top();
        }
        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }

    private:
        std::stack<Node*> m_stack;
        void pushLeft(Node* node) {
            while (node) {
                m_stack.push(node);
                node = node->left;
            }
        }
    };

    // ---------- 构造/析构/拷贝/移动 ----------
    zhuziBinaryTree() : root_(nullptr), node_count_(0) {}
    zhuziBinaryTree(const zhuziBinaryTree& other) : root_(nullptr), node_count_(other.node_count_) {
        root_ = copyTree(other.root_);
    }
    zhuziBinaryTree(zhuziBinaryTree&& other) noexcept
        : root_(other.root_), node_count_(other.node_count_) {
        other.root_ = nullptr;
        other.node_count_ = 0;
    }
    ~zhuziBinaryTree() {
        clearTree(root_);
    }

    zhuziBinaryTree& operator=(const zhuziBinaryTree& other) {
        if (this != &other) {
            zhuziBinaryTree temp(other);
            swap(temp);
        }
        return *this;
    }
    zhuziBinaryTree& operator=(zhuziBinaryTree&& other) noexcept {
        if (this != &other) {
            clearTree(root_);
            root_ = other.root_;
            node_count_ = other.node_count_;
            other.root_ = nullptr;
            other.node_count_ = 0;
        }
        return *this;
    }
    void swap(zhuziBinaryTree& other) noexcept {
        std::swap(root_, other.root_);
        std::swap(node_count_, other.node_count_);
    }

    // ---------- BST 操作 ----------
    bool insert(const T& value) {
        bool inserted = false;
        root_ = insertNode(root_, value, inserted);
        if (inserted) ++node_count_;
        return inserted;
    }

    bool erase(const T& value) {
        bool erased = false;
        root_ = eraseNode(root_, value, erased);
        if (erased) --node_count_;
        return erased;
    }

    bool contains(const T& value) const {
        return containsNode(root_, value);
    }

    size_t size() const { return node_count_; }
    bool empty() const { return node_count_ == 0; }
    size_t height() const { return getHeight(root_); }
    void clear() {
        clearTree(root_);
        node_count_ = 0;
    }

    const T& getMin() const {
        if (empty()) throw std::out_of_range("Tree is empty");
        Node* node = findMinNode(root_);
        return node->data;
    }
    const T& getMax() const {
        if (empty()) throw std::out_of_range("Tree is empty");
        Node* node = findMaxNode(root_);
        return node->data;
    }

    // ---------- 迭代器 ----------
    iterator begin() { return iterator(root_); }
    iterator end() { return iterator(); }

    // ---------- 搜索 ----------
    iterator find(const T& value) {
        Node* cur = root_;
        while (cur) {
            if (value < cur->data) cur = cur->left;
            else if (cur->data < value) cur = cur->right;
            else return iterator(cur);
        }
        return end();
    }

    iterator lower_bound(const T& value) {
        Node* cur = root_;
        Node* result = nullptr;
        while (cur) {
            if (!(cur->data < value)) {
                result = cur;
                cur = cur->left;
            }
            else {
                cur = cur->right;
            }
        }
        return result ? iterator(result) : end();
    }

    iterator upper_bound(const T& value) {
        Node* cur = root_;
        Node* result = nullptr;
        while (cur) {
            if (value < cur->data) {
                result = cur;
                cur = cur->left;
            }
            else {
                cur = cur->right;
            }
        }
        return result ? iterator(result) : end();
    }

    // ---------- 结构访问（供外部遍历树形结构使用）----------
    const Node* getRoot() const { return root_; }
    Node* getRoot() { return root_; }

    // ---------- 手动构建树接口（不破坏 BST 性质的前提下允许直接操作）----------
    // 创建一个新节点（仅分配内存，不插入树中）
    static Node* createNode(const T& value) {
        return new Node(value);
    }

    // 设置根节点（仅在树为空时有效，否则抛出异常）
    void setRoot(Node* node) {
        if (root_ != nullptr) {
            throw std::runtime_error("Tree already has a root, clear it first or use manual assignment");
        }
        root_ = node;
        // 重新计算节点数（递归统计）
        node_count_ = countNodes(root_);
    }

    // 设置节点的左孩子（不进行任何比较，直接赋值）
    void setLeft(Node* parent, Node* child) {
        if (!parent) throw std::invalid_argument("Parent cannot be null");
        parent->left = child;
        // 更新节点计数（简单起见重新统计全部）
        node_count_ = countNodes(root_);
    }

    // 设置节点的右孩子
    void setRight(Node* parent, Node* child) {
        if (!parent) throw std::invalid_argument("Parent cannot be null");
        parent->right = child;
        node_count_ = countNodes(root_);
    }

    // 重新计算节点数（如果手动修改了树结构，可调用此函数）
    void updateNodeCount() {
        node_count_ = countNodes(root_);
    }

    // 遍历（回调）
    void preorder(std::function<void(const T&)> func) const { preorderRec(root_, func); }
    void inorder(std::function<void(const T&)> func) const { inorderRec(root_, func); }
    void postorder(std::function<void(const T&)> func) const { postorderRec(root_, func); }
    void levelorder(std::function<void(const T&)> func) const { levelorderRec(func); }

    // 遍历（返回 vector）
    std::vector<T> preorder() const {
        std::vector<T> res;
        preorder([&](const T& v) { res.push_back(v); });
        return res;
    }
    std::vector<T> inorder() const {
        std::vector<T> res;
        inorder([&](const T& v) { res.push_back(v); });
        return res;
    }
    std::vector<T> postorder() const {
        std::vector<T> res;
        postorder([&](const T& v) { res.push_back(v); });
        return res;
    }
    std::vector<T> levelorder() const {
        std::vector<T> res;
        levelorder([&](const T& v) { res.push_back(v); });
        return res;
    }

private:
    Node* root_;
    size_t node_count_;

    // ---------- 辅助函数 ----------
    Node* copyTree(Node* node) {
        if (!node) return nullptr;
        Node* newNode = new Node(node->data);
        newNode->left = copyTree(node->left);
        newNode->right = copyTree(node->right);
        return newNode;
    }

    void clearTree(Node*& node) {
        if (!node) return;
        clearTree(node->left);
        clearTree(node->right);
        delete node;
        node = nullptr;
    }

    Node* insertNode(Node* node, const T& value, bool& inserted) {
        if (!node) {
            inserted = true;
            return new Node(value);
        }
        if (value < node->data) {
            node->left = insertNode(node->left, value, inserted);
        }
        else if (node->data < value) {
            node->right = insertNode(node->right, value, inserted);
        }
        else {
            inserted = false;
        }
        return node;
    }

    Node* findMinNode(Node* node) const {
        while (node && node->left) node = node->left;
        return node;
    }

    Node* findMaxNode(Node* node) const {
        while (node && node->right) node = node->right;
        return node;
    }

    Node* eraseNode(Node* node, const T& value, bool& erased) {
        if (!node) {
            erased = false;
            return nullptr;
        }
        if (value < node->data) {
            node->left = eraseNode(node->left, value, erased);
        }
        else if (node->data < value) {
            node->right = eraseNode(node->right, value, erased);
        }
        else {
            erased = true;
            if (!node->left) {
                Node* right = node->right;
                delete node;
                return right;
            }
            else if (!node->right) {
                Node* left = node->left;
                delete node;
                return left;
            }
            else {
                Node* minRight = findMinNode(node->right);
                node->data = minRight->data;
                bool dummy = false;
                node->right = eraseNode(node->right, minRight->data, dummy);
            }
        }
        return node;
    }

    bool containsNode(Node* node, const T& value) const {
        if (!node) return false;
        if (value < node->data) return containsNode(node->left, value);
        if (node->data < value) return containsNode(node->right, value);
        return true;
    }

    // 使用条件运算符，避免 std::max 与 Windows 宏冲突
    size_t getHeight(Node* node) const {
        if (!node) return 0;
        size_t leftH = getHeight(node->left);
        size_t rightH = getHeight(node->right);
        return 1 + (leftH > rightH ? leftH : rightH);
    }

    size_t countNodes(Node* node) const {
        if (!node) return 0;
        return 1 + countNodes(node->left) + countNodes(node->right);
    }

    void preorderRec(Node* node, std::function<void(const T&)>& func) const {
        if (!node) return;
        func(node->data);
        preorderRec(node->left, func);
        preorderRec(node->right, func);
    }

    void inorderRec(Node* node, std::function<void(const T&)>& func) const {
        if (!node) return;
        inorderRec(node->left, func);
        func(node->data);
        inorderRec(node->right, func);
    }

    void postorderRec(Node* node, std::function<void(const T&)>& func) const {
        if (!node) return;
        postorderRec(node->left, func);
        postorderRec(node->right, func);
        func(node->data);
    }

    void levelorderRec(std::function<void(const T&)>& func) const {
        if (!root_) return;
        std::queue<Node*> q;
        q.push(root_);
        while (!q.empty()) {
            Node* cur = q.front(); q.pop();
            func(cur->data);
            if (cur->left) q.push(cur->left);
            if (cur->right) q.push(cur->right);
        }
    }
};

// 非成员 swap
template<typename T>
void swap(zhuziBinaryTree<T>& lhs, zhuziBinaryTree<T>& rhs) noexcept {
    lhs.swap(rhs);
}

#endif // ZHUZIBINARYTREE_H