# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.1.2] - 2025-09-03

[2.1.2]: https://github.com/shepmaster/twox-hash/tree/v2.1.2

### Changed

- The documentation has been updated to account for `XxHash3_128`.

## [2.1.1] - 2025-06-09

[2.1.1]: https://github.com/shepmaster/twox-hash/tree/v2.1.1

### Changed

- The version range for the optional `rand` dependency is now 0.9.

## [2.1.0] - 2024-12-09

[2.1.0]: https://github.com/shepmaster/twox-hash/tree/v2.1.0

### Added

- The XXH3 128-bit algorithm is implemented via `XxHash3_128` and the
  `xxhash3_128` module.

## [2.0.1] - 2024-11-04

[2.0.1]: https://github.com/shepmaster/twox-hash/tree/v2.0.1

### Fixed

- Removed a panic that could occur when using `XxHash3_64` to hash 1
  to 3 bytes of data in debug mode. Release mode and different lengths
  of data are unaffected.

## [2.0.0] - 2024-10-18

[2.0.0]: https://github.com/shepmaster/twox-hash/tree/v2.0.0

This release is a complete rewrite of the crate, including
reorganization of the code. The XXH3 algorithm now matches the 0.8
release of the reference C xxHash implementation.

### Added

- `XxHash32::oneshot` and `XxHash64::oneshot` can perform hashing with
  zero allocation and generally improved performance. If you have code
  that creates a hasher and hashes a slice of bytes exactly once, you
  are strongly encouraged to use the new functions. This might look
  like:

  ```rust
  // Before
  let mut hasher = XxHash64::new(); // or XxHash32, or with seeds
  some_bytes.hash(&mut hasher);
  let hash = hasher.finish();

  // After
  let hash = XxHash64::oneshot(some_bytes);
  ```

- There is a feature flag for each hashing implementation. It is
  recommended that you opt-out of the crate's default features and
  only select the implementations you need to improve compile speed.

### Changed

- The crates minimum supported Rust version (MSRV) is now 1.81.

- Functional and performance comparisons are made against the
  reference C xxHash library version 0.8.2, which includes a stable
  XXH3 algorithm.

- Support for randomly-generated hasher instances is now behind the
  `random` feature flag. It was previously combined with the `std`
  feature flag.

### Removed

- The deprecated type aliases `XxHash` and `RandomXxHashBuilder` have
  been removed. Replace them with `XxHash64` and
  `xxhash64::RandomState` respectively.

- `RandomXxHashBuilder32` and `RandomXxHashBuilder64` are no longer
  available at the top-level of the crate. Replace them with
  `xxhash32::RandomState` and ``xxhash64::RandomState` respectively.

- `Xxh3Hash64` and `xx3::Hash64` have been renamed to `XxHash3_64` and
  `xxhash3_64::Hasher` respectively.

- The free functions `xxh3::hash64`, `xxh3::hash64_with_seed`, and
  `xxh3::hash64_with_secret` are now associated functions of
  `xxhash3_64::Hasher`: `oneshot`, `oneshot_with_seed` and
  `oneshot_with_secret`. Note that the argument order has changed.

- Support for the [digest][] crate has been removed. The digest crate
  is for **cryptographic** hash functions and xxHash is
  **non-cryptographic**.

- `XxHash32` and `XxHash64` no longer implement `Copy`. This prevents
  accidentally mutating a duplicate instance of the state instead of
  the original state. `Clone` is still implemented so you can make
  deliberate duplicates.

- The XXH3 128-bit variant is not yet re-written. Work is in progress
  for this.

- We no longer provide support for randomly-generated instances of the
  XXH3 64-bit variant. The XXH3 algorithm takes both a seed and a
  secret as input and deciding what to randomize is non-trivial and
  can have negative impacts on performance.

[digest]: https://docs.rs/digest/latest/digest/
