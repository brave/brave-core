# Changelog
All notable changes to this library will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this library adheres to Rust's notion of
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.3] - 2024-03-25

### Added
- `impl {PartialOrd, Ord} for sapling_crypto::note::Nullifier`
- Additions under the `test-dependencies` feature flag:
  - `sapling-crypto::tree::Node::random`
  - `impl Distribution<sapling-crypto::tree::Node> for Standard`

## [0.1.2] - 2024-03-08
### Added
- `sapling_crypto::zip32::IncomingViewingKey`
- `sapling_crypto::zip32::DiversifiableFullViewingKey::to_external_ivk`

## [0.1.1] - 2024-02-15
### Fixed
- `sapling_crypto::builder::BundleType::num_outputs` now matches the previous
  behaviour for Sapling bundle padding, by including dummy outputs if there are
  no requested outputs but some requested spends, and `bundle_required` is set
  to `false` (as in `BundleType::DEFAULT`).

## [0.1.0] - 2024-01-26
The crate has been completely rewritten. See [`zcash/librustzcash`] for the
history of this rewrite.

The entries below are relative to the `zcash_primitives::sapling` module as of
`zcash_primitives 0.13.0`.

### Added
- `sapling_crypto::Anchor`
- `sapling_crypto::BatchValidator` (moved from `zcash_proofs::sapling`).
- `sapling_crypto::SaplingVerificationContext` (moved from
  `zcash_proofs::sapling`).
- `sapling_crypto::builder` (moved from
  `zcash_primitives::transaction::components::sapling::builder`). Further
  additions to this module:
  - `UnauthorizedBundle`
  - `InProgress`
  - `{InProgressProofs, Unproven, Proven}`
  - `{InProgressSignatures, Unsigned, PartiallyAuthorized}`
  - `{MaybeSigned, SigningParts}`
  - `SpendInfo`
  - `OutputInfo`
  - `ProverProgress`
  - `BundleType`
  - `SigningMetadata`
  - `bundle` bundle builder function.
- `sapling_crypto::bundle` module:
  - The following types moved from
    `zcash_primitives::transaction::components::sapling`:
    - `Bundle`
    - `SpendDescription, SpendDescriptionV5`
    - `OutputDescription, OutputDescriptionV5`
    - `Authorization, Authorized`
    - `GrothProofBytes`
  - `Bundle::<InProgress<Unproven, _>>::create_proofs`
  - `Bundle::<InProgress<_, Unsigned>>::prepare`
  - `Bundle::<InProgress<_, PartiallyAuthorized>>::{sign, append_signatures}`
  - `Bundle::<InProgress<Proven, PartiallyAuthorized>>::finalize`
  - `Bundle::<InProgress<Proven, Unsigned>>::apply_signatures`
  - `Bundle::try_map_authorization`
  - `testing` module, containing the following functions moved from
    `zcash_primitives::transaction::components::sapling::testing`:
    - `arb_output_description`
    - `arb_bundle`
- `sapling_crypto::circuit` module (moved from `zcash_proofs::circuit::sapling`).
  Additional additions to this module:
  - `{SpendParameters, OutputParameters}`
  - `{SpendVerifyingKey, PreparedSpendVerifyingKey}`
  - `{OutputVerifyingKey, PreparedOutputVerifyingKey}`
- `sapling_crypto::constants` module.
- `sapling_crypto::keys`:
  - `SpendAuthorizingKey`
  - `SpendValidatingKey`
- `sapling_crypto::note_encryption`:
  - `CompactOutputDescription` (moved from
    `zcash_primitives::transaction::components::sapling`).
  - `SaplingDomain::new`
  - `Zip212Enforcement`
- `sapling_crypto::prover::{SpendProver, OutputProver}`
- `sapling_crypto::tree::Node::{from_bytes, to_bytes}`
- `sapling_crypto::value`:
  - `NoteValue::ZERO`
  - `ValueCommitTrapdoor::from_bytes`
  - `impl Sub<TrapdoorSum> for TrapdoorSum`
  - `impl Sub<CommitmentSum> for CommitmentSum`
- `sapling_crypto::zip32` module (moved from `zcash_primitives::zip32::sapling`).
- `impl Debug for sapling_crypto::keys::{ExpandedSpendingKey, ProofGenerationKey}`
- Test helpers, behind the `test-dependencies` feature flag:
  - `sapling_crypto::prover::mock::{MockSpendProver, MockOutputProver}`

### Changed
- `sapling_crypto`:
  - `BatchValidator::validate` now takes the `SpendVerifyingKey` and
    `OutputVerifyingKey` newtypes.
  - `SaplingVerificationContext::new` now always creates a context with ZIP 216
    rules enforced, and no longer has a boolean for configuring this.
  - `SaplingVerificationContext::{check_spend, final_check}` now use the
    `redjubjub` crate types for `rk`, `spend_auth_sig`, and `binding_sig`.
  - `SaplingVerificationContext::{check_spend, check_output}` now take
    the `PreparedSpendVerifyingKey` and `PreparedOutputVerifyingKey`
    newtypes.
  - `SaplingVerificationContext::final_check` now takes its `value_balance`
    argument as `V: Into<i64>` instead of
    `zcash_primitives::transaction::components::Amount`.
- `sapling_crypto::address::PaymentAddress::create_note` now takes its `value`
  argument as a `NoteValue` instead of as a bare `u64`.
