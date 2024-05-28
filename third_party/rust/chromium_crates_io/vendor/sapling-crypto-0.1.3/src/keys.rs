//! Sapling key components.
//!
//! Implements [section 4.2.2] of the Zcash Protocol Specification.
//!
//! [section 4.2.2]: https://zips.z.cash/protocol/protocol.pdf#saplingkeycomponents

use std::fmt;
use std::io::{self, Read, Write};

use super::{
    address::PaymentAddress,
    constants::{self, PROOF_GENERATION_KEY_GENERATOR},
    note_encryption::KDF_SAPLING_PERSONALIZATION,
    spec::{
        crh_ivk, diversify_hash, ka_sapling_agree, ka_sapling_agree_prepared,
        ka_sapling_derive_public, ka_sapling_derive_public_subgroup_prepared, PreparedBase,
        PreparedBaseSubgroup, PreparedScalar,
    },
};

use blake2b_simd::{Hash as Blake2bHash, Params as Blake2bParams};
use ff::{Field, PrimeField};
use group::{Curve, Group, GroupEncoding};
use redjubjub::SpendAuth;
use subtle::{Choice, ConditionallySelectable, ConstantTimeEq, CtOption};
use zcash_note_encryption::EphemeralKeyBytes;
use zcash_spec::PrfExpand;

#[cfg(test)]
use rand_core::RngCore;

/// Errors that can occur in the decoding of Sapling spending keys.
pub enum DecodingError {
    /// The length of the byte slice provided for decoding was incorrect.
    LengthInvalid { expected: usize, actual: usize },
    /// Could not decode the `ask` bytes to a jubjub field element.
    InvalidAsk,
    /// Could not decode the `nsk` bytes to a jubjub field element.
    InvalidNsk,
    /// An extended spending key had an unsupported child index: either a non-hardened
    /// index, or a non-zero index at depth 0.
    UnsupportedChildIndex,
}

/// A spend authorizing key, used to create spend authorization signatures.
///
/// $\mathsf{ask}$ as defined in [Zcash Protocol Spec § 4.2.2: Sapling Key Components][saplingkeycomponents].
///
/// [saplingkeycomponents]: https://zips.z.cash/protocol/protocol.pdf#saplingkeycomponents
#[derive(Clone, Debug)]
pub struct SpendAuthorizingKey(redjubjub::SigningKey<SpendAuth>);

impl PartialEq for SpendAuthorizingKey {
    fn eq(&self, other: &Self) -> bool {
        <[u8; 32]>::from(self.0)
            .ct_eq(&<[u8; 32]>::from(other.0))
            .into()
    }
}

impl Eq for SpendAuthorizingKey {}

impl From<&SpendValidatingKey> for jubjub::ExtendedPoint {
    fn from(spend_validating_key: &SpendValidatingKey) -> jubjub::ExtendedPoint {
        jubjub::ExtendedPoint::from_bytes(&spend_validating_key.to_bytes()).unwrap()
    }
}

impl SpendAuthorizingKey {
    /// Derives ask from sk. Internal use only, does not enforce all constraints.
    fn derive_inner(sk: &[u8]) -> jubjub::Scalar {
        jubjub::Scalar::from_bytes_wide(&PrfExpand::SAPLING_ASK.with(sk))
    }

    /// Constructs a `SpendAuthorizingKey` from a raw scalar.
    pub(crate) fn from_scalar(ask: jubjub::Scalar) -> Option<Self> {
        if ask.is_zero().into() {
            None
        } else {
            Some(SpendAuthorizingKey(ask.to_bytes().try_into().unwrap()))
        }
    }

    /// Derives a `SpendAuthorizingKey` from a spending key.
    fn from_spending_key(sk: &[u8]) -> Option<Self> {
        Self::from_scalar(Self::derive_inner(sk))
    }

