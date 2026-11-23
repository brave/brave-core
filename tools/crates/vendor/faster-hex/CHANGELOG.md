# Changelog

## 2.0.0

- Replace the `heapless-08` feature and `heapless_08` module with `heapless-09`
  and `heapless_09`. Both fixed-capacity encoders now return `heapless::String<N>`
  from the 0.9 series. The minimum version is 0.9.4, which includes the
  [upstream soundness fixes](https://rustsec.org/packages/heapless.html).
- Add `defmt` as the recommended feature name for logging with `defmt` 1.x.
  The existing `defmt-03` feature remains available.
- For downstream dependency and import changes, see the
  [1.0 to 2.0 migration guide](MIGRATION.md#migrating-from-10-to-20).

## 1.0.0

Changes since 0.10.1. See [MIGRATION.md](MIGRATION.md) for upgrading existing code.

### Breaking changes

- Require Rust 1.95.0. Edition 2018 and the default `std, serde` features are retained.
- Decode the complete input and return the written prefix as `Result<&mut [u8], Error>`.
  A short destination returns an error; extra capacity remains untouched. Odd input
  is rejected, including when the destination is empty.
- Make `Error` non-exhaustive. Replace `InvalidLength(n)` with
  `OutputTooSmall { required, .. }`; add `OddLength` and byte positions to
  `InvalidChar { index, byte, .. }`. Error formatting changes.
- Remove public backend, fallback and unchecked functions, and the deprecated `hex_to`.
- Make heapless support opt-in through `heapless-08` and the `heapless_08` module.
  `hex_string` and `hex_string_upper` require `alloc` and always return `String`.

### Added

- Export `CheckCase` and `hex_decode_with_case` for case-specific decoding.
- Add `hex_append` and `hex_append_upper` for reusing a `String` allocation.
- Add exact `hex_decode_array::<N>` and owned `hex_decode_vec` decoding, each with
  a case-policy variant. Array length errors use `LengthMismatch { expected, actual, .. }`.
- Add borrowed `Hex` formatting through `core::fmt`, including integer-style flags.
- Add Serde Option policies, fixed-array adapters and `deserialize_bounded` helpers.
- Add AArch64 NEON encoding, validation and decoding.
- Add AVX-512BW validation and decoding with runtime CPU/OS detection.
- Add `defmt-03` support and implement `Hash` for `Error` and `CheckCase`.

### Fixed

- Preserve the entire destination on checked slice conversion errors.
- Keep SIMD accesses within their slices at short lengths and independent alignments.
- Check supported CPUID leaves and OS AVX/ZMM state before selecting AVX2 or AVX-512.
- Preserve Serde's `Some` tag in binary formats and read each serialized `AsRef<[u8]>`
  view once. Existing JSON prefix and case rules remain unchanged.
- Support dependency-free core builds and Serde with `alloc` but without `std`.
  Implement `core::error::Error` with every feature set.

### Performance and validation

- Refine short-input SIMD decoding, SSE4.1 conversion, AVX2 validation and scalar paths.
- Reuse the existing NEON block for 4-byte array decoding.
- Keep 65–96-byte x86 slice decoding in registers until validation succeeds.
- Decode large owned arrays and vectors in one checked pass using existing SIMD
  blocks; discard private output on error and retain exact error positions.
- Encode strings and Serde output directly into uninitialized storage; borrow Serde input and
  use stack storage for serialized fixed values through 65 bytes, including their prefix.
- Avoid clearing unused `Hex` scratch storage, stream long values in larger chunks,
  and reduce short-value formatting and padding overhead.
- Add differential tests, guard pages, feature consumers, Miri checks and fuzz targets.
- Exercise independent decode lengths at guard pages and run 32-bit x86 tests.
- Add explicit native AVX-512 fuzz acceptance for libFuzzer and AFL, requiring
  execution inside every AVX-512 kernel after corpus minimization.
- Check structured valid and damaged inputs, allocation-free contracts, concurrent
  dispatch, fixed-capacity boundaries and partial writer failures.
- Expand CI across operating systems, architectures and feature combinations, with
  Wasm/embedded/32-bit builds and native x86 backend checks.
- Keep focused Criterion suites for slice and owned conversion, validation, capacity reuse,
  borrowed formatting and Serde. Development aliases support saved baselines and
  competitor comparisons; CI executes the cases without collecting timings.

## 0.10.1

### Bug fixes

- Bound AVX2 reads in `hex_decode_unchecked` by the source length.
- Return only the written prefix from `hex_encode` and `hex_encode_upper`.
- Restore the `Error` trait implementation on Rust 1.61 while retaining
  `core::error::Error` on newer toolchains.

## 0.10.0

### Features

* Add PartialEq derived macro to Error in order to be able to test error cases
* Allow `[no_alloc]` for faster-hex (without any feature), hex_string(_upper)() rely on heapless:String

## [0.9.0](https://github.com/nervosnetwork/faster-hex/compare/v0.8.2...v0.9.0) (2023-11-22)
Re create `v0.9.0`, since `v0.8.2` introduced a [break change](https://github.com/nervosnetwork/faster-hex/issues/43#issuecomment-1822551961),

## Yanked: [0.8.2](https://github.com/nervosnetwork/faster-hex/compare/v0.8.1...v0.8.2) (2023-11-19)

### Bug Fixes

* Fix `hex_decode` panic when `dst.len` > `src.len * 2` [pr#38](https://github.com/nervosnetwork/faster-hex/pull/38)

## [0.8.1](https://github.com/nervosnetwork/faster-hex/compare/v0.8.0...v0.8.1) (2023-11-19)

### Bug Fixes

* Fix Fails to build on x86 without SSE2 [pr#33](https://github.com/nervosnetwork/faster-hex/pull/33)
* Fix deserializing owned hex string [pr#35](https://github.com/nervosnetwork/faster-hex/pull/35)

## [0.8.0](https://github.com/nervosnetwork/faster-hex/compare/v0.7.0...v0.8.0) (2023-02-27)

### Features

* Add serde feature for faster-hex ([pr#28](https://github.com/nervosnetwork/faster-hex/pull/28))

## [0.7.0](https://github.com/nervosnetwork/faster-hex/compare/v0.6.1...v0.7.0) (2023-02-27)

### Features

* Allow faster-hex encode/decode to/from lower/uppercase  ([pr#26](https://github.com/nervosnetwork/faster-hex/pull/26))
### Bug Fixes
* Improve encode/decode length check ([pr#27](https://github.com/nervosnetwork/faster-hex/pull/27))

### Features

* Improve performance of fallback implementation ([pr#19](https://github.com/nervosnetwork/faster-hex/pull/19))

### Bug Fixes

* hex_string should not return Result ([0a5b5f4](https://github.com/nervosnetwork/faster-hex/commit/0a5b5f4e60ba149b30991e322f2e474c63813d21))

## [0.5.0](https://github.com/nervosnetwork/faster-hex/compare/v0.4.1...v0.5.0) (2021-01-13)

## [0.4.0](https://github.com/nervosnetwork/faster-hex/compare/v0.3.1...v0.4.0) (2019-09-10)

### Bug Fixes

* Do not expose hex_check_see on non-supported platform ([3e1cc75](https://github.com/nervosnetwork/faster-hex/commit/3e1cc75c1352e604709f32162ca55bdb64544779))

## [0.3.1](https://github.com/nervosnetwork/faster-hex/compare/v0.1.0...v0.3.1) (2019-03-12)

### Features

* check decode length ([857b0f7](https://github.com/nervosnetwork/faster-hex/commit/857b0f7511ce3b33a315768972b155385f823d1e))
* fuzz test ([b888363](https://github.com/nervosnetwork/faster-hex/commit/b888363adb3e3734bce2a8e2b3469191cdf20f5d))
* impl hex decode ([abb37fa](https://github.com/nervosnetwork/faster-hex/commit/abb37fa99e2346059218a32d62d25ac4d28f1d91))

## [0.1.0](https://github.com/nervosnetwork/faster-hex/compare/6c884911ba875ba3ac15f02fbba094cd9efef49a...v0.1.0) (2018-10-30)

### Features

* leverage simd to hex faster ([6c88491](https://github.com/nervosnetwork/faster-hex/commit/6c884911ba875ba3ac15f02fbba094cd9efef49a))
