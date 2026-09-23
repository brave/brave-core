//! Structs related to bundles of Orchard actions.

use alloc::vec::Vec;

pub mod commitments;

#[cfg(feature = "circuit")]
mod batch;
#[cfg(feature = "circuit")]
pub use batch::{BatchError, BatchValidator};

use core::fmt;

use blake2b_simd::Hash as Blake2bHash;
use nonempty::NonEmpty;
use zcash_note_encryption::{try_note_decryption, try_output_recovery_with_ovk};

#[cfg(feature = "std")]
use memuse::DynamicUsage;

use crate::{
    action::Action,
    address::Address,
    bundle::commitments::{hash_bundle_auth_data, hash_bundle_txid_data},
    keys::{IncomingViewingKey, OutgoingViewingKey, PreparedIncomingViewingKey},
    note::{Note, NoteVersion},
    note_encryption::BundleDomain,
    primitives::redpallas::{self, Binding, SpendAuth},
    tree::Anchor,
    value::{ValueCommitTrapdoor, ValueCommitment, ValueSum},
    Proof, ProtocolVersion, ValuePool,
};

#[cfg(feature = "circuit")]
use crate::circuit::{Instance, OrchardCircuitVersion, VerifyingKey};

#[cfg(feature = "circuit")]
impl<T> Action<T> {
    /// Prepares the public instance for this action, for creating and verifying the
    /// bundle proof.
    pub fn to_instance(&self, flags: Flags, anchor: Anchor) -> Instance {
        Instance::from_parts(
            anchor,
            self.cv_net().clone(),
            *self.nullifier(),
            self.rk().clone(),
            *self.cmx(),
            flags,
        )
        .expect("this Action's rk is non-identity by construction (Action::from_parts)")
    }
}

/// A shielded [`ValuePool`] (Orchard or Ironwood) together with the [`ProtocolVersion`]
/// under which a bundle in that pool is built. The Ironwood pool only exists from NU6.3
/// onward, so it is only valid in combination with [`ProtocolVersion::V3`].
///
/// This pins the `circuit_version` and the flag-byte format, and determines whether
/// cross-address transfers are permitted (`permits_cross_address_transfers`).
/// The `flagsOrchard` / `flagsIronwood` value emitted by the [`Builder`](crate::builder::Builder)
/// depends on that constraint. The integration layer uses the pool and consensus branch ID
/// to select the `BundleVersion` value, and threads it through bundle construction
/// and wire encoding.
///
/// This crate has no concept of consensus branches or activation heights, so it can't
/// derive the `BundleVersion` itself. Note that bit 2 of the flags is *reserved*
/// (required to be clear) for v5, and encodes `enableCrossAddress` only for v6. The
/// correct choice of `BundleVersion` is needed to get that right, as well as
/// affecting how requested spends and outputs map to actions. Using the wrong value
/// may result in constructing a consensus-invalid transaction.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[non_exhaustive]
pub struct BundleVersion {
    value_pool: ValuePool,
    protocol_version: ProtocolVersion,
}

impl BundleVersion {
    /// The [`BundleVersion`] for the [`ValuePool::Orchard`] pool under
    /// [`ProtocolVersion::InsecureV1`] (the Orchard pool prior to NU6.2).
    pub const fn orchard_insecure_v1() -> Self {
        Self {
            value_pool: ValuePool::Orchard,
            protocol_version: ProtocolVersion::InsecureV1,
        }
    }

    /// The [`BundleVersion`] for the [`ValuePool::Orchard`] pool under
    /// [`ProtocolVersion::V2`] (the Orchard pool from NU6.2 until NU6.3).
    pub const fn orchard_v2() -> Self {
        Self {
            value_pool: ValuePool::Orchard,
            protocol_version: ProtocolVersion::V2,
        }
    }

    /// The [`BundleVersion`] for the [`ValuePool::Orchard`] pool under
    /// [`ProtocolVersion::V3`] (the Orchard pool at NU6.3 and later).
    pub const fn orchard_v3() -> Self {
        Self {
            value_pool: ValuePool::Orchard,
            protocol_version: ProtocolVersion::V3,
        }
    }

    /// The [`BundleVersion`] for the [`ValuePool::Ironwood`] pool under
    /// [`ProtocolVersion::V3`] (the Ironwood pool, introduced at NU6.3).
    pub const fn ironwood_v3() -> Self {
        Self {
            value_pool: ValuePool::Ironwood,
            protocol_version: ProtocolVersion::V3,
        }
    }

    /// Returns the [`ValuePool`] to which this bundle version applies.
    pub fn value_pool(&self) -> ValuePool {
        self.value_pool
    }

    /// Returns the [`ProtocolVersion`] under which this bundle is built.
    pub fn protocol_version(&self) -> ProtocolVersion {
        self.protocol_version
    }

    /// The circuit version whose proving and verifying keys prove and verify actions consistent
    /// with this bundle version.
    ///
    /// This is many-to-one: both the [`ValuePool::Orchard`] and [`ValuePool::Ironwood`] pools
    /// under [`ProtocolVersion::V3`] share the post-NU6.3 circuit, so build a key with
    /// `ProvingKey::build(bundle_version.circuit_version())` /
    /// `VerifyingKey::build(bundle_version.circuit_version())`.
    #[cfg(feature = "circuit")]
    pub fn circuit_version(&self) -> OrchardCircuitVersion {
        match self.protocol_version {
            ProtocolVersion::InsecureV1 => OrchardCircuitVersion::InsecurePreNu6_2,
            ProtocolVersion::V2 => OrchardCircuitVersion::FixedPostNu6_2,
            ProtocolVersion::V3 => OrchardCircuitVersion::PostNu6_3,
        }
    }

    /// The [`NoteVersion`] associated with this bundle version.
    ///
    /// Orchard pools use V2 note plaintexts, and Ironwood pools use V3 note
    /// plaintexts.
    pub fn note_version(&self) -> NoteVersion {
        match self.value_pool {
            ValuePool::Orchard => NoteVersion::V2,
            ValuePool::Ironwood => NoteVersion::V3,
        }
    }

    /// Whether the consensus rules for this version *permit* cross-address transfers within an
    /// action (`enableCrossAddress = 1` in a v6 transaction).
    ///
    /// Every version permits them except the [`ValuePool::Orchard`] pool under
    /// [`ProtocolVersion::V3`], which mandates the cross-address restriction. This is not
    /// necessarily the same cross-address-enabled decision the
    /// [`Builder`](crate::builder::Builder) makes; that is builder policy chosen within this
    /// constraint.
    pub(crate) fn permits_cross_address_transfers(&self) -> bool {
        !matches!(
            (self.protocol_version, self.value_pool),
            (ProtocolVersion::V3, ValuePool::Orchard)
        )
    }

    /// The default [`Flags`] for a bundle of this version: spends and outputs enabled, with the
    /// cross-address bit set to the least-restrictive value the version permits (enabled unless
    /// the version mandates the restriction).
    ///
    /// This is the prover-side default a builder uses when the caller does not restrict the bundle
    /// further. Where the version leaves the cross-address choice free (e.g. the Ironwood pool), a
    /// caller may instead pass a more restricted flag set such as
    /// [`Flags::CROSS_ADDRESS_DISABLED`]; the chosen flags must be representable under the version.
    pub fn default_flags(&self) -> Flags {
        Flags::from_parts(true, true, self.permits_cross_address_transfers())
    }

    /// Whether an authorized bundle of this version must carry a canonically-sized proof.
    ///
    /// The historical pre-NU6.2 Orchard pool ([`ProtocolVersion::InsecureV1`]) is only used to
    /// parse already-committed transactions, whose proofs cannot be re-canonicalized, so its
    /// proof size is not enforced. Every later version requires a canonical proof, rejecting
    /// non-canonical (e.g. padded) proofs (GHSA-2x4w-pxqw-58v9).
    pub(crate) fn enforces_canonical_proof_size(&self) -> bool {
        !matches!(self.protocol_version, ProtocolVersion::InsecureV1)
    }
}

/// The transaction version a bundle's commitments are computed for.
///
/// An Orchard bundle at NU6.3 and later may be encoded in either a v5 or a v6 transaction.
/// The two formats use different commitment personalization strings and include the bundle's
/// anchor in different digests: v5 includes the anchor in the transaction-ID digest, while v6
/// includes it in the authorizing digest. Ironwood bundles exist only in v6 transactions, so
/// attempting to compute an Ironwood commitment for a v5 transaction returns an error.
///
/// This is independent of the [`BundleVersion`] that governs construction: the same
/// Orchard bundle can be committed under either transaction version, and the caller must pass the one
/// matching the transaction the bundle is encoded in. See [`Bundle::commitment`] and
/// [`Bundle::authorizing_commitment`].
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[non_exhaustive]
pub enum TxVersion {
    /// A v5 transaction.
    V5,
    /// A v6 transaction.
    V6,
}

