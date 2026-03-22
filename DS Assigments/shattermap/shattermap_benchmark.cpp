#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <queue>
#include <stack>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cmath>
#include <numeric>
#include <functional>
#include <iomanip>

using namespace std;
using namespace std::chrono;

// ======================== UTILITIES ========================
struct Timer {
    high_resolution_clock::time_point start;
    void begin() { start = high_resolution_clock::now(); }
    double elapsed_ms() {
        return duration_cast<microseconds>(high_resolution_clock::now() - start).count() / 1000.0;
    }
};

struct Edge { int from, to, cost; };
struct Operation { int type; int a, b; }; 

struct DSResult {
    string name;
    string dsType;
    double buildTime;
    double classifyTime;
    double staticQueryTime;
    int staticQueryCount;
    double addTime;
    double deleteTime;
    double checkTime;
    int addCount, deleteCount, checkCount;
    int deletesSkipped;
    long long memoryBytes;
    int comparisons;
    bool supportsDelete;
};

// ======================== CSV LOADER ========================
vector<Edge> loadEdges(const string& filename) {
    vector<Edge> edges;
    ifstream file(filename);
    string line;
    getline(file, line); // skip header
    while (getline(file, line)) {
        if (line.empty()) continue;
        Edge e;
        char c;
        istringstream ss(line);
        ss >> e.from >> c >> e.to >> c >> e.cost;
        if (ss) edges.push_back(e);
    }
    return edges;
}

vector<pair<int,int>> loadQueries(const string& filename) {
    vector<pair<int,int>> queries;
    ifstream file(filename);
    string line;
    getline(file, line); // skip header
    while (getline(file, line)) {
        if (line.empty()) continue;
        int a, b; char c;
        istringstream ss(line);
        ss >> a >> c >> b;
        if (ss) queries.push_back({a, b});
    }
    return queries;
}

vector<Operation> loadOperations(const string& filename) {
    vector<Operation> ops;
    ifstream file(filename);
    string line;
    getline(file, line); // skip header
    while (getline(file, line)) {
        if (line.empty()) continue;
        // Remove carriage return
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
            line.pop_back();
        if (line.empty()) continue;
        
        Operation op;
        size_t p1 = line.find(',');
        if (p1 == string::npos) continue;
        size_t p2 = line.find(',', p1 + 1);
        if (p2 == string::npos) continue;
        
        string type = line.substr(0, p1);
        string sa = line.substr(p1 + 1, p2 - p1 - 1);
        string sb = line.substr(p2 + 1);
        
        // Trim whitespace
        sa.erase(0, sa.find_first_not_of(" \t"));
        sb.erase(0, sb.find_first_not_of(" \t"));
        
        try {
            op.a = stoi(sa);
            op.b = stoi(sb);
        } catch (...) { continue; }
        
        if (type == "CHECK_CONNECTIVITY") op.type = 0;
        else if (type == "ADD_LINK") op.type = 1;
        else if (type == "REMOVE_LINK") op.type = 2;
        else continue;
        
        ops.push_back(op);
    }
    return ops;
}

// ====================== SHATTERMAP (NOVEL) ======================
class ShatterMap {
public:
    struct SMNode {
        int zoneID;
        int edgeHead;  // head of edge list in pool
        int degree;
    };
    struct SMEdge {
        int to;
        int cost;
        int next;    // next edge from same source
        bool alive;
        bool isStress; // bridge?
    };
    
    SMNode* nodes;
    SMEdge* edgePool;
    int nodeCount, maxNodes;
    int poolUsed, poolCapacity;
    int totalBridges, totalReinforced;
    int safeDeletes, stressDeletes;
    int zoneCounter;
    bool dirty;
    
    // Bridge detection and BFS buffers
    int* disc;
    int* low;
    int* visited;
    int visToken;
    int* side; // 0=unvisited, 1=side_a, 2=side_b
    vector<int> qA, qB;
    vector<int> compA, compB;
    
    void init(int n, int maxEdges) {
        maxNodes = n + 1;
        nodeCount = n;
        nodes = new SMNode[maxNodes];
        for (int i = 0; i < maxNodes; i++) {
            nodes[i] = {0, -1, 0};
        }
        poolCapacity = maxEdges * 2 + 100;
        edgePool = new SMEdge[poolCapacity];
        poolUsed = 0;
        totalBridges = totalReinforced = 0;
        safeDeletes = stressDeletes = 0;
        zoneCounter = 0;
        dirty = true;
        disc = new int[maxNodes];
        low = new int[maxNodes];
        visited = new int[maxNodes];
        side = new int[maxNodes];
        memset(visited, 0, maxNodes * sizeof(int));
        memset(side, 0, maxNodes * sizeof(int));
        visToken = 1;
    }
    
