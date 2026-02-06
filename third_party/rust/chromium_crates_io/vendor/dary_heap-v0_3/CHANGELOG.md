# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

The changelog of 0.2.x releases for x > 3 can be found [on the
non-const-generics branch](https://github.com/hanmertens/dary_heap/tree/non-const-generics).
The 0.3.0 release was based on 0.2.3, later 0.2.x releases are backports of
0.3.y releases that can be used with older Rust compilers without const generics
support.

## [Unreleased]

## [0.3.8] &ndash; 2025-09-16
### Added
- Add `PeekMut::refresh` when the `unstable` feature is enabled.

### Changed
- Synchronize source code with standard library of Rust version 1.89.0.
- When `extra` is enabled, `PeekMut::pop` is potentially faster.

## [0.3.7] &ndash; 2024-10-18
### Added
- Implement `Default` for `Iter`.

### Changed
- Synchronize source code with standard library of Rust version 1.82.0.
- Make the `new` method `const` when the `extra` feature is enabled. This
  raises the MSRV of the `extra` feature to 1.61.0.
- No longer require the `unstable` feature for the `as_slice` method.

### Fixed
- Compiles again with feature `unstable_nightly` enabled on recent nightly
  after the unstable trait `InPlaceIterable` was changed upstream.

## [0.3.6] &ndash; 2023-06-12
### Added
- Implement `Default` for `IntoIter`.

### Changed
- Synchronize source code with standard library of Rust version 1.70.0.
- The `retain` method no longer requires the `unstable` feature.
- Improve `extend` performance.

## [0.3.5] &ndash; 2023-05-21
### Changed
- Synchronize source code with standard library of Rust version 1.69.0.

### Fixed
- Leaking a `PeekMut` value can no longer lead to an inconsistent state, but it
  can leak other heap elements instead.
- A panic in the closure provided to `retain` can no longer lead to an
  inconsistent state.

## [0.3.4] &ndash; 2022-08-19
### Changed
- Synchronize source code with standard library of Rust version 1.63.0.
- Move `try_reserve` and `try_reserve_exact` methods from `unstable` to `extra`
  feature. This raises the MSRV of the `extra` feature to 1.57.0.

## [0.3.3] &ndash; 2022-02-25
### Added
- Add `try_reserve` and `try_reserve_exact` methods when `unstable` feature is
  enabled.

### Changed
- Synchronize source code with standard library of Rust version 1.61.0.
- Several functions are now marked `must_use` (`new`, `with_capacity`,
  `into_sorted_vec`, `as_slice`, `into_vec`, `peek`, `capacity`, `len`,
  `is_empty`), as well as some iterators (`Iter` and `IntoIterSorted`).

## [0.3.2] &ndash; 2021-10-30
### Added
- Implement array conversion `From<[T; N]>` for `DaryHeap`.
- The feature `extra` is added for non-essential functions that require a higher
  MSRV than the crate otherwise would. This higher MSRV is currently 1.56.0.

### Changed
- Synchronize source code with standard library of Rust version 1.56.0.
- `DaryHeap::shrink_to` no longer needs the `unstable_nightly` flag. Because it
  requires a higher MSRV it is now available under the `extra` feature flag.

### Fixed
- For `unstable_nightly`, fix necessary Rust feature flags since `SourceIter`
  has been marked as `rustc_specialization_trait`.

## [0.3.1] &ndash; 2021-06-18
### Added
- New function `DaryHeap::as_slice` when `unstable` feature is enabled.

### Changed
- Synchronize source code with standard library of Rust version 1.53.0.
- Performance improvement for `DaryHeap::retain`.

### Fixed
- No integer overflow when rebuilding heaps with arities greater than 13 in
  `DaryHeap::append`.

## [0.3.0] &ndash; 2021-03-28
### Changed
- Use const generics to specify arity instead of `Arity` trait.
- Raise MSRV to 1.51.0 for const generics support.

## [0.2.3] &ndash; 2021-03-27
### Changed
- Synchronize source code with standard library of Rust version 1.51.0.
- Performance improvement for `DaryHeap::append`.

## [0.2.2] &ndash; 2021-01-13
### Changed
- Synchronize source code with standard library of Rust version 1.49.0.
- Performance improvements, especially for arities up to four due to specialized
  code for those arities.

## [0.2.1] &ndash; 2020-11-20
### Added
- Implement `SourceIter` and `InplaceIterable` for `IntoIter` when
  `unstable_nightly` is enabled.

### Changed
- Synchronize source code with standard library of Rust version 1.48.0.

## [0.2.0] &ndash; 2020-10-26
### Changed
- Change `serde` serialization format to be the same as sequence types in the
  standard library like `std::collections::BinaryHeap`.
- MSRV lowered to 1.31.0, with caveats (`Vec::from(DaryHeap)` requires 1.41.0+;
  `no_std` support and `serde` feature require 1.36.0+).

### Fixed
- Ensure heaps are valid after deserializing via `serde`.

## [0.1.1] &ndash; 2020-10-08
### Added
- Add support for Serde behind `serde` feature.
- Establish stability guidelines and set MSRV at 1.41.0.

### Changed
- Extra safeguards against constructing and using a nullary heap.
- Simpler unstable Cargo features: `unstable` for everything available on stable
  compilers (previously `drain_sorted`, `into_iter_sorted`, and `retain`) and
  `unstable_nightly` for everything only available on nightly (previously
  `exact_size_is_empty`, `extend_one`, `shrink_to`, and `trusted_len`).
- Synchronize source code with standard library of Rust version 1.47.0.

### Fixed
- Fix division by zero for unary heap in `append`.

## [0.1.0] &ndash; 2020-09-26
### Added
- `DaryHeap` based on `std::collections::BinaryHeap` (Rust version 1.46.0).
- `Arity` trait and `arity` macro to specify heap arity.
- Arity markers for two to eight, `D2`&ndash;`D8`, and type aliases for heaps
  with those arities, `BinaryHeap`&ndash;`OctonaryHeap`.
- Cargo features corresponding to unstable Rust features, specifically the
  features `drain_sorted`, `into_iter_sorted`, and `retain` that are available
  on stable compilers, and the features `exact_size_is_empty`, `extend_one`,
  `shrink_to`, and `trusted_len` that are only available on nightly compilers.

[Unreleased]: https://github.com/hanmertens/dary_heap/compare/v0.3.8...HEAD
[0.3.8]: https://github.com/hanmertens/dary_heap/compare/v0.3.7...v0.3.8
[0.3.7]: https://github.com/hanmertens/dary_heap/compare/v0.3.6...v0.3.7
[0.3.6]: https://github.com/hanmertens/dary_heap/compare/v0.3.5...v0.3.6
[0.3.5]: https://github.com/hanmertens/dary_heap/compare/v0.3.4...v0.3.5
[0.3.4]: https://github.com/hanmertens/dary_heap/compare/v0.3.3...v0.3.4
[0.3.3]: https://github.com/hanmertens/dary_heap/compare/v0.3.2...v0.3.3
[0.3.2]: https://github.com/hanmertens/dary_heap/compare/v0.3.1...v0.3.2
[0.3.1]: https://github.com/hanmertens/dary_heap/compare/v0.3.0...v0.3.1
[0.3.0]: https://github.com/hanmertens/dary_heap/compare/v0.2.3...v0.3.0
[0.2.3]: https://github.com/hanmertens/dary_heap/compare/v0.2.2...v0.2.3
[0.2.2]: https://github.com/hanmertens/dary_heap/compare/v0.2.1...v0.2.2
[0.2.1]: https://github.com/hanmertens/dary_heap/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/hanmertens/dary_heap/compare/v0.1.1...v0.2.0
[0.1.1]: https://github.com/hanmertens/dary_heap/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/hanmertens/dary_heap/releases/tag/v0.1.0
