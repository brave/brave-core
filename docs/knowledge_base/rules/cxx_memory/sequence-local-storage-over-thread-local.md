---
name: brave/sequence-local-storage-over-thread-local
description:
  'Sequence-aware code uses base::SequenceLocalStorageSlot instead of
  thread_local.'
rubric: design
severity: should_fix
---

**Rule.** For state that is per sequence in code that runs on task runners, use
`base::SequenceLocalStorageSlot<T>` rather than a `thread_local` variable.

**Why.** Chromium code runs on sequences, which may hop between threads.
`SequenceLocalStorageSlot` integrates with the task runner model and avoids the
strict requirements Chromium places on `thread_local`.

## Examples

Bad:

```cpp
thread_local int g_request_count = 0;
```

Good:

```cpp
base::SequenceLocalStorageSlot<int>& GetRequestCountSlot() {
  static base::NoDestructor<base::SequenceLocalStorageSlot<int>> slot;
  return *slot;
}
```
