---
name: brave/raw-ptr-exclusion-only-for-listed-cases
description:
  'RAW_PTR_EXCLUSION is used on a field only for unprotected memory, fields that
  cannot compile with raw_ptr, or a measured regression or runtime error.'
rubric: safety
severity: should_fix
---

**Rule.** Annotate a pointer field with `RAW_PTR_EXCLUSION` only when it is one
of these cases, and otherwise make it a `raw_ptr<T>`:

- it points to unprotected memory where BackupRefPtr gives no benefit: literals,
  stack allocations, `mmap`'d or shared memory, V8/Oilpan/Java heaps, TLS;
- it won't compile with `raw_ptr<T>`: a pointer field in a union (prefer
  `std::variant`), or in a class or struct used as a `global`, `static` or
  `thread_local` variable;
- (very rare) a measured, documented performance regression;
- (very rare) `raw_ptr<T>` causes runtime errors, with a comment explaining it
  (consult `base/memory/raw_ptr.md`).

**Why.** An excluded field loses the dangling-pointer protection `raw_ptr<T>`
provides, so the exclusion is reserved for fields where that protection cannot
work or cannot be afforded.

## Examples

Bad:

```cpp
class Fetcher {
  RAW_PTR_EXCLUSION PrefService* prefs_;  // heap object, no reason given
};
```

Good:

```cpp
union Storage {
  RAW_PTR_EXCLUSION Node* node;
  uintptr_t bits;
};
```