/// Flags denoting what operations may be performed by the Orchard actions
/// in a bundle.
///
/// `Flags` are version-agnostic: a given flag set is not inherently valid for any
/// particular [`BundleVersion`]. Whether it can be used with a version is checked
/// separately, when it is encoded for that version (see [`Flags::to_byte`]) or supplied
/// to the [`Builder`](crate::builder::Builder).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Flags {
    /// Flag denoting whether Orchard spends are enabled in the transaction.
    ///
    /// If `false`, spent notes within [`Action`]s in the transaction's [`Bundle`] are
    /// guaranteed to be dummy notes. If `true`, the spent notes may be either real or
    /// dummy notes.
    spends_enabled: bool,
    /// Flag denoting whether Orchard outputs are enabled in the transaction.
    ///
    /// If `false`, created notes within [`Action`]s in the transaction's [`Bundle`] are
    /// guaranteed to be dummy notes. If `true`, the created notes may be either real or
    /// dummy notes.
    outputs_enabled: bool,
    /// Flag denoting whether Orchard spends and outputs may use different expanded
    /// receivers.
    ///
    /// If `false`, every action's output is constrained to be addressed to the same
    /// expanded receiver as the note it spends; proving and verification must reject the
    /// bundle unless they use a circuit key that supports the restriction.
    cross_address_enabled: bool,
}

const FLAG_SPENDS_ENABLED: u8 = 0b0000_0001;
const FLAG_OUTPUTS_ENABLED: u8 = 0b0000_0010;
const FLAG_V6_CROSS_ADDRESS_ENABLED: u8 = 0b0000_0100;
const FLAGS_ALWAYS_EXPECTED_UNSET: u8 =
    !(FLAG_SPENDS_ENABLED | FLAG_OUTPUTS_ENABLED | FLAG_V6_CROSS_ADDRESS_ENABLED);

impl Flags {
    /// Construct a set of flags from its constituent parts, including the cross-address bit.
    ///
    /// Crate-internal: the builder supplies `cross_address_enabled` from its prover-side default
    /// for the bundle version (see [`Builder`](crate::builder::Builder)).
    pub(crate) const fn from_parts(
        spends_enabled: bool,
        outputs_enabled: bool,
        cross_address_enabled: bool,
    ) -> Self {
        Flags {
            spends_enabled,
            outputs_enabled,
            cross_address_enabled,
        }
    }

    /// The flag set for an unrestricted bundle: spends and outputs are both
    /// enabled, so its actions may spend and create real notes addressed to any
    /// expanded receivers.
    ///
    /// Like [`Self::SPENDS_DISABLED`] and [`Self::OUTPUTS_DISABLED`], this leaves
    /// cross-address transfers enabled; see [`Self::CROSS_ADDRESS_DISABLED`] for
    /// the restricted variant.
    pub const ENABLED: Flags = Flags {
        spends_enabled: true,
        outputs_enabled: true,
        cross_address_enabled: true,
    };

    /// The flag set for a bundle that may create notes but not spend them: every
    /// spent note is constrained to be a dummy, while outputs may be real.
    ///
    /// This is the flag set used by coinbase transactions, which mint value into
    /// the Orchard pool without consuming existing notes.
    pub const SPENDS_DISABLED: Flags = Flags {
        spends_enabled: false,
        outputs_enabled: true,
        cross_address_enabled: true,
    };

    /// The flag set for a bundle that may spend notes but not create them: every
    /// created note is constrained to be a dummy, while spends may be real.
    ///
    /// This is used to remove value from the Orchard pool without producing new
    /// notes within the bundle.
    pub const OUTPUTS_DISABLED: Flags = Flags {
        spends_enabled: true,
        outputs_enabled: false,
        cross_address_enabled: true,
    };

    /// The flag set with spends and outputs enabled and cross-address transfers disabled.
    ///
    /// This flag set cannot be encoded in pre-NU6.3 formats. Proof creation and
    /// verification for instances built with this flag require a post-NU 6.3 circuit key.
    pub const CROSS_ADDRESS_DISABLED: Flags = Flags {
        spends_enabled: true,
        outputs_enabled: true,
        cross_address_enabled: false,
    };

    /// Flag denoting whether Orchard spends are enabled in the transaction.
    ///
    /// If `false`, spent notes within [`Action`]s in the transaction's [`Bundle`] are
    /// guaranteed to be dummy notes. If `true`, the spent notes may be either real or
    /// dummy notes.
    pub fn spends_enabled(&self) -> bool {
        self.spends_enabled
    }

    /// Flag denoting whether Orchard outputs are enabled in the transaction.
    ///
    /// If `false`, created notes within [`Action`]s in the transaction's [`Bundle`] are
    /// guaranteed to be dummy notes. If `true`, the created notes may be either real or
    /// dummy notes.
    pub fn outputs_enabled(&self) -> bool {
        self.outputs_enabled
    }

    /// Flag denoting whether Orchard spends and outputs may use different expanded
    /// receivers.
    ///
    /// If `false`, every action's output is constrained to be addressed to the same
    /// expanded receiver as the note it spends; proving and verification must reject the
    /// bundle unless they use a circuit key that supports the restriction.
    pub fn cross_address_enabled(&self) -> bool {
        self.cross_address_enabled
    }

    /// Serialize flags to a byte as defined in [Zcash Protocol Spec § 7.1: Transaction
    /// Encoding And Consensus][txencoding].
    ///
    /// [txencoding]: https://zips.z.cash/protocol/protocol.pdf#txnencoding
    pub fn to_byte(&self, bundle_version: BundleVersion) -> Option<u8> {
        let mut value = 0u8;
        if self.spends_enabled {
            value |= FLAG_SPENDS_ENABLED;
        }
        if self.outputs_enabled {
            value |= FLAG_OUTPUTS_ENABLED;
        }

        // Validate `cross_address_enabled` against what the value pool and protocol version can
        // represent, and set the v6 flag bit where it carries that choice. A flag set whose
        // `cross_address_enabled` value is not representable in this context cannot be encoded.
        match (bundle_version.value_pool, bundle_version.protocol_version) {
            // Cross-address Orchard pool transfers are always permitted prior to
            // ProtocolVersion::V3; there is no flag bit, so a disabled flag set cannot be
            // represented.
            (ValuePool::Orchard, ProtocolVersion::InsecureV1 | ProtocolVersion::V2) => {
                if !self.cross_address_enabled {
                    return None;
                }
            }
            // Cross-address Orchard pool transfers are disallowed in ProtocolVersion::V3.
            (ValuePool::Orchard, ProtocolVersion::V3) => {
                if self.cross_address_enabled {
                    return None;
                }
            }
            // The Ironwood pool encodes the caller's choice in bit 2.
            (ValuePool::Ironwood, ProtocolVersion::V3) => {
                if self.cross_address_enabled {
                    value |= FLAG_V6_CROSS_ADDRESS_ENABLED;
                }
            }
            // The Ironwood pool is not defined prior to ProtocolVersion::V3.
            (ValuePool::Ironwood, _) => return None,
        }

        Some(value)
    }

    /// Parses flags from a single byte as defined in [Zcash Protocol Spec §
    /// 7.1: Transaction Encoding And Consensus][txencoding], according to the
    /// interpretation implied by `bundle_version`. Returns `None` if
    /// unexpected bits are set in the flag byte.
    ///
    /// The protocol specification and ZIPs 225 and 229 define bit 2 of `flags`
    /// to be reserved in v5 transactions, and to encode the
    /// `enableCrossAddress` flag in v6 transactions. However, we can (by
    /// design) parse and validate the flags knowing only `bundle_version`:
    /// bit 2 can only be 1 for a bundle in the [`ValuePool::Ironwood`] pool, and
    /// otherwise MUST be 0. Assuming that has been checked, cross-address
    /// transfers are always enabled prior to [`ProtocolVersion::V3`], and under
    /// [`ProtocolVersion::V3`] are taken to be enabled exactly when bit 2 is set.
    ///
    /// Note: if the wrong value of `bundle_version` is passed for the actual
    /// pool and epoch of the transaction, then a consensus-invalid transaction
    /// may be constructed (see [`BundleVersion`]).
    ///
    /// [txencoding]: https://zips.z.cash/protocol/protocol.pdf#txnencoding
    pub fn from_byte(value: u8, bundle_version: BundleVersion) -> Option<Self> {
        // Bits 3..=7 are always reserved and MUST be 0.
        // https://p.z.cash/TCR:bad-txns-v5-reserved-bits-nonzero
        if value & FLAGS_ALWAYS_EXPECTED_UNSET != 0 {
            return None;
        }

        // Bit 2 can only be 1 for an Ironwood bundle. For Orchard it MUST be 0, independent of the
        // tx version:
        //
        // * for a v5 transaction it is still reserved and MUST be 0;
        // * for a v6+ transaction it encodes `enableCrossAddress` and MUST be 0.
        //
        // https://p.z.cash/TCR:bad-txns-v5-reserved-bits-nonzero
        let bit2 = value & FLAG_V6_CROSS_ADDRESS_ENABLED != 0;
        if bit2 && bundle_version.value_pool == ValuePool::Orchard {
            return None;
        }

        // We have already validated bit2 against the pool type
        let cross_address_enabled = match bundle_version.protocol_version {
            ProtocolVersion::InsecureV1 | ProtocolVersion::V2 => true,
            ProtocolVersion::V3 => bit2,
        };
        Some(Self {
            spends_enabled: value & FLAG_SPENDS_ENABLED != 0,
            outputs_enabled: value & FLAG_OUTPUTS_ENABLED != 0,
            cross_address_enabled,
        })
    }
}