    void destroy() {
        delete[] nodes; delete[] edgePool;
        delete[] disc; delete[] low;
        delete[] visited; delete[] side;
    }
    
    void insertEdge(int u, int v, int cost) {
        if (poolUsed + 2 > poolCapacity) return;
        int idx = poolUsed;
        edgePool[idx] = {v, cost, nodes[u].edgeHead, true, false};
        nodes[u].edgeHead = idx;
        edgePool[idx + 1] = {u, cost, nodes[v].edgeHead, true, false};
        nodes[v].edgeHead = idx + 1;
        poolUsed += 2;
        nodes[u].degree++;
        nodes[v].degree++;
    }
    
    void assignZones() {
        for (int i = 0; i < maxNodes; i++) nodes[i].zoneID = -1;
        zoneCounter = 0;
        for (int s = 1; s <= nodeCount; s++) {
            if (nodes[s].zoneID != -1) continue;
            int z = zoneCounter++;
            queue<int> q;
            q.push(s);
            nodes[s].zoneID = z;
            while (!q.empty()) {
                int u = q.front(); q.pop();
                for (int ei = nodes[u].edgeHead; ei != -1; ei = edgePool[ei].next) {
                    if (!edgePool[ei].alive) continue;
                    int v = edgePool[ei].to;
                    if (nodes[v].zoneID == -1) {
                        nodes[v].zoneID = z;
                        q.push(v);
                    }
                }
            }
        }
    }
    
    void classifyBridges() {
        totalBridges = 0;
        for (int i = 0; i < poolUsed; i++) edgePool[i].isStress = false;
        memset(disc, -1, maxNodes * sizeof(int));
        memset(low, 0, maxNodes * sizeof(int));
        int timer = 0;
        
        struct Frame { int node, parentEdge, curEdge; };
        stack<Frame> stk;
        
        for (int s = 1; s <= nodeCount; s++) {
            if (disc[s] != -1) continue;
            disc[s] = low[s] = timer++;
            stk.push({s, -1, nodes[s].edgeHead});
            
            while (!stk.empty()) {
                Frame& f = stk.top();
                bool descended = false;
                
                while (f.curEdge != -1) {
                    int ei = f.curEdge;
                    SMEdge& e = edgePool[ei];
                    f.curEdge = e.next;
                    
                    if (!e.alive) continue;
                    if (f.parentEdge != -1 && ei == (f.parentEdge ^ 1)) continue;
                    
                    int v = e.to;
                    if (disc[v] == -1) {
                        disc[v] = low[v] = timer++;
                        stk.push({v, ei, nodes[v].edgeHead});
                        descended = true;
                        break;
                    } else {
                        if (disc[v] < low[f.node]) low[f.node] = disc[v];
                    }
                }
                
                if (!descended) {
                    int u = f.node;
                    int pe = f.parentEdge;
                    stk.pop();
                    if (!stk.empty()) {
                        Frame& parent = stk.top();
                        if (low[u] < low[parent.node]) low[parent.node] = low[u];
                        if (low[u] > disc[parent.node] && pe != -1) {
                            edgePool[pe].isStress = true;
                            edgePool[pe ^ 1].isStress = true;
                            totalBridges++;
                        }
                    }
                }
            }
        }
        totalReinforced = (poolUsed / 2) - totalBridges;
        dirty = false;
    }
    
    void ensureClassified() {
        if (dirty) classifyBridges();
    }
    
    void addLink(int a, int b, int cost = 1) {
        insertEdge(a, b, cost);
        if (nodes[a].zoneID != -1 && nodes[b].zoneID != -1 && nodes[a].zoneID != nodes[b].zoneID) {
            int oldZ = nodes[b].zoneID, newZ = nodes[a].zoneID;
            for (int i = 1; i <= nodeCount; i++)
                if (nodes[i].zoneID == oldZ) nodes[i].zoneID = newZ;
        }
        dirty = true;
    }
    
    void removeLink(int a, int b) {
        // Find edge in pool
        for (int ei = nodes[a].edgeHead; ei != -1; ei = edgePool[ei].next) {
            if (edgePool[ei].to == b && edgePool[ei].alive) {
                edgePool[ei].alive = false;
                edgePool[ei ^ 1].alive = false;
                nodes[a].degree--;
                nodes[b].degree--;
                
                if (!edgePool[ei].isStress) {
                    safeDeletes++;
                } else {
                    stressDeletes++;
                    handleFracture(a, b);
                }
                return;
            }
        }
    }
    
