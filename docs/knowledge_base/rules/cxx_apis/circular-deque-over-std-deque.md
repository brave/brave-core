---
name: brave/circular-deque-over-std-deque
description:
  'Use base::circular_deque, base::queue and base::stack instead of std::deque,
  std::queue and std::stack.'
rubric: perf
severity: should_fix
---

**Rule.** Declare double-ended queues, FIFO queues and stacks as
`base::circular_deque<T>`, `base::queue<T>` and `base::stack<T>` (from
`base/containers/`), not `std::deque<T>`, `std::queue<T>` or `std::stack<T>`.
When the code needs iterators that stay valid while the container is mutated,
with constant-time insertion and removal, use `std::list` instead, since
`base::circular_deque` does not provide that.

**Why.** `std::deque`'s block size and memory use vary widely between standard
library implementations, and it costs more code size. The base containers behave
the same on every platform.

## Examples

Bad:

```cpp
std::deque<int> items;
std::queue<int> pending;
std::stack<int> history;
```

Good:

```cpp
base::circular_deque<int> items;
base::queue<int> pending;
base::stack<int> history;
```