    /// Parses a `SpendAuthorizingKey` from its encoded form.
    pub(crate) fn from_bytes(bytes: &[u8]) -> Option<Self> {
        <[u8; 32]>::try_from(bytes)
            .ok()
            .and_then(|b| {
                // RedJubjub.Private permits the full set of Jubjub scalars including
                // zero. However, a SpendAuthorizingKey is further restricted within the
                // Sapling key tree to be a non-zero scalar.
                jubjub::Scalar::from_repr(b)
                    .and_then(|s| {
                        CtOption::new(
                            redjubjub::SigningKey::try_from(b)
                                .expect("RedJubjub permits the set of valid SpendAuthorizingKeys"),
                            !s.is_zero(),
                        )
                    })
                    .into()
            })
            .map(SpendAuthorizingKey)
    }

    /// Converts this spend authorizing key to its serialized form.
    pub(crate) fn to_bytes(&self) -> [u8; 32] {
        <[u8; 32]>::from(self.0)
    }

    /// Converts this spend authorizing key to a raw scalar.
    ///
    /// Only used for ZIP 32 child derivation.
    pub(crate) fn to_scalar(&self) -> jubjub::Scalar {
        jubjub::Scalar::from_repr(self.0.into()).unwrap()
    }

    /// Randomizes this spend authorizing key with the given `randomizer`.
    ///
    /// The resulting key can be used to actually sign a spend.
    pub fn randomize(&self, randomizer: &jubjub::Scalar) -> redjubjub::SigningKey<SpendAuth> {
        self.0.randomize(randomizer)
    }
}

/// A key used to validate spend authorization signatures.
///
/// Defined in [Zcash Protocol Spec § 4.2.2: Sapling Key Components][saplingkeycomponents].
///
/// [saplingkeycomponents]: https://zips.z.cash/protocol/protocol.pdf#saplingkeycomponents
#[derive(Clone, Debug)]
pub struct SpendValidatingKey(redjubjub::VerificationKey<SpendAuth>);

impl From<&SpendAuthorizingKey> for SpendValidatingKey {
    fn from(ask: &SpendAuthorizingKey) -> Self {
        SpendValidatingKey((&ask.0).into())
    }
}

impl PartialEq for SpendValidatingKey {
    fn eq(&self, other: &Self) -> bool {
        <[u8; 32]>::from(self.0)
            .ct_eq(&<[u8; 32]>::from(other.0))
            .into()
    }
}

impl Eq for SpendValidatingKey {}

impl SpendValidatingKey {
    /// For circuit tests only.
    #[cfg(test)]
    pub(crate) fn fake_random<R: RngCore>(mut rng: R) -> Self {
        loop {
            if let Some(k) = Self::from_bytes(&jubjub::SubgroupPoint::random(&mut rng).to_bytes()) {
                break k;
            }
        }
    }

    /// Only exposed for `zcashd` unit tests.
    #[cfg(feature = "temporary-zcashd")]
    pub fn temporary_zcash_from_bytes(bytes: &[u8]) -> Option<Self> {
        Self::from_bytes(bytes)
    }

    /// Parses a `SpendValidatingKey` from its encoded form.
    pub(crate) fn from_bytes(bytes: &[u8]) -> Option<Self> {
        <[u8; 32]>::try_from(bytes)
            .ok()
            .and_then(|b| {
                // RedJubjub.Public permits the full set of Jubjub points including the
                // identity and cofactors; this is the type used for `rk` in Spend
                // descriptions. However, a SpendValidatingKey is further restricted
                // within the Sapling key tree to be a non-identity element of the
                // prime-order subgroup.
                jubjub::SubgroupPoint::from_bytes(&b)
                    .and_then(|p| {
                        CtOption::new(
                            redjubjub::VerificationKey::try_from(b)
                                .expect("RedJubjub permits the set of valid SpendValidatingKeys"),
                            !p.is_identity(),
                        )
                    })
                    .into()
            })
            .map(SpendValidatingKey)
    }

    /// Converts this spend validating key to its serialized form,
    /// `LEBS2OSP_256(repr_J(ak))`.
    pub(crate) fn to_bytes(&self) -> [u8; 32] {
        <[u8; 32]>::from(self.0)
    }

    /// Randomizes this spend validating key with the given `randomizer`.
    pub fn randomize(&self, randomizer: &jubjub::Scalar) -> redjubjub::VerificationKey<SpendAuth> {
        self.0.randomize(randomizer)
    }
}

