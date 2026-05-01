FR-DCO: Fracture-Resilient Dynamic Connectivity Oracle

A topology-aware dynamic connectivity system designed for efficient handling of large-scale, real-world network graphs under dynamic edge updates.

---

1. Overview

FR-DCO (Fracture-Resilient Dynamic Connectivity Oracle) is a high-performance system for maintaining connectivity in dynamically evolving graphs, particularly under conditions such as link insertions, failures, and network volatility.

Conventional dynamic connectivity approaches often prioritize worst-case guarantees at the cost of practical performance. These methods typically rely on pointer-heavy data structures and incur significant overhead due to cache inefficiency and complex memory access patterns.

FR-DCO adopts a systems-oriented approach, optimizing for real-world network characteristics, especially scale-free topologies commonly observed in internet backbone infrastructures.

---

2. Problem Statement

Given an undirected dynamic graph G = (V, E), the system must efficiently support:

- Edge insertion: "AddEdge(u, v)"
- Edge deletion: "DeleteEdge(u, v)"
- Connectivity query: "IsConnected(u, v)"

The primary challenge lies in handling edge deletions efficiently while maintaining accurate connectivity information at scale.

---

3. Key Insight

Empirical analysis of large-scale network graphs reveals:

- The majority of edges participate in cycles (non-critical)
- A small fraction of edges act as bridges (critical)

FR-DCO exploits this structural property by treating edges differently based on their topological role.

---

4. System Design

4.1 Edge Classification

Edges are categorized into two tiers:

- Tier-0 (Cycle Edges)
  Removal does not affect connectivity

- Tier-1 (Bridge Edges)
  Removal may partition the graph and requires verification

---

4.2 Zone Mapping for Connectivity

Each vertex is assigned a zone identifier. Connectivity queries are resolved using:

IsConnected(u, v) ⇔ zone[u] == zone[v]

This eliminates the need for traversal-based queries, achieving constant-time performance.

---

4.3 Fast-Path Deletion

For Tier-0 edges:

- Deletion is handled in constant time
- No global recomputation is required

---

4.4 Bi-directional Component Verification (BCV)

For Tier-1 edges:

- Simultaneous search is initiated from both endpoints
- Search terminates when a boundary is detected
- Complexity is bounded by the size of the smaller component

---

4.5 Memory Architecture

FR-DCO employs a flat edge pool with contiguous memory allocation:

- Eliminates pointer fragmentation
- Improves cache locality
- Reduces memory access latency

---

5. Complexity Characteristics

Operation| Complexity
Connectivity Query| O(1)
Edge Deletion (Tier-0)| O(1)
Edge Deletion (Tier-1)| O(min component size)

---

6. Benchmark Summary

Experiments were conducted on large-scale graph instances:

- Number of nodes: 90,000
- Number of edges: 300,000
- Bridge ratio: approximately 0.3%

Observed characteristics:

- Constant-time connectivity queries
- Efficient handling of non-critical edge deletions
- Significant practical performance improvements over traversal-based methods

---

7. Applicability

FR-DCO is particularly suited for:

- Scale-free network topologies
- Internet backbone monitoring
- Software-defined networking (SDN) systems
- High-throughput, low-latency environments

---

8. Limitations

- Performance depends on structural properties of the graph
- Worst-case scenarios (e.g., tree-like graphs) may reduce efficiency
- Currently lacks formal theoretical bounds for all graph classes

---

9. Implementation Details

- Language: C++
- Optimization: AVX2 / SIMD where applicable
- Memory Model: Flat, contiguous allocation
- Benchmarking: Custom evaluation framework

---

10. Build and Execution

git clone https://github.com/dhanasai2/Fracture-Resilient-Dynamic-Connectivity-Oracle.git
cd Fracture-Resilient-Dynamic-Connectivity-Oracle
g++ -O3 main.cpp -o frdco
./frdco

---

11. Future Work

- Formal complexity analysis and proofs
- Optimization for adversarial graph structures
- Integration with real-time network systems
- Exploration of hardware acceleration (FPGA / SmartNIC)

---

12. Reference

FR-DCO: Fracture-Resilient Dynamic Connectivity Oracle
JNTUGV College of Engineering, 2025

---

13. Author

Gundumogula Dhana Sai
B.Tech Information Technology
JNTUGV College of Engineering

Email: saigundumogula5@gmail.com
GitHub: https://github.com/dhanasai2
