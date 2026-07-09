# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to Rust's notion of
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

This release introduces `orchard::bundle::BundleVersion`, the `(value pool, protocol
version)` of an Orchard bundle, built from the new `orchard::ValuePool` and
`orchard::ProtocolVersion` types. Each `Bundle` now carries its `BundleVersion` as
non-serialized context, so a bundle can be serialized and committed to without separately
supplying a — possibly mismatching — version, and is encodable and committable by
construction. The post-NU 6.3 Action circuit enforces the cross-address restriction.
Existing callers keep the current behavior by constructing bundles with
`BundleVersion::orchard_v2()` and `BundleVersion::orchard_v2().default_flags()` (and
`OrchardCircuitVersion::FixedPostNu6_2` when building proving/verifying keys).

### Added
- NU6.3 and Ironwood bundle-version APIs:
  - `orchard::ValuePool`, the value pool an Orchard bundle belongs to (`Orchard` or
    `Ironwood`), and `orchard::ProtocolVersion`, the Orchard protocol version
    (`InsecureV1`, the historical pre-NU6.2 protocol that uses the unsound circuit; `V2`,
    NU6.2; `V3`, NU6.3, which also instantiates the Ironwood pool).
  - `orchard::bundle::BundleVersion`, the `(value pool, protocol version)` of an Orchard
    bundle. Its `const fn` constructors `orchard_insecure_v1`, `orchard_v2`, `orchard_v3`,
    and `ironwood_v3` make only the valid combinations representable. It determines the
    note plaintext version (`BundleVersion::note_version`), the circuit version
    (`BundleVersion::circuit_version`, when the `circuit` feature is enabled), the
    flag-byte interpretation (pre-NU6.3 rules, where bit 2 is reserved and cross-address
    transfers are implicitly enabled, vs NU6.3 rules, where bit 2 is `enableCrossAddress`),
    and whether consensus mandates the cross-address restriction (the builder then chooses
    the value within that constraint). `BundleVersion::value_pool` and
    `BundleVersion::protocol_version` return the bundle's `ValuePool` and
    `ProtocolVersion`; the Ironwood pool (`ironwood_v3`) shares the post-NU6.3 circuit and
    uses V3 note plaintexts. `BundleVersion::default_flags` returns the least-restrictive
    `Flags` consensus permits under the bundle version (spends and outputs enabled,
    cross-address transfers enabled except where the version mandates the restriction),
    the suitable default for the builder that a caller may restrict further.
  - `orchard::bundle::TxVersion`, the transaction version (`V5` or `V6`) a
    bundle's commitments are computed for. At NU6.3 an Orchard bundle may be
    encoded in a v5 or a v6 transaction; the two use different commitment
    personalizations and place the anchor in different digests (v5 in the
    transaction-ID effects, v6 in the authorizing data). It is passed to
    `Bundle::commitment`, `Bundle::authorizing_commitment`, and the
    `hash_bundle_*_empty` helpers.
