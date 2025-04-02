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

/// This represents an element of $\mathbb{F}_p$ where
///
/// `p = 0x40000000000000000000000000000000224698fc094cf91b992d30ed00000001`
///
/// is the base field of the Pallas curve.
// The internal representation of this type is four 64-bit unsigned
// integers in little-endian order. `Fp` values are always in
// Montgomery form; i.e., Fp(a) = aR mod p, with R = 2^256.
#[derive(Clone, Copy, Eq)]
#[repr(transparent)]
pub struct Fp(pub(crate) [u64; 4]);

impl fmt::Debug for Fp {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        let tmp = self.to_repr();
        write!(f, "0x")?;
        for &b in tmp.iter().rev() {
            write!(f, "{:02x}", b)?;
        }
        Ok(())
    }
}

impl From<bool> for Fp {
    fn from(bit: bool) -> Fp {
        if bit {
            Fp::one()
        } else {
            Fp::zero()
        }
    }
}

impl From<u64> for Fp {
    fn from(val: u64) -> Fp {
        Fp([val, 0, 0, 0]) * R2
    }
}

impl ConstantTimeEq for Fp {
    fn ct_eq(&self, other: &Self) -> Choice {
        self.0[0].ct_eq(&other.0[0])
            & self.0[1].ct_eq(&other.0[1])
            & self.0[2].ct_eq(&other.0[2])
            & self.0[3].ct_eq(&other.0[3])
    }
}

impl PartialEq for Fp {
    #[inline]
    fn eq(&self, other: &Self) -> bool {
        self.ct_eq(other).unwrap_u8() == 1
    }
}

impl core::cmp::Ord for Fp {
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

impl core::cmp::PartialOrd for Fp {
    fn partial_cmp(&self, other: &Self) -> Option<core::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl ConditionallySelectable for Fp {
    fn conditional_select(a: &Self, b: &Self, choice: Choice) -> Self {
        Fp([
            u64::conditional_select(&a.0[0], &b.0[0], choice),
            u64::conditional_select(&a.0[1], &b.0[1], choice),
            u64::conditional_select(&a.0[2], &b.0[2], choice),
            u64::conditional_select(&a.0[3], &b.0[3], choice),
        ])
    }
}

/// Constant representing the modulus
/// p = 0x40000000000000000000000000000000224698fc094cf91b992d30ed00000001
const MODULUS: Fp = Fp([
    0x992d30ed00000001,
    0x224698fc094cf91b,
    0x0000000000000000,
    0x4000000000000000,
]);

/// The modulus as u32 limbs.
#[cfg(not(target_pointer_width = "64"))]
const MODULUS_LIMBS_32: [u32; 8] = [
    0x0000_0001,
    0x992d_30ed,
    0x094c_f91b,
    0x2246_98fc,
    0x0000_0000,
    0x0000_0000,
    0x0000_0000,
    0x4000_0000,
];

impl<'a> Neg for &'a Fp {
    type Output = Fp;

    #[inline]
    fn neg(self) -> Fp {
        self.neg()
    }
}

impl Neg for Fp {
    type Output = Fp;

    #[inline]
    fn neg(self) -> Fp {
        -&self
    }
}

impl<'a, 'b> Sub<&'b Fp> for &'a Fp {
    type Output = Fp;

    #[inline]
    fn sub(self, rhs: &'b Fp) -> Fp {
        self.sub(rhs)
    }
}

impl<'a, 'b> Add<&'b Fp> for &'a Fp {
    type Output = Fp;

    #[inline]
    fn add(self, rhs: &'b Fp) -> Fp {
        self.add(rhs)
    }
}

impl<'a, 'b> Mul<&'b Fp> for &'a Fp {
    type Output = Fp;

