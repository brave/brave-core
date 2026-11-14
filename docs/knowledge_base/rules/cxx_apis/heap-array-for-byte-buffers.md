---
name: brave/heap-array-for-byte-buffers
description:
  'An owned byte buffer that is not resized after creation is a
  base::HeapArray<uint8_t>, not a std::vector<uint8_t> or std::vector<unsigned
  char>, except at Mojo or FFI boundaries that require a vector.'
rubric: safety
severity: should_fix
---

**Rule.** For an owned byte buffer whose size is fixed once it is allocated, use
`base::HeapArray<uint8_t>::WithSize(size)` (or `::Uninit(size)` on
performance-sensitive paths where the bytes are overwritten before being read),
and pass it on as `.as_span()`. Do not use `std::vector<uint8_t>` or
`std::vector<unsigned char>` for such a buffer. A vector remains acceptable
where an interface requires one, such as a Mojo or Rust FFI signature, but only
at that boundary.

**Why.** `HeapArray` states that the size is fixed, bounds-checks its indexing,
and converts directly to `base::span`. A vector implies the buffer may grow, and
invites passing `.data()` and `.size()` separately.

## Examples

Bad:

```cpp
std::vector<unsigned char> dat_buffer(size);
ProcessBuffer(dat_buffer.data(), dat_buffer.size());
```

Good:

```cpp
auto dat_buffer = base::HeapArray<uint8_t>::WithSize(size);
ProcessBuffer(dat_buffer.as_span());
```
