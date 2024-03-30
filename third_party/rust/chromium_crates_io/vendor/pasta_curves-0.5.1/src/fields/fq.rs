use core::fmt;
use core::ops::{Add, Mul, Neg, Sub};

use ff::{Field, FromUniformBytes, PrimeField, WithSmallOrderMulGroup};
use rand::RngCore;
use subtle::{Choice, ConditionallySelectable, ConstantTimeEq, CtOption};

#[cfg(feature = "sqrt-table")]
use lazy_static::lazy_static;

#[cfg(feature = "bits")]
use ff::{FieldBits, PrimeFieldBits};

use crate::arithmetic::{adc, mac, sbb, SqrtTableHelpers};

#[cfg(feature = "sqrt-table")]
use crate::arithmetic::SqrtTables;

/// This represents an element of $\mathbb{F}_q$ where
///
/// `q = 0x40000000000000000000000000000000224698fc0994a8dd8c46eb2100000001`
///
/// is the base field of the Vesta curve.
// The internal representation of this type is four 64-bit unsigned
// integers in little-endian order. `Fq` values are always in
// Montgomery form; i.e., Fq(a) = aR mod q, with R = 2^256.
#[derive(Clone, Copy, Eq)]
#[repr(transparent)]
pub struct Fq(pub(crate) [u64; 4]);

impl fmt::Debug for Fq {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        let tmp = self.to_repr();
        write!(f, "0x")?;
        for &b in tmp.iter().rev() {
            write!(f, "{:02x}", b)?;
        }
        Ok(())
    }
}

impl From<bool> for Fq {
    fn from(bit: bool) -> Fq {
        if bit {
            Fq::one()
        } else {
            Fq::zero()
        }
    }
}

impl From<u64> for Fq {
    fn from(val: u64) -> Fq {
        Fq([val, 0, 0, 0]) * R2
    }
}

impl ConstantTimeEq for Fq {
    fn ct_eq(&self, other: &Self) -> Choice {
        self.0[0].ct_eq(&other.0[0])
            & self.0[1].ct_eq(&other.0[1])
            & self.0[2].ct_eq(&other.0[2])
            & self.0[3].ct_eq(&other.0[3])
    }
}

impl PartialEq for Fq {
    #[inline]
    fn eq(&self, other: &Self) -> bool {
        self.ct_eq(other).unwrap_u8() == 1
    }
}

impl core::cmp::Ord for Fq {
    fn cmp(&self, other: &Self) -> core::cmp::Ordering {
        let left = self.to_repr();
        let right = other.to_repr();
        left.iter()
            .zip(right.iter())
            .rev()
            .find_map(|(left_byte, right_byte)| match left_byte.cmp(right_byte) {
                core::cmp::Ordering::Equal => None,
                res => Some(res),
            })
            .unwrap_or(core::cmp::Ordering::Equal)
    }
}

impl core::cmp::PartialOrd for Fq {
    fn partial_cmp(&self, other: &Self) -> Option<core::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl ConditionallySelectable for Fq {
    fn conditional_select(a: &Self, b: &Self, choice: Choice) -> Self {
        Fq([
            u64::conditional_select(&a.0[0], &b.0[0], choice),
            u64::conditional_select(&a.0[1], &b.0[1], choice),
            u64::conditional_select(&a.0[2], &b.0[2], choice),
            u64::conditional_select(&a.0[3], &b.0[3], choice),
        ])
    }
}

/// Constant representing the modulus
/// q = 0x40000000000000000000000000000000224698fc0994a8dd8c46eb2100000001
const MODULUS: Fq = Fq([
    0x8c46eb2100000001,
    0x224698fc0994a8dd,
    0x0,
    0x4000000000000000,
]);

/// The modulus as u32 limbs.
#[cfg(not(target_pointer_width = "64"))]
const MODULUS_LIMBS_32: [u32; 8] = [
    0x0000_0001,
    0x8c46_eb21,
    0x0994_a8dd,
    0x2246_98fc,
    0x0000_0000,
    0x0000_0000,
    0x0000_0000,
    0x4000_0000,
];

impl<'a> Neg for &'a Fq {
    type Output = Fq;

    #[inline]
    fn neg(self) -> Fq {
        self.neg()
    }
}

impl Neg for Fq {
    type Output = Fq;

    #[inline]
    fn neg(self) -> Fq {
        -&self
    }
}

impl<'a, 'b> Sub<&'b Fq> for &'a Fq {
    type Output = Fq;

