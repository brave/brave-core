//! This module provides an implementation of the BLS12-381 base field `GF(p)`
//! where `p = 0x1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab`

use core::fmt;
use core::ops::{Add, AddAssign, Mul, MulAssign, Neg, Sub, SubAssign};
use rand_core::RngCore;
use subtle::{Choice, ConditionallySelectable, ConstantTimeEq, CtOption};

use crate::util::{adc, mac, sbb};

// The internal representation of this type is six 64-bit unsigned
// integers in little-endian order. `Fp` values are always in
// Montgomery form; i.e., Scalar(a) = aR mod p, with R = 2^384.
#[derive(Copy, Clone)]
pub struct Fp(pub(crate) [u64; 6]);

impl fmt::Debug for Fp {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        let tmp = self.to_bytes();
        write!(f, "0x")?;
        for &b in tmp.iter() {
            write!(f, "{:02x}", b)?;
        }
        Ok(())
    }
}

impl Default for Fp {
    fn default() -> Self {
        Fp::zero()
    }
}

#[cfg(feature = "zeroize")]
impl zeroize::DefaultIsZeroes for Fp {}

impl ConstantTimeEq for Fp {
    fn ct_eq(&self, other: &Self) -> Choice {
        self.0[0].ct_eq(&other.0[0])
            & self.0[1].ct_eq(&other.0[1])
            & self.0[2].ct_eq(&other.0[2])
            & self.0[3].ct_eq(&other.0[3])
            & self.0[4].ct_eq(&other.0[4])
            & self.0[5].ct_eq(&other.0[5])
    }
}

impl Eq for Fp {}
impl PartialEq for Fp {
    #[inline]
    fn eq(&self, other: &Self) -> bool {
        bool::from(self.ct_eq(other))
    }
}

impl ConditionallySelectable for Fp {
    fn conditional_select(a: &Self, b: &Self, choice: Choice) -> Self {
        Fp([
            u64::conditional_select(&a.0[0], &b.0[0], choice),
            u64::conditional_select(&a.0[1], &b.0[1], choice),
            u64::conditional_select(&a.0[2], &b.0[2], choice),
            u64::conditional_select(&a.0[3], &b.0[3], choice),
            u64::conditional_select(&a.0[4], &b.0[4], choice),
            u64::conditional_select(&a.0[5], &b.0[5], choice),
        ])
    }
}

/// p = 4002409555221667393417789825735904156556882819939007885332058136124031650490837864442687629129015664037894272559787
const MODULUS: [u64; 6] = [
    0xb9fe_ffff_ffff_aaab,
    0x1eab_fffe_b153_ffff,
    0x6730_d2a0_f6b0_f624,
    0x6477_4b84_f385_12bf,
    0x4b1b_a7b6_434b_acd7,
    0x1a01_11ea_397f_e69a,
];

/// INV = -(p^{-1} mod 2^64) mod 2^64
const INV: u64 = 0x89f3_fffc_fffc_fffd;

/// R = 2^384 mod p
const R: Fp = Fp([
    0x7609_0000_0002_fffd,
    0xebf4_000b_c40c_0002,
    0x5f48_9857_53c7_58ba,
    0x77ce_5853_7052_5745,
    0x5c07_1a97_a256_ec6d,
    0x15f6_5ec3_fa80_e493,
]);

/// R2 = 2^(384*2) mod p
const R2: Fp = Fp([
    0xf4df_1f34_1c34_1746,
    0x0a76_e6a6_09d1_04f1,
    0x8de5_476c_4c95_b6d5,
    0x67eb_88a9_939d_83c0,
    0x9a79_3e85_b519_952d,
    0x1198_8fe5_92ca_e3aa,
]);

/// R3 = 2^(384*3) mod p
const R3: Fp = Fp([
    0xed48_ac6b_d94c_a1e0,
    0x315f_831e_03a7_adf8,
    0x9a53_352a_615e_29dd,
    0x34c0_4e5e_921e_1761,
    0x2512_d435_6572_4728,
    0x0aa6_3460_9175_5d4d,
]);

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

impl Fp {
    /// Returns zero, the additive identity.
    #[inline]
    pub const fn zero() -> Fp {
        Fp([0, 0, 0, 0, 0, 0])
    }

    /// Returns one, the multiplicative identity.
    #[inline]
    pub const fn one() -> Fp {
        R
    }

    pub fn is_zero(&self) -> Choice {
        self.ct_eq(&Fp::zero())
    }

