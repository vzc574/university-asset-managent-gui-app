#include "dsaengine.h"
#include <algorithm>
#include <functional>
#include <queue>
#include <unordered_set>

DSAEngine::DSAEngine() {}

DSAEngine::~DSAEngine()
{
    // Free linked list nodes
    LLNode *cur = m_llHead;
    while (cur) {
        LLNode *next = cur->next;
        delete cur;
        cur = next;
    }
    // Free BST nodes
    bstClearHelper(m_bstRoot);
}

// ════════════════════════════════════════════════════════════
//  STACK OPERATIONS  (Ch. 4 — LIFO, Last In First Out)
//  Time: O(1) push/pop   Space: O(n)
// ════════════════════════════════════════════════════════════

void DSAEngine::pushUndo(const AssetSnapshot &snap)
{
    // Cap at MAX_STACK_SIZE by draining the bottom element — O(n)
    if ((int)m_undoStack.size() >= MAX_STACK_SIZE) {
        std::stack<AssetSnapshot> temp;
        while (m_undoStack.size() > 1) { temp.push(m_undoStack.top()); m_undoStack.pop(); }
        m_undoStack.pop();
        while (!temp.empty())          { m_undoStack.push(temp.top()); temp.pop(); }
    }
    m_undoStack.push(snap);   // O(1)
}

bool DSAEngine::canUndo()  const { return !m_undoStack.empty(); }
int  DSAEngine::undoSize() const { return (int)m_undoStack.size(); }

AssetSnapshot DSAEngine::popUndo()
{
    AssetSnapshot snap = m_undoStack.top();   // O(1)
    m_undoStack.pop();
    return snap;
}

void DSAEngine::pushRedo(const AssetSnapshot &snap) { m_redoStack.push(snap); }
bool DSAEngine::canRedo()  const { return !m_redoStack.empty(); }
int  DSAEngine::redoSize() const { return (int)m_redoStack.size(); }

AssetSnapshot DSAEngine::popRedo()
{
    AssetSnapshot snap = m_redoStack.top();
    m_redoStack.pop();
    return snap;
}

void DSAEngine::clearRedo()
{
    while (!m_redoStack.empty()) m_redoStack.pop();
}

// ════════════════════════════════════════════════════════════
//  SINGLY-LINKED LIST — ACTION LOG  (Ch. 3)
//  Custom FIFO: enqueue at tail O(1), dequeue from head O(1)
//  Space: O(n)   vs. array list: no wasted capacity slots
// ════════════════════════════════════════════════════════════

void DSAEngine::llEnqueue(const std::string &action)
{
    LLNode *node = new LLNode(action);   // allocate new node O(1)
    if (!m_llTail) {
        m_llHead = m_llTail = node;      // list was empty
    } else {
        m_llTail->next = node;           // link at tail O(1)
        m_llTail       = node;
    }
    ++m_llCount;
}

std::string DSAEngine::llDequeue()
{
    if (!m_llHead) return "";
    std::string val = m_llHead->data;
    LLNode *old = m_llHead;
    m_llHead    = m_llHead->next;        // advance head O(1)
    if (!m_llHead) m_llTail = nullptr;
    delete old;
    --m_llCount;
    return val;
}

bool DSAEngine::llIsEmpty() const { return m_llCount == 0; }
int  DSAEngine::llSize()    const { return m_llCount; }

std::string DSAEngine::llContents() const
{
    std::string result = "Action Log — Linked List (head→tail):\n";
    LLNode *cur = m_llHead;
    int i = 1;
    while (cur) {
        result += "  " + std::to_string(i++) + ". " + cur->data + "\n";
        cur = cur->next;
    }
    if (m_llCount == 0) result += "  (empty)\n";
    return result;
}

// ════════════════════════════════════════════════════════════
//  BINARY SEARCH TREE — SERIAL NUMBER INDEX  (Ch. 6)
//  Insert: O(log n) average, O(n) worst (unbalanced)
//  Search: O(log n) average, O(n) worst
//  Space:  O(n)
//  Used by SecurityCheckWindow to check serial before DB query
// ════════════════════════════════════════════════════════════

void DSAEngine::bstInsert(const std::string &serial, int itemId)
{
    m_bstRoot = bstInsertHelper(m_bstRoot, serial, itemId);
    ++m_bstCount;
}

