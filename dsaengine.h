#pragma once
#include <vector>
#include <stack>
#include <string>
#include <unordered_map>

struct AssetRecord {
    int id;
    std::string name;
    std::string category;
    std::string condition;
    std::string serialNumber;
    double value;
    std::string location;
};

enum class ActionType { ADD_ASSET, DELETE_ASSET, EDIT_ASSET };

struct AssetSnapshot {
    int assetId = 0;

    std::string assetName;
    std::string category;
    std::string subcategory;
    std::string condition;
    std::string serialNumber;

    double purchaseValue = 0;

    std::string newAssetName;
    std::string newCategory;
    std::string newSubcategory;
    std::string newCondition;
    std::string newSerialNumber;
    double newPurchaseValue = 0;

    ActionType action = ActionType::ADD_ASSET;
};

// ── Singly-Linked List Node (Ch. 3) ───────────────────────
// Used for action log: O(1) append at tail, O(1) remove from head
struct LLNode {
    std::string data;
    LLNode     *next = nullptr;
    explicit LLNode(const std::string &d) : data(d) {}
};

// ── BST Node (Ch. 6) ──────────────────────────────────────
// Indexes student items by serial number for O(log n) lookup
struct BSTNode {
    std::string serial;
    int         itemId;
    BSTNode    *left  = nullptr;
    BSTNode    *right = nullptr;
    BSTNode(const std::string &s, int id) : serial(s), itemId(id) {}
};

class DSAEngine {
public:
    static const int MAX_STACK_SIZE = 20;

    DSAEngine();
    ~DSAEngine();

    // ── Stack operations (Ch. 4) ──────────────────────────
    void pushUndo(const AssetSnapshot &snap);
    AssetSnapshot popUndo();
    bool canUndo() const;
    int  undoSize() const;

    void pushRedo(const AssetSnapshot &snap);
    AssetSnapshot popRedo();
    bool canRedo() const;
    int  redoSize() const;
    void clearRedo();

    // ── Linked List action log (Ch. 3) ────────────────────
    // Replaces std::queue — custom FIFO via singly-linked list
    void        llEnqueue(const std::string &action);   // append at tail O(1)
    std::string llDequeue();                            // remove from head O(1)
    bool        llIsEmpty() const;
    int         llSize() const;
    std::string llContents() const;                     // for report screen
    bool        hasActions() const { return !llIsEmpty(); }

    // ── BST serial-number index (Ch. 6) ───────────────────
    void        bstInsert(const std::string &serial, int itemId);
    bool        bstSearch(const std::string &serial, int &outItemId) const;
    void        bstClear();
    int         bstSize() const;
    int         bstHeight() const;
    std::string bstInorder() const;                     // for report screen

    // ── Sorting (Ch. 2 & 8) ───────────────────────────────
    void bubbleSortById(std::vector<AssetRecord> &assets);
    void bubbleSortByValue(std::vector<AssetRecord> &assets);
    void selectionSortByName(std::vector<AssetRecord> &assets);
    void selectionSortByLocation(std::vector<AssetRecord> &assets);

    // ── Merge Sort — Divide & Conquer (Ch. 8 & 9) ────────
    // T(n) = 2T(n/2) + O(n)  →  O(n log n) all cases, O(n) space
    void mergeSortByValue(std::vector<AssetRecord> &assets);
    void mergeSortByName(std::vector<AssetRecord> &assets);

    // ── Graph — Adjacency List (Ch. 7) ────────────────────
    // Undirected graph of DBU campus buildings
    // BFS shortest path: O(V + E)   DFS traversal: O(V + E)
    void graphInit();                                        // populate DBU campus
    void graphAddEdge(const std::string &u, const std::string &v);
    std::vector<std::string> graphBFS(const std::string &start,
                                      const std::string &end) const;
    std::vector<std::string> graphDFS(const std::string &start) const;
    std::vector<std::string> graphNodes() const;
    std::string              graphAdjList() const;

    // ── Stack content info (for report screen) ────────────
    std::string getUndoStackContents() const;
    std::string getRedoStackContents() const;

    // Legacy enqueue/dequeue aliases (keep old callers working)
    void        enqueueAction(const std::string &action) { llEnqueue(action); }
    std::string dequeueAction()                          { return llDequeue(); }
    bool        isQueueEmpty() const                     { return llIsEmpty(); }
    int         queueSize() const                        { return llSize(); }

private:
    // Stack members
    std::stack<AssetSnapshot> m_undoStack;
    std::stack<AssetSnapshot> m_redoStack;

    // Linked list members (head + tail for O(1) append)
    LLNode *m_llHead = nullptr;
    LLNode *m_llTail = nullptr;
    int     m_llCount = 0;

    // BST members
    BSTNode *m_bstRoot  = nullptr;
    int      m_bstCount = 0;

    // BST helpers (recursive)
    BSTNode    *bstInsertHelper(BSTNode *node, const std::string &serial, int itemId);
    bool        bstSearchHelper(BSTNode *node, const std::string &serial, int &outId) const;
    void        bstClearHelper(BSTNode *node);
    int         bstHeightHelper(BSTNode *node) const;
    void        bstInorderHelper(BSTNode *node, std::string &out, int &count) const;

    // Merge Sort helpers (recursive divide & conquer)
    void mergeByValue(std::vector<AssetRecord> &a, int l, int m, int r);
    void mergeSortHelperByValue(std::vector<AssetRecord> &a, int l, int r);
    void mergeByName(std::vector<AssetRecord> &a, int l, int m, int r);
    void mergeSortHelperByName(std::vector<AssetRecord> &a, int l, int r);

    // Graph adjacency list
    std::unordered_map<std::string, std::vector<std::string>> m_graph;
};
