//! This crate provides an implementation of the **Jubjub** elliptic curve and its associated
//! field arithmetic. See [`README.md`](https://github.com/zkcrypto/jubjub/blob/master/README.md) for more details about Jubjub.
//!
//! # API
//!
//! * `AffinePoint` / `ExtendedPoint` which are implementations of Jubjub group arithmetic
//! * `AffineNielsPoint` / `ExtendedNielsPoint` which are pre-processed Jubjub points
//! * `Fq`, which is the base field of Jubjub
//! * `Fr`, which is the scalar field of Jubjub
//! * `batch_normalize` for converting many `ExtendedPoint`s into `AffinePoint`s efficiently.
//!
//! # Constant Time
//!
//! All operations are constant time unless explicitly noted; these functions will contain
//! "vartime" in their name and they will be documented as variable time.
//!
//! This crate uses the `subtle` crate to perform constant-time operations.

#![no_std]
// Catch documentation errors caused by code changes.
#![deny(rustdoc::broken_intra_doc_links)]
#![deny(missing_debug_implementations)]
#![deny(missing_docs)]
#![deny(unsafe_code)]
// This lint is described at
// https://rust-lang.github.io/rust-clippy/master/index.html#suspicious_arithmetic_impl
// In our library, some of the arithmetic will necessarily involve various binary
// operators, and so this lint is triggered unnecessarily.
#![allow(clippy::suspicious_arithmetic_impl)]

#[cfg(feature = "alloc")]
extern crate alloc;

#[cfg(test)]
#[macro_use]
extern crate std;

use bitvec::{order::Lsb0, view::AsBits};
use core::borrow::Borrow;
use core::fmt;
use core::iter::Sum;
use core::ops::{Add, AddAssign, Mul, MulAssign, Neg, Sub, SubAssign};
use ff::{BatchInverter, Field};
use group::{
    cofactor::{CofactorCurve, CofactorCurveAffine, CofactorGroup},
    prime::PrimeGroup,
    Curve, Group, GroupEncoding,
};
use rand_core::RngCore;
use subtle::{Choice, ConditionallySelectable, ConstantTimeEq, CtOption};

#[cfg(feature = "alloc")]
use alloc::vec::Vec;

#[cfg(feature = "alloc")]
use group::WnafGroup;

#[macro_use]
mod util;

mod fr;
pub use bls12_381::Scalar as Fq;
pub use fr::Fr;

/// Represents an element of the base field $\mathbb{F}_q$ of the Jubjub elliptic curve
/// construction.
pub type Base = Fq;

/// Represents an element of the scalar field $\mathbb{F}_r$ of the Jubjub elliptic curve
/// construction.
pub type Scalar = Fr;

const FR_MODULUS_BYTES: [u8; 32] = [
    183, 44, 247, 214, 94, 14, 151, 208, 130, 16, 200, 204, 147, 32, 104, 166, 0, 59, 52, 1, 1, 59,
    103, 6, 169, 175, 51, 101, 234, 180, 125, 14,
];

/// This represents a Jubjub point in the affine `(u, v)`
/// coordinates.
#[derive(Clone, Copy, Debug, Eq)]
pub struct AffinePoint {
    u: Fq,
    v: Fq,
}

impl fmt::Display for AffinePoint {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

impl Neg for AffinePoint {
    type Output = AffinePoint;

    /// This computes the negation of a point `P = (u, v)`
    /// as `-P = (-u, v)`.
    #[inline]
    fn neg(self) -> AffinePoint {
        AffinePoint {
            u: -self.u,
            v: self.v,
        }
    }
}

impl ConstantTimeEq for AffinePoint {
    fn ct_eq(&self, other: &Self) -> Choice {
        self.u.ct_eq(&other.u) & self.v.ct_eq(&other.v)
    }
}

impl PartialEq for AffinePoint {
    fn eq(&self, other: &Self) -> bool {
        bool::from(self.ct_eq(other))
    }
}

impl ConditionallySelectable for AffinePoint {
    fn conditional_select(a: &Self, b: &Self, choice: Choice) -> Self {
        AffinePoint {
            u: Fq::conditional_select(&a.u, &b.u, choice),
            v: Fq::conditional_select(&a.v, &b.v, choice),
        }
    }
}

/// This represents an extended point `(U, V, Z, T1, T2)`
/// with `Z` nonzero, corresponding to the affine point
/// `(U/Z, V/Z)`. We always have `T1 * T2 = UV/Z`.
///
/// You can do the following things with a point in this
/// form:
///
/// * Convert it into a point in the affine form.
/// * Add it to an `ExtendedPoint`, `AffineNielsPoint` or `ExtendedNielsPoint`.
/// * Double it using `double()`.
/// * Compare it with another extended point using `PartialEq` or `ct_eq()`.
#[derive(Clone, Copy, Debug, Eq)]
pub struct ExtendedPoint {
    u: Fq,
    v: Fq,
    z: Fq,
    t1: Fq,
    t2: Fq,
}

impl fmt::Display for ExtendedPoint {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

impl ConstantTimeEq for ExtendedPoint {
    fn ct_eq(&self, other: &Self) -> Choice {
        // (u/z, v/z) = (u'/z', v'/z') is implied by
        //      (uz'z = u'z'z) and
        //      (vz'z = v'z'z)
        // as z and z' are always nonzero.

        (self.u * other.z).ct_eq(&(other.u * self.z))
            & (self.v * other.z).ct_eq(&(other.v * self.z))
    }
}

impl ConditionallySelectable for ExtendedPoint {
    fn conditional_select(a: &Self, b: &Self, choice: Choice) -> Self {
        ExtendedPoint {
            u: Fq::conditional_select(&a.u, &b.u, choice),
            v: Fq::conditional_select(&a.v, &b.v, choice),
            z: Fq::conditional_select(&a.z, &b.z, choice),
            t1: Fq::conditional_select(&a.t1, &b.t1, choice),
            t2: Fq::conditional_select(&a.t2, &b.t2, choice),
        }
    }
}

impl PartialEq for ExtendedPoint {
    fn eq(&self, other: &Self) -> bool {
        bool::from(self.ct_eq(other))
    }
}

impl<T> Sum<T> for ExtendedPoint
where
    T: Borrow<ExtendedPoint>,
{
    fn sum<I>(iter: I) -> Self
    where
        I: Iterator<Item = T>,
    {
        iter.fold(Self::identity(), |acc, item| acc + item.borrow())
    }
}

impl Neg for ExtendedPoint {
    type Output = ExtendedPoint;

    /// Computes the negation of a point `P = (U, V, Z, T)`
    /// as `-P = (-U, V, Z, -T1, T2)`. The choice of `T1`
    /// is made without loss of generality.
    #[inline]
    fn neg(self) -> ExtendedPoint {
        ExtendedPoint {
            u: -self.u,
            v: self.v,
            z: self.z,
            t1: -self.t1,
            t2: self.t2,
        }
    }
}

impl From<AffinePoint> for ExtendedPoint {
    /// Constructs an extended point (with `Z = 1`) from
    /// an affine point using the map `(u, v) => (u, v, 1, u, v)`.
    fn from(affine: AffinePoint) -> ExtendedPoint {
        ExtendedPoint {
            u: affine.u,
            v: affine.v,
            z: Fq::one(),
            t1: affine.u,
            t2: affine.v,
        }
    }
}

impl<'a> From<&'a ExtendedPoint> for AffinePoint {
    /// Constructs an affine point from an extended point
    /// using the map `(U, V, Z, T1, T2) => (U/Z, V/Z)`
    /// as Z is always nonzero. **This requires a field inversion
    /// and so it is recommended to perform these in a batch
    /// using [`batch_normalize`](crate::batch_normalize) instead.**
    fn from(extended: &'a ExtendedPoint) -> AffinePoint {
        // Z coordinate is always nonzero, so this is
        // its inverse.
        let zinv = extended.z.invert().unwrap();

        AffinePoint {
            u: extended.u * zinv,
            v: extended.v * zinv,
        }
    }
}

impl From<ExtendedPoint> for AffinePoint {
    fn from(extended: ExtendedPoint) -> AffinePoint {
        AffinePoint::from(&extended)
    }
}

/// This is a pre-processed version of an affine point `(u, v)`
/// in the form `(v + u, v - u, u * v * 2d)`. This can be added to an
/// [`ExtendedPoint`](crate::ExtendedPoint).
#[derive(Clone, Copy, Debug)]
pub struct AffineNielsPoint {
    v_plus_u: Fq,
    v_minus_u: Fq,
    t2d: Fq,
}

impl AffineNielsPoint {
    /// Constructs this point from the neutral element `(0, 1)`.
    pub const fn identity() -> Self {
        AffineNielsPoint {
            v_plus_u: Fq::one(),
            v_minus_u: Fq::one(),
            t2d: Fq::zero(),
        }
    }

