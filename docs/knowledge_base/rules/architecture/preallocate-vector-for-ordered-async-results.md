---
name: brave/preallocate-vector-for-ordered-async-results
description:
  'Ordered results of parallel async calls go into a vector pre-sized to the
  number of requests, by request index, not into a map sorted later.'
rubric: readability
severity: nit
---

**Rule.** When the results of parallel asynchronous calls must keep the order of
the requests, size a `std::vector` to the number of requests up front, bind each
request's index into its callback, and store each result at its index. Do not
collect them in a `std::map` keyed by index, or sort them afterwards.

**Why.** The pre-sized vector holds the results in request order as they arrive,
with no extra container, sorting step or ordering bug.

## Examples

Bad:

```cpp
std::map<int, Result> results_by_index_;
```

Good:

```cpp
std::vector<Result> results_(num_requests);
// In each callback:
results_[request_index] = std::move(result);
```
