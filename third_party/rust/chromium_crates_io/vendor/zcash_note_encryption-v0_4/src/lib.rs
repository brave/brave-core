//! Note encryption for Zcash transactions.
//!
//! This crate implements the [in-band secret distribution scheme] for the Sapling and
//! Orchard protocols. It provides reusable methods that implement common note encryption
//! and trial decryption logic, and enforce protocol-agnostic verification requirements.
//!
//! Protocol-specific logic is handled via the [`Domain`] trait. Implementations of this
//! trait are provided in the [`zcash_primitives`] (for Sapling) and [`orchard`] crates;
//! users with their own existing types can similarly implement the trait themselves.
//!
//! [in-band secret distribution scheme]: https://zips.z.cash/protocol/protocol.pdf#saplingandorchardinband
//! [`zcash_primitives`]: https://crates.io/crates/zcash_primitives
//! [`orchard`]: https://crates.io/crates/orchard

#![no_std]
#![cfg_attr(docsrs, feature(doc_cfg))]
// Catch documentation errors caused by code changes.
#![deny(rustdoc::broken_intra_doc_links)]
#![deny(unsafe_code)]
// TODO: #![deny(missing_docs)]

#[cfg(feature = "alloc")]
extern crate alloc;
#[cfg(feature = "alloc")]
use alloc::vec::Vec;

use chacha20::{
    cipher::{StreamCipher, StreamCipherSeek},
    ChaCha20,
};
use chacha20poly1305::{aead::AeadInPlace, ChaCha20Poly1305, KeyInit};
use cipher::KeyIvInit;

use rand_core::RngCore;
use subtle::{Choice, ConstantTimeEq};

#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
pub mod batch;

/// The size of a compact note.
pub const COMPACT_NOTE_SIZE: usize = 1 + // version
    11 + // diversifier
    8  + // value
    32; // rseed (or rcm prior to ZIP 212)
/// The size of [`NotePlaintextBytes`].
pub const NOTE_PLAINTEXT_SIZE: usize = COMPACT_NOTE_SIZE + 512;
/// The size of [`OutPlaintextBytes`].
pub const OUT_PLAINTEXT_SIZE: usize = 32 + // pk_d
    32; // esk
const AEAD_TAG_SIZE: usize = 16;
/// The size of an encrypted note plaintext.
pub const ENC_CIPHERTEXT_SIZE: usize = NOTE_PLAINTEXT_SIZE + AEAD_TAG_SIZE;
/// The size of an encrypted outgoing plaintext.
pub const OUT_CIPHERTEXT_SIZE: usize = OUT_PLAINTEXT_SIZE + AEAD_TAG_SIZE;

/// A symmetric key that can be used to recover a single Sapling or Orchard output.
pub struct OutgoingCipherKey(pub [u8; 32]);

impl From<[u8; 32]> for OutgoingCipherKey {
    fn from(ock: [u8; 32]) -> Self {
        OutgoingCipherKey(ock)
    }
}

impl AsRef<[u8]> for OutgoingCipherKey {
    fn as_ref(&self) -> &[u8] {
        &self.0
    }
}

/// Newtype representing the byte encoding of an [`EphemeralPublicKey`].
///
/// [`EphemeralPublicKey`]: Domain::EphemeralPublicKey
#[derive(Clone, Debug)]
pub struct EphemeralKeyBytes(pub [u8; 32]);

impl AsRef<[u8]> for EphemeralKeyBytes {
    fn as_ref(&self) -> &[u8] {
        &self.0
    }
}

impl From<[u8; 32]> for EphemeralKeyBytes {
    fn from(value: [u8; 32]) -> EphemeralKeyBytes {
        EphemeralKeyBytes(value)
    }
}

impl ConstantTimeEq for EphemeralKeyBytes {
    fn ct_eq(&self, other: &Self) -> Choice {
        self.0.ct_eq(&other.0)
    }
}

/// Newtype representing the byte encoding of a note plaintext.
pub struct NotePlaintextBytes(pub [u8; NOTE_PLAINTEXT_SIZE]);
/// Newtype representing the byte encoding of a outgoing plaintext.
pub struct OutPlaintextBytes(pub [u8; OUT_PLAINTEXT_SIZE]);