- Cross-address restriction APIs:
  - `orchard::bundle::Flags::CROSS_ADDRESS_DISABLED`, the restricted flag set. It
    cannot be encoded in pre-NU6.3 formats.
  - `orchard::bundle::Flags::cross_address_enabled`
  - `orchard::circuit::OrchardCircuitVersion::PostNu6_3`, the circuit version that
    enforces the `disableCrossAddress` public input. The post-NU 6.3 circuit has
    its own proving and verifying keys.
  - `orchard::circuit::OrchardCircuitVersion::supports_cross_address_restriction`,
    and `orchard::circuit::{ProvingKey, VerifyingKey}::supports_cross_address_restriction`,
    introspection for whether a circuit version (or a key's circuit version)
    constrains the `disableCrossAddress` public input.
  - `orchard::circuit::VerifyingKey::circuit_version`
- `orchard::bundle::CommitmentError`, with its `InvalidTransactionVersion` variant,
  returned by the bundle commitment APIs when an Ironwood bundle's commitment is requested
  for a `TxVersion::V5` transaction.
- `orchard::Bundle::bundle_version`, returning the `BundleVersion` the bundle is encoded
  under, and `orchard::Bundle::flag_byte`, the infallible byte encoding of the bundle's
  flags under that version.
- `orchard::bundle::BatchError` (requires the `circuit` feature), with its
  `RestrictionUnsupportedByKey` variant, returned by
  `orchard::bundle::BatchValidator::add_bundle` when a restricted bundle is added
  with a verifying key whose circuit version cannot enforce the cross-address
  restriction.
- Wallet-controlled change-output APIs, the only way to retain shielded value in
  a bundle that disables cross-address transfers:
  - `orchard::builder::ChangeInfo`, the change-output counterpart of `OutputInfo`,
    recording the full viewing key that owns the recipient (validated on
    construction) so the builder can fabricate the paired same-expanded-receiver
    spend.
  - `orchard::builder::Builder::add_change_output`
  - `orchard::builder::Builder::changes`
  - `impl orchard::builder::OutputView for ChangeInfo`
- New builder errors:
  - `orchard::builder::BuildError::{CrossAddressDisabled, InvalidNoteVersion, UnrepresentableFlags, CoinbaseSpendsEnabled}`
  - `orchard::builder::OutputError::{SpendsDisabled, CrossAddressDisabled, RecipientNotOwned}`
- `orchard::builder::BundleType::UNPADDED`, a transactional bundle type that disables
  padding for transactions whose shape is already public (e.g. pool migrations).
- `orchard::builder::Builder::bundle_type`, returning the `BundleType` the builder
  was constructed with.
- Ironwood and note-version APIs:
  - `orchard::NoteVersion`, the note plaintext version selector, with variants
    `V2` (the ZIP 212 Orchard note plaintext format) and `V3` (the
    quantum-recoverable Ironwood note plaintext version defined in ZIP 2005).
  - `orchard::Note::version`, returning the note's plaintext version.
  - `orchard::note_encryption::{NoteEncryptionDomain, DomainVersion, OrchardVersion, IronwoodVersion}`,
    the sealed marker-domain API underlying `OrchardDomain` and `IronwoodDomain`.
  - `orchard::note_encryption::{IronwoodDomain, IronwoodNoteEncryption}`, matching
    `OrchardDomain` note-encryption behavior but accepting V3 note plaintexts
    during parsing.
- PCZT note-version and cross-address APIs:
  - `orchard::pczt::{Spend, Output}::note_version`, the generated getters for the
    note plaintext version of a parsed spend or output.
  - `orchard::pczt::Bundle::bundle_version`, the generated getter for the bundle's
    `BundleVersion`, and `orchard::pczt::Bundle::flag_byte`, the infallible byte encoding of
    its flags under that version.
  - `orchard::pczt::Spend::parse_preverified_for_signing`, a PCZT spend parse
    entry point for a preverified signing pass. It skips FVK derivation and does
    not validate or preserve the wire `fvk`; callers must have already run the
    full Verifier checks over the same PCZT bytes.
  - `orchard::pczt::Bundle::verify_cross_address_restriction`, so that Signers can
    check the cross-address restriction's same-expanded-receiver structural
    property before signing. It is a no-op for bundles that permit cross-address
    transfers.
  - `orchard::pczt::ParseError::InvalidNoteVersion`
  - `orchard::pczt::VerifyError::DisallowedCrossAddressTransfer`
  - `orchard::pczt::IoFinalizerError::CrossAddressRestriction`
  - `orchard::pczt::ProverError::DisallowedCrossAddressTransfer`, wrapping the
    underlying `orchard::pczt::VerifyError`.

### Changed
- Bundle construction now requires an explicit `BundleVersion` and `Flags`:
  - `orchard::builder::Builder::new` now takes
    `(BundleType, BundleVersion, Flags, Anchor)` and returns
    `Result<Builder, BuildError>`; the `Flags` are validated against the
    `BundleVersion` (rejected with `BuildError::UnrepresentableFlags`), and a
    `BundleType::Coinbase` builder requires spends-disabled flags (rejected with
    `BuildError::CoinbaseSpendsEnabled`). `BundleVersion::default_flags` supplies a
    suitable default that the caller may restrict further. The builder derives the
    circuit version from the bundle version rather than from an explicit
    `OrchardCircuitVersion`.
  - `orchard::builder::bundle` now takes a `BundleVersion` and `Flags` in place of
    the circuit-version argument, and takes the wallet-controlled change outputs
    as a separate `changes: Vec<ChangeInfo>` argument (plain `outputs` and
    `changes` are distinct). It rejects supplied `OutputInfo`/`ChangeInfo` values
    whose note version does not match the `BundleVersion`, returning
    `BuildError::InvalidNoteVersion`.
  - `orchard::builder::BundleType` no longer carries any flag settings; it is now
    just the construction policy, `Transactional { bundle_required }` or
    `Coinbase`. The bundle's `Flags` are supplied separately to the builder. `flags`
    are no longer derived from the bundle type, so `BundleType::flags` has been
    removed.
  - `orchard::builder::BundleType::num_actions` now takes a `Flags` (in place of a
    `BundleVersion`), reading the spend/output/cross-address policy directly from it.
    For bundles that disable cross-address transfers, `num_actions` counts
    `num_spends + num_outputs` requested actions (a requested spend and a requested
    output never share an action) rather than the maximum of the two, and
    `BundleMetadata` maps them to distinct actions; wallets estimating fees (e.g. per
    ZIP 317) must account for the larger action count.
  - `orchard::builder::OutputInfo::{new, dummy}` now take an `orchard::NoteVersion`,
    so callers choose between V2 Orchard notes and V3 Ironwood notes;
    builder-created outputs use the note version associated with the selected
    `BundleVersion`.
  - `orchard::builder::BundleMetadata::output_action_index` now indexes the plain
    outputs first, followed by the wallet-controlled change outputs.
- `orchard::builder::BundleType::Transactional` now carries a required
  `pad_to_minimum: Option<u8>` field, so callers that directly construct or
  pattern-match this public enum variant must update their code. When set to
  `Some(1)`, the bundle is not padded to the 2-action minimum and contains
  exactly the requested actions (at least one, if `bundle_required` is set).
  When set to `None`, the default 2-action minimum is used.
  `BundleType::DEFAULT` keeps the existing padded behavior.
- For `BundleVersion::orchard_v3()`, the builder constructs
  withdrawal/change bundles that disable cross-address transfers: every action's
  output is addressed to the expanded receiver of the note it spends. The
  fabricated zero-value output paired with each real spend carries a randomized,
  undecryptable note ciphertext rather than one encrypted to the spent note's
  receiver, so neither the owning wallet nor a holder of that receiver's incoming
  viewing key can use it to detect the spend. Ordinary outputs are rejected
  (`Builder::add_output` returns `OutputError::CrossAddressDisabled`); retained
  shielded value must be added with `Builder::add_change_output`, which rejects a
  recipient not owned by the full viewing key (`OutputError::RecipientNotOwned`)
  and requires spends to be enabled (`OutputError::SpendsDisabled`). The cross-address
  bit is now a caller-supplied flag rather than a builder-chosen default:
  `BundleVersion::default_flags` returns the least-restrictive flag set consensus permits
  — cross-address transfers enabled, except for the Orchard pool under
  `BundleVersion::orchard_v3()`, where consensus mandates the restriction — and a caller
  may restrict it further (a tighter choice the bundle version permits) before passing the
  flags to the builder. Coinbase bundles follow the same constraints as non-coinbase
  bundles: post-NU6.3 Orchard coinbase transactions cannot contain Orchard actions, so
  post-NU6.3 coinbase bundle construction in this crate is only useful for
  `BundleVersion::ironwood_v3()`.
- `orchard::bundle::Flags::{to_byte, from_byte}` now take a
  `BundleVersion`. Bit 2 (`enableCrossAddress`) is only representable for
  the Ironwood pool post-NU6.3; it is rejected for pre-NU6.3 (where bit 2 is
  reserved) and for Orchard post-NU6.3 (where consensus mandates the cross-address
  restriction). `to_byte` now returns `Option<u8>`, yielding `None` when the flag
  set is not representable under the given bundle version. A byte with bit 2
  clear is interpreted differently per epoch: an unrestricted bundle before NU6.3,
  a restricted bundle under NU6.3. A `Bundle` exposes the infallible
  `Bundle::flag_byte` for its own flag encoding.
- A bundle's `BundleVersion` is now carried by the bundle itself rather than passed to its
  decryption, recovery, and commitment methods:
  - `orchard::Bundle::{decrypt_outputs_with_keys, decrypt_output_with_key}` and
    `orchard::Bundle::{recover_outputs_with_ovks, recover_output_with_ovk}` no longer take a
    version argument; they use the bundle's own `BundleVersion` (so an Ironwood bundle's
    helpers discover its V3 notes).
  - `orchard::Bundle::commitment` and
    `orchard::Bundle::<Authorized, V>::authorizing_commitment` no longer take a
    `BundleVersion` (the bundle supplies it); they still take a `TxVersion`, which selects
    the commitment personalization strings and the anchor placement. The ZIP-244 digest —
    and therefore the transaction ID and sighash — depends on both the bundle's version and
    the transaction version: under a NU6.3 protocol an unrestricted bundle's flag byte sets
    bit 2, and `TxVersion::V6` uses the v6 personalization strings and commits the anchor in
    the authorizing commitment instead of the effects commitment. Callers must construct the
    bundle with the version matching the transaction and pass the matching `TxVersion`;
    these APIs check only that the combination is representable, not that it is
    consensus-valid. Both now return only
    `Err(CommitmentError::InvalidTransactionVersion)` (for an Ironwood bundle in a v5
    transaction); because flags are validated when the bundle is constructed, commitment can
    no longer fail on unrepresentable flags.
  - `orchard::bundle::commitments::{hash_bundle_txid_empty, hash_bundle_auth_empty}`, which
    operate on an absent bundle (and so hash no flags), now take a `ValuePool` and a
    `TxVersion`, and return `Result<Blake2bHash, CommitmentError>`, rejecting an Ironwood pool
    in a v5 transaction with `CommitmentError::InvalidTransactionVersion`.