- `sapling_crypto::builder`:
  - `SaplingBuilder` has been renamed to `Builder`
  - `MaybeSigned::SigningMetadata` has been renamed to `MaybeSigned::SigningParts`
  - `Builder` no longer has a `P: zcash_primitives::consensus::Parameters`
    type parameter.
  - `Builder::new` now takes a `Zip212Enforcement` argument instead of a
    `P: zcash_primitives::consensus::Parameters` argument and a target height.
    It also now takes as an argument the Sapling anchor to be used for all
    spends in the bundle. 
  - `Builder::add_spend` now takes `extsk` by reference. Also, it no
    longer takes a `diversifier` argument as the diversifier may be obtained
    from the note. All calls to `add_spend` are now required to use an anchor
    that corresponds to the anchor provided at builder construction.
  - `Builder::add_output` now takes an `Option<[u8; 512]>` memo instead
    of a `MemoBytes`.
  - `Builder::build` no longer takes a prover, proving context, progress
    notifier, or target height. Instead, it has `SpendProver, OutputProver`
    generic parameters and returns `(UnauthorizedBundle, SaplingMetadata)`. The
    caller can then use `Bundle::<InProgress<Unproven, _>>::create_proofs` to
    create spend and output proofs for the bundle.
  - `Builder::build` now takes a `BundleType` argument that instructs
    it how to pad the bundle with dummy outputs.
  - `Error` has new error variants:
    - `Error::DuplicateSignature`
    - `Error::InvalidExternalSignature`
    - `Error::MissingSignatures`
    - `Error::BundleTypeNotSatisfiable`
- `sapling_crypto::bundle`:
  - `Bundle` now has a second generic parameter `V`.
  - `Bundle::value_balance` now returns `&V` instead of
    `&zcash_primitives::transaction::components::Amount`.
  - `Bundle::map_authorization` now takes a context argument and explicit 
    functions for each mappable field, rather than a `MapAuth` value, in 
    order to simplify handling of context values.
  - `Authorized::binding_sig` now has type `redjubjub::Signature<Binding>`.
  - `Authorized::AuthSig` now has type `redjubjub::Signature<SpendAuth>`.
  - `SpendDescription::temporary_zcashd_from_parts` now takes `rk` as
    `redjubjub::VerificationKey<SpendAuth>` instead of
    `sapling_crypto::redjubjub::PublicKey`.
  - `SpendDescription::rk` now returns `&redjubjub::VerificationKey<SpendAuth>`.
  - `SpendDescriptionV5::into_spend_description` now takes `spend_auth_sig` as
    `redjubjub::Signature<SpendAuth>` instead of
    `sapling_crypto::redjubjub::Signature`.
  - `testing::arb_bundle` now takes a `value_balance: V` argument.
- `sapling_crypto::circuit::ValueCommitmentOpening::value` is now represented as
  a `NoteValue` instead of as a bare `u64`.
- `sapling_crypto::keys`:
  - `DecodingError` has a new variant `UnsupportedChildIndex`.
  - `ExpandedSpendingKey.ask` now has type `SpendAuthorizingKey`.
  - `ProofGenerationKey.ak` now has type `SpendValidatingKey`.
  - `ViewingKey.ak` now has type `SpendValidatingKey`.
- `sapling_crypto::note_encryption`:
  - `SaplingDomain` no longer has a `P: zcash_primitives::consensus::Parameters`
    type parameter.
  - The following methods now take a `Zip212Enforcement` argument instead of a
    `P: zcash_primitives::consensus::Parameters` argument:
    - `plaintext_version_is_valid`
    - `try_sapling_note_decryption`
    - `try_sapling_compact_note_decryption`
    - `try_sapling_output_recovery_with_ock`
    - `try_sapling_output_recovery`
  - `SaplingDomain::Memo` now has type `[u8; 512]` instead of
    `zcash_primitives::memo::MemoBytes`.
  - `sapling_note_encryption` now takes `memo` as a `[u8; 512]` instead of
    `zcash_primitives::memo::MemoBytes`.
  - The following methods now return `[u8; 512]` instead of
    `zcash_primitives::memo::MemoBytes`:
    - `try_sapling_note_decryption`
    - `try_sapling_output_recovery_with_ock`
    - `try_sapling_output_recovery`
- `sapling_crypto::util::generate_random_rseed` now takes a `Zip212Enforcement`
  argument instead of a `P: zcash_primitives::consensus::Parameters` argument
  and a height.
- `sapling_crypto::value`:
  - `TrapdoorSum::into_bsk` now returns `redjubjub::SigningKey<Binding>` instead
    of `sapling_crypto::redjubjub::PrivateKey`.
  - `CommitmentSum::into_bvk` now returns `redjubjub::VerificationKey<Binding>`
    instead of `sapling_crypto::redjubjub::PublicKey`.

### Removed
- `sapling_crypto::bundle`:
  - `SpendDescription::{read, read_nullifier, read_rk, read_spend_auth_sig}`
  - `SpendDescription::{write_v4, write_v5_without_witness_data}`
  - `SpendDescriptionV5::read`
  - `OutputDescription::read`
  - `OutputDescription::{write_v4, write_v5_without_proof}`
  - `OutputDescriptionV5::read`
  - `MapAuth` trait
- `sapling_crypto::builder`:
  - `SpendDescriptionInfo`
- `sapling_crypto::note_encryption::SaplingDomain::for_height` (use
  `SaplingDomain::new` instead).
- `sapling_crypto::redjubjub` module (use the `redjubjub` crate instead).
- `sapling_crypto::spend_sig` (use `redjubjub::SigningKey::{randomize, sign}`
  instead).
- `sapling_crypto::builder::SaplingBuilder::bundle_output_count`

## [0.0.1] - 2017-12-06
Initial release to reserve crate name.