#[derive(Copy, Clone, PartialEq, Eq)]
enum NoteValidity {
    Valid,
    Invalid,
}

/// Trait that encapsulates protocol-specific note encryption types and logic.
///
/// This trait enables most of the note encryption logic to be shared between Sapling and
/// Orchard, as well as between different implementations of those protocols.
pub trait Domain {
    type EphemeralSecretKey: ConstantTimeEq;
    type EphemeralPublicKey;
    type PreparedEphemeralPublicKey;
    type SharedSecret;
    type SymmetricKey: AsRef<[u8]>;
    type Note;
    type Recipient;
    type DiversifiedTransmissionKey;
    type IncomingViewingKey;
    type OutgoingViewingKey;
    type ValueCommitment;
    type ExtractedCommitment;
    type ExtractedCommitmentBytes: Eq + for<'a> From<&'a Self::ExtractedCommitment>;
    type Memo;

    /// Derives the `EphemeralSecretKey` corresponding to this note.
    ///
    /// Returns `None` if the note was created prior to [ZIP 212], and doesn't have a
    /// deterministic `EphemeralSecretKey`.
    ///
    /// [ZIP 212]: https://zips.z.cash/zip-0212
    fn derive_esk(note: &Self::Note) -> Option<Self::EphemeralSecretKey>;

    /// Extracts the `DiversifiedTransmissionKey` from the note.
    fn get_pk_d(note: &Self::Note) -> Self::DiversifiedTransmissionKey;

    /// Prepare an ephemeral public key for more efficient scalar multiplication.
    fn prepare_epk(epk: Self::EphemeralPublicKey) -> Self::PreparedEphemeralPublicKey;

    /// Derives `EphemeralPublicKey` from `esk` and the note's diversifier.
    fn ka_derive_public(
        note: &Self::Note,
        esk: &Self::EphemeralSecretKey,
    ) -> Self::EphemeralPublicKey;

    /// Derives the `SharedSecret` from the sender's information during note encryption.
    fn ka_agree_enc(
        esk: &Self::EphemeralSecretKey,
        pk_d: &Self::DiversifiedTransmissionKey,
    ) -> Self::SharedSecret;

    /// Derives the `SharedSecret` from the recipient's information during note trial
    /// decryption.
    fn ka_agree_dec(
        ivk: &Self::IncomingViewingKey,
        epk: &Self::PreparedEphemeralPublicKey,
    ) -> Self::SharedSecret;

    /// Derives the `SymmetricKey` used to encrypt the note plaintext.
    ///
    /// `secret` is the `SharedSecret` obtained from [`Self::ka_agree_enc`] or
    /// [`Self::ka_agree_dec`].
    ///
    /// `ephemeral_key` is the byte encoding of the [`EphemeralPublicKey`] used to derive
    /// `secret`. During encryption it is derived via [`Self::epk_bytes`]; during trial
    /// decryption it is obtained from [`ShieldedOutput::ephemeral_key`].
    ///
    /// [`EphemeralPublicKey`]: Self::EphemeralPublicKey
    /// [`EphemeralSecretKey`]: Self::EphemeralSecretKey
    fn kdf(secret: Self::SharedSecret, ephemeral_key: &EphemeralKeyBytes) -> Self::SymmetricKey;

    /// Encodes the given `Note` and `Memo` as a note plaintext.
    fn note_plaintext_bytes(note: &Self::Note, memo: &Self::Memo) -> NotePlaintextBytes;

    /// Derives the [`OutgoingCipherKey`] for an encrypted note, given the note-specific
    /// public data and an `OutgoingViewingKey`.
    fn derive_ock(
        ovk: &Self::OutgoingViewingKey,
        cv: &Self::ValueCommitment,
        cmstar_bytes: &Self::ExtractedCommitmentBytes,
        ephemeral_key: &EphemeralKeyBytes,
    ) -> OutgoingCipherKey;

    /// Encodes the outgoing plaintext for the given note.
    fn outgoing_plaintext_bytes(
        note: &Self::Note,
        esk: &Self::EphemeralSecretKey,
    ) -> OutPlaintextBytes;

