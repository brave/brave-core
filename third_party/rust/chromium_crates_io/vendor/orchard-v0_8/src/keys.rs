//! Key structures for Orchard.

use std::io::{self, Read, Write};

use ::zip32::{AccountId, ChildIndex};
use aes::Aes256;
use blake2b_simd::{Hash as Blake2bHash, Params};
use fpe::ff1::{BinaryNumeralString, FF1};
use group::{
    ff::{Field, PrimeField},
    prime::PrimeCurveAffine,
    Curve, GroupEncoding,
};
use pasta_curves::pallas;
use rand::RngCore;
use subtle::{Choice, ConditionallySelectable, ConstantTimeEq, CtOption};
use zcash_note_encryption::EphemeralKeyBytes;

use crate::{
    address::Address,
    primitives::redpallas::{self, SpendAuth},
    spec::{
        commit_ivk, diversify_hash, extract_p, ka_orchard, ka_orchard_prepared, prf_nf, to_base,
        to_scalar, NonIdentityPallasPoint, NonZeroPallasBase, NonZeroPallasScalar,
        PreparedNonIdentityBase, PreparedNonZeroScalar, PrfExpand,
    },
    zip32::{self, ExtendedSpendingKey},
};

pub use ::zip32::{DiversifierIndex, Scope};

const KDF_ORCHARD_PERSONALIZATION: &[u8; 16] = b"Zcash_OrchardKDF";
const ZIP32_PURPOSE: u32 = 32;

/// A spending key, from which all key material is derived.
///
/// $\mathsf{sk}$ as defined in [Zcash Protocol Spec § 4.2.3: Orchard Key Components][orchardkeycomponents].
///
/// [orchardkeycomponents]: https://zips.z.cash/protocol/nu5.pdf#orchardkeycomponents
#[derive(Debug, Copy, Clone)]
pub struct SpendingKey([u8; 32]);

impl ConstantTimeEq for SpendingKey {
    fn ct_eq(&self, other: &Self) -> Choice {
        self.to_bytes().ct_eq(other.to_bytes())
    }
}

impl SpendingKey {
    /// Generates a random spending key.
    ///
    /// This is only used when generating dummy notes. Real spending keys should be
    /// derived according to [ZIP 32].
    ///
    /// [ZIP 32]: https://zips.z.cash/zip-0032
    pub(crate) fn random(rng: &mut impl RngCore) -> Self {
        loop {
            let mut bytes = [0; 32];
            rng.fill_bytes(&mut bytes);
            let sk = SpendingKey::from_bytes(bytes);
            if sk.is_some().into() {
                break sk.unwrap();
            }
        }
    }

    /// Constructs an Orchard spending key from uniformly-random bytes.
    ///
    /// Returns `None` if the bytes do not correspond to a valid Orchard spending key.
    pub fn from_bytes(sk: [u8; 32]) -> CtOption<Self> {
        let sk = SpendingKey(sk);
        // If ask = 0, discard this key. We call `derive_inner` rather than
        // `SpendAuthorizingKey::from` here because we only need to know
        // whether ask = 0; the adjustment to potentially negate ask is not
        // needed. Also, `from` would panic on ask = 0.
        let ask = SpendAuthorizingKey::derive_inner(&sk);
        // If ivk is 0 or ⊥, discard this key.
        let fvk = (&sk).into();
        let external_ivk = KeyAgreementPrivateKey::derive_inner(&fvk);
        let internal_ivk = KeyAgreementPrivateKey::derive_inner(&fvk.derive_internal());
        CtOption::new(
            sk,
            !(ask.is_zero() | external_ivk.is_none() | internal_ivk.is_none()),
        )
    }

    /// Returns the raw bytes of the spending key.
    pub fn to_bytes(&self) -> &[u8; 32] {
        &self.0
    }

    /// Derives the Orchard spending key for the given seed, coin type, and account.
    pub fn from_zip32_seed(
        seed: &[u8],
        coin_type: u32,
        account: AccountId,
    ) -> Result<Self, zip32::Error> {
        if coin_type >= (1 << 31) {
            return Err(zip32::Error::InvalidChildIndex(coin_type));
        }

        // Call zip32 logic
        let path = &[
            ChildIndex::hardened(ZIP32_PURPOSE),
            ChildIndex::hardened(coin_type),
            ChildIndex::hardened(account.into()),
        ];
        ExtendedSpendingKey::from_path(seed, path).map(|esk| esk.sk())
    }
}

/// A spend authorizing key, used to create spend authorization signatures.
/// This type enforces that the corresponding public point (ak^ℙ) has ỹ = 0.
///
/// $\mathsf{ask}$ as defined in [Zcash Protocol Spec § 4.2.3: Orchard Key Components][orchardkeycomponents].
///
/// [orchardkeycomponents]: https://zips.z.cash/protocol/nu5.pdf#orchardkeycomponents
#[derive(Clone, Debug)]
pub struct SpendAuthorizingKey(redpallas::SigningKey<SpendAuth>);

impl SpendAuthorizingKey {
    /// Derives ask from sk. Internal use only, does not enforce all constraints.
    fn derive_inner(sk: &SpendingKey) -> pallas::Scalar {
        to_scalar(PrfExpand::ORCHARD_ASK.with(&sk.0))
    }

    /// Randomizes this spend authorizing key with the given `randomizer`.
    ///
    /// The resulting key can be used to actually sign a spend.
    pub fn randomize(&self, randomizer: &pallas::Scalar) -> redpallas::SigningKey<SpendAuth> {
        self.0.randomize(randomizer)
    }
}