/// Defines the authorization type of an Orchard bundle.
pub trait Authorization: fmt::Debug {
    /// The authorization type of an Orchard action.
    type SpendAuth: fmt::Debug;
}

/// A bundle of actions to be applied to the ledger.
#[derive(Clone)]
pub struct Bundle<T: Authorization, V> {
    /// The list of actions that make up this bundle.
    actions: NonEmpty<Action<T::SpendAuth>>,
    /// Orchard-specific transaction-level flags for this bundle.
    flags: Flags,
    /// The net value moved out of the Orchard shielded pool.
    ///
    /// This is the sum of Orchard spends minus the sum of Orchard outputs.
    value_balance: V,
    /// The root of the Orchard commitment tree that this bundle commits to.
    anchor: Anchor,
    /// The authorization for this bundle.
    authorization: T,
    /// The value pool and protocol version this bundle is encoded under.
    ///
    /// This is interpretive context rather than wire data: it is never serialized, but it
    /// determines how the bundle's flags are encoded and which commitment format applies. A
    /// `Bundle` is only ever constructed with flags that are representable under this version
    /// (see [`Bundle::try_from_parts`] / [`Bundle::from_parts`]), so the bundle is safe to
    /// serialize and commit to without supplying — and possibly mismatching — a version.
    bundle_version: BundleVersion,
}

impl<T: Authorization, V: fmt::Debug> fmt::Debug for Bundle<T, V> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        /// Helper struct for debug-printing actions without exposing `NonEmpty`.
        struct Actions<'a, T>(&'a NonEmpty<Action<T>>);
        impl<T: fmt::Debug> fmt::Debug for Actions<'_, T> {
            fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
                f.debug_list().entries(self.0.iter()).finish()
            }
        }

        f.debug_struct("Bundle")
            .field("actions", &Actions(&self.actions))
            .field("flags", &self.flags)
            .field("value_balance", &self.value_balance)
            .field("anchor", &self.anchor)
            .field("authorization", &self.authorization)
            .field("bundle_version", &self.bundle_version)
            .finish()
    }
}

/// Checks that `proof` has the canonical length for a bundle of `num_actions` actions.
///
/// Returns [`BundleError::NonCanonicalProofSize`] if it does not. This is the shared check
/// used by the proof-carrying bundle constructors to reject non-canonical (e.g. padded)
/// proofs; see [`Bundle::try_from_parts`] (GHSA-2x4w-pxqw-58v9).
pub(crate) fn validate_proof_size(proof: &Proof, num_actions: usize) -> Result<(), BundleError> {
    let expected = Proof::expected_proof_size(num_actions);
    let actual = proof.as_ref().len();
    if actual == expected {
        Ok(())
    } else {
        Err(BundleError::NonCanonicalProofSize { expected, actual })
    }
}

/// Checks that `flags` can be encoded under `bundle_version`.
///
/// Returns [`BundleError::UnrepresentableFlags`] if it cannot. This is the shared check used by
/// the checked bundle constructors so that a constructed `Bundle` is always encodable and
/// committable under the version it carries.
fn validate_flags(flags: &Flags, bundle_version: BundleVersion) -> Result<(), BundleError> {
    if flags.to_byte(bundle_version).is_some() {
        Ok(())
    } else {
        Err(BundleError::UnrepresentableFlags)
    }
}

impl<T: Authorization, V> Bundle<T, V> {
    /// Constructs a `Bundle` from its constituent parts without validating the authorization.
    ///
    /// This does not check the proof size, so it must only be used with an authorization that
    /// either carries no proof or carries a proof that is already known to be canonical (e.g.
    /// one produced by [`Proof::create`]). Construction from untrusted parts must instead go
    /// through a checked, authorization-specific constructor such as [`Bundle::try_from_parts`].
    ///
    /// `flags` must be representable under `bundle_version`. Every `Bundle` upholds this, so that
    /// [`Bundle::flag_byte`] and the commitment APIs cannot fail; callers are responsible for the
    /// guarantee (it is debug-asserted here).
    pub(crate) fn from_parts_unchecked(
        actions: NonEmpty<Action<T::SpendAuth>>,
        flags: Flags,
        value_balance: V,
        anchor: Anchor,
        authorization: T,
        bundle_version: BundleVersion,
    ) -> Self {
        debug_assert!(flags.to_byte(bundle_version).is_some());
        Bundle {
            actions,
            flags,
            value_balance,
            anchor,
            authorization,
            bundle_version,
        }
    }

    /// Returns the list of actions that make up this bundle.
    pub fn actions(&self) -> &NonEmpty<Action<T::SpendAuth>> {
        &self.actions
    }

    /// Returns the Orchard-specific transaction-level flags for this bundle.
    pub fn flags(&self) -> &Flags {
        &self.flags
    }

    /// Returns the net value moved into or out of the Orchard shielded pool.
    ///
    /// This is the sum of Orchard spends minus the sum Orchard outputs.
    pub fn value_balance(&self) -> &V {
        &self.value_balance
    }

    /// Returns the root of the Orchard commitment tree that this bundle commits to.
    pub fn anchor(&self) -> &Anchor {
        &self.anchor
    }

    /// Returns the authorization for this bundle.
    ///
    /// In the case of a `Bundle<Authorized>`, this is the proof and binding signature.
    pub fn authorization(&self) -> &T {
        &self.authorization
    }

    /// Returns the [`BundleVersion`] (value pool and protocol version) this bundle is encoded
    /// under.
    pub fn bundle_version(&self) -> BundleVersion {
        self.bundle_version
    }

    /// Returns the byte encoding of this bundle's flags, as defined in [Zcash Protocol Spec §
    /// 7.1: Transaction Encoding And Consensus][txencoding], under the bundle's own
    /// [`BundleVersion`].
    ///
    /// Unlike [`Flags::to_byte`], this is infallible: a `Bundle` is only ever constructed with
    /// flags that are representable under its version.
    ///
    /// [txencoding]: https://zips.z.cash/protocol/protocol.pdf#txnencoding
    pub fn flag_byte(&self) -> u8 {
        self.flags
            .to_byte(self.bundle_version)
            .expect("flags are validated against the bundle version at construction")
    }

    /// Construct a new bundle by applying a transformation that might fail
    /// to the value balance.
    pub fn try_map_value_balance<V0, E, F: FnOnce(V) -> Result<V0, E>>(
        self,
        f: F,
    ) -> Result<Bundle<T, V0>, E> {
        Ok(Bundle {
            actions: self.actions,
            flags: self.flags,
            value_balance: f(self.value_balance)?,
            anchor: self.anchor,
            authorization: self.authorization,
            bundle_version: self.bundle_version,
        })
    }

    /// Transitions this bundle from one authorization state to another.
    pub fn map_authorization<R, U: Authorization>(
        self,
        context: &mut R,
        mut spend_auth: impl FnMut(&mut R, &T, T::SpendAuth) -> U::SpendAuth,
        step: impl FnOnce(&mut R, T) -> U,
    ) -> Bundle<U, V> {
        let authorization = self.authorization;
        Bundle {
            actions: self
                .actions
                .map(|a| a.map(|a_auth| spend_auth(context, &authorization, a_auth))),
            flags: self.flags,
            value_balance: self.value_balance,
            anchor: self.anchor,
            authorization: step(context, authorization),
            bundle_version: self.bundle_version,
        }
    }

