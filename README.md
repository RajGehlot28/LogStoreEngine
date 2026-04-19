# LogStore Engine
A high-performance, thread-safe **Key-Value Storage Engine** built in C++ based on the **Bitcask (LSM-tree style) architecture**. It is optimized for high-throughput write operations and ensures data persistence with built-in integrity checks.

## What It Does
Unlike traditional databases that perform expensive "in-place" updates, **LogStore** uses an **Append-Only Log** structure. Every operation is sequentially written to the end of the file, which significantly reduces disk head movement and maximizes write speeds.

### Key Features
* **Append-Only Storage:** Achieves **$O(1)$ write throughput** by treating the database as a sequential log.
* **In-Memory Indexing:** Maintains a sorted index using `std::map` to facilitate **Range Queries** and point lookups in **$O(\log N)$** time.
* **Data Integrity:** Implements a **Checksum validation** system (ASCII-sum) for every record to detect and handle partial writes or file corruption.
* **Log Compaction:** Includes an automated "Garbage Collection" mechanism to reclaim disk space by removing stale records and tombstones.
* **Concurrency Control:** Engineered with `std::mutex` and `lock_guard` patterns to support safe, concurrent access in multi-threaded environments.

## Performance Analysis
| Operation | Time Complexity | Note |
| :--- | :--- | :--- |
| **SET (Write)** | $O(1)$ | Sequential disk append |
| **GET (Read)** | $O(\log N)$ | Index lookup + 1 Disk Seek |
| **Range Scan** | $O(K + \log N)$ | Where $K$ is the number of elements |

## Tech Stack
* **Language:** C++ (Standard: C++11 or higher)
* **Architecture:** Bitcask / Log-Structured Storage
* **Core Libraries:** `std::mutex`, `std::fstream`, `std::map`
* **Environment:** MinGW-w64 (POSIX Thread Model)

## How to Run
1. Ensure you have a C++ compiler (GCC/G++) with POSIX threads enabled.
2. Compile the source code:
   ```bash
   g++ -std=c++11 main.cpp -o LogStore -pthread