impl From<&SpendingKey> for SpendAuthorizingKey {
    fn from(sk: &SpendingKey) -> Self {
        let ask = Self::derive_inner(sk);
        // SpendingKey cannot be constructed such that this assertion would fail.
        assert!(!bool::from(ask.is_zero()));
        // TODO: Add TryFrom<S::Scalar> for SpendAuthorizingKey.
        let ret = SpendAuthorizingKey(ask.to_repr().try_into().unwrap());
        // If the last bit of repr_P(ak) is 1, negate ask.
        if (<[u8; 32]>::from(SpendValidatingKey::from(&ret).0)[31] >> 7) == 1 {
            SpendAuthorizingKey((-ask).to_repr().try_into().unwrap())
        } else {
            ret
        }
    }
}

/// A key used to validate spend authorization signatures.
///
/// Defined in [Zcash Protocol Spec § 4.2.3: Orchard Key Components][orchardkeycomponents].
/// Note that this is $\mathsf{ak}^\mathbb{P}$, which by construction is equivalent to
/// $\mathsf{ak}$ but stored here as a RedPallas verification key.
///
/// [orchardkeycomponents]: https://zips.z.cash/protocol/nu5.pdf#orchardkeycomponents
#[derive(Debug, Clone, PartialOrd, Ord)]
pub struct SpendValidatingKey(redpallas::VerificationKey<SpendAuth>);

impl From<&SpendAuthorizingKey> for SpendValidatingKey {
    fn from(ask: &SpendAuthorizingKey) -> Self {
        SpendValidatingKey((&ask.0).into())
    }
}

impl From<&SpendValidatingKey> for pallas::Point {
    fn from(spend_validating_key: &SpendValidatingKey) -> pallas::Point {
        pallas::Point::from_bytes(&(&spend_validating_key.0).into()).unwrap()
    }
}

impl PartialEq for SpendValidatingKey {
    fn eq(&self, other: &Self) -> bool {
        <[u8; 32]>::from(&self.0).eq(&<[u8; 32]>::from(&other.0))
    }
}

impl Eq for SpendValidatingKey {}

impl SpendValidatingKey {
    /// Randomizes this spend validating key with the given `randomizer`.
    pub fn randomize(&self, randomizer: &pallas::Scalar) -> redpallas::VerificationKey<SpendAuth> {
        self.0.randomize(randomizer)
    }

    /// Converts this spend validating key to its serialized form,
    /// I2LEOSP_256(ak).
    pub(crate) fn to_bytes(&self) -> [u8; 32] {
        // This is correct because the wrapped point must have ỹ = 0, and
        // so the point repr is the same as I2LEOSP of its x-coordinate.
        <[u8; 32]>::from(&self.0)
    }

    pub(crate) fn from_bytes(bytes: &[u8]) -> Option<Self> {
        <[u8; 32]>::try_from(bytes)
            .ok()
            .and_then(|b| {
                // Structural validity checks for ak_P:
                // - The point must not be the identity
                //   (which for Pallas is canonically encoded as all-zeroes).
                // - The sign of the y-coordinate must be positive.
                if b != [0; 32] && b[31] & 0x80 == 0 {
                    <redpallas::VerificationKey<SpendAuth>>::try_from(b).ok()
                } else {
                    None
                }
            })
            .map(SpendValidatingKey)
    }
}

/// A key used to derive [`Nullifier`]s from [`Note`]s.
///
/// $\mathsf{nk}$ as defined in [Zcash Protocol Spec § 4.2.3: Orchard Key Components][orchardkeycomponents].
///
/// [`Nullifier`]: crate::note::Nullifier
/// [`Note`]: crate::note::Note
/// [orchardkeycomponents]: https://zips.z.cash/protocol/nu5.pdf#orchardkeycomponents
#[derive(Copy, Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub(crate) struct NullifierDerivingKey(pallas::Base);

impl NullifierDerivingKey {
    pub(crate) fn inner(&self) -> pallas::Base {
        self.0
    }
}

impl From<&SpendingKey> for NullifierDerivingKey {
    fn from(sk: &SpendingKey) -> Self {
        NullifierDerivingKey(to_base(PrfExpand::ORCHARD_NK.with(&sk.0)))
    }
}

impl NullifierDerivingKey {
    pub(crate) fn prf_nf(&self, rho: pallas::Base) -> pallas::Base {
        prf_nf(self.0, rho)
    }

    /// Converts this nullifier deriving key to its serialized form.
    pub(crate) fn to_bytes(self) -> [u8; 32] {
        <[u8; 32]>::from(self.0)
    }

    pub(crate) fn from_bytes(bytes: &[u8]) -> Option<Self> {
        let nk_bytes = <[u8; 32]>::try_from(bytes).ok()?;
        let nk = pallas::Base::from_repr(nk_bytes).map(NullifierDerivingKey);
        if nk.is_some().into() {
            Some(nk.unwrap())
        } else {
            None
        }
    }
}

/// The randomness for $\mathsf{Commit}^\mathsf{ivk}$.
///
/// $\mashsf{rivk}$ as defined in [Zcash Protocol Spec § 4.2.3: Orchard Key Components][orchardkeycomponents].
///
/// [orchardkeycomponents]: https://zips.z.cash/protocol/nu5.pdf#orchardkeycomponents
#[derive(Copy, Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub(crate) struct CommitIvkRandomness(pallas::Scalar);

impl From<&SpendingKey> for CommitIvkRandomness {
    fn from(sk: &SpendingKey) -> Self {
        CommitIvkRandomness(to_scalar(PrfExpand::ORCHARD_RIVK.with(&sk.0)))
    }
}