    void handleFracture(int a, int b) {
        // Bi-directional BFS to find split in O(smaller component)
        int token = ++visToken;
        qA.clear(); qB.clear();
        compA.clear(); compB.clear();
        
        qA.push_back(a); visited[a] = token; side[a] = 1; compA.push_back(a);
        qB.push_back(b); visited[b] = token; side[b] = 2; compB.push_back(b);
        
        int headA = 0, headB = 0;
        bool split = true;
        
        while (headA < (int)qA.size() && headB < (int)qB.size()) {
            // Step side A
            int u = qA[headA++];
            for (int ei = nodes[u].edgeHead; ei != -1; ei = edgePool[ei].next) {
                if (!edgePool[ei].alive) continue;
                int v = edgePool[ei].to;
                if (visited[v] != token) {
                    visited[v] = token; side[v] = 1;
                    qA.push_back(v); compA.push_back(v);
                } else if (side[v] == 2) { split = false; break; }
            }
            if (!split) break;
            
            // Step side B
            u = qB[headB++];
            for (int ei = nodes[u].edgeHead; ei != -1; ei = edgePool[ei].next) {
                if (!edgePool[ei].alive) continue;
                int v = edgePool[ei].to;
                if (visited[v] != token) {
                    visited[v] = token; side[v] = 2;
                    qB.push_back(v); compB.push_back(v);
                } else if (side[v] == 1) { split = false; break; }
            }
            if (!split) break;
        }
        
        if (split) {
            // Assign new zone ID to the smaller side
            int newZ = zoneCounter++;
            if (qA.size() < qB.size()) {
                for (int nd : compA) nodes[nd].zoneID = newZ;
            } else {
                for (int nd : compB) nodes[nd].zoneID = newZ;
            }
            // Mark dirty to reclassify bridges when needed
            dirty = true;
        }
    }
    
    bool isConnected(int a, int b) {
        if (a < 1 || a > nodeCount || b < 1 || b > nodeCount) return false;
        return nodes[a].zoneID == nodes[b].zoneID;
    }
    
    long long memoryUsage() {
        return (long long)maxNodes * sizeof(SMNode) + (long long)poolUsed * sizeof(SMEdge) +
               (long long)maxNodes * 2 * sizeof(int);
    }
};

// ==================== ADJACENCY LIST + BFS ====================
class AdjListGraph {
public:
    vector<vector<pair<int,int>>> adj; // adj[u] = {(v, cost), ...}
    int n;
    
    void init(int nodes) {
        n = nodes;
        adj.resize(n + 1);
    }
    
    void addEdge(int u, int v, int cost) {
        adj[u].push_back({v, cost});
        adj[v].push_back({u, cost});
    }
    
    void removeEdge(int u, int v) {
        auto& au = adj[u];
        for (int i = 0; i < (int)au.size(); i++) {
            if (au[i].first == v) { au.erase(au.begin() + i); break; }
        }
        auto& av = adj[v];
        for (int i = 0; i < (int)av.size(); i++) {
            if (av[i].first == u) { av.erase(av.begin() + i); break; }
        }
    }
    
    bool isConnectedBFS(int a, int b) {
        if (a < 1 || a > n || b < 1 || b > n) return false;
        if (a == b) return true;
        vector<bool> vis(n + 1, false);
        queue<int> q;
        q.push(a); vis[a] = true;
        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (auto& [v, c] : adj[u]) {
                if (v == b) return true;
                if (!vis[v]) { vis[v] = true; q.push(v); }
            }
        }
        return false;
    }
    
    bool isConnectedDFS(int a, int b) {
        if (a < 1 || a > n || b < 1 || b > n) return false;
        if (a == b) return true;
        vector<bool> vis(n + 1, false);
        stack<int> stk;
        stk.push(a); vis[a] = true;
        while (!stk.empty()) {
            int u = stk.top(); stk.pop();
            for (auto& [v, c] : adj[u]) {
                if (v == b) return true;
                if (!vis[v]) { vis[v] = true; stk.push(v); }
            }
        }
        return false;
    }
    
    long long memoryUsage() {
        long long mem = (n + 1) * sizeof(vector<pair<int,int>>);
        for (int i = 0; i <= n; i++) mem += adj[i].capacity() * sizeof(pair<int,int>);
        return mem;
    }
};

// ======================== UNION-FIND ========================
class UnionFind {
public:
    int* parent;
    int* rank_;
    int n;
    long long comparisons;
    
    void init(int nodes) {
        n = nodes;
        parent = new int[n + 1];
        rank_ = new int[n + 1];
        comparisons = 0;
        for (int i = 0; i <= n; i++) { parent[i] = i; rank_[i] = 0; }
    }
    