BSTNode *DSAEngine::bstInsertHelper(BSTNode *node, const std::string &serial, int itemId)
{
    if (!node) return new BSTNode(serial, itemId);   // base case

    if (serial < node->serial)
        node->left  = bstInsertHelper(node->left,  serial, itemId);
    else if (serial > node->serial)
        node->right = bstInsertHelper(node->right, serial, itemId);
    // duplicate serial: ignore (same item already indexed)
    return node;
}

bool DSAEngine::bstSearch(const std::string &serial, int &outItemId) const
{
    return bstSearchHelper(m_bstRoot, serial, outItemId);
}

bool DSAEngine::bstSearchHelper(BSTNode *node, const std::string &serial, int &outId) const
{
    if (!node) return false;                         // not found
    if (serial == node->serial) { outId = node->itemId; return true; }
    if (serial < node->serial)
        return bstSearchHelper(node->left,  serial, outId);
    return bstSearchHelper(node->right, serial, outId);
}

void DSAEngine::bstClear()
{
    bstClearHelper(m_bstRoot);
    m_bstRoot  = nullptr;
    m_bstCount = 0;
}

void DSAEngine::bstClearHelper(BSTNode *node)
{
    if (!node) return;
    bstClearHelper(node->left);    // recurse left
    bstClearHelper(node->right);   // recurse right
    delete node;
}

int DSAEngine::bstSize()   const { return m_bstCount; }

int DSAEngine::bstHeight() const { return bstHeightHelper(m_bstRoot); }

int DSAEngine::bstHeightHelper(BSTNode *node) const
{
    if (!node) return 0;
    int l = bstHeightHelper(node->left);
    int r = bstHeightHelper(node->right);
    return 1 + std::max(l, r);
}

std::string DSAEngine::bstInorder() const
{
    std::string out = "BST Serial Index — In-Order (sorted):\n";
    int count = 0;
    bstInorderHelper(m_bstRoot, out, count);
    if (count == 0) out += "  (empty)\n";
    return out;
}

void DSAEngine::bstInorderHelper(BSTNode *node, std::string &out, int &count) const
{
    if (!node) return;
    bstInorderHelper(node->left, out, count);
    if (count < 20)   // show first 20 to keep output readable
        out += "  " + std::to_string(++count) + ". [" + std::to_string(node->itemId) + "] " + node->serial + "\n";
    else if (count == 20)
        out += "  ... (" + std::to_string(m_bstCount - 20) + " more)\n";
    bstInorderHelper(node->right, out, count);
}

// ════════════════════════════════════════════════════════════
//  BUBBLE SORT  (Ch. 2 & 8)
//  Time: O(n²) best/average/worst   Space: O(1) in-place
//  Compare adjacent pairs, bubble largest to end each pass
// ════════════════════════════════════════════════════════════

void DSAEngine::bubbleSortById(std::vector<AssetRecord> &assets)
{
    int n = (int)assets.size();
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (assets[j].id > assets[j+1].id) {
                AssetRecord tmp = assets[j];
                assets[j]       = assets[j+1];
                assets[j+1]     = tmp;
            }
        }
    }
}

void DSAEngine::bubbleSortByValue(std::vector<AssetRecord> &assets)
{
    int n = (int)assets.size();
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (assets[j].value > assets[j+1].value) {
                AssetRecord tmp = assets[j];
                assets[j]       = assets[j+1];
                assets[j+1]     = tmp;
            }
        }
    }
}

// ════════════════════════════════════════════════════════════
//  SELECTION SORT  (Ch. 2)
//  Time: O(n²) all cases   Space: O(1) in-place
//  Find minimum in unsorted portion, swap into position
// ════════════════════════════════════════════════════════════

void DSAEngine::selectionSortByName(std::vector<AssetRecord> &assets)
{
    int n = (int)assets.size();
    for (int i = 0; i < n - 1; i++) {
        int minIdx = i;
        for (int j = i + 1; j < n; j++) {
            if (assets[j].name < assets[minIdx].name)
                minIdx = j;
        }
        if (minIdx != i) {
            AssetRecord tmp   = assets[i];
            assets[i]         = assets[minIdx];
            assets[minIdx]    = tmp;
        }
    }
}