impl CommitIvkRandomness {
    pub(crate) fn inner(&self) -> pallas::Scalar {
        self.0
    }

    /// Converts this nullifier deriving key to its serialized form.
    pub(crate) fn to_bytes(self) -> [u8; 32] {
        <[u8; 32]>::from(self.0)
    }

    pub(crate) fn from_bytes(bytes: &[u8]) -> Option<Self> {
        let rivk_bytes = <[u8; 32]>::try_from(bytes).ok()?;
        let rivk = pallas::Scalar::from_repr(rivk_bytes).map(CommitIvkRandomness);
        if rivk.is_some().into() {
            Some(rivk.unwrap())
        } else {
            None
        }
    }
}

/// A key that provides the capability to view incoming and outgoing transactions.
///
/// This key is useful anywhere you need to maintain accurate balance, but do not want the
/// ability to spend funds (such as a view-only wallet).
///
/// Defined in [Zcash Protocol Spec § 4.2.3: Orchard Key Components][orchardkeycomponents].
///
/// [orchardkeycomponents]: https://zips.z.cash/protocol/nu5.pdf#orchardkeycomponents
#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub struct FullViewingKey {
    ak: SpendValidatingKey,
    nk: NullifierDerivingKey,
    rivk: CommitIvkRandomness,
}

impl From<&SpendingKey> for FullViewingKey {
    fn from(sk: &SpendingKey) -> Self {
        FullViewingKey {
            ak: (&SpendAuthorizingKey::from(sk)).into(),
            nk: sk.into(),
            rivk: sk.into(),
        }
    }
}

impl From<&ExtendedSpendingKey> for FullViewingKey {
    fn from(extsk: &ExtendedSpendingKey) -> Self {
        (&extsk.sk()).into()
    }
}

impl From<FullViewingKey> for SpendValidatingKey {
    fn from(fvk: FullViewingKey) -> Self {
        fvk.ak
    }
}

impl FullViewingKey {
    pub(crate) fn nk(&self) -> &NullifierDerivingKey {
        &self.nk
    }

    /// Returns either `rivk` or `rivk_internal` based on `scope`.
    pub(crate) fn rivk(&self, scope: Scope) -> CommitIvkRandomness {
        match scope {
            Scope::External => self.rivk,
            Scope::Internal => {
                let k = self.rivk.0.to_repr();
                let ak = self.ak.to_bytes();
                let nk = self.nk.to_bytes();
                CommitIvkRandomness(to_scalar(
                    PrfExpand::ORCHARD_RIVK_INTERNAL.with(&k, &ak, &nk),
                ))
            }
        }
    }

    /// Defined in [Zcash Protocol Spec § 4.2.3: Orchard Key Components][orchardkeycomponents].
    ///
    /// [orchardkeycomponents]: https://zips.z.cash/protocol/nu5.pdf#orchardkeycomponents
    fn derive_dk_ovk(&self) -> (DiversifierKey, OutgoingViewingKey) {
        let k = self.rivk.0.to_repr();
        let b = [(&self.ak.0).into(), self.nk.0.to_repr()];
        let r = PrfExpand::ORCHARD_DK_OVK.with(&k, &b[0], &b[1]);
        (
            DiversifierKey(r[..32].try_into().unwrap()),
            OutgoingViewingKey(r[32..].try_into().unwrap()),
        )
    }

    /// Returns the payment address for this key at the given index.
    pub fn address_at(&self, j: impl Into<DiversifierIndex>, scope: Scope) -> Address {
        self.to_ivk(scope).address_at(j)
    }

    /// Returns the payment address for this key corresponding to the given diversifier.
    pub fn address(&self, d: Diversifier, scope: Scope) -> Address {
        // Shortcut: we don't need to derive DiversifierKey.
        match scope {
            Scope::External => KeyAgreementPrivateKey::from_fvk(self),
            Scope::Internal => KeyAgreementPrivateKey::from_fvk(&self.derive_internal()),
        }
        .address(d)
    }

    /// Returns the scope of the given address, or `None` if the address is not derived
    /// from this full viewing key.
    pub fn scope_for_address(&self, address: &Address) -> Option<Scope> {
        [Scope::External, Scope::Internal]
            .into_iter()
            .find(|scope| self.to_ivk(*scope).diversifier_index(address).is_some())
    }

    /// Serializes the full viewing key as specified in [Zcash Protocol Spec § 5.6.4.4: Orchard Raw Full Viewing Keys][orchardrawfullviewingkeys]
    ///
    /// [orchardrawfullviewingkeys]: https://zips.z.cash/protocol/protocol.pdf#orchardfullviewingkeyencoding
    pub fn write<W: Write>(&self, mut writer: W) -> io::Result<()> {
        writer.write_all(&self.to_bytes())
    }

    /// Parses a full viewing key from its "raw" encoding as specified in [Zcash Protocol Spec § 5.6.4.4: Orchard Raw Full Viewing Keys][orchardrawfullviewingkeys]
    ///
    /// [orchardrawfullviewingkeys]: https://zips.z.cash/protocol/protocol.pdf#orchardfullviewingkeyencoding
    pub fn read<R: Read>(mut reader: R) -> io::Result<Self> {
        let mut data = [0u8; 96];
        reader.read_exact(&mut data)?;

        Self::from_bytes(&data).ok_or_else(|| {
            io::Error::new(
                io::ErrorKind::InvalidInput,
                "Unable to deserialize a valid Orchard FullViewingKey from bytes".to_owned(),
            )
        })
    }

