//! Helper functions defined in the Zcash Protocol Specification.

use core::iter;
use core::ops::Deref;

use ff::{Field, FromUniformBytes, PrimeField, PrimeFieldBits};
use group::{Curve, Group, GroupEncoding, WnafBase, WnafScalar};
use halo2_gadgets::{poseidon::primitives as poseidon, sinsemilla::primitives as sinsemilla};
use halo2_proofs::arithmetic::{CurveAffine, CurveExt};
use memuse::DynamicUsage;
use pasta_curves::pallas;
use subtle::{ConditionallySelectable, CtOption};

use crate::constants::{
    fixed_bases::COMMIT_IVK_PERSONALIZATION, util::gen_const_array,
    KEY_DIVERSIFICATION_PERSONALIZATION, L_ORCHARD_BASE,
};

pub(crate) use zcash_spec::PrfExpand;

/// A Pallas point that is guaranteed to not be the identity.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) struct NonIdentityPallasPoint(pallas::Point);

impl Default for NonIdentityPallasPoint {
    fn default() -> Self {
        NonIdentityPallasPoint(pallas::Point::generator())
    }
}

impl ConditionallySelectable for NonIdentityPallasPoint {
    fn conditional_select(a: &Self, b: &Self, choice: subtle::Choice) -> Self {
        NonIdentityPallasPoint(pallas::Point::conditional_select(&a.0, &b.0, choice))
    }
}

impl NonIdentityPallasPoint {
    pub(crate) fn from_bytes(bytes: &[u8; 32]) -> CtOption<Self> {
        pallas::Point::from_bytes(bytes)
            .and_then(|p| CtOption::new(NonIdentityPallasPoint(p), !p.is_identity()))
    }
}

impl Deref for NonIdentityPallasPoint {
    type Target = pallas::Point;

    fn deref(&self) -> &pallas::Point {
        &self.0
    }
}

/// An integer in [1..q_P].
#[derive(Clone, Copy, Debug)]
pub(crate) struct NonZeroPallasBase(pallas::Base);

impl Default for NonZeroPallasBase {
    fn default() -> Self {
        NonZeroPallasBase(pallas::Base::one())
    }
}

impl ConditionallySelectable for NonZeroPallasBase {
    fn conditional_select(a: &Self, b: &Self, choice: subtle::Choice) -> Self {
        NonZeroPallasBase(pallas::Base::conditional_select(&a.0, &b.0, choice))
    }
}

impl NonZeroPallasBase {
    pub(crate) fn from_bytes(bytes: &[u8; 32]) -> CtOption<Self> {
        pallas::Base::from_repr(*bytes).and_then(NonZeroPallasBase::from_base)
    }

    pub(crate) fn to_bytes(self) -> [u8; 32] {
        self.0.to_repr()
    }

    pub(crate) fn from_base(b: pallas::Base) -> CtOption<Self> {
        CtOption::new(NonZeroPallasBase(b), !b.is_zero())
    }

    /// Constructs a wrapper for a base field element that is guaranteed to be non-zero.
    ///
    /// # Panics
    ///
    /// Panics if `s.is_zero()`.
    fn guaranteed(s: pallas::Base) -> Self {
        assert!(!bool::from(s.is_zero()));
        NonZeroPallasBase(s)
    }
}

/// An integer in [1..r_P].
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub(crate) struct NonZeroPallasScalar(pallas::Scalar);

impl Default for NonZeroPallasScalar {
    fn default() -> Self {
        NonZeroPallasScalar(pallas::Scalar::one())
    }
}

impl From<NonZeroPallasBase> for NonZeroPallasScalar {
    fn from(s: NonZeroPallasBase) -> Self {
        NonZeroPallasScalar::guaranteed(mod_r_p(s.0))
    }
}

impl ConditionallySelectable for NonZeroPallasScalar {
    fn conditional_select(a: &Self, b: &Self, choice: subtle::Choice) -> Self {
        NonZeroPallasScalar(pallas::Scalar::conditional_select(&a.0, &b.0, choice))
    }
}

impl NonZeroPallasScalar {
    pub(crate) fn from_bytes(bytes: &[u8; 32]) -> CtOption<Self> {
        pallas::Scalar::from_repr(*bytes).and_then(NonZeroPallasScalar::from_scalar)
    }

    pub(crate) fn from_scalar(s: pallas::Scalar) -> CtOption<Self> {
        CtOption::new(NonZeroPallasScalar(s), !s.is_zero())
    }

    /// Constructs a wrapper for a scalar field element that is guaranteed to be non-zero.
    ///
    /// # Panics
    ///
    /// Panics if `s.is_zero()`.
    fn guaranteed(s: pallas::Scalar) -> Self {
        assert!(!bool::from(s.is_zero()));
        NonZeroPallasScalar(s)
    }
}

impl Deref for NonZeroPallasScalar {
    type Target = pallas::Scalar;