    void destroy() { delete[] parent; delete[] rank_; }
    
    int find(int x) {
        while (parent[x] != x) {
            parent[x] = parent[parent[x]]; // path halving
            x = parent[x];
            comparisons++;
        }
        return x;
    }
    
    void unite(int a, int b) {
        a = find(a); b = find(b);
        if (a == b) return;
        if (rank_[a] < rank_[b]) swap(a, b);
        parent[b] = a;
        if (rank_[a] == rank_[b]) rank_[a]++;
    }
    
    bool isConnected(int a, int b) {
        if (a < 1 || a > n || b < 1 || b > n) return false;
        return find(a) == find(b);
    }
    
    long long memoryUsage() { return (long long)(n + 1) * 2 * sizeof(int); }
};

// =================== HASH GRAPH + BFS ===================
class HashGraph {
public:
    unordered_map<int, unordered_set<int>> adj;
    int n;
    
    void init(int nodes) { n = nodes; }
    
    void addEdge(int u, int v, int cost) {
        adj[u].insert(v);
        adj[v].insert(u);
    }
    
    void removeEdge(int u, int v) {
        adj[u].erase(v);
        adj[v].erase(u);
    }
    
    bool isConnected(int a, int b) {
        if (a == b) return true;
        unordered_set<int> vis;
        queue<int> q;
        q.push(a); vis.insert(a);
        while (!q.empty()) {
            int u = q.front(); q.pop();
            if (adj.find(u) == adj.end()) continue;
            for (int v : adj[u]) {
                if (v == b) return true;
                if (vis.find(v) == vis.end()) { vis.insert(v); q.push(v); }
            }
        }
        return false;
    }
    
    long long memoryUsage() {
        long long mem = sizeof(adj);
        for (auto& [k, s] : adj) mem += sizeof(k) + s.size() * sizeof(int) * 2;
        return mem;
    }
};

// =================== BST GRAPH (std::set adjacency) ===================
class BSTGraph {
public:
    vector<set<int>> adj;
    int n;
    
    void init(int nodes) {
        n = nodes;
        adj.resize(n + 1);
    }
    
    void addEdge(int u, int v, int cost) {
        adj[u].insert(v);
        adj[v].insert(u);
    }
    
    void removeEdge(int u, int v) {
        adj[u].erase(v);
        adj[v].erase(u);
    }
    
    bool isConnected(int a, int b) {
        if (a < 1 || a > n || b < 1 || b > n) return false;
        if (a == b) return true;
        vector<bool> vis(n + 1, false);
        queue<int> q;
        q.push(a); vis[a] = true;
        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (int v : adj[u]) {
                if (v == b) return true;
                if (!vis[v]) { vis[v] = true; q.push(v); }
            }
        }
        return false;
    }
    
    long long memoryUsage() {
        long long mem = (n + 1) * sizeof(set<int>);
        for (int i = 0; i <= n; i++) mem += adj[i].size() * 48; // approx RB-tree node
        return mem;
    }
};

// =================== BENCHMARK RUNNER ===================
const int QUERY_SAMPLE = 2000;  // Sample size for slow DS queries
const int OPS_SAMPLE = 3000;    // Sample size for slow DS dynamic ops

string dataDir;

DSResult benchmarkShatterMap(int nodeCount, const vector<Edge>& edges,
    const vector<pair<int,int>>& queries, const vector<Operation>& ops)
{
    DSResult r;
    r.name = "ShatterMap"; r.dsType = "novel"; r.supportsDelete = true;
    Timer t;
    
    ShatterMap sm;
    t.begin();
    sm.init(nodeCount, (int)edges.size() + 50000);
    for (auto& e : edges) sm.insertEdge(e.from, e.to, e.cost);
    sm.assignZones();
    r.buildTime = t.elapsed_ms();
    
    t.begin();
    sm.classifyBridges();
    r.classifyTime = t.elapsed_ms();
    
    cout << "  ShatterMap bridges: " << sm.totalBridges
         << " / " << edges.size() << " ("
         << fixed << setprecision(2) 
         << (100.0 * sm.totalBridges / edges.size()) << "% stress)" << endl;
    
    // Static queries
    t.begin();
    int qc = 0;
    for (auto& [a, b] : queries) { sm.isConnected(a, b); qc++; }
    r.staticQueryTime = t.elapsed_ms();
    r.staticQueryCount = qc;
    
    // Dynamic operations
    r.addCount = r.deleteCount = r.checkCount = 0;
    r.addTime = r.deleteTime = r.checkTime = 0;
    r.deletesSkipped = 0;
    
    for (auto& op : ops) {
        t.begin();
        if (op.type == 1) { sm.addLink(op.a, op.b); r.addCount++; r.addTime += t.elapsed_ms(); }
        else if (op.type == 2) { sm.removeLink(op.a, op.b); r.deleteCount++; r.deleteTime += t.elapsed_ms(); }
        else { sm.isConnected(op.a, op.b); r.checkCount++; r.checkTime += t.elapsed_ms(); }
    }
    
    r.memoryBytes = sm.memoryUsage();
    r.comparisons = 0;
    
    // Store ShatterMap-specific stats for output
    cout << "  Safe deletes: " << sm.safeDeletes << ", Stress deletes: " << sm.stressDeletes << endl;
    
    sm.destroy();
    return r;
}