    /// Serializes the full viewing key as specified in [Zcash Protocol Spec § 5.6.4.4: Orchard Raw Full Viewing Keys][orchardrawfullviewingkeys]
    ///
    /// [orchardrawfullviewingkeys]: https://zips.z.cash/protocol/protocol.pdf#orchardfullviewingkeyencoding
    pub fn to_bytes(&self) -> [u8; 96] {
        let mut result = [0u8; 96];
        result[0..32].copy_from_slice(&<[u8; 32]>::from(self.ak.0.clone()));
        result[32..64].copy_from_slice(&self.nk.0.to_repr());
        result[64..96].copy_from_slice(&self.rivk.0.to_repr());
        result
    }

    /// Parses a full viewing key from its "raw" encoding as specified in [Zcash Protocol Spec § 5.6.4.4: Orchard Raw Full Viewing Keys][orchardrawfullviewingkeys]
    ///
    /// [orchardrawfullviewingkeys]: https://zips.z.cash/protocol/protocol.pdf#orchardfullviewingkeyencoding
    pub fn from_bytes(bytes: &[u8; 96]) -> Option<Self> {
        let ak = SpendValidatingKey::from_bytes(&bytes[..32])?;
        let nk = NullifierDerivingKey::from_bytes(&bytes[32..64])?;
        let rivk = CommitIvkRandomness::from_bytes(&bytes[64..])?;

        let fvk = FullViewingKey { ak, nk, rivk };

        // If either ivk is 0 or ⊥, this FVK is invalid.
        let _: NonZeroPallasBase = Option::from(KeyAgreementPrivateKey::derive_inner(&fvk))?;
        let _: NonZeroPallasBase =
            Option::from(KeyAgreementPrivateKey::derive_inner(&fvk.derive_internal()))?;

        Some(fvk)
    }

    /// Derives an internal full viewing key from a full viewing key, as specified in
    /// [ZIP32][orchardinternalfullviewingkey]. Internal use only.
    ///
    /// [orchardinternalfullviewingkey]: https://zips.z.cash/zip-0032#orchard-internal-key-derivation
    fn derive_internal(&self) -> Self {
        FullViewingKey {
            ak: self.ak.clone(),
            nk: self.nk,
            rivk: self.rivk(Scope::Internal),
        }
    }

    /// Derives an `IncomingViewingKey` for this full viewing key.
    pub fn to_ivk(&self, scope: Scope) -> IncomingViewingKey {
        match scope {
            Scope::External => IncomingViewingKey::from_fvk(self),
            Scope::Internal => IncomingViewingKey::from_fvk(&self.derive_internal()),
        }
    }

    /// Derives an `OutgoingViewingKey` for this full viewing key.
    pub fn to_ovk(&self, scope: Scope) -> OutgoingViewingKey {
        match scope {
            Scope::External => OutgoingViewingKey::from_fvk(self),
            Scope::Internal => OutgoingViewingKey::from_fvk(&self.derive_internal()),
        }
    }
}

/// A key that provides the capability to derive a sequence of diversifiers.
///
/// $\mathsf{dk}$ as defined in [Zcash Protocol Spec § 4.2.3: Orchard Key Components][orchardkeycomponents].
///
/// [orchardkeycomponents]: https://zips.z.cash/protocol/nu5.pdf#orchardkeycomponents
#[derive(Clone, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub(crate) struct DiversifierKey([u8; 32]);

impl DiversifierKey {
    /// Returns the diversifier at the given index.
    pub fn get(&self, j: impl Into<DiversifierIndex>) -> Diversifier {
        let ff = FF1::<Aes256>::new(&self.0, 2).expect("valid radix");
        let enc = ff
            .encrypt(
                &[],
                &BinaryNumeralString::from_bytes_le(j.into().as_bytes()),
            )
            .unwrap();
        Diversifier(enc.to_bytes_le().try_into().unwrap())
    }

    /// Returns the diversifier index obtained by decrypting the diversifier.
    pub fn diversifier_index(&self, d: &Diversifier) -> DiversifierIndex {
        let ff = FF1::<Aes256>::new(&self.0, 2).expect("valid radix");
        let dec = ff
            .decrypt(&[], &BinaryNumeralString::from_bytes_le(d.as_array()))
            .unwrap();
        DiversifierIndex::from(<[u8; 11]>::try_from(dec.to_bytes_le()).unwrap())
    }

    /// Return the raw bytes of the diversifier key
    pub fn to_bytes(&self) -> &[u8; 32] {
        &self.0
    }

    /// Construct a diversifier key from bytes
    pub fn from_bytes(bytes: [u8; 32]) -> Self {
        DiversifierKey(bytes)
    }
}

/// A diversifier that can be used to derive a specific [`Address`] from a
/// [`FullViewingKey`] or [`IncomingViewingKey`].
///
/// $\mathsf{d}$ as defined in [Zcash Protocol Spec § 4.2.3: Orchard Key Components][orchardkeycomponents].
///
/// [orchardkeycomponents]: https://zips.z.cash/protocol/nu5.pdf#orchardkeycomponents
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Diversifier([u8; 11]);

impl Diversifier {
    /// Reads a diversifier from a byte array.
    pub fn from_bytes(d: [u8; 11]) -> Self {
        Diversifier(d)
    }

    /// Returns the byte array corresponding to this diversifier.
    pub fn as_array(&self) -> &[u8; 11] {
        &self.0
    }
}

