---
name: brave/rust-ffi-large-payloads-by-pointer
description:
  'Massive payloads cross a cxx bridge as a pointer move: Box<Vec<u8>> from Rust
  to C++, UniquePtr<CxxString> from C++ to Rust, not String.'
rubric: perf
severity: should_fix
---

**Rule.** When a bridge function transfers ownership of a large payload (file
contents, JSON blobs, streamed data):

- Rust to C++: return `Box<Vec<u8>>` (`rust::Box<rust::Vec<uint8_t>>`), and have
  C++ read it through a `base::span<const uint8_t>` or `std::string_view`.
- C++ to Rust: take `UniquePtr<CxxString>`, and have C++ pass
  `std::make_unique<std::string>(...)` with `std::move`.

Do not return or take `String` for such payloads.

**Why.** A `String` crossing the bridge is copied, from `rust::String` layout or
into the Rust heap. A boxed or `UniquePtr` payload moves only a pointer, and the
side that allocated it frees it when the owner is dropped.

## Examples

Bad:

```rust
fn produce_payload() -> String;
fn ingest(blob: String);
```

Good:

```rust
fn produce_payload() -> Box<Vec<u8>>;
fn ingest(blob: UniquePtr<CxxString>);
```