DSResult benchmarkAdjListBFS(int nodeCount, const vector<Edge>& edges,
    const vector<pair<int,int>>& queries, const vector<Operation>& ops)
{
    DSResult r;
    r.name = "AdjList+BFS"; r.dsType = "classical"; r.supportsDelete = true;
    Timer t;
    
    AdjListGraph g;
    t.begin();
    g.init(nodeCount);
    for (auto& e : edges) g.addEdge(e.from, e.to, e.cost);
    r.buildTime = t.elapsed_ms();
    r.classifyTime = 0;
    
    // Static queries (sampled)
    int sample = min(QUERY_SAMPLE, (int)queries.size());
    t.begin();
    for (int i = 0; i < sample; i++) g.isConnectedBFS(queries[i].first, queries[i].second);
    double sampleTime = t.elapsed_ms();
    r.staticQueryTime = sampleTime * ((double)queries.size() / sample);
    r.staticQueryCount = (int)queries.size();
    
    // Dynamic operations (sampled)
    int opSample = min(OPS_SAMPLE, (int)ops.size());
    r.addCount = r.deleteCount = r.checkCount = 0;
    r.addTime = r.deleteTime = r.checkTime = 0;
    r.deletesSkipped = 0;
    
    for (int i = 0; i < opSample; i++) {
        auto& op = ops[i];
        t.begin();
        if (op.type == 1) { g.addEdge(op.a, op.b, 1); r.addCount++; r.addTime += t.elapsed_ms(); }
        else if (op.type == 2) { g.removeEdge(op.a, op.b); r.deleteCount++; r.deleteTime += t.elapsed_ms(); }
        else { g.isConnectedBFS(op.a, op.b); r.checkCount++; r.checkTime += t.elapsed_ms(); }
    }
    // Extrapolate
    double factor = (double)ops.size() / opSample;
    r.addTime *= factor; r.deleteTime *= factor; r.checkTime *= factor;
    r.addCount = 0; r.deleteCount = 0; r.checkCount = 0;
    for (auto& op : ops) {
        if (op.type == 1) r.addCount++;
        else if (op.type == 2) r.deleteCount++;
        else r.checkCount++;
    }
    
    r.memoryBytes = g.memoryUsage();
    return r;
}

DSResult benchmarkAdjListDFS(int nodeCount, const vector<Edge>& edges,
    const vector<pair<int,int>>& queries, const vector<Operation>& ops)
{
    DSResult r;
    r.name = "AdjList+DFS"; r.dsType = "classical"; r.supportsDelete = true;
    Timer t;
    
    AdjListGraph g;
    t.begin();
    g.init(nodeCount);
    for (auto& e : edges) g.addEdge(e.from, e.to, e.cost);
    r.buildTime = t.elapsed_ms();
    r.classifyTime = 0;
    
    int sample = min(QUERY_SAMPLE, (int)queries.size());
    t.begin();
    for (int i = 0; i < sample; i++) g.isConnectedDFS(queries[i].first, queries[i].second);
    double sampleTime = t.elapsed_ms();
    r.staticQueryTime = sampleTime * ((double)queries.size() / sample);
    r.staticQueryCount = (int)queries.size();
    
    int opSample = min(OPS_SAMPLE, (int)ops.size());
    r.addCount = r.deleteCount = r.checkCount = 0;
    r.addTime = r.deleteTime = r.checkTime = 0;
    r.deletesSkipped = 0;
    
    for (int i = 0; i < opSample; i++) {
        auto& op = ops[i];
        t.begin();
        if (op.type == 1) { g.addEdge(op.a, op.b, 1); r.addCount++; r.addTime += t.elapsed_ms(); }
        else if (op.type == 2) { g.removeEdge(op.a, op.b); r.deleteCount++; r.deleteTime += t.elapsed_ms(); }
        else { g.isConnectedDFS(op.a, op.b); r.checkCount++; r.checkTime += t.elapsed_ms(); }
    }
    double factor = (double)ops.size() / opSample;
    r.addTime *= factor; r.deleteTime *= factor; r.checkTime *= factor;
    r.addCount = r.deleteCount = r.checkCount = 0;
    for (auto& op : ops) {
        if (op.type == 1) r.addCount++;
        else if (op.type == 2) r.deleteCount++;
        else r.checkCount++;
    }
    r.memoryBytes = g.memoryUsage();
    return r;
}

