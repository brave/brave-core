use core::fmt;
use core::ops::Not as _;

use group::{Group as _, GroupEncoding as _};
use memuse::DynamicUsage;
use pasta_curves::pallas;
use subtle::CtOption;

use crate::{
    note::{ExtractedNoteCommitment, Nullifier, Rho, TransmittedNoteCiphertext},
    primitives::redpallas::{self, SpendAuth},
    value::ValueCommitment,
};

/// An action applied to the global ledger.
///
/// This both creates a note (adding a commitment to the global ledger), and consumes
/// some note created prior to this action (adding a nullifier to the global ledger).
///
/// # Invariants
///
/// Every `Action` has a non-identity `rk`, and an `epk_bytes` that encodes a
/// non-identity [`pasta_curves::pallas::Point`].
#[derive(Debug, Clone)]
pub struct Action<A> {
    /// The nullifier of the note being spent.
    nf: Nullifier,
    /// The randomized verification key for the note being spent.
    rk: redpallas::VerificationKey<SpendAuth>,
    /// A commitment to the new note being created.
    cmx: ExtractedNoteCommitment,
    /// The transmitted note ciphertext.
    encrypted_note: TransmittedNoteCiphertext,
    /// A commitment to the net value created or consumed by this action.
    cv_net: ValueCommitment,
    /// The authorization for this action.
    authorization: A,
}

impl<T> Action<T> {
    /// Constructs an `Action` from its constituent parts.
    ///
    /// Returns an [`ActionFromPartsError`] if `rk` is the identity
    /// [`pasta_curves::pallas::Point`], or if `encrypted_note.epk_bytes` does
    /// not encode a non-identity point.
    ///
    /// zcashd v6.12.1 and Zebra 4.3.1 both added a consensus rule rejecting
    /// transactions whose Orchard actions have an identity `rk`; the Zcash
    /// protocol specification will be updated to match, and this crate aligns
    /// with that rule. The ephemeral public key `epk` is likewise required to
    /// be a non-identity point: it is a `KA^{Orchard}` public key (ℙ*), and the
    /// identity is not a valid key-agreement public key.
    ///
    /// See:
    /// - <https://zodl.com/zcashd-zebra-april-2026-disclosure/>
    /// - <https://zfnd.org/zebra-4-3-1-critical-security-fixes-dockerized-mining-and-ci-hardening/>
    pub fn from_parts(
        nf: Nullifier,
        rk: redpallas::VerificationKey<SpendAuth>,
        cmx: ExtractedNoteCommitment,
        encrypted_note: TransmittedNoteCiphertext,
        cv_net: ValueCommitment,
        authorization: T,
    ) -> Result<Self, ActionFromPartsError> {
        if rk.is_identity() {
            return Err(ActionFromPartsError::IdentityRk);
        }

        Option::<()>::from(
            pallas::Point::from_bytes(&encrypted_note.epk_bytes)
                .and_then(|p| CtOption::new((), p.is_identity().not())),
        )
        .ok_or(ActionFromPartsError::InvalidEpk)?;

        Ok(Action {
            nf,
            rk,
            cmx,
            encrypted_note,
            cv_net,
            authorization,
        })
    }

    /// Returns the nullifier of the note being spent.
    pub fn nullifier(&self) -> &Nullifier {
        &self.nf
    }

    /// Returns the randomized verification key for the note being spent.
    pub fn rk(&self) -> &redpallas::VerificationKey<SpendAuth> {
        &self.rk
    }

    /// Returns the commitment to the new note being created.
    pub fn cmx(&self) -> &ExtractedNoteCommitment {
        &self.cmx
    }

    /// Returns the encrypted note ciphertext.
    pub fn encrypted_note(&self) -> &TransmittedNoteCiphertext {
        &self.encrypted_note
    }

    /// Obtains the [`Rho`] value that was used to construct the new note being created.
    pub fn rho(&self) -> Rho {
        Rho::from_nf_old(self.nf)
    }

    /// Returns the commitment to the net value created or consumed by this action.
    pub fn cv_net(&self) -> &ValueCommitment {
        &self.cv_net
    }

    /// Returns the authorization for this action.
    pub fn authorization(&self) -> &T {
        &self.authorization
    }

    /// Transitions this action from one authorization state to another.
    pub fn map<U>(self, step: impl FnOnce(T) -> U) -> Action<U> {
        Action {
            nf: self.nf,
            rk: self.rk,
            cmx: self.cmx,
            encrypted_note: self.encrypted_note,
            cv_net: self.cv_net,
            authorization: step(self.authorization),
        }
    }

    /// Transitions this action from one authorization state to another.
    pub fn try_map<U, E>(self, step: impl FnOnce(T) -> Result<U, E>) -> Result<Action<U>, E> {
        Ok(Action {
            nf: self.nf,
            rk: self.rk,
            cmx: self.cmx,
            encrypted_note: self.encrypted_note,
            cv_net: self.cv_net,
            authorization: step(self.authorization)?,
        })
    }
}