- Bundle construction now takes a `BundleVersion` and validates flags against it:
  - `orchard::Bundle::<EffectsOnly, V>::from_parts` and
    `orchard::Bundle::<Authorized, V>::try_from_parts` now take a `BundleVersion`, and
    reject a flag set that cannot be encoded under it with the new
    `orchard::bundle::BundleError::UnrepresentableFlags` variant. `from_parts` is now
    fallible (returns `Result<_, BundleError>`) for this reason, so a constructed `Bundle`
    is always encodable and committable.
  - `try_from_parts` no longer takes an `orchard::bundle::ProofSizeEnforcement`: the
    canonical proof-size check (GHSA-2x4w-pxqw-58v9) is derived from the bundle version,
    enforced for every version except the historical pre-NU6.2 Orchard pool
    (`BundleVersion::orchard_insecure_v1`), whose already-committed transactions may carry
    non-canonical proofs.
- Circuit APIs now require explicit circuit versions:
  - `orchard::circuit::Circuit::from_action_context` now takes an
    `OrchardCircuitVersion` instead of implicitly selecting `FixedPostNu6_2`.
  - `orchard::circuit::Instance::from_parts` now takes an `orchard::bundle::Flags`
    instead of separate spend/output enable booleans, so the cross-address
    restriction is carried into the public instances.
  - `orchard::circuit::{ProvingKey, VerifyingKey}::build` now take an
    `OrchardCircuitVersion` — pass `FixedPostNu6_2` for the previous behavior, or
    `PostNu6_3` for restricted proofs.
  - `orchard::Proof::{create, verify}` and `orchard::Bundle::verify_proof` reject
    instances that disable cross-address transfers unless the key's circuit
    version supports the restriction, returning
    `halo2_proofs::plonk::Error::InvalidInstances`; with pre-NU 6.3 keys, proving
    a restricted builder-created bundle returns `BuildError::Proof`, and PCZT
    proving returns `ProverError::ProofFailed`. Restricted bundles can still be
    constructed and round-tripped (`Bundle::<Authorized, V>::try_from_parts` and
    `orchard::pczt::Bundle::extract` preserve the flag), with enforcement at
    proving and verification.