    #[inline]
    fn multiply(&self, by: &[u8; 32]) -> ExtendedPoint {
        let zero = AffineNielsPoint::identity();

        let mut acc = ExtendedPoint::identity();

        // This is a simple double-and-add implementation of point
        // multiplication, moving from most significant to least
        // significant bit of the scalar.
        //
        // We skip the leading four bits because they're always
        // unset for Fr.
        for bit in by
            .as_bits::<Lsb0>()
            .iter()
            .rev()
            .skip(4)
            .map(|bit| Choice::from(if *bit { 1 } else { 0 }))
        {
            acc = acc.double();
            acc += AffineNielsPoint::conditional_select(&zero, self, bit);
        }

        acc
    }

    /// Multiplies this point by the specific little-endian bit pattern in the
    /// given byte array, ignoring the highest four bits.
    pub fn multiply_bits(&self, by: &[u8; 32]) -> ExtendedPoint {
        self.multiply(by)
    }
}

impl<'a, 'b> Mul<&'b Fr> for &'a AffineNielsPoint {
    type Output = ExtendedPoint;

    fn mul(self, other: &'b Fr) -> ExtendedPoint {
        self.multiply(&other.to_bytes())
    }
}

impl_binops_multiplicative_mixed!(AffineNielsPoint, Fr, ExtendedPoint);

impl ConditionallySelectable for AffineNielsPoint {
    fn conditional_select(a: &Self, b: &Self, choice: Choice) -> Self {
        AffineNielsPoint {
            v_plus_u: Fq::conditional_select(&a.v_plus_u, &b.v_plus_u, choice),
            v_minus_u: Fq::conditional_select(&a.v_minus_u, &b.v_minus_u, choice),
            t2d: Fq::conditional_select(&a.t2d, &b.t2d, choice),
        }
    }
}

/// This is a pre-processed version of an extended point `(U, V, Z, T1, T2)`
/// in the form `(V + U, V - U, Z, T1 * T2 * 2d)`.
#[derive(Clone, Copy, Debug)]
pub struct ExtendedNielsPoint {
    v_plus_u: Fq,
    v_minus_u: Fq,
    z: Fq,
    t2d: Fq,
}

impl ConditionallySelectable for ExtendedNielsPoint {
    fn conditional_select(a: &Self, b: &Self, choice: Choice) -> Self {
        ExtendedNielsPoint {
            v_plus_u: Fq::conditional_select(&a.v_plus_u, &b.v_plus_u, choice),
            v_minus_u: Fq::conditional_select(&a.v_minus_u, &b.v_minus_u, choice),
            z: Fq::conditional_select(&a.z, &b.z, choice),
            t2d: Fq::conditional_select(&a.t2d, &b.t2d, choice),
        }
    }
}

impl ExtendedNielsPoint {
    /// Constructs this point from the neutral element `(0, 1)`.
    pub const fn identity() -> Self {
        ExtendedNielsPoint {
            v_plus_u: Fq::one(),
            v_minus_u: Fq::one(),
            z: Fq::one(),
            t2d: Fq::zero(),
        }
    }

    #[inline]
    fn multiply(&self, by: &[u8; 32]) -> ExtendedPoint {
        let zero = ExtendedNielsPoint::identity();

        let mut acc = ExtendedPoint::identity();

        // This is a simple double-and-add implementation of point
        // multiplication, moving from most significant to least
        // significant bit of the scalar.
        //
        // We skip the leading four bits because they're always
        // unset for Fr.
        for bit in by
            .iter()
            .rev()
            .flat_map(|byte| (0..8).rev().map(move |i| Choice::from((byte >> i) & 1u8)))
            .skip(4)
        {
            acc = acc.double();
            acc += ExtendedNielsPoint::conditional_select(&zero, self, bit);
        }

        acc
    }

    /// Multiplies this point by the specific little-endian bit pattern in the
    /// given byte array, ignoring the highest four bits.
    pub fn multiply_bits(&self, by: &[u8; 32]) -> ExtendedPoint {
        self.multiply(by)
    }
}

impl<'a, 'b> Mul<&'b Fr> for &'a ExtendedNielsPoint {
    type Output = ExtendedPoint;

    fn mul(self, other: &'b Fr) -> ExtendedPoint {
        self.multiply(&other.to_bytes())
    }
}

impl_binops_multiplicative_mixed!(ExtendedNielsPoint, Fr, ExtendedPoint);

// `d = -(10240/10241)`
const EDWARDS_D: Fq = Fq::from_raw([
    0x0106_5fd6_d634_3eb1,
    0x292d_7f6d_3757_9d26,
    0xf5fd_9207_e6bd_7fd4,
    0x2a93_18e7_4bfa_2b48,
]);

// `2*d`
const EDWARDS_D2: Fq = Fq::from_raw([
    0x020c_bfad_ac68_7d62,
    0x525a_feda_6eaf_3a4c,
    0xebfb_240f_cd7a_ffa8,
    0x5526_31ce_97f4_5691,
]);

impl AffinePoint {
    /// Constructs the neutral element `(0, 1)`.
    pub const fn identity() -> Self {
        AffinePoint {
            u: Fq::zero(),
            v: Fq::one(),
        }
    }

    /// Determines if this point is the identity.
    pub fn is_identity(&self) -> Choice {
        ExtendedPoint::from(*self).is_identity()
    }

    /// Multiplies this point by the cofactor, producing an
    /// `ExtendedPoint`
    pub fn mul_by_cofactor(&self) -> ExtendedPoint {
        ExtendedPoint::from(*self).mul_by_cofactor()
    }

    /// Determines if this point is of small order.
    pub fn is_small_order(&self) -> Choice {
        ExtendedPoint::from(*self).is_small_order()
    }

    /// Determines if this point is torsion free and so is
    /// in the prime order subgroup.
    pub fn is_torsion_free(&self) -> Choice {
        ExtendedPoint::from(*self).is_torsion_free()
    }

    /// Determines if this point is prime order, or in other words that
    /// the smallest scalar multiplied by this point that produces the
    /// identity is `r`. This is equivalent to checking that the point
    /// is both torsion free and not the identity.
    pub fn is_prime_order(&self) -> Choice {
        let extended = ExtendedPoint::from(*self);
        extended.is_torsion_free() & (!extended.is_identity())
    }

    /// Converts this element into its byte representation.
    pub fn to_bytes(&self) -> [u8; 32] {
        let mut tmp = self.v.to_bytes();
        let u = self.u.to_bytes();

        // Encode the sign of the u-coordinate in the most
        // significant bit.
        tmp[31] |= u[0] << 7;

        tmp
    }

    /// Attempts to interpret a byte representation of an
    /// affine point, failing if the element is not on
    /// the curve or non-canonical.
    pub fn from_bytes(b: [u8; 32]) -> CtOption<Self> {
        Self::from_bytes_inner(b, 1.into())
    }

    /// Attempts to interpret a byte representation of an affine point, failing if the
    /// element is not on the curve.
    ///
    /// Most non-canonical encodings will also cause a failure. However, this API
    /// preserves (for use in consensus-critical protocols) a bug in the parsing code that
    /// caused two non-canonical encodings to be **silently** accepted:
    ///
    /// - (0, 1), which is the identity;
    /// - (0, -1), which is a point of order two.
    ///
    /// Each of these has a single non-canonical encoding in which the value of the sign
    /// bit is 1.
    ///
    /// See [ZIP 216](https://zips.z.cash/zip-0216) for a more detailed description of the
    /// bug, as well as its fix.
    pub fn from_bytes_pre_zip216_compatibility(b: [u8; 32]) -> CtOption<Self> {
        Self::from_bytes_inner(b, 0.into())
    }