    #[inline]
    fn sub(self, rhs: &'b Fq) -> Fq {
        self.sub(rhs)
    }
}

impl<'a, 'b> Add<&'b Fq> for &'a Fq {
    type Output = Fq;

    #[inline]
    fn add(self, rhs: &'b Fq) -> Fq {
        self.add(rhs)
    }
}

impl<'a, 'b> Mul<&'b Fq> for &'a Fq {
    type Output = Fq;

    #[inline]
    fn mul(self, rhs: &'b Fq) -> Fq {
        self.mul(rhs)
    }
}

impl_binops_additive!(Fq, Fq);
impl_binops_multiplicative!(Fq, Fq);

impl<T: ::core::borrow::Borrow<Fq>> ::core::iter::Sum<T> for Fq {
    fn sum<I: Iterator<Item = T>>(iter: I) -> Self {
        iter.fold(Self::ZERO, |acc, item| acc + item.borrow())
    }
}

impl<T: ::core::borrow::Borrow<Fq>> ::core::iter::Product<T> for Fq {
    fn product<I: Iterator<Item = T>>(iter: I) -> Self {
        iter.fold(Self::ONE, |acc, item| acc * item.borrow())
    }
}

/// INV = -(q^{-1} mod 2^64) mod 2^64
const INV: u64 = 0x8c46eb20ffffffff;

/// R = 2^256 mod q
const R: Fq = Fq([
    0x5b2b3e9cfffffffd,
    0x992c350be3420567,
    0xffffffffffffffff,
    0x3fffffffffffffff,
]);

/// R^2 = 2^512 mod q
const R2: Fq = Fq([
    0xfc9678ff0000000f,
    0x67bb433d891a16e3,
    0x7fae231004ccf590,
    0x096d41af7ccfdaa9,
]);

/// R^3 = 2^768 mod q
const R3: Fq = Fq([
    0x008b421c249dae4c,
    0xe13bda50dba41326,
    0x88fececb8e15cb63,
    0x07dd97a06e6792c8,
]);

/// `GENERATOR = 5 mod q` is a generator of the `q - 1` order multiplicative
/// subgroup, or in other words a primitive root of the field.
const GENERATOR: Fq = Fq::from_raw([
    0x0000_0000_0000_0005,
    0x0000_0000_0000_0000,
    0x0000_0000_0000_0000,
    0x0000_0000_0000_0000,
]);

const S: u32 = 32;

/// GENERATOR^t where t * 2^s + 1 = q
/// with t odd. In other words, this
/// is a 2^s root of unity.
const ROOT_OF_UNITY: Fq = Fq::from_raw([
    0xa70e2c1102b6d05f,
    0x9bb97ea3c106f049,
    0x9e5c4dfd492ae26e,
    0x2de6a9b8746d3f58,
]);

/// GENERATOR^{2^s} where t * 2^s + 1 = q
/// with t odd. In other words, this
/// is a t root of unity.
const DELTA: Fq = Fq::from_raw([
    0x8494392472d1683c,
    0xe3ac3376541d1140,
    0x06f0a88e7f7949f8,
    0x2237d54423724166,
]);

/// `(t - 1) // 2` where t * 2^s + 1 = p with t odd.
#[cfg(any(test, not(feature = "sqrt-table")))]
const T_MINUS1_OVER2: [u64; 4] = [
    0x04ca_546e_c623_7590,
    0x0000_0000_1123_4c7e,
    0x0000_0000_0000_0000,
    0x0000_0000_2000_0000,
];

impl Default for Fq {
    #[inline]
    fn default() -> Self {
        Self::zero()
    }
}

impl Fq {
    /// Returns zero, the additive identity.
    #[inline]
    pub const fn zero() -> Fq {
        Fq([0, 0, 0, 0])
    }

    /// Returns one, the multiplicative identity.
    #[inline]
    pub const fn one() -> Fq {
        R
    }

    /// Doubles this field element.
    #[inline]
    pub const fn double(&self) -> Fq {
        // TODO: This can be achieved more efficiently with a bitshift.
        self.add(self)
    }