- `orchard::bundle::BatchValidator` (requires the `circuit` feature) now binds its
  verifying key at construction: `BatchValidator::new` now takes a
  `&orchard::circuit::VerifyingKey`, and `BatchValidator::validate` no longer takes
  one. `BatchValidator::add_bundle` now returns `Result<(), BatchError>`, rejecting
  a bundle that disables cross-address transfers — without adding it to the batch —
  when the verifying key's circuit version does not support the cross-address
  restriction. Other proof or signature failures still surface as `validate`
  returning `false`.
- `orchard::Note::from_parts` now takes an `orchard::NoteVersion`, so callers
  choose between V2 Orchard notes and V3 Ironwood notes. Orchard note decryption
  domains now reject note plaintext versions that do not match their domain, and
  `orchard::note_encryption::OrchardDomain` is now an alias for
  `NoteEncryptionDomain<OrchardVersion>`. Encryption uses the note's own
  `NoteVersion`; the `OrchardNoteEncryption` and `IronwoodNoteEncryption` aliases
  differ only in which note plaintext versions they accept during parsing and
  decryption.
- PCZT parsing and role checks now carry pool and note-version context:
  - `orchard::pczt::Bundle::parse` now takes a `BundleVersion` and rejects
    flags and output note versions that do not match it.
  - `orchard::pczt::{Spend, Output}::parse` now take the `orchard::NoteVersion` for
    the parsed spend or output.
  - `orchard::pczt::Bundle::create_proof` now builds the Action circuits for the
    provided `ProvingKey`'s circuit version (previously always `FixedPostNu6_2`)
    and checks the cross-address restriction's same-expanded-receiver property,
    returning `ProverError::DisallowedCrossAddressTransfer` (or
    `ProverError::MissingRecipient` if a `recipient` field is unset).
  - `orchard::pczt::Bundle::finalize_io` verifies the cross-address restriction
    before modifying the bundle, returning
    `IoFinalizerError::CrossAddressRestriction` (wrapping the underlying
    `VerifyError`) and leaving the bundle unmodified if the PCZT is missing
    recipient data or violates the restriction.
