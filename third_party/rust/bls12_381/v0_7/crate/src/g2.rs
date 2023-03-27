//! This module provides an implementation of the $\mathbb{G}_2$ group of BLS12-381.

use core::borrow::Borrow;
use core::fmt;
use core::iter::Sum;
use core::ops::{Add, AddAssign, Mul, MulAssign, Neg, Sub, SubAssign};
use group::{
    prime::{PrimeCurve, PrimeCurveAffine, PrimeGroup},
    Curve, Group, GroupEncoding, UncompressedEncoding,
};
use rand_core::RngCore;
use subtle::{Choice, ConditionallySelectable, ConstantTimeEq, CtOption};

#[cfg(feature = "alloc")]
use group::WnafGroup;

use crate::fp::Fp;
use crate::fp2::Fp2;
use crate::Scalar;

/// This is an element of $\mathbb{G}_2$ represented in the affine coordinate space.
/// It is ideal to keep elements in this representation to reduce memory usage and
/// improve performance through the use of mixed curve model arithmetic.
///
/// Values of `G2Affine` are guaranteed to be in the $q$-order subgroup unless an
/// "unchecked" API was misused.
#[cfg_attr(docsrs, doc(cfg(feature = "groups")))]
#[derive(Copy, Clone, Debug)]
pub struct G2Affine {
    pub(crate) x: Fp2,
    pub(crate) y: Fp2,
    infinity: Choice,
}

impl Default for G2Affine {
    fn default() -> G2Affine {
        G2Affine::identity()
    }
}

#[cfg(feature = "zeroize")]
impl zeroize::DefaultIsZeroes for G2Affine {}

impl fmt::Display for G2Affine {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

impl<'a> From<&'a G2Projective> for G2Affine {
    fn from(p: &'a G2Projective) -> G2Affine {
        let zinv = p.z.invert().unwrap_or(Fp2::zero());
        let x = p.x * zinv;
        let y = p.y * zinv;

        let tmp = G2Affine {
            x,
            y,
            infinity: Choice::from(0u8),
        };

        G2Affine::conditional_select(&tmp, &G2Affine::identity(), zinv.is_zero())
    }
}

impl From<G2Projective> for G2Affine {
    fn from(p: G2Projective) -> G2Affine {
        G2Affine::from(&p)
    }
}

impl ConstantTimeEq for G2Affine {
    fn ct_eq(&self, other: &Self) -> Choice {
        // The only cases in which two points are equal are
        // 1. infinity is set on both
        // 2. infinity is not set on both, and their coordinates are equal

        (self.infinity & other.infinity)
            | ((!self.infinity)
                & (!other.infinity)
                & self.x.ct_eq(&other.x)
                & self.y.ct_eq(&other.y))
    }
}

impl ConditionallySelectable for G2Affine {
    fn conditional_select(a: &Self, b: &Self, choice: Choice) -> Self {
        G2Affine {
            x: Fp2::conditional_select(&a.x, &b.x, choice),
            y: Fp2::conditional_select(&a.y, &b.y, choice),
            infinity: Choice::conditional_select(&a.infinity, &b.infinity, choice),
        }
    }
}

impl Eq for G2Affine {}
impl PartialEq for G2Affine {
    #[inline]
    fn eq(&self, other: &Self) -> bool {
        bool::from(self.ct_eq(other))
    }
}

impl<'a> Neg for &'a G2Affine {
    type Output = G2Affine;

    #[inline]
    fn neg(self) -> G2Affine {
        G2Affine {
            x: self.x,
            y: Fp2::conditional_select(&-self.y, &Fp2::one(), self.infinity),
            infinity: self.infinity,
        }
    }
}

impl Neg for G2Affine {
    type Output = G2Affine;

    #[inline]
    fn neg(self) -> G2Affine {
        -&self
    }
}

impl<'a, 'b> Add<&'b G2Projective> for &'a G2Affine {
    type Output = G2Projective;

    #[inline]
    fn add(self, rhs: &'b G2Projective) -> G2Projective {
        rhs.add_mixed(self)
    }
}

impl<'a, 'b> Add<&'b G2Affine> for &'a G2Projective {
    type Output = G2Projective;

    #[inline]
    fn add(self, rhs: &'b G2Affine) -> G2Projective {
        self.add_mixed(rhs)
    }
}

impl<'a, 'b> Sub<&'b G2Projective> for &'a G2Affine {
    type Output = G2Projective;

    #[inline]
    fn sub(self, rhs: &'b G2Projective) -> G2Projective {
        self + (-rhs)
    }
}

impl<'a, 'b> Sub<&'b G2Affine> for &'a G2Projective {
    type Output = G2Projective;

    #[inline]
    fn sub(self, rhs: &'b G2Affine) -> G2Projective {
        self + (-rhs)
    }
}

impl<T> Sum<T> for G2Projective
where
    T: Borrow<G2Projective>,
{
    fn sum<I>(iter: I) -> Self
    where
        I: Iterator<Item = T>,
    {
        iter.fold(Self::identity(), |acc, item| acc + item.borrow())
    }
}

impl_binops_additive!(G2Projective, G2Affine);
impl_binops_additive_specify_output!(G2Affine, G2Projective, G2Projective);

const B: Fp2 = Fp2 {
    c0: Fp::from_raw_unchecked([
        0xaa27_0000_000c_fff3,
        0x53cc_0032_fc34_000a,
        0x478f_e97a_6b0a_807f,
        0xb1d3_7ebe_e6ba_24d7,
        0x8ec9_733b_bf78_ab2f,
        0x09d6_4551_3d83_de7e,
    ]),
    c1: Fp::from_raw_unchecked([
        0xaa27_0000_000c_fff3,
        0x53cc_0032_fc34_000a,
        0x478f_e97a_6b0a_807f,
        0xb1d3_7ebe_e6ba_24d7,
        0x8ec9_733b_bf78_ab2f,
        0x09d6_4551_3d83_de7e,
    ]),
};

const B3: Fp2 = Fp2::add(&Fp2::add(&B, &B), &B);

impl G2Affine {
    /// Returns the identity of the group: the point at infinity.
    pub fn identity() -> G2Affine {
        G2Affine {
            x: Fp2::zero(),
            y: Fp2::one(),
            infinity: Choice::from(1u8),
        }
    }

    /// Returns a fixed generator of the group. See [`notes::design`](notes/design/index.html#fixed-generators)
    /// for how this generator is chosen.
    pub fn generator() -> G2Affine {
        G2Affine {
            x: Fp2 {
                c0: Fp::from_raw_unchecked([
                    0xf5f2_8fa2_0294_0a10,
                    0xb3f5_fb26_87b4_961a,
                    0xa1a8_93b5_3e2a_e580,
                    0x9894_999d_1a3c_aee9,
                    0x6f67_b763_1863_366b,
                    0x0581_9192_4350_bcd7,
                ]),
                c1: Fp::from_raw_unchecked([
                    0xa5a9_c075_9e23_f606,
                    0xaaa0_c59d_bccd_60c3,
                    0x3bb1_7e18_e286_7806,
                    0x1b1a_b6cc_8541_b367,
                    0xc2b6_ed0e_f215_8547,
                    0x1192_2a09_7360_edf3,
                ]),
            },
            y: Fp2 {
                c0: Fp::from_raw_unchecked([
                    0x4c73_0af8_6049_4c4a,
                    0x597c_fa1f_5e36_9c5a,
                    0xe7e6_856c_aa0a_635a,
                    0xbbef_b5e9_6e0d_495f,
                    0x07d3_a975_f0ef_25a2,
                    0x0083_fd8e_7e80_dae5,
                ]),
                c1: Fp::from_raw_unchecked([
                    0xadc0_fc92_df64_b05d,
                    0x18aa_270a_2b14_61dc,
                    0x86ad_ac6a_3be4_eba0,
                    0x7949_5c4e_c93d_a33a,
                    0xe717_5850_a43c_caed,
                    0x0b2b_c2a1_63de_1bf2,
                ]),
            },
            infinity: Choice::from(0u8),
        }
    }

    /// Serializes this element into compressed form. See [`notes::serialization`](crate::notes::serialization)
    /// for details about how group elements are serialized.
    pub fn to_compressed(&self) -> [u8; 96] {
        // Strictly speaking, self.x is zero already when self.infinity is true, but
        // to guard against implementation mistakes we do not assume this.
        let x = Fp2::conditional_select(&self.x, &Fp2::zero(), self.infinity);

        let mut res = [0; 96];

        (&mut res[0..48]).copy_from_slice(&x.c1.to_bytes()[..]);
        (&mut res[48..96]).copy_from_slice(&x.c0.to_bytes()[..]);

        // This point is in compressed form, so we set the most significant bit.
        res[0] |= 1u8 << 7;

        // Is this point at infinity? If so, set the second-most significant bit.
        res[0] |= u8::conditional_select(&0u8, &(1u8 << 6), self.infinity);

        // Is the y-coordinate the lexicographically largest of the two associated with the
        // x-coordinate? If so, set the third-most significant bit so long as this is not
        // the point at infinity.
        res[0] |= u8::conditional_select(
            &0u8,
            &(1u8 << 5),
            (!self.infinity) & self.y.lexicographically_largest(),
        );

        res
    }

