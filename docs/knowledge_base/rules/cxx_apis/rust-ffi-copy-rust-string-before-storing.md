---
name: brave/rust-ffi-copy-rust-string-before-storing
description:
  'Owned text returned from Rust to C++ is a String (rust::String) used for the
  scope of the call; C++ deep-copies it into std::string to keep it, and never
  stores rust::String as a class member.'
rubric: safety
severity: blocking
---

**Rule.** A bridge function that hands owned, normal-sized text from Rust to C++
returns `String` (`rust::String` in C++). The C++ caller uses the `rust::String`
within the calling scope; to keep the text beyond it, copy it into a
`std::string` (`assign(r.data(), r.length())`). NEVER declare a `rust::String`
data member, or keep a `rust::String` in any other long-lived C++ storage.

**Why.** A `rust::String` is a Rust-allocated buffer in the bridge's layout:
holding one in a C++ class ties that class to Rust's allocator and to `cxx`
internals for its whole lifetime. Copying at the boundary leaves C++ with an
ordinary `std::string`.

## Examples

Bad:

```cpp
class Reporter {
  rust::String last_report_;
};
void Reporter::Refresh() {
  last_report_ = ffi::build_report();
}
```

Good:

```cpp
class Reporter {
  std::string last_report_;
};
void Reporter::Refresh() {
  rust::String r = ffi::build_report();
  last_report_.assign(r.data(), r.length());
}
```