/// The private key $\mathsf{ivk}$ used in $KA^{Orchard}$, for decrypting incoming notes.
///
/// In Sapling this is what was encoded as an incoming viewing key. For Orchard, we store
/// both this and [`DiversifierKey`] inside [`IncomingViewingKey`] for usability (to
/// enable deriving the default address for an incoming viewing key), while this separate
/// type represents $\mathsf{ivk}$.
///
/// Defined in [Zcash Protocol Spec § 4.2.3: Orchard Key Components][orchardkeycomponents].
///
/// [orchardkeycomponents]: https://zips.z.cash/protocol/nu5.pdf#orchardkeycomponents
///
/// # Implementation notes
///
/// We store $\mathsf{ivk}$ in memory as a scalar instead of a base, so that we aren't
/// incurring an expensive serialize-and-parse step every time we use it (e.g. for trial
/// decryption of notes). When we actually want to serialize ivk, we're guaranteed to get
/// a valid base field element encoding, because we always construct ivk from an integer
/// in the correct range.
#[derive(Clone, Debug, PartialEq, Eq, PartialOrd, Ord)]
struct KeyAgreementPrivateKey(NonZeroPallasScalar);

impl KeyAgreementPrivateKey {
    /// Derives `KeyAgreementPrivateKey` from fvk.
    ///
    /// Defined in [Zcash Protocol Spec § 4.2.3: Orchard Key Components][orchardkeycomponents].
    ///
    /// [orchardkeycomponents]: https://zips.z.cash/protocol/protocol.pdf#orchardkeycomponents
    fn from_fvk(fvk: &FullViewingKey) -> Self {
        // FullViewingKey cannot be constructed such that this unwrap would fail.
        let ivk = KeyAgreementPrivateKey::derive_inner(fvk).unwrap();
        KeyAgreementPrivateKey(ivk.into())
    }
}

impl KeyAgreementPrivateKey {
    /// Derives ivk from fvk. Internal use only, does not enforce all constraints.
    ///
    /// Defined in [Zcash Protocol Spec § 4.2.3: Orchard Key Components][orchardkeycomponents].
    ///
    /// [orchardkeycomponents]: https://zips.z.cash/protocol/protocol.pdf#orchardkeycomponents
    fn derive_inner(fvk: &FullViewingKey) -> CtOption<NonZeroPallasBase> {
        let ak = extract_p(&pallas::Point::from_bytes(&(&fvk.ak.0).into()).unwrap());
        commit_ivk(&ak, &fvk.nk.0, &fvk.rivk.0)
            // sinsemilla::CommitDomain::short_commit returns a value in range
            // [0..q_P] ∪ {⊥}:
            // - sinsemilla::HashDomain::hash_to_point uses incomplete addition and
            //   returns a point in P* ∪ {⊥}.
            // - sinsemilla::CommitDomain::commit applies a final complete addition step
            //   and returns a point in P ∪ {⊥}.
            // - 0 is not a valid x-coordinate for any Pallas point.
            // - sinsemilla::CommitDomain::short_commit calls extract_p_bottom, which
            //   replaces the identity (which has no affine coordinates) with 0.
            //
            // Commit^ivk.Output is specified as [1..q_P] ∪ {⊥}, so we explicitly check
            // for 0 and map it to None. Note that we are collapsing this case (which is
            // rejected by the circuit) with ⊥ (which the circuit explicitly allows for
            // efficiency); this is fine because we don't want users of the `orchard`
            // crate to encounter either case (and it matches the behaviour described in
            // Section 4.2.3 of the protocol spec when generating spending keys).
            .and_then(NonZeroPallasBase::from_base)
    }

    /// Returns the payment address for this key corresponding to the given diversifier.
    fn address(&self, d: Diversifier) -> Address {
        let prepared_ivk = PreparedIncomingViewingKey::new_inner(self);
        let pk_d = DiversifiedTransmissionKey::derive(&prepared_ivk, &d);
        Address::from_parts(d, pk_d)
    }
}

/// A key that provides the capability to detect and decrypt incoming notes from the block
/// chain, without being able to spend the notes or detect when they are spent.
///
/// This key is useful in situations where you only need the capability to detect inbound
/// payments, such as merchant terminals.
///
/// This key is not suitable for use on its own in a wallet, as it cannot maintain
/// accurate balance. You should use a [`FullViewingKey`] instead.
///
/// Defined in [Zcash Protocol Spec § 5.6.4.3: Orchard Raw Incoming Viewing Keys][orchardinviewingkeyencoding].
///
/// [orchardinviewingkeyencoding]: https://zips.z.cash/protocol/nu5.pdf#orchardinviewingkeyencoding
#[derive(Clone, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub struct IncomingViewingKey {
    dk: DiversifierKey,
    ivk: KeyAgreementPrivateKey,
}

impl IncomingViewingKey {
    /// Helper method.
    fn from_fvk(fvk: &FullViewingKey) -> Self {
        IncomingViewingKey {
            dk: fvk.derive_dk_ovk().0,
            ivk: KeyAgreementPrivateKey::from_fvk(fvk),
        }
    }
}

impl IncomingViewingKey {
    /// Serializes an Orchard incoming viewing key to its raw encoding as specified in [Zcash Protocol Spec § 5.6.4.3: Orchard Raw Incoming Viewing Keys][orchardrawinviewingkeys]
    ///
    /// [orchardrawinviewingkeys]: https://zips.z.cash/protocol/protocol.pdf#orchardinviewingkeyencoding
    pub fn to_bytes(&self) -> [u8; 64] {
        let mut result = [0u8; 64];
        result[..32].copy_from_slice(self.dk.to_bytes());
        result[32..].copy_from_slice(&self.ivk.0.to_repr());
        result
    }