    /// Serializes this element into uncompressed form. See [`notes::serialization`](crate::notes::serialization)
    /// for details about how group elements are serialized.
    pub fn to_uncompressed(&self) -> [u8; 192] {
        let mut res = [0; 192];

        let x = Fp2::conditional_select(&self.x, &Fp2::zero(), self.infinity);
        let y = Fp2::conditional_select(&self.y, &Fp2::zero(), self.infinity);

        res[0..48].copy_from_slice(&x.c1.to_bytes()[..]);
        res[48..96].copy_from_slice(&x.c0.to_bytes()[..]);
        res[96..144].copy_from_slice(&y.c1.to_bytes()[..]);
        res[144..192].copy_from_slice(&y.c0.to_bytes()[..]);

        // Is this point at infinity? If so, set the second-most significant bit.
        res[0] |= u8::conditional_select(&0u8, &(1u8 << 6), self.infinity);

        res
    }

    /// Attempts to deserialize an uncompressed element. See [`notes::serialization`](crate::notes::serialization)
    /// for details about how group elements are serialized.
    pub fn from_uncompressed(bytes: &[u8; 192]) -> CtOption<Self> {
        Self::from_uncompressed_unchecked(bytes)
            .and_then(|p| CtOption::new(p, p.is_on_curve() & p.is_torsion_free()))
    }

    /// Attempts to deserialize an uncompressed element, not checking if the
    /// element is on the curve and not checking if it is in the correct subgroup.
    /// **This is dangerous to call unless you trust the bytes you are reading; otherwise,
    /// API invariants may be broken.** Please consider using `from_uncompressed()` instead.
    pub fn from_uncompressed_unchecked(bytes: &[u8; 192]) -> CtOption<Self> {
        // Obtain the three flags from the start of the byte sequence
        let compression_flag_set = Choice::from((bytes[0] >> 7) & 1);
        let infinity_flag_set = Choice::from((bytes[0] >> 6) & 1);
        let sort_flag_set = Choice::from((bytes[0] >> 5) & 1);

        // Attempt to obtain the x-coordinate
        let xc1 = {
            let mut tmp = [0; 48];
            tmp.copy_from_slice(&bytes[0..48]);

            // Mask away the flag bits
            tmp[0] &= 0b0001_1111;

            Fp::from_bytes(&tmp)
        };
        let xc0 = {
            let mut tmp = [0; 48];
            tmp.copy_from_slice(&bytes[48..96]);

            Fp::from_bytes(&tmp)
        };

        // Attempt to obtain the y-coordinate
        let yc1 = {
            let mut tmp = [0; 48];
            tmp.copy_from_slice(&bytes[96..144]);

            Fp::from_bytes(&tmp)
        };
        let yc0 = {
            let mut tmp = [0; 48];
            tmp.copy_from_slice(&bytes[144..192]);

            Fp::from_bytes(&tmp)
        };

        xc1.and_then(|xc1| {
            xc0.and_then(|xc0| {
                yc1.and_then(|yc1| {
                    yc0.and_then(|yc0| {
                        let x = Fp2 {
                            c0: xc0,
                            c1: xc1
                        };
                        let y = Fp2 {
                            c0: yc0,
                            c1: yc1
                        };

                        // Create a point representing this value
                        let p = G2Affine::conditional_select(
                            &G2Affine {
                                x,
                                y,
                                infinity: infinity_flag_set,
                            },
                            &G2Affine::identity(),
                            infinity_flag_set,
                        );

                        CtOption::new(
                            p,
                            // If the infinity flag is set, the x and y coordinates should have been zero.
                            ((!infinity_flag_set) | (infinity_flag_set & x.is_zero() & y.is_zero())) &
                            // The compression flag should not have been set, as this is an uncompressed element
                            (!compression_flag_set) &
                            // The sort flag should not have been set, as this is an uncompressed element
                            (!sort_flag_set),
                        )
                    })
                })
            })
        })
    }

    /// Attempts to deserialize a compressed element. See [`notes::serialization`](crate::notes::serialization)
    /// for details about how group elements are serialized.
    pub fn from_compressed(bytes: &[u8; 96]) -> CtOption<Self> {
        // We already know the point is on the curve because this is established
        // by the y-coordinate recovery procedure in from_compressed_unchecked().

        Self::from_compressed_unchecked(bytes).and_then(|p| CtOption::new(p, p.is_torsion_free()))
    }

    /// Attempts to deserialize an uncompressed element, not checking if the
    /// element is in the correct subgroup.
    /// **This is dangerous to call unless you trust the bytes you are reading; otherwise,
    /// API invariants may be broken.** Please consider using `from_compressed()` instead.
    pub fn from_compressed_unchecked(bytes: &[u8; 96]) -> CtOption<Self> {
        // Obtain the three flags from the start of the byte sequence
        let compression_flag_set = Choice::from((bytes[0] >> 7) & 1);
        let infinity_flag_set = Choice::from((bytes[0] >> 6) & 1);
        let sort_flag_set = Choice::from((bytes[0] >> 5) & 1);

        // Attempt to obtain the x-coordinate
        let xc1 = {
            let mut tmp = [0; 48];
            tmp.copy_from_slice(&bytes[0..48]);

            // Mask away the flag bits
            tmp[0] &= 0b0001_1111;

            Fp::from_bytes(&tmp)
        };
        let xc0 = {
            let mut tmp = [0; 48];
            tmp.copy_from_slice(&bytes[48..96]);

            Fp::from_bytes(&tmp)
        };

        xc1.and_then(|xc1| {
            xc0.and_then(|xc0| {
                let x = Fp2 { c0: xc0, c1: xc1 };

                // If the infinity flag is set, return the value assuming
                // the x-coordinate is zero and the sort bit is not set.
                //
                // Otherwise, return a recovered point (assuming the correct
                // y-coordinate can be found) so long as the infinity flag
                // was not set.
                CtOption::new(
                    G2Affine::identity(),
                    infinity_flag_set & // Infinity flag should be set
                    compression_flag_set & // Compression flag should be set
                    (!sort_flag_set) & // Sort flag should not be set
                    x.is_zero(), // The x-coordinate should be zero
                )
                .or_else(|| {
                    // Recover a y-coordinate given x by y = sqrt(x^3 + 4)
                    ((x.square() * x) + B).sqrt().and_then(|y| {
                        // Switch to the correct y-coordinate if necessary.
                        let y = Fp2::conditional_select(
                            &y,
                            &-y,
                            y.lexicographically_largest() ^ sort_flag_set,
                        );

                        CtOption::new(
                            G2Affine {
                                x,
                                y,
                                infinity: infinity_flag_set,
                            },
                            (!infinity_flag_set) & // Infinity flag should not be set
                            compression_flag_set, // Compression flag should be set
                        )
                    })
                })
            })
        })
    }

    /// Returns true if this element is the identity (the point at infinity).
    #[inline]
    pub fn is_identity(&self) -> Choice {
        self.infinity
    }

    /// Returns true if this point is free of an $h$-torsion component, and so it
    /// exists within the $q$-order subgroup $\mathbb{G}_2$. This should always return true
    /// unless an "unchecked" API was used.
    pub fn is_torsion_free(&self) -> Choice {
        // Algorithm from Section 4 of https://eprint.iacr.org/2021/1130
        // Updated proof of correctness in https://eprint.iacr.org/2022/352
        //
        // Check that psi(P) == [x] P
        let p = G2Projective::from(self);
        p.psi().ct_eq(&p.mul_by_x())
    }