    /// Transitions this bundle from one authorization state to another.
    pub fn try_map_authorization<R, U: Authorization, E>(
        self,
        context: &mut R,
        mut spend_auth: impl FnMut(&mut R, &T, T::SpendAuth) -> Result<U::SpendAuth, E>,
        step: impl FnOnce(&mut R, T) -> Result<U, E>,
    ) -> Result<Bundle<U, V>, E> {
        let authorization = self.authorization;
        let new_actions = self
            .actions
            .into_iter()
            .map(|a| a.try_map(|a_auth| spend_auth(context, &authorization, a_auth)))
            .collect::<Result<Vec<_>, E>>()?;

        Ok(Bundle {
            actions: NonEmpty::from_vec(new_actions).unwrap(),
            flags: self.flags,
            value_balance: self.value_balance,
            anchor: self.anchor,
            authorization: step(context, authorization)?,
            bundle_version: self.bundle_version,
        })
    }

    #[cfg(feature = "circuit")]
    pub(crate) fn to_instances(&self) -> Vec<Instance> {
        self.actions
            .iter()
            .map(|a| a.to_instance(self.flags, self.anchor))
            .collect()
    }

    /// Performs trial decryption of each action in the bundle with each of the
    /// specified incoming viewing keys, and returns a vector of each decrypted
    /// note plaintext contents along with the index of the action from which it
    /// was derived.
    pub fn decrypt_outputs_with_keys(
        &self,
        keys: &[IncomingViewingKey],
    ) -> Vec<(usize, IncomingViewingKey, Note, Address, [u8; 512])> {
        let prepared_keys: Vec<_> = keys
            .iter()
            .map(|ivk| (ivk, PreparedIncomingViewingKey::new(ivk)))
            .collect();
        self.actions
            .iter()
            .enumerate()
            .filter_map(|(idx, action)| {
                let domain = BundleDomain::for_action(action, self.bundle_version.note_version());
                prepared_keys.iter().find_map(|(ivk, prepared_ivk)| {
                    try_note_decryption(&domain, prepared_ivk, action)
                        .map(|(n, a, m)| (idx, (*ivk).clone(), n, a, m))
                })
            })
            .collect()
    }

    /// Performs trial decryption of the action at `action_idx` in the bundle
    /// with the specified incoming viewing key, and returns the decrypted note
    /// plaintext contents if successful.
    pub fn decrypt_output_with_key(
        &self,
        action_idx: usize,
        key: &IncomingViewingKey,
    ) -> Option<(Note, Address, [u8; 512])> {
        let prepared_ivk = PreparedIncomingViewingKey::new(key);
        self.actions.get(action_idx).and_then(move |action| {
            let domain = BundleDomain::for_action(action, self.bundle_version.note_version());
            try_note_decryption(&domain, &prepared_ivk, action)
        })
    }

    /// Performs trial decryption of each action in the bundle with each of the
    /// specified outgoing viewing keys, and returns a vector of each decrypted
    /// note plaintext contents along with the index of the action from which it
    /// was derived.
    pub fn recover_outputs_with_ovks(
        &self,
        keys: &[OutgoingViewingKey],
    ) -> Vec<(usize, OutgoingViewingKey, Note, Address, [u8; 512])> {
        self.actions
            .iter()
            .enumerate()
            .filter_map(|(idx, action)| {
                let domain = BundleDomain::for_action(action, self.bundle_version.note_version());
                keys.iter().find_map(move |key| {
                    try_output_recovery_with_ovk(
                        &domain,
                        key,
                        action,
                        action.cv_net(),
                        &action.encrypted_note().out_ciphertext,
                    )
                    .map(|(n, a, m)| (idx, key.clone(), n, a, m))
                })
            })
            .collect()
    }

    /// Attempts to decrypt the action at the specified index with the specified
    /// outgoing viewing key, and returns the decrypted note plaintext contents
    /// if successful.
    pub fn recover_output_with_ovk(
        &self,
        action_idx: usize,
        key: &OutgoingViewingKey,
    ) -> Option<(Note, Address, [u8; 512])> {
        self.actions.get(action_idx).and_then(move |action| {
            let domain = BundleDomain::for_action(action, self.bundle_version.note_version());
            try_output_recovery_with_ovk(
                &domain,
                key,
                action,
                action.cv_net(),
                &action.encrypted_note().out_ciphertext,
            )
        })
    }
}

impl<T: Authorization, V: Copy + Into<i64>> Bundle<T, V> {
    /// Computes this bundle's transaction-ID commitment component.
    ///
    /// The flag-byte encoding follows the bundle's own [`BundleVersion`]; `tx_version` selects the
    /// commitment personalizations and whether the bundle anchor bytes are included here or in the
    /// authorizing digest. In a v5 transaction the anchor bytes are included here; in a v6
    /// transaction they are included by [`Bundle::authorizing_commitment`] instead.
    ///
    /// # Errors
    ///
    /// Returns [`CommitmentError::InvalidTransactionVersion`] if `tx_version` is not valid for the
    /// bundle's [`BundleVersion`] (e.g. an Ironwood bundle committed with [`TxVersion::V5`]).
    pub fn commitment(&self, tx_version: TxVersion) -> Result<BundleCommitment, CommitmentError> {
        hash_bundle_txid_data(self, tx_version).map(BundleCommitment)
    }

    /// Returns the transaction binding validating key for this bundle.
    ///
    /// This can be used to validate the [`Authorized::binding_signature`] returned from
    /// [`Bundle::authorization`].
    pub fn binding_validating_key(&self) -> redpallas::VerificationKey<Binding> {
        // https://p.z.cash/TCR:bad-txns-orchard-binding-signature-invalid?partial
        (self
            .actions
            .iter()
            .map(|a| a.cv_net())
            .sum::<ValueCommitment>()
            - ValueCommitment::derive(
                ValueSum::from_raw(self.value_balance.into()),
                ValueCommitTrapdoor::zero(),
            ))
        .into_bvk()
    }
}

/// Marker type for a bundle that contains no authorizing data.
#[derive(Clone, Debug)]
pub struct EffectsOnly;

impl Authorization for EffectsOnly {
    type SpendAuth = ();
}

impl<V> Bundle<EffectsOnly, V> {
    /// Constructs an effects-only `Bundle` from its constituent parts.
    ///
    /// An effects-only bundle carries no proof, so there is no proof size to validate, and flags
    /// are not checked against circuit support (there is no proof key to check against). The flags
    /// are, however, checked for representability under `bundle_version`, so that the resulting
    /// bundle is safe to serialize and commit to.
    ///
    /// # Errors
    ///
    /// Returns [`BundleError::UnrepresentableFlags`] if `flags` cannot be encoded under
    /// `bundle_version`.
    pub fn from_parts(
        actions: NonEmpty<Action<<EffectsOnly as Authorization>::SpendAuth>>,
        flags: Flags,
        value_balance: V,
        anchor: Anchor,
        authorization: EffectsOnly,
        bundle_version: BundleVersion,
    ) -> Result<Self, BundleError> {
        validate_flags(&flags, bundle_version)?;
        Ok(Bundle::from_parts_unchecked(
            actions,
            flags,
            value_balance,
            anchor,
            authorization,
            bundle_version,
        ))
    }
}

/// Authorizing data for a bundle of actions, ready to be committed to the ledger.
#[derive(Debug, Clone)]
pub struct Authorized {
    proof: Proof,
    binding_signature: redpallas::Signature<Binding>,
}

impl Authorization for Authorized {
    type SpendAuth = redpallas::Signature<SpendAuth>;
}

impl Authorized {
    /// Constructs the authorizing data for a bundle of actions from its constituent parts.
    pub fn from_parts(proof: Proof, binding_signature: redpallas::Signature<Binding>) -> Self {
        Authorized {
            proof,
            binding_signature,
        }
    }

    /// Return the proof component of the authorizing data.
    pub fn proof(&self) -> &Proof {
        &self.proof
    }

    /// Return the binding signature.
    pub fn binding_signature(&self) -> &redpallas::Signature<Binding> {
        &self.binding_signature
    }
}

/// Errors that can occur when constructing an authorized [`Bundle`] from untrusted parts.
#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub enum BundleError {
    /// The proof does not have the canonical length for the bundle's number of actions.
    ///
    /// A valid Orchard proof authorizing `n` actions is always exactly
    /// [`Proof::expected_proof_size(n)`] bytes; any other length indicates a non-canonical
    /// encoding, such as a proof padded with arbitrary trailing data.
    ///
    /// [`Proof::expected_proof_size(n)`]: crate::Proof::expected_proof_size
    NonCanonicalProofSize {
        /// The canonical proof length for the bundle's number of actions.
        expected: usize,
        /// The length of the proof that was provided.
        actual: usize,
    },
    /// The bundle's flags cannot be encoded under its [`BundleVersion`]. This happens in two
    /// cases:
    ///
    /// * cross-address transfers are disabled but the version specifies a pre-NU6.3 Orchard pool
    ///   (where cross-address transfers are implicitly enabled);
    /// * cross-address transfers are enabled but the version specifies a post-NU6.3 Orchard pool
    ///   (where cross-address transfers are forbidden).
    UnrepresentableFlags,
}