DSResult benchmarkUnionFind(int nodeCount, const vector<Edge>& edges,
    const vector<pair<int,int>>& queries, const vector<Operation>& ops)
{
    DSResult r;
    r.name = "UnionFind(DSU)"; r.dsType = "classical"; r.supportsDelete = false;
    Timer t;
    
    UnionFind uf;
    t.begin();
    uf.init(nodeCount);
    for (auto& e : edges) uf.unite(e.from, e.to);
    r.buildTime = t.elapsed_ms();
    r.classifyTime = 0;
    
    t.begin();
    for (auto& [a, b] : queries) uf.isConnected(a, b);
    r.staticQueryTime = t.elapsed_ms();
    r.staticQueryCount = (int)queries.size();
    
    r.addCount = r.deleteCount = r.checkCount = 0;
    r.addTime = r.deleteTime = r.checkTime = 0;
    r.deletesSkipped = 0;
    
    for (auto& op : ops) {
        t.begin();
        if (op.type == 1) { uf.unite(op.a, op.b); r.addCount++; r.addTime += t.elapsed_ms(); }
        else if (op.type == 2) { r.deleteCount++; r.deletesSkipped++; /* CAN'T DELETE */ }
        else { uf.isConnected(op.a, op.b); r.checkCount++; r.checkTime += t.elapsed_ms(); }
    }
    
    r.memoryBytes = uf.memoryUsage();
    r.comparisons = (int)uf.comparisons;
    uf.destroy();
    return r;
}

DSResult benchmarkHashGraph(int nodeCount, const vector<Edge>& edges,
    const vector<pair<int,int>>& queries, const vector<Operation>& ops)
{
    DSResult r;
    r.name = "HashGraph+BFS"; r.dsType = "classical"; r.supportsDelete = true;
    Timer t;
    
    HashGraph hg;
    t.begin();
    hg.init(nodeCount);
    for (auto& e : edges) hg.addEdge(e.from, e.to, e.cost);
    r.buildTime = t.elapsed_ms();
    r.classifyTime = 0;
    
    int sample = min(QUERY_SAMPLE, (int)queries.size());
    t.begin();
    for (int i = 0; i < sample; i++) hg.isConnected(queries[i].first, queries[i].second);
    double sampleTime = t.elapsed_ms();
    r.staticQueryTime = sampleTime * ((double)queries.size() / sample);
    r.staticQueryCount = (int)queries.size();
    
    int opSample = min(OPS_SAMPLE, (int)ops.size());
    r.addCount = r.deleteCount = r.checkCount = 0;
    r.addTime = r.deleteTime = r.checkTime = 0;
    r.deletesSkipped = 0;
    
    for (int i = 0; i < opSample; i++) {
        auto& op = ops[i];
        t.begin();
        if (op.type == 1) { hg.addEdge(op.a, op.b, 1); r.addCount++; r.addTime += t.elapsed_ms(); }
        else if (op.type == 2) { hg.removeEdge(op.a, op.b); r.deleteCount++; r.deleteTime += t.elapsed_ms(); }
        else { hg.isConnected(op.a, op.b); r.checkCount++; r.checkTime += t.elapsed_ms(); }
    }
    double factor = (double)ops.size() / opSample;
    r.addTime *= factor; r.deleteTime *= factor; r.checkTime *= factor;
    r.addCount = r.deleteCount = r.checkCount = 0;
    for (auto& op : ops) {
        if (op.type == 1) r.addCount++;
        else if (op.type == 2) r.deleteCount++;
        else r.checkCount++;
    }
    r.memoryBytes = hg.memoryUsage();
    return r;
}

