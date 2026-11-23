// SPDX-License-Identifier: Apache-2.0 OR MIT

/*!
<!-- Note: Document from sync-markdown-to-rustdoc:start through sync-markdown-to-rustdoc:end
     is synchronized from README.md. Any changes to that range are not preserved. -->
<!-- tidy:sync-markdown-to-rustdoc:start -->

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
*/

#![no_std]
#![doc(test(
    no_crate_inject,
    attr(allow(
        dead_code,
        unused_variables,
        clippy::undocumented_unsafe_blocks,
        clippy::unused_trait_names,
    ))
))]
#![cfg_attr(not(portable_atomic_no_unsafe_op_in_unsafe_fn), warn(unsafe_op_in_unsafe_fn))] // unsafe_op_in_unsafe_fn requires Rust 1.52
#![cfg_attr(portable_atomic_no_unsafe_op_in_unsafe_fn, allow(unused_unsafe))]
#![warn(
    // Lints that may help when writing public library.
    missing_debug_implementations,
    missing_docs,
    clippy::alloc_instead_of_core,
    clippy::exhaustive_enums,
    clippy::exhaustive_structs,
    clippy::impl_trait_in_params,
    clippy::std_instead_of_alloc,
    clippy::std_instead_of_core,
    // clippy::missing_inline_in_public_items,
)]
#![cfg_attr(portable_atomic_no_strict_provenance, allow(unstable_name_collisions))]
#![allow(clippy::inline_always)]
// docs.rs only (cfg is enabled by docs.rs, not build script)
#![cfg_attr(docsrs, feature(doc_cfg))]
#![cfg_attr(docsrs, doc(auto_cfg = false))]
// Enable custom unsized coercions if the user explicitly opts-in to unstable cfg
#![cfg_attr(portable_atomic_unstable_coerce_unsized, feature(coerce_unsized, unsize))]

#[cfg(feature = "alloc")]
extern crate alloc;
#[cfg(feature = "std")]
extern crate std;

#[macro_use]
mod utils;

#[cfg(feature = "alloc")]
mod arc;
#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(any(feature = "alloc", feature = "std"))))]
pub use self::arc::{Arc, Weak};

#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(any(feature = "alloc", feature = "std"))))]
pub mod task;