impl fmt::Display for BundleError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            BundleError::NonCanonicalProofSize { expected, actual } => write!(
                f,
                "Orchard proof has non-canonical length {actual}; expected {expected} bytes",
            ),
            BundleError::UnrepresentableFlags => write!(
                f,
                "bundle flags are not representable under the bundle's value pool and protocol version",
            ),
        }
    }
}

impl core::error::Error for BundleError {}

/// Errors that can occur when computing a [`Bundle`] commitment.
#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub enum CommitmentError {
    /// The requested transaction version is not valid for the bundle's [`BundleVersion`].
    ///
    /// Ironwood bundles exist only in v6 transactions, so an Ironwood bundle
    /// (`BundleVersion::ironwood_v3()`) cannot be committed with `TxVersion::V5`.
    InvalidTransactionVersion,
}

impl fmt::Display for CommitmentError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            CommitmentError::InvalidTransactionVersion => write!(
                f,
                "Ironwood bundles can only be committed in a v6 transaction",
            ),
        }
    }
}

impl core::error::Error for CommitmentError {}

impl<V> Bundle<Authorized, V> {
    /// Constructs an authorized `Bundle` from its constituent parts.
    ///
    /// This is the only constructor for an authorized bundle. For every version except the
    /// historical pre-NU6.2 Orchard pool ([`BundleVersion::orchard_insecure_v1`]) it
    /// validates that the proof has exactly [`Proof::expected_proof_size`] bytes for
    /// `actions.len()`, so such an authorized bundle can never hold a non-canonical proof. This
    /// matters when building a bundle from untrusted input (e.g. deserializing from bytes), as it
    /// prevents a proof from being padded with arbitrary data, which would otherwise impose
    /// unbounded bandwidth and storage costs without affecting proof validity
    /// (GHSA-2x4w-pxqw-58v9). Circuit-key support for the bundle flags is checked when proving or
    /// verifying the proof.
    ///
    /// The flags are also checked for representability under `bundle_version`, so that the
    /// resulting bundle is safe to serialize and commit to.
    ///
    /// # Errors
    ///
    /// Returns [`BundleError::NonCanonicalProofSize`] if the proof length is not canonical (for a
    /// version that enforces it), or [`BundleError::UnrepresentableFlags`] if `flags` cannot be
    /// encoded under `bundle_version`.
    pub fn try_from_parts(
        actions: NonEmpty<Action<<Authorized as Authorization>::SpendAuth>>,
        flags: Flags,
        value_balance: V,
        anchor: Anchor,
        authorization: Authorized,
        bundle_version: BundleVersion,
    ) -> Result<Self, BundleError> {
        if bundle_version.enforces_canonical_proof_size() {
            validate_proof_size(authorization.proof(), actions.len())?;
        }
        validate_flags(&flags, bundle_version)?;
        Ok(Bundle::from_parts_unchecked(
            actions,
            flags,
            value_balance,
            anchor,
            authorization,
            bundle_version,
        ))
    }

    /// Computes the authorizing-data commitment for this bundle.
    ///
    /// This together with `Bundle::commitment` binds the entire bundle. The bundle's own
    /// [`BundleVersion`] and `tx_version` select the commitment personalization; in a v6
    /// transaction this digest also includes the bundle anchor bytes (in a v5 transaction they are
    /// included by [`Bundle::commitment`] instead).
    ///
    /// # Errors
    ///
    /// Returns [`CommitmentError::InvalidTransactionVersion`] if `tx_version` is not valid for the
    /// bundle's [`BundleVersion`].
    pub fn authorizing_commitment(
        &self,
        tx_version: TxVersion,
    ) -> Result<BundleAuthorizingCommitment, CommitmentError> {
        hash_bundle_auth_data(self, tx_version).map(BundleAuthorizingCommitment)
    }

    /// Verifies the proof for this bundle.
    ///
    /// # Errors
    ///
    /// Returns `Err(`[`halo2_proofs::plonk::Error::InvalidInstances`]`)` if this
    /// bundle disables cross-address transfers and `vk` is not an
    /// [`OrchardCircuitVersion::PostNu6_3`] verifying key.
    ///
    /// Also returns an error if proof verification fails.
    ///
    /// [`OrchardCircuitVersion::PostNu6_3`]: crate::circuit::OrchardCircuitVersion::PostNu6_3
    #[cfg(feature = "circuit")]
    pub fn verify_proof(&self, vk: &VerifyingKey) -> Result<(), halo2_proofs::plonk::Error> {
        self.authorization()
            .proof()
            .verify(vk, &self.to_instances())
    }
}

#[cfg(feature = "std")]
impl<V: DynamicUsage> DynamicUsage for Bundle<Authorized, V> {
    fn dynamic_usage(&self) -> usize {
        self.actions.tail.dynamic_usage()
            + self.value_balance.dynamic_usage()
            + self.authorization.proof.dynamic_usage()
    }

    fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
        let bounds = (
            self.actions.tail.dynamic_usage_bounds(),
            self.value_balance.dynamic_usage_bounds(),
            self.authorization.proof.dynamic_usage_bounds(),
        );
        (
            bounds.0 .0 + bounds.1 .0 + bounds.2 .0,
            bounds
                .0
                 .1
                .zip(bounds.1 .1)
                .zip(bounds.2 .1)
                .map(|((a, b), c)| a + b + c),
        )
    }
}

/// A commitment to a bundle of actions.
///
/// This commitment is non-malleable, in the sense that a bundle's commitment will only
/// change if the effects of the bundle are altered.
#[derive(Debug)]
pub struct BundleCommitment(pub Blake2bHash);

impl From<BundleCommitment> for [u8; 32] {
    fn from(commitment: BundleCommitment) -> Self {
        // The commitment uses BLAKE2b-256.
        commitment.0.as_bytes().try_into().unwrap()
    }
}

/// A commitment to the authorizing data within a bundle of actions.
#[derive(Debug)]
pub struct BundleAuthorizingCommitment(pub Blake2bHash);

/// Generators for property testing.
#[cfg(any(test, feature = "test-dependencies"))]
#[cfg_attr(docsrs, doc(cfg(feature = "test-dependencies")))]
pub mod testing {
    use alloc::vec::Vec;

    use group::ff::FromUniformBytes;
    use nonempty::NonEmpty;
    use pasta_curves::pallas;
    use rand::{rngs::StdRng, SeedableRng};
    use reddsa::orchard::SpendAuth;

    use proptest::collection::vec;
    use proptest::prelude::*;

    use crate::{
        bundle::BundleVersion,
        primitives::redpallas::{self, testing::arb_binding_signing_key},
        value::{testing::arb_note_value_bounded, NoteValue, ValueSum, MAX_NOTE_VALUE},
        Anchor, NoteVersion, Proof,
    };

    use super::{Action, Authorized, Bundle, Flags};

    pub use crate::action::testing::{arb_action, arb_unauthorized_action};

    /// Marker type for a bundle that contains no authorizing data.
    pub type Unauthorized = super::EffectsOnly;

    /// Create an arbitrary [`BundleVersion`].
    pub fn arb_bundle_version() -> impl Strategy<Value = BundleVersion> {
        prop_oneof![
            Just(BundleVersion::orchard_insecure_v1()),
            Just(BundleVersion::orchard_v2()),
            Just(BundleVersion::orchard_v3()),
            Just(BundleVersion::ironwood_v3()),
        ]
    }

    /// Returns `flags` with its `cross_address_enabled` bit forced to the value representable under
    /// `bundle_version`, leaving the spend/output bits untouched.
    ///
    /// The arbitrary-bundle strategies generate flags independently of the version; this pairs them
    /// into a combination that a `Bundle` can actually be constructed from.
    fn flags_for_version(bundle_version: BundleVersion, flags: Flags) -> Flags {
        Flags::from_parts(
            flags.spends_enabled(),
            flags.outputs_enabled(),
            bundle_version.permits_cross_address_transfers(),
        )
    }

    /// Generate an unauthorized action having spend and output values less than MAX_NOTE_VALUE / n_actions.
    pub fn arb_unauthorized_action_n(
        note_version: NoteVersion,
        n_actions: usize,
        flags: Flags,
    ) -> impl Strategy<Value = (ValueSum, Action<()>)> {
        let spend_value_gen = if flags.spends_enabled {
            Strategy::boxed(arb_note_value_bounded(MAX_NOTE_VALUE / n_actions as u64))
        } else {
            Strategy::boxed(Just(NoteValue::ZERO))
        };

        spend_value_gen.prop_flat_map(move |spend_value| {
            let output_value_gen = if flags.outputs_enabled {
                Strategy::boxed(arb_note_value_bounded(MAX_NOTE_VALUE / n_actions as u64))
            } else {
                Strategy::boxed(Just(NoteValue::ZERO))
            };

            output_value_gen.prop_flat_map(move |output_value| {
                arb_unauthorized_action(note_version, spend_value, output_value)
                    .prop_map(move |a| (spend_value - output_value, a))
            })
        })
    }