    /// Returns true if this point is on the curve. This should always return
    /// true unless an "unchecked" API was used.
    pub fn is_on_curve(&self) -> Choice {
        // y^2 - x^3 ?= 4(u + 1)
        (self.y.square() - (self.x.square() * self.x)).ct_eq(&B) | self.infinity
    }
}

/// This is an element of $\mathbb{G}_2$ represented in the projective coordinate space.
#[cfg_attr(docsrs, doc(cfg(feature = "groups")))]
#[derive(Copy, Clone, Debug)]
pub struct G2Projective {
    pub(crate) x: Fp2,
    pub(crate) y: Fp2,
    pub(crate) z: Fp2,
}

impl Default for G2Projective {
    fn default() -> G2Projective {
        G2Projective::identity()
    }
}

#[cfg(feature = "zeroize")]
impl zeroize::DefaultIsZeroes for G2Projective {}

impl fmt::Display for G2Projective {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

impl<'a> From<&'a G2Affine> for G2Projective {
    fn from(p: &'a G2Affine) -> G2Projective {
        G2Projective {
            x: p.x,
            y: p.y,
            z: Fp2::conditional_select(&Fp2::one(), &Fp2::zero(), p.infinity),
        }
    }
}

impl From<G2Affine> for G2Projective {
    fn from(p: G2Affine) -> G2Projective {
        G2Projective::from(&p)
    }
}

impl ConstantTimeEq for G2Projective {
    fn ct_eq(&self, other: &Self) -> Choice {
        // Is (xz, yz, z) equal to (x'z', y'z', z') when converted to affine?

        let x1 = self.x * other.z;
        let x2 = other.x * self.z;

        let y1 = self.y * other.z;
        let y2 = other.y * self.z;

        let self_is_zero = self.z.is_zero();
        let other_is_zero = other.z.is_zero();

        (self_is_zero & other_is_zero) // Both point at infinity
            | ((!self_is_zero) & (!other_is_zero) & x1.ct_eq(&x2) & y1.ct_eq(&y2))
        // Neither point at infinity, coordinates are the same
    }
}

impl ConditionallySelectable for G2Projective {
    fn conditional_select(a: &Self, b: &Self, choice: Choice) -> Self {
        G2Projective {
            x: Fp2::conditional_select(&a.x, &b.x, choice),
            y: Fp2::conditional_select(&a.y, &b.y, choice),
            z: Fp2::conditional_select(&a.z, &b.z, choice),
        }
    }
}

impl Eq for G2Projective {}
impl PartialEq for G2Projective {
    #[inline]
    fn eq(&self, other: &Self) -> bool {
        bool::from(self.ct_eq(other))
    }
}

impl<'a> Neg for &'a G2Projective {
    type Output = G2Projective;

    #[inline]
    fn neg(self) -> G2Projective {
        G2Projective {
            x: self.x,
            y: -self.y,
            z: self.z,
        }
    }
}

impl Neg for G2Projective {
    type Output = G2Projective;

    #[inline]
    fn neg(self) -> G2Projective {
        -&self
    }
}

impl<'a, 'b> Add<&'b G2Projective> for &'a G2Projective {
    type Output = G2Projective;

    #[inline]
    fn add(self, rhs: &'b G2Projective) -> G2Projective {
        self.add(rhs)
    }
}

impl<'a, 'b> Sub<&'b G2Projective> for &'a G2Projective {
    type Output = G2Projective;

    #[inline]
    fn sub(self, rhs: &'b G2Projective) -> G2Projective {
        self + (-rhs)
    }
}

impl<'a, 'b> Mul<&'b Scalar> for &'a G2Projective {
    type Output = G2Projective;

    fn mul(self, other: &'b Scalar) -> Self::Output {
        self.multiply(&other.to_bytes())
    }
}

impl<'a, 'b> Mul<&'b Scalar> for &'a G2Affine {
    type Output = G2Projective;

    fn mul(self, other: &'b Scalar) -> Self::Output {
        G2Projective::from(self).multiply(&other.to_bytes())
    }
}

impl_binops_additive!(G2Projective, G2Projective);
impl_binops_multiplicative!(G2Projective, Scalar);
impl_binops_multiplicative_mixed!(G2Affine, Scalar, G2Projective);

#[inline(always)]
fn mul_by_3b(x: Fp2) -> Fp2 {
    x * B3
}

impl G2Projective {
    /// Returns the identity of the group: the point at infinity.
    pub fn identity() -> G2Projective {
        G2Projective {
            x: Fp2::zero(),
            y: Fp2::one(),
            z: Fp2::zero(),
        }
    }

    /// Returns a fixed generator of the group. See [`notes::design`](notes/design/index.html#fixed-generators)
    /// for how this generator is chosen.
    pub fn generator() -> G2Projective {
        G2Projective {
            x: Fp2 {
                c0: Fp::from_raw_unchecked([
                    0xf5f2_8fa2_0294_0a10,
                    0xb3f5_fb26_87b4_961a,
                    0xa1a8_93b5_3e2a_e580,
                    0x9894_999d_1a3c_aee9,
                    0x6f67_b763_1863_366b,
                    0x0581_9192_4350_bcd7,
                ]),
                c1: Fp::from_raw_unchecked([
                    0xa5a9_c075_9e23_f606,
                    0xaaa0_c59d_bccd_60c3,
                    0x3bb1_7e18_e286_7806,
                    0x1b1a_b6cc_8541_b367,
                    0xc2b6_ed0e_f215_8547,
                    0x1192_2a09_7360_edf3,
                ]),
            },
            y: Fp2 {
                c0: Fp::from_raw_unchecked([
                    0x4c73_0af8_6049_4c4a,
                    0x597c_fa1f_5e36_9c5a,
                    0xe7e6_856c_aa0a_635a,
                    0xbbef_b5e9_6e0d_495f,
                    0x07d3_a975_f0ef_25a2,
                    0x0083_fd8e_7e80_dae5,
                ]),
                c1: Fp::from_raw_unchecked([
                    0xadc0_fc92_df64_b05d,
                    0x18aa_270a_2b14_61dc,
                    0x86ad_ac6a_3be4_eba0,
                    0x7949_5c4e_c93d_a33a,
                    0xe717_5850_a43c_caed,
                    0x0b2b_c2a1_63de_1bf2,
                ]),
            },
            z: Fp2::one(),
        }
    }

    /// Computes the doubling of this point.
    pub fn double(&self) -> G2Projective {
        // Algorithm 9, https://eprint.iacr.org/2015/1060.pdf

        let t0 = self.y.square();
        let z3 = t0 + t0;
        let z3 = z3 + z3;
        let z3 = z3 + z3;
        let t1 = self.y * self.z;
        let t2 = self.z.square();
        let t2 = mul_by_3b(t2);
        let x3 = t2 * z3;
        let y3 = t0 + t2;
        let z3 = t1 * z3;
        let t1 = t2 + t2;
        let t2 = t1 + t2;
        let t0 = t0 - t2;
        let y3 = t0 * y3;
        let y3 = x3 + y3;
        let t1 = self.x * self.y;
        let x3 = t0 * t1;
        let x3 = x3 + x3;

        let tmp = G2Projective {
            x: x3,
            y: y3,
            z: z3,
        };

        G2Projective::conditional_select(&tmp, &G2Projective::identity(), self.is_identity())
    }

    /// Adds this point to another point.
    pub fn add(&self, rhs: &G2Projective) -> G2Projective {
        // Algorithm 7, https://eprint.iacr.org/2015/1060.pdf

        let t0 = self.x * rhs.x;
        let t1 = self.y * rhs.y;
        let t2 = self.z * rhs.z;
        let t3 = self.x + self.y;
        let t4 = rhs.x + rhs.y;
        let t3 = t3 * t4;
        let t4 = t0 + t1;
        let t3 = t3 - t4;
        let t4 = self.y + self.z;
        let x3 = rhs.y + rhs.z;
        let t4 = t4 * x3;
        let x3 = t1 + t2;
        let t4 = t4 - x3;
        let x3 = self.x + self.z;
        let y3 = rhs.x + rhs.z;
        let x3 = x3 * y3;
        let y3 = t0 + t2;
        let y3 = x3 - y3;
        let x3 = t0 + t0;
        let t0 = x3 + t0;
        let t2 = mul_by_3b(t2);
        let z3 = t1 + t2;
        let t1 = t1 - t2;
        let y3 = mul_by_3b(y3);
        let x3 = t4 * y3;
        let t2 = t3 * t1;
        let x3 = t2 - x3;
        let y3 = y3 * t0;
        let t1 = t1 * z3;
        let y3 = t1 + y3;
        let t0 = t0 * t3;
        let z3 = z3 * t4;
        let z3 = z3 + t0;

        G2Projective {
            x: x3,
            y: y3,
            z: z3,
        }
    }