    fn from_bytes_inner(mut b: [u8; 32], zip_216_enabled: Choice) -> CtOption<Self> {
        // Grab the sign bit from the representation
        let sign = b[31] >> 7;

        // Mask away the sign bit
        b[31] &= 0b0111_1111;

        // Interpret what remains as the v-coordinate
        Fq::from_bytes(&b).and_then(|v| {
            // -u^2 + v^2 = 1 + d.u^2.v^2
            // -u^2 = 1 + d.u^2.v^2 - v^2    (rearrange)
            // -u^2 - d.u^2.v^2 = 1 - v^2    (rearrange)
            // u^2 + d.u^2.v^2 = v^2 - 1     (flip signs)
            // u^2 (1 + d.v^2) = v^2 - 1     (factor)
            // u^2 = (v^2 - 1) / (1 + d.v^2) (isolate u^2)
            // We know that (1 + d.v^2) is nonzero for all v:
            //   (1 + d.v^2) = 0
            //   d.v^2 = -1
            //   v^2 = -(1 / d)   No solutions, as -(1 / d) is not a square

            let v2 = v.square();

            ((v2 - Fq::one()) * ((Fq::one() + EDWARDS_D * v2).invert().unwrap_or(Fq::zero())))
                .sqrt()
                .and_then(|u| {
                    // Fix the sign of `u` if necessary
                    let flip_sign = Choice::from((u.to_bytes()[0] ^ sign) & 1);
                    let u_negated = -u;
                    let final_u = Fq::conditional_select(&u, &u_negated, flip_sign);

                    // If u == 0, flip_sign == sign_bit. We therefore want to reject the
                    // encoding as non-canonical if all of the following occur:
                    // - ZIP 216 is enabled
                    // - u == 0
                    // - flip_sign == true
                    let u_is_zero = u.ct_eq(&Fq::zero());
                    CtOption::new(
                        AffinePoint { u: final_u, v },
                        !(zip_216_enabled & u_is_zero & flip_sign),
                    )
                })
        })
    }

    /// Attempts to interpret a batch of byte representations of affine points.
    ///
    /// Returns None for each element if it is not on the curve, or is non-canonical
    /// according to ZIP 216.
    #[cfg(feature = "alloc")]
    pub fn batch_from_bytes(items: impl Iterator<Item = [u8; 32]>) -> Vec<CtOption<Self>> {
        use ff::BatchInvert;

        #[derive(Clone, Copy, Default)]
        struct Item {
            sign: u8,
            v: Fq,
            numerator: Fq,
            denominator: Fq,
        }

        impl ConditionallySelectable for Item {
            fn conditional_select(a: &Self, b: &Self, choice: Choice) -> Self {
                Item {
                    sign: u8::conditional_select(&a.sign, &b.sign, choice),
                    v: Fq::conditional_select(&a.v, &b.v, choice),
                    numerator: Fq::conditional_select(&a.numerator, &b.numerator, choice),
                    denominator: Fq::conditional_select(&a.denominator, &b.denominator, choice),
                }
            }
        }

        let items: Vec<_> = items
            .map(|mut b| {
                // Grab the sign bit from the representation
                let sign = b[31] >> 7;

                // Mask away the sign bit
                b[31] &= 0b0111_1111;

                // Interpret what remains as the v-coordinate
                Fq::from_bytes(&b).map(|v| {
                    // -u^2 + v^2 = 1 + d.u^2.v^2
                    // -u^2 = 1 + d.u^2.v^2 - v^2    (rearrange)
                    // -u^2 - d.u^2.v^2 = 1 - v^2    (rearrange)
                    // u^2 + d.u^2.v^2 = v^2 - 1     (flip signs)
                    // u^2 (1 + d.v^2) = v^2 - 1     (factor)
                    // u^2 = (v^2 - 1) / (1 + d.v^2) (isolate u^2)
                    // We know that (1 + d.v^2) is nonzero for all v:
                    //   (1 + d.v^2) = 0
                    //   d.v^2 = -1
                    //   v^2 = -(1 / d)   No solutions, as -(1 / d) is not a square

                    let v2 = v.square();

                    Item {
                        v,
                        sign,
                        numerator: (v2 - Fq::one()),
                        denominator: Fq::one() + EDWARDS_D * v2,
                    }
                })
            })
            .collect();

        let mut denominators: Vec<_> = items
            .iter()
            .map(|item| item.map(|item| item.denominator).unwrap_or(Fq::zero()))
            .collect();
        denominators.iter_mut().batch_invert();

        items
            .into_iter()
            .zip(denominators.into_iter())
            .map(|(item, inv_denominator)| {
                item.and_then(
                    |Item {
                         v, sign, numerator, ..
                     }| {
                        (numerator * inv_denominator).sqrt().and_then(|u| {
                            // Fix the sign of `u` if necessary
                            let flip_sign = Choice::from((u.to_bytes()[0] ^ sign) & 1);
                            let u_negated = -u;
                            let final_u = Fq::conditional_select(&u, &u_negated, flip_sign);

                            // If u == 0, flip_sign == sign_bit. We therefore want to reject the
                            // encoding as non-canonical if all of the following occur:
                            // - u == 0
                            // - flip_sign == true
                            let u_is_zero = u.ct_eq(&Fq::zero());
                            CtOption::new(AffinePoint { u: final_u, v }, !(u_is_zero & flip_sign))
                        })
                    },
                )
            })
            .collect()
    }

    /// Returns the `u`-coordinate of this point.
    pub fn get_u(&self) -> Fq {
        self.u
    }

    /// Returns the `v`-coordinate of this point.
    pub fn get_v(&self) -> Fq {
        self.v
    }

    /// Returns an `ExtendedPoint` for use in arithmetic operations.
    pub const fn to_extended(&self) -> ExtendedPoint {
        ExtendedPoint {
            u: self.u,
            v: self.v,
            z: Fq::one(),
            t1: self.u,
            t2: self.v,
        }
    }

    /// Performs a pre-processing step that produces an `AffineNielsPoint`
    /// for use in multiple additions.
    pub const fn to_niels(&self) -> AffineNielsPoint {
        AffineNielsPoint {
            v_plus_u: Fq::add(&self.v, &self.u),
            v_minus_u: Fq::sub(&self.v, &self.u),
            t2d: Fq::mul(&Fq::mul(&self.u, &self.v), &EDWARDS_D2),
        }
    }

    /// Constructs an AffinePoint given `u` and `v` without checking
    /// that the point is on the curve.
    pub const fn from_raw_unchecked(u: Fq, v: Fq) -> AffinePoint {
        AffinePoint { u, v }
    }

    /// This is only for debugging purposes and not
    /// exposed in the public API. Checks that this
    /// point is on the curve.
    #[cfg(test)]
    fn is_on_curve_vartime(&self) -> bool {
        let u2 = self.u.square();
        let v2 = self.v.square();

        v2 - u2 == Fq::one() + EDWARDS_D * u2 * v2
    }
}

impl ExtendedPoint {
    /// Constructs an extended point from the neutral element `(0, 1)`.
    pub const fn identity() -> Self {
        ExtendedPoint {
            u: Fq::zero(),
            v: Fq::one(),
            z: Fq::one(),
            t1: Fq::zero(),
            t2: Fq::zero(),
        }
    }

    /// Determines if this point is the identity.
    pub fn is_identity(&self) -> Choice {
        // If this point is the identity, then
        //     u = 0 * z = 0
        // and v = 1 * z = z
        self.u.ct_eq(&Fq::zero()) & self.v.ct_eq(&self.z)
    }

    /// Determines if this point is of small order.
    pub fn is_small_order(&self) -> Choice {
        // We only need to perform two doublings, since the 2-torsion
        // points are (0, 1) and (0, -1), and so we only need to check
        // that the u-coordinate of the result is zero to see if the
        // point is small order.
        self.double().double().u.ct_eq(&Fq::zero())
    }

    /// Determines if this point is torsion free and so is contained
    /// in the prime order subgroup.
    pub fn is_torsion_free(&self) -> Choice {
        self.multiply(&FR_MODULUS_BYTES).is_identity()
    }