    /// Returns the byte encoding of the given `EphemeralPublicKey`.
    fn epk_bytes(epk: &Self::EphemeralPublicKey) -> EphemeralKeyBytes;

    /// Attempts to parse `ephemeral_key` as an `EphemeralPublicKey`.
    ///
    /// Returns `None` if `ephemeral_key` is not a valid byte encoding of an
    /// `EphemeralPublicKey`.
    fn epk(ephemeral_key: &EphemeralKeyBytes) -> Option<Self::EphemeralPublicKey>;

    /// Derives the `ExtractedCommitment` for this note.
    fn cmstar(note: &Self::Note) -> Self::ExtractedCommitment;

    /// Parses the given note plaintext from the recipient's perspective.
    ///
    /// The implementation of this method must check that:
    /// - The note plaintext version is valid (for the given decryption domain's context,
    ///   which may be passed via `self`).
    /// - The note plaintext contains valid encodings of its various fields.
    /// - Any domain-specific requirements are satisfied.
    ///
    /// `&self` is passed here to enable the implementation to enforce contextual checks,
    /// such as rules like [ZIP 212] that become active at a specific block height.
    ///
    /// [ZIP 212]: https://zips.z.cash/zip-0212
    ///
    /// # Panics
    ///
    /// Panics if `plaintext` is shorter than [`COMPACT_NOTE_SIZE`].
    fn parse_note_plaintext_without_memo_ivk(
        &self,
        ivk: &Self::IncomingViewingKey,
        plaintext: &[u8],
    ) -> Option<(Self::Note, Self::Recipient)>;

    /// Parses the given note plaintext from the sender's perspective.
    ///
    /// The implementation of this method must check that:
    /// - The note plaintext version is valid (for the given decryption domain's context,
    ///   which may be passed via `self`).
    /// - The note plaintext contains valid encodings of its various fields.
    /// - Any domain-specific requirements are satisfied.
    ///
    /// `&self` is passed here to enable the implementation to enforce contextual checks,
    /// such as rules like [ZIP 212] that become active at a specific block height.
    ///
    /// [ZIP 212]: https://zips.z.cash/zip-0212
    fn parse_note_plaintext_without_memo_ovk(
        &self,
        pk_d: &Self::DiversifiedTransmissionKey,
        plaintext: &NotePlaintextBytes,
    ) -> Option<(Self::Note, Self::Recipient)>;

    /// Extracts the memo field from the given note plaintext.
    ///
    /// # Compatibility
    ///
    /// `&self` is passed here in anticipation of future changes to memo handling, where
    /// the memos may no longer be part of the note plaintext.
    fn extract_memo(&self, plaintext: &NotePlaintextBytes) -> Self::Memo;

    /// Parses the `DiversifiedTransmissionKey` field of the outgoing plaintext.
    ///
    /// Returns `None` if `out_plaintext` does not contain a valid byte encoding of a
    /// `DiversifiedTransmissionKey`.
    fn extract_pk_d(out_plaintext: &OutPlaintextBytes) -> Option<Self::DiversifiedTransmissionKey>;

    /// Parses the `EphemeralSecretKey` field of the outgoing plaintext.
    ///
    /// Returns `None` if `out_plaintext` does not contain a valid byte encoding of an
    /// `EphemeralSecretKey`.
    fn extract_esk(out_plaintext: &OutPlaintextBytes) -> Option<Self::EphemeralSecretKey>;
}

/// Trait that encapsulates protocol-specific batch trial decryption logic.
///
/// Each batchable operation has a default implementation that calls through to the
/// non-batched implementation. Domains can override whichever operations benefit from
/// batched logic.
#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
pub trait BatchDomain: Domain {
    /// Computes `Self::kdf` on a batch of items.
    ///
    /// For each item in the batch, if the shared secret is `None`, this returns `None` at
    /// that position.
    fn batch_kdf<'a>(
        items: impl Iterator<Item = (Option<Self::SharedSecret>, &'a EphemeralKeyBytes)>,
    ) -> Vec<Option<Self::SymmetricKey>> {
        // Default implementation: do the non-batched thing.
        items
            .map(|(secret, ephemeral_key)| secret.map(|secret| Self::kdf(secret, ephemeral_key)))
            .collect()
    }