    /// Adds this point to another point in the affine model.
    pub fn add_mixed(&self, rhs: &G2Affine) -> G2Projective {
        // Algorithm 8, https://eprint.iacr.org/2015/1060.pdf

        let t0 = self.x * rhs.x;
        let t1 = self.y * rhs.y;
        let t3 = rhs.x + rhs.y;
        let t4 = self.x + self.y;
        let t3 = t3 * t4;
        let t4 = t0 + t1;
        let t3 = t3 - t4;
        let t4 = rhs.y * self.z;
        let t4 = t4 + self.y;
        let y3 = rhs.x * self.z;
        let y3 = y3 + self.x;
        let x3 = t0 + t0;
        let t0 = x3 + t0;
        let t2 = mul_by_3b(self.z);
        let z3 = t1 + t2;
        let t1 = t1 - t2;
        let y3 = mul_by_3b(y3);
        let x3 = t4 * y3;
        let t2 = t3 * t1;
        let x3 = t2 - x3;
        let y3 = y3 * t0;
        let t1 = t1 * z3;
        let y3 = t1 + y3;
        let t0 = t0 * t3;
        let z3 = z3 * t4;
        let z3 = z3 + t0;

        let tmp = G2Projective {
            x: x3,
            y: y3,
            z: z3,
        };

        G2Projective::conditional_select(&tmp, self, rhs.is_identity())
    }

    fn multiply(&self, by: &[u8]) -> G2Projective {
        let mut acc = G2Projective::identity();

        // This is a simple double-and-add implementation of point
        // multiplication, moving from most significant to least
        // significant bit of the scalar.
        //
        // We skip the leading bit because it's always unset for Fq
        // elements.
        for bit in by
            .iter()
            .rev()
            .flat_map(|byte| (0..8).rev().map(move |i| Choice::from((byte >> i) & 1u8)))
            .skip(1)
        {
            acc = acc.double();
            acc = G2Projective::conditional_select(&acc, &(acc + self), bit);
        }

        acc
    }

    fn psi(&self) -> G2Projective {
        // 1 / ((u+1) ^ ((q-1)/3))
        let psi_coeff_x = Fp2 {
            c0: Fp::zero(),
            c1: Fp::from_raw_unchecked([
                0x890dc9e4867545c3,
                0x2af322533285a5d5,
                0x50880866309b7e2c,
                0xa20d1b8c7e881024,
                0x14e4f04fe2db9068,
                0x14e56d3f1564853a,
            ]),
        };
        // 1 / ((u+1) ^ (p-1)/2)
        let psi_coeff_y = Fp2 {
            c0: Fp::from_raw_unchecked([
                0x3e2f585da55c9ad1,
                0x4294213d86c18183,
                0x382844c88b623732,
                0x92ad2afd19103e18,
                0x1d794e4fac7cf0b9,
                0x0bd592fc7d825ec8,
            ]),
            c1: Fp::from_raw_unchecked([
                0x7bcfa7a25aa30fda,
                0xdc17dec12a927e7c,
                0x2f088dd86b4ebef1,
                0xd1ca2087da74d4a7,
                0x2da2596696cebc1d,
                0x0e2b7eedbbfd87d2,
            ]),
        };

        G2Projective {
            // x = frobenius(x)/((u+1)^((p-1)/3))
            x: self.x.frobenius_map() * psi_coeff_x,
            // y = frobenius(y)/(u+1)^((p-1)/2)
            y: self.y.frobenius_map() * psi_coeff_y,
            // z = frobenius(z)
            z: self.z.frobenius_map(),
        }
    }

    fn psi2(&self) -> G2Projective {
        // 1 / 2 ^ ((q-1)/3)
        let psi2_coeff_x = Fp2 {
            c0: Fp::from_raw_unchecked([
                0xcd03c9e48671f071,
                0x5dab22461fcda5d2,
                0x587042afd3851b95,
                0x8eb60ebe01bacb9e,
                0x03f97d6e83d050d2,
                0x18f0206554638741,
            ]),
            c1: Fp::zero(),
        };

        G2Projective {
            // x = frobenius^2(x)/2^((p-1)/3); note that q^2 is the order of the field.
            x: self.x * psi2_coeff_x,
            // y = -frobenius^2(y); note that q^2 is the order of the field.
            y: self.y.neg(),
            // z = z
            z: self.z,
        }
    }

    /// Multiply `self` by `crate::BLS_X`, using double and add.
    fn mul_by_x(&self) -> G2Projective {
        let mut xself = G2Projective::identity();
        // NOTE: in BLS12-381 we can just skip the first bit.
        let mut x = crate::BLS_X >> 1;
        let mut acc = *self;
        while x != 0 {
            acc = acc.double();
            if x % 2 == 1 {
                xself += acc;
            }
            x >>= 1;
        }
        // finally, flip the sign
        if crate::BLS_X_IS_NEGATIVE {
            xself = -xself;
        }
        xself
    }

    /// Clears the cofactor, using [Budroni-Pintore](https://ia.cr/2017/419).
    /// This is equivalent to multiplying by $h\_\textrm{eff} = 3(z^2 - 1) \cdot
    /// h_2$, where $h_2$ is the cofactor of $\mathbb{G}\_2$ and $z$ is the
    /// parameter of BLS12-381.
    pub fn clear_cofactor(&self) -> G2Projective {
        let t1 = self.mul_by_x(); // [x] P
        let t2 = self.psi(); // psi(P)

        self.double().psi2() // psi^2(2P)
            + (t1 + t2).mul_by_x() // psi^2(2P) + [x^2] P + [x] psi(P)
            - t1 // psi^2(2P) + [x^2 - x] P + [x] psi(P)
            - t2 // psi^2(2P) + [x^2 - x] P + [x - 1] psi(P)
            - self // psi^2(2P) + [x^2 - x - 1] P + [x - 1] psi(P)
    }

    /// Converts a batch of `G2Projective` elements into `G2Affine` elements. This
    /// function will panic if `p.len() != q.len()`.
    pub fn batch_normalize(p: &[Self], q: &mut [G2Affine]) {
        assert_eq!(p.len(), q.len());

        let mut acc = Fp2::one();
        for (p, q) in p.iter().zip(q.iter_mut()) {
            // We use the `x` field of `G2Affine` to store the product
            // of previous z-coordinates seen.
            q.x = acc;

            // We will end up skipping all identities in p
            acc = Fp2::conditional_select(&(acc * p.z), &acc, p.is_identity());
        }

        // This is the inverse, as all z-coordinates are nonzero and the ones
        // that are not are skipped.
        acc = acc.invert().unwrap();

        for (p, q) in p.iter().rev().zip(q.iter_mut().rev()) {
            let skip = p.is_identity();

            // Compute tmp = 1/z
            let tmp = q.x * acc;

            // Cancel out z-coordinate in denominator of `acc`
            acc = Fp2::conditional_select(&(acc * p.z), &acc, skip);

            // Set the coordinates to the correct value
            q.x = p.x * tmp;
            q.y = p.y * tmp;
            q.infinity = Choice::from(0u8);

            *q = G2Affine::conditional_select(q, &G2Affine::identity(), skip);
        }
    }

    /// Returns true if this element is the identity (the point at infinity).
    #[inline]
    pub fn is_identity(&self) -> Choice {
        self.z.is_zero()
    }

    /// Returns true if this point is on the curve. This should always return
    /// true unless an "unchecked" API was used.
    pub fn is_on_curve(&self) -> Choice {
        // Y^2 Z = X^3 + b Z^3

        (self.y.square() * self.z).ct_eq(&(self.x.square() * self.x + self.z.square() * self.z * B))
            | self.z.is_zero()
    }
}

#[derive(Clone, Copy)]
pub struct G2Compressed([u8; 96]);

impl fmt::Debug for G2Compressed {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        self.0[..].fmt(f)
    }
}

impl Default for G2Compressed {
    fn default() -> Self {
        G2Compressed([0; 96])
    }
}

#[cfg(feature = "zeroize")]
impl zeroize::DefaultIsZeroes for G2Compressed {}

impl AsRef<[u8]> for G2Compressed {
    fn as_ref(&self) -> &[u8] {
        &self.0
    }
}

impl AsMut<[u8]> for G2Compressed {
    fn as_mut(&mut self) -> &mut [u8] {
        &mut self.0
    }
}

impl ConstantTimeEq for G2Compressed {
    fn ct_eq(&self, other: &Self) -> Choice {
        self.0.ct_eq(&other.0)
    }
}

impl Eq for G2Compressed {}
impl PartialEq for G2Compressed {
    #[inline]
    fn eq(&self, other: &Self) -> bool {
        bool::from(self.ct_eq(other))
    }
}

#[derive(Clone, Copy)]
pub struct G2Uncompressed([u8; 192]);

impl fmt::Debug for G2Uncompressed {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        self.0[..].fmt(f)
    }
}

impl Default for G2Uncompressed {
    fn default() -> Self {
        G2Uncompressed([0; 192])
    }
}

#[cfg(feature = "zeroize")]
impl zeroize::DefaultIsZeroes for G2Uncompressed {}

impl AsRef<[u8]> for G2Uncompressed {
    fn as_ref(&self) -> &[u8] {
        &self.0
    }
}

impl AsMut<[u8]> for G2Uncompressed {
    fn as_mut(&mut self) -> &mut [u8] {
        &mut self.0
    }
}