/// An outgoing viewing key
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct OutgoingViewingKey(pub [u8; 32]);

/// A Sapling expanded spending key
#[derive(Clone)]
pub struct ExpandedSpendingKey {
    pub ask: SpendAuthorizingKey,
    pub nsk: jubjub::Fr,
    pub ovk: OutgoingViewingKey,
}

impl fmt::Debug for ExpandedSpendingKey {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("ExpandedSpendingKey")
            .finish_non_exhaustive()
    }
}

impl ExpandedSpendingKey {
    /// Expands a spending key into its components.
    ///
    /// # Panics
    ///
    /// Panics if this spending key expands to `ask = 0`. This has a negligible
    /// probability of occurring.
    pub fn from_spending_key(sk: &[u8]) -> Self {
        let ask =
            SpendAuthorizingKey::from_spending_key(sk).expect("negligible chance of ask == 0");
        let nsk = jubjub::Fr::from_bytes_wide(&PrfExpand::SAPLING_NSK.with(sk));
        let mut ovk = OutgoingViewingKey([0u8; 32]);
        ovk.0
            .copy_from_slice(&PrfExpand::SAPLING_OVK.with(sk)[..32]);
        ExpandedSpendingKey { ask, nsk, ovk }
    }

    pub fn proof_generation_key(&self) -> ProofGenerationKey {
        ProofGenerationKey {
            ak: (&self.ask).into(),
            nsk: self.nsk,
        }
    }

    /// Decodes the expanded spending key from its serialized representation
    /// as part of the encoding of the extended spending key as defined in
    /// [ZIP 32](https://zips.z.cash/zip-0032)
    pub fn from_bytes(b: &[u8]) -> Result<Self, DecodingError> {
        if b.len() != 96 {
            return Err(DecodingError::LengthInvalid {
                expected: 96,
                actual: b.len(),
            });
        }

        let ask = SpendAuthorizingKey::from_bytes(&b[0..32]).ok_or(DecodingError::InvalidAsk)?;
        let nsk = Option::from(jubjub::Fr::from_repr(b[32..64].try_into().unwrap()))
            .ok_or(DecodingError::InvalidNsk)?;
        let ovk = OutgoingViewingKey(b[64..96].try_into().unwrap());

        Ok(ExpandedSpendingKey { ask, nsk, ovk })
    }

    pub fn read<R: Read>(mut reader: R) -> io::Result<Self> {
        let mut repr = [0u8; 96];
        reader.read_exact(repr.as_mut())?;
        Self::from_bytes(&repr).map_err(|e| match e {
            DecodingError::InvalidAsk => {
                io::Error::new(io::ErrorKind::InvalidData, "ask not in field")
            }
            DecodingError::InvalidNsk => {
                io::Error::new(io::ErrorKind::InvalidData, "nsk not in field")
            }
            DecodingError::LengthInvalid { .. } | DecodingError::UnsupportedChildIndex => {
                unreachable!()
            }
        })
    }

    pub fn write<W: Write>(&self, mut writer: W) -> io::Result<()> {
        writer.write_all(&self.to_bytes())
    }

    /// Encodes the expanded spending key to its serialized representation
    /// as part of the encoding of the extended spending key as defined in
    /// [ZIP 32](https://zips.z.cash/zip-0032)
    pub fn to_bytes(&self) -> [u8; 96] {
        let mut result = [0u8; 96];
        result[0..32].copy_from_slice(&self.ask.to_bytes());
        result[32..64].copy_from_slice(&self.nsk.to_repr());
        result[64..96].copy_from_slice(&self.ovk.0);
        result
    }
}

#[derive(Clone)]
pub struct ProofGenerationKey {
    pub ak: SpendValidatingKey,
    pub nsk: jubjub::Fr,
}

impl fmt::Debug for ProofGenerationKey {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("ProofGenerationKey")
            .field("ak", &self.ak)
            .finish_non_exhaustive()
    }
}

impl ProofGenerationKey {
    pub fn to_viewing_key(&self) -> ViewingKey {
        ViewingKey {
            ak: self.ak.clone(),
            nk: NullifierDerivingKey(constants::PROOF_GENERATION_KEY_GENERATOR * self.nsk),
        }
    }
}