void DSAEngine::selectionSortByLocation(std::vector<AssetRecord> &assets)
{
    int n = (int)assets.size();
    for (int i = 0; i < n - 1; i++) {
        int minIdx = i;
        for (int j = i + 1; j < n; j++) {
            if (assets[j].location < assets[minIdx].location)
                minIdx = j;
        }
        if (minIdx != i) {
            AssetRecord tmp   = assets[i];
            assets[i]         = assets[minIdx];
            assets[minIdx]    = tmp;
        }
    }
}

// ════════════════════════════════════════════════════════════
//  MERGE SORT — Divide & Conquer  (Ch. 8 & 9)
//  Recurrence: T(n) = 2T(n/2) + O(n)  →  O(n log n) all cases
//  Space: O(n) auxiliary (merge buffer)
//
//  Strategy:
//    Divide  — split array in half recursively until size == 1
//    Conquer — merge two sorted halves into a sorted whole
// ════════════════════════════════════════════════════════════

void DSAEngine::mergeSortByValue(std::vector<AssetRecord> &assets)
{
    if (assets.size() > 1)
        mergeSortHelperByValue(assets, 0, (int)assets.size() - 1);
}

void DSAEngine::mergeSortHelperByValue(std::vector<AssetRecord> &a, int l, int r)
{
    if (l >= r) return;                          // base case: single element
    int m = l + (r - l) / 2;                    // divide: find midpoint
    mergeSortHelperByValue(a, l, m);             // conquer: sort left half
    mergeSortHelperByValue(a, m + 1, r);         // conquer: sort right half
    mergeByValue(a, l, m, r);                    // combine: merge sorted halves
}

void DSAEngine::mergeByValue(std::vector<AssetRecord> &a, int l, int m, int r)
{
    std::vector<AssetRecord> left (a.begin() + l,     a.begin() + m + 1);
    std::vector<AssetRecord> right(a.begin() + m + 1, a.begin() + r + 1);

    int i = 0, j = 0, k = l;
    while (i < (int)left.size() && j < (int)right.size())
        a[k++] = (left[i].value <= right[j].value) ? left[i++] : right[j++];
    while (i < (int)left.size())  a[k++] = left[i++];
    while (j < (int)right.size()) a[k++] = right[j++];
}

void DSAEngine::mergeSortByName(std::vector<AssetRecord> &assets)
{
    if (assets.size() > 1)
        mergeSortHelperByName(assets, 0, (int)assets.size() - 1);
}

void DSAEngine::mergeSortHelperByName(std::vector<AssetRecord> &a, int l, int r)
{
    if (l >= r) return;
    int m = l + (r - l) / 2;
    mergeSortHelperByName(a, l, m);
    mergeSortHelperByName(a, m + 1, r);
    mergeByName(a, l, m, r);
}

void DSAEngine::mergeByName(std::vector<AssetRecord> &a, int l, int m, int r)
{
    std::vector<AssetRecord> left (a.begin() + l,     a.begin() + m + 1);
    std::vector<AssetRecord> right(a.begin() + m + 1, a.begin() + r + 1);

    int i = 0, j = 0, k = l;
    while (i < (int)left.size() && j < (int)right.size())
        a[k++] = (left[i].name <= right[j].name) ? left[i++] : right[j++];
    while (i < (int)left.size())  a[k++] = left[i++];
    while (j < (int)right.size()) a[k++] = right[j++];
}

// ════════════════════════════════════════════════════════════
//  GRAPH — Adjacency List  (Ch. 7)
//  Models DBU campus buildings as an undirected graph.
//
//  BFS (Breadth-First Search):
//    Visits nodes level by level using a queue.
//    Guarantees shortest path in an unweighted graph.
//    Time: O(V + E)   Space: O(V)
//
//  DFS (Depth-First Search):
//    Explores as deep as possible before backtracking (uses recursion/stack).
//    Used for connectivity checks and full traversal.
//    Time: O(V + E)   Space: O(V)
// ════════════════════════════════════════════════════════════

void DSAEngine::graphAddEdge(const std::string &u, const std::string &v)
{
    m_graph[u].push_back(v);   // undirected: add both directions
    m_graph[v].push_back(u);
}