    /// Attempts to convert a big-endian byte representation of
    /// a scalar into an `Fp`, failing if the input is not canonical.
    pub fn from_bytes(bytes: &[u8; 48]) -> CtOption<Fp> {
        let mut tmp = Fp([0, 0, 0, 0, 0, 0]);

        tmp.0[5] = u64::from_be_bytes(<[u8; 8]>::try_from(&bytes[0..8]).unwrap());
        tmp.0[4] = u64::from_be_bytes(<[u8; 8]>::try_from(&bytes[8..16]).unwrap());
        tmp.0[3] = u64::from_be_bytes(<[u8; 8]>::try_from(&bytes[16..24]).unwrap());
        tmp.0[2] = u64::from_be_bytes(<[u8; 8]>::try_from(&bytes[24..32]).unwrap());
        tmp.0[1] = u64::from_be_bytes(<[u8; 8]>::try_from(&bytes[32..40]).unwrap());
        tmp.0[0] = u64::from_be_bytes(<[u8; 8]>::try_from(&bytes[40..48]).unwrap());

        // Try to subtract the modulus
        let (_, borrow) = sbb(tmp.0[0], MODULUS[0], 0);
        let (_, borrow) = sbb(tmp.0[1], MODULUS[1], borrow);
        let (_, borrow) = sbb(tmp.0[2], MODULUS[2], borrow);
        let (_, borrow) = sbb(tmp.0[3], MODULUS[3], borrow);
        let (_, borrow) = sbb(tmp.0[4], MODULUS[4], borrow);
        let (_, borrow) = sbb(tmp.0[5], MODULUS[5], borrow);

        // If the element is smaller than MODULUS then the
        // subtraction will underflow, producing a borrow value
        // of 0xffff...ffff. Otherwise, it'll be zero.
        let is_some = (borrow as u8) & 1;

        // Convert to Montgomery form by computing
        // (a.R^0 * R^2) / R = a.R
        tmp *= &R2;

        CtOption::new(tmp, Choice::from(is_some))
    }

    /// Converts an element of `Fp` into a byte representation in
    /// big-endian byte order.
    pub fn to_bytes(self) -> [u8; 48] {
        // Turn into canonical form by computing
        // (a.R) / R = a
        let tmp = Fp::montgomery_reduce(
            self.0[0], self.0[1], self.0[2], self.0[3], self.0[4], self.0[5], 0, 0, 0, 0, 0, 0,
        );

        let mut res = [0; 48];
        res[0..8].copy_from_slice(&tmp.0[5].to_be_bytes());
        res[8..16].copy_from_slice(&tmp.0[4].to_be_bytes());
        res[16..24].copy_from_slice(&tmp.0[3].to_be_bytes());
        res[24..32].copy_from_slice(&tmp.0[2].to_be_bytes());
        res[32..40].copy_from_slice(&tmp.0[1].to_be_bytes());
        res[40..48].copy_from_slice(&tmp.0[0].to_be_bytes());

        res
    }

    pub(crate) fn random(mut rng: impl RngCore) -> Fp {
        let mut bytes = [0u8; 96];
        rng.fill_bytes(&mut bytes);

        // Parse the random bytes as a big-endian number, to match Fp encoding order.
        Fp::from_u768([
            u64::from_be_bytes(<[u8; 8]>::try_from(&bytes[0..8]).unwrap()),
            u64::from_be_bytes(<[u8; 8]>::try_from(&bytes[8..16]).unwrap()),
            u64::from_be_bytes(<[u8; 8]>::try_from(&bytes[16..24]).unwrap()),
            u64::from_be_bytes(<[u8; 8]>::try_from(&bytes[24..32]).unwrap()),
            u64::from_be_bytes(<[u8; 8]>::try_from(&bytes[32..40]).unwrap()),
            u64::from_be_bytes(<[u8; 8]>::try_from(&bytes[40..48]).unwrap()),
            u64::from_be_bytes(<[u8; 8]>::try_from(&bytes[48..56]).unwrap()),
            u64::from_be_bytes(<[u8; 8]>::try_from(&bytes[56..64]).unwrap()),
            u64::from_be_bytes(<[u8; 8]>::try_from(&bytes[64..72]).unwrap()),
            u64::from_be_bytes(<[u8; 8]>::try_from(&bytes[72..80]).unwrap()),
            u64::from_be_bytes(<[u8; 8]>::try_from(&bytes[80..88]).unwrap()),
            u64::from_be_bytes(<[u8; 8]>::try_from(&bytes[88..96]).unwrap()),
        ])
    }

    /// Reduces a big-endian 64-bit limb representation of a 768-bit number.
    fn from_u768(limbs: [u64; 12]) -> Fp {
        // We reduce an arbitrary 768-bit number by decomposing it into two 384-bit digits
        // with the higher bits multiplied by 2^384. Thus, we perform two reductions
        //
        // 1. the lower bits are multiplied by R^2, as normal
        // 2. the upper bits are multiplied by R^2 * 2^384 = R^3
        //
        // and computing their sum in the field. It remains to see that arbitrary 384-bit
        // numbers can be placed into Montgomery form safely using the reduction. The
        // reduction works so long as the product is less than R=2^384 multiplied by
        // the modulus. This holds because for any `c` smaller than the modulus, we have
        // that (2^384 - 1)*c is an acceptable product for the reduction. Therefore, the
        // reduction always works so long as `c` is in the field; in this case it is either the
        // constant `R2` or `R3`.
        let d1 = Fp([limbs[11], limbs[10], limbs[9], limbs[8], limbs[7], limbs[6]]);
        let d0 = Fp([limbs[5], limbs[4], limbs[3], limbs[2], limbs[1], limbs[0]]);
        // Convert to Montgomery form
        d0 * R2 + d1 * R3
    }