    /// Parses an Orchard incoming viewing key from its raw encoding.
    pub fn from_bytes(bytes: &[u8; 64]) -> CtOption<Self> {
        NonZeroPallasBase::from_bytes(bytes[32..].try_into().unwrap()).map(|ivk| {
            IncomingViewingKey {
                dk: DiversifierKey(bytes[..32].try_into().unwrap()),
                ivk: KeyAgreementPrivateKey(ivk.into()),
            }
        })
    }

    /// Checks whether the given address was derived from this incoming viewing
    /// key, and returns the diversifier index used to derive the address if
    /// so. Returns `None` if the address was not derived from this key.
    pub fn diversifier_index(&self, addr: &Address) -> Option<DiversifierIndex> {
        let j = self.dk.diversifier_index(&addr.diversifier());
        if &self.address_at(j) == addr {
            Some(j)
        } else {
            None
        }
    }

    /// Returns the payment address for this key at the given index.
    pub fn address_at(&self, j: impl Into<DiversifierIndex>) -> Address {
        self.address(self.dk.get(j))
    }

    /// Returns the payment address for this key corresponding to the given diversifier.
    pub fn address(&self, d: Diversifier) -> Address {
        self.ivk.address(d)
    }
}

/// An Orchard incoming viewing key that has been precomputed for trial decryption.
#[derive(Clone, Debug)]
pub struct PreparedIncomingViewingKey(PreparedNonZeroScalar);

impl memuse::DynamicUsage for PreparedIncomingViewingKey {
    fn dynamic_usage(&self) -> usize {
        self.0.dynamic_usage()
    }

    fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
        self.0.dynamic_usage_bounds()
    }
}

impl PreparedIncomingViewingKey {
    /// Performs the necessary precomputations to use an `IncomingViewingKey` for note
    /// decryption.
    pub fn new(ivk: &IncomingViewingKey) -> Self {
        Self::new_inner(&ivk.ivk)
    }

    fn new_inner(ivk: &KeyAgreementPrivateKey) -> Self {
        Self(PreparedNonZeroScalar::new(&ivk.0))
    }
}

/// A key that provides the capability to recover outgoing transaction information from
/// the block chain.
///
/// This key is not suitable for use on its own in a wallet, as it cannot maintain
/// accurate balance. You should use a [`FullViewingKey`] instead.
///
/// Defined in [Zcash Protocol Spec § 4.2.3: Orchard Key Components][orchardkeycomponents].
///
/// [orchardkeycomponents]: https://zips.z.cash/protocol/nu5.pdf#orchardkeycomponents
#[derive(Debug, Clone)]
pub struct OutgoingViewingKey([u8; 32]);

impl OutgoingViewingKey {
    /// Helper method.
    fn from_fvk(fvk: &FullViewingKey) -> Self {
        fvk.derive_dk_ovk().1
    }
}

impl From<[u8; 32]> for OutgoingViewingKey {
    fn from(ovk: [u8; 32]) -> Self {
        OutgoingViewingKey(ovk)
    }
}

impl AsRef<[u8; 32]> for OutgoingViewingKey {
    fn as_ref(&self) -> &[u8; 32] {
        &self.0
    }
}

/// The diversified transmission key for a given payment address.
///
/// Defined in [Zcash Protocol Spec § 4.2.3: Orchard Key Components][orchardkeycomponents].
///
/// [orchardkeycomponents]: https://zips.z.cash/protocol/nu5.pdf#orchardkeycomponents
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct DiversifiedTransmissionKey(NonIdentityPallasPoint);

impl DiversifiedTransmissionKey {
    pub(crate) fn inner(&self) -> NonIdentityPallasPoint {
        self.0
    }
}

impl DiversifiedTransmissionKey {
    /// Defined in [Zcash Protocol Spec § 4.2.3: Orchard Key Components][orchardkeycomponents].
    ///
    /// [orchardkeycomponents]: https://zips.z.cash/protocol/nu5.pdf#orchardkeycomponents
    pub(crate) fn derive(ivk: &PreparedIncomingViewingKey, d: &Diversifier) -> Self {
        let g_d = PreparedNonIdentityBase::new(diversify_hash(d.as_array()));
        DiversifiedTransmissionKey(ka_orchard_prepared(&ivk.0, &g_d))
    }

    /// $abst_P(bytes)$
    pub(crate) fn from_bytes(bytes: &[u8; 32]) -> CtOption<Self> {
        NonIdentityPallasPoint::from_bytes(bytes).map(DiversifiedTransmissionKey)
    }

    /// $repr_P(self)$
    pub(crate) fn to_bytes(self) -> [u8; 32] {
        self.0.to_bytes()
    }
}

impl ConditionallySelectable for DiversifiedTransmissionKey {
    fn conditional_select(a: &Self, b: &Self, choice: Choice) -> Self {
        DiversifiedTransmissionKey(NonIdentityPallasPoint::conditional_select(
            &a.0, &b.0, choice,
        ))
    }
}

/// An ephemeral secret key used to encrypt an output note on-chain.
///
/// `esk` is "ephemeral" in the sense that each secret key is only used once. In
/// practice, `esk` is derived deterministically from the note that it is encrypting.
///
/// $\mathsf{KA}^\mathsf{Orchard}.\mathsf{Private} := \mathbb{F}^{\ast}_{r_P}$
///
/// Defined in [section 5.4.5.5: Orchard Key Agreement][concreteorchardkeyagreement].
///
/// [concreteorchardkeyagreement]: https://zips.z.cash/protocol/nu5.pdf#concreteorchardkeyagreement
#[derive(Debug)]
pub struct EphemeralSecretKey(pub(crate) NonZeroPallasScalar);

