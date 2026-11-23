# Changelog

All notable changes to this project will be documented in this file.

This project adheres to [Semantic Versioning](https://semver.org).

Releases may yanked if there is a security bug, a soundness bug, or a regression.

<!--
Note: In this file, do not use the hard wrap in the middle of a sentence for compatibility with GitHub comment style markdown rendering.
-->

## [Unreleased]

## [0.2.8] - 2026-09-04

- Increase the minimum supported Rust version from Rust 1.34 to Rust 1.36. Using the `alloc` feature without the `std` feature already required Rust 1.36, so this does not change anything for such no-std use cases. ([#8](https://github.com/taiki-e/portable-atomic-util/pull/8))

- Add `Arc::map`. (align to the [std `Arc` change in Rust 1.100](https://github.com/rust-lang/rust/pull/160534)) ([#10](https://github.com/taiki-e/portable-atomic-util/pull/10), thanks @c410-f3r)

- Fix thread sanitizer false positives when using x86_64-unknown-linux-gnutsan target in stable Rust. ([e02eca7](https://github.com/taiki-e/portable-atomic-util/commit/e02eca7cc7d8d0c2d854fb7828b0fb2a95071702))

## [0.2.7] - 2026-04-16

- Implement serde serialization and deserialization for `Arc`, gated behind "serde" feature. ([#2](https://github.com/taiki-e/portable-atomic-util/pull/2), thanks @tommasoclini)

## [0.2.6] - 2026-03-14

portable-atomic-util has been extracted from the portable-atomic repository into its own repository: <https://github.com/taiki-e/portable-atomic-util>

## [0.2.5] - 2026-01-31

- Add `Arc::{new_zeroed,new_zeroed_slice}` at Rust 1.36+. (align to the [std `Arc` change in Rust 1.92](https://github.com/rust-lang/rust/pull/144091)) ([f8affb6](https://github.com/taiki-e/portable-atomic/commit/f8affb661abb1ac928cd78a66bf8fe1e72c42e42), [ae5aba7](https://github.com/taiki-e/portable-atomic/commit/ae5aba7cbe182b5d83b4b4973ccdaab372bef4d6))

- Implement `Default` for `Pin<Arc<T>: Default>`. (align to the [std `Arc` change in Rust 1.91](https://github.com/rust-lang/rust/pull/143717)) ([2d8d33c](https://github.com/taiki-e/portable-atomic/commit/2d8d33c127126e9d290c6f368f54c2a784b43b57))

- Implement `From<&mut {[T],str}>` for `Arc<{[T],str}>` at Rust 1.36+. (align to the [std `Arc` change in Rust 1.84](https://github.com/rust-lang/rust/pull/129329)) ([99640d6](https://github.com/taiki-e/portable-atomic/commit/99640d656fb93c54e3fc255b820f61653d6c425e), [ae5aba7](https://github.com/taiki-e/portable-atomic/commit/ae5aba7cbe182b5d83b4b4973ccdaab372bef4d6))

- Implement `{AsFd, AsRawFd}` for `Arc<T>` on Trusty. ([1b09ffb](https://github.com/taiki-e/portable-atomic/commit/1b09ffbc3010dc64b16e4fdf39742caee933ec7d))

- Support slice-related methods that previously required Rust 1.44+ at Rust 1.36+. ([ae5aba7](https://github.com/taiki-e/portable-atomic/commit/ae5aba7cbe182b5d83b4b4973ccdaab372bef4d6))

- Support `AsRawFd for Arc<T>` implementation on Unix in all Rust versions. Previously, it was only for Rust 1.63+. ([1b09ffb](https://github.com/taiki-e/portable-atomic/commit/1b09ffbc3010dc64b16e4fdf39742caee933ec7d))

- Fix build error when building for HermitOS with `std` feature in Rust 1.63-1.68. ([1b09ffb](https://github.com/taiki-e/portable-atomic/commit/1b09ffbc3010dc64b16e4fdf39742caee933ec7d))

- Documentation improvements.

- Enable [release immutability](https://docs.github.com/en/code-security/supply-chain-security/understanding-your-software-supply-chain/immutable-releases).

## [0.2.4] - 2024-11-23

- Add unstable `portable_atomic_unstable_coerce_unsized` cfg (requires Rust nightly). ([#195](https://github.com/taiki-e/portable-atomic/pull/195), thanks @brodycj)

- Respect [`RUSTC_BOOTSTRAP=-1` recently added in nightly](https://github.com/rust-lang/rust/pull/132993) in rustc version detection. ([5b2847a](https://github.com/taiki-e/portable-atomic/commit/5b2847a8b99aa2a57a6c80f5a47327b2764f08cc))

## [0.2.3] - 2024-10-17

- Add `Arc::{new_uninit,assume_init}` at Rust 1.36+ and `Arc::new_uninit_slice` at Rust 1.44+. (align to the [std `Arc` change in Rust 1.82](https://github.com/rust-lang/rust/pull/129401)) ([362dc9a](https://github.com/taiki-e/portable-atomic/commit/362dc9af2779c81aa346e89c4d3f3eef71cf29ed))

- Support `make_mut` on `Arc<[T]>` and `Arc<str>` at Rust 1.36+. (align to the [std `Arc` change in Rust 1.81](https://github.com/rust-lang/rust/pull/116113)) ([362dc9a](https://github.com/taiki-e/portable-atomic/commit/362dc9af2779c81aa346e89c4d3f3eef71cf29ed))

## [0.2.2] - 2024-07-11

- Fix [build issue with `esp` toolchain](https://github.com/taiki-e/semihosting/issues/11). ([f8ea85e](https://github.com/taiki-e/portable-atomic/commit/f8ea85e1aa46fa00bc865633fb40b05f8a0c823b))

## [0.2.1] - 2024-06-22

**Note:** This release has been yanked due to an issue fixed in 0.2.2.

- Support `impl Error for Arc<T: Error>` in no-std at Rust 1.81+. ([30b9f90](https://github.com/taiki-e/portable-atomic/commit/30b9f90346dfad14ab00f1c7e1f988f941330bcf))

- Implement `Default` for `Arc<[T]>` and `Arc<str>` at Rust 1.51+. (align to the [std `Arc` change in Rust 1.80](https://github.com/rust-lang/rust/pull/124640)) ([c6ee296](https://github.com/taiki-e/portable-atomic/commit/c6ee29606984863d008c2cf2209751ed0fa43b14))

- Implement `{AsFd, AsRawFd}` for `Arc<T>` on HermitOS. ([b778244](https://github.com/taiki-e/portable-atomic/commit/b778244917e17bfc431c9add4d028ff26d00e3b7))

## [0.2.0] - 2024-05-07

- Rewrite `Arc` based on `std::sync::Arc`'s implementation. ([#142](https://github.com/taiki-e/portable-atomic/pull/142))

  This fixes accidental API differences with std ([#139](https://github.com/taiki-e/portable-atomic/issues/139), [#140](https://github.com/taiki-e/portable-atomic/issues/140)) and adds many missing APIs compared to std:
  - Add `Arc::{downcast, into_inner, make_mut, new_cyclic}` ([#142](https://github.com/taiki-e/portable-atomic/pull/142))
  - Implement `{fmt::Display, fmt::Pointer, Error, From<T>, From<Box<T>>, From<Cow<'a,T>>, AsFd, AsRawFd, AsHandle, AsSocket}` for `Arc<T>` ([#142](https://github.com/taiki-e/portable-atomic/pull/142), [78690d7](https://github.com/taiki-e/portable-atomic/commit/78690d7cad3b394119ea147c5773f67806a6ac09), [aba0930](https://github.com/taiki-e/portable-atomic/commit/aba0930269d7075b81810b49bbbbb6c5edc85ea0))
  - Implement `{From<&[T]>, From<Vec<T>>, From<[T; N]>, FromIterator<T>}` for `Arc<[T]>` ([#142](https://github.com/taiki-e/portable-atomic/pull/142), [5e9f693](https://github.com/taiki-e/portable-atomic/commit/5e9f693dcb43c35187ca95ce1c824e0cb1d3c4f8))
  - Implement `TryFrom<Arc<[T]>>` for `Arc<[T; N]>` ([#142](https://github.com/taiki-e/portable-atomic/pull/142))
  - Implement `From<Arc<str>>` for `Arc<[u8]>` ([#142](https://github.com/taiki-e/portable-atomic/pull/142))
  - Implement `{From<&str>, From<String>}` for `Arc<str>` ([#142](https://github.com/taiki-e/portable-atomic/pull/142))
  - Implement `{Read, Write, Seek}` for `Arc<File>` ([591ece5](https://github.com/taiki-e/portable-atomic/commit/591ece5bde0f19f1895853791924ee55c51ee61e))
  - Remove `T: UnwindSafe` bound from `impl UnwindSafe for Arc<T>` ([#142](https://github.com/taiki-e/portable-atomic/pull/142))

- Add `task::Wake`. ([#145](https://github.com/taiki-e/portable-atomic/pull/145))

  This is equivalent to `std::task::Wake`, but using `portable_atomic_util::Arc` as a reference-counted pointer.

- Respect `RUSTC_WRAPPER` in rustc version detection.

## [0.1.5] - 2023-12-17

- Improve offset calculation in `Arc::{into_raw,as_ptr,from_ptr}`. ([#141](https://github.com/taiki-e/portable-atomic/pull/141), thanks @gtsiam)

## [0.1.4] - 2023-12-16

- Fix a bug where `Arc::{into_raw,as_ptr}` returned invalid pointers for larger alignment types. ([#138](https://github.com/taiki-e/portable-atomic/pull/138), thanks @notgull)

## [0.1.3] - 2023-05-06

**Note:** This release has been yanked due to a bug fixed in 0.1.4.

- Enable `portable-atomic`'s `require-cas` feature to display helpful error messages to users on targets requiring additional action on the user side to provide atomic CAS. ([#100](https://github.com/taiki-e/portable-atomic/pull/100))

## [0.1.2] - 2023-04-04

**Note:** This release has been yanked due to a bug fixed in 0.1.4.

- Implement `AsRef`, `Borrow`, and `Unpin` on `Arc`. ([#92](https://github.com/taiki-e/portable-atomic/pull/92) [#93](https://github.com/taiki-e/portable-atomic/pull/93), thanks @notgull)

## [0.1.1] - 2023-03-24

**Note:** This release has been yanked due to a bug fixed in 0.1.4.

- Prevent weak counter overflow in `Arc::downgrade`. ([#83](https://github.com/taiki-e/portable-atomic/pull/83))

  This fixes [a potential unsoundness recently found in the standard library's `Arc`](https://github.com/rust-lang/rust/issues/108706).

## [0.1.0] - 2023-01-15

**Note:** This release has been yanked due to a bug fixed in 0.1.4.

Initial release

[Unreleased]: https://github.com/taiki-e/portable-atomic-util/compare/v0.2.8...HEAD
[0.2.8]: https://github.com/taiki-e/portable-atomic-util/compare/v0.2.7...v0.2.8
[0.2.7]: https://github.com/taiki-e/portable-atomic-util/compare/v0.2.6...v0.2.7
[0.2.6]: https://github.com/taiki-e/portable-atomic-util/compare/v0.2.5...v0.2.6
[0.2.5]: https://github.com/taiki-e/portable-atomic-util/compare/v0.2.4...v0.2.5
[0.2.4]: https://github.com/taiki-e/portable-atomic-util/compare/v0.2.3...v0.2.4
[0.2.3]: https://github.com/taiki-e/portable-atomic-util/compare/v0.2.2...v0.2.3
[0.2.2]: https://github.com/taiki-e/portable-atomic-util/compare/v0.2.1...v0.2.2
[0.2.1]: https://github.com/taiki-e/portable-atomic-util/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/taiki-e/portable-atomic-util/compare/v0.1.5...v0.2.0
[0.1.5]: https://github.com/taiki-e/portable-atomic-util/compare/v0.1.4...v0.1.5
[0.1.4]: https://github.com/taiki-e/portable-atomic-util/compare/v0.1.3...v0.1.4
[0.1.3]: https://github.com/taiki-e/portable-atomic-util/compare/v0.1.2...v0.1.3
[0.1.2]: https://github.com/taiki-e/portable-atomic-util/compare/v0.1.1...v0.1.2
[0.1.1]: https://github.com/taiki-e/portable-atomic-util/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/taiki-e/portable-atomic-util/releases/tag/v0.1.0