    /// Returns whether or not this element is strictly lexicographically
    /// larger than its negation.
    pub fn lexicographically_largest(&self) -> Choice {
        // This can be determined by checking to see if the element is
        // larger than (p - 1) // 2. If we subtract by ((p - 1) // 2) + 1
        // and there is no underflow, then the element must be larger than
        // (p - 1) // 2.

        // First, because self is in Montgomery form we need to reduce it
        let tmp = Fp::montgomery_reduce(
            self.0[0], self.0[1], self.0[2], self.0[3], self.0[4], self.0[5], 0, 0, 0, 0, 0, 0,
        );

        let (_, borrow) = sbb(tmp.0[0], 0xdcff_7fff_ffff_d556, 0);
        let (_, borrow) = sbb(tmp.0[1], 0x0f55_ffff_58a9_ffff, borrow);
        let (_, borrow) = sbb(tmp.0[2], 0xb398_6950_7b58_7b12, borrow);
        let (_, borrow) = sbb(tmp.0[3], 0xb23b_a5c2_79c2_895f, borrow);
        let (_, borrow) = sbb(tmp.0[4], 0x258d_d3db_21a5_d66b, borrow);
        let (_, borrow) = sbb(tmp.0[5], 0x0d00_88f5_1cbf_f34d, borrow);

        // If the element was smaller, the subtraction will underflow
        // producing a borrow value of 0xffff...ffff, otherwise it will
        // be zero. We create a Choice representing true if there was
        // overflow (and so this element is not lexicographically larger
        // than its negation) and then negate it.

        !Choice::from((borrow as u8) & 1)
    }

    /// Constructs an element of `Fp` without checking that it is
    /// canonical.
    pub const fn from_raw_unchecked(v: [u64; 6]) -> Fp {
        Fp(v)
    }

    /// Although this is labeled "vartime", it is only
    /// variable time with respect to the exponent. It
    /// is also not exposed in the public API.
    pub fn pow_vartime(&self, by: &[u64; 6]) -> Self {
        let mut res = Self::one();
        for e in by.iter().rev() {
            for i in (0..64).rev() {
                res = res.square();

                if ((*e >> i) & 1) == 1 {
                    res *= self;
                }
            }
        }
        res
    }

    #[inline]
    pub fn sqrt(&self) -> CtOption<Self> {
        // We use Shank's method, as p = 3 (mod 4). This means
        // we only need to exponentiate by (p+1)/4. This only
        // works for elements that are actually quadratic residue,
        // so we check that we got the correct result at the end.

        let sqrt = self.pow_vartime(&[
            0xee7f_bfff_ffff_eaab,
            0x07aa_ffff_ac54_ffff,
            0xd9cc_34a8_3dac_3d89,
            0xd91d_d2e1_3ce1_44af,
            0x92c6_e9ed_90d2_eb35,
            0x0680_447a_8e5f_f9a6,
        ]);

        CtOption::new(sqrt, sqrt.square().ct_eq(self))
    }

    #[inline]
    /// Computes the multiplicative inverse of this field
    /// element, returning None in the case that this element
    /// is zero.
    pub fn invert(&self) -> CtOption<Self> {
        // Exponentiate by p - 2
        let t = self.pow_vartime(&[
            0xb9fe_ffff_ffff_aaa9,
            0x1eab_fffe_b153_ffff,
            0x6730_d2a0_f6b0_f624,
            0x6477_4b84_f385_12bf,
            0x4b1b_a7b6_434b_acd7,
            0x1a01_11ea_397f_e69a,
        ]);

        CtOption::new(t, !self.is_zero())
    }

    #[inline]
    const fn subtract_p(&self) -> Fp {
        let (r0, borrow) = sbb(self.0[0], MODULUS[0], 0);
        let (r1, borrow) = sbb(self.0[1], MODULUS[1], borrow);
        let (r2, borrow) = sbb(self.0[2], MODULUS[2], borrow);
        let (r3, borrow) = sbb(self.0[3], MODULUS[3], borrow);
        let (r4, borrow) = sbb(self.0[4], MODULUS[4], borrow);
        let (r5, borrow) = sbb(self.0[5], MODULUS[5], borrow);

        // If underflow occurred on the final limb, borrow = 0xfff...fff, otherwise
        // borrow = 0x000...000. Thus, we use it as a mask!
        let r0 = (self.0[0] & borrow) | (r0 & !borrow);
        let r1 = (self.0[1] & borrow) | (r1 & !borrow);
        let r2 = (self.0[2] & borrow) | (r2 & !borrow);
        let r3 = (self.0[3] & borrow) | (r3 & !borrow);
        let r4 = (self.0[4] & borrow) | (r4 & !borrow);
        let r5 = (self.0[5] & borrow) | (r5 & !borrow);

        Fp([r0, r1, r2, r3, r4, r5])
    }

    #[inline]
    pub const fn add(&self, rhs: &Fp) -> Fp {
        let (d0, carry) = adc(self.0[0], rhs.0[0], 0);
        let (d1, carry) = adc(self.0[1], rhs.0[1], carry);
        let (d2, carry) = adc(self.0[2], rhs.0[2], carry);
        let (d3, carry) = adc(self.0[3], rhs.0[3], carry);
        let (d4, carry) = adc(self.0[4], rhs.0[4], carry);
        let (d5, _) = adc(self.0[5], rhs.0[5], carry);

        // Attempt to subtract the modulus, to ensure the value
        // is smaller than the modulus.
        (&Fp([d0, d1, d2, d3, d4, d5])).subtract_p()
    }