    fn from_u512(limbs: [u64; 8]) -> Fq {
        // We reduce an arbitrary 512-bit number by decomposing it into two 256-bit digits
        // with the higher bits multiplied by 2^256. Thus, we perform two reductions
        //
        // 1. the lower bits are multiplied by R^2, as normal
        // 2. the upper bits are multiplied by R^2 * 2^256 = R^3
        //
        // and computing their sum in the field. It remains to see that arbitrary 256-bit
        // numbers can be placed into Montgomery form safely using the reduction. The
        // reduction works so long as the product is less than R=2^256 multiplied by
        // the modulus. This holds because for any `c` smaller than the modulus, we have
        // that (2^256 - 1)*c is an acceptable product for the reduction. Therefore, the
        // reduction always works so long as `c` is in the field; in this case it is either the
        // constant `R2` or `R3`.
        let d0 = Fq([limbs[0], limbs[1], limbs[2], limbs[3]]);
        let d1 = Fq([limbs[4], limbs[5], limbs[6], limbs[7]]);
        // Convert to Montgomery form
        d0 * R2 + d1 * R3
    }

    /// Converts from an integer represented in little endian
    /// into its (congruent) `Fq` representation.
    pub const fn from_raw(val: [u64; 4]) -> Self {
        (&Fq(val)).mul(&R2)
    }

    /// Squares this element.
    #[cfg_attr(not(feature = "uninline-portable"), inline)]
    pub const fn square(&self) -> Fq {
        let (r1, carry) = mac(0, self.0[0], self.0[1], 0);
        let (r2, carry) = mac(0, self.0[0], self.0[2], carry);
        let (r3, r4) = mac(0, self.0[0], self.0[3], carry);

        let (r3, carry) = mac(r3, self.0[1], self.0[2], 0);
        let (r4, r5) = mac(r4, self.0[1], self.0[3], carry);

        let (r5, r6) = mac(r5, self.0[2], self.0[3], 0);

        let r7 = r6 >> 63;
        let r6 = (r6 << 1) | (r5 >> 63);
        let r5 = (r5 << 1) | (r4 >> 63);
        let r4 = (r4 << 1) | (r3 >> 63);
        let r3 = (r3 << 1) | (r2 >> 63);
        let r2 = (r2 << 1) | (r1 >> 63);
        let r1 = r1 << 1;

        let (r0, carry) = mac(0, self.0[0], self.0[0], 0);
        let (r1, carry) = adc(0, r1, carry);
        let (r2, carry) = mac(r2, self.0[1], self.0[1], carry);
        let (r3, carry) = adc(0, r3, carry);
        let (r4, carry) = mac(r4, self.0[2], self.0[2], carry);
        let (r5, carry) = adc(0, r5, carry);
        let (r6, carry) = mac(r6, self.0[3], self.0[3], carry);
        let (r7, _) = adc(0, r7, carry);

        Fq::montgomery_reduce(r0, r1, r2, r3, r4, r5, r6, r7)
    }

    #[allow(clippy::too_many_arguments)]
    #[cfg_attr(not(feature = "uninline-portable"), inline(always))]
    const fn montgomery_reduce(
        r0: u64,
        r1: u64,
        r2: u64,
        r3: u64,
        r4: u64,
        r5: u64,
        r6: u64,
        r7: u64,
    ) -> Self {
        // The Montgomery reduction here is based on Algorithm 14.32 in
        // Handbook of Applied Cryptography
        // <http://cacr.uwaterloo.ca/hac/about/chap14.pdf>.

        let k = r0.wrapping_mul(INV);
        let (_, carry) = mac(r0, k, MODULUS.0[0], 0);
        let (r1, carry) = mac(r1, k, MODULUS.0[1], carry);
        let (r2, carry) = mac(r2, k, MODULUS.0[2], carry);
        let (r3, carry) = mac(r3, k, MODULUS.0[3], carry);
        let (r4, carry2) = adc(r4, 0, carry);

        let k = r1.wrapping_mul(INV);
        let (_, carry) = mac(r1, k, MODULUS.0[0], 0);
        let (r2, carry) = mac(r2, k, MODULUS.0[1], carry);
        let (r3, carry) = mac(r3, k, MODULUS.0[2], carry);
        let (r4, carry) = mac(r4, k, MODULUS.0[3], carry);
        let (r5, carry2) = adc(r5, carry2, carry);

        let k = r2.wrapping_mul(INV);
        let (_, carry) = mac(r2, k, MODULUS.0[0], 0);
        let (r3, carry) = mac(r3, k, MODULUS.0[1], carry);
        let (r4, carry) = mac(r4, k, MODULUS.0[2], carry);
        let (r5, carry) = mac(r5, k, MODULUS.0[3], carry);
        let (r6, carry2) = adc(r6, carry2, carry);

        let k = r3.wrapping_mul(INV);
        let (_, carry) = mac(r3, k, MODULUS.0[0], 0);
        let (r4, carry) = mac(r4, k, MODULUS.0[1], carry);
        let (r5, carry) = mac(r5, k, MODULUS.0[2], carry);
        let (r6, carry) = mac(r6, k, MODULUS.0[3], carry);
        let (r7, _) = adc(r7, carry2, carry);

        // Result may be within MODULUS of the correct value
        (&Fq([r4, r5, r6, r7])).sub(&MODULUS)
    }

