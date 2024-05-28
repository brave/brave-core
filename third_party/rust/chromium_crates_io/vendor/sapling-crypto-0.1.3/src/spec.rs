//! Helper functions defined in the Zcash Protocol Specification.

use blake2s_simd::Params as Blake2sParams;
use group::{cofactor::CofactorGroup, ff::PrimeField, Curve, GroupEncoding, WnafBase, WnafScalar};

use super::{
    constants::{
        CRH_IVK_PERSONALIZATION, KEY_DIVERSIFICATION_PERSONALIZATION,
        NOTE_COMMITMENT_RANDOMNESS_GENERATOR, NULLIFIER_POSITION_GENERATOR, PRF_NF_PERSONALIZATION,
    },
    group_hash::group_hash,
    pedersen_hash::{pedersen_hash, Personalization},
};

const PREPARED_WINDOW_SIZE: usize = 4;
pub(crate) type PreparedBase = WnafBase<jubjub::ExtendedPoint, PREPARED_WINDOW_SIZE>;
pub(crate) type PreparedBaseSubgroup = WnafBase<jubjub::SubgroupPoint, PREPARED_WINDOW_SIZE>;
pub(crate) type PreparedScalar = WnafScalar<jubjub::Scalar, PREPARED_WINDOW_SIZE>;

/// $CRH^\mathsf{ivk}(ak, nk)$
///
/// Defined in [Zcash Protocol Spec § 5.4.1.5: CRH^ivk Hash Function][concretecrhivk].
///
/// [concretecrhivk]: https://zips.z.cash/protocol/protocol.pdf#concretecrhivk
pub(crate) fn crh_ivk(ak: [u8; 32], nk: [u8; 32]) -> jubjub::Scalar {
    let mut h: [u8; 32] = Blake2sParams::new()
        .hash_length(32)
        .personal(CRH_IVK_PERSONALIZATION)
        .to_state()
        .update(&ak)
        .update(&nk)
        .finalize()
        .as_bytes()
        .try_into()
        .expect("output length is correct");

    // Drop the most significant five bits, so it can be interpreted as a scalar.
    h[31] &= 0b0000_0111;

    jubjub::Fr::from_repr(h).unwrap()
}

/// Defined in [Zcash Protocol Spec § 5.4.1.6: DiversifyHash^Sapling and DiversifyHash^Orchard Hash Functions][concretediversifyhash].
///
/// [concretediversifyhash]: https://zips.z.cash/protocol/protocol.pdf#concretediversifyhash
pub(crate) fn diversify_hash(d: &[u8; 11]) -> Option<jubjub::SubgroupPoint> {
    group_hash(d, KEY_DIVERSIFICATION_PERSONALIZATION)
}

/// $MixingPedersenHash$.
///
/// Defined in [Zcash Protocol Spec § 5.4.1.8: Mixing Pedersen Hash Function][concretemixinghash].
///
/// [concretemixinghash]: https://zips.z.cash/protocol/protocol.pdf#concretemixinghash
pub(crate) fn mixing_pedersen_hash(
    cm: jubjub::SubgroupPoint,
    position: u64,
) -> jubjub::SubgroupPoint {
    cm + (NULLIFIER_POSITION_GENERATOR * jubjub::Fr::from(position))
}

/// $PRF^\mathsf{nfSapling}_{nk}(\rho)$
///
/// Defined in [Zcash Protocol Spec § 5.4.2: Pseudo Random Functions][concreteprfs].
///
/// [concreteprfs]: https://zips.z.cash/protocol/protocol.pdf#concreteprfs
pub(crate) fn prf_nf(nk: &jubjub::SubgroupPoint, rho: &jubjub::SubgroupPoint) -> [u8; 32] {
    Blake2sParams::new()
        .hash_length(32)
        .personal(PRF_NF_PERSONALIZATION)
        .to_state()
        .update(&nk.to_bytes())
        .update(&rho.to_bytes())
        .finalize()
        .as_bytes()
        .try_into()
        .expect("output length is correct")
}

