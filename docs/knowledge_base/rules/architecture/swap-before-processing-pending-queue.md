---
name: brave/swap-before-processing-pending-queue
description:
  'Before running a member queue of pending callbacks or items, move it into a
  local with std::exchange and iterate the local, so reentrant additions cannot
  corrupt the loop.'
rubric: correctness
severity: blocking
---

**Rule.** To process a member queue of pending callbacks or items, first move it
into a local (`auto callbacks = std::exchange(pending_callbacks_, {});`) and
iterate the local. Do not iterate the member while running callbacks, and clear
it afterwards.

**Why.** Running an item can re-enter the class and add to the queue, which
invalidates the iterators of a loop over the member (a use-after-free), or keeps
extending the loop forever; and the `clear()` afterwards drops the items added
meanwhile. Items queued while the local is processed wait for the next round.

## Examples

Bad:

```cpp
for (auto& callback : pending_callbacks_) {
  std::move(callback).Run();  // might add to pending_callbacks_!
}
pending_callbacks_.clear();
```

Good:

```cpp
auto callbacks = std::exchange(pending_callbacks_, {});
for (auto& callback : callbacks) {
  std::move(callback).Run();
}
```
