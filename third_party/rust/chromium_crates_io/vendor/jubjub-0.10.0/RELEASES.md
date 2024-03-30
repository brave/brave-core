# 0.10.0
## Changed
- Bumped dependencies to `bls12_381 0.8`, `ff 0.13`, `group 0.13`.

# 0.9.0

## Changed
- Bumped MSRV to `1.56.0`
- Bumped dependencies to `bls12_381 0.7`, `ff 0.12`, `group 0.12`, `bitvec 1.0`.

# 0.8.0
## Added
- `jubjub::Base`, as an alias for `jubjub::Fq`.
- `jubjub::AffinePoint::batch_from_bytes`, which enables the inversion inside
  `jubjub::AffinePoint::from_bytes` to be batched.

## Changed
- Bumped dependencies to `bls12_381 0.6`, `ff 0.11`, `group 0.11`.

# 0.7.0
## Security
- A bug in the `jubjub::{AffinePoint, ExtendedPoint, SubgroupPoint}::from_bytes`
  APIs (and their `group::GroupEncoding` implementations) has been fixed. The
  APIs were documented as rejecting non-canonical points, but were accidentally
  accepting two specific non-canonical encodings. This could potentially cause a
  problem in consensus-critical protocols that expect encodings to be round-trip
  compatible (i.e. `AffinePoint::from_bytes(b).unwrap().to_bytes() == b`). See
  [ZIP 216](https://zips.z.cash/zip-0216) for more details.
  - A new API `jubjub::AffinePoint::from_bytes_pre_zip216_compatibility`
    preserves the previous behaviour, for use where consensus compatibility is
    required.

## Changed
- Bumped dependencies to `bitvec 0.22`, `bls12_381 0.5`, `ff 0.10`,
  `group 0.10`.
- MSRV is now 1.51.0.

# 0.6.0
## Changed
- Bumped dependencies to `bitvec 0.20`, `bls12_381 0.4`, `ff 0.9`, `group 0.9`,
  `rand_core 0.6`.
- MSRV is now 1.47.0.

# 0.5.1

# Fixed
* The crate now compiles for non-64-bit targets, such as the `wasm32-*` targets.

# 0.5.0

This upgrade bumps our dependencies `bls12_381`, `group` and `ff`, while making
corresponding changes to the APIs. This release now only supports Rust compilers
version 1.44.0 or later.

# 0.4.0

This release adds implementations of the `ff` and `group` traits. Additional trait
implementations (for standard traits) have been added where the `ff` and `group` trait
bounds require them.

## Added
* `jubjub::SubgroupPoint`, which represents an element of Jubjub's prime-order subgroup.
  It implements the following traits:
  * `group::{Group, GroupEncoding}`
  * `group::prime::PrimeGroup`
* New trait implementations for `jubjub::ExtendedPoint`:
  * `group::{Curve, Group, GroupEncoding, WnafGroup}`
  * `group::cofactor::{CofactorCurve, CofactorGroup}`
* New trait implementations for `jubjub::AffinePoint`:
  * `group::GroupEncoding`
  * `group::cofactor::CofactorCurveAffine`
* New trait implementations for `jubjub::Fr`:
  * `ff::{Field, PrimeField}`
* `jubjub::AffinePoint::is_identity`
* `jubjub::AffinePoint::to_extended`
* `jubjub::Scalar`, as an alias for `jubjub::Fr`.

## Changed
* We've migrated to `bls12_381 0.2`.
* `rand_core` is now a regular dependency.
* We depend on the `byteorder` crate again, as it is part of the `ff::PrimeField` trait.
* The benchmarks are now implemented using `criterion`.

# 0.3.0

This release now depends on the `bls12_381` crate, which exposes the `Fq` field type that we re-export.

* The `Fq` and `Fr` field types now have better constant function support for various operations and constructors.
* We no longer depend on the `byteorder` crate.
* We've bumped our `rand_core` dev-dependency up to 0.5.
* We've removed the `std` and `nightly` features.
* We've bumped our dependency of `subtle` up to `^2.2.1`.

# 0.2.0

This release switches to `subtle 2.1` to bring in the `CtOption` type, and also makes a few useful API changes.

* Implemented `Mul<Fr>` for `AffineNielsPoint` and `ExtendedNielsPoint`
* Changed `AffinePoint::to_niels()` to be a `const` function so that constant curve points can be constructed without statics.
* Implemented `multiply_bits` for `AffineNielsPoint`, `ExtendedNielsPoint`
* Removed `CtOption` and replaced it with `CtOption` from `subtle` crate.
* Modified receivers of some methods to reduce stack usage
* Changed various `into_bytes` methods into `to_bytes`

# 0.1.0

Initial release.