impl ConstantTimeEq for G2Uncompressed {
    fn ct_eq(&self, other: &Self) -> Choice {
        self.0.ct_eq(&other.0)
    }
}

impl Eq for G2Uncompressed {}
impl PartialEq for G2Uncompressed {
    #[inline]
    fn eq(&self, other: &Self) -> bool {
        bool::from(self.ct_eq(other))
    }
}

impl Group for G2Projective {
    type Scalar = Scalar;

    fn random(mut rng: impl RngCore) -> Self {
        loop {
            let x = Fp2::random(&mut rng);
            let flip_sign = rng.next_u32() % 2 != 0;

            // Obtain the corresponding y-coordinate given x as y = sqrt(x^3 + 4)
            let p = ((x.square() * x) + B).sqrt().map(|y| G2Affine {
                x,
                y: if flip_sign { -y } else { y },
                infinity: 0.into(),
            });

            if p.is_some().into() {
                let p = p.unwrap().to_curve().clear_cofactor();

                if bool::from(!p.is_identity()) {
                    return p;
                }
            }
        }
    }

    fn identity() -> Self {
        Self::identity()
    }

    fn generator() -> Self {
        Self::generator()
    }

    fn is_identity(&self) -> Choice {
        self.is_identity()
    }

    #[must_use]
    fn double(&self) -> Self {
        self.double()
    }
}

#[cfg(feature = "alloc")]
impl WnafGroup for G2Projective {
    fn recommended_wnaf_for_num_scalars(num_scalars: usize) -> usize {
        const RECOMMENDATIONS: [usize; 11] = [1, 3, 8, 20, 47, 126, 260, 826, 1501, 4555, 84071];

        let mut ret = 4;
        for r in &RECOMMENDATIONS {
            if num_scalars > *r {
                ret += 1;
            } else {
                break;
            }
        }

        ret
    }
}

impl PrimeGroup for G2Projective {}

impl Curve for G2Projective {
    type AffineRepr = G2Affine;

    fn batch_normalize(p: &[Self], q: &mut [Self::AffineRepr]) {
        Self::batch_normalize(p, q);
    }

    fn to_affine(&self) -> Self::AffineRepr {
        self.into()
    }
}

impl PrimeCurve for G2Projective {
    type Affine = G2Affine;
}

impl PrimeCurveAffine for G2Affine {
    type Scalar = Scalar;
    type Curve = G2Projective;

    fn identity() -> Self {
        Self::identity()
    }

    fn generator() -> Self {
        Self::generator()
    }

    fn is_identity(&self) -> Choice {
        self.is_identity()
    }

    fn to_curve(&self) -> Self::Curve {
        self.into()
    }
}

impl GroupEncoding for G2Projective {
    type Repr = G2Compressed;

    fn from_bytes(bytes: &Self::Repr) -> CtOption<Self> {
        G2Affine::from_bytes(bytes).map(Self::from)
    }

    fn from_bytes_unchecked(bytes: &Self::Repr) -> CtOption<Self> {
        G2Affine::from_bytes_unchecked(bytes).map(Self::from)
    }

    fn to_bytes(&self) -> Self::Repr {
        G2Affine::from(self).to_bytes()
    }
}

impl GroupEncoding for G2Affine {
    type Repr = G2Compressed;

    fn from_bytes(bytes: &Self::Repr) -> CtOption<Self> {
        Self::from_compressed(&bytes.0)
    }

    fn from_bytes_unchecked(bytes: &Self::Repr) -> CtOption<Self> {
        Self::from_compressed_unchecked(&bytes.0)
    }

    fn to_bytes(&self) -> Self::Repr {
        G2Compressed(self.to_compressed())
    }
}

impl UncompressedEncoding for G2Affine {
    type Uncompressed = G2Uncompressed;

    fn from_uncompressed(bytes: &Self::Uncompressed) -> CtOption<Self> {
        Self::from_uncompressed(&bytes.0)
    }

    fn from_uncompressed_unchecked(bytes: &Self::Uncompressed) -> CtOption<Self> {
        Self::from_uncompressed_unchecked(&bytes.0)
    }

    fn to_uncompressed(&self) -> Self::Uncompressed {
        G2Uncompressed(self.to_uncompressed())
    }
}

#[test]
fn test_is_on_curve() {
    assert!(bool::from(G2Affine::identity().is_on_curve()));
    assert!(bool::from(G2Affine::generator().is_on_curve()));
    assert!(bool::from(G2Projective::identity().is_on_curve()));
    assert!(bool::from(G2Projective::generator().is_on_curve()));

    let z = Fp2 {
        c0: Fp::from_raw_unchecked([
            0xba7a_fa1f_9a6f_e250,
            0xfa0f_5b59_5eaf_e731,
            0x3bdc_4776_94c3_06e7,
            0x2149_be4b_3949_fa24,
            0x64aa_6e06_49b2_078c,
            0x12b1_08ac_3364_3c3e,
        ]),
        c1: Fp::from_raw_unchecked([
            0x1253_25df_3d35_b5a8,
            0xdc46_9ef5_555d_7fe3,
            0x02d7_16d2_4431_06a9,
            0x05a1_db59_a6ff_37d0,
            0x7cf7_784e_5300_bb8f,
            0x16a8_8922_c7a5_e844,
        ]),
    };

    let gen = G2Affine::generator();
    let mut test = G2Projective {
        x: gen.x * z,
        y: gen.y * z,
        z,
    };

    assert!(bool::from(test.is_on_curve()));

    test.x = z;
    assert!(!bool::from(test.is_on_curve()));
}

#[test]
#[allow(clippy::eq_op)]
fn test_affine_point_equality() {
    let a = G2Affine::generator();
    let b = G2Affine::identity();

    assert!(a == a);
    assert!(b == b);
    assert!(a != b);
    assert!(b != a);
}

#[test]
#[allow(clippy::eq_op)]
fn test_projective_point_equality() {
    let a = G2Projective::generator();
    let b = G2Projective::identity();

    assert!(a == a);
    assert!(b == b);
    assert!(a != b);
    assert!(b != a);

    let z = Fp2 {
        c0: Fp::from_raw_unchecked([
            0xba7a_fa1f_9a6f_e250,
            0xfa0f_5b59_5eaf_e731,
            0x3bdc_4776_94c3_06e7,
            0x2149_be4b_3949_fa24,
            0x64aa_6e06_49b2_078c,
            0x12b1_08ac_3364_3c3e,
        ]),
        c1: Fp::from_raw_unchecked([
            0x1253_25df_3d35_b5a8,
            0xdc46_9ef5_555d_7fe3,
            0x02d7_16d2_4431_06a9,
            0x05a1_db59_a6ff_37d0,
            0x7cf7_784e_5300_bb8f,
            0x16a8_8922_c7a5_e844,
        ]),
    };

    let mut c = G2Projective {
        x: a.x * z,
        y: a.y * z,
        z,
    };
    assert!(bool::from(c.is_on_curve()));

    assert!(a == c);
    assert!(b != c);
    assert!(c == a);
    assert!(c != b);

    c.y = -c.y;
    assert!(bool::from(c.is_on_curve()));

    assert!(a != c);
    assert!(b != c);
    assert!(c != a);
    assert!(c != b);

    c.y = -c.y;
    c.x = z;
    assert!(!bool::from(c.is_on_curve()));
    assert!(a != b);
    assert!(a != c);
    assert!(b != c);
}

#[test]
fn test_conditionally_select_affine() {
    let a = G2Affine::generator();
    let b = G2Affine::identity();

    assert_eq!(G2Affine::conditional_select(&a, &b, Choice::from(0u8)), a);
    assert_eq!(G2Affine::conditional_select(&a, &b, Choice::from(1u8)), b);
}

#[test]
fn test_conditionally_select_projective() {
    let a = G2Projective::generator();
    let b = G2Projective::identity();

    assert_eq!(
        G2Projective::conditional_select(&a, &b, Choice::from(0u8)),
        a
    );
    assert_eq!(
        G2Projective::conditional_select(&a, &b, Choice::from(1u8)),
        b
    );
}

