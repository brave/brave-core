use group::{ff::Field, GroupEncoding};
use rand_core::{CryptoRng, RngCore};
use zcash_spec::PrfExpand;

use crate::{
    keys::{ExpandedSpendingKey, FullViewingKey},
    zip32::ExtendedSpendingKey,
};

use super::{
    keys::EphemeralSecretKey, value::NoteValue, Nullifier, NullifierDerivingKey, PaymentAddress,
};

mod commitment;
pub use self::commitment::{ExtractedNoteCommitment, NoteCommitment};

pub(super) mod nullifier;

/// Enum for note randomness before and after [ZIP 212](https://zips.z.cash/zip-0212).
///
/// Before ZIP 212, the note commitment trapdoor `rcm` must be a scalar value.
/// After ZIP 212, the note randomness `rseed` is a 32-byte sequence, used to derive
/// both the note commitment trapdoor `rcm` and the ephemeral private key `esk`.
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum Rseed {
    BeforeZip212(jubjub::Fr),
    AfterZip212([u8; 32]),
}

impl Rseed {
    /// Defined in [Zcash Protocol Spec § 4.7.2: Sending Notes (Sapling)][saplingsend].
    ///
    /// [saplingsend]: https://zips.z.cash/protocol/protocol.pdf#saplingsend
    pub(crate) fn rcm(&self) -> commitment::NoteCommitTrapdoor {
        commitment::NoteCommitTrapdoor(match self {
            Rseed::BeforeZip212(rcm) => *rcm,
            Rseed::AfterZip212(rseed) => {
                jubjub::Fr::from_bytes_wide(&PrfExpand::SAPLING_RCM.with(rseed))
            }
        })
    }
}

/// A discrete amount of funds received by an address.
#[derive(Clone, Debug)]
pub struct Note {
    /// The recipient of the funds.
    recipient: PaymentAddress,
    /// The value of this note.
    value: NoteValue,
    /// The seed randomness for various note components.
    rseed: Rseed,
}

impl PartialEq for Note {
    fn eq(&self, other: &Self) -> bool {
        // Notes are canonically defined by their commitments.
        self.cmu().eq(&other.cmu())
    }
}

impl Eq for Note {}

impl Note {
    /// Creates a note from its component parts.
    ///
    /// # Caveats
    ///
    /// This low-level constructor enforces that the provided arguments produce an
    /// internally valid `Note`. However, it allows notes to be constructed in a way that
    /// violates required security checks for note decryption, as specified in
    /// [Section 4.19] of the Zcash Protocol Specification. Users of this constructor
    /// should only call it with note components that have been fully validated by
    /// decrypting a received note according to [Section 4.19].
    ///
    /// [Section 4.19]: https://zips.z.cash/protocol/protocol.pdf#saplingandorchardinband
    pub fn from_parts(recipient: PaymentAddress, value: NoteValue, rseed: Rseed) -> Self {
        Note {
            recipient,
            value,
            rseed,
        }
    }

    /// Returns the recipient of this note.
    pub fn recipient(&self) -> PaymentAddress {
        self.recipient
    }

    /// Returns the value of this note.
    pub fn value(&self) -> NoteValue {
        self.value
    }

    /// Returns the rseed value of this note.
    pub fn rseed(&self) -> &Rseed {
        &self.rseed
    }

    /// Computes the note commitment, returning the full point.
    fn cm_full_point(&self) -> NoteCommitment {
        NoteCommitment::derive(
            self.recipient.g_d().to_bytes(),
            self.recipient.pk_d().to_bytes(),
            self.value,
            self.rseed.rcm(),
        )
    }

    /// Computes the nullifier given the nullifier deriving key and
    /// note position
    pub fn nf(&self, nk: &NullifierDerivingKey, position: u64) -> Nullifier {
        Nullifier::derive(nk, self.cm_full_point(), position)
    }

    /// Computes the note commitment
    pub fn cmu(&self) -> ExtractedNoteCommitment {
        self.cm_full_point().into()
    }

    /// Defined in [Zcash Protocol Spec § 4.7.2: Sending Notes (Sapling)][saplingsend].
    ///
    /// [saplingsend]: https://zips.z.cash/protocol/protocol.pdf#saplingsend
    pub fn rcm(&self) -> jubjub::Fr {
        self.rseed.rcm().0
    }

    /// Derives `esk` from the internal `Rseed` value, or generates a random value if this
    /// note was created with a v1 (i.e. pre-ZIP 212) note plaintext.
    pub fn generate_or_derive_esk<R: RngCore + CryptoRng>(
        &self,
        rng: &mut R,
    ) -> EphemeralSecretKey {
        self.generate_or_derive_esk_internal(rng)
    }

    pub(crate) fn generate_or_derive_esk_internal<R: RngCore>(
        &self,
        rng: &mut R,
    ) -> EphemeralSecretKey {
        match self.derive_esk() {
            None => EphemeralSecretKey(jubjub::Fr::random(rng)),
            Some(esk) => esk,
        }
    }

    /// Returns the derived `esk` if this note was created after ZIP 212 activated.
    pub(crate) fn derive_esk(&self) -> Option<EphemeralSecretKey> {
        match self.rseed {
            Rseed::BeforeZip212(_) => None,
            Rseed::AfterZip212(rseed) => Some(EphemeralSecretKey(jubjub::Fr::from_bytes_wide(
                &PrfExpand::SAPLING_ESK.with(&rseed),
            ))),
        }
    }

    /// Generates a dummy spent note.
    ///
    /// Defined in [Zcash Protocol Spec § 4.8.2: Dummy Notes (Sapling)][saplingdummynotes].
    ///
    /// [saplingdummynotes]: https://zips.z.cash/protocol/nu5.pdf#saplingdummynotes
    pub(crate) fn dummy<R: RngCore>(mut rng: R) -> (ExpandedSpendingKey, FullViewingKey, Self) {
        let mut sk_bytes = [0; 32];
        rng.fill_bytes(&mut sk_bytes);

        let extsk = ExtendedSpendingKey::master(&sk_bytes[..]);
        let fvk = extsk.to_diversifiable_full_viewing_key().fvk().clone();
        let recipient = extsk.default_address();

        let mut rseed_bytes = [0; 32];
        rng.fill_bytes(&mut rseed_bytes);
        let rseed = Rseed::AfterZip212(rseed_bytes);

        let note = Note::from_parts(recipient.1, NoteValue::ZERO, rseed);

        (extsk.expsk, fvk, note)
    }
}

#[cfg(any(test, feature = "test-dependencies"))]
pub(super) mod testing {
    use proptest::{collection::vec, prelude::*};

    use super::{
        super::{testing::arb_payment_address, value::NoteValue},
        ExtractedNoteCommitment, Note, Rseed,
    };

    prop_compose! {
        pub fn arb_note(value: NoteValue)(
            recipient in arb_payment_address(),
            rseed in prop::array::uniform32(prop::num::u8::ANY).prop_map(Rseed::AfterZip212)
        ) -> Note {
            Note {
                recipient,
                value,
                rseed
            }
        }
    }

    prop_compose! {
        pub(crate) fn arb_cmu()(
            cmu in vec(any::<u8>(), 64)
                .prop_map(|v| <[u8;64]>::try_from(v.as_slice()).unwrap())
                .prop_map(|v| bls12_381::Scalar::from_bytes_wide(&v)),
        ) -> ExtractedNoteCommitment {
            ExtractedNoteCommitment(cmu)
        }
    }
}