/// Defined in [Zcash Protocol Spec § 5.4.5.3: Sapling Key Agreement][concretesaplingkeyagreement].
///
/// [concretesaplingkeyagreement]: https://zips.z.cash/protocol/protocol.pdf#concretesaplingkeyagreement
pub(crate) fn ka_sapling_derive_public(
    sk: &jubjub::Scalar,
    b: &jubjub::ExtendedPoint,
) -> jubjub::ExtendedPoint {
    ka_sapling_derive_public_prepared(&PreparedScalar::new(sk), &PreparedBase::new(*b))
}

/// Defined in [Zcash Protocol Spec § 5.4.5.3: Sapling Key Agreement][concretesaplingkeyagreement].
///
/// [concretesaplingkeyagreement]: https://zips.z.cash/protocol/protocol.pdf#concretesaplingkeyagreement
pub(crate) fn ka_sapling_derive_public_prepared(
    sk: &PreparedScalar,
    b: &PreparedBase,
) -> jubjub::ExtendedPoint {
    // [sk] b
    b * sk
}

/// This is defined implicitly by [Zcash Protocol Spec § 4.2.2: Sapling Key Components][saplingkeycomponents]
/// which uses $KA^\mathsf{Sapling}.\mathsf{DerivePublic}$ to produce a diversified
/// transmission key with type $KA^\mathsf{Sapling}.\mathsf{PublicPrimeSubgroup}$.
///
/// [saplingkeycomponents]: https://zips.z.cash/protocol/protocol.pdf#saplingkeycomponents
pub(crate) fn ka_sapling_derive_public_subgroup_prepared(
    sk: &PreparedScalar,
    b: &PreparedBaseSubgroup,
) -> jubjub::SubgroupPoint {
    // [sk] b
    b * sk
}

/// Defined in [Zcash Protocol Spec § 5.4.5.3: Sapling Key Agreement][concretesaplingkeyagreement].
///
/// [concretesaplingkeyagreement]: https://zips.z.cash/protocol/protocol.pdf#concretesaplingkeyagreement
pub(crate) fn ka_sapling_agree(
    sk: &jubjub::Scalar,
    b: &jubjub::ExtendedPoint,
) -> jubjub::SubgroupPoint {
    ka_sapling_agree_prepared(&PreparedScalar::new(sk), &PreparedBase::new(*b))
}

/// Defined in [Zcash Protocol Spec § 5.4.5.3: Sapling Key Agreement][concretesaplingkeyagreement].
///
/// [concretesaplingkeyagreement]: https://zips.z.cash/protocol/protocol.pdf#concretesaplingkeyagreement
pub(crate) fn ka_sapling_agree_prepared(
    sk: &PreparedScalar,
    b: &PreparedBase,
) -> jubjub::SubgroupPoint {
    // [8 sk] b
    // <ExtendedPoint as CofactorGroup>::clear_cofactor is implemented using
    // ExtendedPoint::mul_by_cofactor in the jubjub crate.

    (b * sk).clear_cofactor()
}

/// $WindowedPedersenCommit_r(s)$
///
/// Defined in [Zcash Protocol Spec § 5.4.8.2: Windowed Pedersen commitments][concretewindowedcommit].
///
/// [concretewindowedcommit]: https://zips.z.cash/protocol/protocol.pdf#concretewindowedcommit
pub(crate) fn windowed_pedersen_commit<I>(
    personalization: Personalization,
    s: I,
    r: jubjub::Scalar,
) -> jubjub::SubgroupPoint
where
    I: IntoIterator<Item = bool>,
{
    pedersen_hash(personalization, s) + (NOTE_COMMITMENT_RANDOMNESS_GENERATOR * r)
}

/// Coordinate extractor for Jubjub.
///
/// Defined in [Zcash Protocol Spec § 5.4.9.4: Coordinate Extractor for Jubjub][concreteextractorjubjub].
///
/// [concreteextractorjubjub]: https://zips.z.cash/protocol/protocol.pdf#concreteextractorjubjub
pub(crate) fn extract_p(point: &jubjub::SubgroupPoint) -> bls12_381::Scalar {
    // The commitment is in the prime order subgroup, so mapping the
    // commitment to the u-coordinate is an injective encoding.
    Into::<&jubjub::ExtendedPoint>::into(point)
        .to_affine()
        .get_u()
}
