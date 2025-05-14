# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to Rust's notion of
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.3.0] - 2023-03-21
### Added
- `halo2_gadgets::poseidon::primitives::{Mds, generate_constants}`

### Changed
- Migrated to `ff 0.13`, `group 0.13`, `pasta_curves 0.5` and `halo2_proofs 0.3`.
- APIs with `F: pasta_curves::arithmetic::FieldExt` bounds have been changed to
  use `ff` traits directly.

## [0.2.0] - 2022-06-23
### Added
- `halo2_gadgets::utilities::RangeConstrained<F, Value<F>>::bitrange_of`

### Changed
All APIs that represented witnessed values as `Option<V>` now represent them as
`halo2_proofs::circuit::Value<V>`. The core API changes are listed below.

- Migrated to `halo2_proofs 0.2.0`.
- The following APIs now take `Value<_>` instead of `Option<_>`:
  - `halo2_gadgets::ecc`:
    - `EccInstructions::{witness_point, witness_point_non_id}`
    - `EccInstructions::{witness_scalar_var, witness_scalar_fixed}`
    - `ScalarVar::new`
    - `ScalarFixed::new`
    - `NonIdentityPoint::new`
    - `Point::new`
  - `halo2_gadgets::sinsemilla`:
    - `SinsemillaInstructions::witness_message_piece`
    - `MessagePiece::{from_field_elem, from_subpieces}`
  - `halo2_gadgets::sinsemilla::merkle`:
    - `MerklePath::construct`
  - `halo2_gadgets::utilities`:
    - `UtilitiesInstructions::load_private`
    - `RangeConstrained::witness_short`
  - `halo2_gadgets::utilities::cond_swap`:
    - `CondSwapInstructions::swap`
  - `halo2_gadgets::utilities::decompose_running_sum`:
    - `RunningSumConfig::witness_decompose`
  - `halo2_gadgets::utilities::lookup_range_check`:
    - `LookupRangeCheckConfig::{witness_check, witness_short_check}`
- The following APIs now return `Value<_>` instead of `Option<_>`:
  - `halo2_gadgets::ecc::chip`:
    - `EccPoint::{point, is_identity}`
    - `NonIdentityEccPoint::point`
  - `halo2_gadgets::utilities`:
    - `FieldValue::value`
    - `Var::value`
    - `RangeConstrained::value`
- `halo2_gadgets::sha256::BlockWord` is now a newtype wrapper around
  `Value<u32>` instead of `Option<u32>`.

### Removed
- `halo2_gadgets::utilities::RangeConstrained<F, Option<F>>::bitrange_of`

## [0.1.0] - 2022-05-10
### Added
- `halo2_gadgets::utilities`:
  - `FieldValue` trait.
  - `RangeConstrained` newtype wrapper.
- `halo2_gadgets::ecc`:
  - `EccInstructions::witness_scalar_var` API to witness a full-width scalar
    used in variable-base scalar multiplication.
  - `EccInstructions::witness_scalar_fixed`, to witness a full-width scalar
    used in fixed-base scalar multiplication.
  - `EccInstructions::scalar_fixed_from_signed_short`, to construct a signed
    short scalar used in fixed-base scalar multiplication from its magnitude and
    sign.
  - `BaseFitsInScalarInstructions` trait that can be implemented for a curve
    whose base field fits into its scalar field. This provides a method
    `scalar_var_from_base` that converts a base field element that exists as
    a variable in the circuit, into a scalar to be used in variable-base
    scalar multiplication.
  - `ScalarFixed::new`
  - `ScalarFixedShort::new`
  - `ScalarVar::new` and `ScalarVar::from_base` gadget APIs.
- `halo2_gadgets::ecc::chip`:
  - `ScalarVar` enum with `BaseFieldElem` and `FullWidth` variants. `FullWidth`
    is unimplemented for `halo2_gadgets v0.1.0`.
- `halo2_gadgets::poseidon`:
  - `primitives` (moved from `halo2_gadgets::primitives::poseidon`)
- `halo2_gadgets::sinsemilla`:
  - `primitives` (moved from `halo2_gadgets::primitives::sinsemilla`)
  - `MessagePiece::from_subpieces`

### Changed
- `halo2_gadgets::ecc`:
  - `EccInstructions::ScalarVar` is now treated as a full-width scalar, instead
    of being restricted to a base field element.
  - `EccInstructions::mul` now takes a `Self::ScalarVar` as argument, instead
    of assuming that the scalar fits in a base field element `Self::Var`.
  - `EccInstructions::mul_fixed` now takes a `Self::ScalarFixed` as argument,
    instead of requiring that the chip always witness a new scalar.
  - `EccInstructions::mul_fixed_short` now takes a `Self::ScalarFixedShort` as
    argument, instead of the magnitude and sign directly.
  - `FixedPoint::mul` now takes `ScalarFixed` instead of `Option<C::Scalar>`.
  - `FixedPointShort::mul` now takes `ScalarFixedShort` instead of
    `(EccChip::Var, EccChip::Var)`.
- `halo2_gadgets::ecc::chip`:
  - `FixedPoint::u` now returns `Vec<[<C::Base as PrimeField>::Repr; H]>`
    instead of `Vec<[[u8; 32]; H]>`.
  - `ScalarKind` has been renamed to `FixedScalarKind`.
- `halo2_gadgets::sinsemilla`:
  - `CommitDomain::{commit, short_commit}` now take the trapdoor `r` as an
    `ecc::ScalarFixed<C, EccChip>` instead of `Option<C::Scalar>`.
  - `merkle::MerklePath` can now be constructed with more or fewer than two
    `MerkleChip`s.

### Removed
- `halo2_gadgets::primitives` (use `halo2_gadgets::poseidon::primitives` or
  `halo2_gadgets::sinsemilla::primitives` instead).

## [0.1.0-beta.3] - 2022-04-06
### Changed
- Migrated to `halo2_proofs 0.1.0-beta.4`.

## [0.1.0-beta.2] - 2022-03-22
### Changed
- Migrated to `halo2_proofs 0.1.0-beta.3`.

## [0.1.0-beta.1] - 2022-02-14
Initial release!
