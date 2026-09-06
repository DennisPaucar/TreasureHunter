# 🗺️ Treasure Hunter

> A multiplayer strategy and exploration game written in **C++**, modeled around **undirected graphs** and pathfinding algorithms.

---

## 📌 Overview

**Treasure Hunter** is a networked multiplayer turn-based game where players navigate a dynamically structured map represented as an **undirected graph** ($G = (V, E)$). 

Players compete to locate hidden treasures, strategically choosing routes between connected nodes while managing movement costs, obstacles, and opponent positions across the graph topology.

---

## 🚀 Key Features

- **Graph-Based Map Topology:** The game world is built dynamically as an undirected graph, utilizing adjacency lists/matrices to handle bidirectional paths between locations.
- **Pathfinding & Graph Traversal:** Incorporates graph algorithms (such as BFS/Dijkstra) for path validation, connectivity checks, and optimal route calculations.
- **Multiplayer Architecture:** Client-server model enabling real-time or turn-based interaction across multiple connected players over TCP/UDP sockets.
- **Deterministic Game Logic:** Robust state synchronization to ensure fairness, turn validation, and concurrent action handling.

---

## 🛠️ Tech Stack & Concepts

- **Language:** C++ (Modern C++17 / C++20)
- **Data Structures:** Undirected Graphs (Adjacency List), Priority Queues, Hash Maps.
- **Networking:** Sockets (POSIX Sockets / Winsock / Boost.Asio).
- **Core Algorithms:** Shortest path (Dijkstra / BFS), Connected Components, Minimum Spanning Tree (MST).

---

## 📐 How the Graph Model Works

- **Vertices ($V$):** Represent rooms, islands, or map landmarks containing rewards, traps, or exit points.
- **Edges ($E$):** Bidirectional paths connecting neighboring vertices.
- **Weights ($W$):** Movement penalties, terrain difficulty, or distance costs required to travel between nodes.

---