#[test]
fn test_projective_to_affine() {
    let a = G2Projective::generator();
    let b = G2Projective::identity();

    assert!(bool::from(G2Affine::from(a).is_on_curve()));
    assert!(!bool::from(G2Affine::from(a).is_identity()));
    assert!(bool::from(G2Affine::from(b).is_on_curve()));
    assert!(bool::from(G2Affine::from(b).is_identity()));

    let z = Fp2 {
        c0: Fp::from_raw_unchecked([
            0xba7a_fa1f_9a6f_e250,
            0xfa0f_5b59_5eaf_e731,
            0x3bdc_4776_94c3_06e7,
            0x2149_be4b_3949_fa24,
            0x64aa_6e06_49b2_078c,
            0x12b1_08ac_3364_3c3e,
        ]),
        c1: Fp::from_raw_unchecked([
            0x1253_25df_3d35_b5a8,
            0xdc46_9ef5_555d_7fe3,
            0x02d7_16d2_4431_06a9,
            0x05a1_db59_a6ff_37d0,
            0x7cf7_784e_5300_bb8f,
            0x16a8_8922_c7a5_e844,
        ]),
    };

    let c = G2Projective {
        x: a.x * z,
        y: a.y * z,
        z,
    };

    assert_eq!(G2Affine::from(c), G2Affine::generator());
}

#[test]
fn test_affine_to_projective() {
    let a = G2Affine::generator();
    let b = G2Affine::identity();

    assert!(bool::from(G2Projective::from(a).is_on_curve()));
    assert!(!bool::from(G2Projective::from(a).is_identity()));
    assert!(bool::from(G2Projective::from(b).is_on_curve()));
    assert!(bool::from(G2Projective::from(b).is_identity()));
}

#[test]
fn test_doubling() {
    {
        let tmp = G2Projective::identity().double();
        assert!(bool::from(tmp.is_identity()));
        assert!(bool::from(tmp.is_on_curve()));
    }
    {
        let tmp = G2Projective::generator().double();
        assert!(!bool::from(tmp.is_identity()));
        assert!(bool::from(tmp.is_on_curve()));

        assert_eq!(
            G2Affine::from(tmp),
            G2Affine {
                x: Fp2 {
                    c0: Fp::from_raw_unchecked([
                        0xe9d9_e2da_9620_f98b,
                        0x54f1_1993_46b9_7f36,
                        0x3db3_b820_376b_ed27,
                        0xcfdb_31c9_b0b6_4f4c,
                        0x41d7_c127_8635_4493,
                        0x0571_0794_c255_c064,
                    ]),
                    c1: Fp::from_raw_unchecked([
                        0xd6c1_d3ca_6ea0_d06e,
                        0xda0c_bd90_5595_489f,
                        0x4f53_52d4_3479_221d,
                        0x8ade_5d73_6f8c_97e0,
                        0x48cc_8433_925e_f70e,
                        0x08d7_ea71_ea91_ef81,
                    ]),
                },
                y: Fp2 {
                    c0: Fp::from_raw_unchecked([
                        0x15ba_26eb_4b0d_186f,
                        0x0d08_6d64_b7e9_e01e,
                        0xc8b8_48dd_652f_4c78,
                        0xeecf_46a6_123b_ae4f,
                        0x255e_8dd8_b6dc_812a,
                        0x1641_42af_21dc_f93f,
                    ]),
                    c1: Fp::from_raw_unchecked([
                        0xf9b4_a1a8_9598_4db4,
                        0xd417_b114_cccf_f748,
                        0x6856_301f_c89f_086e,
                        0x41c7_7787_8931_e3da,
                        0x3556_b155_066a_2105,
                        0x00ac_f7d3_25cb_89cf,
                    ]),
                },
                infinity: Choice::from(0u8)
            }
        );
    }
}

#[test]
fn test_projective_addition() {
    {
        let a = G2Projective::identity();
        let b = G2Projective::identity();
        let c = a + b;
        assert!(bool::from(c.is_identity()));
        assert!(bool::from(c.is_on_curve()));
    }
    {
        let a = G2Projective::identity();
        let mut b = G2Projective::generator();
        {
            let z = Fp2 {
                c0: Fp::from_raw_unchecked([
                    0xba7a_fa1f_9a6f_e250,
                    0xfa0f_5b59_5eaf_e731,
                    0x3bdc_4776_94c3_06e7,
                    0x2149_be4b_3949_fa24,
                    0x64aa_6e06_49b2_078c,
                    0x12b1_08ac_3364_3c3e,
                ]),
                c1: Fp::from_raw_unchecked([
                    0x1253_25df_3d35_b5a8,
                    0xdc46_9ef5_555d_7fe3,
                    0x02d7_16d2_4431_06a9,
                    0x05a1_db59_a6ff_37d0,
                    0x7cf7_784e_5300_bb8f,
                    0x16a8_8922_c7a5_e844,
                ]),
            };

            b = G2Projective {
                x: b.x * z,
                y: b.y * z,
                z,
            };
        }
        let c = a + b;
        assert!(!bool::from(c.is_identity()));
        assert!(bool::from(c.is_on_curve()));
        assert!(c == G2Projective::generator());
    }
    {
        let a = G2Projective::identity();
        let mut b = G2Projective::generator();
        {
            let z = Fp2 {
                c0: Fp::from_raw_unchecked([
                    0xba7a_fa1f_9a6f_e250,
                    0xfa0f_5b59_5eaf_e731,
                    0x3bdc_4776_94c3_06e7,
                    0x2149_be4b_3949_fa24,
                    0x64aa_6e06_49b2_078c,
                    0x12b1_08ac_3364_3c3e,
                ]),
                c1: Fp::from_raw_unchecked([
                    0x1253_25df_3d35_b5a8,
                    0xdc46_9ef5_555d_7fe3,
                    0x02d7_16d2_4431_06a9,
                    0x05a1_db59_a6ff_37d0,
                    0x7cf7_784e_5300_bb8f,
                    0x16a8_8922_c7a5_e844,
                ]),
            };

            b = G2Projective {
                x: b.x * z,
                y: b.y * z,
                z,
            };
        }
        let c = b + a;
        assert!(!bool::from(c.is_identity()));
        assert!(bool::from(c.is_on_curve()));
        assert!(c == G2Projective::generator());
    }
    {
        let a = G2Projective::generator().double().double(); // 4P
        let b = G2Projective::generator().double(); // 2P
        let c = a + b;

        let mut d = G2Projective::generator();
        for _ in 0..5 {
            d += G2Projective::generator();
        }
        assert!(!bool::from(c.is_identity()));
        assert!(bool::from(c.is_on_curve()));
        assert!(!bool::from(d.is_identity()));
        assert!(bool::from(d.is_on_curve()));
        assert_eq!(c, d);
    }

    // Degenerate case
    {
        let beta = Fp2 {
            c0: Fp::from_raw_unchecked([
                0xcd03_c9e4_8671_f071,
                0x5dab_2246_1fcd_a5d2,
                0x5870_42af_d385_1b95,
                0x8eb6_0ebe_01ba_cb9e,
                0x03f9_7d6e_83d0_50d2,
                0x18f0_2065_5463_8741,
            ]),
            c1: Fp::zero(),
        };
        let beta = beta.square();
        let a = G2Projective::generator().double().double();
        let b = G2Projective {
            x: a.x * beta,
            y: -a.y,
            z: a.z,
        };
        assert!(bool::from(a.is_on_curve()));
        assert!(bool::from(b.is_on_curve()));

        let c = a + b;
        assert_eq!(
            G2Affine::from(c),
            G2Affine::from(G2Projective {
                x: Fp2 {
                    c0: Fp::from_raw_unchecked([
                        0x705a_bc79_9ca7_73d3,
                        0xfe13_2292_c1d4_bf08,
                        0xf37e_ce3e_07b2_b466,
                        0x887e_1c43_f447_e301,
                        0x1e09_70d0_33bc_77e8,
                        0x1985_c81e_20a6_93f2,
                    ]),
                    c1: Fp::from_raw_unchecked([
                        0x1d79_b25d_b36a_b924,
                        0x2394_8e4d_5296_39d3,
                        0x471b_a7fb_0d00_6297,
                        0x2c36_d4b4_465d_c4c0,
                        0x82bb_c3cf_ec67_f538,
                        0x051d_2728_b67b_f952,
                    ])
                },
                y: Fp2 {
                    c0: Fp::from_raw_unchecked([
                        0x41b1_bbf6_576c_0abf,
                        0xb6cc_9371_3f7a_0f9a,
                        0x6b65_b43e_48f3_f01f,
                        0xfb7a_4cfc_af81_be4f,
                        0x3e32_dadc_6ec2_2cb6,
                        0x0bb0_fc49_d798_07e3,
                    ]),
                    c1: Fp::from_raw_unchecked([
                        0x7d13_9778_8f5f_2ddf,
                        0xab29_0714_4ff0_d8e8,
                        0x5b75_73e0_cdb9_1f92,
                        0x4cb8_932d_d31d_af28,
                        0x62bb_fac6_db05_2a54,
                        0x11f9_5c16_d14c_3bbe,
                    ])
                },
                z: Fp2::one()
            })
        );
        assert!(!bool::from(c.is_identity()));
        assert!(bool::from(c.is_on_curve()));
    }
}

