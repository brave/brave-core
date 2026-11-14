---
name: brave/use-barrier-callback-for-parallel-aggregation
description:
  'The results of several parallel async operations are gathered with
  base::BarrierCallback, not a hand-maintained pending count.'
rubric: readability
severity: should_fix
---

**Rule.** To wait for several parallel asynchronous operations and collect their
results, pass each one a callback from a single
`base::BarrierCallback<T>(n, done)`. Do not keep a pending counter and a results
vector as members and decrement the counter in each callback.

**Why.** `BarrierCallback` collects the results and runs the done callback once,
after the last one; a hand-rolled counter is extra state that is easy to get
wrong, such as a count that goes negative or a done step that runs twice.

## Examples

Bad:

```cpp
int pending_count_ = 3;
std::vector<Result> results_;
void OnResult(Result r) {
  results_.push_back(std::move(r));
  if (--pending_count_ == 0) OnAllComplete();
}
```

Good:

```cpp
auto barrier = base::BarrierCallback<Result>(
    3, base::BindOnce(&MyClass::OnAllComplete, weak_factory_.GetWeakPtr()));
service1->Fetch(barrier);
service2->Fetch(barrier);
service3->Fetch(barrier);
```
