# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to Rust's notion of
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.8.0] - 2024-03-25

### Added
- `orchard::note::Rho`
- `orchard::action::Action::rho`
- `orchard::note_encryption::CompactAction::rho`
- `orchard::note_encryption::OrchardDomain::for_compact_action`
- Additions under the `test-dependencies` feature flag:
  - `orchard::tree::MerkleHashOrchard::random`
  - `impl Distribution<MerkleHashOrchard> for Standard`

### Changed
- The following methods have their `Nullifier`-typed argument or return value
  now take or return `note::Rho` instead:
  - `orchard::note::RandomSeed::from_bytes`
  - `orchard::note::Note::from_parts`
  - `orchard::note::Note::rho`

### Removed
- `orchard::note_encryption::OrchardDomain::for_nullifier` (use `for_action`
  or `for_compact_action` instead).

## [0.7.1] - 2024-02-29
### Added
- `impl subtle::ConstantTimeEq for orchard::note::Nullifier`
- `orchard::note_encryption`:
  - `CompactAction::cmx`
  - `impl Clone for CompactAction`

## [0.7.0] - 2024-01-26
### Licensing
- The license for this crate is now "MIT OR Apache-2.0". The license
  exception that applied to the Zcash and Zebra projects, other projects
  designed to integrate with Zcash, and certain forks of Zcash, is no longer
  necessary. For clarity, this is intended to be a strict relaxation of the
  previous licensing, i.e. it permits all usage that was previously possible
  with or without use of the license exception.

### Added
- `orchard::builder`:
  - `bundle`
  - `BundleMetadata`
  - `BundleType`
  - `OutputInfo`
- `orchard::bundle::Flags::{ENABLED, SPENDS_DISABLED, OUTPUTS_DISABLED}`
- `orchard::tree::Anchor::empty_tree`

### Changed
- Migrated to the `zip32` crate. The following types have been replaced by the
  equivalent ones in that crate are now re-exported from there:
  - `orchard::keys::{DiversifierIndex, Scope}`
  - `orchard::zip32::ChildIndex`
- `orchard::builder`:
  - `Builder::new` now takes the bundle type to be used in bundle construction,
    instead of taking the flags and anchor separately.
  - `Builder::add_recipient` has been renamed to `add_output` in order to
    clarify than more than one output of a given transaction may be sent to the
    same recipient.
  - `Builder::build` now takes an additional `BundleType` argument that
    specifies how actions should be padded, instead of using hardcoded padding.
    It also now returns a `Result<Option<(Bundle<...>, BundleMetadata)>, ...>`
    instead of a  `Result<Bundle<...>, ...>`.
  - `BuildError` has additional variants:
    - `SpendsDisabled`
    - `OutputsDisabled`
    - `AnchorMismatch`
  - `SpendInfo::new` now returns a `Result<SpendInfo, SpendError>` instead of an
    `Option`.
- `orchard::keys::SpendingKey::from_zip32_seed` now takes a `zip32::AccountId`.

### Removed
- `orchard::bundle::Flags::from_parts`

## [0.6.0] - 2023-09-08
### Changed
- MSRV is now 1.65.0.
- Migrated to `incrementalmerkletree 0.5`.

## [0.5.0] - 2023-06-06
### Changed
- Migrated to `zcash_note_encryption 0.4`, `incrementalmerkletree 0.4`, `bridgetree 0.3`.
  `bridgetree` is now exclusively a test dependency.

## [0.4.0] - 2023-04-11
### Added
- `orchard::builder`:
  - `{SpendInfo::new, InputView, OutputView}`
  - `Builder::{spends, outputs}`
  - `SpendError`
  - `OutputError`
- `orchard::keys`:
  - `PreparedEphemeralPublicKey`
  - `PreparedIncomingViewingKey`
- impls of `memuse::DynamicUsage` for:
  - `orchard::note::Nullifier`
  - `orchard::note_encryption::OrchardDomain`
- impls of `Eq` for:
  - `orchard::zip32::ChildIndex`
  - `orchard::value::ValueSum`

### Changed
- MSRV is now 1.60.0.
- Migrated to `ff 0.13`, `group 0.13`, `pasta_curves 0.5`, `halo2_proofs 0.3`,
  `halo2_gadgets 0.3`, `reddsa 0.5`, `zcash_note_encryption 0.3`.
- `orchard::builder`:
  - `Builder::{add_spend, add_output}` now use concrete error types instead of
    `&'static str`s.
  - `Error` has been renamed to `BuildError` to differentiate from new error
    types.
  - `BuildError` now implements `std::error::Error` and `std::fmt::Display`.

### Fixed
- Several bugs have been fixed that were preventing Orchard bundles from being
  created or verified on 32-bit platforms, or with recent versions of Rust.

## [0.3.0] - 2022-10-19
### Added
- `orchard::Proof::add_to_batch`
- `orchard::address::Address::diversifier`
- `orchard::keys::Diversifier::from_bytes`
- `orchard::note`:
  - `RandomSeed`
  - `Note::{from_parts, rseed}`
- `orchard::circuit::Circuit::from_action_context`

### Changed
- Migrated to `zcash_note_encryption 0.2`.

