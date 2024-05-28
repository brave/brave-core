# Changelog
All notable changes to this library will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this library adheres to Rust's notion of
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.15.1] - 2024-05-23

- Fixed `sapling-crypto` dependency to not enable its `multicore` feature flag
  when the default features of `zcash_primitives` are disabled.

## [0.15.0] - 2024-03-25

### Added
- `zcash_primitives::transaction::components::sapling::zip212_enforcement`

### Changed
- The following modules are now re-exported from the `zcash_protocol` crate.
  Additional changes have also been made therein; refer to the `zcash_protocol`
  changelog for details.
  - `zcash_primitives::consensus` re-exports `zcash_protocol::consensus`.
  - `zcash_primitives::constants` re-exports `zcash_protocol::constants`.
  - `zcash_primitives::transaction::components::amount` re-exports
    `zcash_protocol::value`. Many of the conversions to and from the
    `Amount` and `NonNegativeAmount` value types now return
    `Result<_, BalanceError>` instead of `Result<_, ()>`.
  - `zcash_primitives::memo` re-exports `zcash_protocol::memo`.
  - Update to `orchard` version `0.8.0`

### Removed
- `zcash_primitives::consensus::sapling_zip212_enforcement` instead use
  `zcash_primitives::transaction::components::sapling::zip212_enforcement`.
- From `zcash_primitive::components::transaction`:
  - `impl From<Amount> for u64`
  - `impl TryFrom<sapling::value::NoteValue> for NonNegativeAmount`
  - `impl From<NonNegativeAmount> for sapling::value::NoteValue`
  - `impl TryFrom<orchard::ValueSum> for Amount`
  - `impl From<NonNegativeAmount> for orchard::NoteValue`
- The `local_consensus` module and feature flag have been removed; use the module
  from the `zcash_protocol` crate instead.
- `unstable-nu6` and `zfuture` feature flags (use `--cfg zcash_unstable=\"nu6\"`
  or `--cfg zcash_unstable=\"zfuture\"` in `RUSTFLAGS` and `RUSTDOCFLAGS`
  instead).

## [0.14.0] - 2024-03-01
### Added
- Dependency on `bellman 0.14`, `sapling-crypto 0.1`.
- `zcash_primitives::consensus::sapling_zip212_enforcement`
- `zcash_primitives::legacy::keys`:
  - `AccountPrivKey::derive_secret_key`
  - `NonHardenedChildIndex`
  - `TransparentKeyScope`
- `zcash_primitives::local_consensus` module, behind the `local-consensus`
  feature flag.
  - The `LocalNetwork` struct provides a type for specifying network upgrade
    activation heights for a local or specific configuration of a full node.
    Developers can make use of this type when connecting to a Regtest node by
    replicating the activation heights used on their node configuration.
  - `impl zcash_primitives::consensus::Parameters for LocalNetwork` uses the
    provided activation heights, and `zcash_primitives::constants::regtest::`
    for everything else.
- `zcash_primitives::transaction`:
  - `builder::{BuildConfig, FeeError, get_fee, BuildResult}`
  - `builder::Error::SaplingBuilderNotAvailable`
  - `components::sapling`:
    - Sapling bundle component parsers, behind the `temporary-zcashd` feature
      flag:
      - `temporary_zcashd_read_spend_v4`
      - `temporary_zcashd_read_output_v4`
      - `temporary_zcashd_write_output_v4`
      - `temporary_zcashd_read_v4_components`
      - `temporary_zcashd_write_v4_components`
  - `components::transparent`:
    - `builder::TransparentInputInfo`
  - `fees::StandardFeeRule`
  - Constants in `fees::zip317`:
    - `MARGINAL_FEE`
    - `GRACE_ACTIONS`
    - `P2PKH_STANDARD_INPUT_SIZE`
    - `P2PKH_STANDARD_OUTPUT_SIZE`
  - `impl From<TxId> for [u8; 32]`
- `zcash_primitives::zip32`:
  - `ChildIndex::hardened`
  - `ChildIndex::index`
  - `ChainCode::new`
  - `ChainCode::as_bytes`
  - `impl From<AccountId> for ChildIndex`
- Additions related to `zcash_primitive::components::amount::Amount`
  and `zcash_primitive::components::amount::NonNegativeAmount`:
  - `impl TryFrom<Amount> for u64`
  - `Amount::const_from_u64`
  - `NonNegativeAmount::const_from_u64`
  - `NonNegativeAmount::from_nonnegative_i64_le_bytes`
  - `NonNegativeAmount::to_i64_le_bytes`
  - `NonNegativeAmount::is_zero`
  - `NonNegativeAmount::is_positive`
  - `impl From<&NonNegativeAmount> for Amount`
  - `impl From<NonNegativeAmount> for u64`
  - `impl From<NonNegativeAmount> for zcash_primitives::sapling::value::NoteValue`
  - `impl From<NonNegativeAmount> for orchard::::NoteValue`
  - `impl Sum<NonNegativeAmount> for Option<NonNegativeAmount>`
  - `impl<'a> Sum<&'a NonNegativeAmount> for Option<NonNegativeAmount>`
  - `impl TryFrom<sapling::value::NoteValue> for NonNegativeAmount`
  - `impl TryFrom<orchard::NoteValue> for NonNegativeAmount`
- `impl {Clone, PartialEq, Eq} for zcash_primitives::memo::Error`

### Changed
- Migrated to `orchard 0.7`.
- `zcash_primitives::legacy`:
  - `TransparentAddress` variants have changed:
    - `TransparentAddress::PublicKey` has been renamed to `PublicKeyHash`
    - `TransparentAddress::Script` has been renamed to `ScriptHash`
  - `keys::{derive_external_secret_key, derive_internal_secret_key}` arguments
    changed from `u32` to `NonHardenedChildIndex`.