/// A key used to derive the nullifier for a Sapling note.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub struct NullifierDerivingKey(pub jubjub::SubgroupPoint);

#[derive(Debug, Clone)]
pub struct ViewingKey {
    pub ak: SpendValidatingKey,
    pub nk: NullifierDerivingKey,
}

impl ViewingKey {
    pub fn rk(&self, ar: jubjub::Fr) -> redjubjub::VerificationKey<SpendAuth> {
        self.ak.randomize(&ar)
    }

    pub fn ivk(&self) -> SaplingIvk {
        SaplingIvk(crh_ivk(self.ak.to_bytes(), self.nk.0.to_bytes()))
    }

    pub fn to_payment_address(&self, diversifier: Diversifier) -> Option<PaymentAddress> {
        self.ivk().to_payment_address(diversifier)
    }
}

/// A Sapling key that provides the capability to view incoming and outgoing transactions.
#[derive(Debug)]
pub struct FullViewingKey {
    pub vk: ViewingKey,
    pub ovk: OutgoingViewingKey,
}

impl Clone for FullViewingKey {
    fn clone(&self) -> Self {
        FullViewingKey {
            vk: ViewingKey {
                ak: self.vk.ak.clone(),
                nk: self.vk.nk,
            },
            ovk: self.ovk,
        }
    }
}

impl FullViewingKey {
    pub fn from_expanded_spending_key(expsk: &ExpandedSpendingKey) -> Self {
        FullViewingKey {
            vk: ViewingKey {
                ak: (&expsk.ask).into(),
                nk: NullifierDerivingKey(PROOF_GENERATION_KEY_GENERATOR * expsk.nsk),
            },
            ovk: expsk.ovk,
        }
    }

    pub fn read<R: Read>(mut reader: R) -> io::Result<Self> {
        let ak = {
            let mut buf = [0u8; 32];
            reader.read_exact(&mut buf)?;
            SpendValidatingKey::from_bytes(&buf)
        };
        let nk = {
            let mut buf = [0u8; 32];
            reader.read_exact(&mut buf)?;
            jubjub::SubgroupPoint::from_bytes(&buf)
        };
        if ak.is_none() {
            return Err(io::Error::new(
                io::ErrorKind::InvalidInput,
                "ak not of prime order",
            ));
        }
        if nk.is_none().into() {
            return Err(io::Error::new(
                io::ErrorKind::InvalidInput,
                "nk not in prime-order subgroup",
            ));
        }
        let ak = ak.unwrap();
        let nk = NullifierDerivingKey(nk.unwrap());

        let mut ovk = [0u8; 32];
        reader.read_exact(&mut ovk)?;

        Ok(FullViewingKey {
            vk: ViewingKey { ak, nk },
            ovk: OutgoingViewingKey(ovk),
        })
    }

    pub fn write<W: Write>(&self, mut writer: W) -> io::Result<()> {
        writer.write_all(&self.vk.ak.to_bytes())?;
        writer.write_all(&self.vk.nk.0.to_bytes())?;
        writer.write_all(&self.ovk.0)?;

        Ok(())
    }

    pub fn to_bytes(&self) -> [u8; 96] {
        let mut result = [0u8; 96];
        self.write(&mut result[..])
            .expect("should be able to serialize a FullViewingKey");
        result
    }
}

#[derive(Debug, Clone)]
pub struct SaplingIvk(pub jubjub::Fr);

impl SaplingIvk {
    pub fn to_payment_address(&self, diversifier: Diversifier) -> Option<PaymentAddress> {
        let prepared_ivk = PreparedIncomingViewingKey::new(self);
        DiversifiedTransmissionKey::derive(&prepared_ivk, &diversifier)
            .and_then(|pk_d| PaymentAddress::from_parts(diversifier, pk_d))
    }

    pub fn to_repr(&self) -> [u8; 32] {
        self.0.to_repr()
    }
}

/// A Sapling incoming viewing key that has been precomputed for trial decryption.
#[derive(Clone, Debug)]
pub struct PreparedIncomingViewingKey(PreparedScalar);