    /// Computes `Self::epk` on a batch of ephemeral keys.
    ///
    /// This is useful for protocols where the underlying curve requires an inversion to
    /// parse an encoded point.
    ///
    /// For usability, this returns tuples of the ephemeral keys and the result of parsing
    /// them.
    fn batch_epk(
        ephemeral_keys: impl Iterator<Item = EphemeralKeyBytes>,
    ) -> Vec<(Option<Self::PreparedEphemeralPublicKey>, EphemeralKeyBytes)> {
        // Default implementation: do the non-batched thing.
        ephemeral_keys
            .map(|ephemeral_key| {
                (
                    Self::epk(&ephemeral_key).map(Self::prepare_epk),
                    ephemeral_key,
                )
            })
            .collect()
    }
}

/// Trait that provides access to the components of an encrypted transaction output.
///
/// Implementations of this trait are required to define the length of their ciphertext
/// field. In order to use the trial decryption APIs in this crate, the length must be
/// either [`ENC_CIPHERTEXT_SIZE`] or [`COMPACT_NOTE_SIZE`].
pub trait ShieldedOutput<D: Domain, const CIPHERTEXT_SIZE: usize> {
    /// Exposes the `ephemeral_key` field of the output.
    fn ephemeral_key(&self) -> EphemeralKeyBytes;

    /// Exposes the `cmu_bytes` or `cmx_bytes` field of the output.
    fn cmstar_bytes(&self) -> D::ExtractedCommitmentBytes;

    /// Exposes the note ciphertext of the output.
    fn enc_ciphertext(&self) -> &[u8; CIPHERTEXT_SIZE];
}

/// A struct containing context required for encrypting Sapling and Orchard notes.
///
/// This struct provides a safe API for encrypting Sapling and Orchard notes. In particular, it
/// enforces that fresh ephemeral keys are used for every note, and that the ciphertexts are
/// consistent with each other.
///
/// Implements section 4.19 of the
/// [Zcash Protocol Specification](https://zips.z.cash/protocol/nu5.pdf#saplingandorchardinband)
pub struct NoteEncryption<D: Domain> {
    epk: D::EphemeralPublicKey,
    esk: D::EphemeralSecretKey,
    note: D::Note,
    memo: D::Memo,
    /// `None` represents the `ovk = ⊥` case.
    ovk: Option<D::OutgoingViewingKey>,
}

impl<D: Domain> NoteEncryption<D> {
    /// Construct a new note encryption context for the specified note,
    /// recipient, and memo.
    pub fn new(ovk: Option<D::OutgoingViewingKey>, note: D::Note, memo: D::Memo) -> Self {
        let esk = D::derive_esk(&note).expect("ZIP 212 is active.");
        NoteEncryption {
            epk: D::ka_derive_public(&note, &esk),
            esk,
            note,
            memo,
            ovk,
        }
    }

    /// For use only with Sapling. This method is preserved in order that test code
    /// be able to generate pre-ZIP-212 ciphertexts so that tests can continue to
    /// cover pre-ZIP-212 transaction decryption.
    #[cfg(feature = "pre-zip-212")]
    #[cfg_attr(docsrs, doc(cfg(feature = "pre-zip-212")))]
    pub fn new_with_esk(
        esk: D::EphemeralSecretKey,
        ovk: Option<D::OutgoingViewingKey>,
        note: D::Note,
        memo: D::Memo,
    ) -> Self {
        NoteEncryption {
            epk: D::ka_derive_public(&note, &esk),
            esk,
            note,
            memo,
            ovk,
        }
    }

    /// Exposes the ephemeral secret key being used to encrypt this note.
    pub fn esk(&self) -> &D::EphemeralSecretKey {
        &self.esk
    }

    /// Exposes the encoding of the ephemeral public key being used to encrypt this note.
    pub fn epk(&self) -> &D::EphemeralPublicKey {
        &self.epk
    }

