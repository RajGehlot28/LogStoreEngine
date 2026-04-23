# LogStore Engine
A high-performance Key-Value Storage Engine built in C++ following the Bitcask architecture. It is designed for low-latency writes and thread-safe data persistence.

## Core Features
* **Append-Only Log:** Ensures $O(1)$ write performance by sequentially appending records to the end of the file.
* **In-Memory Index:** Maintains a sorted `std::map` of keys and their file offsets, allowing $O(\log N)$ point lookups and range queries.
* **Thread Safety:** Uses `std::mutex` and `lock_guard` for safe concurrent operations in multi-threaded environments.
* **Data Integrity:** Implements checksum validation to detect file corruption and ensure reliable crash recovery.
* **Log Compaction:** Includes a background-style garbage collection process to reclaim disk space by removing stale data.

## Tech Stack
* **Language:** C++ (C++11 or higher)
* **Architecture:** Bitcask / Log-Structured Storage
* **Core Libraries:** `std::mutex`, `std::fstream`, `std::map`

## How to Run
1. Compile the code with thread support:
   ```bash
   g++ -std=c++11 main.cpp -o LogStore -pthread