    /// Generate an authorized action having spend and output values less than MAX_NOTE_VALUE / n_actions.
    pub fn arb_action_n(
        note_version: NoteVersion,
        n_actions: usize,
        flags: Flags,
    ) -> impl Strategy<Value = (ValueSum, Action<redpallas::Signature<SpendAuth>>)> {
        let spend_value_gen = if flags.spends_enabled {
            Strategy::boxed(arb_note_value_bounded(MAX_NOTE_VALUE / n_actions as u64))
        } else {
            Strategy::boxed(Just(NoteValue::ZERO))
        };

        spend_value_gen.prop_flat_map(move |spend_value| {
            let output_value_gen = if flags.outputs_enabled {
                Strategy::boxed(arb_note_value_bounded(MAX_NOTE_VALUE / n_actions as u64))
            } else {
                Strategy::boxed(Just(NoteValue::ZERO))
            };

            output_value_gen.prop_flat_map(move |output_value| {
                arb_action(note_version, spend_value, output_value)
                    .prop_map(move |a| (spend_value - output_value, a))
            })
        })
    }

    prop_compose! {
        /// Create an arbitrary set of flags with cross-address transfers enabled.
        /// This is representable for all `bundle_version` other than Orchard post-NU6.3.
        ///
        /// Use `arb_flags_ironwood_post_nu6_3` for a strategy that can also disable
        /// cross-address transfers.
        pub fn arb_flags()(spends_enabled in prop::bool::ANY, outputs_enabled in prop::bool::ANY) -> Flags {
            Flags::from_parts(spends_enabled, outputs_enabled, true)
        }
    }

    prop_compose! {
        /// Create an arbitrary set of flags that are valid for an Ironwood bundle post-NU6.3.
        pub fn arb_flags_ironwood_post_nu6_3()(
            spends_enabled in prop::bool::ANY,
            outputs_enabled in prop::bool::ANY,
            cross_address_enabled in prop::bool::ANY,
        ) -> Flags {
            Flags {
                spends_enabled,
                outputs_enabled,
                cross_address_enabled,
            }
        }
    }

    prop_compose! {
        fn arb_base()(bytes in prop::array::uniform32(0u8..)) -> pallas::Base {
            // Instead of rejecting out-of-range bytes, let's reduce them.
            let mut buf = [0; 64];
            buf[..32].copy_from_slice(&bytes);
            pallas::Base::from_uniform_bytes(&buf)
        }
    }

    prop_compose! {
        /// Generate an arbitrary unauthorized bundle. This bundle does not
        /// necessarily respect consensus rules; for that use
        /// [`crate::builder::testing::arb_bundle`]
        pub fn arb_unauthorized_bundle(n_actions: usize)
        (
            bundle_version in arb_bundle_version(),
            flags in arb_flags(),
        )
        (
            acts in vec(arb_unauthorized_action_n(bundle_version.note_version(), n_actions, flags), n_actions),
            anchor in arb_base().prop_map(Anchor::from),
            flags in Just(flags),
            bundle_version in Just(bundle_version),
        ) -> Bundle<Unauthorized, ValueSum> {
            let (balances, actions): (Vec<ValueSum>, Vec<Action<_>>) = acts.into_iter().unzip();
            let flags = flags_for_version(bundle_version, flags);

            Bundle::from_parts(
                NonEmpty::from_vec(actions).unwrap(),
                flags,
                balances.into_iter().sum::<Result<ValueSum, _>>().unwrap(),
                anchor,
                super::EffectsOnly,
                bundle_version,
            )
            .expect("flags are normalized to be representable under bundle_version")
        }
    }

    prop_compose! {
        /// Generate an arbitrary bundle with fake authorization data. This bundle does not
        /// necessarily respect consensus rules; for that use
        /// [`crate::builder::testing::arb_bundle`]
        pub fn arb_bundle(n_actions: usize)
        (
            bundle_version in arb_bundle_version(),
            flags in arb_flags(),
        )
        (
            acts in vec(arb_action_n(bundle_version.note_version(), n_actions, flags), n_actions),
            anchor in arb_base().prop_map(Anchor::from),
            sk in arb_binding_signing_key(),
            rng_seed in prop::array::uniform32(prop::num::u8::ANY),
            // A fake proof of the canonical length, so the bundle passes `try_from_parts`.
            fake_proof in vec(prop::num::u8::ANY, Proof::expected_proof_size(n_actions)),
            fake_sighash in prop::array::uniform32(prop::num::u8::ANY),
            flags in Just(flags),
            bundle_version in Just(bundle_version),
        ) -> Bundle<Authorized, ValueSum> {
            let (balances, actions): (Vec<ValueSum>, Vec<Action<_>>) = acts.into_iter().unzip();
            let rng = StdRng::from_seed(rng_seed);
            let flags = flags_for_version(bundle_version, flags);

            Bundle::try_from_parts(
                NonEmpty::from_vec(actions).unwrap(),
                flags,
                balances.into_iter().sum::<Result<ValueSum, _>>().unwrap(),
                anchor,
                Authorized {
                    proof: Proof::new(fake_proof),
                    binding_signature: sk.sign(rng, &fake_sighash),
                },
                bundle_version,
            )
            .expect("fake proof has the canonical length and flags are representable")
        }
    }
}

#[cfg(test)]
pub(crate) mod tests {
    use alloc::vec;

    use proptest::prelude::*;

    use super::testing::{arb_bundle, arb_flags, arb_flags_ironwood_post_nu6_3};
    use super::{
        Authorized, Bundle, BundleError, BundleVersion, CommitmentError, Flags, TxVersion,
    };
    use crate::Proof;

    #[cfg(feature = "circuit")]
    pub(crate) fn with_cross_address_disabled(
        bundle: Bundle<Authorized, crate::value::ValueSum>,
    ) -> Bundle<Authorized, crate::value::ValueSum> {
        let mut flags = *bundle.flags();
        flags.cross_address_enabled = false;

        Bundle::from_parts_unchecked(
            bundle.actions().clone(),
            flags,
            *bundle.value_balance(),
            *bundle.anchor(),
            bundle.authorization().clone(),
            bundle.bundle_version(),
        )
    }

    #[cfg(feature = "circuit")]
    pub(crate) fn sample_authorized_bundle(
        n_actions: usize,
    ) -> Bundle<Authorized, crate::value::ValueSum> {
        use proptest::strategy::ValueTree;

        let mut runner = proptest::test_runner::TestRunner::deterministic();
        arb_bundle(n_actions)
            .new_tree(&mut runner)
            .expect("strategy can generate a bundle")
            .current()
    }

    #[test]
    fn flags_byte_encoding() {
        for (flags, orchard_pre_nu6_3, orchard_nu6_3, ironwood_nu6_3) in [
            (Flags::ENABLED, Some(0b011), None, Some(0b111)),
            (Flags::SPENDS_DISABLED, Some(0b010), None, Some(0b110)),
            (Flags::OUTPUTS_DISABLED, Some(0b001), None, Some(0b101)),
            (
                Flags::CROSS_ADDRESS_DISABLED,
                None,
                Some(0b011),
                Some(0b011),
            ),
        ] {
            assert_eq!(
                flags.to_byte(BundleVersion::orchard_v2()),
                orchard_pre_nu6_3
            );
            assert_eq!(flags.to_byte(BundleVersion::orchard_v3()), orchard_nu6_3);
            assert_eq!(flags.to_byte(BundleVersion::ironwood_v3()), ironwood_nu6_3);
        }
    }

    #[test]
    fn flags_parsing_diverges_between_epochs() {
        // A byte with bit 2 clear parses as an unrestricted bundle for Orchard pre-NU6.3,
        // and a restricted bundle for Orchard or Ironwood post-NU6.3.
        for value in 0b000..=0b011 {
            let pre_nu6_3_flags = Flags::from_byte(value, BundleVersion::orchard_v2()).unwrap();
            let nu6_3_flags = Flags::from_byte(value, BundleVersion::orchard_v3()).unwrap();
            let ironwood_flags = Flags::from_byte(value, BundleVersion::ironwood_v3()).unwrap();

            assert_eq!(
                pre_nu6_3_flags.spends_enabled(),
                nu6_3_flags.spends_enabled()
            );
            assert_eq!(
                pre_nu6_3_flags.outputs_enabled(),
                nu6_3_flags.outputs_enabled()
            );
            assert!(pre_nu6_3_flags.cross_address_enabled());
            assert!(!nu6_3_flags.cross_address_enabled());
            assert_eq!(ironwood_flags, nu6_3_flags);

            // Each parse round-trips to the same byte under its own era, but the
            // restricted set is unrepresentable pre-NU6.3.
            assert_eq!(
                pre_nu6_3_flags.to_byte(BundleVersion::orchard_v2()),
                Some(value)
            );
            assert_eq!(
                nu6_3_flags.to_byte(BundleVersion::orchard_v3()),
                Some(value)
            );
            assert_eq!(nu6_3_flags.to_byte(BundleVersion::orchard_v2()), None);
        }

        assert_eq!(
            Flags::from_byte(0b011, BundleVersion::orchard_v2()),
            Some(Flags::ENABLED)
        );
        assert_eq!(
            Flags::from_byte(0b011, BundleVersion::orchard_v3()),
            Some(Flags::CROSS_ADDRESS_DISABLED)
        );
        assert_eq!(
            Flags::from_byte(0b011, BundleVersion::ironwood_v3()),
            Some(Flags::CROSS_ADDRESS_DISABLED)
        );
    }