    #[inline]
    pub const fn neg(&self) -> Fp {
        let (d0, borrow) = sbb(MODULUS[0], self.0[0], 0);
        let (d1, borrow) = sbb(MODULUS[1], self.0[1], borrow);
        let (d2, borrow) = sbb(MODULUS[2], self.0[2], borrow);
        let (d3, borrow) = sbb(MODULUS[3], self.0[3], borrow);
        let (d4, borrow) = sbb(MODULUS[4], self.0[4], borrow);
        let (d5, _) = sbb(MODULUS[5], self.0[5], borrow);

        // Let's use a mask if `self` was zero, which would mean
        // the result of the subtraction is p.
        let mask = (((self.0[0] | self.0[1] | self.0[2] | self.0[3] | self.0[4] | self.0[5]) == 0)
            as u64)
            .wrapping_sub(1);

        Fp([
            d0 & mask,
            d1 & mask,
            d2 & mask,
            d3 & mask,
            d4 & mask,
            d5 & mask,
        ])
    }

    #[inline]
    pub const fn sub(&self, rhs: &Fp) -> Fp {
        (&rhs.neg()).add(self)
    }

    /// Returns `c = a.zip(b).fold(0, |acc, (a_i, b_i)| acc + a_i * b_i)`.
    ///
    /// Implements Algorithm 2 from Patrick Longa's
    /// [ePrint 2022-367](https://eprint.iacr.org/2022/367) §3.
    #[inline]
    pub(crate) fn sum_of_products<const T: usize>(a: [Fp; T], b: [Fp; T]) -> Fp {
        // For a single `a x b` multiplication, operand scanning (schoolbook) takes each
        // limb of `a` in turn, and multiplies it by all of the limbs of `b` to compute
        // the result as a double-width intermediate representation, which is then fully
        // reduced at the end. Here however we have pairs of multiplications (a_i, b_i),
        // the results of which are summed.
        //
        // The intuition for this algorithm is two-fold:
        // - We can interleave the operand scanning for each pair, by processing the jth
        //   limb of each `a_i` together. As these have the same offset within the overall
        //   operand scanning flow, their results can be summed directly.
        // - We can interleave the multiplication and reduction steps, resulting in a
        //   single bitshift by the limb size after each iteration. This means we only
        //   need to store a single extra limb overall, instead of keeping around all the
        //   intermediate results and eventually having twice as many limbs.

        // Algorithm 2, line 2
        let (u0, u1, u2, u3, u4, u5) =
            (0..6).fold((0, 0, 0, 0, 0, 0), |(u0, u1, u2, u3, u4, u5), j| {
                // Algorithm 2, line 3
                // For each pair in the overall sum of products:
                let (t0, t1, t2, t3, t4, t5, t6) = (0..T).fold(
                    (u0, u1, u2, u3, u4, u5, 0),
                    |(t0, t1, t2, t3, t4, t5, t6), i| {
                        // Compute digit_j x row and accumulate into `u`.
                        let (t0, carry) = mac(t0, a[i].0[j], b[i].0[0], 0);
                        let (t1, carry) = mac(t1, a[i].0[j], b[i].0[1], carry);
                        let (t2, carry) = mac(t2, a[i].0[j], b[i].0[2], carry);
                        let (t3, carry) = mac(t3, a[i].0[j], b[i].0[3], carry);
                        let (t4, carry) = mac(t4, a[i].0[j], b[i].0[4], carry);
                        let (t5, carry) = mac(t5, a[i].0[j], b[i].0[5], carry);
                        let (t6, _) = adc(t6, 0, carry);

                        (t0, t1, t2, t3, t4, t5, t6)
                    },
                );

                // Algorithm 2, lines 4-5
                // This is a single step of the usual Montgomery reduction process.
                let k = t0.wrapping_mul(INV);
                let (_, carry) = mac(t0, k, MODULUS[0], 0);
                let (r1, carry) = mac(t1, k, MODULUS[1], carry);
                let (r2, carry) = mac(t2, k, MODULUS[2], carry);
                let (r3, carry) = mac(t3, k, MODULUS[3], carry);
                let (r4, carry) = mac(t4, k, MODULUS[4], carry);
                let (r5, carry) = mac(t5, k, MODULUS[5], carry);
                let (r6, _) = adc(t6, 0, carry);

                (r1, r2, r3, r4, r5, r6)
            });

        // Because we represent F_p elements in non-redundant form, we need a final
        // conditional subtraction to ensure the output is in range.
        (&Fp([u0, u1, u2, u3, u4, u5])).subtract_p()
    }