    /// Multiplies `rhs` by `self`, returning the result.
    #[cfg_attr(not(feature = "uninline-portable"), inline)]
    pub const fn mul(&self, rhs: &Self) -> Self {
        // Schoolbook multiplication

        let (r0, carry) = mac(0, self.0[0], rhs.0[0], 0);
        let (r1, carry) = mac(0, self.0[0], rhs.0[1], carry);
        let (r2, carry) = mac(0, self.0[0], rhs.0[2], carry);
        let (r3, r4) = mac(0, self.0[0], rhs.0[3], carry);

        let (r1, carry) = mac(r1, self.0[1], rhs.0[0], 0);
        let (r2, carry) = mac(r2, self.0[1], rhs.0[1], carry);
        let (r3, carry) = mac(r3, self.0[1], rhs.0[2], carry);
        let (r4, r5) = mac(r4, self.0[1], rhs.0[3], carry);

        let (r2, carry) = mac(r2, self.0[2], rhs.0[0], 0);
        let (r3, carry) = mac(r3, self.0[2], rhs.0[1], carry);
        let (r4, carry) = mac(r4, self.0[2], rhs.0[2], carry);
        let (r5, r6) = mac(r5, self.0[2], rhs.0[3], carry);

        let (r3, carry) = mac(r3, self.0[3], rhs.0[0], 0);
        let (r4, carry) = mac(r4, self.0[3], rhs.0[1], carry);
        let (r5, carry) = mac(r5, self.0[3], rhs.0[2], carry);
        let (r6, r7) = mac(r6, self.0[3], rhs.0[3], carry);

        Fq::montgomery_reduce(r0, r1, r2, r3, r4, r5, r6, r7)
    }

    /// Subtracts `rhs` from `self`, returning the result.
    #[cfg_attr(not(feature = "uninline-portable"), inline)]
    pub const fn sub(&self, rhs: &Self) -> Self {
        let (d0, borrow) = sbb(self.0[0], rhs.0[0], 0);
        let (d1, borrow) = sbb(self.0[1], rhs.0[1], borrow);
        let (d2, borrow) = sbb(self.0[2], rhs.0[2], borrow);
        let (d3, borrow) = sbb(self.0[3], rhs.0[3], borrow);

        // If underflow occurred on the final limb, borrow = 0xfff...fff, otherwise
        // borrow = 0x000...000. Thus, we use it as a mask to conditionally add the modulus.
        let (d0, carry) = adc(d0, MODULUS.0[0] & borrow, 0);
        let (d1, carry) = adc(d1, MODULUS.0[1] & borrow, carry);
        let (d2, carry) = adc(d2, MODULUS.0[2] & borrow, carry);
        let (d3, _) = adc(d3, MODULUS.0[3] & borrow, carry);

        Fq([d0, d1, d2, d3])
    }

    /// Adds `rhs` to `self`, returning the result.
    #[cfg_attr(not(feature = "uninline-portable"), inline)]
    pub const fn add(&self, rhs: &Self) -> Self {
        let (d0, carry) = adc(self.0[0], rhs.0[0], 0);
        let (d1, carry) = adc(self.0[1], rhs.0[1], carry);
        let (d2, carry) = adc(self.0[2], rhs.0[2], carry);
        let (d3, _) = adc(self.0[3], rhs.0[3], carry);

        // Attempt to subtract the modulus, to ensure the value
        // is smaller than the modulus.
        (&Fq([d0, d1, d2, d3])).sub(&MODULUS)
    }