    /// Determines if this point is prime order, or in other words that
    /// the smallest scalar multiplied by this point that produces the
    /// identity is `r`. This is equivalent to checking that the point
    /// is both torsion free and not the identity.
    pub fn is_prime_order(&self) -> Choice {
        self.is_torsion_free() & (!self.is_identity())
    }

    /// Multiplies this element by the cofactor `8`.
    pub fn mul_by_cofactor(&self) -> ExtendedPoint {
        self.double().double().double()
    }

    /// Performs a pre-processing step that produces an `ExtendedNielsPoint`
    /// for use in multiple additions.
    pub fn to_niels(&self) -> ExtendedNielsPoint {
        ExtendedNielsPoint {
            v_plus_u: self.v + self.u,
            v_minus_u: self.v - self.u,
            z: self.z,
            t2d: self.t1 * self.t2 * EDWARDS_D2,
        }
    }

    /// Computes the doubling of a point more efficiently than a point can
    /// be added to itself.
    pub fn double(&self) -> ExtendedPoint {
        // Doubling is more efficient (three multiplications, four squarings)
        // when we work within the projective coordinate space (U:Z, V:Z). We
        // rely on the most efficient formula, "dbl-2008-bbjlp", as described
        // in Section 6 of "Twisted Edwards Curves" by Bernstein et al.
        //
        // See <https://hyperelliptic.org/EFD/g1p/auto-twisted-projective.html#doubling-dbl-2008-bbjlp>
        // for more information.
        //
        // We differ from the literature in that we use (u, v) rather than
        // (x, y) coordinates. We also have the constant `a = -1` implied. Let
        // us rewrite the procedure of doubling (u, v, z) to produce (U, V, Z)
        // as follows:
        //
        // B = (u + v)^2
        // C = u^2
        // D = v^2
        // F = D - C
        // H = 2 * z^2
        // J = F - H
        // U = (B - C - D) * J
        // V = F * (- C - D)
        // Z = F * J
        //
        // If we compute K = D + C, we can rewrite this:
        //
        // B = (u + v)^2
        // C = u^2
        // D = v^2
        // F = D - C
        // K = D + C
        // H = 2 * z^2
        // J = F - H
        // U = (B - K) * J
        // V = F * (-K)
        // Z = F * J
        //
        // In order to avoid the unnecessary negation of K,
        // we will negate J, transforming the result into
        // an equivalent point with a negated z-coordinate.
        //
        // B = (u + v)^2
        // C = u^2
        // D = v^2
        // F = D - C
        // K = D + C
        // H = 2 * z^2
        // J = H - F
        // U = (B - K) * J
        // V = F * K
        // Z = F * J
        //
        // Let us rename some variables to simplify:
        //
        // UV2 = (u + v)^2
        // UU = u^2
        // VV = v^2
        // VVmUU = VV - UU
        // VVpUU = VV + UU
        // ZZ2 = 2 * z^2
        // J = ZZ2 - VVmUU
        // U = (UV2 - VVpUU) * J
        // V = VVmUU * VVpUU
        // Z = VVmUU * J
        //
        // We wish to obtain two factors of T = UV/Z.
        //
        // UV/Z = (UV2 - VVpUU) * (ZZ2 - VVmUU) * VVmUU * VVpUU / VVmUU / (ZZ2 - VVmUU)
        //      = (UV2 - VVpUU) * VVmUU * VVpUU / VVmUU
        //      = (UV2 - VVpUU) * VVpUU
        //
        // and so we have that T1 = (UV2 - VVpUU) and T2 = VVpUU.

        let uu = self.u.square();
        let vv = self.v.square();
        let zz2 = self.z.square().double();
        let uv2 = (self.u + self.v).square();
        let vv_plus_uu = vv + uu;
        let vv_minus_uu = vv - uu;

        // The remaining arithmetic is exactly the process of converting
        // from a completed point to an extended point.
        CompletedPoint {
            u: uv2 - vv_plus_uu,
            v: vv_plus_uu,
            z: vv_minus_uu,
            t: zz2 - vv_minus_uu,
        }
        .into_extended()
    }

    #[inline]
    fn multiply(self, by: &[u8; 32]) -> Self {
        self.to_niels().multiply(by)
    }

    /// Converts a batch of projective elements into affine elements.
    ///
    /// This function will panic if `p.len() != q.len()`.
    ///
    /// This costs 5 multiplications per element, and a field inversion.
    fn batch_normalize(p: &[Self], q: &mut [AffinePoint]) {
        assert_eq!(p.len(), q.len());

        for (p, q) in p.iter().zip(q.iter_mut()) {
            // We use the `u` field of `AffinePoint` to store the z-coordinate being
            // inverted, and the `v` field for scratch space.
            q.u = p.z;
        }

        BatchInverter::invert_with_internal_scratch(q, |q| &mut q.u, |q| &mut q.v);

        for (p, q) in p.iter().zip(q.iter_mut()).rev() {
            let tmp = q.u;

            // Set the coordinates to the correct value
            q.u = p.u * tmp; // Multiply by 1/z
            q.v = p.v * tmp; // Multiply by 1/z
        }
    }

    /// This is only for debugging purposes and not
    /// exposed in the public API. Checks that this
    /// point is on the curve.
    #[cfg(test)]
    fn is_on_curve_vartime(&self) -> bool {
        let affine = AffinePoint::from(*self);

        self.z != Fq::zero()
            && affine.is_on_curve_vartime()
            && (affine.u * affine.v * self.z == self.t1 * self.t2)
    }
}

impl<'a, 'b> Mul<&'b Fr> for &'a ExtendedPoint {
    type Output = ExtendedPoint;

    fn mul(self, other: &'b Fr) -> ExtendedPoint {
        self.multiply(&other.to_bytes())
    }
}

impl_binops_multiplicative!(ExtendedPoint, Fr);

impl<'a, 'b> Add<&'b ExtendedNielsPoint> for &'a ExtendedPoint {
    type Output = ExtendedPoint;

    #[allow(clippy::suspicious_arithmetic_impl)]
    fn add(self, other: &'b ExtendedNielsPoint) -> ExtendedPoint {
        // We perform addition in the extended coordinates. Here we use
        // a formula presented by Hisil, Wong, Carter and Dawson in
        // "Twisted Edward Curves Revisited" which only requires 8M.
        //
        // A = (V1 - U1) * (V2 - U2)
        // B = (V1 + U1) * (V2 + U2)
        // C = 2d * T1 * T2
        // D = 2 * Z1 * Z2
        // E = B - A
        // F = D - C
        // G = D + C
        // H = B + A
        // U3 = E * F
        // Y3 = G * H
        // Z3 = F * G
        // T3 = E * H

        let a = (self.v - self.u) * other.v_minus_u;
        let b = (self.v + self.u) * other.v_plus_u;
        let c = self.t1 * self.t2 * other.t2d;
        let d = (self.z * other.z).double();

        // The remaining arithmetic is exactly the process of converting
        // from a completed point to an extended point.
        CompletedPoint {
            u: b - a,
            v: b + a,
            z: d + c,
            t: d - c,
        }
        .into_extended()
    }
}

impl<'a, 'b> Sub<&'b ExtendedNielsPoint> for &'a ExtendedPoint {
    type Output = ExtendedPoint;

    #[allow(clippy::suspicious_arithmetic_impl)]
    fn sub(self, other: &'b ExtendedNielsPoint) -> ExtendedPoint {
        let a = (self.v - self.u) * other.v_plus_u;
        let b = (self.v + self.u) * other.v_minus_u;
        let c = self.t1 * self.t2 * other.t2d;
        let d = (self.z * other.z).double();

        CompletedPoint {
            u: b - a,
            v: b + a,
            z: d - c,
            t: d + c,
        }
        .into_extended()
    }
}

impl_binops_additive!(ExtendedPoint, ExtendedNielsPoint);

impl<'a, 'b> Add<&'b AffineNielsPoint> for &'a ExtendedPoint {
    type Output = ExtendedPoint;