impl memuse::DynamicUsage for PreparedIncomingViewingKey {
    fn dynamic_usage(&self) -> usize {
        self.0.dynamic_usage()
    }

    fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
        self.0.dynamic_usage_bounds()
    }
}

impl PreparedIncomingViewingKey {
    /// Performs the necessary precomputations to use a `SaplingIvk` for note decryption.
    pub fn new(ivk: &SaplingIvk) -> Self {
        Self(PreparedScalar::new(&ivk.0))
    }
}

#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub struct Diversifier(pub [u8; 11]);

impl Diversifier {
    pub fn g_d(&self) -> Option<jubjub::SubgroupPoint> {
        diversify_hash(&self.0)
    }
}

/// The diversified transmission key for a given payment address.
///
/// Defined in [Zcash Protocol Spec § 4.2.2: Sapling Key Components][saplingkeycomponents].
///
/// Note that this type is allowed to be the identity in the protocol, but we reject this
/// in [`PaymentAddress::from_parts`].
///
/// [saplingkeycomponents]: https://zips.z.cash/protocol/protocol.pdf#saplingkeycomponents
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct DiversifiedTransmissionKey(jubjub::SubgroupPoint);

impl DiversifiedTransmissionKey {
    /// Defined in [Zcash Protocol Spec § 4.2.2: Sapling Key Components][saplingkeycomponents].
    ///
    /// Returns `None` if `d` is an invalid diversifier.
    ///
    /// [saplingkeycomponents]: https://zips.z.cash/protocol/protocol.pdf#saplingkeycomponents
    pub(crate) fn derive(ivk: &PreparedIncomingViewingKey, d: &Diversifier) -> Option<Self> {
        d.g_d()
            .map(PreparedBaseSubgroup::new)
            .map(|g_d| ka_sapling_derive_public_subgroup_prepared(&ivk.0, &g_d))
            .map(DiversifiedTransmissionKey)
    }

    /// $abst_J(bytes)$
    pub(crate) fn from_bytes(bytes: &[u8; 32]) -> CtOption<Self> {
        jubjub::SubgroupPoint::from_bytes(bytes).map(DiversifiedTransmissionKey)
    }

    /// $repr_J(self)$
    pub(crate) fn to_bytes(self) -> [u8; 32] {
        self.0.to_bytes()
    }

    /// Returns true if this is the identity.
    pub(crate) fn is_identity(&self) -> bool {
        self.0.is_identity().into()
    }

    /// Exposes the inner Jubjub point.
    ///
    /// This API is exposed for `zcash_proof` usage, and will be removed when this type is
    /// refactored into the `sapling-crypto` crate.
    pub fn inner(&self) -> jubjub::SubgroupPoint {
        self.0
    }
}

impl ConditionallySelectable for DiversifiedTransmissionKey {
    fn conditional_select(a: &Self, b: &Self, choice: Choice) -> Self {
        DiversifiedTransmissionKey(jubjub::SubgroupPoint::conditional_select(
            &a.0, &b.0, choice,
        ))
    }
}

/// An ephemeral secret key used to encrypt an output note on-chain.
///
/// `esk` is "ephemeral" in the sense that each secret key is only used once. In
/// practice, `esk` is derived deterministically from the note that it is encrypting.
///
/// $\mathsf{KA}^\mathsf{Sapling}.\mathsf{Private} := \mathbb{F}_{r_J}$
///
/// Defined in [section 5.4.5.3: Sapling Key Agreement][concretesaplingkeyagreement].
///
/// [concretesaplingkeyagreement]: https://zips.z.cash/protocol/protocol.pdf#concretesaplingkeyagreement
#[derive(Debug)]
pub struct EphemeralSecretKey(pub(crate) jubjub::Scalar);

impl ConstantTimeEq for EphemeralSecretKey {
    fn ct_eq(&self, other: &Self) -> subtle::Choice {
        self.0.ct_eq(&other.0)
    }
}

impl EphemeralSecretKey {
    pub(crate) fn from_bytes(bytes: &[u8; 32]) -> CtOption<Self> {
        jubjub::Scalar::from_bytes(bytes).map(EphemeralSecretKey)
    }