    /// Negates `self`.
    #[cfg_attr(not(feature = "uninline-portable"), inline)]
    pub const fn neg(&self) -> Self {
        // Subtract `self` from `MODULUS` to negate. Ignore the final
        // borrow because it cannot underflow; self is guaranteed to
        // be in the field.
        let (d0, borrow) = sbb(MODULUS.0[0], self.0[0], 0);
        let (d1, borrow) = sbb(MODULUS.0[1], self.0[1], borrow);
        let (d2, borrow) = sbb(MODULUS.0[2], self.0[2], borrow);
        let (d3, _) = sbb(MODULUS.0[3], self.0[3], borrow);

        // `tmp` could be `MODULUS` if `self` was zero. Create a mask that is
        // zero if `self` was zero, and `u64::max_value()` if self was nonzero.
        let mask = (((self.0[0] | self.0[1] | self.0[2] | self.0[3]) == 0) as u64).wrapping_sub(1);

        Fq([d0 & mask, d1 & mask, d2 & mask, d3 & mask])
    }
}

impl From<Fq> for [u8; 32] {
    fn from(value: Fq) -> [u8; 32] {
        value.to_repr()
    }
}

impl<'a> From<&'a Fq> for [u8; 32] {
    fn from(value: &'a Fq) -> [u8; 32] {
        value.to_repr()
    }
}

impl ff::Field for Fq {
    const ZERO: Self = Self::zero();
    const ONE: Self = Self::one();

    fn random(mut rng: impl RngCore) -> Self {
        Self::from_u512([
            rng.next_u64(),
            rng.next_u64(),
            rng.next_u64(),
            rng.next_u64(),
            rng.next_u64(),
            rng.next_u64(),
            rng.next_u64(),
            rng.next_u64(),
        ])
    }

    fn double(&self) -> Self {
        self.double()
    }

    #[inline(always)]
    fn square(&self) -> Self {
        self.square()
    }

    fn sqrt_ratio(num: &Self, div: &Self) -> (Choice, Self) {
        #[cfg(feature = "sqrt-table")]
        {
            FQ_TABLES.sqrt_ratio(num, div)
        }

        #[cfg(not(feature = "sqrt-table"))]
        ff::helpers::sqrt_ratio_generic(num, div)
    }

    #[cfg(feature = "sqrt-table")]
    fn sqrt_alt(&self) -> (Choice, Self) {
        FQ_TABLES.sqrt_alt(self)
    }

    /// Computes the square root of this element, if it exists.
    fn sqrt(&self) -> CtOption<Self> {
        #[cfg(feature = "sqrt-table")]
        {
            let (is_square, res) = FQ_TABLES.sqrt_alt(self);
            CtOption::new(res, is_square)
        }

        #[cfg(not(feature = "sqrt-table"))]
        ff::helpers::sqrt_tonelli_shanks(self, &T_MINUS1_OVER2)
    }

    /// Computes the multiplicative inverse of this element,
    /// failing if the element is zero.
    fn invert(&self) -> CtOption<Self> {
        let tmp = self.pow_vartime(&[
            0x8c46eb20ffffffff,
            0x224698fc0994a8dd,
            0x0,
            0x4000000000000000,
        ]);

        CtOption::new(tmp, !self.ct_eq(&Self::zero()))
    }

    fn pow_vartime<S: AsRef<[u64]>>(&self, exp: S) -> Self {
        let mut res = Self::one();
        let mut found_one = false;
        for e in exp.as_ref().iter().rev() {
            for i in (0..64).rev() {
                if found_one {
                    res = res.square();
                }

                if ((*e >> i) & 1) == 1 {
                    found_one = true;
                    res *= self;
                }
            }
        }
        res
    }
}

impl ff::PrimeField for Fq {
    type Repr = [u8; 32];

    const MODULUS: &'static str =
        "0x40000000000000000000000000000000224698fc0994a8dd8c46eb2100000001";
    const NUM_BITS: u32 = 255;
    const CAPACITY: u32 = 254;
    const TWO_INV: Self = Fq::from_raw([
        0xc623759080000001,
        0x11234c7e04ca546e,
        0x0000000000000000,
        0x2000000000000000,
    ]);
    const MULTIPLICATIVE_GENERATOR: Self = GENERATOR;
    const S: u32 = S;
    const ROOT_OF_UNITY: Self = ROOT_OF_UNITY;
    const ROOT_OF_UNITY_INV: Self = Fq::from_raw([
        0x57eecda0a84b6836,
        0x4ad38b9084b8a80c,
        0xf4c8f353124086c1,
        0x2235e1a7415bf936,
    ]);
    const DELTA: Self = DELTA;

    fn from_u128(v: u128) -> Self {
        Fq::from_raw([v as u64, (v >> 64) as u64, 0, 0])
    }