#[test]
fn test_mixed_addition() {
    {
        let a = G2Affine::identity();
        let b = G2Projective::identity();
        let c = a + b;
        assert!(bool::from(c.is_identity()));
        assert!(bool::from(c.is_on_curve()));
    }
    {
        let a = G2Affine::identity();
        let mut b = G2Projective::generator();
        {
            let z = Fp2 {
                c0: Fp::from_raw_unchecked([
                    0xba7a_fa1f_9a6f_e250,
                    0xfa0f_5b59_5eaf_e731,
                    0x3bdc_4776_94c3_06e7,
                    0x2149_be4b_3949_fa24,
                    0x64aa_6e06_49b2_078c,
                    0x12b1_08ac_3364_3c3e,
                ]),
                c1: Fp::from_raw_unchecked([
                    0x1253_25df_3d35_b5a8,
                    0xdc46_9ef5_555d_7fe3,
                    0x02d7_16d2_4431_06a9,
                    0x05a1_db59_a6ff_37d0,
                    0x7cf7_784e_5300_bb8f,
                    0x16a8_8922_c7a5_e844,
                ]),
            };

            b = G2Projective {
                x: b.x * z,
                y: b.y * z,
                z,
            };
        }
        let c = a + b;
        assert!(!bool::from(c.is_identity()));
        assert!(bool::from(c.is_on_curve()));
        assert!(c == G2Projective::generator());
    }
    {
        let a = G2Affine::identity();
        let mut b = G2Projective::generator();
        {
            let z = Fp2 {
                c0: Fp::from_raw_unchecked([
                    0xba7a_fa1f_9a6f_e250,
                    0xfa0f_5b59_5eaf_e731,
                    0x3bdc_4776_94c3_06e7,
                    0x2149_be4b_3949_fa24,
                    0x64aa_6e06_49b2_078c,
                    0x12b1_08ac_3364_3c3e,
                ]),
                c1: Fp::from_raw_unchecked([
                    0x1253_25df_3d35_b5a8,
                    0xdc46_9ef5_555d_7fe3,
                    0x02d7_16d2_4431_06a9,
                    0x05a1_db59_a6ff_37d0,
                    0x7cf7_784e_5300_bb8f,
                    0x16a8_8922_c7a5_e844,
                ]),
            };

            b = G2Projective {
                x: b.x * z,
                y: b.y * z,
                z,
            };
        }
        let c = b + a;
        assert!(!bool::from(c.is_identity()));
        assert!(bool::from(c.is_on_curve()));
        assert!(c == G2Projective::generator());
    }
    {
        let a = G2Projective::generator().double().double(); // 4P
        let b = G2Projective::generator().double(); // 2P
        let c = a + b;

        let mut d = G2Projective::generator();
        for _ in 0..5 {
            d += G2Affine::generator();
        }
        assert!(!bool::from(c.is_identity()));
        assert!(bool::from(c.is_on_curve()));
        assert!(!bool::from(d.is_identity()));
        assert!(bool::from(d.is_on_curve()));
        assert_eq!(c, d);
    }

    // Degenerate case
    {
        let beta = Fp2 {
            c0: Fp::from_raw_unchecked([
                0xcd03_c9e4_8671_f071,
                0x5dab_2246_1fcd_a5d2,
                0x5870_42af_d385_1b95,
                0x8eb6_0ebe_01ba_cb9e,
                0x03f9_7d6e_83d0_50d2,
                0x18f0_2065_5463_8741,
            ]),
            c1: Fp::zero(),
        };
        let beta = beta.square();
        let a = G2Projective::generator().double().double();
        let b = G2Projective {
            x: a.x * beta,
            y: -a.y,
            z: a.z,
        };
        let a = G2Affine::from(a);
        assert!(bool::from(a.is_on_curve()));
        assert!(bool::from(b.is_on_curve()));

        let c = a + b;
        assert_eq!(
            G2Affine::from(c),
            G2Affine::from(G2Projective {
                x: Fp2 {
                    c0: Fp::from_raw_unchecked([
                        0x705a_bc79_9ca7_73d3,
                        0xfe13_2292_c1d4_bf08,
                        0xf37e_ce3e_07b2_b466,
                        0x887e_1c43_f447_e301,
                        0x1e09_70d0_33bc_77e8,
                        0x1985_c81e_20a6_93f2,
                    ]),
                    c1: Fp::from_raw_unchecked([
                        0x1d79_b25d_b36a_b924,
                        0x2394_8e4d_5296_39d3,
                        0x471b_a7fb_0d00_6297,
                        0x2c36_d4b4_465d_c4c0,
                        0x82bb_c3cf_ec67_f538,
                        0x051d_2728_b67b_f952,
                    ])
                },
                y: Fp2 {
                    c0: Fp::from_raw_unchecked([
                        0x41b1_bbf6_576c_0abf,
                        0xb6cc_9371_3f7a_0f9a,
                        0x6b65_b43e_48f3_f01f,
                        0xfb7a_4cfc_af81_be4f,
                        0x3e32_dadc_6ec2_2cb6,
                        0x0bb0_fc49_d798_07e3,
                    ]),
                    c1: Fp::from_raw_unchecked([
                        0x7d13_9778_8f5f_2ddf,
                        0xab29_0714_4ff0_d8e8,
                        0x5b75_73e0_cdb9_1f92,
                        0x4cb8_932d_d31d_af28,
                        0x62bb_fac6_db05_2a54,
                        0x11f9_5c16_d14c_3bbe,
                    ])
                },
                z: Fp2::one()
            })
        );
        assert!(!bool::from(c.is_identity()));
        assert!(bool::from(c.is_on_curve()));
    }
}

#[test]
#[allow(clippy::eq_op)]
fn test_projective_negation_and_subtraction() {
    let a = G2Projective::generator().double();
    assert_eq!(a + (-a), G2Projective::identity());
    assert_eq!(a + (-a), a - a);
}

#[test]
fn test_affine_negation_and_subtraction() {
    let a = G2Affine::generator();
    assert_eq!(G2Projective::from(a) + (-a), G2Projective::identity());
    assert_eq!(G2Projective::from(a) + (-a), G2Projective::from(a) - a);
}

#[test]
fn test_projective_scalar_multiplication() {
    let g = G2Projective::generator();
    let a = Scalar::from_raw([
        0x2b56_8297_a56d_a71c,
        0xd8c3_9ecb_0ef3_75d1,
        0x435c_38da_67bf_bf96,
        0x8088_a050_26b6_59b2,
    ]);
    let b = Scalar::from_raw([
        0x785f_dd9b_26ef_8b85,
        0xc997_f258_3769_5c18,
        0x4c8d_bc39_e7b7_56c1,
        0x70d9_b6cc_6d87_df20,
    ]);
    let c = a * b;

    assert_eq!((g * a) * b, g * c);
}

#[test]
fn test_affine_scalar_multiplication() {
    let g = G2Affine::generator();
    let a = Scalar::from_raw([
        0x2b56_8297_a56d_a71c,
        0xd8c3_9ecb_0ef3_75d1,
        0x435c_38da_67bf_bf96,
        0x8088_a050_26b6_59b2,
    ]);
    let b = Scalar::from_raw([
        0x785f_dd9b_26ef_8b85,
        0xc997_f258_3769_5c18,
        0x4c8d_bc39_e7b7_56c1,
        0x70d9_b6cc_6d87_df20,
    ]);
    let c = a * b;

    assert_eq!(G2Affine::from(g * a) * b, g * c);
}

#[test]
fn test_is_torsion_free() {
    let a = G2Affine {
        x: Fp2 {
            c0: Fp::from_raw_unchecked([
                0x89f5_50c8_13db_6431,
                0xa50b_e8c4_56cd_8a1a,
                0xa45b_3741_14ca_e851,
                0xbb61_90f5_bf7f_ff63,
                0x970c_a02c_3ba8_0bc7,
                0x02b8_5d24_e840_fbac,
            ]),
            c1: Fp::from_raw_unchecked([
                0x6888_bc53_d707_16dc,
                0x3dea_6b41_1768_2d70,
                0xd8f5_f930_500c_a354,
                0x6b5e_cb65_56f5_c155,
                0xc96b_ef04_3477_8ab0,
                0x0508_1505_5150_06ad,
            ]),
        },
        y: Fp2 {
            c0: Fp::from_raw_unchecked([
                0x3cf1_ea0d_434b_0f40,
                0x1a0d_c610_e603_e333,
                0x7f89_9561_60c7_2fa0,
                0x25ee_03de_cf64_31c5,
                0xeee8_e206_ec0f_e137,
                0x0975_92b2_26df_ef28,
            ]),
            c1: Fp::from_raw_unchecked([
                0x71e8_bb5f_2924_7367,
                0xa5fe_049e_2118_31ce,
                0x0ce6_b354_502a_3896,
                0x93b0_1200_0997_314e,
                0x6759_f3b6_aa5b_42ac,
                0x1569_44c4_dfe9_2bbb,
            ]),
        },
        infinity: Choice::from(0u8),
    };
    assert!(!bool::from(a.is_torsion_free()));

    assert!(bool::from(G2Affine::identity().is_torsion_free()));
    assert!(bool::from(G2Affine::generator().is_torsion_free()));
}