    fn deref(&self) -> &pallas::Scalar {
        &self.0
    }
}

const PREPARED_WINDOW_SIZE: usize = 4;

#[derive(Clone, Debug)]
pub(crate) struct PreparedNonIdentityBase(WnafBase<pallas::Point, PREPARED_WINDOW_SIZE>);

impl PreparedNonIdentityBase {
    pub(crate) fn new(base: NonIdentityPallasPoint) -> Self {
        PreparedNonIdentityBase(WnafBase::new(base.0))
    }
}

#[derive(Clone, Debug)]
pub(crate) struct PreparedNonZeroScalar(WnafScalar<pallas::Scalar, PREPARED_WINDOW_SIZE>);

impl DynamicUsage for PreparedNonZeroScalar {
    fn dynamic_usage(&self) -> usize {
        self.0.dynamic_usage()
    }

    fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
        self.0.dynamic_usage_bounds()
    }
}

impl PreparedNonZeroScalar {
    pub(crate) fn new(scalar: &NonZeroPallasScalar) -> Self {
        PreparedNonZeroScalar(WnafScalar::new(scalar))
    }
}

/// $\mathsf{ToBase}^\mathsf{Orchard}(x) := LEOS2IP_{\ell_\mathsf{PRFexpand}}(x) (mod q_P)$
///
/// Defined in [Zcash Protocol Spec § 4.2.3: Orchard Key Components][orchardkeycomponents].
///
/// [orchardkeycomponents]: https://zips.z.cash/protocol/nu5.pdf#orchardkeycomponents
pub(crate) fn to_base(x: [u8; 64]) -> pallas::Base {
    pallas::Base::from_uniform_bytes(&x)
}

/// $\mathsf{ToScalar}^\mathsf{Orchard}(x) := LEOS2IP_{\ell_\mathsf{PRFexpand}}(x) (mod r_P)$
///
/// Defined in [Zcash Protocol Spec § 4.2.3: Orchard Key Components][orchardkeycomponents].
///
/// [orchardkeycomponents]: https://zips.z.cash/protocol/nu5.pdf#orchardkeycomponents
pub(crate) fn to_scalar(x: [u8; 64]) -> pallas::Scalar {
    pallas::Scalar::from_uniform_bytes(&x)
}

/// Converts from pallas::Base to pallas::Scalar (aka $x \pmod{r_\mathbb{P}}$).
///
/// This requires no modular reduction because Pallas' base field is smaller than its
/// scalar field.
pub(crate) fn mod_r_p(x: pallas::Base) -> pallas::Scalar {
    pallas::Scalar::from_repr(x.to_repr()).unwrap()
}

/// Defined in [Zcash Protocol Spec § 5.4.8.4: Sinsemilla commitments][concretesinsemillacommit].
///
/// [concretesinsemillacommit]: https://zips.z.cash/protocol/protocol.pdf#concretesinsemillacommit
pub(crate) fn commit_ivk(
    ak: &pallas::Base,
    nk: &pallas::Base,
    rivk: &pallas::Scalar,
) -> CtOption<pallas::Base> {
    // We rely on the API contract that to_le_bits() returns at least PrimeField::NUM_BITS
    // bits, which is equal to L_ORCHARD_BASE.
    let domain = sinsemilla::CommitDomain::new(COMMIT_IVK_PERSONALIZATION);
    domain.short_commit(
        iter::empty()
            .chain(ak.to_le_bits().iter().by_vals().take(L_ORCHARD_BASE))
            .chain(nk.to_le_bits().iter().by_vals().take(L_ORCHARD_BASE)),
        rivk,
    )
}

/// Defined in [Zcash Protocol Spec § 5.4.1.6: DiversifyHash^Sapling and DiversifyHash^Orchard Hash Functions][concretediversifyhash].
///
/// [concretediversifyhash]: https://zips.z.cash/protocol/nu5.pdf#concretediversifyhash
pub(crate) fn diversify_hash(d: &[u8; 11]) -> NonIdentityPallasPoint {
    let hasher = pallas::Point::hash_to_curve(KEY_DIVERSIFICATION_PERSONALIZATION);
    let g_d = hasher(d);
    // If the identity occurs, we replace it with a different fixed point.
    // TODO: Replace the unwrap_or_else with a cached fixed point.
    NonIdentityPallasPoint(CtOption::new(g_d, !g_d.is_identity()).unwrap_or_else(|| hasher(&[])))
}

/// $PRF^\mathsf{nfOrchard}(nk, \rho) := Poseidon(nk, \rho)$
///
/// Defined in [Zcash Protocol Spec § 5.4.2: Pseudo Random Functions][concreteprfs].
///
/// [concreteprfs]: https://zips.z.cash/protocol/nu5.pdf#concreteprfs
pub(crate) fn prf_nf(nk: pallas::Base, rho: pallas::Base) -> pallas::Base {
    poseidon::Hash::<_, poseidon::P128Pow5T3, poseidon::ConstantLength<2>, 3, 2>::init()
        .hash([nk, rho])
}

