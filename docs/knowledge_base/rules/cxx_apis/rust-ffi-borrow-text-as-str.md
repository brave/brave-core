---
name: brave/rust-ffi-borrow-text-as-str
description:
  'Read-only text crossing a cxx bridge in either direction is &str on the Rust
  side (rust::Str in C++), never &CxxString; C++ reads a rust::Str as a view and
  does not store it.'
rubric: perf
severity: should_fix
---

**Rule.** When text is only read by the other side of a `#[cxx::bridge]`
function, declare it as `&str` in the bridge, which C++ sees as `rust::Str`:

- Rust to C++ (`unsafe extern "C++"` functions): take `name: &str`, not
  `name: &CxxString`. In C++, treat the `rust::Str` like a `std::string_view`
  (`base::RustStrToStringView()`): read it, do not store it.
- C++ to Rust (`extern "Rust"` functions): take `key: &str`, not
  `key: &CxxString`; C++ passes a `std::string_view` or `std::string`, which
  converts implicitly.

**Why.** `&str` crosses the bridge as a pointer and a length, without any copy.
`&CxxString` forces a C++ `std::string` to exist for a borrow, and leaks the
bridge type into Rust code that then cannot use ordinary string methods.

## Examples

Bad:

```rust
unsafe extern "C++" {
    fn log_event(name: &CxxString);
}
fn is_valid_key(key: &CxxString) -> bool;
```

Good:

```rust
unsafe extern "C++" {
    fn log_event(name: &str);
}
fn is_valid_key(key: &str) -> bool {
    KNOWN_KEYS.contains(&key)
}
```