void DSAEngine::graphInit()
{
    m_graph.clear();

    // DBU campus building connections (undirected)
    graphAddEdge("Main Gate",              "Library");
    graphAddEdge("Main Gate",              "Admin Block");
    graphAddEdge("Library",               "College of Computing");
    graphAddEdge("Library",               "Cafeteria");
    graphAddEdge("Admin Block",           "College of Computing");
    graphAddEdge("Admin Block",           "Lab Block");
    graphAddEdge("College of Computing",  "Lab Block");
    graphAddEdge("Cafeteria",             "Sports Complex");
    graphAddEdge("Cafeteria",             "Dormitory A");
    graphAddEdge("Sports Complex",        "Dormitory B");
    graphAddEdge("Dormitory A",           "Medical Center");
    graphAddEdge("Dormitory B",           "Medical Center");
}

// BFS — returns the shortest path from start to end as a list of nodes.
// Uses a queue: enqueue start, then explore neighbors level by level.
std::vector<std::string> DSAEngine::graphBFS(const std::string &start,
                                              const std::string &end) const
{
    if (m_graph.find(start) == m_graph.end()) return {};

    std::unordered_map<std::string, std::string> parent;  // child → parent
    std::queue<std::string>                       bfsQ;
    std::unordered_set<std::string>               visited;

    bfsQ.push(start);
    visited.insert(start);
    parent[start] = "";

    while (!bfsQ.empty()) {
        std::string node = bfsQ.front();
        bfsQ.pop();

        if (node == end) {
            // Reconstruct path by walking parent pointers
            std::vector<std::string> path;
            for (std::string cur = end; cur != ""; cur = parent[cur])
                path.push_back(cur);
            std::reverse(path.begin(), path.end());
            return path;
        }

        auto it = m_graph.find(node);
        if (it == m_graph.end()) continue;
        for (const std::string &nb : it->second) {
            if (!visited.count(nb)) {
                visited.insert(nb);
                parent[nb] = node;
                bfsQ.push(nb);
            }
        }
    }
    return {};   // no path found
}

// DFS — returns all nodes reachable from start in DFS visit order.
// Uses recursion (implicit call stack).
std::vector<std::string> DSAEngine::graphDFS(const std::string &start) const
{
    std::vector<std::string>        visited;
    std::unordered_set<std::string> seen;

    std::function<void(const std::string &)> dfs =
        [&](const std::string &node) {
            seen.insert(node);
            visited.push_back(node);
            auto it = m_graph.find(node);
            if (it == m_graph.end()) return;
            for (const std::string &nb : it->second)
                if (!seen.count(nb))
                    dfs(nb);
        };

    if (m_graph.count(start))
        dfs(start);
    return visited;
}

std::vector<std::string> DSAEngine::graphNodes() const
{
    std::vector<std::string> nodes;
    for (const auto &kv : m_graph)
        nodes.push_back(kv.first);
    std::sort(nodes.begin(), nodes.end());
    return nodes;
}

std::string DSAEngine::graphAdjList() const
{
    std::string out = "Campus Graph — Adjacency List:\n";
    auto nodes = graphNodes();
    for (const std::string &node : nodes) {
        out += "  " + node + " → ";
        const auto &nb = m_graph.at(node);
        for (int i = 0; i < (int)nb.size(); i++) {
            if (i) out += ", ";
            out += nb[i];
        }
        out += "\n";
    }
    return out;
}

// ════════════════════════════════════════════════════════════
//  STACK CONTENT INFO (for report/presentation screen)
// ════════════════════════════════════════════════════════════

std::string DSAEngine::getUndoStackContents() const
{
    std::stack<AssetSnapshot> temp = m_undoStack;
    std::string result = "Undo Stack (top→bottom):\n";
    int i = 1;
    while (!temp.empty()) {
        AssetSnapshot s = temp.top(); temp.pop();
        std::string act = (s.action == ActionType::ADD_ASSET)    ? "ADD"
                        : (s.action == ActionType::DELETE_ASSET) ? "DELETE" : "EDIT";
        result += "  " + std::to_string(i++) + ". [" + act + "] " + s.assetName + "\n";
    }
    return result;
}

std::string DSAEngine::getRedoStackContents() const
{
    std::stack<AssetSnapshot> temp = m_redoStack;
    std::string result = "Redo Stack (top→bottom):\n";
    int i = 1;
    while (!temp.empty()) {
        AssetSnapshot s = temp.top(); temp.pop();
        std::string act = (s.action == ActionType::ADD_ASSET)    ? "ADD"
                        : (s.action == ActionType::DELETE_ASSET) ? "DELETE" : "EDIT";
        result += "  " + std::to_string(i++) + ". [" + act + "] " + s.assetName + "\n";
    }
    return result;
}