    fn from_repr(repr: Self::Repr) -> CtOption<Self> {
        let mut tmp = Fq([0, 0, 0, 0]);

        tmp.0[0] = u64::from_le_bytes(repr[0..8].try_into().unwrap());
        tmp.0[1] = u64::from_le_bytes(repr[8..16].try_into().unwrap());
        tmp.0[2] = u64::from_le_bytes(repr[16..24].try_into().unwrap());
        tmp.0[3] = u64::from_le_bytes(repr[24..32].try_into().unwrap());

        // Try to subtract the modulus
        let (_, borrow) = sbb(tmp.0[0], MODULUS.0[0], 0);
        let (_, borrow) = sbb(tmp.0[1], MODULUS.0[1], borrow);
        let (_, borrow) = sbb(tmp.0[2], MODULUS.0[2], borrow);
        let (_, borrow) = sbb(tmp.0[3], MODULUS.0[3], borrow);

        // If the element is smaller than MODULUS then the
        // subtraction will underflow, producing a borrow value
        // of 0xffff...ffff. Otherwise, it'll be zero.
        let is_some = (borrow as u8) & 1;

        // Convert to Montgomery form by computing
        // (a.R^0 * R^2) / R = a.R
        tmp *= &R2;

        CtOption::new(tmp, Choice::from(is_some))
    }

    fn to_repr(&self) -> Self::Repr {
        // Turn into canonical form by computing
        // (a.R) / R = a
        let tmp = Fq::montgomery_reduce(self.0[0], self.0[1], self.0[2], self.0[3], 0, 0, 0, 0);

        let mut res = [0; 32];
        res[0..8].copy_from_slice(&tmp.0[0].to_le_bytes());
        res[8..16].copy_from_slice(&tmp.0[1].to_le_bytes());
        res[16..24].copy_from_slice(&tmp.0[2].to_le_bytes());
        res[24..32].copy_from_slice(&tmp.0[3].to_le_bytes());

        res
    }

    fn is_odd(&self) -> Choice {
        Choice::from(self.to_repr()[0] & 1)
    }
}

#[cfg(all(feature = "bits", not(target_pointer_width = "64")))]
type ReprBits = [u32; 8];

#[cfg(all(feature = "bits", target_pointer_width = "64"))]
type ReprBits = [u64; 4];

#[cfg(feature = "bits")]
impl PrimeFieldBits for Fq {
    type ReprBits = ReprBits;

    fn to_le_bits(&self) -> FieldBits<Self::ReprBits> {
        let bytes = self.to_repr();

        #[cfg(not(target_pointer_width = "64"))]
        let limbs = [
            u32::from_le_bytes(bytes[0..4].try_into().unwrap()),
            u32::from_le_bytes(bytes[4..8].try_into().unwrap()),
            u32::from_le_bytes(bytes[8..12].try_into().unwrap()),
            u32::from_le_bytes(bytes[12..16].try_into().unwrap()),
            u32::from_le_bytes(bytes[16..20].try_into().unwrap()),
            u32::from_le_bytes(bytes[20..24].try_into().unwrap()),
            u32::from_le_bytes(bytes[24..28].try_into().unwrap()),
            u32::from_le_bytes(bytes[28..32].try_into().unwrap()),
        ];

        #[cfg(target_pointer_width = "64")]
        let limbs = [
            u64::from_le_bytes(bytes[0..8].try_into().unwrap()),
            u64::from_le_bytes(bytes[8..16].try_into().unwrap()),
            u64::from_le_bytes(bytes[16..24].try_into().unwrap()),
            u64::from_le_bytes(bytes[24..32].try_into().unwrap()),
        ];

        FieldBits::new(limbs)
    }