    #[inline(always)]
    pub(crate) const fn montgomery_reduce(
        t0: u64,
        t1: u64,
        t2: u64,
        t3: u64,
        t4: u64,
        t5: u64,
        t6: u64,
        t7: u64,
        t8: u64,
        t9: u64,
        t10: u64,
        t11: u64,
    ) -> Self {
        // The Montgomery reduction here is based on Algorithm 14.32 in
        // Handbook of Applied Cryptography
        // <http://cacr.uwaterloo.ca/hac/about/chap14.pdf>.

        let k = t0.wrapping_mul(INV);
        let (_, carry) = mac(t0, k, MODULUS[0], 0);
        let (r1, carry) = mac(t1, k, MODULUS[1], carry);
        let (r2, carry) = mac(t2, k, MODULUS[2], carry);
        let (r3, carry) = mac(t3, k, MODULUS[3], carry);
        let (r4, carry) = mac(t4, k, MODULUS[4], carry);
        let (r5, carry) = mac(t5, k, MODULUS[5], carry);
        let (r6, r7) = adc(t6, 0, carry);

        let k = r1.wrapping_mul(INV);
        let (_, carry) = mac(r1, k, MODULUS[0], 0);
        let (r2, carry) = mac(r2, k, MODULUS[1], carry);
        let (r3, carry) = mac(r3, k, MODULUS[2], carry);
        let (r4, carry) = mac(r4, k, MODULUS[3], carry);
        let (r5, carry) = mac(r5, k, MODULUS[4], carry);
        let (r6, carry) = mac(r6, k, MODULUS[5], carry);
        let (r7, r8) = adc(t7, r7, carry);

        let k = r2.wrapping_mul(INV);
        let (_, carry) = mac(r2, k, MODULUS[0], 0);
        let (r3, carry) = mac(r3, k, MODULUS[1], carry);
        let (r4, carry) = mac(r4, k, MODULUS[2], carry);
        let (r5, carry) = mac(r5, k, MODULUS[3], carry);
        let (r6, carry) = mac(r6, k, MODULUS[4], carry);
        let (r7, carry) = mac(r7, k, MODULUS[5], carry);
        let (r8, r9) = adc(t8, r8, carry);

        let k = r3.wrapping_mul(INV);
        let (_, carry) = mac(r3, k, MODULUS[0], 0);
        let (r4, carry) = mac(r4, k, MODULUS[1], carry);
        let (r5, carry) = mac(r5, k, MODULUS[2], carry);
        let (r6, carry) = mac(r6, k, MODULUS[3], carry);
        let (r7, carry) = mac(r7, k, MODULUS[4], carry);
        let (r8, carry) = mac(r8, k, MODULUS[5], carry);
        let (r9, r10) = adc(t9, r9, carry);

        let k = r4.wrapping_mul(INV);
        let (_, carry) = mac(r4, k, MODULUS[0], 0);
        let (r5, carry) = mac(r5, k, MODULUS[1], carry);
        let (r6, carry) = mac(r6, k, MODULUS[2], carry);
        let (r7, carry) = mac(r7, k, MODULUS[3], carry);
        let (r8, carry) = mac(r8, k, MODULUS[4], carry);
        let (r9, carry) = mac(r9, k, MODULUS[5], carry);
        let (r10, r11) = adc(t10, r10, carry);

        let k = r5.wrapping_mul(INV);
        let (_, carry) = mac(r5, k, MODULUS[0], 0);
        let (r6, carry) = mac(r6, k, MODULUS[1], carry);
        let (r7, carry) = mac(r7, k, MODULUS[2], carry);
        let (r8, carry) = mac(r8, k, MODULUS[3], carry);
        let (r9, carry) = mac(r9, k, MODULUS[4], carry);
        let (r10, carry) = mac(r10, k, MODULUS[5], carry);
        let (r11, _) = adc(t11, r11, carry);

        // Attempt to subtract the modulus, to ensure the value
        // is smaller than the modulus.
        (&Fp([r6, r7, r8, r9, r10, r11])).subtract_p()
    }

    #[inline]
    pub const fn mul(&self, rhs: &Fp) -> Fp {
        let (t0, carry) = mac(0, self.0[0], rhs.0[0], 0);
        let (t1, carry) = mac(0, self.0[0], rhs.0[1], carry);
        let (t2, carry) = mac(0, self.0[0], rhs.0[2], carry);
        let (t3, carry) = mac(0, self.0[0], rhs.0[3], carry);
        let (t4, carry) = mac(0, self.0[0], rhs.0[4], carry);
        let (t5, t6) = mac(0, self.0[0], rhs.0[5], carry);

        let (t1, carry) = mac(t1, self.0[1], rhs.0[0], 0);
        let (t2, carry) = mac(t2, self.0[1], rhs.0[1], carry);
        let (t3, carry) = mac(t3, self.0[1], rhs.0[2], carry);
        let (t4, carry) = mac(t4, self.0[1], rhs.0[3], carry);
        let (t5, carry) = mac(t5, self.0[1], rhs.0[4], carry);
        let (t6, t7) = mac(t6, self.0[1], rhs.0[5], carry);

        let (t2, carry) = mac(t2, self.0[2], rhs.0[0], 0);
        let (t3, carry) = mac(t3, self.0[2], rhs.0[1], carry);
        let (t4, carry) = mac(t4, self.0[2], rhs.0[2], carry);
        let (t5, carry) = mac(t5, self.0[2], rhs.0[3], carry);
        let (t6, carry) = mac(t6, self.0[2], rhs.0[4], carry);
        let (t7, t8) = mac(t7, self.0[2], rhs.0[5], carry);

        let (t3, carry) = mac(t3, self.0[3], rhs.0[0], 0);
        let (t4, carry) = mac(t4, self.0[3], rhs.0[1], carry);
        let (t5, carry) = mac(t5, self.0[3], rhs.0[2], carry);
        let (t6, carry) = mac(t6, self.0[3], rhs.0[3], carry);
        let (t7, carry) = mac(t7, self.0[3], rhs.0[4], carry);
        let (t8, t9) = mac(t8, self.0[3], rhs.0[5], carry);

        let (t4, carry) = mac(t4, self.0[4], rhs.0[0], 0);
        let (t5, carry) = mac(t5, self.0[4], rhs.0[1], carry);
        let (t6, carry) = mac(t6, self.0[4], rhs.0[2], carry);
        let (t7, carry) = mac(t7, self.0[4], rhs.0[3], carry);
        let (t8, carry) = mac(t8, self.0[4], rhs.0[4], carry);
        let (t9, t10) = mac(t9, self.0[4], rhs.0[5], carry);

        let (t5, carry) = mac(t5, self.0[5], rhs.0[0], 0);
        let (t6, carry) = mac(t6, self.0[5], rhs.0[1], carry);
        let (t7, carry) = mac(t7, self.0[5], rhs.0[2], carry);
        let (t8, carry) = mac(t8, self.0[5], rhs.0[3], carry);
        let (t9, carry) = mac(t9, self.0[5], rhs.0[4], carry);
        let (t10, t11) = mac(t10, self.0[5], rhs.0[5], carry);

        Self::montgomery_reduce(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11)
    }

