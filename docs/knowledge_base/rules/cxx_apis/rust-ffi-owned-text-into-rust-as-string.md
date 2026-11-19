---
name: brave/rust-ffi-owned-text-into-rust-as-string
description:
  'Normal-sized text that Rust keeps after a call from C++ is taken as String
  (deep-copied by C++ at the call site), not borrowed as &CxxString into
  long-lived Rust state.'
rubric: safety
severity: should_fix
---

**Rule.** A bridge function through which C++ gives Rust text that Rust stores
(in a struct, a cache, a global) takes `String`. C++ copies its text into a
`rust::String` at the call site. Do not take `&CxxString` and keep the borrow or
the C++ container in Rust state.

**Why.** Borrowing a `CxxString` into long-lived Rust state ties Rust's lifetime
to a C++ container it does not control. A `String` gives Rust its own idiomatic,
un-aliased copy once the call returns.

## Examples

Bad:

```rust
fn store_name(name: &CxxString);
```

Good:

```rust
fn store_name(name: String);
```

```cpp
std::string computed = base::StrCat({prefix, suffix});
ffi::store_name(rust::String(computed));  // deep copy into Rust heap
```