impl ConstantTimeEq for EphemeralSecretKey {
    fn ct_eq(&self, other: &Self) -> subtle::Choice {
        self.0.ct_eq(&other.0)
    }
}

impl EphemeralSecretKey {
    pub(crate) fn from_bytes(bytes: &[u8; 32]) -> CtOption<Self> {
        NonZeroPallasScalar::from_bytes(bytes).map(EphemeralSecretKey)
    }

    pub(crate) fn derive_public(&self, g_d: NonIdentityPallasPoint) -> EphemeralPublicKey {
        EphemeralPublicKey(ka_orchard(&self.0, &g_d))
    }

    pub(crate) fn agree(&self, pk_d: &DiversifiedTransmissionKey) -> SharedSecret {
        SharedSecret(ka_orchard(&self.0, &pk_d.0))
    }
}

/// An ephemeral public key used to encrypt an output note on-chain.
///
/// `epk` is "ephemeral" in the sense that each public key is only used once. In practice,
/// `epk` is derived deterministically from the note that it is encrypting.
///
/// $\mathsf{KA}^\mathsf{Orchard}.\mathsf{Public} := \mathbb{P}^{\ast}$
///
/// Defined in [section 5.4.5.5: Orchard Key Agreement][concreteorchardkeyagreement].
///
/// [concreteorchardkeyagreement]: https://zips.z.cash/protocol/nu5.pdf#concreteorchardkeyagreement
#[derive(Debug)]
pub struct EphemeralPublicKey(NonIdentityPallasPoint);

impl EphemeralPublicKey {
    pub(crate) fn from_bytes(bytes: &[u8; 32]) -> CtOption<Self> {
        NonIdentityPallasPoint::from_bytes(bytes).map(EphemeralPublicKey)
    }

    pub(crate) fn to_bytes(&self) -> EphemeralKeyBytes {
        EphemeralKeyBytes(self.0.to_bytes())
    }

    pub(crate) fn agree(&self, ivk: &IncomingViewingKey) -> SharedSecret {
        SharedSecret(ka_orchard(&ivk.ivk.0, &self.0))
    }
}

/// An Orchard ephemeral public key that has been precomputed for trial decryption.
#[derive(Clone, Debug)]
pub struct PreparedEphemeralPublicKey(PreparedNonIdentityBase);

impl PreparedEphemeralPublicKey {
    pub(crate) fn new(epk: EphemeralPublicKey) -> Self {
        PreparedEphemeralPublicKey(PreparedNonIdentityBase::new(epk.0))
    }

    pub(crate) fn agree(&self, ivk: &PreparedIncomingViewingKey) -> SharedSecret {
        SharedSecret(ka_orchard_prepared(&ivk.0, &self.0))
    }
}

/// $\mathsf{KA}^\mathsf{Orchard}.\mathsf{SharedSecret} := \mathbb{P}^{\ast}$
///
/// Defined in [section 5.4.5.5: Orchard Key Agreement][concreteorchardkeyagreement].
///
/// [concreteorchardkeyagreement]: https://zips.z.cash/protocol/nu5.pdf#concreteorchardkeyagreement
#[derive(Debug)]
pub struct SharedSecret(NonIdentityPallasPoint);

impl SharedSecret {
    /// For checking test vectors only.
    #[cfg(test)]
    pub(crate) fn to_bytes(&self) -> [u8; 32] {
        self.0.to_bytes()
    }

    /// Only for use in batched note encryption.
    pub(crate) fn batch_to_affine(
        shared_secrets: Vec<Option<Self>>,
    ) -> impl Iterator<Item = Option<pallas::Affine>> {
        // Filter out the positions for which ephemeral_key was not a valid encoding.
        let secrets: Vec<_> = shared_secrets
            .iter()
            .filter_map(|s| s.as_ref().map(|s| *(s.0)))
            .collect();

        // Batch-normalize the shared secrets.
        let mut secrets_affine = vec![pallas::Affine::identity(); secrets.len()];
        group::Curve::batch_normalize(&secrets, &mut secrets_affine);

        // Re-insert the invalid ephemeral_key positions.
        let mut secrets_affine = secrets_affine.into_iter();
        shared_secrets
            .into_iter()
            .map(move |s| s.and_then(|_| secrets_affine.next()))
    }

    /// Defined in [Zcash Protocol Spec § 5.4.5.6: Orchard Key Agreement][concreteorchardkdf].
    ///
    /// [concreteorchardkdf]: https://zips.z.cash/protocol/nu5.pdf#concreteorchardkdf
    pub(crate) fn kdf_orchard(self, ephemeral_key: &EphemeralKeyBytes) -> Blake2bHash {
        Self::kdf_orchard_inner(self.0.to_affine(), ephemeral_key)
    }

    /// Only for direct use in batched note encryption.
    pub(crate) fn kdf_orchard_inner(
        secret: pallas::Affine,
        ephemeral_key: &EphemeralKeyBytes,
    ) -> Blake2bHash {
        Params::new()
            .hash_length(32)
            .personal(KDF_ORCHARD_PERSONALIZATION)
            .to_state()
            .update(&secret.to_bytes())
            .update(&ephemeral_key.0)
            .finalize()
    }
}

/// Generators for property testing.
#[cfg(any(test, feature = "test-dependencies"))]
#[cfg_attr(docsrs, doc(cfg(feature = "test-dependencies")))]
pub mod testing {
    use proptest::prelude::*;

    use super::{DiversifierIndex, DiversifierKey, EphemeralSecretKey, SpendingKey};