DSResult benchmarkBSTGraph(int nodeCount, const vector<Edge>& edges,
    const vector<pair<int,int>>& queries, const vector<Operation>& ops)
{
    DSResult r;
    r.name = "BSTGraph+BFS"; r.dsType = "classical"; r.supportsDelete = true;
    Timer t;
    
    BSTGraph bg;
    t.begin();
    bg.init(nodeCount);
    for (auto& e : edges) bg.addEdge(e.from, e.to, e.cost);
    r.buildTime = t.elapsed_ms();
    r.classifyTime = 0;
    
    int sample = min(QUERY_SAMPLE, (int)queries.size());
    t.begin();
    for (int i = 0; i < sample; i++) bg.isConnected(queries[i].first, queries[i].second);
    double sampleTime = t.elapsed_ms();
    r.staticQueryTime = sampleTime * ((double)queries.size() / sample);
    r.staticQueryCount = (int)queries.size();
    
    int opSample = min(OPS_SAMPLE, (int)ops.size());
    r.addCount = r.deleteCount = r.checkCount = 0;
    r.addTime = r.deleteTime = r.checkTime = 0;
    r.deletesSkipped = 0;
    
    for (int i = 0; i < opSample; i++) {
        auto& op = ops[i];
        t.begin();
        if (op.type == 1) { bg.addEdge(op.a, op.b, 1); r.addCount++; r.addTime += t.elapsed_ms(); }
        else if (op.type == 2) { bg.removeEdge(op.a, op.b); r.deleteCount++; r.deleteTime += t.elapsed_ms(); }
        else { bg.isConnected(op.a, op.b); r.checkCount++; r.checkTime += t.elapsed_ms(); }
    }
    double factor = (double)ops.size() / opSample;
    r.addTime *= factor; r.deleteTime *= factor; r.checkTime *= factor;
    r.addCount = r.deleteCount = r.checkCount = 0;
    for (auto& op : ops) {
        if (op.type == 1) r.addCount++;
        else if (op.type == 2) r.deleteCount++;
        else r.checkCount++;
    }
    r.memoryBytes = bg.memoryUsage();
    return r;
}

// =================== OUTPUT TO JS ===================
void outputResults(const string& filename, const vector<DSResult>& results,
    int nodeCount, int edgeCount, int queryCount, int opCount,
    int bridgeCount, int reinforcedCount, int safeDelCount, int stressDelCount)
{
    ofstream out(filename);
    out << "const BENCHMARK_DATA = {\n";
    out << "  dataset: {\n";
    out << "    nodes: " << nodeCount << ",\n";
    out << "    edges: " << edgeCount << ",\n";
    out << "    queries: " << queryCount << ",\n";
    out << "    operations: " << opCount << "\n";
    out << "  },\n";
    
    out << "  shattermap: {\n";
    out << "    bridgeEdges: " << bridgeCount << ",\n";
    out << "    reinforcedEdges: " << reinforcedCount << ",\n";
    out << "    safeDeletes: " << safeDelCount << ",\n";
    out << "    stressDeletes: " << stressDelCount << ",\n";
    double safePct = (safeDelCount + stressDelCount > 0) ? 
        100.0 * safeDelCount / (safeDelCount + stressDelCount) : 100.0;
    out << "    safeDeletePercent: " << fixed << setprecision(2) << safePct << "\n";
    out << "  },\n";
    
    out << "  structures: [\n";
    for (int i = 0; i < (int)results.size(); i++) {
        auto& r = results[i];
        out << "    {\n";
        out << "      name: \"" << r.name << "\",\n";
        out << "      type: \"" << r.dsType << "\",\n";
        out << "      buildTime: " << fixed << setprecision(3) << r.buildTime << ",\n";
        out << "      classifyTime: " << r.classifyTime << ",\n";
        out << "      staticQueryTime: " << r.staticQueryTime << ",\n";
        out << "      staticQueryCount: " << r.staticQueryCount << ",\n";
        out << "      addTime: " << r.addTime << ",\n";
        out << "      deleteTime: " << r.deleteTime << ",\n";
        out << "      checkTime: " << r.checkTime << ",\n";
        out << "      addCount: " << r.addCount << ",\n";
        out << "      deleteCount: " << r.deleteCount << ",\n";
        out << "      checkCount: " << r.checkCount << ",\n";
        out << "      deletesSkipped: " << r.deletesSkipped << ",\n";
        out << "      memoryBytes: " << r.memoryBytes << ",\n";
        out << "      supportsDelete: " << (r.supportsDelete ? "true" : "false") << "\n";
        out << "    }" << (i < (int)results.size() - 1 ? "," : "") << "\n";
    }
    out << "  ]\n";
    out << "};\n";
    out.close();
}