    /// Generates `encCiphertext` for this note.
    pub fn encrypt_note_plaintext(&self) -> [u8; ENC_CIPHERTEXT_SIZE] {
        let pk_d = D::get_pk_d(&self.note);
        let shared_secret = D::ka_agree_enc(&self.esk, &pk_d);
        let key = D::kdf(shared_secret, &D::epk_bytes(&self.epk));
        let input = D::note_plaintext_bytes(&self.note, &self.memo);

        let mut output = [0u8; ENC_CIPHERTEXT_SIZE];
        output[..NOTE_PLAINTEXT_SIZE].copy_from_slice(&input.0);
        let tag = ChaCha20Poly1305::new(key.as_ref().into())
            .encrypt_in_place_detached(
                [0u8; 12][..].into(),
                &[],
                &mut output[..NOTE_PLAINTEXT_SIZE],
            )
            .unwrap();
        output[NOTE_PLAINTEXT_SIZE..].copy_from_slice(&tag);

        output
    }

    /// Generates `outCiphertext` for this note.
    pub fn encrypt_outgoing_plaintext<R: RngCore>(
        &self,
        cv: &D::ValueCommitment,
        cmstar: &D::ExtractedCommitment,
        rng: &mut R,
    ) -> [u8; OUT_CIPHERTEXT_SIZE] {
        let (ock, input) = if let Some(ovk) = &self.ovk {
            let ock = D::derive_ock(ovk, cv, &cmstar.into(), &D::epk_bytes(&self.epk));
            let input = D::outgoing_plaintext_bytes(&self.note, &self.esk);

            (ock, input)
        } else {
            // ovk = ⊥
            let mut ock = OutgoingCipherKey([0; 32]);
            let mut input = [0u8; OUT_PLAINTEXT_SIZE];

            rng.fill_bytes(&mut ock.0);
            rng.fill_bytes(&mut input);

            (ock, OutPlaintextBytes(input))
        };

        let mut output = [0u8; OUT_CIPHERTEXT_SIZE];
        output[..OUT_PLAINTEXT_SIZE].copy_from_slice(&input.0);
        let tag = ChaCha20Poly1305::new(ock.as_ref().into())
            .encrypt_in_place_detached([0u8; 12][..].into(), &[], &mut output[..OUT_PLAINTEXT_SIZE])
            .unwrap();
        output[OUT_PLAINTEXT_SIZE..].copy_from_slice(&tag);

        output
    }
}

/// Trial decryption of the full note plaintext by the recipient.
///
/// Attempts to decrypt and validate the given shielded output using the given `ivk`.
/// If successful, the corresponding note and memo are returned, along with the address to
/// which the note was sent.
///
/// Implements section 4.19.2 of the
/// [Zcash Protocol Specification](https://zips.z.cash/protocol/nu5.pdf#decryptivk).
pub fn try_note_decryption<D: Domain, Output: ShieldedOutput<D, ENC_CIPHERTEXT_SIZE>>(
    domain: &D,
    ivk: &D::IncomingViewingKey,
    output: &Output,
) -> Option<(D::Note, D::Recipient, D::Memo)> {
    let ephemeral_key = output.ephemeral_key();

    let epk = D::prepare_epk(D::epk(&ephemeral_key)?);
    let shared_secret = D::ka_agree_dec(ivk, &epk);
    let key = D::kdf(shared_secret, &ephemeral_key);

    try_note_decryption_inner(domain, ivk, &ephemeral_key, output, &key)
}

fn try_note_decryption_inner<D: Domain, Output: ShieldedOutput<D, ENC_CIPHERTEXT_SIZE>>(
    domain: &D,
    ivk: &D::IncomingViewingKey,
    ephemeral_key: &EphemeralKeyBytes,
    output: &Output,
    key: &D::SymmetricKey,
) -> Option<(D::Note, D::Recipient, D::Memo)> {
    let enc_ciphertext = output.enc_ciphertext();

    let mut plaintext =
        NotePlaintextBytes(enc_ciphertext[..NOTE_PLAINTEXT_SIZE].try_into().unwrap());

    ChaCha20Poly1305::new(key.as_ref().into())
        .decrypt_in_place_detached(
            [0u8; 12][..].into(),
            &[],
            &mut plaintext.0,
            enc_ciphertext[NOTE_PLAINTEXT_SIZE..].into(),
        )
        .ok()?;

    let (note, to) = parse_note_plaintext_without_memo_ivk(
        domain,
        ivk,
        ephemeral_key,
        &output.cmstar_bytes(),
        &plaintext.0,
    )?;
    let memo = domain.extract_memo(&plaintext);

    Some((note, to, memo))
}

