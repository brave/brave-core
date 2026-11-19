---
name: brave/span-params-over-const-vector-refs
description:
  'A function parameter that only reads a contiguous sequence takes
  base::span<const T>, not const std::vector<T>&, especially for byte buffers.'
rubric: design
severity: should_fix
---

**Rule.** When a function only reads the elements of a sequence, declare the
parameter as `base::span<const T>` (passed by value) instead of
`const std::vector<T>&`. This applies above all to byte buffers
(`base::span<const uint8_t>`). Keep `const std::vector<T>&` only where the
callee needs the vector itself, for example to copy it whole into a member of
that type or to pass it to an API that requires a vector.

**Why.** A span is a cheap, non-owning view that accepts any contiguous
container (`std::vector`, `base::HeapArray`, `std::array`, C arrays,
`base::FixedArray`). A `const std::vector&` parameter forces every caller to
hold its data in a vector, often by copying it into one first.

## Examples

Bad:

```cpp
void ProcessBuffer(const std::vector<uint8_t>& data);
```

Good:

```cpp
void ProcessBuffer(base::span<const uint8_t> data);
```