    #[inline]
    fn mul(self, rhs: &'b Fp) -> Fp {
        self.mul(rhs)
    }
}

impl_binops_additive!(Fp, Fp);
impl_binops_multiplicative!(Fp, Fp);

impl<T: ::core::borrow::Borrow<Fp>> ::core::iter::Sum<T> for Fp {
    fn sum<I: Iterator<Item = T>>(iter: I) -> Self {
        iter.fold(Self::ZERO, |acc, item| acc + item.borrow())
    }
}

impl<T: ::core::borrow::Borrow<Fp>> ::core::iter::Product<T> for Fp {
    fn product<I: Iterator<Item = T>>(iter: I) -> Self {
        iter.fold(Self::ONE, |acc, item| acc * item.borrow())
    }
}

/// INV = -(p^{-1} mod 2^64) mod 2^64
const INV: u64 = 0x992d30ecffffffff;

/// R = 2^256 mod p
const R: Fp = Fp([
    0x34786d38fffffffd,
    0x992c350be41914ad,
    0xffffffffffffffff,
    0x3fffffffffffffff,
]);

/// R^2 = 2^512 mod p
const R2: Fp = Fp([
    0x8c78ecb30000000f,
    0xd7d30dbd8b0de0e7,
    0x7797a99bc3c95d18,
    0x096d41af7b9cb714,
]);

/// R^3 = 2^768 mod p
const R3: Fp = Fp([
    0xf185a5993a9e10f9,
    0xf6a68f3b6ac5b1d1,
    0xdf8d1014353fd42c,
    0x2ae309222d2d9910,
]);

/// `GENERATOR = 5 mod p` is a generator of the `p - 1` order multiplicative
/// subgroup, or in other words a primitive root of the field.
const GENERATOR: Fp = Fp::from_raw([
    0x0000_0000_0000_0005,
    0x0000_0000_0000_0000,
    0x0000_0000_0000_0000,
    0x0000_0000_0000_0000,
]);

const S: u32 = 32;

/// GENERATOR^t where t * 2^s + 1 = p
/// with t odd. In other words, this
/// is a 2^s root of unity.
const ROOT_OF_UNITY: Fp = Fp::from_raw([
    0xbdad6fabd87ea32f,
    0xea322bf2b7bb7584,
    0x362120830561f81a,
    0x2bce74deac30ebda,
]);

/// GENERATOR^{2^s} where t * 2^s + 1 = p
/// with t odd. In other words, this
/// is a t root of unity.
const DELTA: Fp = Fp::from_raw([
    0x6a6ccd20dd7b9ba2,
    0xf5e4f3f13eee5636,
    0xbd455b7112a5049d,
    0x0a757d0f0006ab6c,
]);

/// `(t - 1) // 2` where t * 2^s + 1 = p with t odd.
#[cfg(any(test, not(feature = "sqrt-table")))]
const T_MINUS1_OVER2: [u64; 4] = [
    0x04a6_7c8d_cc96_9876,
    0x0000_0000_1123_4c7e,
    0x0000_0000_0000_0000,
    0x0000_0000_2000_0000,
];

impl Default for Fp {
    #[inline]
    fn default() -> Self {
        Self::zero()
    }
}

impl Fp {
    /// Returns zero, the additive identity.
    #[inline]
    pub const fn zero() -> Fp {
        Fp([0, 0, 0, 0])
    }

    /// Returns one, the multiplicative identity.
    #[inline]
    pub const fn one() -> Fp {
        R
    }

    /// Doubles this field element.
    #[inline]
    pub const fn double(&self) -> Fp {
        // TODO: This can be achieved more efficiently with a bitshift.
        self.add(self)
    }

    fn from_u512(limbs: [u64; 8]) -> Fp {
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
        let d0 = Fp([limbs[0], limbs[1], limbs[2], limbs[3]]);
        let d1 = Fp([limbs[4], limbs[5], limbs[6], limbs[7]]);
        // Convert to Montgomery form
        d0 * R2 + d1 * R3
    }

    /// Converts from an integer represented in little endian
    /// into its (congruent) `Fp` representation.
    pub const fn from_raw(val: [u64; 4]) -> Self {
        (&Fp(val)).mul(&R2)
    }

    /// Squares this element.
    #[cfg_attr(not(feature = "uninline-portable"), inline)]
    pub const fn square(&self) -> Fp {
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

        Fp::montgomery_reduce(r0, r1, r2, r3, r4, r5, r6, r7)
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
        (&Fp([r4, r5, r6, r7])).sub(&MODULUS)
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

        Fp::montgomery_reduce(r0, r1, r2, r3, r4, r5, r6, r7)
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

        Fp([d0, d1, d2, d3])
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
        (&Fp([d0, d1, d2, d3])).sub(&MODULUS)
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

        Fp([d0 & mask, d1 & mask, d2 & mask, d3 & mask])
    }
}

impl From<Fp> for [u8; 32] {
    fn from(value: Fp) -> [u8; 32] {
        value.to_repr()
    }
}

impl<'a> From<&'a Fp> for [u8; 32] {
    fn from(value: &'a Fp) -> [u8; 32] {
        value.to_repr()
    }
}

impl ff::Field for Fp {
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
            FP_TABLES.sqrt_ratio(num, div)
        }

        #[cfg(not(feature = "sqrt-table"))]
        ff::helpers::sqrt_ratio_generic(num, div)
    }

    #[cfg(feature = "sqrt-table")]
    fn sqrt_alt(&self) -> (Choice, Self) {
        FP_TABLES.sqrt_alt(self)
    }

