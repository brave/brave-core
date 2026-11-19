---
name: brave/rust-ffi-string-collections
description:
  'Collections of strings cross a cxx bridge by the same borrow-or-own split:
  &[&str] to read, UniquePtr<CxxVector<CxxString>> for large owned lists,
  Vec<String> for small ones, and almost never &CxxVector<CxxString>.'
rubric: perf
severity: should_fix
---

**Rule.** Pick the bridge type of a list of strings by data flow and size:

- Rust to C++, read-only: `&[&str]`.
- C++ to Rust, ownership of a large list: `UniquePtr<CxxVector<CxxString>>`,
  passed from C++ as `std::make_unique<std::vector<std::string>>(...)`.
- C++ to Rust, ownership of a small list: `Vec<String>` (a deep copy).

Do not use `&CxxVector<CxxString>` unless none of these fits.

**Why.** `&[&str]` borrows without any allocation, a `UniquePtr` hands over a
large list without converting each element, and `Vec<String>` gives Rust a fully
independent copy. `&CxxVector<CxxString>` forces C++ containers on a borrow and
exposes them to Rust code.

## Examples

Bad:

```rust
unsafe extern "C++" {
    fn process(keys: &CxxVector<CxxString>);
}
```

Good:

```rust
unsafe extern "C++" {
    fn process(keys: &[&str]);
}
fn ingest(items: UniquePtr<CxxVector<CxxString>>);
fn ingest_small(items: Vec<String>);
```