    pub(crate) fn derive_public(&self, g_d: jubjub::ExtendedPoint) -> EphemeralPublicKey {
        EphemeralPublicKey(ka_sapling_derive_public(&self.0, &g_d))
    }

    pub(crate) fn agree(&self, pk_d: &DiversifiedTransmissionKey) -> SharedSecret {
        SharedSecret(ka_sapling_agree(&self.0, &pk_d.0.into()))
    }
}

/// An ephemeral public key used to encrypt an output note on-chain.
///
/// `epk` is "ephemeral" in the sense that each public key is only used once. In practice,
/// `epk` is derived deterministically from the note that it is encrypting.
///
/// $\mathsf{KA}^\mathsf{Sapling}.\mathsf{Public} := \mathbb{J}$
///
/// Defined in [section 5.4.5.3: Sapling Key Agreement][concretesaplingkeyagreement].
///
/// [concretesaplingkeyagreement]: https://zips.z.cash/protocol/protocol.pdf#concretesaplingkeyagreement
#[derive(Debug)]
pub struct EphemeralPublicKey(jubjub::ExtendedPoint);

impl EphemeralPublicKey {
    pub(crate) fn from_affine(epk: jubjub::AffinePoint) -> Self {
        EphemeralPublicKey(epk.into())
    }

    pub(crate) fn from_bytes(bytes: &[u8; 32]) -> CtOption<Self> {
        jubjub::ExtendedPoint::from_bytes(bytes).map(EphemeralPublicKey)
    }

    pub(crate) fn to_bytes(&self) -> EphemeralKeyBytes {
        EphemeralKeyBytes(self.0.to_bytes())
    }
}

/// A Sapling ephemeral public key that has been precomputed for trial decryption.
#[derive(Clone, Debug)]
pub struct PreparedEphemeralPublicKey(PreparedBase);

impl PreparedEphemeralPublicKey {
    pub(crate) fn new(epk: EphemeralPublicKey) -> Self {
        PreparedEphemeralPublicKey(PreparedBase::new(epk.0))
    }

    pub(crate) fn agree(&self, ivk: &PreparedIncomingViewingKey) -> SharedSecret {
        SharedSecret(ka_sapling_agree_prepared(&ivk.0, &self.0))
    }
}

/// $\mathsf{KA}^\mathsf{Sapling}.\mathsf{SharedSecret} := \mathbb{J}^{(r)}$
///
/// Defined in [section 5.4.5.3: Sapling Key Agreement][concretesaplingkeyagreement].
///
/// [concretesaplingkeyagreement]: https://zips.z.cash/protocol/protocol.pdf#concretesaplingkeyagreement
#[derive(Debug)]
pub struct SharedSecret(jubjub::SubgroupPoint);

impl SharedSecret {
    /// For checking test vectors only.
    #[cfg(test)]
    pub(crate) fn to_bytes(&self) -> [u8; 32] {
        self.0.to_bytes()
    }

    /// Only for use in batched note encryption.
    pub(crate) fn batch_to_affine(
        shared_secrets: Vec<Option<Self>>,
    ) -> impl Iterator<Item = Option<jubjub::AffinePoint>> {
        // Filter out the positions for which ephemeral_key was not a valid encoding.
        let secrets: Vec<_> = shared_secrets
            .iter()
            .filter_map(|s| s.as_ref().map(|s| jubjub::ExtendedPoint::from(s.0)))
            .collect();

        // Batch-normalize the shared secrets.
        let mut secrets_affine = vec![jubjub::AffinePoint::identity(); secrets.len()];
        group::Curve::batch_normalize(&secrets, &mut secrets_affine);

        // Re-insert the invalid ephemeral_key positions.
        let mut secrets_affine = secrets_affine.into_iter();
        shared_secrets
            .into_iter()
            .map(move |s| s.and_then(|_| secrets_affine.next()))
    }