- `test-dependencies`-only:
  - `orchard::note::testing::arb_note`,
    `orchard::bundle::testing::{arb_action, arb_unauthorized_action}`, and
    `orchard::bundle::testing::{arb_action_n, arb_unauthorized_action_n}` now take
    an `orchard::NoteVersion`.
  - `orchard::bundle::testing::{arb_bundle, arb_unauthorized_bundle}` now construct the
    bundle with a generated `BundleVersion` (via the new
    `orchard::bundle::testing::arb_bundle_version` strategy), choosing flags
    consistent with that version.
  - `orchard::bundle::testing::arb_flags` is unchanged and only generates flag sets
    with cross-address transfers enabled, representable under every bundle
    version other than Orchard post-NU6.3; use the new
    `arb_flags_ironwood_post_nu6_3` for Ironwood post-NU6.3 flag sets that may
    disable cross-address transfers.
- `unstable-voting-circuits`-only (not covered by the crate's semver guarantees):
  - `orchard::Note::{new, dummy}` now take an `orchard::NoteVersion`.
  - `RandomSeed::rcm` is replaced by `RandomSeed::{rcm_v2, rcm_v3}`: `rcm_v2` is the
    rcm derivation for V2 (ZIP 212) notes, and `rcm_v3` the derivation for V3
    (ZIP 2005, Ironwood) notes.

### Removed
- `orchard::bundle::ProofSizeEnforcement`; `Bundle::try_from_parts` now derives the
  canonical proof-size check from the `BundleVersion` (enforced for every version except
  `BundleVersion::orchard_insecure_v1`).
- `orchard::builder::Builder::new_for_version`; use
  `Builder::new(bundle_type, bundle_version, flags, anchor)`.
- `orchard::builder::bundle_for_version`; use `builder::bundle` with
  `BundleVersion` and a `Vec<ChangeInfo>`.
- `orchard::builder::BundleType::DISABLED`; construct the builder with a `Flags` value
  that disables spends and outputs instead.
- Zero-argument `orchard::circuit::{ProvingKey, VerifyingKey}::build`; pass an
  `OrchardCircuitVersion` explicitly.
- `orchard::circuit::{ProvingKey, VerifyingKey}::build_for_version`; use
  `build(version)`.
- `orchard::circuit::Circuit::from_action_context_for_version`; use
  `Circuit::from_action_context(..., circuit_version)`.
- `orchard::Proof::add_to_batch` is no longer public; it could add restricted
  instances to a raw batch later finalized against a verifying key whose circuit
  version does not enforce the cross-address restriction. Use
  `orchard::bundle::BatchValidator`, which binds its verifying key at construction.
- The `Default` implementations for `orchard::circuit::OrchardCircuitVersion`,
  `orchard::circuit::Circuit`, and `orchard::bundle::BatchValidator`; callers must
  choose a circuit version explicitly, and `BatchValidator` must be constructed
  with `BatchValidator::new`, which requires a verifying key.

### Fixed
- The `Display` output of `orchard::builder::BuildError::OutputsDisabled`
  previously described spends rather than outputs.

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