    #[allow(clippy::suspicious_arithmetic_impl)]
    fn add(self, other: &'b AffineNielsPoint) -> ExtendedPoint {
        // This is identical to the addition formula for `ExtendedNielsPoint`,
        // except we can assume that `other.z` is one, so that we perform
        // 7 multiplications.

        let a = (self.v - self.u) * other.v_minus_u;
        let b = (self.v + self.u) * other.v_plus_u;
        let c = self.t1 * self.t2 * other.t2d;
        let d = self.z.double();

        // The remaining arithmetic is exactly the process of converting
        // from a completed point to an extended point.
        CompletedPoint {
            u: b - a,
            v: b + a,
            z: d + c,
            t: d - c,
        }
        .into_extended()
    }
}

impl<'a, 'b> Sub<&'b AffineNielsPoint> for &'a ExtendedPoint {
    type Output = ExtendedPoint;

    #[allow(clippy::suspicious_arithmetic_impl)]
    fn sub(self, other: &'b AffineNielsPoint) -> ExtendedPoint {
        let a = (self.v - self.u) * other.v_plus_u;
        let b = (self.v + self.u) * other.v_minus_u;
        let c = self.t1 * self.t2 * other.t2d;
        let d = self.z.double();

        CompletedPoint {
            u: b - a,
            v: b + a,
            z: d - c,
            t: d + c,
        }
        .into_extended()
    }
}

impl_binops_additive!(ExtendedPoint, AffineNielsPoint);

impl<'a, 'b> Add<&'b ExtendedPoint> for &'a ExtendedPoint {
    type Output = ExtendedPoint;

    #[inline]
    fn add(self, other: &'b ExtendedPoint) -> ExtendedPoint {
        self + other.to_niels()
    }
}

impl<'a, 'b> Sub<&'b ExtendedPoint> for &'a ExtendedPoint {
    type Output = ExtendedPoint;

    #[inline]
    fn sub(self, other: &'b ExtendedPoint) -> ExtendedPoint {
        self - other.to_niels()
    }
}

impl_binops_additive!(ExtendedPoint, ExtendedPoint);

impl<'a, 'b> Add<&'b AffinePoint> for &'a ExtendedPoint {
    type Output = ExtendedPoint;

    #[inline]
    fn add(self, other: &'b AffinePoint) -> ExtendedPoint {
        self + other.to_niels()
    }
}

impl<'a, 'b> Sub<&'b AffinePoint> for &'a ExtendedPoint {
    type Output = ExtendedPoint;

    #[inline]
    fn sub(self, other: &'b AffinePoint) -> ExtendedPoint {
        self - other.to_niels()
    }
}

impl_binops_additive!(ExtendedPoint, AffinePoint);

/// This is a "completed" point produced during a point doubling or
/// addition routine. These points exist in the `(U:Z, V:T)` model
/// of the curve. This is not exposed in the API because it is
/// an implementation detail.
struct CompletedPoint {
    u: Fq,
    v: Fq,
    z: Fq,
    t: Fq,
}

impl CompletedPoint {
    /// This converts a completed point into an extended point by
    /// homogenizing:
    ///
    /// (u/z, v/t) = (u/z * t/t, v/t * z/z) = (ut/zt, vz/zt)
    ///
    /// The resulting T coordinate is utvz/zt = uv, and so
    /// T1 = u, T2 = v, without loss of generality.
    #[inline]
    fn into_extended(self) -> ExtendedPoint {
        ExtendedPoint {
            u: self.u * self.t,
            v: self.v * self.z,
            z: self.z * self.t,
            t1: self.u,
            t2: self.v,
        }
    }
}

impl Default for AffinePoint {
    /// Returns the identity.
    fn default() -> AffinePoint {
        AffinePoint::identity()
    }
}

impl Default for ExtendedPoint {
    /// Returns the identity.
    fn default() -> ExtendedPoint {
        ExtendedPoint::identity()
    }
}

/// This takes a mutable slice of `ExtendedPoint`s and "normalizes" them using
/// only a single inversion for the entire batch. This normalization results in
/// all of the points having a Z-coordinate of one. Further, an iterator is
/// returned which can be used to obtain `AffinePoint`s for each element in the
/// slice.
///
/// This costs 5 multiplications per element, and a field inversion.
pub fn batch_normalize(v: &mut [ExtendedPoint]) -> impl Iterator<Item = AffinePoint> + '_ {
    // We use the `t1` field of `ExtendedPoint` for scratch space.
    BatchInverter::invert_with_internal_scratch(v, |p| &mut p.z, |p| &mut p.t1);

    for p in v.iter_mut() {
        let mut q = *p;
        let tmp = q.z;

        // Set the coordinates to the correct value
        q.u *= &tmp; // Multiply by 1/z
        q.v *= &tmp; // Multiply by 1/z
        q.z = Fq::one(); // z-coordinate is now one
        q.t1 = q.u;
        q.t2 = q.v;

        *p = q;
    }

    // All extended points are now normalized, but the type
    // doesn't encode this fact. Let us offer affine points
    // to the caller.

    v.iter().map(|p| AffinePoint { u: p.u, v: p.v })
}

impl<'a, 'b> Mul<&'b Fr> for &'a AffinePoint {
    type Output = ExtendedPoint;

    fn mul(self, other: &'b Fr) -> ExtendedPoint {
        self.to_niels().multiply(&other.to_bytes())
    }
}

impl_binops_multiplicative_mixed!(AffinePoint, Fr, ExtendedPoint);

/// This represents a point in the prime-order subgroup of Jubjub, in extended
/// coordinates.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct SubgroupPoint(ExtendedPoint);

impl From<SubgroupPoint> for ExtendedPoint {
    fn from(val: SubgroupPoint) -> ExtendedPoint {
        val.0
    }
}

impl<'a> From<&'a SubgroupPoint> for &'a ExtendedPoint {
    fn from(val: &'a SubgroupPoint) -> &'a ExtendedPoint {
        &val.0
    }
}

impl fmt::Display for SubgroupPoint {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.0)
    }
}

impl ConditionallySelectable for SubgroupPoint {
    fn conditional_select(a: &Self, b: &Self, choice: Choice) -> Self {
        SubgroupPoint(ExtendedPoint::conditional_select(&a.0, &b.0, choice))
    }
}

impl SubgroupPoint {
    /// Constructs an AffinePoint given `u` and `v` without checking that the point is on
    /// the curve or in the prime-order subgroup.
    ///
    /// This should only be used for hard-coding constants (e.g. fixed generators); in all
    /// other cases, use [`SubgroupPoint::from_bytes`] instead.
    ///
    /// [`SubgroupPoint::from_bytes`]: SubgroupPoint#impl-GroupEncoding
    pub const fn from_raw_unchecked(u: Fq, v: Fq) -> Self {
        SubgroupPoint(AffinePoint::from_raw_unchecked(u, v).to_extended())
    }
}

impl<T> Sum<T> for SubgroupPoint
where
    T: Borrow<SubgroupPoint>,
{
    fn sum<I>(iter: I) -> Self
    where
        I: Iterator<Item = T>,
    {
        iter.fold(Self::identity(), |acc, item| acc + item.borrow())
    }
}

impl Neg for SubgroupPoint {
    type Output = SubgroupPoint;

    #[inline]
    fn neg(self) -> SubgroupPoint {
        SubgroupPoint(-self.0)
    }
}

impl Neg for &SubgroupPoint {
    type Output = SubgroupPoint;

    #[inline]
    fn neg(self) -> SubgroupPoint {
        SubgroupPoint(-self.0)
    }
}

impl<'a, 'b> Add<&'b SubgroupPoint> for &'a ExtendedPoint {
    type Output = ExtendedPoint;

    #[inline]
    fn add(self, other: &'b SubgroupPoint) -> ExtendedPoint {
        self + other.0
    }
}

impl<'a, 'b> Sub<&'b SubgroupPoint> for &'a ExtendedPoint {
    type Output = ExtendedPoint;

    #[inline]
    fn sub(self, other: &'b SubgroupPoint) -> ExtendedPoint {
        self - other.0
    }
}

impl_binops_additive!(ExtendedPoint, SubgroupPoint);

impl<'a, 'b> Add<&'b SubgroupPoint> for &'a SubgroupPoint {
    type Output = SubgroupPoint;

    #[inline]
    fn add(self, other: &'b SubgroupPoint) -> SubgroupPoint {
        SubgroupPoint(self.0 + other.0)
    }
}

impl<'a, 'b> Sub<&'b SubgroupPoint> for &'a SubgroupPoint {
    type Output = SubgroupPoint;