    /// Defined in [Zcash Protocol Spec § 5.4.5.4: Sapling Key Agreement][concretesaplingkdf].
    ///
    /// [concretesaplingkdf]: https://zips.z.cash/protocol/protocol.pdf#concretesaplingkdf
    pub(crate) fn kdf_sapling(self, ephemeral_key: &EphemeralKeyBytes) -> Blake2bHash {
        Self::kdf_sapling_inner(
            jubjub::ExtendedPoint::from(self.0).to_affine(),
            ephemeral_key,
        )
    }

    /// Only for direct use in batched note encryption.
    pub(crate) fn kdf_sapling_inner(
        secret: jubjub::AffinePoint,
        ephemeral_key: &EphemeralKeyBytes,
    ) -> Blake2bHash {
        Blake2bParams::new()
            .hash_length(32)
            .personal(KDF_SAPLING_PERSONALIZATION)
            .to_state()
            .update(&secret.to_bytes())
            .update(ephemeral_key.as_ref())
            .finalize()
    }
}

#[cfg(any(test, feature = "test-dependencies"))]
#[cfg_attr(docsrs, doc(cfg(feature = "test-dependencies")))]
pub mod testing {
    use proptest::collection::vec;
    use proptest::prelude::*;

    use super::{ExpandedSpendingKey, FullViewingKey, SaplingIvk};

    prop_compose! {
        pub fn arb_expanded_spending_key()(v in vec(any::<u8>(), 32..252)) -> ExpandedSpendingKey {
            ExpandedSpendingKey::from_spending_key(&v)
        }
    }

    prop_compose! {
        pub fn arb_full_viewing_key()(sk in arb_expanded_spending_key()) -> FullViewingKey {
            FullViewingKey::from_expanded_spending_key(&sk)
        }
    }

    prop_compose! {
        pub fn arb_incoming_viewing_key()(fvk in arb_full_viewing_key()) -> SaplingIvk {
            fvk.vk.ivk()
        }
    }
}

#[cfg(test)]
mod tests {
    use group::{Group, GroupEncoding};

    use super::{FullViewingKey, SpendAuthorizingKey, SpendValidatingKey};
    use crate::{constants::SPENDING_KEY_GENERATOR, test_vectors};

    #[test]
    fn ak_must_be_prime_order() {
        let mut buf = [0; 96];
        let identity = jubjub::SubgroupPoint::identity();

        // Set both ak and nk to the identity.
        buf[0..32].copy_from_slice(&identity.to_bytes());
        buf[32..64].copy_from_slice(&identity.to_bytes());

        // ak is not allowed to be the identity.
        assert_eq!(
            FullViewingKey::read(&buf[..]).unwrap_err().to_string(),
            "ak not of prime order"
        );

        // Set ak to a basepoint.
        let basepoint = SPENDING_KEY_GENERATOR;
        buf[0..32].copy_from_slice(&basepoint.to_bytes());

        // nk is allowed to be the identity.
        assert!(FullViewingKey::read(&buf[..]).is_ok());
    }

    #[test]
    fn spend_auth_sig_test_vectors() {
        for tv in test_vectors::signatures::make_test_vectors() {
            let sk = SpendAuthorizingKey::from_bytes(&tv.sk).unwrap();
            let vk = SpendValidatingKey::from_bytes(&tv.vk).unwrap();
            let rvk = redjubjub::VerificationKey::try_from(tv.rvk).unwrap();
            let sig = redjubjub::Signature::from(tv.sig);
            let rsig = redjubjub::Signature::from(tv.rsig);

            let alpha = jubjub::Scalar::from_bytes(&tv.alpha).unwrap();

            assert_eq!(<[u8; 32]>::from(sk.randomize(&alpha)), tv.rsk);
            assert_eq!(vk.randomize(&alpha), rvk);

            // assert_eq!(vk.0.verify(&tv.m, &sig), Ok(()));
            // assert_eq!(rvk.verify(&tv.m, &rsig), Ok(()));
            assert_eq!(
                vk.0.verify(&tv.m, &rsig),
                Err(redjubjub::Error::InvalidSignature),
            );
            assert_eq!(
                rvk.verify(&tv.m, &sig),
                Err(redjubjub::Error::InvalidSignature),
            );
        }
    }
}
