# High-Frequency Trading Exchange System (C++20)

A cache-aware, zero-dynamic-allocation matching engine with lock-free queues, binary wire protocol support, and replication/validation tooling.

## Why This Project

This repository is a systems-focused implementation of a low-latency exchange core designed around deterministic memory and predictable performance.

Key constraints:
- Zero dynamic allocation after initialization
- Index-based memory ownership and recycling
- Lock-free SPSC communication primitives
- Cross-platform support (macOS/Linux) with compile-time guards

Observed benchmark in this repo: ~617k orders/sec on macOS arm64 (details in Performance section).

## Project Status

- Completed: Phases 1 through 5
- Build: Passing
- Tests: Passing (functional, protocol, replication, stress)

## Repository Layout

- `core/include/`: core types, order book, matching engine, memory pool, protocol, queues
- `core/src/`: core implementation files
- `apps/passive_engine/`: passive replication app
- `apps/market_maker/`: market-making order flow generator
- `tests/`: latency, protocol, functional, stress, and lifecycle tests
- `CMakeLists.txt`: top-level build configuration

## Architecture

### Phase 1: Core Engine
- `core/include/order_book.h`: price-level container with FIFO per level
- `core/include/matching_engine.h`: routes Add/Cancel/Modify and emits executions
- `core/include/memory_pool.h`: O(1) index allocation/release via free list
- `core/include/types.h`: fixed-width domain types and snapshot structs

### Phase 2: Latency Validation
- `tests/lockfree_queue_latency_test.cpp`: lock-free queue benchmark
- Timing source:
  - macOS arm64: `mach_absolute_time()`
  - x86/Linux: `__rdtsc()`

### Phase 3: Network Layer
- `core/include/protocol.h`: binary wire encode/decode for orders/executions
- `core/include/epoll_server.h`: Linux epoll TCP ingress
- `core/include/udp_multicast.h`: UDP multicast publish/receive

### Phase 4: Passive Replication
- `apps/passive_engine/main.cpp`: consumes multicast order flow and replays locally
- `tests/passive_engine_replication_test.cpp`: compares primary vs passive snapshots

### Phase 5: Market Making + Accuracy Expansion
- `apps/market_maker/main.cpp`: quote sender over TCP
- `tests/wire_protocol_test.cpp`: protocol correctness and sequence checks
- `tests/market_maker_functional_test.cpp`: end-to-end behavior on 1000 orders
- `tests/order_book_stress_test.cpp`: throughput run on 50k orders
- `tests/cancel_replace_test.cpp`: repeated add/modify/cancel lifecycle checks

## Build

### Requirements
- CMake 3.20+
- C++20 compiler (Apple Clang 15+, GCC 10+, or equivalent)

### Configure and Compile

```bash
cmake -S . -B build
cmake --build build -j
```

## Run Tests

```bash
./build/tests/lockfree_queue_latency_test
./build/tests/passive_engine_replication_test
./build/tests/wire_protocol_test
./build/tests/market_maker_functional_test
./build/tests/order_book_stress_test
./build/tests/cancel_replace_test
```

## Performance Snapshot

Measured on macOS arm64 in current workspace runs:

| Metric | Result |
|---|---|
| Order book throughput | 617,283 orders/sec (50k orders in 81 ms) |
| Lock-free queue latency | 107,592 cycles/message (1M messages) |
| Replication accuracy | 100% snapshot match (100k deterministic orders) |
| Functional execution rate | 716 executions out of 1000 generated orders |

These numbers are hardware/compiler dependent and should be treated as indicative, not guaranteed.

## Platform Notes

| Feature | macOS | Linux |
|---|---|---|
| Core matching engine | Yes | Yes |
| UDP multicast utilities | Yes | Yes |
| epoll TCP server | No | Yes |
| Benchmark/test suite | Yes | Yes |

`epoll_server.h` is guarded for Linux (`__linux__`).

## Public Repo Notes

- This is an educational and engineering project, not production trading infrastructure.
- Not financial advice. Do not use as-is for live market deployment.
- If you publish this repo, add a `LICENSE` file so usage terms are explicit.

## Useful Entry Points

- `core/include/order_book.h`
- `core/include/matching_engine.h`
- `core/include/protocol.h`
- `apps/market_maker/main.cpp`
- `apps/passive_engine/main.cpp`

## Next Improvements

- Multi-symbol books and symbol routing
- Risk checks (price bands, position limits)
- Persistence/recovery (append-only log + snapshot)
- kqueue-based ingress path for macOS parity with epoll
- Incremental multicast snapshot/delta dissemination
