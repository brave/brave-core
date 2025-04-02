//! Structs related to bundles of Orchard actions.

mod batch;
pub mod commitments;

pub use batch::BatchValidator;

use core::fmt;

use blake2b_simd::Hash as Blake2bHash;
use memuse::DynamicUsage;
use nonempty::NonEmpty;
use zcash_note_encryption::{try_note_decryption, try_output_recovery_with_ovk};

use crate::{
    action::Action,
    address::Address,
    bundle::commitments::{hash_bundle_auth_data, hash_bundle_txid_data},
    circuit::{Instance, Proof, VerifyingKey},
    keys::{IncomingViewingKey, OutgoingViewingKey, PreparedIncomingViewingKey},
    note::Note,
    note_encryption::OrchardDomain,
    primitives::redpallas::{self, Binding, SpendAuth},
    tree::Anchor,
    value::{ValueCommitTrapdoor, ValueCommitment, ValueSum},
};

impl<T> Action<T> {
    /// Prepares the public instance for this action, for creating and verifying the
    /// bundle proof.
    pub fn to_instance(&self, flags: Flags, anchor: Anchor) -> Instance {
        Instance {
            anchor,
            cv_net: self.cv_net().clone(),
            nf_old: *self.nullifier(),
            rk: self.rk().clone(),
            cmx: *self.cmx(),
            enable_spend: flags.spends_enabled,
            enable_output: flags.outputs_enabled,
        }
    }
}

/// Orchard-specific flags.
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
}

const FLAG_SPENDS_ENABLED: u8 = 0b0000_0001;
const FLAG_OUTPUTS_ENABLED: u8 = 0b0000_0010;
const FLAGS_EXPECTED_UNSET: u8 = !(FLAG_SPENDS_ENABLED | FLAG_OUTPUTS_ENABLED);

impl Flags {
    /// Construct a set of flags from its constituent parts
    pub(crate) const fn from_parts(spends_enabled: bool, outputs_enabled: bool) -> Self {
        Flags {
            spends_enabled,
            outputs_enabled,
        }
    }

    /// The flag set with both spends and outputs enabled.
    pub const ENABLED: Flags = Flags {
        spends_enabled: true,
        outputs_enabled: true,
    };

    /// The flag set with spends disabled.
    pub const SPENDS_DISABLED: Flags = Flags {
        spends_enabled: false,
        outputs_enabled: true,
    };

    /// The flag set with outputs disabled.
    pub const OUTPUTS_DISABLED: Flags = Flags {
        spends_enabled: true,
        outputs_enabled: false,
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

    /// Serialize flags to a byte as defined in [Zcash Protocol Spec § 7.1: Transaction
    /// Encoding And Consensus][txencoding].
    ///
    /// [txencoding]: https://zips.z.cash/protocol/protocol.pdf#txnencoding
    pub fn to_byte(&self) -> u8 {
        let mut value = 0u8;
        if self.spends_enabled {
            value |= FLAG_SPENDS_ENABLED;
        }
        if self.outputs_enabled {
            value |= FLAG_OUTPUTS_ENABLED;
        }
        value
    }

    /// Parses flags from a single byte as defined in [Zcash Protocol Spec § 7.1:
    /// Transaction Encoding And Consensus][txencoding].
    ///
    /// Returns `None` if unexpected bits are set in the flag byte.
    ///
    /// [txencoding]: https://zips.z.cash/protocol/protocol.pdf#txnencoding
    pub fn from_byte(value: u8) -> Option<Self> {
        // https://p.z.cash/TCR:bad-txns-v5-reserved-bits-nonzero
        if value & FLAGS_EXPECTED_UNSET == 0 {
            Some(Self {
                spends_enabled: value & FLAG_SPENDS_ENABLED != 0,
                outputs_enabled: value & FLAG_OUTPUTS_ENABLED != 0,
            })
        } else {
            None
        }
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
}

impl<T: Authorization, V: fmt::Debug> fmt::Debug for Bundle<T, V> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        /// Helper struct for debug-printing actions without exposing `NonEmpty`.
        struct Actions<'a, T>(&'a NonEmpty<Action<T>>);
        impl<'a, T: fmt::Debug> fmt::Debug for Actions<'a, T> {
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
            .finish()
    }
}

