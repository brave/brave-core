# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to Rust's notion of
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.5.0] - 2026-06-02

### Added
- `halo2_gadgets::ecc`:
  - `CircuitVersion` (re-exported from `ecc::chip`), an enum with variants
    `AnchoredBase` and `InsecureUnanchoredBase` that selects which version of the
    variable-base scalar-multiplication subcircuit the ECC chip synthesizes. The
    two versions share an identical `ConstraintSystem`/`configure` and differ only
    in witness assignment, so a single binary can build and verify against both
    the historical and the fixed verifying keys.

### Changed
- `halo2_gadgets::ecc::chip`:
  - `EccChip::construct` now takes a `CircuitVersion` argument (there is no
    default). Use `CircuitVersion::AnchoredBase` for all proving and current
    verification, and `CircuitVersion::InsecureUnanchoredBase` only to rebuild the
    original verifying key in order to verify proofs produced by the original
    (pre-fix) circuit.

### Fixed
- A critical soundness bug in variable-base scalar multiplication
  (`halo2_gadgets::ecc::chip::mul`). The incomplete double-and-add loop kept the
  per-iteration base `(x_p, y_p)` constant across its rows via `q_mul_2`, but
  never tied it to the real base: the coordinates were written with
  `assign_advice`, and the constancy chain reached neither the doubling-row nor
  the complete-addition base anchors. A malicious prover could therefore run the
  incomplete loop against a free base `B' != base`, making the gadget output
  `[a] base + [b] B'` rather than `[scalar] base`. The base is now anchored into
  the first incomplete-addition row via `copy_advice`, and `q_mul_2` propagates
  the equality to every loop row. This fix changes the verifying key; see
  `CircuitVersion` above for how to verify proofs produced before the fix.

## [0.4.0] - 2025-12-04

### Added
- `halo2_gadgets::ecc`:
  - `Point::new_from_constant`
  - `Point::mul_sign`
- `halo2_gadgets::sinsemilla`:
  - `CommitDomain::blinding_factor`
  - `CommitDomain::hash_with_private_init`
  - `CommitDomain::q_init`
  - `HashDomain::hash_to_point_with_private_init`
  - `merkle::chip::MerkleConfig::cond_swap_config`
- `halo2_gadgets::utilities::cond_swap`:
  - `CondSwapChip::mux_on_points`
  - `CondSwapChip::mux_on_non_identity_points`
- `halo2_gadgets::utilities::lookup_range_check`:
  - `LookupRangeCheck` trait
  - `LookupRangeCheck4_5BConfig<F, K>`, for efficient 4-, 5-, and K-bit lookup
    range checks.
  - `impl LookupRangeCheck for LookupRangeCheckConfig`
  - `PallasLookupRangeCheck: LookupRangeCheck<pallas::Base, { sinsemilla::K }>`
    trait.
  - `PallasLookupRangeCheckConfig` type alias, for
    `LookupRangeCheckConfig<pallas::Base, { sinsemilla::K }>`.
  - `PallasLookupRangeCheck4_5BConfig` type alias, for
    `LookupRangeCheck4_5BConfig<pallas::Base, { sinsemilla::K }>`.

### Changed
- `halo2_gadgets::ecc`:
  - Changes to the `EccInstructions` trait:
    - Added `EccInstructions::witness_point_from_constant`.
    - Added `EccInstructions::mul_sign`.
  - `chip::EccConfig` now has a `Lookup: PallasLookupRangeCheck` generic
    parameter, which defaults to the previous type of the `lookup_config` field
    (`PallasLookupRangeCheckConfig`).
  - `chip::EccChip` now has a `Lookup: PallasLookupRangeCheck` generic
    parameter, which defaults to `PallasLookupRangeCheckConfig`.
- `halo2_gadgets::sinsemilla`:
  - Changes to the `SinsemillaInstructions` trait:
    - Added `SinsemillaInstructions::hash_to_point_with_private_init`.
  - `chip`:
    - `SinsemillaConfig` and `SinsemillaChip` now have a
      `Lookup: PallasLookupRangeCheck` generic parameter, which defaults to
      `PallasLookupRangeCheckConfig`.
    - `SinsemillaConfig::lookup_config` now returns `Lookup`.
    - `SinsemillaChip::configure` now takes `Lookup`, and has a new input
      `allow_init_from_private_point` to enable the evaluation of Sinsemilla
      hash from a private point.
  - `merkle::chip`:
    - `MerkleConfig` and `MerkleChip` now have a `Lookup: PallasLookupRangeCheck`
      generic parameter, which defaults to `PallasLookupRangeCheckConfig`.
- `halo2_gadgets::utilities`:
  - `RangeConstrained::witness_short` now takes a generic
    `Lookup: lookup_range_check::LookupRangeCheck<F, K>` instead of the concrete
    type `lookup_range_check::LookupRangeCheckConfig<F, K>`.
- `halo2_gadgets::utilities::cond_swap`:
  - Changes to the `CondSwapInstructions` trait:
    - Added `CondSwapInstructions::mux`.

## [0.3.1] - 2024-12-16
- `halo2_gadgets::poseidon::primitives` is now a re-export of the new `halo2_poseidon`
  crate.
- `halo2_gadgets::sinsemilla::primitives` is now a re-export of the new `sinsemilla`
  crate.

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
