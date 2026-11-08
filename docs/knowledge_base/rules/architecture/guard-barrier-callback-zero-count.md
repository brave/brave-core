---
name: brave/guard-barrier-callback-zero-count
description:
  'Handle a count of zero before constructing base::BarrierCallback, which
  otherwise runs its done callback synchronously, inside the BarrierCallback()
  call.'
rubric: correctness
severity: blocking
---

**Rule.** When the count passed to `base::BarrierCallback` can be zero (the size
of a list that may be empty), check for zero first and return early, posting the
done callback with an empty result when it normally runs asynchronously. Do not
construct the barrier with a count that may be zero.

**Why.** With a count of zero, `BarrierCallback` runs the done callback at once,
inside its own construction: before the code following it, and synchronously
where the non-empty case is asynchronous, which breaks callers that rely on the
callback running later (see `brave/consistent-sync-or-async-callbacks`).

## Examples

Bad:

```cpp
auto barrier = base::BarrierCallback<Result>(items.size(), done_callback);
for (const auto& item : items) {
  FetchAsync(item, barrier);
}
```

Good:

```cpp
if (items.empty()) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(done_callback), std::vector<Result>()));
  return;
}
auto barrier =
    base::BarrierCallback<Result>(items.size(), std::move(done_callback));
```