    fn char_le_bits() -> FieldBits<Self::ReprBits> {
        #[cfg(not(target_pointer_width = "64"))]
        {
            FieldBits::new(MODULUS_LIMBS_32)
        }

        #[cfg(target_pointer_width = "64")]
        FieldBits::new(MODULUS.0)
    }
}

#[cfg(feature = "sqrt-table")]
lazy_static! {
    // The perfect hash parameters are found by `squareroottab.sage` in zcash/pasta.
    #[cfg_attr(docsrs, doc(cfg(feature = "sqrt-table")))]
    static ref FQ_TABLES: SqrtTables<Fq> = SqrtTables::new(0x116A9E, 1206);
}

impl SqrtTableHelpers for Fq {
    fn pow_by_t_minus1_over2(&self) -> Self {
        let sqr = |x: Fq, i: u32| (0..i).fold(x, |x, _| x.square());

        let s10 = self.square();
        let s11 = s10 * self;
        let s111 = s11.square() * self;
        let s1001 = s111 * s10;
        let s1011 = s1001 * s10;
        let s1101 = s1011 * s10;
        let sa = sqr(*self, 129) * self;
        let sb = sqr(sa, 7) * s1001;
        let sc = sqr(sb, 7) * s1101;
        let sd = sqr(sc, 4) * s11;
        let se = sqr(sd, 6) * s111;
        let sf = sqr(se, 3) * s111;
        let sg = sqr(sf, 10) * s1001;
        let sh = sqr(sg, 4) * s1001;
        let si = sqr(sh, 5) * s1001;
        let sj = sqr(si, 5) * s1001;
        let sk = sqr(sj, 3) * s1001;
        let sl = sqr(sk, 4) * s1011;
        let sm = sqr(sl, 4) * s1011;
        let sn = sqr(sm, 5) * s11;
        let so = sqr(sn, 4) * self;
        let sp = sqr(so, 5) * s11;
        let sq = sqr(sp, 4) * s111;
        let sr = sqr(sq, 5) * s1011;
        let ss = sqr(sr, 3) * self;
        sqr(ss, 4) // st
    }

    fn get_lower_32(&self) -> u32 {
        // TODO: don't reduce, just hash the Montgomery form. (Requires rebuilding perfect hash table.)
        let tmp = Fq::montgomery_reduce(self.0[0], self.0[1], self.0[2], self.0[3], 0, 0, 0, 0);

        tmp.0[0] as u32
    }
}

impl WithSmallOrderMulGroup<3> for Fq {
    const ZETA: Self = Fq::from_raw([
        0x2aa9d2e050aa0e4f,
        0x0fed467d47c033af,
        0x511db4d81cf70f5a,
        0x06819a58283e528e,
    ]);
}

impl FromUniformBytes<64> for Fq {
    /// Converts a 512-bit little endian integer into
    /// a `Fq` by reducing by the modulus.
    fn from_uniform_bytes(bytes: &[u8; 64]) -> Fq {
        Fq::from_u512([
            u64::from_le_bytes(bytes[0..8].try_into().unwrap()),
            u64::from_le_bytes(bytes[8..16].try_into().unwrap()),
            u64::from_le_bytes(bytes[16..24].try_into().unwrap()),
            u64::from_le_bytes(bytes[24..32].try_into().unwrap()),
            u64::from_le_bytes(bytes[32..40].try_into().unwrap()),
            u64::from_le_bytes(bytes[40..48].try_into().unwrap()),
            u64::from_le_bytes(bytes[48..56].try_into().unwrap()),
            u64::from_le_bytes(bytes[56..64].try_into().unwrap()),
        ])
    }
}

#[cfg(feature = "gpu")]
impl ec_gpu::GpuName for Fq {
    fn name() -> alloc::string::String {
        ec_gpu::name!()
    }
}

#[cfg(feature = "gpu")]
impl ec_gpu::GpuField for Fq {
    fn one() -> alloc::vec::Vec<u32> {
        crate::fields::u64_to_u32(&R.0[..])
    }

    fn r2() -> alloc::vec::Vec<u32> {
        crate::fields::u64_to_u32(&R2.0[..])
    }