fn parse_note_plaintext_without_memo_ivk<D: Domain>(
    domain: &D,
    ivk: &D::IncomingViewingKey,
    ephemeral_key: &EphemeralKeyBytes,
    cmstar_bytes: &D::ExtractedCommitmentBytes,
    plaintext: &[u8],
) -> Option<(D::Note, D::Recipient)> {
    let (note, to) = domain.parse_note_plaintext_without_memo_ivk(ivk, plaintext)?;

    if let NoteValidity::Valid = check_note_validity::<D>(&note, ephemeral_key, cmstar_bytes) {
        Some((note, to))
    } else {
        None
    }
}

fn check_note_validity<D: Domain>(
    note: &D::Note,
    ephemeral_key: &EphemeralKeyBytes,
    cmstar_bytes: &D::ExtractedCommitmentBytes,
) -> NoteValidity {
    if &D::ExtractedCommitmentBytes::from(&D::cmstar(note)) == cmstar_bytes {
        // In the case corresponding to specification section 4.19.3, we check that `esk` is equal
        // to `D::derive_esk(note)` prior to calling this method.
        if let Some(derived_esk) = D::derive_esk(note) {
            if D::epk_bytes(&D::ka_derive_public(note, &derived_esk))
                .ct_eq(ephemeral_key)
                .into()
            {
                NoteValidity::Valid
            } else {
                NoteValidity::Invalid
            }
        } else {
            // Before ZIP 212
            NoteValidity::Valid
        }
    } else {
        // Published commitment doesn't match calculated commitment
        NoteValidity::Invalid
    }
}

/// Trial decryption of the compact note plaintext by the recipient for light clients.
///
/// Attempts to decrypt and validate the given compact shielded output using the
/// given `ivk`. If successful, the corresponding note is returned, along with the address
/// to which the note was sent.
///
/// Implements the procedure specified in [`ZIP 307`].
///
/// [`ZIP 307`]: https://zips.z.cash/zip-0307
pub fn try_compact_note_decryption<D: Domain, Output: ShieldedOutput<D, COMPACT_NOTE_SIZE>>(
    domain: &D,
    ivk: &D::IncomingViewingKey,
    output: &Output,
) -> Option<(D::Note, D::Recipient)> {
    let ephemeral_key = output.ephemeral_key();

    let epk = D::prepare_epk(D::epk(&ephemeral_key)?);
    let shared_secret = D::ka_agree_dec(ivk, &epk);
    let key = D::kdf(shared_secret, &ephemeral_key);

    try_compact_note_decryption_inner(domain, ivk, &ephemeral_key, output, &key)
}

fn try_compact_note_decryption_inner<D: Domain, Output: ShieldedOutput<D, COMPACT_NOTE_SIZE>>(
    domain: &D,
    ivk: &D::IncomingViewingKey,
    ephemeral_key: &EphemeralKeyBytes,
    output: &Output,
    key: &D::SymmetricKey,
) -> Option<(D::Note, D::Recipient)> {
    // Start from block 1 to skip over Poly1305 keying output
    let mut plaintext = [0; COMPACT_NOTE_SIZE];
    plaintext.copy_from_slice(output.enc_ciphertext());
    let mut keystream = ChaCha20::new(key.as_ref().into(), [0u8; 12][..].into());
    keystream.seek(64);
    keystream.apply_keystream(&mut plaintext);

    parse_note_plaintext_without_memo_ivk(
        domain,
        ivk,
        ephemeral_key,
        &output.cmstar_bytes(),
        &plaintext,
    )
}