impl<T: Authorization, V> Bundle<T, V> {
    /// Constructs a `Bundle` from its constituent parts.
    pub fn from_parts(
        actions: NonEmpty<Action<T::SpendAuth>>,
        flags: Flags,
        value_balance: V,
        anchor: Anchor,
        authorization: T,
    ) -> Self {
        Bundle {
            actions,
            flags,
            value_balance,
            anchor,
            authorization,
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
        })
    }

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
                let domain = OrchardDomain::for_action(action);
                prepared_keys.iter().find_map(|(ivk, prepared_ivk)| {
                    try_note_decryption(&domain, prepared_ivk, action)
                        .map(|(n, a, m)| (idx, (*ivk).clone(), n, a, m))
                })
            })
            .collect()
    }

    /// Performs trial decryption of the action at `action_idx` in the bundle with the
    /// specified incoming viewing key, and returns the decrypted note plaintext
    /// contents if successful.
    pub fn decrypt_output_with_key(
        &self,
        action_idx: usize,
        key: &IncomingViewingKey,
    ) -> Option<(Note, Address, [u8; 512])> {
        let prepared_ivk = PreparedIncomingViewingKey::new(key);
        self.actions.get(action_idx).and_then(move |action| {
            let domain = OrchardDomain::for_action(action);
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
                let domain = OrchardDomain::for_action(action);
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
            let domain = OrchardDomain::for_action(action);
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
    /// Computes a commitment to the effects of this bundle, suitable for inclusion within
    /// a transaction ID.
    pub fn commitment(&self) -> BundleCommitment {
        BundleCommitment(hash_bundle_txid_data(self))
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

impl<V> Bundle<Authorized, V> {
    /// Computes a commitment to the authorizing data within for this bundle.
    ///
    /// This together with `Bundle::commitment` bind the entire bundle.
    pub fn authorizing_commitment(&self) -> BundleAuthorizingCommitment {
        BundleAuthorizingCommitment(hash_bundle_auth_data(self))
    }

    /// Verifies the proof for this bundle.
    pub fn verify_proof(&self, vk: &VerifyingKey) -> Result<(), halo2_proofs::plonk::Error> {
        self.authorization()
            .proof()
            .verify(vk, &self.to_instances())
    }
}

impl<V: DynamicUsage> DynamicUsage for Bundle<Authorized, V> {
    fn dynamic_usage(&self) -> usize {
        self.actions.dynamic_usage()
            + self.value_balance.dynamic_usage()
            + self.authorization.proof.dynamic_usage()
    }

    fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
        let bounds = (
            self.actions.dynamic_usage_bounds(),
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
    use group::ff::FromUniformBytes;
    use nonempty::NonEmpty;
    use pasta_curves::pallas;
    use rand::{rngs::StdRng, SeedableRng};
    use reddsa::orchard::SpendAuth;

    use proptest::collection::vec;
    use proptest::prelude::*;

    use crate::{
        circuit::Proof,
        primitives::redpallas::{self, testing::arb_binding_signing_key},
        value::{testing::arb_note_value_bounded, NoteValue, ValueSum, MAX_NOTE_VALUE},
        Anchor,
    };

    use super::{Action, Authorization, Authorized, Bundle, Flags};

    pub use crate::action::testing::{arb_action, arb_unauthorized_action};

    /// Marker for an unauthorized bundle with no proofs or signatures.
    #[derive(Debug)]
    pub struct Unauthorized;

    impl Authorization for Unauthorized {
        type SpendAuth = ();
    }

    /// Generate an unauthorized action having spend and output values less than MAX_NOTE_VALUE / n_actions.
    pub fn arb_unauthorized_action_n(
        n_actions: usize,
        flags: Flags,
    ) -> impl Strategy<Value = (ValueSum, Action<()>)> {
        let spend_value_gen = if flags.spends_enabled {
            Strategy::boxed(arb_note_value_bounded(MAX_NOTE_VALUE / n_actions as u64))
        } else {
            Strategy::boxed(Just(NoteValue::zero()))
        };

        spend_value_gen.prop_flat_map(move |spend_value| {
            let output_value_gen = if flags.outputs_enabled {
                Strategy::boxed(arb_note_value_bounded(MAX_NOTE_VALUE / n_actions as u64))
            } else {
                Strategy::boxed(Just(NoteValue::zero()))
            };

            output_value_gen.prop_flat_map(move |output_value| {
                arb_unauthorized_action(spend_value, output_value)
                    .prop_map(move |a| (spend_value - output_value, a))
            })
        })
    }

    /// Generate an authorized action having spend and output values less than MAX_NOTE_VALUE / n_actions.
    pub fn arb_action_n(
        n_actions: usize,
        flags: Flags,
    ) -> impl Strategy<Value = (ValueSum, Action<redpallas::Signature<SpendAuth>>)> {
        let spend_value_gen = if flags.spends_enabled {
            Strategy::boxed(arb_note_value_bounded(MAX_NOTE_VALUE / n_actions as u64))
        } else {
            Strategy::boxed(Just(NoteValue::zero()))
        };

        spend_value_gen.prop_flat_map(move |spend_value| {
            let output_value_gen = if flags.outputs_enabled {
                Strategy::boxed(arb_note_value_bounded(MAX_NOTE_VALUE / n_actions as u64))
            } else {
                Strategy::boxed(Just(NoteValue::zero()))
            };

            output_value_gen.prop_flat_map(move |output_value| {
                arb_action(spend_value, output_value)
                    .prop_map(move |a| (spend_value - output_value, a))
            })
        })
    }

    prop_compose! {
        /// Create an arbitrary set of flags.
        pub fn arb_flags()(spends_enabled in prop::bool::ANY, outputs_enabled in prop::bool::ANY) -> Flags {
            Flags::from_parts(spends_enabled, outputs_enabled)
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
            flags in arb_flags(),
        )
        (
            acts in vec(arb_unauthorized_action_n(n_actions, flags), n_actions),
            anchor in arb_base().prop_map(Anchor::from),
            flags in Just(flags)
        ) -> Bundle<Unauthorized, ValueSum> {
            let (balances, actions): (Vec<ValueSum>, Vec<Action<_>>) = acts.into_iter().unzip();

            Bundle::from_parts(
                NonEmpty::from_vec(actions).unwrap(),
                flags,
                balances.into_iter().sum::<Result<ValueSum, _>>().unwrap(),
                anchor,
                Unauthorized
            )
        }
    }

    prop_compose! {
        /// Generate an arbitrary bundle with fake authorization data. This bundle does not
        /// necessarily respect consensus rules; for that use
        /// [`crate::builder::testing::arb_bundle`]
        pub fn arb_bundle(n_actions: usize)
        (
            flags in arb_flags(),
        )
        (
            acts in vec(arb_action_n(n_actions, flags), n_actions),
            anchor in arb_base().prop_map(Anchor::from),
            sk in arb_binding_signing_key(),
            rng_seed in prop::array::uniform32(prop::num::u8::ANY),
            fake_proof in vec(prop::num::u8::ANY, 1973),
            fake_sighash in prop::array::uniform32(prop::num::u8::ANY),
            flags in Just(flags)
        ) -> Bundle<Authorized, ValueSum> {
            let (balances, actions): (Vec<ValueSum>, Vec<Action<_>>) = acts.into_iter().unzip();
            let rng = StdRng::from_seed(rng_seed);

            Bundle::from_parts(
                NonEmpty::from_vec(actions).unwrap(),
                flags,
                balances.into_iter().sum::<Result<ValueSum, _>>().unwrap(),
                anchor,
                Authorized {
                    proof: Proof::new(fake_proof),
                    binding_signature: sk.sign(rng, &fake_sighash),
                }
            )
        }
    }
}
