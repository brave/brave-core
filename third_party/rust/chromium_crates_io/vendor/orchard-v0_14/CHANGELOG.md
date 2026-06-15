# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to Rust's notion of
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.14.0] - 2026-06-02

### Added
- `orchard::action::ActionFromPartsError`
- `orchard::Proof::expected_proof_size`, the canonical byte length of a proof
  for a given number of actions.
- `orchard::bundle::BundleError`
- `impl From<orchard::action::ActionFromPartsError> for orchard::pczt::TxExtractorError`
- `impl From<orchard::bundle::BundleError> for orchard::pczt::TxExtractorError`
- `orchard::bundle::ProofSizeEnforcement`
- `orchard::Bundle::<Authorized, V>::try_from_parts`, which constructs an
  authorized bundle while rejecting a proof whose length is not the canonical
  size for the bundle's number of actions (GHSA-2x4w-pxqw-58v9). This is now the
  only way to construct a `Bundle<Authorized, _>`, so an authorized bundle can
  no longer hold a proof padded with arbitrary data when proof size enforcement
  is strict.
- `orchard::Bundle::<EffectsOnly, V>::from_parts`
- `orchard::circuit::OrchardCircuitVersion`, an enum selecting the Action circuit
  version, with variants `InsecurePreNu6_2` and `FixedPostNu6_2`.
- `orchard::circuit::ProvingKey::build_for_version` and
  `orchard::circuit::VerifyingKey::build_for_version`, which build the key for a
  given `OrchardCircuitVersion`; `build()` continues to build the fixed circuit.
  `ProvingKey::build_for_version` can build the proving key for the pre-NU6.2
  (insecure) circuit.
- `orchard::circuit::ProvingKey::circuit_version`, the version the proving key
  produces proofs for. `Proof::create` now returns an error if a circuit's
  version does not match the proving key's.
- `orchard::circuit::Circuit::from_action_context_for_version`, like
  `from_action_context` but building the circuit for a chosen
  `OrchardCircuitVersion`.
- `orchard::builder::Builder::new_for_version` (requires the `circuit` feature),
  which constructs a builder that produces proofs for a given
  `OrchardCircuitVersion` (`Builder::new` uses `FixedPostNu6_2`).
- `orchard::builder::bundle_for_version` (requires the `circuit` feature), like
  `bundle` but building the Action circuits for a given `OrchardCircuitVersion`.
- `orchard::Bundle::<InProgress<Unproven, S>, V>::circuit_version` (requires the
  `circuit` feature), the `OrchardCircuitVersion` the bundle's actions were built
  for, so a caller can select a matching `ProvingKey` without tracking it
  separately.

### Changed
- Updated to `halo2_gadgets 0.5.0`
- `orchard::action::Action::from_parts` now returns
  `Result<Self, orchard::action::ActionFromPartsError>` instead of `Option<Self>`.
- `orchard::pczt::TxExtractorError` has added variants `InvalidEpk` and
  `NonCanonicalProofSize`. The Transaction Extractor role now rejects a PCZT
  whose `zkproof` is not the canonical size for its number of actions
  (GHSA-2x4w-pxqw-58v9).
- `unstable-voting-circuits`-only:
  - `orchard::constants::OrchardFixedBases` is now a unit struct rather than a
    3-variant enum. It is a trait carrier for the halo2_gadgets `FixedPoints`
    impl and was never constructed as a value; the concrete fixed bases live
    in `OrchardFixedBasesFull`, `OrchardBaseFieldBases`, and
    `OrchardShortScalarBases`, which are unchanged.

### Removed
- `orchard::Bundle::from_parts`. Construct a bundle through the
  authorization-specific constructor instead: `Bundle::<EffectsOnly, V>::from_parts`,
  or `Bundle::<Authorized, V>::try_from_parts` for an authorized bundle.
- `unstable-voting-circuits`-only:
  - The five dead `From<X> for OrchardFixedBases` conversions (from
    `OrchardFixedBasesFull`, `NullifierK`, `ValueCommitV`,
    `OrchardBaseFieldBases`, `OrchardShortScalarBases`). None were reachable;
    in-circuit dispatch goes through the per-slot enums.
  - `impl FixedPoint<pallas::Affine> for NullifierK` and
    `impl FixedPoint<pallas::Affine> for ValueCommitV`. After the 0.13.1
    enum refactor, dispatch routes through `OrchardBaseFieldBases::NullifierK`
    and `OrchardShortScalarBases::ValueCommitV`, leaving the standalone
    unit-struct impls dead.