    /// Squares this element.
    #[inline]
    pub const fn square(&self) -> Self {
        let (t1, carry) = mac(0, self.0[0], self.0[1], 0);
        let (t2, carry) = mac(0, self.0[0], self.0[2], carry);
        let (t3, carry) = mac(0, self.0[0], self.0[3], carry);
        let (t4, carry) = mac(0, self.0[0], self.0[4], carry);
        let (t5, t6) = mac(0, self.0[0], self.0[5], carry);

        let (t3, carry) = mac(t3, self.0[1], self.0[2], 0);
        let (t4, carry) = mac(t4, self.0[1], self.0[3], carry);
        let (t5, carry) = mac(t5, self.0[1], self.0[4], carry);
        let (t6, t7) = mac(t6, self.0[1], self.0[5], carry);

        let (t5, carry) = mac(t5, self.0[2], self.0[3], 0);
        let (t6, carry) = mac(t6, self.0[2], self.0[4], carry);
        let (t7, t8) = mac(t7, self.0[2], self.0[5], carry);

        let (t7, carry) = mac(t7, self.0[3], self.0[4], 0);
        let (t8, t9) = mac(t8, self.0[3], self.0[5], carry);

        let (t9, t10) = mac(t9, self.0[4], self.0[5], 0);

        let t11 = t10 >> 63;
        let t10 = (t10 << 1) | (t9 >> 63);
        let t9 = (t9 << 1) | (t8 >> 63);
        let t8 = (t8 << 1) | (t7 >> 63);
        let t7 = (t7 << 1) | (t6 >> 63);
        let t6 = (t6 << 1) | (t5 >> 63);
        let t5 = (t5 << 1) | (t4 >> 63);
        let t4 = (t4 << 1) | (t3 >> 63);
        let t3 = (t3 << 1) | (t2 >> 63);
        let t2 = (t2 << 1) | (t1 >> 63);
        let t1 = t1 << 1;

        let (t0, carry) = mac(0, self.0[0], self.0[0], 0);
        let (t1, carry) = adc(t1, 0, carry);
        let (t2, carry) = mac(t2, self.0[1], self.0[1], carry);
        let (t3, carry) = adc(t3, 0, carry);
        let (t4, carry) = mac(t4, self.0[2], self.0[2], carry);
        let (t5, carry) = adc(t5, 0, carry);
        let (t6, carry) = mac(t6, self.0[3], self.0[3], carry);
        let (t7, carry) = adc(t7, 0, carry);
        let (t8, carry) = mac(t8, self.0[4], self.0[4], carry);
        let (t9, carry) = adc(t9, 0, carry);
        let (t10, carry) = mac(t10, self.0[5], self.0[5], carry);
        let (t11, _) = adc(t11, 0, carry);

        Self::montgomery_reduce(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11)
    }
}

#[test]
fn test_conditional_selection() {
    let a = Fp([1, 2, 3, 4, 5, 6]);
    let b = Fp([7, 8, 9, 10, 11, 12]);

    assert_eq!(
        ConditionallySelectable::conditional_select(&a, &b, Choice::from(0u8)),
        a
    );
    assert_eq!(
        ConditionallySelectable::conditional_select(&a, &b, Choice::from(1u8)),
        b
    );
}

#[test]
fn test_equality() {
    fn is_equal(a: &Fp, b: &Fp) -> bool {
        let eq = a == b;
        let ct_eq = a.ct_eq(&b);

        assert_eq!(eq, bool::from(ct_eq));

        eq
    }

    assert!(is_equal(&Fp([1, 2, 3, 4, 5, 6]), &Fp([1, 2, 3, 4, 5, 6])));

    assert!(!is_equal(&Fp([7, 2, 3, 4, 5, 6]), &Fp([1, 2, 3, 4, 5, 6])));
    assert!(!is_equal(&Fp([1, 7, 3, 4, 5, 6]), &Fp([1, 2, 3, 4, 5, 6])));
    assert!(!is_equal(&Fp([1, 2, 7, 4, 5, 6]), &Fp([1, 2, 3, 4, 5, 6])));
    assert!(!is_equal(&Fp([1, 2, 3, 7, 5, 6]), &Fp([1, 2, 3, 4, 5, 6])));
    assert!(!is_equal(&Fp([1, 2, 3, 4, 7, 6]), &Fp([1, 2, 3, 4, 5, 6])));
    assert!(!is_equal(&Fp([1, 2, 3, 4, 5, 7]), &Fp([1, 2, 3, 4, 5, 6])));
}