- `zcash_primitives::transaction`:
  - `builder`:
    - `Builder` now has a generic parameter for the type of progress notifier,
      which needs to implement `sapling::builder::ProverProgress` in order to
      build transactions.
    - `Builder::new` now takes a `BuildConfig` argument instead of an optional
      Orchard anchor. Anchors for both Sapling and Orchard are now required at
      the time of builder construction.
    - `Builder::{build, build_zfuture}` now take
      `&impl SpendProver, &impl OutputProver` instead of `&impl TxProver`.
    - `Builder::add_sapling_spend` no longer takes a `diversifier` argument as
      the diversifier may be obtained from the note.
    - `Builder::add_sapling_spend` now takes its `ExtendedSpendingKey` argument
      by reference.
    - `Builder::{add_sapling_spend, add_sapling_output}` now return `Error`s
      instead of the underlying `sapling_crypto::builder::Error`s when returning
      `Err`.
    - `Builder::add_orchard_spend` now takes its `SpendingKey` argument by
      reference.
    - `Builder::with_progress_notifier` now consumes `self` and returns a
      `Builder` typed on the provided channel.
    - `Builder::get_fee` now returns a `builder::FeeError` instead of the bare
      `FeeRule::Error` when returning `Err`.
    - `Builder::build` now returns a `Result<BuildResult, ...>` instead of
      using a tuple to return the constructed transaction and build metadata.
    - `Error::OrchardAnchorNotAvailable` has been renamed to
      `OrchardBuilderNotAvailable`.
    - `build` and `build_zfuture` each now take an additional `rng` argument.
  - `components`:
    - `transparent::TxOut.value` now has type `NonNegativeAmount` instead of
      `Amount`.
    - `sapling::MapAuth` trait methods now take `&mut self` instead of `&self`.
    - `transparent::fees` has been moved to
      `zcash_primitives::transaction::fees::transparent`
    - `transparent::builder::TransparentBuilder::{inputs, outputs}` have changed
      to return `&[TransparentInputInfo]` and `&[TxOut]` respectively, in order
      to avoid coupling to the fee traits.
  - `Unauthorized::SaplingAuth` now has type `InProgress<Proven, Unsigned>`.
  - `fees::FeeRule::fee_required` now takes an additional `orchard_action_count`
    argument.
  - The following methods now take `NonNegativeAmount` instead of `Amount`:
    - `builder::Builder::{add_sapling_output, add_transparent_output}`
    - `components::transparent::builder::TransparentBuilder::add_output`
    - `fees::fixed::FeeRule::non_standard`
    - `fees::zip317::FeeRule::non_standard`
  - The following methods now return `NonNegativeAmount` instead of `Amount`:
    - `components::amount::testing::arb_nonnegative_amount`
    - `fees::transparent`:
      - `InputView::value`
      - `OutputView::value`
    - `fees::FeeRule::{fee_required, fee_required_zfuture}`
    - `fees::fixed::FeeRule::fixed_fee`
    - `fees::zip317::FeeRule::marginal_fee`
    - `sighash::TransparentAuthorizingContext::input_amounts`
- `zcash_primitives::zip32`:
  - `ChildIndex` has been changed from an enum to an opaque struct, and no
    longer supports non-hardened indices.

### Removed
- `zcash_primitives::constants`:
  - All `const` values (moved to `sapling_crypto::constants`).
- `zcash_primitives::keys` module, as it was empty after the removal of:
  - `PRF_EXPAND_PERSONALIZATION`
  - `OutgoingViewingKey` (use `sapling_crypto::keys::OutgoingViewingKey`
    instead).
  - `prf_expand, prf_expand_vec` (use `zcash_spec::PrfExpand` instead).
- `zcash_primitives::sapling` module (use the `sapling-crypto` crate instead).
- `zcash_primitives::transaction::components::sapling`:
  - The following types were removed from this module (moved into
    `sapling_crypto::bundle`):
    - `Bundle`
    - `SpendDescription, SpendDescriptionV5`
    - `OutputDescription, OutputDescriptionV5`
    - `Authorization, Authorized`
    - `GrothProofBytes`
  - `CompactOutputDescription` (moved to `sapling_crypto::note_encryption`).
  - `Unproven`
  - `builder` (moved to `sapling_crypto::builder`).
  - `builder::Unauthorized` (use `builder::InProgress` instead).
  - `testing::{arb_bundle, arb_output_description}` (moved into
    `sapling_crypto::bundle::testing`).
  - `SpendDescription::<Unauthorized>::apply_signature`
  - `Bundle::<Unauthorized>::apply_signatures` (use
    `Bundle::<InProgress<Proven, Unsigned>>::apply_signatures` instead).
  - The `fees` module was removed. Its contents were unused in this crate,
    are now instead made available by `zcash_client_backend::fees::sapling`.
- `impl From<zcash_primitive::components::transaction::Amount> for u64`
- `zcash_primitives::zip32`:
  - `sapling` module (moved to `sapling_crypto::zip32`).
  - `ChildIndex::Hardened` (use `ChildIndex::hardened` instead).
  - `ChildIndex::NonHardened`
  - `sapling::ExtendedFullViewingKey::derive_child`

### Fixed
- `zcash_primitives::keys::ExpandedSpendingKey::from_spending_key` now panics if the
  spending key expands to `ask = 0`. This has a negligible probability of occurring.
- `zcash_primitives::zip32::ExtendedSpendingKey::derive_child` now panics if the
  child key has `ask = 0`. This has a negligible probability of occurring.

## [0.13.0] - 2023-09-25
### Added
- `zcash_primitives::consensus::BlockHeight::saturating_sub`
- `zcash_primitives::transaction::builder`:
  - `Builder::add_orchard_spend`
  - `Builder::add_orchard_output`
- `zcash_primitives::transaction::components::orchard::builder` module
- `impl HashSer for String` is provided under the `test-dependencies` feature
  flag. This is a test-only impl; the identity leaf value is `_` and the combining
  operation is concatenation.
- `zcash_primitives::transaction::components::amount::NonNegativeAmount::ZERO`
- Additional trait implementations for `NonNegativeAmount`:
  - `TryFrom<Amount> for NonNegativeAmount`
  - `Add<NonNegativeAmount> for NonNegativeAmount`
  - `Add<NonNegativeAmount> for Option<NonNegativeAmount>`
  - `Sub<NonNegativeAmount> for NonNegativeAmount`
  - `Sub<NonNegativeAmount> for Option<NonNegativeAmount>`
  - `Mul<usize> for NonNegativeAmount`
- `zcash_primitives::block::BlockHash::try_from_slice`

