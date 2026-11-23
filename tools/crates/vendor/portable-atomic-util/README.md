# portable-atomic-util

[![crates.io](https://img.shields.io/crates/v/portable-atomic-util?style=flat-square&logo=rust)](https://crates.io/crates/portable-atomic-util)
[![docs.rs](https://img.shields.io/badge/docs.rs-portable--atomic--util-blue?style=flat-square&logo=docs.rs)](https://docs.rs/portable-atomic-util)
[![license](https://img.shields.io/badge/license-Apache--2.0_OR_MIT-blue?style=flat-square)](#license)
[![msrv](https://img.shields.io/badge/msrv-1.36-blue?style=flat-square&logo=rust)](https://www.rust-lang.org)
[![github actions](https://img.shields.io/github/actions/workflow/status/taiki-e/portable-atomic-util/ci.yml?branch=main&style=flat-square&logo=github)](https://github.com/taiki-e/portable-atomic-util/actions)

<!-- tidy:sync-markdown-to-rustdoc:start:src/lib.rs -->

Synchronization primitives built with [portable-atomic].

- Provide `Arc`. (optional, requires the `std` or `alloc` feature)
- Provide `task::Wake`. (optional, requires the `std` or `alloc` feature)
<!-- - Provide generic `Atomic<T>` type. (optional, requires the `generic` feature) -->

See [portable-atomic#1] for other primitives being considered for addition to this crate.

This crate was originally [part of the portable-atomic repository](https://github.com/taiki-e/portable-atomic/tree/cbbee0c0d202a5944f7d66aaafaac6ed76e6f599/portable-atomic-util) and was extracted into its own repository.

## Optional features

- **`std`**<br>
  Use `std`.

  Note:
  - This implicitly enables the `alloc` feature.

- **`alloc`**<br>
  Use `alloc`.

- **`serde`**<br>
  Implement `serde::{Serialize, Deserialize}` for `Arc`.

  Note:
  - The MSRV when this feature is enabled is the one coming from serde, as of now it's Rust 1.56

<!-- TODO: https://github.com/taiki-e/portable-atomic/issues/1
- **`generic`**<br>
  Provides generic `Atomic<T>` type.
-->

[portable-atomic]: https://github.com/taiki-e/portable-atomic
[portable-atomic#1]: https://github.com/taiki-e/portable-atomic/issues/1

## Optional cfg

One of the ways to enable cfg is to set [rustflags in the cargo config](https://doc.rust-lang.org/cargo/reference/config.html#targettriplerustflags):

```toml
# .cargo/config.toml
[target.<target>]
rustflags = ["--cfg", "portable_atomic_unstable_coerce_unsized"]
```

Or set environment variable:

```sh
RUSTFLAGS="--cfg portable_atomic_unstable_coerce_unsized" cargo ...
```

- <a name="optional-cfg-unstable-coerce-unsized"></a>**`--cfg portable_atomic_unstable_coerce_unsized`**<br>
  Support coercing of `Arc<T>` to `Arc<U>` as in `std::sync::Arc`.

  <!-- TODO: add coercing of `Weak<T>` to `Weak<U>` as well, with testing & documentation updates -->

  This cfg requires Rust nightly because this coercing requires [unstable `CoerceUnsized` trait](https://doc.rust-lang.org/nightly/core/ops/trait.CoerceUnsized.html).

  See [this issue comment](https://github.com/taiki-e/portable-atomic/issues/143#issuecomment-1866488569) for another known workaround.

  **Note:** This cfg is unstable and outside of the normal semver guarantees and minor or patch versions of portable-atomic-util may make breaking changes to them at any time.

## Related Projects

- [portable-atomic]: Portable atomic types including support for 128-bit atomics, atomic float, etc.
- [atomic-maybe-uninit]: Atomic operations on potentially uninitialized integers.
- [atomic-memcpy]: Byte-wise atomic memcpy.

[atomic-maybe-uninit]: https://github.com/taiki-e/atomic-maybe-uninit
[atomic-memcpy]: https://github.com/taiki-e/atomic-memcpy

<!-- tidy:sync-markdown-to-rustdoc:end -->

## License

Licensed under either of [Apache License, Version 2.0](LICENSE-APACHE) or
[MIT license](LICENSE-MIT) at your option.

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in the work by you, as defined in the Apache-2.0 license, shall
be dual licensed as above, without any additional terms or conditions.
