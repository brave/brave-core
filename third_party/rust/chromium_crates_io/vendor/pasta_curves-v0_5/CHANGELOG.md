# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to Rust's notion of
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
## [0.5.1] - 2023-03-02
### Fixed
- Fix a bug on 32-bit platforms that could cause the square root implementation
  to return an incorrect result.
- The `sqrt-table` feature now works without `std` and only requires `alloc`.

## [0.5.0] - 2022-12-06
### Added
- `serde` feature flag, which enables Serde compatibility to the crate types.
  Field elements and points are serialized to their canonical byte encoding
  (encoded as hexadecimal if the data format is human readable).

### Changed
- Migrated to `ff 0.13`, `group 0.13`, `ec-gpu 0.2`.
- `pasta_curves::arithmetic`:
  - `FieldExt` bounds on associated types of `CurveExt` and `CurveAffine` have
    been replaced by bounds on `ff::WithSmallOrderMulGroup<3>` (and `Ord` in the
    case of `CurveExt`).
- `pasta_curves::hashtocurve`:
  - `FieldExt` bounds on the module functions have been replaced by equivalent
    `ff` trait bounds.

### Removed
- `pasta_curves::arithmetic`:
  - `FieldExt` (use `ff::PrimeField` or `ff::WithSmallOrderMulGroup` instead).
  - `Group`
  - `SqrtRatio` (use `ff::Field::{sqrt_ratio, sqrt_alt}` instead).
  - `SqrtTables` (from public API, as it isn't suitable for generic usage).

## [0.4.1] - 2022-10-13
### Added
- `uninline-portable` feature flag, which disables inlining of some functions.
  This is useful for tiny microchips (such as ARM Cortex-M0), where inlining
  can hurt performance and blow up binary size.

## [0.4.0] - 2022-05-05
### Changed
- MSRV is now 1.56.0.
- Migrated to `ff 0.12`, `group 0.12`.

## [0.3.1] - 2022-04-20
### Added
- `gpu` feature flag, which exposes implementations of the `GpuField` trait from
  the `ec-gpu` crate for `pasta_curves::{Fp, Fq}`. This flag will eventually
  control all GPU functionality.
- `repr-c` feature flag, which helps to facilitate usage of this crate's types
  across FFI by conditionally adding `repr(C)` attribute to point structures.
- `pasta_curves::arithmetic::Coordinates::from_xy`

### Changed
- `pasta_curves::{Fp, Fq}` are now declared as `repr(transparent)`, to enable
  their use across FFI. They remain opaque structs in Rust code.

## [0.3.0] - 2022-01-03
### Added
- Support for `no-std` builds, via two new (default-enabled) feature flags:
  - `alloc` enables the `pasta_curves::arithmetic::{CurveAffine, CurveExt}`
    traits, as well as implementations of traits like `group::WnafGroup`.
  - `sqrt-table` depends on `alloc`, and enables the large precomputed tables
    (stored on the heap) that speed up square root computation.
- `pasta_curves::arithmetic::SqrtRatio` trait, extending `ff::PrimeField` with
  square roots of ratios. This trait is likely to be moved into the `ff` crate
  in a future release (once we're satisfied with it).

### Removed
- `pasta_curves::arithmetic`:
  - `Field` re-export (`pasta_curves::group::ff::Field` is equivalent).
  - `FieldExt::ROOT_OF_UNITY` (use `ff::PrimeField::root_of_unity` instead).
  - `FieldExt::{T_MINUS1_OVER2, pow_by_t_minus1_over2, get_lower_32, sqrt_alt,`
    `sqrt_ratio}` (moved to `SqrtRatio` trait).
  - `FieldExt::{RESCUE_ALPHA, RESCUE_INVALPHA}`
  - `FieldExt::from_u64` (use `From<u64> for ff::PrimeField` instead).
  - `FieldExt::{from_bytes, read, to_bytes, write}`
    (use `ff::PrimeField::{from_repr, to_repr}` instead).
  - `FieldExt::rand` (use `ff::Field::random` instead).
  - `CurveAffine::{read, write}`
    (use `group::GroupEncoding::{from_bytes, to_bytes}` instead).

## [0.2.1] - 2021-09-17
### Changed
- The crate is now licensed as `MIT OR Apache-2.0`.

## [0.2.0] - 2021-09-02
### Changed
- Migrated to `ff 0.11`, `group 0.11`.

## [0.1.2] - 2021-08-06
### Added
- Implementation of `group::WnafGroup` for Pallas and Vesta, enabling them to be
  used with `group::Wnaf` for targeted performance improvements.

## [0.1.1] - 2021-06-04
### Added
- Implementations of `group::cofactor::{CofactorCurve, CofactorCurveAffine}` for
  Pallas and Vesta, enabling them to be used in cofactor-aware protocols that
  also want to leverage the affine point representation.

## [0.1.0] - 2021-06-01
Initial release!