### Changed
- Migrated to `incrementalmerkletree 0.5`, `orchard 0.6`.
- `zcash_primitives::transaction`:
  - `builder::Builder::{new, new_with_rng}` now take an optional `orchard_anchor`
    argument which must be provided in order to enable Orchard spends and recipients.
  - All `builder::Builder` methods now require the bound `R: CryptoRng` on
    `Builder<'a, P, R>`. A non-`CryptoRng` randomness source is still accepted
    by `builder::Builder::test_only_new_with_rng`, which **MUST NOT** be used in
    production.
  - `builder::Error` has several additional variants for Orchard-related errors.
  - `fees::FeeRule::fee_required` now takes an additional argument,
    `orchard_action_count`
  - `Unauthorized`'s associated type `OrchardAuth` is now
    `orchard::builder::InProgress<orchard::builder::Unproven, orchard::builder::Unauthorized>`
    instead of `zcash_primitives::transaction::components::orchard::Unauthorized`
- `zcash_primitives::consensus::NetworkUpgrade` now implements `PartialEq`, `Eq`
- `zcash_primitives::legacy::Script` now has a custom `Debug` implementation that
  renders script details in a much more legible fashion.
- `zcash_primitives::sapling::redjubjub::Signature` now has a custom `Debug`
  implementation that renders details in a much more legible fashion.
- `zcash_primitives::sapling::tree::Node` now has a custom `Debug`
  implementation that renders details in a much more legible fashion.

### Removed
- `impl {PartialEq, Eq} for transaction::builder::Error`
  (use `assert_matches!` where error comparisons are required)
- `zcash_primitives::transaction::components::orchard::Unauthorized`
- `zcash_primitives::transaction::components::amount::DEFAULT_FEE` was
  deprecated in 0.12.0 and has now been removed.

## [0.12.0] - 2023-06-06
### Added
- `zcash_primitives::transaction`:
  - `Transaction::temporary_zcashd_read_v5_sapling`
  - `Transaction::temporary_zcashd_write_v5_sapling`
- Implementations of `memuse::DynamicUsage` for the following types:
  - `zcash_primitives::transaction::components::sapling`:
    - `Bundle<Authorized>`
    - `SpendDescription<Authorized>`

### Changed
- MSRV is now 1.65.0.
- Bumped dependencies to `secp256k1 0.26`, `hdwallet 0.4`, `incrementalmerkletree 0.4`
  `zcash_note_encryption 0.4`, `orchard 0.5`

### Removed
- `merkle_tree::Hashable` has been removed and its uses have been replaced by
  `incrementalmerkletree::Hashable` and `merkle_tree::HashSer`.
- The `Hashable` bound on the `Node` parameter to the `IncrementalWitness`
  type has been removed.
- `sapling::SAPLING_COMMITMENT_TREE_DEPTH_U8` and `sapling::SAPLING_COMMITMENT_TREE_DEPTH`
  have been removed; use `sapling::NOTE_COMMITMENT_TREE_DEPTH` instead.
- `merkle_tree::{CommitmentTree, IncrementalWitness, MerklePath}` have been removed in
  favor of versions of these types that are now provided by the
  `incrementalmerkletree` crate. The replacement types now use const generic
  parameters for enforcing the note commitment tree depth. Serialization
  methods for these types that do not exist for the `incrementalmerkletree`
  replacement types have been replaced by new methods in the `merkle_tree` module.