/// Errors that can occur when constructing an [`Action`] via
/// [`Action::from_parts`].
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[non_exhaustive]
pub enum ActionFromPartsError {
    /// `rk` is the identity point, which is forbidden by the consensus rule
    /// introduced in zcashd v6.12.1 and Zebra 4.3.1.
    IdentityRk,
    /// `epk_bytes` does not encode a non-identity Pallas point, so it is not a
    /// valid `KA^{Orchard}` public key.
    InvalidEpk,
}

impl fmt::Display for ActionFromPartsError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ActionFromPartsError::IdentityRk => {
                write!(f, "an Orchard action with identity `rk` is not valid")
            }
            ActionFromPartsError::InvalidEpk => write!(
                f,
                "an Orchard action's `epk` is not a valid non-identity Pallas point"
            ),
        }
    }
}

impl core::error::Error for ActionFromPartsError {}

impl DynamicUsage for Action<redpallas::Signature<SpendAuth>> {
    #[inline(always)]
    fn dynamic_usage(&self) -> usize {
        0
    }

    #[inline(always)]
    fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
        (0, Some(0))
    }
}

/// Generators for property testing.
#[cfg(any(test, feature = "test-dependencies"))]
#[cfg_attr(docsrs, doc(cfg(feature = "test-dependencies")))]
pub(crate) mod testing {
    use rand::{rngs::StdRng, RngCore, SeedableRng};
    use reddsa::orchard::SpendAuth;
    use zcash_note_encryption::Domain as _;

    use proptest::prelude::*;

    use crate::{
        note::{
            commitment::ExtractedNoteCommitment, nullifier::testing::arb_nullifier,
            testing::arb_note, TransmittedNoteCiphertext,
        },
        note_encryption::{OrchardDomain, OrchardNoteEncryption},
        primitives::redpallas::{self, testing::arb_valid_spendauth_keypair},
        value::{NoteValue, ValueCommitTrapdoor, ValueCommitment},
        Note,
    };

    use super::Action;

    /// Builds a real, decryptable `TransmittedNoteCiphertext` for `note`,
    /// mirroring `OutputInfo::build`: the same encryptor yields a non-identity
    /// ephemeral public key (satisfying the `Action::from_parts` epk invariant)
    /// together with the note and outgoing ciphertexts. No outgoing viewing key
    /// is used, so `out_ciphertext` is encrypted under a random key, as for a
    /// real output sent without an `ovk`.
    fn encrypted_note_for(
        note: Note,
        cv_net: &ValueCommitment,
        cmx: &ExtractedNoteCommitment,
        mut rng: impl RngCore,
    ) -> TransmittedNoteCiphertext {
        let encryptor = OrchardNoteEncryption::new(None, note, [0u8; 512]);
        TransmittedNoteCiphertext {
            epk_bytes: OrchardDomain::epk_bytes(encryptor.epk()).0,
            enc_ciphertext: encryptor.encrypt_note_plaintext(),
            out_ciphertext: encryptor.encrypt_outgoing_plaintext(cv_net, cmx, &mut rng),
        }
    }

    prop_compose! {
        /// Generate an action without authorization data.
        pub fn arb_unauthorized_action(spend_value: NoteValue, output_value: NoteValue)(
            nf in arb_nullifier(),
            (_, rk) in arb_valid_spendauth_keypair(),
            note in arb_note(output_value),
            rng_seed in prop::array::uniform32(prop::num::u8::ANY),
        ) -> Action<()> {
            let cmx = ExtractedNoteCommitment::from(note.commitment());
            let cv_net = ValueCommitment::derive(
                spend_value - output_value,
                ValueCommitTrapdoor::zero()
            );
            let encrypted_note =
                encrypted_note_for(note, &cv_net, &cmx, StdRng::from_seed(rng_seed));
            Action {
                nf,
                rk,
                cmx,
                encrypted_note,
                cv_net,
                authorization: ()
            }
        }
    }