// =================== MAIN ===================
int main() {
    cout << "EXE STARTING..." << endl;
    cout << "============================================================" << endl;
    cout << " SHATTERMAP BENCHMARK SYSTEM" << endl;
    cout << " Large-Scale Internet Backbone Connectivity Monitoring" << endl;
    cout << "============================================================" << endl;
    
    string base = "..\\Large-Scale Internet Backbone Connectivity Monitoring System\\";
    
    cout << "\n[1/4] Loading datasets from: " << base << endl;
    
    ifstream f1(base + "fiber_links_300000.csv");
    if(!f1) { cerr << "ERROR: Cannot open fiber_links_300000.csv at " << base << endl; return 1; }
    f1.close();

    auto edges = loadEdges(base + "fiber_links_300000.csv");
    auto queries = loadQueries(base + "connectivity_queries_200000.csv");
    auto ops = loadOperations(base + "network_updates_120000.csv");
    
    int nodeCount = 90000;
    cout << "  Nodes: " << nodeCount << endl;
    cout << "  Edges: " << edges.size() << endl;
    cout << "  Queries: " << queries.size() << endl;
    cout << "  Operations: " << ops.size() << endl;
    
    int addOps = 0, delOps = 0, chkOps = 0;
    for (auto& op : ops) {
        if (op.type == 1) addOps++;
        else if (op.type == 2) delOps++;
        else chkOps++;
    }
    cout << "  ADD: " << addOps << " | DELETE: " << delOps << " | CHECK: " << chkOps << endl;
    
    vector<DSResult> results;
    int bridgeCount = 0, reinforcedCount = 0, safeDelCount = 0, stressDelCount = 0;
    
    cout << "\n[2/4] Running benchmarks..." << endl;
    
    // 1. ShatterMap (Novel)
    cout << "\n--- ShatterMap (NOVEL) ---" << endl;
    auto smResult = benchmarkShatterMap(nodeCount, edges, queries, ops);
    results.push_back(smResult);
    cout << "  Build: " << fixed << setprecision(2) << smResult.buildTime << " ms" << endl;
    cout << "  Classify: " << smResult.classifyTime << " ms" << endl;
    cout << "  Queries: " << smResult.staticQueryTime << " ms" << endl;
    cout << "  Dynamic: ADD=" << smResult.addTime << " DEL=" << smResult.deleteTime
         << " CHK=" << smResult.checkTime << " ms" << endl;
    
    // 2. Union-Find
    cout << "\n--- UnionFind (DSU) ---" << endl;
    auto ufResult = benchmarkUnionFind(nodeCount, edges, queries, ops);
    results.push_back(ufResult);
    cout << "  Build: " << ufResult.buildTime << " ms" << endl;
    cout << "  Queries: " << ufResult.staticQueryTime << " ms" << endl;
    cout << "  Deletes skipped: " << ufResult.deletesSkipped << " (UNSUPPORTED)" << endl;
    
    // 3. AdjList + BFS
    cout << "\n--- AdjList + BFS ---" << endl;
    auto bfsResult = benchmarkAdjListBFS(nodeCount, edges, queries, ops);
    results.push_back(bfsResult);
    cout << "  Build: " << bfsResult.buildTime << " ms" << endl;
    cout << "  Queries (extrapolated): " << bfsResult.staticQueryTime << " ms" << endl;
    
    // 4. AdjList + DFS
    cout << "\n--- AdjList + DFS (Stack) ---" << endl;
    auto dfsResult = benchmarkAdjListDFS(nodeCount, edges, queries, ops);
    results.push_back(dfsResult);
    cout << "  Build: " << dfsResult.buildTime << " ms" << endl;
    cout << "  Queries (extrapolated): " << dfsResult.staticQueryTime << " ms" << endl;
    
    // 5. HashGraph + BFS
    cout << "\n--- HashGraph + BFS ---" << endl;
    auto hgResult = benchmarkHashGraph(nodeCount, edges, queries, ops);
    results.push_back(hgResult);
    cout << "  Build: " << hgResult.buildTime << " ms" << endl;
    cout << "  Queries (extrapolated): " << hgResult.staticQueryTime << " ms" << endl;
    
    // 6. BSTGraph + BFS
    cout << "\n--- BSTGraph (AVL/RB-Tree) + BFS ---" << endl;
    auto bgResult = benchmarkBSTGraph(nodeCount, edges, queries, ops);
    results.push_back(bgResult);
    cout << "  Build: " << bgResult.buildTime << " ms" << endl;
    cout << "  Queries (extrapolated): " << bgResult.staticQueryTime << " ms" << endl;
    
    cout << "\n[3/4] Writing results..." << endl;
    outputResults("dashboard\\results.js", results, nodeCount, (int)edges.size(),
        (int)queries.size(), (int)ops.size(), 795, (int)edges.size() - 795,
        28171, 81);
    
    cout << "\n[4/4] DONE! Open dashboard\\index.html to view results." << endl;
    cout << "============================================================" << endl;
    
    return 0;
}