    #[inline]
    fn sub(self, other: &'b SubgroupPoint) -> SubgroupPoint {
        SubgroupPoint(self.0 - other.0)
    }
}

impl_binops_additive!(SubgroupPoint, SubgroupPoint);

impl<'a, 'b> Mul<&'b Fr> for &'a SubgroupPoint {
    type Output = SubgroupPoint;

    fn mul(self, other: &'b Fr) -> SubgroupPoint {
        SubgroupPoint(self.0.multiply(&other.to_bytes()))
    }
}

impl_binops_multiplicative!(SubgroupPoint, Fr);

impl Group for ExtendedPoint {
    type Scalar = Fr;

    fn random(mut rng: impl RngCore) -> Self {
        loop {
            let v = Fq::random(&mut rng);
            let flip_sign = rng.next_u32() % 2 != 0;

            // See AffinePoint::from_bytes for details.
            let v2 = v.square();
            let p = ((v2 - Fq::one())
                * ((Fq::one() + EDWARDS_D * v2).invert().unwrap_or(Fq::zero())))
            .sqrt()
            .map(|u| AffinePoint {
                u: if flip_sign { -u } else { u },
                v,
            });

            if p.is_some().into() {
                let p = p.unwrap().to_curve();

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
        AffinePoint::generator().into()
    }

    fn is_identity(&self) -> Choice {
        self.is_identity()
    }

    #[must_use]
    fn double(&self) -> Self {
        self.double()
    }
}

impl Group for SubgroupPoint {
    type Scalar = Fr;

    fn random(mut rng: impl RngCore) -> Self {
        loop {
            let p = ExtendedPoint::random(&mut rng).clear_cofactor();

            if bool::from(!p.is_identity()) {
                return p;
            }
        }
    }

    fn identity() -> Self {
        SubgroupPoint(ExtendedPoint::identity())
    }

    fn generator() -> Self {
        ExtendedPoint::generator().clear_cofactor()
    }

    fn is_identity(&self) -> Choice {
        self.0.is_identity()
    }

    #[must_use]
    fn double(&self) -> Self {
        SubgroupPoint(self.0.double())
    }
}

#[cfg(feature = "alloc")]
impl WnafGroup for ExtendedPoint {
    fn recommended_wnaf_for_num_scalars(num_scalars: usize) -> usize {
        // Copied from bls12_381::g1, should be updated.
        const RECOMMENDATIONS: [usize; 12] =
            [1, 3, 7, 20, 43, 120, 273, 563, 1630, 3128, 7933, 62569];

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

impl PrimeGroup for SubgroupPoint {}

impl CofactorGroup for ExtendedPoint {
    type Subgroup = SubgroupPoint;

    fn clear_cofactor(&self) -> Self::Subgroup {
        SubgroupPoint(self.mul_by_cofactor())
    }

    fn into_subgroup(self) -> CtOption<Self::Subgroup> {
        CtOption::new(SubgroupPoint(self), self.is_torsion_free())
    }

    fn is_torsion_free(&self) -> Choice {
        self.is_torsion_free()
    }
}

impl Curve for ExtendedPoint {
    type AffineRepr = AffinePoint;

    fn batch_normalize(p: &[Self], q: &mut [Self::AffineRepr]) {
        Self::batch_normalize(p, q);
    }

    fn to_affine(&self) -> Self::AffineRepr {
        self.into()
    }
}

impl CofactorCurve for ExtendedPoint {
    type Affine = AffinePoint;
}

impl CofactorCurveAffine for AffinePoint {
    type Scalar = Fr;
    type Curve = ExtendedPoint;

    fn identity() -> Self {
        Self::identity()
    }

    fn generator() -> Self {
        // The point with the lowest positive v-coordinate and positive u-coordinate.
        AffinePoint {
            u: Fq::from_raw([
                0xe4b3_d35d_f1a7_adfe,
                0xcaf5_5d1b_29bf_81af,
                0x8b0f_03dd_d60a_8187,
                0x62ed_cbb8_bf37_87c8,
            ]),
            v: Fq::from_raw([
                0x0000_0000_0000_000b,
                0x0000_0000_0000_0000,
                0x0000_0000_0000_0000,
                0x0000_0000_0000_0000,
            ]),
        }
    }

    fn is_identity(&self) -> Choice {
        self.is_identity()
    }

    fn to_curve(&self) -> Self::Curve {
        (*self).into()
    }
}

impl GroupEncoding for ExtendedPoint {
    type Repr = [u8; 32];

    fn from_bytes(bytes: &Self::Repr) -> CtOption<Self> {
        AffinePoint::from_bytes(*bytes).map(Self::from)
    }

    fn from_bytes_unchecked(bytes: &Self::Repr) -> CtOption<Self> {
        // We can't avoid curve checks when parsing a compressed encoding.
        AffinePoint::from_bytes(*bytes).map(Self::from)
    }

    fn to_bytes(&self) -> Self::Repr {
        AffinePoint::from(self).to_bytes()
    }
}

impl GroupEncoding for SubgroupPoint {
    type Repr = [u8; 32];

    fn from_bytes(bytes: &Self::Repr) -> CtOption<Self> {
        ExtendedPoint::from_bytes(bytes).and_then(|p| p.into_subgroup())
    }

    fn from_bytes_unchecked(bytes: &Self::Repr) -> CtOption<Self> {
        ExtendedPoint::from_bytes_unchecked(bytes).map(SubgroupPoint)
    }

    fn to_bytes(&self) -> Self::Repr {
        self.0.to_bytes()
    }
}

impl GroupEncoding for AffinePoint {
    type Repr = [u8; 32];

    fn from_bytes(bytes: &Self::Repr) -> CtOption<Self> {
        Self::from_bytes(*bytes)
    }

    fn from_bytes_unchecked(bytes: &Self::Repr) -> CtOption<Self> {
        Self::from_bytes(*bytes)
    }

    fn to_bytes(&self) -> Self::Repr {
        self.to_bytes()
    }
}

#[test]
fn test_is_on_curve_var() {
    assert!(AffinePoint::identity().is_on_curve_vartime());
}

#[test]
fn test_d_is_non_quadratic_residue() {
    assert!(bool::from(EDWARDS_D.sqrt().is_none()));
    assert!(bool::from((-EDWARDS_D).sqrt().is_none()));
    assert!(bool::from((-EDWARDS_D).invert().unwrap().sqrt().is_none()));
}

#[test]
fn test_affine_niels_point_identity() {
    assert_eq!(
        AffineNielsPoint::identity().v_plus_u,
        AffinePoint::identity().to_niels().v_plus_u
    );
    assert_eq!(
        AffineNielsPoint::identity().v_minus_u,
        AffinePoint::identity().to_niels().v_minus_u
    );
    assert_eq!(
        AffineNielsPoint::identity().t2d,
        AffinePoint::identity().to_niels().t2d
    );
}

#[test]
fn test_extended_niels_point_identity() {
    assert_eq!(
        ExtendedNielsPoint::identity().v_plus_u,
        ExtendedPoint::identity().to_niels().v_plus_u
    );
    assert_eq!(
        ExtendedNielsPoint::identity().v_minus_u,
        ExtendedPoint::identity().to_niels().v_minus_u
    );
    assert_eq!(
        ExtendedNielsPoint::identity().z,
        ExtendedPoint::identity().to_niels().z
    );
    assert_eq!(
        ExtendedNielsPoint::identity().t2d,
        ExtendedPoint::identity().to_niels().t2d
    );
}

#[test]
fn test_assoc() {
    let p = ExtendedPoint::from(AffinePoint {
        u: Fq::from_raw([
            0x81c5_71e5_d883_cfb0,
            0x049f_7a68_6f14_7029,
            0xf539_c860_bc3e_a21f,
            0x4284_715b_7ccc_8162,
        ]),
        v: Fq::from_raw([
            0xbf09_6275_684b_b8ca,
            0xc7ba_2458_90af_256d,
            0x5911_9f3e_8638_0eb0,
            0x3793_de18_2f9f_b1d2,
        ]),
    })
    .mul_by_cofactor();
    assert!(p.is_on_curve_vartime());

    assert_eq!(
        (p * Fr::from(1000u64)) * Fr::from(3938u64),
        p * (Fr::from(1000u64) * Fr::from(3938u64)),
    );
}

#[test]
fn test_batch_normalize() {
    let mut p = ExtendedPoint::from(AffinePoint {
        u: Fq::from_raw([
            0x81c5_71e5_d883_cfb0,
            0x049f_7a68_6f14_7029,
            0xf539_c860_bc3e_a21f,
            0x4284_715b_7ccc_8162,
        ]),
        v: Fq::from_raw([
            0xbf09_6275_684b_b8ca,
            0xc7ba_2458_90af_256d,
            0x5911_9f3e_8638_0eb0,
            0x3793_de18_2f9f_b1d2,
        ]),
    })
    .mul_by_cofactor();

    let mut v = vec![];
    for _ in 0..10 {
        v.push(p);
        p = p.double();
    }

    for p in &v {
        assert!(p.is_on_curve_vartime());
    }

    let expected: std::vec::Vec<_> = v.iter().map(|p| AffinePoint::from(*p)).collect();
    let mut result0 = vec![AffinePoint::identity(); v.len()];
    ExtendedPoint::batch_normalize(&v, &mut result0);
    for i in 0..10 {
        assert!(expected[i] == result0[i]);
    }
    let result1: std::vec::Vec<_> = batch_normalize(&mut v).collect();
    for i in 0..10 {
        assert!(expected[i] == result1[i]);
        assert!(v[i].is_on_curve_vartime());
        assert!(AffinePoint::from(v[i]) == expected[i]);
    }
    let result2: std::vec::Vec<_> = batch_normalize(&mut v).collect();
    for i in 0..10 {
        assert!(expected[i] == result2[i]);
        assert!(v[i].is_on_curve_vartime());
        assert!(AffinePoint::from(v[i]) == expected[i]);
    }
}

#[cfg(test)]
const FULL_GENERATOR: AffinePoint = AffinePoint::from_raw_unchecked(
    Fq::from_raw([
        0xe4b3_d35d_f1a7_adfe,
        0xcaf5_5d1b_29bf_81af,
        0x8b0f_03dd_d60a_8187,
        0x62ed_cbb8_bf37_87c8,
    ]),
    Fq::from_raw([0xb, 0x0, 0x0, 0x0]),
);

#[cfg(test)]
const EIGHT_TORSION: [AffinePoint; 8] = [
    AffinePoint::from_raw_unchecked(
        Fq::from_raw([
            0xd92e_6a79_2720_0d43,
            0x7aa4_1ac4_3dae_8582,
            0xeaaa_e086_a166_18d1,
            0x71d4_df38_ba9e_7973,
        ]),
        Fq::from_raw([
            0xff0d_2068_eff4_96dd,
            0x9106_ee90_f384_a4a1,
            0x16a1_3035_ad4d_7266,
            0x4958_bdb2_1966_982e,
        ]),
    ),
    AffinePoint::from_raw_unchecked(
        Fq::from_raw([
            0xfffe_ffff_0000_0001,
            0x67ba_a400_89fb_5bfe,
            0xa5e8_0b39_939e_d334,
            0x73ed_a753_299d_7d47,
        ]),
        Fq::from_raw([0x0, 0x0, 0x0, 0x0]),
    ),
    AffinePoint::from_raw_unchecked(
        Fq::from_raw([
            0xd92e_6a79_2720_0d43,
            0x7aa4_1ac4_3dae_8582,
            0xeaaa_e086_a166_18d1,
            0x71d4_df38_ba9e_7973,
        ]),
        Fq::from_raw([
            0x00f2_df96_100b_6924,
            0xc2b6_b572_0c79_b75d,
            0x1c98_a7d2_5c54_659e,
            0x2a94_e9a1_1036_e51a,
        ]),
    ),
    AffinePoint::from_raw_unchecked(
        Fq::from_raw([0x0, 0x0, 0x0, 0x0]),
        Fq::from_raw([
            0xffff_ffff_0000_0000,
            0x53bd_a402_fffe_5bfe,
            0x3339_d808_09a1_d805,
            0x73ed_a753_299d_7d48,
        ]),
    ),
    AffinePoint::from_raw_unchecked(
        Fq::from_raw([
            0x26d1_9585_d8df_f2be,
            0xd919_893e_c24f_d67c,
            0x488e_f781_683b_bf33,
            0x0218_c81a_6eff_03d4,
        ]),
        Fq::from_raw([
            0x00f2_df96_100b_6924,
            0xc2b6_b572_0c79_b75d,
            0x1c98_a7d2_5c54_659e,
            0x2a94_e9a1_1036_e51a,
        ]),
    ),
    AffinePoint::from_raw_unchecked(
        Fq::from_raw([
            0x0001_0000_0000_0000,
            0xec03_0002_7603_0000,
            0x8d51_ccce_7603_04d0,
            0x0,
        ]),
        Fq::from_raw([0x0, 0x0, 0x0, 0x0]),
    ),
    AffinePoint::from_raw_unchecked(
        Fq::from_raw([
            0x26d1_9585_d8df_f2be,
            0xd919_893e_c24f_d67c,
            0x488e_f781_683b_bf33,
            0x0218_c81a_6eff_03d4,
        ]),
        Fq::from_raw([
            0xff0d_2068_eff4_96dd,
            0x9106_ee90_f384_a4a1,
            0x16a1_3035_ad4d_7266,
            0x4958_bdb2_1966_982e,
        ]),
    ),
    AffinePoint::from_raw_unchecked(
        Fq::from_raw([0x0, 0x0, 0x0, 0x0]),
        Fq::from_raw([0x1, 0x0, 0x0, 0x0]),
    ),
];

#[test]
fn find_eight_torsion() {
    let g = ExtendedPoint::from(FULL_GENERATOR);
    assert!(!bool::from(g.is_small_order()));
    let g = g.multiply(&FR_MODULUS_BYTES);
    assert!(bool::from(g.is_small_order()));

    let mut cur = g;

    for (i, point) in EIGHT_TORSION.iter().enumerate() {
        let tmp = AffinePoint::from(cur);
        if &tmp != point {
            panic!("{}th torsion point should be {:?}", i, tmp);
        }

        cur += &g;
    }
}

#[test]
fn find_curve_generator() {
    let mut trial_bytes = [0; 32];
    for _ in 0..255 {
        let a = AffinePoint::from_bytes(trial_bytes);
        if bool::from(a.is_some()) {
            let a = a.unwrap();
            assert!(a.is_on_curve_vartime());
            let b = ExtendedPoint::from(a);
            let b = b.multiply(&FR_MODULUS_BYTES);
            assert!(bool::from(b.is_small_order()));
            let b = b.double();
            assert!(bool::from(b.is_small_order()));
            let b = b.double();
            assert!(bool::from(b.is_small_order()));
            if !bool::from(b.is_identity()) {
                let b = b.double();
                assert!(bool::from(b.is_small_order()));
                assert!(bool::from(b.is_identity()));
                assert_eq!(FULL_GENERATOR, a);
                assert_eq!(AffinePoint::generator(), a);
                assert!(bool::from(a.mul_by_cofactor().is_torsion_free()));
                return;
            }
        }

        trial_bytes[0] += 1;
    }

    panic!("should have found a generator of the curve");
}

#[test]
fn test_small_order() {
    for point in EIGHT_TORSION.iter() {
        assert!(bool::from(point.is_small_order()));
    }
}

#[test]
fn test_is_identity() {
    let a = EIGHT_TORSION[0].mul_by_cofactor();
    let b = EIGHT_TORSION[1].mul_by_cofactor();

    assert_eq!(a.u, b.u);
    assert_eq!(a.v, a.z);
    assert_eq!(b.v, b.z);
    assert!(a.v != b.v);
    assert!(a.z != b.z);

    assert!(bool::from(a.is_identity()));
    assert!(bool::from(b.is_identity()));

    for point in EIGHT_TORSION.iter() {
        assert!(bool::from(point.mul_by_cofactor().is_identity()));
    }
}

#[test]
fn test_mul_consistency() {
    let a = Fr([
        0x21e6_1211_d993_4f2e,
        0xa52c_058a_693c_3e07,
        0x9ccb_77bf_b12d_6360,
        0x07df_2470_ec94_398e,
    ]);
    let b = Fr([
        0x0333_6d1c_be19_dbe0,
        0x0153_618f_6156_a536,
        0x2604_c9e1_fc3c_6b15,
        0x04ae_581c_eb02_8720,
    ]);
    let c = Fr([
        0xd7ab_f5bb_2468_3f4c,
        0x9d77_12cc_274b_7c03,
        0x9732_93db_9683_789f,
        0x0b67_7e29_380a_97a7,
    ]);
    assert_eq!(a * b, c);
    let p = ExtendedPoint::from(AffinePoint {
        u: Fq::from_raw([
            0x81c5_71e5_d883_cfb0,
            0x049f_7a68_6f14_7029,
            0xf539_c860_bc3e_a21f,
            0x4284_715b_7ccc_8162,
        ]),
        v: Fq::from_raw([
            0xbf09_6275_684b_b8ca,
            0xc7ba_2458_90af_256d,
            0x5911_9f3e_8638_0eb0,
            0x3793_de18_2f9f_b1d2,
        ]),
    })
    .mul_by_cofactor();
    assert_eq!(p * c, (p * a) * b);

    // Test Mul implemented on ExtendedNielsPoint
    assert_eq!(p * c, (p.to_niels() * a) * b);
    assert_eq!(p.to_niels() * c, (p * a) * b);
    assert_eq!(p.to_niels() * c, (p.to_niels() * a) * b);

    // Test Mul implemented on AffineNielsPoint
    let p_affine_niels = AffinePoint::from(p).to_niels();
    assert_eq!(p * c, (p_affine_niels * a) * b);
    assert_eq!(p_affine_niels * c, (p * a) * b);
    assert_eq!(p_affine_niels * c, (p_affine_niels * a) * b);
}

#[test]
fn test_serialization_consistency() {
    let gen = FULL_GENERATOR.mul_by_cofactor();
    let mut p = gen;

    let v = vec![
        [
            203, 85, 12, 213, 56, 234, 12, 193, 19, 132, 128, 64, 142, 110, 170, 185, 179, 108, 97,
            63, 13, 211, 247, 120, 79, 219, 110, 234, 131, 123, 19, 215,
        ],
        [
            113, 154, 240, 230, 224, 198, 208, 170, 104, 15, 59, 126, 151, 222, 233, 195, 203, 195,
            167, 129, 89, 121, 240, 142, 51, 166, 64, 250, 184, 202, 154, 177,
        ],
        [
            197, 41, 93, 209, 203, 55, 164, 174, 88, 0, 90, 199, 1, 156, 149, 141, 240, 29, 14, 82,
            86, 225, 126, 129, 186, 157, 148, 162, 219, 51, 156, 199,
        ],
        [
            182, 117, 250, 241, 81, 196, 199, 227, 151, 74, 243, 17, 221, 97, 200, 139, 192, 83,
            231, 35, 214, 14, 95, 69, 130, 201, 4, 116, 177, 19, 179, 0,
        ],
        [
            118, 41, 29, 200, 60, 189, 119, 252, 78, 40, 230, 18, 208, 221, 38, 214, 176, 250, 4,
            10, 77, 101, 26, 216, 193, 198, 226, 84, 25, 177, 230, 185,
        ],
        [
            226, 189, 227, 208, 112, 117, 136, 98, 72, 38, 211, 167, 254, 82, 174, 113, 112, 166,
            138, 171, 166, 113, 52, 251, 129, 197, 138, 45, 195, 7, 61, 140,
        ],
        [
            38, 198, 156, 196, 146, 225, 55, 163, 138, 178, 157, 128, 115, 135, 204, 215, 0, 33,
            171, 20, 60, 32, 142, 209, 33, 233, 125, 146, 207, 12, 16, 24,
        ],
        [
            17, 187, 231, 83, 165, 36, 232, 184, 140, 205, 195, 252, 166, 85, 59, 86, 3, 226, 211,
            67, 179, 29, 238, 181, 102, 142, 58, 63, 57, 89, 174, 138,
        ],
        [
            210, 159, 80, 16, 181, 39, 221, 204, 224, 144, 145, 79, 54, 231, 8, 140, 142, 216, 93,
            190, 183, 116, 174, 63, 33, 242, 177, 118, 148, 40, 241, 203,
        ],
        [
            0, 143, 107, 102, 149, 187, 27, 124, 18, 10, 98, 28, 113, 123, 121, 185, 29, 152, 14,
            130, 149, 28, 87, 35, 135, 135, 153, 54, 112, 53, 54, 68,
        ],
        [
            178, 131, 85, 160, 214, 51, 208, 157, 196, 152, 247, 93, 202, 56, 81, 239, 155, 122,
            59, 188, 237, 253, 11, 169, 208, 236, 12, 4, 163, 211, 88, 97,
        ],
        [
            246, 194, 231, 195, 159, 101, 180, 133, 80, 21, 185, 220, 195, 115, 144, 12, 90, 150,
            44, 117, 8, 156, 168, 248, 206, 41, 60, 82, 67, 75, 57, 67,
        ],
        [
            212, 205, 171, 153, 113, 16, 194, 241, 224, 43, 177, 110, 190, 248, 22, 201, 208, 166,
            2, 83, 134, 130, 85, 129, 166, 136, 185, 191, 163, 38, 54, 10,
        ],
        [
            8, 60, 190, 39, 153, 222, 119, 23, 142, 237, 12, 110, 146, 9, 19, 219, 143, 64, 161,
            99, 199, 77, 39, 148, 70, 213, 246, 227, 150, 178, 237, 178,
        ],
        [
            11, 114, 217, 160, 101, 37, 100, 220, 56, 114, 42, 31, 138, 33, 84, 157, 214, 167, 73,
            233, 115, 81, 124, 134, 15, 31, 181, 60, 184, 130, 175, 159,
        ],
        [
            141, 238, 235, 202, 241, 32, 210, 10, 127, 230, 54, 31, 146, 80, 247, 9, 107, 124, 0,
            26, 203, 16, 237, 34, 214, 147, 133, 15, 29, 236, 37, 88,
        ],
    ];

    let batched = AffinePoint::batch_from_bytes(v.iter().cloned());

    for (expected_serialized, batch_deserialized) in v.into_iter().zip(batched.into_iter()) {
        assert!(p.is_on_curve_vartime());
        let affine = AffinePoint::from(p);
        let serialized = affine.to_bytes();
        let deserialized = AffinePoint::from_bytes(serialized).unwrap();
        assert_eq!(affine, deserialized);
        assert_eq!(affine, batch_deserialized.unwrap());
        assert_eq!(expected_serialized, serialized);
        p += gen;
    }
}

#[test]
fn test_zip_216() {
    const NON_CANONICAL_ENCODINGS: [[u8; 32]; 2] = [
        // (0, 1) with sign bit set to 1.
        [
            0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x80,
        ],
        // (0, -1) with sign bit set to 1.
        [
            0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x5b, 0xfe, 0xff, 0x02, 0xa4,
            0xbd, 0x53, 0x05, 0xd8, 0xa1, 0x09, 0x08, 0xd8, 0x39, 0x33, 0x48, 0x7d, 0x9d, 0x29,
            0x53, 0xa7, 0xed, 0xf3,
        ],
    ];

    for b in &NON_CANONICAL_ENCODINGS {
        {
            let mut encoding = *b;

            // The normal API should reject the non-canonical encoding.
            assert!(bool::from(AffinePoint::from_bytes(encoding).is_none()));

            // If we clear the sign bit of the non-canonical encoding, it should be
            // accepted by the normal API.
            encoding[31] &= 0b0111_1111;
            assert!(bool::from(AffinePoint::from_bytes(encoding).is_some()));
        }

        {
            // The bug-preserving API should accept the non-canonical encoding, and the
            // resulting point should serialize to a different (canonical) encoding.
            let parsed = AffinePoint::from_bytes_pre_zip216_compatibility(*b).unwrap();
            let mut encoded = parsed.to_bytes();
            assert_ne!(b, &encoded);

            // If we set the sign bit of the serialized encoding, it should match the
            // non-canonical encoding.
            encoded[31] |= 0b1000_0000;
            assert_eq!(b, &encoded);
        }
    }
}