- `merkle_tree::incremental::write_auth_fragment_v1` has been removed without replacement.
- The `merkle_tree::incremental` module has been removed; its former contents
  were either moved to the `merkle_tree` module or were `zcashd`-specific
  serialization methods which have been removed entirely and moved into the
  [zcashd](https://github.com/zcash/zcash) repository.
- The dependency on the `bridgetree` crate has been removed from
  `zcash_primitives` and the following zcashd-specific serialization methods
  have been moved to the [zcashd](https://github.com/zcash/zcash) repository:
  - `read_auth_fragment_v1`
  - `read_bridge_v1`
  - `read_bridge_v2`
  - `write_bridge_v2`
  - `write_bridge`
  - `read_checkpoint_v1`
  - `read_checkpoint_v2`
  - `write_checkpoint_v2`
  - `read_tree`
  - `write_tree`
- `merkle_tree::{SER_V1, SER_V2}` have been removed as they are now unused.

### Moved
- The following constants and methods have been moved from the
  `merkle_tree::incremental` module into the `merkle_tree` module to
  consolidate the serialization code for commitment tree frontiers:
  - `write_usize_leu64`
  - `read_leu64_usize`
  - `write_position`
  - `read_position`
  - `write_address`
  - `read_address`
  - `read_frontier_v0`
  - `write_nonempty_frontier`
  - `read_nonempty_frontier_v1`
  - `write_frontier_v1`
  - `read_frontier_v1`

### Added
- `merkle_tree::incremental::{read_address, write_address}`
- `merkle_tree::incremental::read_bridge_v2`
- `merkle_tree::write_commitment_tree` replaces `merkle_tree::CommitmentTree::write`
- `merkle_tree::read_commitment_tree` replaces `merkle_tree::CommitmentTree::read`
- `merkle_tree::write_incremental_witness` replaces `merkle_tree::IncrementalWitness::write`
- `merkle_tree::read_incremental_witness` replaces `merkle_tree::IncrementalWitness::read`
- `merkle_tree::merkle_path_from_slice` replaces `merkle_tree::MerklePath::from_slice`
- `sapling::{CommitmentTree, IncrementalWitness, MerklePath, NOTE_COMMITMENT_TREE_DEPTH}`
- `transaction::fees::zip317::MINIMUM_FEE`, reflecting the minimum possible
  [ZIP 317](https://zips.z.cash/zip-0317) conventional fee.
- `transaction::components::amount::Amount::const_from_i64`, intended for constructing
  a constant `Amount`.

### Changed
- The bounds on the `H` parameter to the following methods have changed:
  - `merkle_tree::incremental::read_frontier_v0`
  - `merkle_tree::incremental::read_auth_fragment_v1`
- The depth of the `merkle_tree::{CommitmentTree, IncrementalWitness, and MerklePath}`
  data types are now statically constrained using const generic type parameters.
- `transaction::fees::fixed::FeeRule::standard()` now uses the ZIP 317 minimum fee
  (10000 zatoshis rather than 1000 zatoshis) as the fixed fee. To be compliant with
  ZIP 317, use `transaction::fees::zip317::FeeRule::standard()` instead.

### Deprecated
- `transaction::components::amount::DEFAULT_FEE` has been deprecated. Depending on
  context, you may want to use `transaction::fees::zip317::MINIMUM_FEE`, or calculate
  the ZIP 317 conventional fee using `transaction::fees::zip317::FeeRule` instead.
- `transaction::fees::fixed::FeeRule::standard()` has been deprecated.
  Use either `transaction::fees::zip317::FeeRule::standard()` or
  `transaction::fees::fixed::FeeRule::non_standard`.

## [0.11.0] - 2023-04-15
### Added
- `zcash_primitives::zip32::fingerprint` module, containing types for deriving
  ZIP 32 Seed Fingerprints.

### Changed
- Bumped dependencies to `bls12_381 0.8`, `ff 0.13`, `group 0.13`,
  `jubjub 0.10`, `orchard 0.4`, `sha2 0.10`, `bip0039 0.10`,
  `zcash_note_encryption 0.3`.

## [0.10.2] - 2023-03-16
### Added
- `zcash_primitives::sapling::note`:
  - `NoteCommitment::temporary_zcashd_derive`
- A new feature flag, `multicore`, has been added and is enabled by default.
  This allows users to selectively disable multicore support for Orchard proof
  creation by setting `default_features = false` on their `zcash_primitives`
  dependency, such as is needed to enable `wasm32-wasi` compilation.

## [0.10.1] - 2023-03-08
### Added
- Sapling bundle component constructors, behind the `temporary-zcashd` feature
  flag. These temporarily re-expose the ability to construct invalid Sapling
  bundles (that was removed in 0.10.0), and will be removed in a future release:
  - `zcash_primitives::transaction::components::sapling`:
    - `Bundle::temporary_zcashd_from_parts`
    - `SpendDescription::temporary_zcashd_from_parts`
    - `OutputDescription::temporary_zcashd_from_parts`

## [0.10.0] - 2023-02-01
### Added
- `zcash_primitives::sapling`:
  - `keys::DiversifiedTransmissionKey`
  - `keys::{EphemeralSecretKey, EphemeralPublicKey, SharedSecret}`
  - `keys::{PreparedIncomingViewingKey, PreparedEphemeralPublicKey}`
    (re-exported from `note_encryption`).
  - `note`, a module containing types related to Sapling notes. The existing
    `Note` and `Rseed` types are re-exported here, and new types are added.
  - `Node::from_cmu`
  - `value`, containing types for handling Sapling note values and value
    commitments.
  - `Note::from_parts`
  - `Note::{recipient, value, rseed}` getter methods.
  - `impl Eq for Note`
  - `impl Copy for PaymentAddress`

### Changed
- MSRV is now 1.60.0.
- `zcash_primitives::transaction::components::sapling::builder`:
  - `SaplingBuilder::add_output` now takes a
    `zcash_primitives::sapling::value::NoteValue`.
- `zcash_primitives::sapling`:
  - `PaymentAddress::from_parts` now rejects invalid diversifiers.
  - `PaymentAddress::create_note` is now infallible.
  - `DiversifiedTransmissionKey` is now used instead of `jubjub::SubgroupPoint`
    in the following places:
    - `PaymentAddress::from_parts`
    - `PaymentAddress::pk_d`
    - `note_encryption::SaplingDomain::DiversifiedTransmissionKey`
  - `EphemeralSecretKey` is now used instead of `jubjub::Scalar` in the
    following places:
    - `Note::generate_or_derive_esk`
    - `note_encryption::SaplingDomain::EphemeralSecretKey`
  - `note_encryption::SaplingDomain::EphemeralPublicKey` is now
    `EphemeralPublicKey` instead of `jubjub::ExtendedPoint`.
  - `note_encryption::SaplingDomain::SharedSecret` is now `SharedSecret` instead
    of `jubjub::SubgroupPoint`.
- Note commitments now use
  `zcash_primitives::sapling::note::ExtractedNoteCommitment` instead of
  `bls12_381::Scalar` in the following places:
  - `zcash_primitives::sapling`:
    - `Note::cmu`
  - `zcash_primitives::sapling::note_encryption`:
    - `SaplingDomain::ExtractedCommitment`
  - `zcash_primitives::transaction::components::sapling`:
    - `OutputDescription::cmu`
    - The `cmu` field of `CompactOutputDescription`.
- Value commitments now use `zcash_primitives::sapling::value::ValueCommitment`
  instead of `jubjub::ExtendedPoint` in the following places:
  - `zcash_primitives::sapling::note_encryption`:
    - `prf_ock`
    - `SaplingDomain::ValueCommitment`
  - `zcash_primitives::sapling::prover`:
    - `TxProver::{spend_proof, output_proof}` return type.
  - `zcash_primitives::transaction::components`:
    - `SpendDescription::cv`
    - `OutputDescription::cv`
- `zcash_primitives::transaction::components`:
  - `sapling::{Bundle, SpendDescription, OutputDescription}` have had their
    fields replaced by getter methods.
  - The associated type `sapling::Authorization::Proof` has been replaced by
    `Authorization::{SpendProof, OutputProof}`.
  - `sapling::MapAuth::map_proof` has been replaced by
    `MapAuth::{map_spend_proof, map_output_proof}`.

### Removed
- `zcash_primitives::sapling`:
  - The fields of `Note` are now private (use the new getter methods instead).
  - `Note::uncommitted` (use `Node::empty_leaf` instead).
  - `Note::derive_esk` (use `SaplingDomain::derive_esk` instead).
  - `Note::commitment` (use `Node::from_cmu` instead).
  - `PaymentAddress::g_d`
  - `NoteValue` (use `zcash_primitives::sapling::value::NoteValue` instead).
  - `ValueCommitment` (use `zcash_primitives::sapling::value::ValueCommitment`
    or `zcash_proofs::circuit::sapling::ValueCommitmentPreimage` instead).
  - `note_encryption::sapling_ka_agree`
  - `testing::{arb_note_value, arb_positive_note_value}` (use the methods in
    `zcash_primitives::sapling::value::testing` instead).
- `zcash_primitives::transaction::components`:
  - The fields of `sapling::{SpendDescriptionV5, OutputDescriptionV5}` (they are
    now opaque types; use `sapling::{SpendDescription, OutputDescription}`
    instead).
  - `sapling::read_point`

## [0.9.1] - 2022-12-06
### Fixed
- `zcash_primitives::transaction::builder`:
  - `Builder::build` was calling `FeeRule::fee_required` with the number of
    Sapling outputs that have been added to the builder. It now instead provides
    the number of outputs that will be in the final Sapling bundle, including
    any padding.

## [0.9.0] - 2022-11-12
### Added
- Added to `zcash_primitives::transaction::builder`:
  - `Error::{InsufficientFunds, ChangeRequired, Balance, Fee}`
  - `Builder` state accessor methods:
    - `Builder::{params, target_height}`
    - `Builder::{transparent_inputs, transparent_outputs}`
    - `Builder::{sapling_inputs, sapling_outputs}`
- `zcash_primitives::transaction::fees`, a new module containing abstractions
  and types related to fee calculations.
  - `FeeRule`, a trait that describes how to compute the fee required for a
    transaction given inputs and outputs to the transaction.
  - `fixed`, a module containing an implementation of the old fixed fee rule.
  - `zip317`, a module containing an implementation of the ZIP 317 fee rules.
- Added to `zcash_primitives::transaction::components`:
  - `amount::NonNegativeAmount`
  - Added to the `orchard` module:
    - `impl MapAuth<orchard::bundle::Authorized, orchard::bundle::Authorized> for ()`
      (the identity map).
  - Added to the `sapling` module:
    - `impl MapAuth<Authorized, Authorized> for ()` (the identity map).
    - `builder::SaplingBuilder::{inputs, outputs}`: accessors for Sapling
      builder state.
    - `fees`, a module with Sapling-specific fee traits.
  - Added to the `transparent` module:
    - `impl {PartialOrd, Ord} for OutPoint`
    - `builder::TransparentBuilder::{inputs, outputs}`: accessors for
      transparent builder state.
    - `fees`, a module with transparent-specific fee traits.
- Added to `zcash_primitives::sapling`:
  - `Note::commitment`
  - `impl Eq for PaymentAddress`
- Added to `zcash_primitives::zip32`:
  - `impl TryFrom<DiversifierIndex> for u32`
  - `sapling::DiversifiableFullViewingKey::{diversified_address, diversified_change_address}`

### Changed
- `zcash_primitives::transaction::builder`:
  - `Builder::build` now takes a `FeeRule` argument which is used to compute the
    fee for the transaction as part of the build process.
  - `Builder::value_balance` now returns `Result<Amount, BalanceError>` instead
    of `Option<Amount>`.
  - `Builder::{new, new_with_rng}` no longer fixes the fee for transactions to
    0.00001 ZEC; the builder instead computes the fee using a `FeeRule`
    implementation at build time.
  - `Error` now is parameterized by the types that can now be produced by fee
    calculation.
- `zcash_primitives::transaction::components::tze::builder::Builder::value_balance` now
  returns `Result<Amount, BalanceError>` instead of `Option<Amount>`.

### Deprecated
- `zcash_primitives::zip32::sapling::to_extended_full_viewing_key` (use
  `to_diversifiable_full_viewing_key` instead).

### Removed
- Removed from `zcash_primitives::transaction::builder`:
  - `Builder::{new_with_fee, new_with_rng_and_fee`} (use `Builder::{new, new_with_rng}`
    instead along with a `FeeRule` implementation passed to `Builder::build`.)
  - `Builder::send_change_to` (change outputs must be added to the builder by
    the caller, just like any other output).
  - `Error::ChangeIsNegative`
  - `Error::NoChangeAddress`
  - `Error::InvalidAmount` (replaced by `Error::BalanceError`).
- Removed from `zcash_primitives::transaction::components::sapling::builder`:
  - `SaplingBuilder::get_candidate_change_address` (change outputs must now be
    added by the caller).
- Removed from `zcash_primitives::zip32::sapling`:
  - `impl From<&ExtendedSpendingKey> for ExtendedFullViewingKey` (use
    `ExtendedSpendingKey::to_diversifiable_full_viewing_key` instead).
- `zcash_primitives::sapling::Node::new` (use `Node::from_scalar` or preferably
  `Note::commitment` instead).

## [0.8.1] - 2022-10-19
### Added
- `zcash_primitives::legacy`:
  - `impl {Copy, Eq, Ord} for TransparentAddress`
  - `keys::AccountPrivKey::{to_bytes, from_bytes}`
- `zcash_primitives::sapling::NullifierDerivingKey`
- Added in `zcash_primitives::sapling::keys`
  - `DecodingError`
  - `Scope`
  - `ExpandedSpendingKey::from_bytes`
  - `ExtendedSpendingKey::{from_bytes, to_bytes}`
- `zcash_primitives::sapling::note_encryption`:
  - `PreparedIncomingViewingKey`
  - `PreparedEphemeralPublicKey`
- Added in `zcash_primitives::zip32`
  - `ChainCode::as_bytes`
  - `DiversifierIndex::{as_bytes}`
  - Implementations of `From<u32>` and `From<u64>` for `DiversifierIndex`
- `zcash_primitives::zip32::sapling` has been added and now contains
  all of the Sapling zip32 key types that were previously located in
  `zcash_primitives::zip32` directly. The base `zip32` module reexports
  the moved types for backwards compatibility.
  - `DiversifierKey::{from_bytes, as_bytes}`
  - `ExtendedSpendingKey::{from_bytes, to_bytes}`
- `zcash_primitives::transaction::Builder` constructors:
  - `Builder::new_with_fee`
  - `Builder::new_with_rng_and_fee`
- `zcash_primitives::transaction::TransactionData::fee_paid`
- `zcash_primitives::transaction::components::amount::BalanceError`
- Added in `zcash_primitives::transaction::components::sprout`
  - `Bundle::value_balance`
  - `JSDescription::net_value`
- Added in `zcash_primitives::transaction::components::transparent`
  - `Bundle::value_balance`
  - `TxOut::recipient_address`
- Implementations of `memuse::DynamicUsage` for the following types:
  - `zcash_primitives::block::BlockHash`
  - `zcash_primitives::consensus`:
    - `BlockHeight`
    - `MainNetwork`, `TestNetwork`, `Network`
    - `NetworkUpgrade`, `BranchId`
  - `zcash_primitives::sapling`:
    - `keys::Scope`
    - `note_encryption::SaplingDomain`
  - `zcash_primitives::transaction`:
    - `TxId`
    - `components::sapling::CompactOutputDescription`
    - `components::sapling::{OutputDescription, OutputDescriptionV5}`
  - `zcash_primitives::zip32::AccountId`

### Changed
- Migrated to `group 0.13`, `orchard 0.3`, `zcash_address 0.2`, `zcash_encoding 0.2`.
- `zcash_primitives::sapling::ViewingKey` now stores `nk` as a
  `NullifierDerivingKey` instead of as a bare `jubjub::SubgroupPoint`.
- The signature of `zcash_primitives::sapling::Note::nf` has changed to
  take just a `NullifierDerivingKey` (the only capability it actually required)
  rather than the full `ViewingKey` as its first argument.
- Made the internals of `zip32::DiversifierKey` private; use `from_bytes` and
  `as_bytes` on this type instead.
- `zcash_primitives::sapling::note_encryption` APIs now expose precomputations
  explicitly (where previously they were performed internally), to enable users
  to avoid recomputing incoming viewing key precomputations. Users now need to
  call `PreparedIncomingViewingKey::new` to convert their `SaplingIvk`s into
  their precomputed forms, and can do so wherever it makes sense in their stack.
  - `SaplingDomain::IncomingViewingKey` is now `PreparedIncomingViewingKey`
    instead of `SaplingIvk`.
  - `try_sapling_note_decryption` and `try_sapling_compact_note_decryption` now
    take `&PreparedIncomingViewingKey` instead of `&SaplingIvk`.

### Removed
- `zcash_primitives::legacy::Script::address` This method was not generally
  safe to use on arbitrary scripts, only on script_pubkey values. Its
  functionality is now available via
  `zcash_primitives::transaction::components::transparent::TxOut::recipient_address`

## [0.8.0] - 2022-10-19
This release was yanked because it depended on the wrong versions of `zcash_address`
and `zcash_encoding`.

## [0.7.0] - 2022-06-24
### Changed
- Bumped dependencies to `equihash 0.2`, `orchard 0.2`.
- `zcash_primitives::consensus`:
  - `MainNetwork::activation_height` now returns the activation height for
    `NetworkUpgrade::Nu5`.

## [0.6.0] - 2022-05-11
### Added
- `zcash_primitives::sapling::redjubjub::PublicKey::verify_with_zip216`, for
  controlling how RedJubjub signatures are validated. `PublicKey::verify` has
  been altered to always use post-ZIP 216 validation rules.
- `zcash_primitives::transaction::Builder::with_progress_notifier`, for setting
  a notification channel on which transaction build progress updates will be
  sent.
- `zcash_primitives::transaction::Txid::{read, write, from_bytes}`
- `zcash_primitives::sapling::NoteValue` a typesafe wrapper for Sapling note values.
- `zcash_primitives::consensus::BranchId::{height_range, height_bounds}` functions
  to provide range values for branch active heights.
- `zcash_primitives::consensus::NetworkUpgrade::Nu5` value representing the Nu5 upgrade.
- `zcash_primitives::consensus::BranchId::Nu5` value representing the Nu5 consensus branch.
- New modules under `zcash_primitives::transaction::components` for building parts of
  transactions:
  - `sapling::builder` for Sapling transaction components.
  - `transparent::builder` for transparent transaction components.
  - `tze::builder` for TZE transaction components.
  - `orchard` parsing and serialization for Orchard transaction components.
- `zcash_primitives::transaction::Authorization` a trait representing a type-level
  record of authorization types that correspond to signatures, witnesses, and
  proofs for each Zcash sub-protocol (transparent, Sprout, Sapling, TZE, and
  Orchard). This type makes it possible to encode a type-safe state machine
  for the application of authorizing data to a transaction; implementations of
  this trait represent different states of the authorization process.
- New bundle types under the `zcash_primitives::transaction` submodules, one for
  each Zcash sub-protocol. These are now used instead of bare fields
  within the `TransactionData` type.
  - `components::sapling::Bundle` bundle of
    Sapling transaction elements. This new struct is parameterized by a
    type bounded on a newly added `sapling::Authorization` trait which
    is used to enable static reasoning about the state of Sapling proofs and
    authorizing data, as described above.
  - `components::transparent::Bundle` bundle of
    transparent transaction elements. This new struct is parameterized by a
    type bounded on a newly added `transparent::Authorization` trait which
    is used to enable static reasoning about the state of transparent witness
    data, as described above.
  - `components::tze::Bundle` bundle of TZE
    transaction elements. This new struct is parameterized by a
    type bounded on a newly added `tze::Authorization` trait which
    is used to enable static reasoning about the state of TZE witness
    data, as described above.
- `zcash_primitives::serialize` has been factored out as a new `zcash_encoding`
  crate, which can be found in the `components` directory.
- `zcash_primitives::transaction::components::Amount` now implements
  `memuse::DynamicUsage`, to enable `orchard::Bundle<_, Amount>::dynamic_usage`.
- `zcash_primitives::zip32::diversifier` has been renamed to `find_sapling_diversifier`
  and `sapling_diversifier` has been added. `find_sapling_diversifier` searches the
  diversifier index space, whereas `sapling_diversifier` just attempts to use the
  provided diversifier index and returns `None` if it does not produce a valid
  diversifier.
- `zcash_primitives::zip32::DiversifierKey::diversifier` has been renamed to
  `find_diversifier` and the `diversifier` method has new semantics.
  `find_diversifier` searches the diversifier index space to find a diversifier
  index which produces a valid diversifier, whereas `diversifier` just attempts
  to use the provided diversifier index and returns `None` if it does not
  produce a valid diversifier.
- `zcash_primitives::zip32::ExtendedFullViewingKey::address` has been renamed
  to `find_address` and the `address` method has new semantics. `find_address`
  searches the diversifier index space until it obtains a valid diversifier,
  and returns the address corresponding to that diversifier, whereas `address`
  just attempts to create an address corresponding to the diversifier derived
  from the provided diversifier index and returns `None` if the provided index
  does not produce a valid diversifier.
- `zcash_primitives::zip32::ExtendedSpendingKey.derive_internal` has been
  added to facilitate the derivation of an internal (change) spending key.
  This spending key can be used to spend change sent to an internal address
  corresponding to the associated full viewing key as specified in
  [ZIP 316](https://zips.z.cash/zip-0316#encoding-of-unified-full-incoming-viewing-keys)..
- `zcash_primitives::zip32::ExtendedFullViewingKey.derive_internal` has been
  added to facilitate the derivation of an internal (change) spending key.
  This spending key can be used to spend change sent to an internal address
  corresponding to the associated full viewing key as specified in
  [ZIP 32](https://zips.z.cash/zip-0032#deriving-a-sapling-internal-spending-key).
- `zcash_primitives::zip32::sapling_derive_internal_fvk` provides the
  internal implementation of `ExtendedFullViewingKey.derive_internal` but does
  not require a complete extended full viewing key, just the full viewing key
  and the diversifier key. In the future, this function will likely be
  refactored to become a member function of a new `DiversifiableFullViewingKey`
  type, which represents the ability to derive IVKs, OVKs, and addresses, but
  not child viewing keys.
- `zcash_primitives::sapling::keys::DiversifiableFullViewingKey::change_address`
  has been added as a convenience method for obtaining the change address
  at the default diversifier. This address **MUST NOT** be encoded and exposed
  to users. User interfaces should instead mark these notes as "change notes" or
  "internal wallet operations".
- A new module `zcash_primitives::legacy::keys` has been added under the
  `transparent-inputs` feature flag to support types related to supporting
  transparent components of unified addresses and derivation of OVKs for
  shielding funds from the transparent pool.
- A `zcash_primitives::transaction::components::amount::Amount::sum`
  convenience method has been added to facilitate bounds-checked summation of
  account values.
- The `zcash_primitives::zip32::AccountId`, a type-safe wrapper for ZIP 32
  account indices.
- In `zcash_primitives::transaction::components::amount`:
  - `impl Sum<&Amount> for Option<Amount>`

### Changed
- MSRV is now 1.56.1.
- Bumped dependencies to `ff 0.12`, `group 0.12`, `bls12_381 0.7`, `jubjub 0.9`,
  `bitvec 1`.
- The following modules and helpers have been moved into
  `zcash_primitives::sapling`:
  - `zcash_primitives::group_hash`
  - `zcash_primitives::keys`
    - `zcash_primitives::sapling::keys::{prf_expand, prf_expand_vec, OutgoingViewingKey}`
      have all been moved into to the this module to reflect the fact that they
      are used outside of the Sapling protocol.
  - `zcash_primitives::pedersen_hash`
  - `zcash_primitives::primitives::*` (moved into `zcash_primitives::sapling`)
  - `zcash_primitives::prover`
  - `zcash_primitives::redjubjub`
  - `zcash_primitives::util::{hash_to_scalar, generate_random_rseed}`
- Renamed `zcash_primitives::transaction::components::JSDescription` to
  `JsDescription` (matching Rust naming conventions).
- `zcash_primitives::transaction::TxId` contents is now private.
- Renamed `zcash_primitives::transaction::components::tze::hash` to
  `zcash_primitives::transaction::components::tze::txid`
- `zcash_primitives::transaction::components::tze::TzeOutPoint` constructor
  now taxes a TxId rather than a raw byte array.
- `zcash_primitives::transaction::components::Amount` addition, subtraction,
  and summation now return `Option` rather than panicing on overflow.
- `zcash_primitives::transaction::builder`:
  - `Error` has been modified to wrap the error types produced by its child
    builders.
  - `Builder::build` no longer takes a consensus branch ID parameter. The
    builder now selects the correct consensus branch ID for the given target
    height.
- The `zcash_primitives::transaction::TransactionData` struct has been modified
  such that it now contains common header information, and then contains
  a separate `Bundle` value for each sub-protocol (transparent, Sprout, Sapling,
  and TZE) and an Orchard bundle value has been added. `TransactionData` is now
  parameterized by a type bounded on the newly added
  `zcash_primitives::transaction::Authorization` trait. This bound has been
  propagated to the individual transaction builders, such that the authorization
  state of a transaction is clearly represented in the type and the presence
  or absence of witness and/or proof data is statically known, instead of being only
  determined at runtime via the presence or absence of `Option`al values.
- `zcash_primitives::transaction::components::sapling` parsing and serialization
  have been adapted for use with the new `sapling::Bundle` type.
- `zcash_primitives::transaction::Transaction` parsing and serialization
  have been adapted for use with the new `TransactionData` organization.
- Generators for property testing have been moved out of the main transaction
  module such that they are now colocated in the modules with the types
  that they generate.
- The `ephemeral_key` field of `OutputDescription` has had its type changed from
  `jubjub::ExtendedPoint` to `zcash_note_encryption::EphemeralKeyBytes`.
- The `epk: jubjub::ExtendedPoint` field of `CompactOutputDescription ` has been
  replaced by `ephemeral_key: zcash_note_encryption::EphemeralKeyBytes`.
- The `zcash_primitives::transaction::Builder::add_sapling_output` method
  now takes its `MemoBytes` argument as a required field rather than an
  optional one. If the empty memo is desired, use
  `MemoBytes::from(Memo::Empty)` explicitly.
- `zcash_primitives::zip32`:
  - `ExtendedSpendingKey::default_address` no longer returns `Option<_>`.
  - `ExtendedFullViewingKey::default_address` no longer returns `Option<_>`.

## [0.5.0] - 2021-03-26
### Added
- Support for implementing candidate ZIPs before they have been selected for a
  network upgrade, behind the `zfuture` feature flag.
  - At runtime, these ZIPs are gated behind the new `NetworkUpgrade::ZFuture`
    enum case, which is inaccessible without the `zfuture` feature flag. This
    pseudo-NU can be enabled for private testing using a custom implementation
    of the `Parameters` trait.
- New structs and methods:
  - `zcash_primitives::consensus`:
    - `BlockHeight`
    - New methods on the `Parameters` trait:
      - `coin_type`
      - `hrp_sapling_extended_spending_key`
      - `hrp_sapling_extended_full_viewing_key`
      - `hrp_sapling_payment_address`
      - `b58_pubkey_address_prefix`
      - `b58_script_address_prefix`
    - The `Network` enum, which enables code to be generic over the network type
      at runtime.
  - `zcash_primitives::memo`:
    - `MemoBytes`, a minimal wrapper around the memo bytes, that only imposes
      the existence of null-padding for shorter memos. `MemoBytes` is guaranteed
      to be round-trip encodable (modulo null padding).
    - `Memo`, an enum that implements the memo field format defined in
      [ZIP 302](https://zips.z.cash/zip-0302). It can be converted to and from
      `MemoBytes`.
  - `zcash_primitives::primitives::Nullifier` struct.
  - `zcash_primitives::transaction`:
    - `TxVersion` enum, representing the set of valid transaction format
      versions.
    - `SignableInput` enum, encapsulating per-input data used when
      creating transaction signatures.
  - `zcash_primitives::primitives::SaplingIvk`, a newtype wrapper around `jubjub::Fr`
    values that are semantically Sapling incoming viewing keys.
- Test helpers, behind the `test-dependencies` feature flag:
  - `zcash_primitives::prover::mock::MockTxProver`, for building transactions in
    tests without creating proofs.
  - `zcash_primitives::transaction::Builder::test_only_new_with_rng` constructor
    which accepts a non-`CryptoRng` randomness source (for e.g. deterministic
    tests).
  - `proptest` APIs for generating arbitrary Zcash types.
- New constants:
  - `zcash_primitives::consensus`:
    - `H0`, the height of the genesis block.
    - `MAIN_NETWORK`
    - `TEST_NETWORK`
  - `zcash_primitives::constants::{mainnet, testnet, regtest}` modules,
    containing network-specific constants.
  - `zcash_primitives::note_encryption`:
    - `ENC_CIPHERTEXT_SIZE`
    - `OUT_CIPHERTEXT_SIZE`
  - `zcash_primitives::transaction::components::amount`:
    - `COIN`
    - `MAX_MONEY`
- More implementations of standard traits:
  - `zcash_primitives::consensus`:
    - `Parameters: Clone`
    - `MainNetwork: PartialEq`
    - `TestNetwork: PartialEq`
  - `zcash_primitives::legacy`:
    - `Script: PartialEq`
    - `TransparentAddress: Clone + PartialOrd + Hash`
  - `zcash_primitives::redjubjub::PublicKey: Clone`
  - `zcash_primitives::transaction`:
    - `Transaction: Clone`
    - `TransactionData: Clone + Default`
    - `components::Amount: Eq + PartialOrd + Ord`
    - `components::TxIn: Clone + PartialEq`
    - `components::TxOut: PartialEq`
    - `components::SpendDescription: Clone`
    - `components::OutputDescription: Clone`
    - `components::SproutProof: Clone`
    - `components::JSDescription: Clone`
  - `zcash_primitives::zip32::DiversifierIndex: Default`

### Changed
- MSRV is now 1.47.0.
- Trial decryption using the APIs in `zcash_primitives::note_encryption` is now
  over 60% faster at detecting which notes are relevant.
  - Part of this improvement was achieved by changing the APIs to take `epk` as
    a `&jubjub::ExtendedPoint` instead of a `&SubgroupPoint`.
- Various APIs now take the network parameters as an explicit variable instead
  of a type parameter:
  - `zcash_primitives::consensus::BranchId::for_height`
  - The `zcash_primitives::note_encryption` APIs.
  - `zcash_primitives::transaction::builder`:
    - `SaplingOutput::new`
    - `Builder::new`
    - `Builder::new_with_rng`
  - `Parameters::activation_height` and `Parameters::is_nu_active` now take
    `&self`.
- `zcash_primitives::merkle_tree::CommitmentTree::new` has been renamed to
  `CommitmentTree::empty`.
- `zcash_primitives::note_encryption`:
  - `SaplingNoteEncryption::new` now takes `MemoBytes`.
  - The following APIs now return `MemoBytes`:
    - `try_sapling_note_decryption`
    - `try_sapling_output_recovery`
    - `try_sapling_output_recovery_with_ock`
- `zcash_primitives::primitives::SaplingIvk` is now used where functions
  previously used undistinguished `jubjub::Fr` values; this affects Sapling
  note decryption and handling of IVKs by the wallet backend code.
- `zcash_primitives::primitives::ViewingKey::ivk` now returns `SaplingIvk`
- `zcash_primitives::primitives::Note::nf` now returns `Nullifier`.
- `zcash_primitives::transaction`:
  - The `overwintered`, `version`, and `version_group_id` properties of the
    `Transaction` and `TransactionData` structs have been replaced by
    `version: TxVersion`.
  - `components::amount::DEFAULT_FEE` is now 1000 zatoshis, following
    [ZIP 313](https://zips.z.cash/zip-0313).
  - The `nullifier` property of `components::SpendDescription` now has the type
    `Nullifier`.
  - `signature_hash` and `signature_hash_data` now take a `SignableInput`
    argument instead of a `transparent_input` argument.
  - `builder::SaplingOutput::new` and `builder::Builder::add_sapling_output` now
    take `Option<MemoBytes>`.

### Removed
- `zcash_primitives::note_encryption::Memo` (replaced by
  `zcash_primitives::memo::{Memo, MemoBytes}`).

## [0.4.0] - 2020-09-09
### Added
- `zcash_primitives::note_encryption::OutgoingCipherKey` - a symmetric key that
  can be used to recover a single Sapling output. This will eventually be used
  to implement Sapling payment disclosures.

### Changed
- MSRV is now 1.44.1.
- `zcash_primitives::note_encryption`:
  - `SaplingNoteEncryption::new` now takes `Option<OutgoingViewingKey>`. Setting
    this to `None` prevents the note from being recovered from the block chain
    by the sender.
    - The `rng: &mut R` parameter (where `R: RngCore + CryptoRng`) has been
      changed to `rng: R` to enable this use case.
  - `prf_ock` now returns `OutgoingCipherKey`.
  - `try_sapling_output_recovery_with_ock` now takes `&OutgoingCipherKey`.
- `zcash_primitives::transaction::builder`:
  - `SaplingOutput::new` and `Builder::add_sapling_output` now take
    `Option<OutgoingViewingKey>` (exposing the new unrecoverable note option).
- Bumped dependencies to `ff 0.8`, `group 0.8`, `bls12_381 0.3.1`,
  `jubjub 0.5.1`, `secp256k1 0.19`.

## [0.3.0] - 2020-08-24
TBD

## [0.2.0] - 2020-03-13
TBD

## [0.1.0] - 2019-10-08
Initial release.