    prop_compose! {
        /// Generate a uniformly distributed Orchard spending key.
        pub fn arb_spending_key()(
            key in prop::array::uniform32(prop::num::u8::ANY)
                .prop_map(SpendingKey::from_bytes)
                .prop_filter(
                    "Values must correspond to valid Orchard spending keys.",
                    |opt| bool::from(opt.is_some())
                )
        ) -> SpendingKey {
            key.unwrap()
        }
    }

    prop_compose! {
        /// Generate a uniformly distributed Orchard ephemeral secret key.
        pub fn arb_esk()(
            esk in prop::array::uniform32(prop::num::u8::ANY)
                .prop_map(|b| EphemeralSecretKey::from_bytes(&b))
                .prop_filter(
                    "Values must correspond to valid Orchard ephemeral secret keys.",
                    |opt| bool::from(opt.is_some())
                )
        ) -> EphemeralSecretKey {
            esk.unwrap()
        }
    }

    prop_compose! {
        /// Generate a uniformly distributed Orchard diversifier key.
        pub(crate) fn arb_diversifier_key()(
            dk_bytes in prop::array::uniform32(prop::num::u8::ANY)
        ) -> DiversifierKey {
            DiversifierKey::from_bytes(dk_bytes)
        }
    }

    prop_compose! {
        /// Generate a uniformly distributed diversifier index.
        pub fn arb_diversifier_index()(
            d_bytes in prop::array::uniform11(prop::num::u8::ANY)
        ) -> DiversifierIndex {
            DiversifierIndex::from(d_bytes)
        }
    }
}

#[cfg(test)]
mod tests {
    use ff::PrimeField;
    use proptest::prelude::*;

    use super::{
        testing::{arb_diversifier_index, arb_diversifier_key, arb_esk, arb_spending_key},
        *,
    };
    use crate::{
        note::{ExtractedNoteCommitment, RandomSeed, Rho},
        value::NoteValue,
        Note,
    };

    #[test]
    fn spend_validating_key_from_bytes() {
        // ak_P must not be the identity.
        assert!(SpendValidatingKey::from_bytes(&[0; 32]).is_none());
    }

    #[test]
    fn parsers_reject_invalid() {
        assert!(bool::from(
            EphemeralSecretKey::from_bytes(&[0xff; 32]).is_none()
        ));
        assert!(bool::from(
            EphemeralPublicKey::from_bytes(&[0xff; 32]).is_none()
        ));
    }

    proptest! {
        #[test]
        fn key_agreement(
            sk in arb_spending_key(),
            esk in arb_esk(),
            j in arb_diversifier_index(),
        ) {
            let ivk = IncomingViewingKey::from_fvk(&(&sk).into());
            let addr = ivk.address_at(j);

            let epk = esk.derive_public(addr.g_d());

            assert!(bool::from(
                esk.agree(addr.pk_d()).0.ct_eq(&epk.agree(&ivk).0)
            ));
        }
    }

    proptest! {
        #[test]
        fn diversifier_index(
            dk in arb_diversifier_key(),
            j in arb_diversifier_index(),
        ) {
            let d = dk.get(j);
            assert_eq!(j, dk.diversifier_index(&d));
        }
    }

    #[test]
    fn test_vectors() {
        for tv in crate::test_vectors::keys::test_vectors() {
            let sk = SpendingKey::from_bytes(tv.sk).unwrap();

            let ask: SpendAuthorizingKey = (&sk).into();
            assert_eq!(<[u8; 32]>::from(&ask.0), tv.ask);

            let ak: SpendValidatingKey = (&ask).into();
            assert_eq!(<[u8; 32]>::from(ak.0), tv.ak);

            let nk: NullifierDerivingKey = (&sk).into();
            assert_eq!(nk.0.to_repr(), tv.nk);

            let rivk: CommitIvkRandomness = (&sk).into();
            assert_eq!(rivk.0.to_repr(), tv.rivk);

            let fvk: FullViewingKey = (&sk).into();
            assert_eq!(<[u8; 32]>::from(&fvk.ak.0), tv.ak);
            assert_eq!(fvk.nk().0.to_repr(), tv.nk);
            assert_eq!(fvk.rivk.0.to_repr(), tv.rivk);

            let external_ivk = fvk.to_ivk(Scope::External);
            assert_eq!(external_ivk.ivk.0.to_repr(), tv.ivk);

            let diversifier = Diversifier(tv.default_d);

            let addr = fvk.address(diversifier, Scope::External);
            assert_eq!(&addr.pk_d().to_bytes(), &tv.default_pk_d);

            let rho = Rho::from_bytes(&tv.note_rho).unwrap();
            let note = Note::from_parts(
                addr,
                NoteValue::from_raw(tv.note_v),
                rho,
                RandomSeed::from_bytes(tv.note_rseed, &rho).unwrap(),
            )
            .unwrap();

            let cmx: ExtractedNoteCommitment = note.commitment().into();
            assert_eq!(cmx.to_bytes(), tv.note_cmx);

            assert_eq!(note.nullifier(&fvk).to_bytes(), tv.note_nf);

            let internal_rivk = fvk.rivk(Scope::Internal);
            assert_eq!(internal_rivk.0.to_repr(), tv.internal_rivk);

            let internal_ivk = fvk.to_ivk(Scope::Internal);
            assert_eq!(internal_ivk.ivk.0.to_repr(), tv.internal_ivk);
            assert_eq!(internal_ivk.dk.0, tv.internal_dk);

            let internal_ovk = fvk.to_ovk(Scope::Internal);
            assert_eq!(internal_ovk.0, tv.internal_ovk);
        }
    }
}