## [0.2.0] - 2022-06-24
### Added
- `orchard::bundle::BatchValidator`
- `orchard::builder::Builder::value_balance`
- `orchard::note_encryption`:
  - `CompactAction::from_parts`
  - `CompactAction::nullifier`
  - `OrchardDomain::for_nullifier`
- Low-level APIs in `orchard::value` for handling `ValueCommitment`s.
  These are useful in code that constructs proof witnesses itself, but
  note that doing so requires a detailed knowledge of the Zcash protocol
  to avoid privacy and correctness pitfalls.
  - `ValueCommitTrapdoor`
  - `ValueCommitment::derive`

### Changed
- Migrated to `halo2_proofs 0.2`.

## [0.1.0] - 2022-05-10
### Changed
- Migrated to `bitvec 1`, `ff 0.12`, `group 0.12`, `incrementalmerkletree 0.3`,
  `pasta_curves 0.4`, `halo2_proofs 0.1`, `reddsa 0.3`.
- `orchard::bundle`:
  - `Action` has been moved to `orchard::Action`.
  - `Bundle::{try_}authorize` have been renamed to
    `Bundle::{try_}map_authorization`.
  - `Flags::from_byte` now returns `Option<Flags>` instead of
    `io::Result<Flags>`.
- `impl Sub for orchard::value::NoteValue` now returns `ValueSum` instead of
  `Option<ValueSum>`, as the result is guaranteed to be within the valid range
  of `ValueSum`.

## [0.1.0-beta.3] - 2022-04-06
### Added
- `orchard::keys`:
  - `Scope` enum, for distinguishing external and internal scopes for viewing
    keys and addresses.
  - `FullViewingKey::{to_ivk, to_ovk}`, which each take a `Scope` argument.
  - `FullViewingKey::scope_for_address`

### Changed
- Migrated to `halo2_proofs 0.1.0-beta.4`, `incrementalmerkletree 0.3.0-beta.2`.
- `orchard::builder`:
  - `Builder::add_spend` now requires that the `FullViewingKey` matches the
    given `Note`, and handles any scoping itself (instead of requiring the
    caller to pass the `FullViewingKey` for the correct scope).
- `orchard::keys`:
  - `FullViewingKey::{address, address_at}` now each take a `Scope` argument.

### Removed
- `orchard::keys`:
  - `FullViewingKey::derive_internal`
  - `impl From<&FullViewingKey> for IncomingViewingKey` (use
    `FullViewingKey::to_ivk` instead).
  - `impl From<&FullViewingKey> for OutgoingViewingKey` (use
    `FullViewingKey::to_ovk` instead).

## [0.1.0-beta.2] - 2022-03-22
### Added
- `orchard::keys`:
  - `DiversifierIndex::to_bytes`
  - `FullViewingKey::derive_internal`
  - `IncomingViewingKey::diversifier_index`
- `orchard::note`:
  - `impl PartialEq, Eq, PartialOrd, Ord for Nullifier`
- `orchard::primitives::redpallas::VerificationKey::verify`
- `orchard::tree`:
  - `MerklePath::from_parts`
  - `impl PartialEq, Eq, PartialOrd, Ord for MerkleHashOrchard`
- `impl From<orchard::bundle::BundleCommitment> for [u8; 32]`
- `Clone` impls for various structs:
  - `orchard::Bundle::{recover_outputs_with_ovks, recover_output_with_ovk}`
  - `orchard::builder`:
    - `InProgress, SigningMetadata, SigningParts, Unauthorized, Unproven`
  - `orchard::circuit::Circuit`
  - `orchard::keys::SpendAuthorizingKey`
  - `orchard::primitives::redpallas::SigningKey`

### Changed
- MSRV is now 1.56.1.
- Bumped dependencies to `pasta_curves 0.3`, `halo2_proofs 0.1.0-beta.3`.
- The following methods now have an additional `rng: impl RngCore` argument:
  - `orchard::builder::Bundle::create_proof`
  - `orchard::builder::InProgress::create_proof`
  - `orchard::circuit::Proof::create`
- `orchard::Bundle::commitment` now requires the bound `V: Copy + Into<i64>`
  instead of `i64: From<&'a V>`.
- `orchard::Bundle::binding_validating_key` now requires the bound
  `V: Into<i64>` instead of `V: Into<ValueSum>`.
- `orchard::builder::InProgressSignatures` and `orchard::bundle::Authorization`
  now have `Debug` bounds on themselves and their associated types.

### Removed
- `orchard::bundle`:
  - `commitments::hash_bundle_txid_data` (use `Bundle::commitment` instead).
  - `commitments::hash_bundle_auth_data` (use `Bundle::authorizing_commitment`
    instead).
- `orchard::keys`:
  - `FullViewingKey::default_address`
  - `IncomingViewingKey::default_address`
  - `DiversifierKey` (use the APIs on `FullViewingKey` and `IncomingViewingKey`
    instead).
- `impl std::hash::Hash for orchard::tree::MerkleHashOrchard` (use `BTreeMap`
  instead of `HashMap`).
- `orchard::value::ValueSum::from_raw`

## [0.1.0-beta.1] - 2021-12-17
Initial release!