    fn modulus() -> alloc::vec::Vec<u32> {
        crate::fields::u64_to_u32(&MODULUS.0[..])
    }
}

#[test]
fn test_inv() {
    // Compute -(r^{-1} mod 2^64) mod 2^64 by exponentiating
    // by totient(2**64) - 1

    let mut inv = 1u64;
    for _ in 0..63 {
        inv = inv.wrapping_mul(inv);
        inv = inv.wrapping_mul(MODULUS.0[0]);
    }
    inv = inv.wrapping_neg();

    assert_eq!(inv, INV);
}

#[test]
fn test_sqrt() {
    // NB: TWO_INV is standing in as a "random" field element
    let v = (Fq::TWO_INV).square().sqrt().unwrap();
    assert!(v == Fq::TWO_INV || (-v) == Fq::TWO_INV);
}

#[test]
fn test_sqrt_32bit_overflow() {
    assert!((Fq::from(5)).sqrt().is_none().unwrap_u8() == 1);
}

#[test]
fn test_pow_by_t_minus1_over2() {
    // NB: TWO_INV is standing in as a "random" field element
    let v = (Fq::TWO_INV).pow_by_t_minus1_over2();
    assert!(v == ff::Field::pow_vartime(&Fq::TWO_INV, &T_MINUS1_OVER2));
}

#[test]
fn test_sqrt_ratio_and_alt() {
    // (true, sqrt(num/div)), if num and div are nonzero and num/div is a square in the field
    let num = (Fq::TWO_INV).square();
    let div = Fq::from(25);
    let div_inverse = div.invert().unwrap();
    let expected = Fq::TWO_INV * Fq::from(5).invert().unwrap();
    let (is_square, v) = Fq::sqrt_ratio(&num, &div);
    assert!(bool::from(is_square));
    assert!(v == expected || (-v) == expected);

    let (is_square_alt, v_alt) = Fq::sqrt_alt(&(num * div_inverse));
    assert!(bool::from(is_square_alt));
    assert!(v_alt == v);

    // (false, sqrt(ROOT_OF_UNITY * num/div)), if num and div are nonzero and num/div is a nonsquare in the field
    let num = num * Fq::ROOT_OF_UNITY;
    let expected = Fq::TWO_INV * Fq::ROOT_OF_UNITY * Fq::from(5).invert().unwrap();
    let (is_square, v) = Fq::sqrt_ratio(&num, &div);
    assert!(!bool::from(is_square));
    assert!(v == expected || (-v) == expected);

    let (is_square_alt, v_alt) = Fq::sqrt_alt(&(num * div_inverse));
    assert!(!bool::from(is_square_alt));
    assert!(v_alt == v);

    // (true, 0), if num is zero
    let num = Fq::zero();
    let expected = Fq::zero();
    let (is_square, v) = Fq::sqrt_ratio(&num, &div);
    assert!(bool::from(is_square));
    assert!(v == expected);

    let (is_square_alt, v_alt) = Fq::sqrt_alt(&(num * div_inverse));
    assert!(bool::from(is_square_alt));
    assert!(v_alt == v);

    // (false, 0), if num is nonzero and div is zero
    let num = (Fq::TWO_INV).square();
    let div = Fq::zero();
    let expected = Fq::zero();
    let (is_square, v) = Fq::sqrt_ratio(&num, &div);
    assert!(!bool::from(is_square));
    assert!(v == expected);
}

#[test]
fn test_zeta() {
    assert_eq!(
        format!("{:?}", Fq::ZETA),
        "0x06819a58283e528e511db4d81cf70f5a0fed467d47c033af2aa9d2e050aa0e4f"
    );
    let a = Fq::ZETA;
    assert!(a != Fq::one());
    let b = a * a;
    assert!(b != Fq::one());
    let c = b * a;
    assert!(c == Fq::one());
}

#[test]
fn test_root_of_unity() {
    assert_eq!(
        Fq::ROOT_OF_UNITY.pow_vartime(&[1 << Fq::S, 0, 0, 0]),
        Fq::one()
    );
}

#[test]
fn test_inv_root_of_unity() {
    assert_eq!(Fq::ROOT_OF_UNITY_INV, Fq::ROOT_OF_UNITY.invert().unwrap());
}

#[test]
fn test_inv_2() {
    assert_eq!(Fq::TWO_INV, Fq::from(2).invert().unwrap());
}

#[test]
fn test_delta() {
    assert_eq!(Fq::DELTA, GENERATOR.pow(&[1u64 << Fq::S, 0, 0, 0]));
    assert_eq!(
        Fq::DELTA,
        Fq::MULTIPLICATIVE_GENERATOR.pow(&[1u64 << Fq::S, 0, 0, 0])
    );
}

#[cfg(not(target_pointer_width = "64"))]
#[test]
fn consistent_modulus_limbs() {
    for (a, &b) in MODULUS
        .0
        .iter()
        .flat_map(|&limb| {
            Some(limb as u32)
                .into_iter()
                .chain(Some((limb >> 32) as u32))
        })
        .zip(MODULUS_LIMBS_32.iter())
    {
        assert_eq!(a, b);
    }
}

#[test]
fn test_from_u512() {
    assert_eq!(
        Fq::from_raw([
            0xe22bd0d1b22cc43e,
            0x6b84e5b52490a7c8,
            0x264262941ac9e229,
            0x27dcfdf361ce4254
        ]),
        Fq::from_u512([
            0x64a80cce0b5a2369,
            0x84f2ef0501bc783c,
            0x696e5e63c86bbbde,
            0x924072f52dc6cc62,
            0x8288a507c8d61128,
            0x3b2efb1ef697e3fe,
            0x75a4998d06855f27,
            0x52ea589e69712cc0
        ])
    );
}