/// Recovery of the full note plaintext by the sender.
///
/// Attempts to decrypt and validate the given shielded output using the given `ovk`.
/// If successful, the corresponding note and memo are returned, along with the address to
/// which the note was sent.
///
/// Implements [Zcash Protocol Specification section 4.19.3][decryptovk].
///
/// [decryptovk]: https://zips.z.cash/protocol/nu5.pdf#decryptovk
pub fn try_output_recovery_with_ovk<D: Domain, Output: ShieldedOutput<D, ENC_CIPHERTEXT_SIZE>>(
    domain: &D,
    ovk: &D::OutgoingViewingKey,
    output: &Output,
    cv: &D::ValueCommitment,
    out_ciphertext: &[u8; OUT_CIPHERTEXT_SIZE],
) -> Option<(D::Note, D::Recipient, D::Memo)> {
    let ock = D::derive_ock(ovk, cv, &output.cmstar_bytes(), &output.ephemeral_key());
    try_output_recovery_with_ock(domain, &ock, output, out_ciphertext)
}

/// Recovery of the full note plaintext by the sender.
///
/// Attempts to decrypt and validate the given shielded output using the given `ock`.
/// If successful, the corresponding note and memo are returned, along with the address to
/// which the note was sent.
///
/// Implements part of section 4.19.3 of the
/// [Zcash Protocol Specification](https://zips.z.cash/protocol/nu5.pdf#decryptovk).
/// For decryption using a Full Viewing Key see [`try_output_recovery_with_ovk`].
pub fn try_output_recovery_with_ock<D: Domain, Output: ShieldedOutput<D, ENC_CIPHERTEXT_SIZE>>(
    domain: &D,
    ock: &OutgoingCipherKey,
    output: &Output,
    out_ciphertext: &[u8; OUT_CIPHERTEXT_SIZE],
) -> Option<(D::Note, D::Recipient, D::Memo)> {
    let enc_ciphertext = output.enc_ciphertext();

    let mut op = OutPlaintextBytes([0; OUT_PLAINTEXT_SIZE]);
    op.0.copy_from_slice(&out_ciphertext[..OUT_PLAINTEXT_SIZE]);

    ChaCha20Poly1305::new(ock.as_ref().into())
        .decrypt_in_place_detached(
            [0u8; 12][..].into(),
            &[],
            &mut op.0,
            out_ciphertext[OUT_PLAINTEXT_SIZE..].into(),
        )
        .ok()?;

    let pk_d = D::extract_pk_d(&op)?;
    let esk = D::extract_esk(&op)?;

    let ephemeral_key = output.ephemeral_key();
    let shared_secret = D::ka_agree_enc(&esk, &pk_d);
    // The small-order point check at the point of output parsing rejects
    // non-canonical encodings, so reencoding here for the KDF should
    // be okay.
    let key = D::kdf(shared_secret, &ephemeral_key);

    let mut plaintext = NotePlaintextBytes([0; NOTE_PLAINTEXT_SIZE]);
    plaintext
        .0
        .copy_from_slice(&enc_ciphertext[..NOTE_PLAINTEXT_SIZE]);

    ChaCha20Poly1305::new(key.as_ref().into())
        .decrypt_in_place_detached(
            [0u8; 12][..].into(),
            &[],
            &mut plaintext.0,
            enc_ciphertext[NOTE_PLAINTEXT_SIZE..].into(),
        )
        .ok()?;

    let (note, to) = domain.parse_note_plaintext_without_memo_ovk(&pk_d, &plaintext)?;
    let memo = domain.extract_memo(&plaintext);

    // ZIP 212: Check that the esk provided to this function is consistent with the esk we can
    // derive from the note. This check corresponds to `ToScalar(PRF^{expand}_{rseed}([4]) = esk`
    // in https://zips.z.cash/protocol/protocol.pdf#decryptovk. (`ρ^opt = []` for Sapling.)
    if let Some(derived_esk) = D::derive_esk(&note) {
        if (!derived_esk.ct_eq(&esk)).into() {
            return None;
        }
    }

    if let NoteValidity::Valid =
        check_note_validity::<D>(&note, &ephemeral_key, &output.cmstar_bytes())
    {
        Some((note, to, memo))
    } else {
        None
    }
}