    #[test]
    fn only_orchard_post_nu6_3_requires_the_cross_address_restriction() {
        // Consensus mandates the restriction only for the Orchard pool at NU6.3; every other
        // variant leaves the choice free (the builder then applies its prover-side default).
        assert!(!BundleVersion::orchard_v3().permits_cross_address_transfers());
        for bundle_version in [
            BundleVersion::orchard_insecure_v1(),
            BundleVersion::orchard_v2(),
            BundleVersion::ironwood_v3(),
        ] {
            assert!(bundle_version.permits_cross_address_transfers());
        }
    }

    #[test]
    fn pre_nu6_3_flags_parsing_rejects_reserved_bits() {
        for value in 0b100..=u8::MAX {
            assert_eq!(Flags::from_byte(value, BundleVersion::orchard_v2()), None);
        }
    }

    #[test]
    fn nu6_3_flags_parsing_handles_the_cross_address_bit() {
        for value in 0b100..=0b111 {
            // Orchard post-NU6.3 mandates the restriction, so bit 2 (`enableCrossAddress`)
            // set is rejected there.
            assert_eq!(Flags::from_byte(value, BundleVersion::orchard_v3()), None);
            // Bit 2 set is only valid for the Ironwood pool, where it is recognized as
            // cross-address enabled and round-trips.
            let flags = Flags::from_byte(value, BundleVersion::ironwood_v3()).unwrap();
            assert!(flags.cross_address_enabled());
            assert_eq!(flags.to_byte(BundleVersion::ironwood_v3()), Some(value));
            // Pre-NU6.3 formats encode the same flag set with bit 2 reserved zero.
            assert_eq!(
                flags.to_byte(BundleVersion::orchard_v2()),
                Some(value & 0b011)
            );
        }

        // Bits 3.. are always reserved, in every NU6.3 pool.
        for value in 0b1000..=u8::MAX {
            assert_eq!(Flags::from_byte(value, BundleVersion::orchard_v3()), None);
            assert_eq!(Flags::from_byte(value, BundleVersion::ironwood_v3()), None);
        }
    }

    #[test]
    fn expected_proof_size_matches_known_values() {
        // The canonical proof sizes for one and two actions, fixed by the action circuit.
        assert_eq!(Proof::expected_proof_size(1), 4992);
        assert_eq!(Proof::expected_proof_size(2), 7264);

        // The size is affine in the number of actions: each action contributes a fixed amount.
        let per_action = Proof::expected_proof_size(2) - Proof::expected_proof_size(1);
        assert_eq!(
            Proof::expected_proof_size(3) - Proof::expected_proof_size(2),
            per_action,
        );
    }

    #[test]
    fn empty_commitments_are_domain_separated() {
        use crate::bundle::commitments::{hash_bundle_auth_empty, hash_bundle_txid_empty};
        use crate::ValuePool;

        // The three commitment formats — Orchard v5, Orchard v6, Ironwood v6 — use distinct
        // personalizations, so the absent-bundle digests are all different from one another.
        let formats = [
            (ValuePool::Orchard, TxVersion::V5),
            (ValuePool::Orchard, TxVersion::V6),
            (ValuePool::Ironwood, TxVersion::V6),
        ];
        for i in 0..formats.len() {
            for j in (i + 1)..formats.len() {
                let (pi, ti) = formats[i];
                let (pj, tj) = formats[j];
                assert_ne!(
                    hash_bundle_txid_empty(pi, ti).unwrap().as_bytes(),
                    hash_bundle_txid_empty(pj, tj).unwrap().as_bytes()
                );
                assert_ne!(
                    hash_bundle_auth_empty(pi, ti).unwrap().as_bytes(),
                    hash_bundle_auth_empty(pj, tj).unwrap().as_bytes()
                );
            }
        }

        assert!(matches!(
            hash_bundle_txid_empty(ValuePool::Ironwood, TxVersion::V5),
            Err(CommitmentError::InvalidTransactionVersion)
        ));
        assert!(matches!(
            hash_bundle_auth_empty(ValuePool::Ironwood, TxVersion::V5),
            Err(CommitmentError::InvalidTransactionVersion)
        ));
    }