### Fixed
- The update to `halo2_gadgets 0.5.0` fixes a critical vulnerability related to
  its use in the Orchard circuit. Please see the release notes for
  `halo2_gadgets 0.5.0` for additional details.
- An authorized `Bundle` or a PCZT can no longer carry a `zkproof` padded with
  arbitrary trailing data, and an `Action` can no longer be constructed with an
  `epk` that does not encode a non-identity Pallas point (GHSA-2x4w-pxqw-58v9).
  See the `Bundle::<Authorized, V>::try_from_parts`,
  `Proof::expected_proof_size`, `Action::from_parts`, and `TxExtractorError`
  entries under `Added` and `Changed` above for the API surface of these checks.

## [0.13.1] - 2026-04-27

### Added
- `orchard::{L_ORCHARD_BASE, L_ORCHARD_SCALAR, L_VALUE}`, the bit-length
  parameters of the Orchard base field, scalar field, and value encoding
  as defined in the Zcash protocol specification.
- `orchard::value::NoteValue::ZERO`, a `const NoteValue` equal to zero.
- The following modules and APIs are available behind the
  `unstable-voting-circuits` feature flag to support downstream
  voting-circuit development. These temporary APIs are not covered by the
  crate's semver stability guarantees and may change in any future release:
  - Modules: `orchard::{constants, spec}`,
    `orchard::circuit::{commit_ivk, commit_ivk::gadgets, note_commit,
    note_commit::gadgets, gadget::add_chip}`,
    `orchard::note::{commitment, nullifier}`.
  - Address and circuit helpers: `Address::{g_d, pk_d}`,
    `circuit::gadget::{AddInstruction, assign_free_advice, derive_nullifier,
    commit_ivk, note_commit}`,
    `circuit::gadget::add_chip::{AddConfig, AddChip}` and
    `AddChip::{configure, construct}`,
    `CommitIvkChip::{configure, construct}`,
    `NoteCommitChip::{configure, construct}`.
  - Fixed bases: `orchard::constants::OrchardFixedBases` has three
    variants: `Full(OrchardFixedBasesFull)` for full-width scalar
    multiplication, `Base(OrchardBaseFieldBases)` for base-field
    scalars, and `Short(OrchardShortScalarBases)` for short signed
    scalars. `OrchardBaseFieldBases` covers `NullifierK` and
    `SpendAuthGBase`; `OrchardShortScalarBases` covers `ValueCommitV`
    and `SpendAuthGShort`. `From<NullifierK>`, `From<ValueCommitV>`,
    `From<OrchardFixedBasesFull>`, `From<OrchardBaseFieldBases>`, and
    `From<OrchardShortScalarBases>` conversions to `OrchardFixedBases`
    are provided.
  - Key, note, tree, and value APIs: `SpendingKey::random`,
    `SpendAuthorizingKey::derive_inner`, `NullifierDerivingKey` and
    `CommitIvkRandomness` and their `inner` methods,
    `FullViewingKey::{nk, rivk}`,
    `DiversifiedTransmissionKey::{inner, to_bytes}`,
    `orchard::note::NoteCommitTrapdoor` and `NoteCommitTrapdoor::inner`,
    `Rho::{from_nf_old, into_inner}`, `RandomSeed::{psi, rcm}`,
    `Note::{new, dummy}`, `NoteCommitment::inner`,
    `ExtractedNoteCommitment::inner`, `Nullifier::{from_inner, inner}`,
    `NonIdentityPallasPoint` and `NonIdentityPallasPoint::from_bytes`,
    `MerklePath::dummy`, and `MerkleHashOrchard::inner`.

## [0.13.0] - 2026-04-22

### Added
- `orchard::primitives::redpallas::VerificationKey<T>::is_identity`, which
  returns `true` if the verification key is the identity `pallas::Point`.
- `orchard::primitives::redpallas::testing::arb_valid_spendauth_keypair`
  (under the `test-dependencies` feature): a uniformly-distributed valid
  `(rsk, rk)` key pair with non-identity `rk`.

### Changed
- MSRV is now 1.85.1
- Migrated from yanked `core2` library to `corez`
- `orchard::pczt::Bundle::extract` now takes its `self` argument by
  reference instead of by value.