#[test]
fn test_squaring() {
    let a = Fp([
        0xd215_d276_8e83_191b,
        0x5085_d80f_8fb2_8261,
        0xce9a_032d_df39_3a56,
        0x3e9c_4fff_2ca0_c4bb,
        0x6436_b6f7_f4d9_5dfb,
        0x1060_6628_ad4a_4d90,
    ]);
    let b = Fp([
        0x33d9_c42a_3cb3_e235,
        0xdad1_1a09_4c4c_d455,
        0xa2f1_44bd_729a_aeba,
        0xd415_0932_be9f_feac,
        0xe27b_c7c4_7d44_ee50,
        0x14b6_a78d_3ec7_a560,
    ]);

    assert_eq!(a.square(), b);
}

#[test]
fn test_multiplication() {
    let a = Fp([
        0x0397_a383_2017_0cd4,
        0x734c_1b2c_9e76_1d30,
        0x5ed2_55ad_9a48_beb5,
        0x095a_3c6b_22a7_fcfc,
        0x2294_ce75_d4e2_6a27,
        0x1333_8bd8_7001_1ebb,
    ]);
    let b = Fp([
        0xb9c3_c7c5_b119_6af7,
        0x2580_e208_6ce3_35c1,
        0xf49a_ed3d_8a57_ef42,
        0x41f2_81e4_9846_e878,
        0xe076_2346_c384_52ce,
        0x0652_e893_26e5_7dc0,
    ]);
    let c = Fp([
        0xf96e_f3d7_11ab_5355,
        0xe8d4_59ea_00f1_48dd,
        0x53f7_354a_5f00_fa78,
        0x9e34_a4f3_125c_5f83,
        0x3fbe_0c47_ca74_c19e,
        0x01b0_6a8b_bd4a_dfe4,
    ]);

    assert_eq!(a * b, c);
}

#[test]
fn test_addition() {
    let a = Fp([
        0x5360_bb59_7867_8032,
        0x7dd2_75ae_799e_128e,
        0x5c5b_5071_ce4f_4dcf,
        0xcdb2_1f93_078d_bb3e,
        0xc323_65c5_e73f_474a,
        0x115a_2a54_89ba_be5b,
    ]);
    let b = Fp([
        0x9fd2_8773_3d23_dda0,
        0xb16b_f2af_738b_3554,
        0x3e57_a75b_d3cc_6d1d,
        0x900b_c0bd_627f_d6d6,
        0xd319_a080_efb2_45fe,
        0x15fd_caa4_e4bb_2091,
    ]);
    let c = Fp([
        0x3934_42cc_b58b_b327,
        0x1092_685f_3bd5_47e3,
        0x3382_252c_ab6a_c4c9,
        0xf946_94cb_7688_7f55,
        0x4b21_5e90_93a5_e071,
        0x0d56_e30f_34f5_f853,
    ]);

    assert_eq!(a + b, c);
}

#[test]
fn test_subtraction() {
    let a = Fp([
        0x5360_bb59_7867_8032,
        0x7dd2_75ae_799e_128e,
        0x5c5b_5071_ce4f_4dcf,
        0xcdb2_1f93_078d_bb3e,
        0xc323_65c5_e73f_474a,
        0x115a_2a54_89ba_be5b,
    ]);
    let b = Fp([
        0x9fd2_8773_3d23_dda0,
        0xb16b_f2af_738b_3554,
        0x3e57_a75b_d3cc_6d1d,
        0x900b_c0bd_627f_d6d6,
        0xd319_a080_efb2_45fe,
        0x15fd_caa4_e4bb_2091,
    ]);
    let c = Fp([
        0x6d8d_33e6_3b43_4d3d,
        0xeb12_82fd_b766_dd39,
        0x8534_7bb6_f133_d6d5,
        0xa21d_aa5a_9892_f727,
        0x3b25_6cfb_3ad8_ae23,
        0x155d_7199_de7f_8464,
    ]);

    assert_eq!(a - b, c);
}

#[test]
fn test_negation() {
    let a = Fp([
        0x5360_bb59_7867_8032,
        0x7dd2_75ae_799e_128e,
        0x5c5b_5071_ce4f_4dcf,
        0xcdb2_1f93_078d_bb3e,
        0xc323_65c5_e73f_474a,
        0x115a_2a54_89ba_be5b,
    ]);
    let b = Fp([
        0x669e_44a6_8798_2a79,
        0xa0d9_8a50_37b5_ed71,
        0x0ad5_822f_2861_a854,
        0x96c5_2bf1_ebf7_5781,
        0x87f8_41f0_5c0c_658c,
        0x08a6_e795_afc5_283e,
    ]);

    assert_eq!(-a, b);
}