    proptest! {
        // The property is deterministic given the actions, so a handful of cases suffices.
        #![proptest_config(ProptestConfig::with_cases(16))]

        #[test]
        fn arb_flags_ironwood_post_nu6_3_round_trips(flags in arb_flags_ironwood_post_nu6_3()) {
            let encoded = flags
                .to_byte(BundleVersion::ironwood_v3())
                .expect("all Ironwood post-NU6.3 flag strategy outputs encode under Ironwood post-NU6.3");

            prop_assert_eq!(Flags::from_byte(encoded, BundleVersion::ironwood_v3()), Some(flags));
        }

        #[test]
        fn orchard_nu6_3_rejects_cross_address_enabled(flags in arb_flags()) {
            // `arb_flags` always enables cross-address transfers, which Orchard post-NU6.3
            // forbids, so encoding under those restrictions must fail. The cross-address-
            // disabled projection must still encode and round-trip.
            prop_assert_eq!(flags.to_byte(BundleVersion::orchard_v3()), None);

            let mut disabled = flags;
            disabled.cross_address_enabled = false;
            let encoded = disabled
                .to_byte(BundleVersion::orchard_v3())
                .expect("cross-address-disabled flags encode under Orchard post-NU6.3");
            prop_assert_eq!(
                Flags::from_byte(encoded, BundleVersion::orchard_v3()),
                Some(disabled)
            );
        }

        #[test]
        fn commitment_hashes_the_wire_flag_byte(bundle in arb_bundle(3)) {
            let actions = bundle.actions().clone();
            let anchor = *bundle.anchor();
            let authorization = bundle.authorization().clone();
            let spends_enabled = bundle.flags().spends_enabled();
            let outputs_enabled = bundle.flags().outputs_enabled();

            let enabled = Flags::from_parts(spends_enabled, outputs_enabled, true);
            let disabled = Flags::from_parts(spends_enabled, outputs_enabled, false);

            // Build the same actions under different (flags, version) combinations, with `V = i64`
            // so that `commitment()` is available.
            let build = |flags, bundle_version| {
                Bundle::from_parts_unchecked(
                    actions.clone(),
                    flags,
                    0i64,
                    anchor,
                    authorization.clone(),
                    bundle_version,
                )
            };

            // Orchard pre-NU6.3 has cross-address implicitly enabled and Orchard NU6.3 has it
            // disabled, but both encode to the same wire byte, so their commitments agree.
            let legacy = build(enabled, BundleVersion::orchard_v2());
            let restricted = build(disabled, BundleVersion::orchard_v3());
            prop_assert_eq!(restricted.flag_byte(), legacy.flag_byte());

            let restricted_commitment: [u8; 32] =
                restricted.commitment(TxVersion::V5).unwrap().into();
            let legacy_commitment: [u8; 32] = legacy.commitment(TxVersion::V5).unwrap().into();
            prop_assert_eq!(restricted_commitment, legacy_commitment);

            // The unrestricted Ironwood encoding sets bit 2, producing a distinct digest. Only
            // the Ironwood pool may set it; Orchard post-NU6.3 prohibits cross-address transfers.
            let unrestricted = build(enabled, BundleVersion::ironwood_v3());
            let unrestricted_commitment: [u8; 32] =
                unrestricted.commitment(TxVersion::V6).unwrap().into();
            prop_assert_ne!(unrestricted_commitment, restricted_commitment);

            // The restricted flag set has no pre-NU6.3 encoding, so no such bundle can be built.
            prop_assert_eq!(disabled.to_byte(BundleVersion::orchard_v2()), None);
        }

        #[test]
        fn ironwood_rejects_v5_commitment_version(bundle in arb_bundle(3)) {
            let bundle_i64 = Bundle::from_parts_unchecked(
                bundle.actions().clone(),
                *bundle.flags(),
                0i64,
                *bundle.anchor(),
                bundle.authorization().clone(),
                BundleVersion::ironwood_v3(),
            );
            let ironwood = Bundle::from_parts_unchecked(
                bundle.actions().clone(),
                *bundle.flags(),
                *bundle.value_balance(),
                *bundle.anchor(),
                bundle.authorization().clone(),
                BundleVersion::ironwood_v3(),
            );

            prop_assert!(matches!(
                bundle_i64.commitment(TxVersion::V5),
                Err(CommitmentError::InvalidTransactionVersion)
            ));
            prop_assert!(matches!(
                ironwood.authorizing_commitment(TxVersion::V5),
                Err(CommitmentError::InvalidTransactionVersion)
            ));
        }

        /// The anchor bytes are included in the transaction-ID digest for the v5 format and in
        /// the authorizing digest for the v6 format, so changing only the anchor moves exactly
        /// one of the two digests. The v5 and v6 Orchard formats are also domain-separated, so
        /// the same bundle commits to distinct transaction-ID digests under each.
        #[test]
        fn anchor_placement_follows_tx_version(bundle in arb_bundle(3)) {
            // Orchard post-NU6.3 cannot encode cross-address transfers, so clear the bit to keep
            // the flags representable in every version under test.
            let flags = Flags::from_parts(
                bundle.flags().spends_enabled(),
                bundle.flags().outputs_enabled(),
                false,
            );

            let with_anchor = |anchor, bundle_version| {
                Bundle::from_parts_unchecked(
                    bundle.actions().clone(),
                    flags,
                    0i64,
                    anchor,
                    bundle.authorization().clone(),
                    bundle_version,
                )
            };
            let anchor_a = crate::Anchor::from_bytes([0u8; 32]).unwrap();
            let anchor_b = crate::Anchor::from_bytes([6u8; 32]).unwrap();

            for (bundle_version, tx, anchor_in_txid_digest) in [
                (BundleVersion::orchard_v3(), TxVersion::V5, true),
                (BundleVersion::orchard_v3(), TxVersion::V6, false),
                (BundleVersion::ironwood_v3(), TxVersion::V6, false),
            ] {
                let a = with_anchor(anchor_a, bundle_version);
                let b = with_anchor(anchor_b, bundle_version);
                let txid_a: [u8; 32] = a.commitment(tx).unwrap().into();
                let txid_b: [u8; 32] = b.commitment(tx).unwrap().into();
                let auth_a = a.authorizing_commitment(tx).unwrap().0;
                let auth_b = b.authorizing_commitment(tx).unwrap().0;
                if anchor_in_txid_digest {
                    prop_assert_ne!(txid_a, txid_b);
                    prop_assert_eq!(auth_a.as_bytes(), auth_b.as_bytes());
                } else {
                    prop_assert_eq!(txid_a, txid_b);
                    prop_assert_ne!(auth_a.as_bytes(), auth_b.as_bytes());
                }
            }

            // The v5 and v6 Orchard formats are domain-separated, so the same bundle commits to
            // distinct transaction-ID digests under each.
            let a_v2 = with_anchor(anchor_a, BundleVersion::orchard_v3());
            let orchard_v5: [u8; 32] = a_v2.commitment(TxVersion::V5).unwrap().into();
            let orchard_v6: [u8; 32] = a_v2.commitment(TxVersion::V6).unwrap().into();
            prop_assert_ne!(orchard_v5, orchard_v6);
        }

        #[test]
        fn try_from_parts_enforces_canonical_proof_size(
            bundle in arb_bundle(3)
        ) {
            let actions = bundle.actions().clone();
            let expected = Proof::expected_proof_size(actions.len());
            let flags = *bundle.flags();
            // Ironwood enforces canonical proof size and accepts any cross-address flag value.
            let bundle_version = BundleVersion::ironwood_v3();
            let value_balance = *bundle.value_balance();
            let anchor = *bundle.anchor();
            let binding_signature = bundle.authorization().binding_signature().clone();

            let with_proof_len = |proof_len: usize| {
                Bundle::try_from_parts(
                    actions.clone(),
                    flags,
                    value_balance,
                    anchor,
                    Authorized::from_parts(
                        Proof::new(vec![0u8; proof_len]),
                        binding_signature.clone(),
                    ),
                    bundle_version,
                )
            };

            // A canonically-sized proof is accepted.
            prop_assert!(with_proof_len(expected).is_ok());

            // A proof padded with trailing data is rejected (the GHSA-2x4w-pxqw-58v9 attack).
            prop_assert_eq!(
                with_proof_len(expected + 1).err(),
                Some(BundleError::NonCanonicalProofSize { expected, actual: expected + 1 })
            );

            // A truncated proof is rejected.
            prop_assert_eq!(
                with_proof_len(expected - 1).err(),
                Some(BundleError::NonCanonicalProofSize { expected, actual: expected - 1 })
            );
        }

        #[test]
        fn try_from_parts_preserves_cross_address_disabled(
            bundle in arb_bundle(3)
        ) {
            let actions = bundle.actions().clone();
            let mut flags = *bundle.flags();
            flags.cross_address_enabled = false;
            let value_balance = *bundle.value_balance();
            let anchor = *bundle.anchor();
            let authorization = bundle.authorization().clone();

            let bundle = Bundle::try_from_parts(
                    actions,
                    flags,
                    value_balance,
                    anchor,
                    authorization,
                    BundleVersion::orchard_v3(),
                )
                .expect("canonical proof size is accepted");
            prop_assert!(!bundle.flags().cross_address_enabled());
        }

        #[test]
        fn try_from_parts_checks_proof_size_with_cross_address_disabled(
            bundle in arb_bundle(3)
        ) {
            let actions = bundle.actions().clone();
            let expected = Proof::expected_proof_size(actions.len());
            let mut flags = *bundle.flags();
            flags.cross_address_enabled = false;
            let value_balance = *bundle.value_balance();
            let anchor = *bundle.anchor();
            let binding_signature = bundle.authorization().binding_signature().clone();

            prop_assert_eq!(
                Bundle::try_from_parts(
                    actions,
                    flags,
                    value_balance,
                    anchor,
                    Authorized::from_parts(
                        Proof::new(vec![0u8; expected + 1]),
                        binding_signature,
                    ),
                    BundleVersion::orchard_v3(),
                )
                .err(),
                Some(BundleError::NonCanonicalProofSize { expected, actual: expected + 1 })
            );
        }

        #[test]
        fn insecure_v1_skips_proof_size_enforcement(bundle in arb_bundle(3)) {
            // The historical pre-NU6.2 Orchard pool does not enforce canonical proof size, so a
            // padded proof is accepted: its transaction is already committed and cannot be
            // re-canonicalized.
            let expected = Proof::expected_proof_size(bundle.actions().len());
            let padded = Bundle::try_from_parts(
                bundle.actions().clone(),
                Flags::ENABLED,
                *bundle.value_balance(),
                *bundle.anchor(),
                Authorized::from_parts(
                    Proof::new(vec![0u8; expected + 1]),
                    bundle.authorization().binding_signature().clone(),
                ),
                BundleVersion::orchard_insecure_v1(),
            );
            prop_assert!(padded.is_ok());
        }
    }

    #[cfg(feature = "circuit")]
    #[test]
    fn from_parts_rejects_unrepresentable_flags() {
        // A cross-address-disabled flag set has no pre-NU6.3 Orchard encoding, so a bundle
        // carrying that combination cannot be constructed under `orchard_v2()`.
        let bundle = sample_authorized_bundle(1);
        let flags = Flags::from_parts(
            bundle.flags().spends_enabled(),
            bundle.flags().outputs_enabled(),
            false,
        );

        assert_eq!(
            Bundle::try_from_parts(
                bundle.actions().clone(),
                flags,
                *bundle.value_balance(),
                *bundle.anchor(),
                bundle.authorization().clone(),
                BundleVersion::orchard_v2(),
            )
            .err(),
            Some(BundleError::UnrepresentableFlags)
        );
    }

    #[cfg(feature = "circuit")]
    #[test]
    fn verify_proof_rejects_cross_address_disabled_for_unsupported_keys() {
        let bundle = with_cross_address_disabled(sample_authorized_bundle(1));

        for circuit_version in [
            crate::circuit::OrchardCircuitVersion::InsecurePreNu6_2,
            crate::circuit::OrchardCircuitVersion::FixedPostNu6_2,
        ] {
            let vk = crate::circuit::VerifyingKey::build(circuit_version);

            assert!(matches!(
                bundle.verify_proof(&vk),
                Err(halo2_proofs::plonk::Error::InvalidInstances)
            ));
        }
    }
}
