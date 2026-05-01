# ⚡ FR-DCO — Fracture-Resilient Dynamic Connectivity Oracle

> A topology-aware dynamic connectivity system optimized for real-world large-scale networks.

---

## 🚀 Overview

FR-DCO (Fracture-Resilient Dynamic Connectivity Oracle) is a high-performance system designed to efficiently handle **dynamic graph connectivity** under real-world conditions such as link failures and network volatility.

Traditional dynamic connectivity structures struggle with performance due to:
- heavy memory overhead
- pointer chasing
- poor cache utilization
- worst-case oriented design

FR-DCO takes a different approach:
👉 It is optimized for **real-world network topology behavior**, not just theoretical worst-case scenarios.

---

## 🧠 Core Idea

Real-world backbone networks are highly redundant:

- ~99%+ edges lie in cycles (non-critical)
- Only a small fraction are bridges (critical edges)

FR-DCO exploits this:

| Case | Strategy | Complexity |
|------|--------|-----------|
| Non-critical edge removal | Fast-path bypass | **O(1)** |
| Critical edge removal | Bi-directional verification | **bounded by smaller component** |
| Connectivity query | Zone-based lookup | **O(1)** |

---

## ⚙️ Key Features

- ⚡ O(1) connectivity queries via zone mapping  
- ⚡ O(1) deletion for non-critical edges  
- 🔥 Bi-directional Component Verification (BCV) for failures  
- 🧠 Topology-aware edge classification (cycle vs bridge)  
- 🧱 Cache-optimized flat memory architecture  
- 🚀 Designed for large-scale systems (90K+ nodes)

---

## 🏗️ System Design

### 1. Tier-Based Edge Classification
Edges are classified into:
- **Tier-0 (Cycle edges)** → safe removal  
- **Tier-1 (Bridge edges)** → requires recovery  

---

### 2. Zone Mapping (O(1) Queries)

Each node maintains a zone ID:

```text
IsConnected(u, v) ⇔ zone[u] == zone[v]