    /// Computes the square root of this element, if it exists.
    fn sqrt(&self) -> CtOption<Self> {
        #[cfg(feature = "sqrt-table")]
        {
            let (is_square, res) = FP_TABLES.sqrt_alt(self);
            CtOption::new(res, is_square)
        }

        #[cfg(not(feature = "sqrt-table"))]
        ff::helpers::sqrt_tonelli_shanks(self, &T_MINUS1_OVER2)
    }

    /// Computes the multiplicative inverse of this element,
    /// failing if the element is zero.
    fn invert(&self) -> CtOption<Self> {
        let tmp = self.pow_vartime(&[
            0x992d30ecffffffff,
            0x224698fc094cf91b,
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

impl ff::PrimeField for Fp {
    type Repr = [u8; 32];

    const MODULUS: &'static str =
        "0x40000000000000000000000000000000224698fc094cf91b992d30ed00000001";
    const TWO_INV: Self = Fp::from_raw([
        0xcc96987680000001,
        0x11234c7e04a67c8d,
        0x0000000000000000,
        0x2000000000000000,
    ]);
    const NUM_BITS: u32 = 255;
    const CAPACITY: u32 = 254;
    const MULTIPLICATIVE_GENERATOR: Self = GENERATOR;
    const S: u32 = S;
    const ROOT_OF_UNITY: Self = ROOT_OF_UNITY;
    const ROOT_OF_UNITY_INV: Self = Fp::from_raw([
        0xf0b87c7db2ce91f6,
        0x84a0a1d8859f066f,
        0xb4ed8e647196dad1,
        0x2cd5282c53116b5c,
    ]);
    const DELTA: Self = DELTA;

    fn from_u128(v: u128) -> Self {
        Fp::from_raw([v as u64, (v >> 64) as u64, 0, 0])
    }

    fn from_repr(repr: Self::Repr) -> CtOption<Self> {
        let mut tmp = Fp([0, 0, 0, 0]);

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
        let tmp = Fp::montgomery_reduce(self.0[0], self.0[1], self.0[2], self.0[3], 0, 0, 0, 0);

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
#[cfg_attr(docsrs, doc(cfg(feature = "bits")))]
impl PrimeFieldBits for Fp {
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
    static ref FP_TABLES: SqrtTables<Fp> = SqrtTables::new(0x11BE, 1098);
}

impl SqrtTableHelpers for Fp {
    fn pow_by_t_minus1_over2(&self) -> Self {
        let sqr = |x: Fp, i: u32| (0..i).fold(x, |x, _| x.square());

        let r10 = self.square();
        let r11 = r10 * self;
        let r110 = r11.square();
        let r111 = r110 * self;
        let r1001 = r111 * r10;
        let r1101 = r111 * r110;
        let ra = sqr(*self, 129) * self;
        let rb = sqr(ra, 7) * r1001;
        let rc = sqr(rb, 7) * r1101;
        let rd = sqr(rc, 4) * r11;
        let re = sqr(rd, 6) * r111;
        let rf = sqr(re, 3) * r111;
        let rg = sqr(rf, 10) * r1001;
        let rh = sqr(rg, 5) * r1001;
        let ri = sqr(rh, 4) * r1001;
        let rj = sqr(ri, 3) * r111;
        let rk = sqr(rj, 4) * r1001;
        let rl = sqr(rk, 5) * r11;
        let rm = sqr(rl, 4) * r111;
        let rn = sqr(rm, 4) * r11;
        let ro = sqr(rn, 6) * r1001;
        let rp = sqr(ro, 5) * r1101;
        let rq = sqr(rp, 4) * r11;
        let rr = sqr(rq, 7) * r111;
        let rs = sqr(rr, 3) * r11;
        rs.square() // rt
    }

    fn get_lower_32(&self) -> u32 {
        // TODO: don't reduce, just hash the Montgomery form. (Requires rebuilding perfect hash table.)
        let tmp = Fp::montgomery_reduce(self.0[0], self.0[1], self.0[2], self.0[3], 0, 0, 0, 0);

        tmp.0[0] as u32
    }
}

impl WithSmallOrderMulGroup<3> for Fp {
    const ZETA: Self = Fp::from_raw([
        0x1dad5ebdfdfe4ab9,
        0x1d1f8bd237ad3149,
        0x2caad5dc57aab1b0,
        0x12ccca834acdba71,
    ]);
}

impl FromUniformBytes<64> for Fp {
    /// Converts a 512-bit little endian integer into
    /// a `Fp` by reducing by the modulus.
    fn from_uniform_bytes(bytes: &[u8; 64]) -> Fp {
        Fp::from_u512([
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
impl ec_gpu::GpuName for Fp {
    fn name() -> alloc::string::String {
        ec_gpu::name!()
    }
}

#[cfg(feature = "gpu")]
impl ec_gpu::GpuField for Fp {
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
    let v = (Fp::TWO_INV).square().sqrt().unwrap();
    assert!(v == Fp::TWO_INV || (-v) == Fp::TWO_INV);
}

#[test]
fn test_sqrt_32bit_overflow() {
    assert!((Fp::from(5)).sqrt().is_none().unwrap_u8() == 1);
}

#[test]
fn test_pow_by_t_minus1_over2() {
    // NB: TWO_INV is standing in as a "random" field element
    let v = (Fp::TWO_INV).pow_by_t_minus1_over2();
    assert!(v == ff::Field::pow_vartime(&Fp::TWO_INV, &T_MINUS1_OVER2));
}

#[test]
fn test_sqrt_ratio_and_alt() {
    // (true, sqrt(num/div)), if num and div are nonzero and num/div is a square in the field
    let num = (Fp::TWO_INV).square();
    let div = Fp::from(25);
    let div_inverse = div.invert().unwrap();
    let expected = Fp::TWO_INV * Fp::from(5).invert().unwrap();
    let (is_square, v) = Fp::sqrt_ratio(&num, &div);
    assert!(bool::from(is_square));
    assert!(v == expected || (-v) == expected);

    let (is_square_alt, v_alt) = Fp::sqrt_alt(&(num * div_inverse));
    assert!(bool::from(is_square_alt));
    assert!(v_alt == v);

    // (false, sqrt(ROOT_OF_UNITY * num/div)), if num and div are nonzero and num/div is a nonsquare in the field
    let num = num * Fp::ROOT_OF_UNITY;
    let expected = Fp::TWO_INV * Fp::ROOT_OF_UNITY * Fp::from(5).invert().unwrap();
    let (is_square, v) = Fp::sqrt_ratio(&num, &div);
    assert!(!bool::from(is_square));
    assert!(v == expected || (-v) == expected);

    let (is_square_alt, v_alt) = Fp::sqrt_alt(&(num * div_inverse));
    assert!(!bool::from(is_square_alt));
    assert!(v_alt == v);

    // (true, 0), if num is zero
    let num = Fp::zero();
    let expected = Fp::zero();
    let (is_square, v) = Fp::sqrt_ratio(&num, &div);
    assert!(bool::from(is_square));
    assert!(v == expected);

    let (is_square_alt, v_alt) = Fp::sqrt_alt(&(num * div_inverse));
    assert!(bool::from(is_square_alt));
    assert!(v_alt == v);

    // (false, 0), if num is nonzero and div is zero
    let num = (Fp::TWO_INV).square();
    let div = Fp::zero();
    let expected = Fp::zero();
    let (is_square, v) = Fp::sqrt_ratio(&num, &div);
    assert!(!bool::from(is_square));
    assert!(v == expected);
}

#[test]
fn test_zeta() {
    assert_eq!(
        format!("{:?}", Fp::ZETA),
        "0x12ccca834acdba712caad5dc57aab1b01d1f8bd237ad31491dad5ebdfdfe4ab9"
    );

    let a = Fp::ZETA;
    assert!(a != Fp::one());
    let b = a * a;
    assert!(b != Fp::one());
    let c = b * a;
    assert!(c == Fp::one());
}

#[test]
fn test_root_of_unity() {
    assert_eq!(
        Fp::ROOT_OF_UNITY.pow_vartime(&[1 << Fp::S, 0, 0, 0]),
        Fp::one()
    );
}

#[test]
fn test_inv_root_of_unity() {
    assert_eq!(Fp::ROOT_OF_UNITY_INV, Fp::ROOT_OF_UNITY.invert().unwrap());
}

#[test]
fn test_inv_2() {
    assert_eq!(Fp::TWO_INV, Fp::from(2).invert().unwrap());
}

#[test]
fn test_delta() {
    assert_eq!(Fp::DELTA, GENERATOR.pow(&[1u64 << Fp::S, 0, 0, 0]));
    assert_eq!(
        Fp::DELTA,
        Fp::MULTIPLICATIVE_GENERATOR.pow(&[1u64 << Fp::S, 0, 0, 0])
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
        Fp::from_raw([
            0x3daec14d565241d9,
            0x0b7af45b6073944b,
            0xea5b8bd611a5bd4c,
            0x150160330625db3d
        ]),
        Fp::from_u512([
            0xee155641297678a1,
            0xd83e156bdbfdbe65,
            0xd9ccd834c68ba0b5,
            0xf508ede312272758,
            0x038df7cbf8228e89,
            0x3505a1e4a3c74b41,
            0xbfa46f775eb82db3,
            0x26ebe27e262f471d
        ])
    );
}