#[test]
fn test_debug() {
    assert_eq!(
        format!(
            "{:?}",
            Fp([
                0x5360_bb59_7867_8032,
                0x7dd2_75ae_799e_128e,
                0x5c5b_5071_ce4f_4dcf,
                0xcdb2_1f93_078d_bb3e,
                0xc323_65c5_e73f_474a,
                0x115a_2a54_89ba_be5b,
            ])
        ),
        "0x104bf052ad3bc99bcb176c24a06a6c3aad4eaf2308fc4d282e106c84a757d061052630515305e59bdddf8111bfdeb704"
    );
}

#[test]
fn test_from_bytes() {
    let mut a = Fp([
        0xdc90_6d9b_e3f9_5dc8,
        0x8755_caf7_4596_91a1,
        0xcff1_a7f4_e958_3ab3,
        0x9b43_821f_849e_2284,
        0xf575_54f3_a297_4f3f,
        0x085d_bea8_4ed4_7f79,
    ]);

    for _ in 0..100 {
        a = a.square();
        let tmp = a.to_bytes();
        let b = Fp::from_bytes(&tmp).unwrap();

        assert_eq!(a, b);
    }

    assert_eq!(
        -Fp::one(),
        Fp::from_bytes(&[
            26, 1, 17, 234, 57, 127, 230, 154, 75, 27, 167, 182, 67, 75, 172, 215, 100, 119, 75,
            132, 243, 133, 18, 191, 103, 48, 210, 160, 246, 176, 246, 36, 30, 171, 255, 254, 177,
            83, 255, 255, 185, 254, 255, 255, 255, 255, 170, 170
        ])
        .unwrap()
    );

    assert!(bool::from(
        Fp::from_bytes(&[
            27, 1, 17, 234, 57, 127, 230, 154, 75, 27, 167, 182, 67, 75, 172, 215, 100, 119, 75,
            132, 243, 133, 18, 191, 103, 48, 210, 160, 246, 176, 246, 36, 30, 171, 255, 254, 177,
            83, 255, 255, 185, 254, 255, 255, 255, 255, 170, 170
        ])
        .is_none()
    ));

    assert!(bool::from(Fp::from_bytes(&[0xff; 48]).is_none()));
}

#[test]
fn test_sqrt() {
    // a = 4
    let a = Fp::from_raw_unchecked([
        0xaa27_0000_000c_fff3,
        0x53cc_0032_fc34_000a,
        0x478f_e97a_6b0a_807f,
        0xb1d3_7ebe_e6ba_24d7,
        0x8ec9_733b_bf78_ab2f,
        0x09d6_4551_3d83_de7e,
    ]);

    assert_eq!(
        // sqrt(4) = -2
        -a.sqrt().unwrap(),
        // 2
        Fp::from_raw_unchecked([
            0x3213_0000_0006_554f,
            0xb93c_0018_d6c4_0005,
            0x5760_5e0d_b0dd_bb51,
            0x8b25_6521_ed1f_9bcb,
            0x6cf2_8d79_0162_2c03,
            0x11eb_ab9d_bb81_e28c,
        ])
    );
}

#[test]
fn test_inversion() {
    let a = Fp([
        0x43b4_3a50_78ac_2076,
        0x1ce0_7630_46f8_962b,
        0x724a_5276_486d_735c,
        0x6f05_c2a6_282d_48fd,
        0x2095_bd5b_b4ca_9331,
        0x03b3_5b38_94b0_f7da,
    ]);
    let b = Fp([
        0x69ec_d704_0952_148f,
        0x985c_cc20_2219_0f55,
        0xe19b_ba36_a9ad_2f41,
        0x19bb_16c9_5219_dbd8,
        0x14dc_acfd_fb47_8693,
        0x115f_f58a_fff9_a8e1,
    ]);

    assert_eq!(a.invert().unwrap(), b);
    assert!(bool::from(Fp::zero().invert().is_none()));
}

#[test]
fn test_lexicographic_largest() {
    assert!(!bool::from(Fp::zero().lexicographically_largest()));
    assert!(!bool::from(Fp::one().lexicographically_largest()));
    assert!(!bool::from(
        Fp::from_raw_unchecked([
            0xa1fa_ffff_fffe_5557,
            0x995b_fff9_76a3_fffe,
            0x03f4_1d24_d174_ceb4,
            0xf654_7998_c199_5dbd,
            0x778a_468f_507a_6034,
            0x0205_5993_1f7f_8103
        ])
        .lexicographically_largest()
    ));
    assert!(bool::from(
        Fp::from_raw_unchecked([
            0x1804_0000_0001_5554,
            0x8550_0005_3ab0_0001,
            0x633c_b57c_253c_276f,
            0x6e22_d1ec_31eb_b502,
            0xd391_6126_f2d1_4ca2,
            0x17fb_b857_1a00_6596,
        ])
        .lexicographically_largest()
    ));
    assert!(bool::from(
        Fp::from_raw_unchecked([
            0x43f5_ffff_fffc_aaae,
            0x32b7_fff2_ed47_fffd,
            0x07e8_3a49_a2e9_9d69,
            0xeca8_f331_8332_bb7a,
            0xef14_8d1e_a0f4_c069,
            0x040a_b326_3eff_0206,
        ])
        .lexicographically_largest()
    ));
}

#[cfg(feature = "zeroize")]
#[test]
fn test_zeroize() {
    use zeroize::Zeroize;

    let mut a = Fp::one();
    a.zeroize();
    assert!(bool::from(a.is_zero()));
}