/// Defined in [Zcash Protocol Spec § 5.4.5.5: Orchard Key Agreement][concreteorchardkeyagreement].
///
/// [concreteorchardkeyagreement]: https://zips.z.cash/protocol/nu5.pdf#concreteorchardkeyagreement
pub(crate) fn ka_orchard(
    sk: &NonZeroPallasScalar,
    b: &NonIdentityPallasPoint,
) -> NonIdentityPallasPoint {
    ka_orchard_prepared(
        &PreparedNonZeroScalar::new(sk),
        &PreparedNonIdentityBase::new(*b),
    )
}

/// Defined in [Zcash Protocol Spec § 5.4.5.5: Orchard Key Agreement][concreteorchardkeyagreement].
///
/// [concreteorchardkeyagreement]: https://zips.z.cash/protocol/nu5.pdf#concreteorchardkeyagreement
pub(crate) fn ka_orchard_prepared(
    sk: &PreparedNonZeroScalar,
    b: &PreparedNonIdentityBase,
) -> NonIdentityPallasPoint {
    NonIdentityPallasPoint(&b.0 * &sk.0)
}

/// Coordinate extractor for Pallas.
///
/// Defined in [Zcash Protocol Spec § 5.4.9.7: Coordinate Extractor for Pallas][concreteextractorpallas].
///
/// [concreteextractorpallas]: https://zips.z.cash/protocol/nu5.pdf#concreteextractorpallas
pub(crate) fn extract_p(point: &pallas::Point) -> pallas::Base {
    point
        .to_affine()
        .coordinates()
        .map(|c| *c.x())
        .unwrap_or_else(pallas::Base::zero)
}

/// Coordinate extractor for Pallas.
///
/// Defined in [Zcash Protocol Spec § 5.4.9.7: Coordinate Extractor for Pallas][concreteextractorpallas].
///
/// [concreteextractorpallas]: https://zips.z.cash/protocol/nu5.pdf#concreteextractorpallas
pub(crate) fn extract_p_bottom(point: CtOption<pallas::Point>) -> CtOption<pallas::Base> {
    point.map(|p| extract_p(&p))
}

/// The field element representation of a u64 integer represented by
/// an L-bit little-endian bitstring.
pub fn lebs2ip_field<F: PrimeField, const L: usize>(bits: &[bool; L]) -> F {
    F::from(lebs2ip::<L>(bits))
}

/// The u64 integer represented by an L-bit little-endian bitstring.
///
/// # Panics
///
/// Panics if the bitstring is longer than 64 bits.
pub fn lebs2ip<const L: usize>(bits: &[bool; L]) -> u64 {
    assert!(L <= 64);
    bits.iter()
        .enumerate()
        .fold(0u64, |acc, (i, b)| acc + if *b { 1 << i } else { 0 })
}

/// The sequence of bits representing a u64 in little-endian order.
///
/// # Panics
///
/// Panics if the expected length of the sequence `NUM_BITS` exceeds
/// 64.
pub fn i2lebsp<const NUM_BITS: usize>(int: u64) -> [bool; NUM_BITS] {
    assert!(NUM_BITS <= 64);
    gen_const_array(|mask: usize| (int & (1 << mask)) != 0)
}

#[cfg(test)]
mod tests {
    use super::{i2lebsp, lebs2ip};

    use group::Group;
    use halo2_proofs::arithmetic::CurveExt;
    use pasta_curves::pallas;
    use rand::{rngs::OsRng, RngCore};

    #[test]
    fn diversify_hash_substitution() {
        assert!(!bool::from(
            pallas::Point::hash_to_curve("z.cash:Orchard-gd")(&[]).is_identity()
        ));
    }

    #[test]
    fn lebs2ip_round_trip() {
        let mut rng = OsRng;
        {
            let int = rng.next_u64();
            assert_eq!(lebs2ip::<64>(&i2lebsp(int)), int);
        }

        assert_eq!(lebs2ip::<64>(&i2lebsp(0)), 0);
        assert_eq!(
            lebs2ip::<64>(&i2lebsp(0xFFFFFFFFFFFFFFFF)),
            0xFFFFFFFFFFFFFFFF
        );
    }

    #[test]
    fn i2lebsp_round_trip() {
        {
            let bitstring = [0; 64].map(|_| rand::random());
            assert_eq!(i2lebsp(lebs2ip(&bitstring)), bitstring);
        }

        {
            let bitstring = [false; 64];
            assert_eq!(i2lebsp(lebs2ip(&bitstring)), bitstring);
        }

        {
            let bitstring = [true; 64];
            assert_eq!(i2lebsp(lebs2ip(&bitstring)), bitstring);
        }

        {
            let bitstring = [];
            assert_eq!(i2lebsp(lebs2ip(&bitstring)), bitstring);
        }
    }
}
