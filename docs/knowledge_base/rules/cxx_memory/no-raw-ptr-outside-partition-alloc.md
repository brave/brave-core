---
name: brave/no-raw-ptr-outside-partition-alloc
description:
  'Blink renderer code using Oilpan, and code whose objects live outside
  PartitionAlloc (V8 or Java heaps), must not use raw_ptr<T> or raw_ref<T>.'
rubric: correctness
severity: blocking
---

**Rule.** Do not use `raw_ptr<T>` or `raw_ref<T>` in Blink renderer code that
uses Oilpan, or for objects allocated outside PartitionAlloc (the V8 heap, the
Java heap and similar). Use the memory system's own reference types there, such
as `Member<T>` / `WeakMember<T>` for Oilpan objects.

**Why.** `raw_ptr` protects only memory allocated by PartitionAlloc. Objects on
other heaps are managed by their own garbage collectors, where these wrappers do
not apply.

## Examples

Bad:

```cpp
class BraveFooAgent : public GarbageCollected<BraveFooAgent> {
  raw_ptr<LocalFrame> frame_;
};
```

Good:

```cpp
class BraveFooAgent : public GarbageCollected<BraveFooAgent> {
  Member<LocalFrame> frame_;
};
```