    prop_compose! {
        /// Generate an action with invalid (random) authorization data.
        pub fn arb_action(spend_value: NoteValue, output_value: NoteValue)(
            nf in arb_nullifier(),
            (rsk, rk) in arb_valid_spendauth_keypair(),
            note in arb_note(output_value),
            enc_rng_seed in prop::array::uniform32(prop::num::u8::ANY),
            rng_seed in prop::array::uniform32(prop::num::u8::ANY),
            fake_sighash in prop::array::uniform32(prop::num::u8::ANY),
        ) -> Action<redpallas::Signature<SpendAuth>> {
            let cmx = ExtractedNoteCommitment::from(note.commitment());
            let cv_net = ValueCommitment::derive(
                spend_value - output_value,
                ValueCommitTrapdoor::zero()
            );

            let encrypted_note =
                encrypted_note_for(note, &cv_net, &cmx, StdRng::from_seed(enc_rng_seed));

            let rng = StdRng::from_seed(rng_seed);

            Action {
                nf,
                rk,
                cmx,
                encrypted_note,
                cv_net,
                authorization: rsk.sign(rng, &fake_sighash),
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use group::ff::{Field as _, PrimeField as _};
    use group::{Group as _, GroupEncoding as _};
    use pasta_curves::pallas;

    use super::{Action, ActionFromPartsError};
    use crate::{
        note::{ExtractedNoteCommitment, Nullifier, TransmittedNoteCiphertext},
        primitives::redpallas::{self, SpendAuth},
        value::{ValueCommitTrapdoor, ValueCommitment, ValueSum},
    };

    /// The canonical Pallas encoding of the identity is [0u8; 32]; plain
    /// redpallas accepts it as a `VerificationKey<SpendAuth>`.
    fn identity_rk() -> redpallas::VerificationKey<SpendAuth> {
        redpallas::VerificationKey::<SpendAuth>::try_from([0u8; 32])
            .expect("plain redpallas accepts the identity encoding")
    }

    /// The verification key derived from a signing key with scalar 1 is the
    /// SpendAuthSig basepoint G, which is not the identity.
    fn non_identity_rk() -> redpallas::VerificationKey<SpendAuth> {
        let ask_bytes: [u8; 32] = pallas::Scalar::ONE.to_repr();
        let ask =
            redpallas::SigningKey::<SpendAuth>::try_from(ask_bytes).expect("1 is a valid scalar");
        (&ask).into()
    }

    /// Arbitrary, individually-valid values for the non-`rk` fields of an
    /// `Action`. Distinct non-zero patterns (rather than all-zeros) avoid
    /// accidental overlap with sentinel values.
    ///
    /// `epk_bytes` must encode a non-identity Pallas point for
    /// `Action::from_parts` to accept the action (it rejects an identity or
    /// non-decodable `epk`, the `KA^{Orchard}.Public = ℙ*` type), so we use the
    /// encoding of the group generator. `cv_net` is an arbitrary value
    /// commitment; its own Pallas-point type check lives at deserialization in
    /// `ValueCommitment::from_bytes` (e.g. `src/pczt/parse.rs`).
    fn dummy_other_fields() -> (
        Nullifier,
        ExtractedNoteCommitment,
        TransmittedNoteCiphertext,
        ValueCommitment,
    ) {
        let nf = Nullifier::from_bytes(&[1u8; 32]).unwrap();
        let cmx = ExtractedNoteCommitment::from_bytes(&[2u8; 32]).unwrap();
        let encrypted_note = TransmittedNoteCiphertext {
            epk_bytes: pallas::Point::generator().to_bytes(),
            enc_ciphertext: [4u8; 580],
            out_ciphertext: [5u8; 80],
        };
        let cv_net = ValueCommitment::derive(ValueSum::from_raw(42), ValueCommitTrapdoor::zero());
        (nf, cmx, encrypted_note, cv_net)
    }

    #[test]
    fn is_identity_detects_identity() {
        assert!(identity_rk().is_identity());
    }

    #[test]
    fn is_identity_rejects_non_identity() {
        assert!(!non_identity_rk().is_identity());
    }

    #[test]
    fn from_parts_rejects_identity_rk() {
        let (nf, cmx, encrypted_note, cv_net) = dummy_other_fields();
        let result = Action::from_parts(nf, identity_rk(), cmx, encrypted_note, cv_net, ());
        assert!(matches!(result, Err(ActionFromPartsError::IdentityRk)));
    }

    #[test]
    fn from_parts_accepts_non_identity_rk() {
        let (nf, cmx, encrypted_note, cv_net) = dummy_other_fields();
        let rk = non_identity_rk();
        let action = Action::from_parts(nf, rk.clone(), cmx, encrypted_note, cv_net, ())
            .expect("non-identity rk must be accepted");
        assert_eq!(action.rk, rk);
    }

    #[test]
    fn from_parts_rejects_identity_epk() {
        // The canonical Pallas encoding of the identity is `[0u8; 32]`; an
        // action whose `epk` decodes to the identity must be rejected even
        // when `rk` is valid.
        let (nf, cmx, mut encrypted_note, cv_net) = dummy_other_fields();
        encrypted_note.epk_bytes = [0u8; 32];
        let result = Action::from_parts(nf, non_identity_rk(), cmx, encrypted_note, cv_net, ());
        assert!(matches!(result, Err(ActionFromPartsError::InvalidEpk)));
    }

    #[test]
    fn from_parts_rejects_undecodable_epk() {
        // An `epk` that is not a valid Pallas point encoding is rejected: it
        // cannot be a `KA^{Orchard}` public key. `[0xff; 32]` is a non-canonical
        // (out-of-range) encoding.
        let (nf, cmx, mut encrypted_note, cv_net) = dummy_other_fields();
        encrypted_note.epk_bytes = [0xff; 32];
        let result = Action::from_parts(nf, non_identity_rk(), cmx, encrypted_note, cv_net, ());
        assert!(matches!(result, Err(ActionFromPartsError::InvalidEpk)));
    }
}