- `orchard::zip32::Error` has added variant `MaxDerivationDepth`
- `orchard::Action::from_parts` and `orchard::circuit::Instance::from_parts`
  now return `Option<Self>`, yielding `None` when `rk` is the identity
  `pallas::Point`. Callers that previously treated the return as `Self`
  must now handle the `None` case. This aligns the crate with the
  consensus rule introduced in zcashd v6.12.1 and Zebra 4.3.1 (see
  <https://zodl.com/zcashd-zebra-april-2026-disclosure/> and
  <https://zfnd.org/zebra-4-3-1-critical-security-fixes-dockerized-mining-and-ci-hardening/>);
  the Zcash protocol specification will be updated to match.
- `orchard::pczt::TxExtractorError` has added variant `IdentityRk`.
- `orchard::pczt::ProverError` has added variant `IdentityRk`.

## [0.12.0] - 2025-12-05

### Added
- `orchard::pczt::Action::apply_signature`
- `orchard::value::BalanceError`
- `impl std::error::Error` for the following errors:
  - `orchard::pczt`:
    - `IoFinalizerError`
    - `ParseError`
    - `ProverError`
    - `SignerError`
    - `TxExtractorError`
    - `UpdaterError`
    - `VerifyError`
  - `orchard::zip32::Error`

### Changed
- `orchard::builder::BuildError::ValueSum` variant now contains
  `orchard::value::BalanceError`.
- `orchard::pczt::SignerError` has added variants:
  - `InvalidExternalSignature`
- All error enums in this crate are now `#[non_exhaustive]`, to allow future
  error variants to be added without a SemVer break:
  - `orchard::builder`:
    - `BuildError`
    - `SpendError`
  - `orchard::pczt`:
    - `IoFinalizerError`
    - `ParseError`
    - `ProverError`
    - `SignerError`
    - `TxExtractorError`
    - `UpdaterError`
    - `VerifyError`
  - `orchard::zip32::Error`
- `orchard::builder::OutputError` has been changed from a zero-sized struct to
  a `#[non_exhaustive]` enum with (for now) a single variant.

### Removed
- `orchard::value::OverflowError` (use `BalanceError` instead).

## [0.10.2] - 2025-05-08

### Fixed
- Fixes problems in test compilation under `--no-default-features`

## [0.11.0] - 2025-02-20

### Added
- `orchard::pczt::Zip32Derivation::extract_account_index`

### Changed
- MSRV is now 1.70
- Migrated to `nonempty 0.11`, `incrementalmerkletree 0.8`, `shardtree 0.6`, 
  `zcash_spec 0.2`, `zip32 0.2`
- `orchard::builder::Builder::add_output` now takes a `[u8; 512]` for its
  `memo` argument instead of an optional value.

## [0.10.1] - 2024-12-16

### Added
- Support for Partially-Created Zcash Transactions:
  - `orchard::builder::Builder::build_for_pczt`
  - `orchard::note_encryption`:
    - `OrchardDomain::for_pczt_action`
    - `impl ShieldedOutput<OrchardDomain, ENC_CIPHERTEXT_SIZE> for orchard::pczt::Action`
  - `orchard::pczt` module.
- `orchard::bundle::EffectsOnly`
- `orchard::tree::MerklePath::{position, auth_path}`
- `orchard::value`:
  - `Sign`
  - `ValueSum::magnitude_sign`
  - `ValueCommitTrapdoor::to_bytes`
- `impl Clone for orchard::tree::MerklePath`

## [0.10.0] - 2024-10-02

### Changed
- Migrated to `incrementalmerkletree 0.7`.

## [0.9.1] - 2024-08-13

### Changed
- Migrated to `visibility 0.1.1`.

## [0.9.0] - 2024-08-12

### Added
- `orchard::keys::SpendValidatingKey::{from_bytes, to_bytes}` behind the
  `unstable-frost` feature flag. These are temporary APIs exposed for development
  purposes, and will be replaced by type-safe FROST APIs once ZIP 312 key
  generation is specified (https://github.com/zcash/zips/pull/883).

### Changed
- Migrated to `incrementalmerkletree 0.6`.

## [0.8.0] - 2024-03-25

### Added
- `orchard::keys::IncomingViewingKey::prepare`
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