#[test]
fn test_mul_by_x() {
    // multiplying by `x` a point in G2 is the same as multiplying by
    // the equivalent scalar.
    let generator = G2Projective::generator();
    let x = if crate::BLS_X_IS_NEGATIVE {
        -Scalar::from(crate::BLS_X)
    } else {
        Scalar::from(crate::BLS_X)
    };
    assert_eq!(generator.mul_by_x(), generator * x);

    let point = G2Projective::generator() * Scalar::from(42);
    assert_eq!(point.mul_by_x(), point * x);
}

#[test]
fn test_psi() {
    let generator = G2Projective::generator();

    let z = Fp2 {
        c0: Fp::from_raw_unchecked([
            0x0ef2ddffab187c0a,
            0x2424522b7d5ecbfc,
            0xc6f341a3398054f4,
            0x5523ddf409502df0,
            0xd55c0b5a88e0dd97,
            0x066428d704923e52,
        ]),
        c1: Fp::from_raw_unchecked([
            0x538bbe0c95b4878d,
            0xad04a50379522881,
            0x6d5c05bf5c12fb64,
            0x4ce4a069a2d34787,
            0x59ea6c8d0dffaeaf,
            0x0d42a083a75bd6f3,
        ]),
    };

    // `point` is a random point in the curve
    let point = G2Projective {
        x: Fp2 {
            c0: Fp::from_raw_unchecked([
                0xee4c8cb7c047eaf2,
                0x44ca22eee036b604,
                0x33b3affb2aefe101,
                0x15d3e45bbafaeb02,
                0x7bfc2154cd7419a4,
                0x0a2d0c2b756e5edc,
            ]),
            c1: Fp::from_raw_unchecked([
                0xfc224361029a8777,
                0x4cbf2baab8740924,
                0xc5008c6ec6592c89,
                0xecc2c57b472a9c2d,
                0x8613eafd9d81ffb1,
                0x10fe54daa2d3d495,
            ]),
        } * z,
        y: Fp2 {
            c0: Fp::from_raw_unchecked([
                0x7de7edc43953b75c,
                0x58be1d2de35e87dc,
                0x5731d30b0e337b40,
                0xbe93b60cfeaae4c9,
                0x8b22c203764bedca,
                0x01616c8d1033b771,
            ]),
            c1: Fp::from_raw_unchecked([
                0xea126fe476b5733b,
                0x85cee68b5dae1652,
                0x98247779f7272b04,
                0xa649c8b468c6e808,
                0xb5b9a62dff0c4e45,
                0x1555b67fc7bbe73d,
            ]),
        },
        z: z.square() * z,
    };
    assert!(bool::from(point.is_on_curve()));

    // psi2(P) = psi(psi(P))
    assert_eq!(generator.psi2(), generator.psi().psi());
    assert_eq!(point.psi2(), point.psi().psi());
    // psi(P) is a morphism
    assert_eq!(generator.double().psi(), generator.psi().double());
    assert_eq!(point.psi() + generator.psi(), (point + generator).psi());
    // psi(P) behaves in the same way on the same projective point
    let mut normalized_point = [G2Affine::identity()];
    G2Projective::batch_normalize(&[point], &mut normalized_point);
    let normalized_point = G2Projective::from(normalized_point[0]);
    assert_eq!(point.psi(), normalized_point.psi());
    assert_eq!(point.psi2(), normalized_point.psi2());
}

#[test]
fn test_clear_cofactor() {
    let z = Fp2 {
        c0: Fp::from_raw_unchecked([
            0x0ef2ddffab187c0a,
            0x2424522b7d5ecbfc,
            0xc6f341a3398054f4,
            0x5523ddf409502df0,
            0xd55c0b5a88e0dd97,
            0x066428d704923e52,
        ]),
        c1: Fp::from_raw_unchecked([
            0x538bbe0c95b4878d,
            0xad04a50379522881,
            0x6d5c05bf5c12fb64,
            0x4ce4a069a2d34787,
            0x59ea6c8d0dffaeaf,
            0x0d42a083a75bd6f3,
        ]),
    };

    // `point` is a random point in the curve
    let point = G2Projective {
        x: Fp2 {
            c0: Fp::from_raw_unchecked([
                0xee4c8cb7c047eaf2,
                0x44ca22eee036b604,
                0x33b3affb2aefe101,
                0x15d3e45bbafaeb02,
                0x7bfc2154cd7419a4,
                0x0a2d0c2b756e5edc,
            ]),
            c1: Fp::from_raw_unchecked([
                0xfc224361029a8777,
                0x4cbf2baab8740924,
                0xc5008c6ec6592c89,
                0xecc2c57b472a9c2d,
                0x8613eafd9d81ffb1,
                0x10fe54daa2d3d495,
            ]),
        } * z,
        y: Fp2 {
            c0: Fp::from_raw_unchecked([
                0x7de7edc43953b75c,
                0x58be1d2de35e87dc,
                0x5731d30b0e337b40,
                0xbe93b60cfeaae4c9,
                0x8b22c203764bedca,
                0x01616c8d1033b771,
            ]),
            c1: Fp::from_raw_unchecked([
                0xea126fe476b5733b,
                0x85cee68b5dae1652,
                0x98247779f7272b04,
                0xa649c8b468c6e808,
                0xb5b9a62dff0c4e45,
                0x1555b67fc7bbe73d,
            ]),
        },
        z: z.square() * z,
    };

    assert!(bool::from(point.is_on_curve()));
    assert!(!bool::from(G2Affine::from(point).is_torsion_free()));
    let cleared_point = point.clear_cofactor();

    assert!(bool::from(cleared_point.is_on_curve()));
    assert!(bool::from(G2Affine::from(cleared_point).is_torsion_free()));

    // the generator (and the identity) are always on the curve,
    // even after clearing the cofactor
    let generator = G2Projective::generator();
    assert!(bool::from(generator.clear_cofactor().is_on_curve()));
    let id = G2Projective::identity();
    assert!(bool::from(id.clear_cofactor().is_on_curve()));

    // test the effect on q-torsion points multiplying by h_eff modulo |Scalar|
    // h_eff % q = 0x2b116900400069009a40200040001ffff
    let h_eff_modq: [u8; 32] = [
        0xff, 0xff, 0x01, 0x00, 0x04, 0x00, 0x02, 0xa4, 0x09, 0x90, 0x06, 0x00, 0x04, 0x90, 0x16,
        0xb1, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00,
    ];
    assert_eq!(generator.clear_cofactor(), generator.multiply(&h_eff_modq));
    assert_eq!(
        cleared_point.clear_cofactor(),
        cleared_point.multiply(&h_eff_modq)
    );
}

#[test]
fn test_batch_normalize() {
    let a = G2Projective::generator().double();
    let b = a.double();
    let c = b.double();

    for a_identity in (0..1).map(|n| n == 1) {
        for b_identity in (0..1).map(|n| n == 1) {
            for c_identity in (0..1).map(|n| n == 1) {
                let mut v = [a, b, c];
                if a_identity {
                    v[0] = G2Projective::identity()
                }
                if b_identity {
                    v[1] = G2Projective::identity()
                }
                if c_identity {
                    v[2] = G2Projective::identity()
                }

                let mut t = [
                    G2Affine::identity(),
                    G2Affine::identity(),
                    G2Affine::identity(),
                ];
                let expected = [
                    G2Affine::from(v[0]),
                    G2Affine::from(v[1]),
                    G2Affine::from(v[2]),
                ];

                G2Projective::batch_normalize(&v[..], &mut t[..]);

                assert_eq!(&t[..], &expected[..]);
            }
        }
    }
}

#[cfg(feature = "zeroize")]
#[test]
fn test_zeroize() {
    use zeroize::Zeroize;

    let mut a = G2Affine::generator();
    a.zeroize();
    assert!(bool::from(a.is_identity()));

    let mut a = G2Projective::generator();
    a.zeroize();
    assert!(bool::from(a.is_identity()));

    let mut a = GroupEncoding::to_bytes(&G2Affine::generator());
    a.zeroize();
    assert_eq!(&a, &G2Compressed::default());

    let mut a = UncompressedEncoding::to_uncompressed(&G2Affine::generator());
    a.zeroize();
    assert_eq!(&a, &G2Uncompressed::default());
}
