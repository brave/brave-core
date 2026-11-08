---
name: brave/consistent-sync-or-async-callbacks
description:
  'A callback always runs synchronously or always asynchronously; a cached or
  early-exit path posts it rather than running it inline when the normal path is
  asynchronous.'
rubric: correctness
severity: blocking
---

**Rule.** A method taking a callback MUST run it the same way on every path:
always before returning, or always later. When the main path completes
asynchronously, a cache hit, an early error or an empty input MUST post the
callback (`base::SequencedTaskRunner::GetCurrentDefault()->PostTask`) instead of
running it inline.

**Why.** A caller cannot know whether its callback has already run when the call
returns, so code after the call, or a callback that deletes the caller, works on
one path and breaks on the other.

## Examples

Bad:

```cpp
void FetchData(Callback cb) {
  if (cache_.contains(key)) {
    std::move(cb).Run(cache_[key]);  // Sync!
  } else {
    network_->Fetch(key, std::move(cb));  // Async!
  }
}
```

Good:

```cpp
void FetchData(Callback cb) {
  if (cache_.contains(key)) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(cb), cache_[key]));
  } else {
    network_->Fetch(key, std::move(cb));
  }
}
```
