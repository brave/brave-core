// -*- mode: rust; -*-
//
// This file is part of curve25519-dalek.
// Copyright (c) 2016-2021 isis lovecruft
// Copyright (c) 2016-2020 Henry de Valence
// See LICENSE for licensing information.
//
// Authors:
// - isis agora lovecruft <isis@patternsinthevoid.net>
// - Henry de Valence <hdevalence@hdevalence.ca>

//! Group operations for Curve25519, in Edwards form.
//!
//! ## Encoding and Decoding
//!
//! Encoding is done by converting to and from a `CompressedEdwardsY`
//! struct, which is a typed wrapper around `[u8; 32]`.
//!
//! ## Equality Testing
//!
//! The `EdwardsPoint` struct implements the [`subtle::ConstantTimeEq`]
//! trait for constant-time equality checking, and the Rust `Eq` trait
//! for variable-time equality checking.
//!
//! ## Cofactor-related functions
//!
//! The order of the group of points on the curve \\(\mathcal E\\)
//! is \\(|\mathcal E| = 8\ell \\), so its structure is \\( \mathcal
//! E = \mathcal E\[8\] \times \mathcal E[\ell]\\).  The torsion
//! subgroup \\( \mathcal E\[8\] \\) consists of eight points of small
//! order.  Technically, all of \\(\mathcal E\\) is torsion, but we
//! use the word only to refer to the small \\(\mathcal E\[8\]\\) part, not
//! the large prime-order \\(\mathcal E[\ell]\\) part.
//!
//! To test if a point is in \\( \mathcal E\[8\] \\), use
//! [`EdwardsPoint::is_small_order`].
//!
//! To test if a point is in \\( \mathcal E[\ell] \\), use
//! [`EdwardsPoint::is_torsion_free`].
//!
//! To multiply by the cofactor, use [`EdwardsPoint::mul_by_cofactor`].
//!
//! To avoid dealing with cofactors entirely, consider using Ristretto.
//!
//! ## Scalars
//!
//! Scalars are represented by the [`Scalar`] struct. To construct a scalar, see
//! [`Scalar::from_canonical_bytes`] or [`Scalar::from_bytes_mod_order_wide`].
//!
//! ## Scalar Multiplication
//!
//! Scalar multiplication on Edwards points is provided by:
//!
//! * the `*` operator between a `Scalar` and a `EdwardsPoint`, which
//! performs constant-time variable-base scalar multiplication;
//!
//! * the `*` operator between a `Scalar` and a
//! `EdwardsBasepointTable`, which performs constant-time fixed-base
//! scalar multiplication;
//!
//! * an implementation of the
//! [`MultiscalarMul`](../traits/trait.MultiscalarMul.html) trait for
//! constant-time variable-base multiscalar multiplication;
//!
//! * an implementation of the
//! [`VartimeMultiscalarMul`](../traits/trait.VartimeMultiscalarMul.html)
//! trait for variable-time variable-base multiscalar multiplication;
//!
//! ## Implementation
//!
//! The Edwards arithmetic is implemented using the “extended twisted
//! coordinates” of Hisil, Wong, Carter, and Dawson, and the
//! corresponding complete formulas.  For more details,
//! see the [`curve_models` submodule][curve_models]
//! of the internal documentation.
//!
//! ## Validity Checking
//!
//! There is no function for checking whether a point is valid.
//! Instead, the `EdwardsPoint` struct is guaranteed to hold a valid
//! point on the curve.
//!
//! We use the Rust type system to make invalid points
//! unrepresentable: `EdwardsPoint` objects can only be created via
//! successful decompression of a compressed point, or else by
//! operations on other (valid) `EdwardsPoint`s.
//!
//! [curve_models]: https://docs.rs/curve25519-dalek/latest/curve25519-dalek/backend/serial/curve_models/index.html

// We allow non snake_case names because coordinates in projective space are
// traditionally denoted by the capitalisation of their respective
// counterparts in affine space.  Yeah, you heard me, rustc, I'm gonna have my
// affine and projective cakes and eat both of them too.
#![allow(non_snake_case)]

use core::array::TryFromSliceError;
use core::borrow::Borrow;
use core::fmt::Debug;
use core::iter::Iterator;
use core::iter::Sum;
use core::ops::{Add, Neg, Sub};
use core::ops::{AddAssign, SubAssign};
use core::ops::{Mul, MulAssign};

use cfg_if::cfg_if;

#[cfg(feature = "digest")]
use digest::{generic_array::typenum::U64, Digest};

#[cfg(feature = "group")]
use {
    group::{cofactor::CofactorGroup, prime::PrimeGroup, GroupEncoding},
    rand_core::RngCore,
    subtle::CtOption,
};

use subtle::Choice;
use subtle::ConditionallyNegatable;
use subtle::ConditionallySelectable;
use subtle::ConstantTimeEq;

#[cfg(feature = "zeroize")]
use zeroize::Zeroize;

use crate::constants;

use crate::field::FieldElement;
use crate::scalar::{clamp_integer, Scalar};

use crate::montgomery::MontgomeryPoint;

use crate::backend::serial::curve_models::AffineNielsPoint;
use crate::backend::serial::curve_models::CompletedPoint;
use crate::backend::serial::curve_models::ProjectiveNielsPoint;
use crate::backend::serial::curve_models::ProjectivePoint;

#[cfg(feature = "precomputed-tables")]
use crate::window::{
    LookupTableRadix128, LookupTableRadix16, LookupTableRadix256, LookupTableRadix32,
    LookupTableRadix64,
};

#[cfg(feature = "precomputed-tables")]
use crate::traits::BasepointTable;

use crate::traits::ValidityCheck;
use crate::traits::{Identity, IsIdentity};

#[cfg(feature = "alloc")]
use crate::traits::MultiscalarMul;
#[cfg(feature = "alloc")]
use crate::traits::{VartimeMultiscalarMul, VartimePrecomputedMultiscalarMul};

// ------------------------------------------------------------------------
// Compressed points
// ------------------------------------------------------------------------

/// In "Edwards y" / "Ed25519" format, the curve point \\((x,y)\\) is
/// determined by the \\(y\\)-coordinate and the sign of \\(x\\).
///
/// The first 255 bits of a `CompressedEdwardsY` represent the
/// \\(y\\)-coordinate.  The high bit of the 32nd byte gives the sign of \\(x\\).
#[derive(Copy, Clone, Eq, PartialEq, Hash)]
pub struct CompressedEdwardsY(pub [u8; 32]);

impl ConstantTimeEq for CompressedEdwardsY {
    fn ct_eq(&self, other: &CompressedEdwardsY) -> Choice {
        self.as_bytes().ct_eq(other.as_bytes())
    }
}

impl Debug for CompressedEdwardsY {
    fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
        write!(f, "CompressedEdwardsY: {:?}", self.as_bytes())
    }
}

impl CompressedEdwardsY {
    /// View this `CompressedEdwardsY` as an array of bytes.
    pub const fn as_bytes(&self) -> &[u8; 32] {
        &self.0
    }

    /// Copy this `CompressedEdwardsY` to an array of bytes.
    pub const fn to_bytes(&self) -> [u8; 32] {
        self.0
    }

    /// Attempt to decompress to an `EdwardsPoint`.
    ///
    /// Returns `None` if the input is not the \\(y\\)-coordinate of a
    /// curve point.
    pub fn decompress(&self) -> Option<EdwardsPoint> {
        let (is_valid_y_coord, X, Y, Z) = decompress::step_1(self);

        if is_valid_y_coord.into() {
            Some(decompress::step_2(self, X, Y, Z))
        } else {
            None
        }
    }
}

mod decompress {
    use super::*;

    #[rustfmt::skip] // keep alignment of explanatory comments
    pub(super) fn step_1(
        repr: &CompressedEdwardsY,
    ) -> (Choice, FieldElement, FieldElement, FieldElement) {
        let Y = FieldElement::from_bytes(repr.as_bytes());
        let Z = FieldElement::ONE;
        let YY = Y.square();
        let u = &YY - &Z;                            // u =  y²-1
        let v = &(&YY * &constants::EDWARDS_D) + &Z; // v = dy²+1
        let (is_valid_y_coord, X) = FieldElement::sqrt_ratio_i(&u, &v);

        (is_valid_y_coord, X, Y, Z)
    }

    #[rustfmt::skip]
    pub(super) fn step_2(
        repr: &CompressedEdwardsY,
        mut X: FieldElement,
        Y: FieldElement,
        Z: FieldElement,
    ) -> EdwardsPoint {
         // FieldElement::sqrt_ratio_i always returns the nonnegative square root,
         // so we negate according to the supplied sign bit.
        let compressed_sign_bit = Choice::from(repr.as_bytes()[31] >> 7);
        X.conditional_negate(compressed_sign_bit);

        EdwardsPoint {
            X,
            Y,
            Z,
            T: &X * &Y,
        }
    }
}

impl TryFrom<&[u8]> for CompressedEdwardsY {
    type Error = TryFromSliceError;

    fn try_from(slice: &[u8]) -> Result<CompressedEdwardsY, TryFromSliceError> {
        Self::from_slice(slice)
    }
}

// ------------------------------------------------------------------------
// Serde support
// ------------------------------------------------------------------------
// Serializes to and from `EdwardsPoint` directly, doing compression
// and decompression internally.  This means that users can create
// structs containing `EdwardsPoint`s and use Serde's derived
// serializers to serialize those structures.

#[cfg(feature = "serde")]
use serde::de::Visitor;
#[cfg(feature = "serde")]
use serde::{self, Deserialize, Deserializer, Serialize, Serializer};

#[cfg(feature = "serde")]
impl Serialize for EdwardsPoint {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        use serde::ser::SerializeTuple;
        let mut tup = serializer.serialize_tuple(32)?;
        for byte in self.compress().as_bytes().iter() {
            tup.serialize_element(byte)?;
        }
        tup.end()
    }
}

#[cfg(feature = "serde")]
impl Serialize for CompressedEdwardsY {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        use serde::ser::SerializeTuple;
        let mut tup = serializer.serialize_tuple(32)?;
        for byte in self.as_bytes().iter() {
            tup.serialize_element(byte)?;
        }
        tup.end()
    }
}

#[cfg(feature = "serde")]
impl<'de> Deserialize<'de> for EdwardsPoint {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct EdwardsPointVisitor;

        impl<'de> Visitor<'de> for EdwardsPointVisitor {
            type Value = EdwardsPoint;

            fn expecting(&self, formatter: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
                formatter.write_str("a valid point in Edwards y + sign format")
            }

            fn visit_seq<A>(self, mut seq: A) -> Result<EdwardsPoint, A::Error>
            where
                A: serde::de::SeqAccess<'de>,
            {
                let mut bytes = [0u8; 32];
                #[allow(clippy::needless_range_loop)]
                for i in 0..32 {
                    bytes[i] = seq
                        .next_element()?
                        .ok_or_else(|| serde::de::Error::invalid_length(i, &"expected 32 bytes"))?;
                }
                CompressedEdwardsY(bytes)
                    .decompress()
                    .ok_or_else(|| serde::de::Error::custom("decompression failed"))
            }
        }

        deserializer.deserialize_tuple(32, EdwardsPointVisitor)
    }
}

#[cfg(feature = "serde")]
impl<'de> Deserialize<'de> for CompressedEdwardsY {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct CompressedEdwardsYVisitor;

        impl<'de> Visitor<'de> for CompressedEdwardsYVisitor {
            type Value = CompressedEdwardsY;

            fn expecting(&self, formatter: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
                formatter.write_str("32 bytes of data")
            }

            fn visit_seq<A>(self, mut seq: A) -> Result<CompressedEdwardsY, A::Error>
            where
                A: serde::de::SeqAccess<'de>,
            {
                let mut bytes = [0u8; 32];
                #[allow(clippy::needless_range_loop)]
                for i in 0..32 {
                    bytes[i] = seq
                        .next_element()?
                        .ok_or_else(|| serde::de::Error::invalid_length(i, &"expected 32 bytes"))?;
                }
                Ok(CompressedEdwardsY(bytes))
            }
        }

        deserializer.deserialize_tuple(32, CompressedEdwardsYVisitor)
    }
}

// ------------------------------------------------------------------------
// Internal point representations
// ------------------------------------------------------------------------

/// An `EdwardsPoint` represents a point on the Edwards form of Curve25519.
#[derive(Copy, Clone)]
#[allow(missing_docs)]
pub struct EdwardsPoint {
    pub(crate) X: FieldElement,
    pub(crate) Y: FieldElement,
    pub(crate) Z: FieldElement,
    pub(crate) T: FieldElement,
}

// ------------------------------------------------------------------------
// Constructors
// ------------------------------------------------------------------------

impl Identity for CompressedEdwardsY {
    fn identity() -> CompressedEdwardsY {
        CompressedEdwardsY([
            1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0,
        ])
    }
}

impl Default for CompressedEdwardsY {
    fn default() -> CompressedEdwardsY {
        CompressedEdwardsY::identity()
    }
}

impl CompressedEdwardsY {
    /// Construct a `CompressedEdwardsY` from a slice of bytes.
    ///
    /// # Errors
    ///
    /// Returns [`TryFromSliceError`] if the input `bytes` slice does not have
    /// a length of 32.
    pub fn from_slice(bytes: &[u8]) -> Result<CompressedEdwardsY, TryFromSliceError> {
        bytes.try_into().map(CompressedEdwardsY)
    }
}

impl Identity for EdwardsPoint {
    fn identity() -> EdwardsPoint {
        EdwardsPoint {
            X: FieldElement::ZERO,
            Y: FieldElement::ONE,
            Z: FieldElement::ONE,
            T: FieldElement::ZERO,
        }
    }
}

impl Default for EdwardsPoint {
    fn default() -> EdwardsPoint {
        EdwardsPoint::identity()
    }
}

// ------------------------------------------------------------------------
// Zeroize implementations for wiping points from memory
// ------------------------------------------------------------------------

#[cfg(feature = "zeroize")]
impl Zeroize for CompressedEdwardsY {
    /// Reset this `CompressedEdwardsY` to the compressed form of the identity element.
    fn zeroize(&mut self) {
        self.0.zeroize();
        self.0[0] = 1;
    }
}

#[cfg(feature = "zeroize")]
impl Zeroize for EdwardsPoint {
    /// Reset this `CompressedEdwardsPoint` to the identity element.
    fn zeroize(&mut self) {
        self.X.zeroize();
        self.Y = FieldElement::ONE;
        self.Z = FieldElement::ONE;
        self.T.zeroize();
    }
}

// ------------------------------------------------------------------------
// Validity checks (for debugging, not CT)
// ------------------------------------------------------------------------

impl ValidityCheck for EdwardsPoint {
    fn is_valid(&self) -> bool {
        let point_on_curve = self.as_projective().is_valid();
        let on_segre_image = (&self.X * &self.Y) == (&self.Z * &self.T);

        point_on_curve && on_segre_image
    }
}

// ------------------------------------------------------------------------
// Constant-time assignment
// ------------------------------------------------------------------------

impl ConditionallySelectable for EdwardsPoint {
    fn conditional_select(a: &EdwardsPoint, b: &EdwardsPoint, choice: Choice) -> EdwardsPoint {
        EdwardsPoint {
            X: FieldElement::conditional_select(&a.X, &b.X, choice),
            Y: FieldElement::conditional_select(&a.Y, &b.Y, choice),
            Z: FieldElement::conditional_select(&a.Z, &b.Z, choice),
            T: FieldElement::conditional_select(&a.T, &b.T, choice),
        }
    }
}

// ------------------------------------------------------------------------
// Equality
// ------------------------------------------------------------------------

impl ConstantTimeEq for EdwardsPoint {
    fn ct_eq(&self, other: &EdwardsPoint) -> Choice {
        // We would like to check that the point (X/Z, Y/Z) is equal to
        // the point (X'/Z', Y'/Z') without converting into affine
        // coordinates (x, y) and (x', y'), which requires two inversions.
        // We have that X = xZ and X' = x'Z'. Thus, x = x' is equivalent to
        // (xZ)Z' = (x'Z')Z, and similarly for the y-coordinate.

        (&self.X * &other.Z).ct_eq(&(&other.X * &self.Z))
            & (&self.Y * &other.Z).ct_eq(&(&other.Y * &self.Z))
    }
}

impl PartialEq for EdwardsPoint {
    fn eq(&self, other: &EdwardsPoint) -> bool {
        self.ct_eq(other).into()
    }
}

impl Eq for EdwardsPoint {}

// ------------------------------------------------------------------------
// Point conversions
// ------------------------------------------------------------------------

impl EdwardsPoint {
    /// Convert to a ProjectiveNielsPoint
    pub(crate) fn as_projective_niels(&self) -> ProjectiveNielsPoint {
        ProjectiveNielsPoint {
            Y_plus_X: &self.Y + &self.X,
            Y_minus_X: &self.Y - &self.X,
            Z: self.Z,
            T2d: &self.T * &constants::EDWARDS_D2,
        }
    }

    /// Convert the representation of this point from extended
    /// coordinates to projective coordinates.
    ///
    /// Free.
    pub(crate) const fn as_projective(&self) -> ProjectivePoint {
        ProjectivePoint {
            X: self.X,
            Y: self.Y,
            Z: self.Z,
        }
    }

    /// Dehomogenize to a AffineNielsPoint.
    /// Mainly for testing.
    pub(crate) fn as_affine_niels(&self) -> AffineNielsPoint {
        let recip = self.Z.invert();
        let x = &self.X * &recip;
        let y = &self.Y * &recip;
        let xy2d = &(&x * &y) * &constants::EDWARDS_D2;
        AffineNielsPoint {
            y_plus_x: &y + &x,
            y_minus_x: &y - &x,
            xy2d,
        }
    }

    /// Convert this `EdwardsPoint` on the Edwards model to the
    /// corresponding `MontgomeryPoint` on the Montgomery model.
    ///
    /// This function has one exceptional case; the identity point of
    /// the Edwards curve is sent to the 2-torsion point \\((0,0)\\)
    /// on the Montgomery curve.
    ///
    /// Note that this is a one-way conversion, since the Montgomery
    /// model does not retain sign information.
    pub fn to_montgomery(&self) -> MontgomeryPoint {
        // We have u = (1+y)/(1-y) = (Z+Y)/(Z-Y).
        //
        // The denominator is zero only when y=1, the identity point of
        // the Edwards curve.  Since 0.invert() = 0, in this case we
        // compute the 2-torsion point (0,0).
        let U = &self.Z + &self.Y;
        let W = &self.Z - &self.Y;
        let u = &U * &W.invert();
        MontgomeryPoint(u.as_bytes())
    }

    /// Compress this point to `CompressedEdwardsY` format.
    pub fn compress(&self) -> CompressedEdwardsY {
        let recip = self.Z.invert();
        let x = &self.X * &recip;
        let y = &self.Y * &recip;
        let mut s: [u8; 32];

        s = y.as_bytes();
        s[31] ^= x.is_negative().unwrap_u8() << 7;
        CompressedEdwardsY(s)
    }

    #[cfg(feature = "digest")]
    /// Maps the digest of the input bytes to the curve. This is NOT a hash-to-curve function, as
    /// it produces points with a non-uniform distribution. Rather, it performs something that
    /// resembles (but is not) half of the
    /// [`hash_to_curve`](https://www.ietf.org/archive/id/draft-irtf-cfrg-hash-to-curve-16.html#section-3-4.2.1)
    /// function from the Elligator2 spec.
    #[deprecated(
        since = "4.0.0",
        note = "previously named `hash_from_bytes`, this is not a secure hash function"
    )]
    pub fn nonspec_map_to_curve<D>(bytes: &[u8]) -> EdwardsPoint
    where
        D: Digest<OutputSize = U64> + Default,
    {
        let mut hash = D::new();
        hash.update(bytes);
        let h = hash.finalize();
        let mut res = [0u8; 32];
        res.copy_from_slice(&h[..32]);

        let sign_bit = (res[31] & 0x80) >> 7;

        let fe = FieldElement::from_bytes(&res);

        let M1 = crate::montgomery::elligator_encode(&fe);
        let E1_opt = M1.to_edwards(sign_bit);

        E1_opt
            .expect("Montgomery conversion to Edwards point in Elligator failed")
            .mul_by_cofactor()
    }
}

// ------------------------------------------------------------------------
// Doubling
// ------------------------------------------------------------------------

impl EdwardsPoint {
    /// Add this point to itself.
    pub(crate) fn double(&self) -> EdwardsPoint {
        self.as_projective().double().as_extended()
    }
}

// ------------------------------------------------------------------------
// Addition and Subtraction
// ------------------------------------------------------------------------

impl<'a, 'b> Add<&'b EdwardsPoint> for &'a EdwardsPoint {
    type Output = EdwardsPoint;
    fn add(self, other: &'b EdwardsPoint) -> EdwardsPoint {
        (self + &other.as_projective_niels()).as_extended()
    }
}

define_add_variants!(
    LHS = EdwardsPoint,
    RHS = EdwardsPoint,
    Output = EdwardsPoint
);

impl<'b> AddAssign<&'b EdwardsPoint> for EdwardsPoint {
    fn add_assign(&mut self, _rhs: &'b EdwardsPoint) {
        *self = (self as &EdwardsPoint) + _rhs;
    }
}

define_add_assign_variants!(LHS = EdwardsPoint, RHS = EdwardsPoint);

impl<'a, 'b> Sub<&'b EdwardsPoint> for &'a EdwardsPoint {
    type Output = EdwardsPoint;
    fn sub(self, other: &'b EdwardsPoint) -> EdwardsPoint {
        (self - &other.as_projective_niels()).as_extended()
    }
}

define_sub_variants!(
    LHS = EdwardsPoint,
    RHS = EdwardsPoint,
    Output = EdwardsPoint
);

impl<'b> SubAssign<&'b EdwardsPoint> for EdwardsPoint {
    fn sub_assign(&mut self, _rhs: &'b EdwardsPoint) {
        *self = (self as &EdwardsPoint) - _rhs;
    }
}

define_sub_assign_variants!(LHS = EdwardsPoint, RHS = EdwardsPoint);

impl<T> Sum<T> for EdwardsPoint
where
    T: Borrow<EdwardsPoint>,
{
    fn sum<I>(iter: I) -> Self
    where
        I: Iterator<Item = T>,
    {
        iter.fold(EdwardsPoint::identity(), |acc, item| acc + item.borrow())
    }
}

// ------------------------------------------------------------------------
// Negation
// ------------------------------------------------------------------------

impl<'a> Neg for &'a EdwardsPoint {
    type Output = EdwardsPoint;

    fn neg(self) -> EdwardsPoint {
        EdwardsPoint {
            X: -(&self.X),
            Y: self.Y,
            Z: self.Z,
            T: -(&self.T),
        }
    }
}

impl Neg for EdwardsPoint {
    type Output = EdwardsPoint;

    fn neg(self) -> EdwardsPoint {
        -&self
    }
}

// ------------------------------------------------------------------------
// Scalar multiplication
// ------------------------------------------------------------------------

impl<'b> MulAssign<&'b Scalar> for EdwardsPoint {
    fn mul_assign(&mut self, scalar: &'b Scalar) {
        let result = (self as &EdwardsPoint) * scalar;
        *self = result;
    }
}

define_mul_assign_variants!(LHS = EdwardsPoint, RHS = Scalar);

define_mul_variants!(LHS = EdwardsPoint, RHS = Scalar, Output = EdwardsPoint);
define_mul_variants!(LHS = Scalar, RHS = EdwardsPoint, Output = EdwardsPoint);

impl<'a, 'b> Mul<&'b Scalar> for &'a EdwardsPoint {
    type Output = EdwardsPoint;
    /// Scalar multiplication: compute `scalar * self`.
    ///
    /// For scalar multiplication of a basepoint,
    /// `EdwardsBasepointTable` is approximately 4x faster.
    fn mul(self, scalar: &'b Scalar) -> EdwardsPoint {
        crate::backend::variable_base_mul(self, scalar)
    }
}

impl<'a, 'b> Mul<&'b EdwardsPoint> for &'a Scalar {
    type Output = EdwardsPoint;

    /// Scalar multiplication: compute `scalar * self`.
    ///
    /// For scalar multiplication of a basepoint,
    /// `EdwardsBasepointTable` is approximately 4x faster.
    fn mul(self, point: &'b EdwardsPoint) -> EdwardsPoint {
        point * self
    }
}

impl EdwardsPoint {
    /// Fixed-base scalar multiplication by the Ed25519 base point.
    ///
    /// Uses precomputed basepoint tables when the `precomputed-tables` feature
    /// is enabled, trading off increased code size for ~4x better performance.
    pub fn mul_base(scalar: &Scalar) -> Self {
        #[cfg(not(feature = "precomputed-tables"))]
        {
            scalar * constants::ED25519_BASEPOINT_POINT
        }

        #[cfg(feature = "precomputed-tables")]
        {
            scalar * constants::ED25519_BASEPOINT_TABLE
        }
    }

    /// Multiply this point by `clamp_integer(bytes)`. For a description of clamping, see
    /// [`clamp_integer`].
    pub fn mul_clamped(self, bytes: [u8; 32]) -> Self {
        // We have to construct a Scalar that is not reduced mod l, which breaks scalar invariant
        // #2. But #2 is not necessary for correctness of variable-base multiplication. All that
        // needs to hold is invariant #1, i.e., the scalar is less than 2^255. This is guaranteed
        // by clamping.
        // Further, we don't do any reduction or arithmetic with this clamped value, so there's no
        // issues arising from the fact that the curve point is not necessarily in the prime-order
        // subgroup.
        let s = Scalar {
            bytes: clamp_integer(bytes),
        };
        s * self
    }

    /// Multiply the basepoint by `clamp_integer(bytes)`. For a description of clamping, see
    /// [`clamp_integer`].
    pub fn mul_base_clamped(bytes: [u8; 32]) -> Self {
        // See reasoning in Self::mul_clamped why it is OK to make an unreduced Scalar here. We
        // note that fixed-base multiplication is also defined for all values of `bytes` less than
        // 2^255.
        let s = Scalar {
            bytes: clamp_integer(bytes),
        };
        Self::mul_base(&s)
    }
}

// ------------------------------------------------------------------------
// Multiscalar Multiplication impls
// ------------------------------------------------------------------------

// These use the iterator's size hint and the target settings to
// forward to a specific backend implementation.

#[cfg(feature = "alloc")]
impl MultiscalarMul for EdwardsPoint {
    type Point = EdwardsPoint;

    fn multiscalar_mul<I, J>(scalars: I, points: J) -> EdwardsPoint
    where
        I: IntoIterator,
        I::Item: Borrow<Scalar>,
        J: IntoIterator,
        J::Item: Borrow<EdwardsPoint>,
    {
        // Sanity-check lengths of input iterators
        let mut scalars = scalars.into_iter();
        let mut points = points.into_iter();

        // Lower and upper bounds on iterators
        let (s_lo, s_hi) = scalars.by_ref().size_hint();
        let (p_lo, p_hi) = points.by_ref().size_hint();

        // They should all be equal
        assert_eq!(s_lo, p_lo);
        assert_eq!(s_hi, Some(s_lo));
        assert_eq!(p_hi, Some(p_lo));

        // Now we know there's a single size.  When we do
        // size-dependent algorithm dispatch, use this as the hint.
        let _size = s_lo;

        crate::backend::straus_multiscalar_mul(scalars, points)
    }
}

#[cfg(feature = "alloc")]
impl VartimeMultiscalarMul for EdwardsPoint {
    type Point = EdwardsPoint;

    fn optional_multiscalar_mul<I, J>(scalars: I, points: J) -> Option<EdwardsPoint>
    where
        I: IntoIterator,
        I::Item: Borrow<Scalar>,
        J: IntoIterator<Item = Option<EdwardsPoint>>,
    {
        // Sanity-check lengths of input iterators
        let mut scalars = scalars.into_iter();
        let mut points = points.into_iter();

        // Lower and upper bounds on iterators
        let (s_lo, s_hi) = scalars.by_ref().size_hint();
        let (p_lo, p_hi) = points.by_ref().size_hint();

        // They should all be equal
        assert_eq!(s_lo, p_lo);
        assert_eq!(s_hi, Some(s_lo));
        assert_eq!(p_hi, Some(p_lo));

        // Now we know there's a single size.
        // Use this as the hint to decide which algorithm to use.
        let size = s_lo;

        if size < 190 {
            crate::backend::straus_optional_multiscalar_mul(scalars, points)
        } else {
            crate::backend::pippenger_optional_multiscalar_mul(scalars, points)
        }
    }
}

/// Precomputation for variable-time multiscalar multiplication with `EdwardsPoint`s.
// This wraps the inner implementation in a facade type so that we can
// decouple stability of the inner type from the stability of the
// outer type.
#[cfg(feature = "alloc")]
pub struct VartimeEdwardsPrecomputation(crate::backend::VartimePrecomputedStraus);

#[cfg(feature = "alloc")]
impl VartimePrecomputedMultiscalarMul for VartimeEdwardsPrecomputation {
    type Point = EdwardsPoint;

    fn new<I>(static_points: I) -> Self
    where
        I: IntoIterator,
        I::Item: Borrow<Self::Point>,
    {
        Self(crate::backend::VartimePrecomputedStraus::new(static_points))
    }

    fn optional_mixed_multiscalar_mul<I, J, K>(
        &self,
        static_scalars: I,
        dynamic_scalars: J,
        dynamic_points: K,
    ) -> Option<Self::Point>
    where
        I: IntoIterator,
        I::Item: Borrow<Scalar>,
        J: IntoIterator,
        J::Item: Borrow<Scalar>,
        K: IntoIterator<Item = Option<Self::Point>>,
    {
        self.0
            .optional_mixed_multiscalar_mul(static_scalars, dynamic_scalars, dynamic_points)
    }
}

impl EdwardsPoint {
    /// Compute \\(aA + bB\\) in variable time, where \\(B\\) is the Ed25519 basepoint.
    pub fn vartime_double_scalar_mul_basepoint(
        a: &Scalar,
        A: &EdwardsPoint,
        b: &Scalar,
    ) -> EdwardsPoint {
        crate::backend::vartime_double_base_mul(a, A, b)
    }
}

#[cfg(feature = "precomputed-tables")]
macro_rules! impl_basepoint_table {
    (Name = $name:ident, LookupTable = $table:ident, Point = $point:ty, Radix = $radix:expr, Additions = $adds:expr) => {
        /// A precomputed table of multiples of a basepoint, for accelerating
        /// fixed-base scalar multiplication.  One table, for the Ed25519
        /// basepoint, is provided in the [`constants`] module.
        ///
        /// The basepoint tables are reasonably large, so they should probably be boxed.
        ///
        /// The sizes for the tables and the number of additions required for one scalar
        /// multiplication are as follows:
        ///
        /// * [`EdwardsBasepointTableRadix16`]: 30KB, 64A
        ///   (this is the default size, and is used for
        ///   [`constants::ED25519_BASEPOINT_TABLE`])
        /// * [`EdwardsBasepointTableRadix64`]: 120KB, 43A
        /// * [`EdwardsBasepointTableRadix128`]: 240KB, 37A
        /// * [`EdwardsBasepointTableRadix256`]: 480KB, 33A
        ///
        /// # Why 33 additions for radix-256?
        ///
        /// Normally, the radix-256 tables would allow for only 32 additions per scalar
        /// multiplication.  However, due to the fact that standardised definitions of
        /// legacy protocols—such as x25519—require allowing unreduced 255-bit scalars
        /// invariants, when converting such an unreduced scalar's representation to
        /// radix-\\(2^{8}\\), we cannot guarantee the carry bit will fit in the last
        /// coefficient (the coefficients are `i8`s).  When, \\(w\\), the power-of-2 of
        /// the radix, is \\(w < 8\\), we can fold the final carry onto the last
        /// coefficient, \\(d\\), because \\(d < 2^{w/2}\\), so
        /// $$
        ///     d + carry \cdot 2^{w} = d + 1 \cdot 2^{w} < 2^{w+1} < 2^{8}
        /// $$
        /// When \\(w = 8\\), we can't fit \\(carry \cdot 2^{w}\\) into an `i8`, so we
        /// add the carry bit onto an additional coefficient.
        #[derive(Clone)]
        #[repr(transparent)]
        pub struct $name(pub(crate) [$table<AffineNielsPoint>; 32]);

        impl BasepointTable for $name {
            type Point = $point;

            /// Create a table of precomputed multiples of `basepoint`.
            fn create(basepoint: &$point) -> $name {
                // XXX use init_with
                let mut table = $name([$table::default(); 32]);
                let mut P = *basepoint;
                for i in 0..32 {
                    // P = (2w)^i * B
                    table.0[i] = $table::from(&P);
                    P = P.mul_by_pow_2($radix + $radix);
                }
                table
            }

            /// Get the basepoint for this table as an `EdwardsPoint`.
            fn basepoint(&self) -> $point {
                // self.0[0].select(1) = 1*(16^2)^0*B
                // but as an `AffineNielsPoint`, so add identity to convert to extended.
                (&<$point>::identity() + &self.0[0].select(1)).as_extended()
            }

            /// The computation uses Pippeneger's algorithm, as described for the
            /// specific case of radix-16 on page 13 of the Ed25519 paper.
            ///
            /// # Piggenger's Algorithm Generalised
            ///
            /// Write the scalar \\(a\\) in radix-\\(w\\), where \\(w\\) is a power of
            /// 2, with coefficients in \\([\frac{-w}{2},\frac{w}{2})\\), i.e.,
            /// $$
            ///     a = a\_0 + a\_1 w\^1 + \cdots + a\_{x} w\^{x},
            /// $$
            /// with
            /// $$
            /// \begin{aligned}
            ///     \frac{-w}{2} \leq a_i < \frac{w}{2}
            ///     &&\cdots&&
            ///     \frac{-w}{2} \leq a\_{x} \leq \frac{w}{2}
            /// \end{aligned}
            /// $$
            /// and the number of additions, \\(x\\), is given by
            /// \\(x = \lceil \frac{256}{w} \rceil\\). Then
            /// $$
            ///     a B = a\_0 B + a\_1 w\^1 B + \cdots + a\_{x-1} w\^{x-1} B.
            /// $$
            /// Grouping even and odd coefficients gives
            /// $$
            /// \begin{aligned}
            ///     a B = \quad a\_0 w\^0 B +& a\_2 w\^2 B + \cdots + a\_{x-2} w\^{x-2} B    \\\\
            ///               + a\_1 w\^1 B +& a\_3 w\^3 B + \cdots + a\_{x-1} w\^{x-1} B    \\\\
            ///         = \quad(a\_0 w\^0 B +& a\_2 w\^2 B + \cdots + a\_{x-2} w\^{x-2} B)   \\\\
            ///             + w(a\_1 w\^0 B +& a\_3 w\^2 B + \cdots + a\_{x-1} w\^{x-2} B).  \\\\
            /// \end{aligned}
            /// $$
            /// For each \\(i = 0 \ldots 31\\), we create a lookup table of
            /// $$
            /// [w\^{2i} B, \ldots, \frac{w}{2}\cdot w\^{2i} B],
            /// $$
            /// and use it to select \\( y \cdot w\^{2i} \cdot B \\) in constant time.
            ///
            /// The radix-\\(w\\) representation requires that the scalar is bounded
            /// by \\(2\^{255}\\), which is always the case.
            ///
            /// The above algorithm is trivially generalised to other powers-of-2 radices.
            fn mul_base(&self, scalar: &Scalar) -> $point {
                let a = scalar.as_radix_2w($radix);

                let tables = &self.0;
                let mut P = <$point>::identity();

                for i in (0..$adds).filter(|x| x % 2 == 1) {
                    P = (&P + &tables[i / 2].select(a[i])).as_extended();
                }

                P = P.mul_by_pow_2($radix);

                for i in (0..$adds).filter(|x| x % 2 == 0) {
                    P = (&P + &tables[i / 2].select(a[i])).as_extended();
                }

                P
            }
        }

        impl<'a, 'b> Mul<&'b Scalar> for &'a $name {
            type Output = $point;

            /// Construct an `EdwardsPoint` from a `Scalar` \\(a\\) by
            /// computing the multiple \\(aB\\) of this basepoint \\(B\\).
            fn mul(self, scalar: &'b Scalar) -> $point {
                // delegate to a private function so that its documentation appears in internal docs
                self.mul_base(scalar)
            }
        }

        impl<'a, 'b> Mul<&'a $name> for &'b Scalar {
            type Output = $point;

            /// Construct an `EdwardsPoint` from a `Scalar` \\(a\\) by
            /// computing the multiple \\(aB\\) of this basepoint \\(B\\).
            fn mul(self, basepoint_table: &'a $name) -> $point {
                basepoint_table * self
            }
        }

        impl Debug for $name {
            fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
                write!(f, "{:?}([\n", stringify!($name))?;
                for i in 0..32 {
                    write!(f, "\t{:?},\n", &self.0[i])?;
                }
                write!(f, "])")
            }
        }
    };
} // End macro_rules! impl_basepoint_table

// The number of additions required is ceil(256/w) where w is the radix representation.
cfg_if! {
    if #[cfg(feature = "precomputed-tables")] {
        impl_basepoint_table! {
            Name = EdwardsBasepointTable,
            LookupTable = LookupTableRadix16,
            Point = EdwardsPoint,
            Radix = 4,
            Additions = 64
        }
        impl_basepoint_table! {
            Name = EdwardsBasepointTableRadix32,
            LookupTable = LookupTableRadix32,
            Point = EdwardsPoint,
            Radix = 5,
            Additions = 52
        }
        impl_basepoint_table! {
            Name = EdwardsBasepointTableRadix64,
            LookupTable = LookupTableRadix64,
            Point = EdwardsPoint,
            Radix = 6,
            Additions = 43
        }
        impl_basepoint_table! {
            Name = EdwardsBasepointTableRadix128,
            LookupTable = LookupTableRadix128,
            Point = EdwardsPoint,
            Radix = 7,
            Additions = 37
        }
        impl_basepoint_table! {
            Name = EdwardsBasepointTableRadix256,
            LookupTable = LookupTableRadix256,
            Point = EdwardsPoint,
            Radix = 8,
            Additions = 33
        }

        /// A type-alias for [`EdwardsBasepointTable`] because the latter is
        /// used as a constructor in the [`constants`] module.
        //
        // Same as for `LookupTableRadix16`, we have to define `EdwardsBasepointTable`
        // first, because it's used as a constructor, and then provide a type alias for
        // it.
        pub type EdwardsBasepointTableRadix16 = EdwardsBasepointTable;
    }
}

#[cfg(feature = "precomputed-tables")]
macro_rules! impl_basepoint_table_conversions {
    (LHS = $lhs:ty, RHS = $rhs:ty) => {
        impl<'a> From<&'a $lhs> for $rhs {
            fn from(table: &'a $lhs) -> $rhs {
                <$rhs>::create(&table.basepoint())
            }
        }

        impl<'a> From<&'a $rhs> for $lhs {
            fn from(table: &'a $rhs) -> $lhs {
                <$lhs>::create(&table.basepoint())
            }
        }
    };
}

cfg_if! {
    if #[cfg(feature = "precomputed-tables")] {
        // Conversions from radix 16
        impl_basepoint_table_conversions! {
            LHS = EdwardsBasepointTableRadix16,
            RHS = EdwardsBasepointTableRadix32
        }
        impl_basepoint_table_conversions! {
            LHS = EdwardsBasepointTableRadix16,
            RHS = EdwardsBasepointTableRadix64
        }
        impl_basepoint_table_conversions! {
            LHS = EdwardsBasepointTableRadix16,
            RHS = EdwardsBasepointTableRadix128
        }
        impl_basepoint_table_conversions! {
            LHS = EdwardsBasepointTableRadix16,
            RHS = EdwardsBasepointTableRadix256
        }

        // Conversions from radix 32
        impl_basepoint_table_conversions! {
            LHS = EdwardsBasepointTableRadix32,
            RHS = EdwardsBasepointTableRadix64
        }
        impl_basepoint_table_conversions! {
            LHS = EdwardsBasepointTableRadix32,
            RHS = EdwardsBasepointTableRadix128
        }
        impl_basepoint_table_conversions! {
            LHS = EdwardsBasepointTableRadix32,
            RHS = EdwardsBasepointTableRadix256
        }

        // Conversions from radix 64
        impl_basepoint_table_conversions! {
            LHS = EdwardsBasepointTableRadix64,
            RHS = EdwardsBasepointTableRadix128
        }
        impl_basepoint_table_conversions! {
            LHS = EdwardsBasepointTableRadix64,
            RHS = EdwardsBasepointTableRadix256
        }

        // Conversions from radix 128
        impl_basepoint_table_conversions! {
            LHS = EdwardsBasepointTableRadix128,
            RHS = EdwardsBasepointTableRadix256
        }
    }
}

impl EdwardsPoint {
    /// Multiply by the cofactor: return \\(\[8\]P\\).
    pub fn mul_by_cofactor(&self) -> EdwardsPoint {
        self.mul_by_pow_2(3)
    }

    /// Compute \\([2\^k] P \\) by successive doublings. Requires \\( k > 0 \\).
    pub(crate) fn mul_by_pow_2(&self, k: u32) -> EdwardsPoint {
        debug_assert!(k > 0);
        let mut r: CompletedPoint;
        let mut s = self.as_projective();
        for _ in 0..(k - 1) {
            r = s.double();
            s = r.as_projective();
        }
        // Unroll last iteration so we can go directly as_extended()
        s.double().as_extended()
    }

    /// Determine if this point is of small order.
    ///
    /// # Return
    ///
    /// * `true` if `self` is in the torsion subgroup \\( \mathcal E\[8\] \\);
    /// * `false` if `self` is not in the torsion subgroup \\( \mathcal E\[8\] \\).
    ///
    /// # Example
    ///
    /// ```
    /// use curve25519_dalek::constants;
    ///
    /// // Generator of the prime-order subgroup
    /// let P = constants::ED25519_BASEPOINT_POINT;
    /// // Generator of the torsion subgroup
    /// let Q = constants::EIGHT_TORSION[1];
    ///
    /// // P has large order
    /// assert_eq!(P.is_small_order(), false);
    ///
    /// // Q has small order
    /// assert_eq!(Q.is_small_order(), true);
    /// ```
    pub fn is_small_order(&self) -> bool {
        self.mul_by_cofactor().is_identity()
    }

    /// Determine if this point is “torsion-free”, i.e., is contained in
    /// the prime-order subgroup.
    ///
    /// # Return
    ///
    /// * `true` if `self` has zero torsion component and is in the
    /// prime-order subgroup;
    /// * `false` if `self` has a nonzero torsion component and is not
    /// in the prime-order subgroup.
    ///
    /// # Example
    ///
    /// ```
    /// use curve25519_dalek::constants;
    ///
    /// // Generator of the prime-order subgroup
    /// let P = constants::ED25519_BASEPOINT_POINT;
    /// // Generator of the torsion subgroup
    /// let Q = constants::EIGHT_TORSION[1];
    ///
    /// // P is torsion-free
    /// assert_eq!(P.is_torsion_free(), true);
    ///
    /// // P + Q is not torsion-free
    /// assert_eq!((P+Q).is_torsion_free(), false);
    /// ```
    pub fn is_torsion_free(&self) -> bool {
        (self * constants::BASEPOINT_ORDER_PRIVATE).is_identity()
    }
}

// ------------------------------------------------------------------------
// Debug traits
// ------------------------------------------------------------------------

impl Debug for EdwardsPoint {
    fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
        write!(
            f,
            "EdwardsPoint{{\n\tX: {:?},\n\tY: {:?},\n\tZ: {:?},\n\tT: {:?}\n}}",
            &self.X, &self.Y, &self.Z, &self.T
        )
    }
}

// ------------------------------------------------------------------------
// group traits
// ------------------------------------------------------------------------

// Use the full trait path to avoid Group::identity overlapping Identity::identity in the
// rest of the module (e.g. tests).
#[cfg(feature = "group")]
impl group::Group for EdwardsPoint {
    type Scalar = Scalar;

    fn random(mut rng: impl RngCore) -> Self {
        let mut repr = CompressedEdwardsY([0u8; 32]);
        loop {
            rng.fill_bytes(&mut repr.0);
            if let Some(p) = repr.decompress() {
                if !IsIdentity::is_identity(&p) {
                    break p;
                }
            }
        }
    }

    fn identity() -> Self {
        Identity::identity()
    }

    fn generator() -> Self {
        constants::ED25519_BASEPOINT_POINT
    }

    fn is_identity(&self) -> Choice {
        self.ct_eq(&Identity::identity())
    }

    fn double(&self) -> Self {
        self.double()
    }
}

#[cfg(feature = "group")]
impl GroupEncoding for EdwardsPoint {
    type Repr = [u8; 32];

    fn from_bytes(bytes: &Self::Repr) -> CtOption<Self> {
        let repr = CompressedEdwardsY(*bytes);
        let (is_valid_y_coord, X, Y, Z) = decompress::step_1(&repr);
        CtOption::new(decompress::step_2(&repr, X, Y, Z), is_valid_y_coord)
    }

    fn from_bytes_unchecked(bytes: &Self::Repr) -> CtOption<Self> {
        // Just use the checked API; there are no checks we can skip.
        Self::from_bytes(bytes)
    }

    fn to_bytes(&self) -> Self::Repr {
        self.compress().to_bytes()
    }
}

/// A `SubgroupPoint` represents a point on the Edwards form of Curve25519, that is
/// guaranteed to be in the prime-order subgroup.
#[cfg(feature = "group")]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct SubgroupPoint(EdwardsPoint);

#[cfg(feature = "group")]
impl From<SubgroupPoint> for EdwardsPoint {
    fn from(p: SubgroupPoint) -> Self {
        p.0
    }
}

#[cfg(feature = "group")]
impl Neg for SubgroupPoint {
    type Output = Self;

    fn neg(self) -> Self::Output {
        SubgroupPoint(-self.0)
    }
}

#[cfg(feature = "group")]
impl Add<&SubgroupPoint> for &SubgroupPoint {
    type Output = SubgroupPoint;
    fn add(self, other: &SubgroupPoint) -> SubgroupPoint {
        SubgroupPoint(self.0 + other.0)
    }
}

#[cfg(feature = "group")]
define_add_variants!(
    LHS = SubgroupPoint,
    RHS = SubgroupPoint,
    Output = SubgroupPoint
);

#[cfg(feature = "group")]
impl Add<&SubgroupPoint> for &EdwardsPoint {
    type Output = EdwardsPoint;
    fn add(self, other: &SubgroupPoint) -> EdwardsPoint {
        self + other.0
    }
}

#[cfg(feature = "group")]
define_add_variants!(
    LHS = EdwardsPoint,
    RHS = SubgroupPoint,
    Output = EdwardsPoint
);

#[cfg(feature = "group")]
impl AddAssign<&SubgroupPoint> for SubgroupPoint {
    fn add_assign(&mut self, rhs: &SubgroupPoint) {
        self.0 += rhs.0
    }
}

#[cfg(feature = "group")]
define_add_assign_variants!(LHS = SubgroupPoint, RHS = SubgroupPoint);

#[cfg(feature = "group")]
impl AddAssign<&SubgroupPoint> for EdwardsPoint {
    fn add_assign(&mut self, rhs: &SubgroupPoint) {
        *self += rhs.0
    }
}

#[cfg(feature = "group")]
define_add_assign_variants!(LHS = EdwardsPoint, RHS = SubgroupPoint);

#[cfg(feature = "group")]
impl Sub<&SubgroupPoint> for &SubgroupPoint {
    type Output = SubgroupPoint;
    fn sub(self, other: &SubgroupPoint) -> SubgroupPoint {
        SubgroupPoint(self.0 - other.0)
    }
}

#[cfg(feature = "group")]
define_sub_variants!(
    LHS = SubgroupPoint,
    RHS = SubgroupPoint,
    Output = SubgroupPoint
);

#[cfg(feature = "group")]
impl Sub<&SubgroupPoint> for &EdwardsPoint {
    type Output = EdwardsPoint;
    fn sub(self, other: &SubgroupPoint) -> EdwardsPoint {
        self - other.0
    }
}

#[cfg(feature = "group")]
define_sub_variants!(
    LHS = EdwardsPoint,
    RHS = SubgroupPoint,
    Output = EdwardsPoint
);

#[cfg(feature = "group")]
impl SubAssign<&SubgroupPoint> for SubgroupPoint {
    fn sub_assign(&mut self, rhs: &SubgroupPoint) {
        self.0 -= rhs.0;
    }
}

#[cfg(feature = "group")]
define_sub_assign_variants!(LHS = SubgroupPoint, RHS = SubgroupPoint);

#[cfg(feature = "group")]
impl SubAssign<&SubgroupPoint> for EdwardsPoint {
    fn sub_assign(&mut self, rhs: &SubgroupPoint) {
        *self -= rhs.0;
    }
}

#[cfg(feature = "group")]
define_sub_assign_variants!(LHS = EdwardsPoint, RHS = SubgroupPoint);

#[cfg(feature = "group")]
impl<T> Sum<T> for SubgroupPoint
where
    T: Borrow<SubgroupPoint>,
{
    fn sum<I>(iter: I) -> Self
    where
        I: Iterator<Item = T>,
    {
        use group::Group;
        iter.fold(SubgroupPoint::identity(), |acc, item| acc + item.borrow())
    }
}

#[cfg(feature = "group")]
impl Mul<&Scalar> for &SubgroupPoint {
    type Output = SubgroupPoint;

    /// Scalar multiplication: compute `scalar * self`.
    ///
    /// For scalar multiplication of a basepoint,
    /// `EdwardsBasepointTable` is approximately 4x faster.
    fn mul(self, scalar: &Scalar) -> SubgroupPoint {
        SubgroupPoint(self.0 * scalar)
    }
}

#[cfg(feature = "group")]
define_mul_variants!(LHS = Scalar, RHS = SubgroupPoint, Output = SubgroupPoint);

#[cfg(feature = "group")]
impl Mul<&SubgroupPoint> for &Scalar {
    type Output = SubgroupPoint;

    /// Scalar multiplication: compute `scalar * self`.
    ///
    /// For scalar multiplication of a basepoint,
    /// `EdwardsBasepointTable` is approximately 4x faster.
    fn mul(self, point: &SubgroupPoint) -> SubgroupPoint {
        point * self
    }
}

#[cfg(feature = "group")]
define_mul_variants!(LHS = SubgroupPoint, RHS = Scalar, Output = SubgroupPoint);

#[cfg(feature = "group")]
impl MulAssign<&Scalar> for SubgroupPoint {
    fn mul_assign(&mut self, scalar: &Scalar) {
        self.0 *= scalar;
    }
}

#[cfg(feature = "group")]
define_mul_assign_variants!(LHS = SubgroupPoint, RHS = Scalar);

#[cfg(feature = "group")]
impl group::Group for SubgroupPoint {
    type Scalar = Scalar;

    fn random(mut rng: impl RngCore) -> Self {
        use group::ff::Field;

        // This will almost never loop, but `Group::random` is documented as returning a
        // non-identity element.
        let s = loop {
            let s: Scalar = Field::random(&mut rng);
            if !s.is_zero_vartime() {
                break s;
            }
        };

        // This gives an element of the prime-order subgroup.
        Self::generator() * s
    }

    fn identity() -> Self {
        SubgroupPoint(Identity::identity())
    }

    fn generator() -> Self {
        SubgroupPoint(EdwardsPoint::generator())
    }

    fn is_identity(&self) -> Choice {
        self.0.ct_eq(&Identity::identity())
    }

    fn double(&self) -> Self {
        SubgroupPoint(self.0.double())
    }
}

#[cfg(feature = "group")]
impl GroupEncoding for SubgroupPoint {
    type Repr = <EdwardsPoint as GroupEncoding>::Repr;

    fn from_bytes(bytes: &Self::Repr) -> CtOption<Self> {
        EdwardsPoint::from_bytes(bytes).and_then(|p| p.into_subgroup())
    }

    fn from_bytes_unchecked(bytes: &Self::Repr) -> CtOption<Self> {
        EdwardsPoint::from_bytes_unchecked(bytes).and_then(|p| p.into_subgroup())
    }

    fn to_bytes(&self) -> Self::Repr {
        self.0.compress().to_bytes()
    }
}

#[cfg(feature = "group")]
impl PrimeGroup for SubgroupPoint {}

/// Ristretto has a cofactor of 1.
#[cfg(feature = "group")]
impl CofactorGroup for EdwardsPoint {
    type Subgroup = SubgroupPoint;

    fn clear_cofactor(&self) -> Self::Subgroup {
        SubgroupPoint(self.mul_by_cofactor())
    }

    fn into_subgroup(self) -> CtOption<Self::Subgroup> {
        CtOption::new(SubgroupPoint(self), CofactorGroup::is_torsion_free(&self))
    }

    fn is_torsion_free(&self) -> Choice {
        (self * constants::BASEPOINT_ORDER_PRIVATE).ct_eq(&Self::identity())
    }
}

// ------------------------------------------------------------------------
// Tests
// ------------------------------------------------------------------------

#[cfg(test)]
mod test {
    use super::*;
    use crate::{field::FieldElement, scalar::Scalar};
    use subtle::ConditionallySelectable;

    #[cfg(feature = "alloc")]
    use alloc::vec::Vec;

    #[cfg(feature = "precomputed-tables")]
    use crate::constants::ED25519_BASEPOINT_TABLE;

    use rand_core::RngCore;

    /// X coordinate of the basepoint.
    /// = 15112221349535400772501151409588531511454012693041857206046113283949847762202
    static BASE_X_COORD_BYTES: [u8; 32] = [
        0x1a, 0xd5, 0x25, 0x8f, 0x60, 0x2d, 0x56, 0xc9, 0xb2, 0xa7, 0x25, 0x95, 0x60, 0xc7, 0x2c,
        0x69, 0x5c, 0xdc, 0xd6, 0xfd, 0x31, 0xe2, 0xa4, 0xc0, 0xfe, 0x53, 0x6e, 0xcd, 0xd3, 0x36,
        0x69, 0x21,
    ];

    /// Compressed Edwards Y form of 2*basepoint.
    static BASE2_CMPRSSD: CompressedEdwardsY = CompressedEdwardsY([
        0xc9, 0xa3, 0xf8, 0x6a, 0xae, 0x46, 0x5f, 0xe, 0x56, 0x51, 0x38, 0x64, 0x51, 0x0f, 0x39,
        0x97, 0x56, 0x1f, 0xa2, 0xc9, 0xe8, 0x5e, 0xa2, 0x1d, 0xc2, 0x29, 0x23, 0x09, 0xf3, 0xcd,
        0x60, 0x22,
    ]);

    /// Compressed Edwards Y form of 16*basepoint.
    static BASE16_CMPRSSD: CompressedEdwardsY = CompressedEdwardsY([
        0xeb, 0x27, 0x67, 0xc1, 0x37, 0xab, 0x7a, 0xd8, 0x27, 0x9c, 0x07, 0x8e, 0xff, 0x11, 0x6a,
        0xb0, 0x78, 0x6e, 0xad, 0x3a, 0x2e, 0x0f, 0x98, 0x9f, 0x72, 0xc3, 0x7f, 0x82, 0xf2, 0x96,
        0x96, 0x70,
    ]);

    /// 4493907448824000747700850167940867464579944529806937181821189941592931634714
    pub static A_SCALAR: Scalar = Scalar {
        bytes: [
            0x1a, 0x0e, 0x97, 0x8a, 0x90, 0xf6, 0x62, 0x2d, 0x37, 0x47, 0x02, 0x3f, 0x8a, 0xd8,
            0x26, 0x4d, 0xa7, 0x58, 0xaa, 0x1b, 0x88, 0xe0, 0x40, 0xd1, 0x58, 0x9e, 0x7b, 0x7f,
            0x23, 0x76, 0xef, 0x09,
        ],
    };

    /// 2506056684125797857694181776241676200180934651973138769173342316833279714961
    pub static B_SCALAR: Scalar = Scalar {
        bytes: [
            0x91, 0x26, 0x7a, 0xcf, 0x25, 0xc2, 0x09, 0x1b, 0xa2, 0x17, 0x74, 0x7b, 0x66, 0xf0,
            0xb3, 0x2e, 0x9d, 0xf2, 0xa5, 0x67, 0x41, 0xcf, 0xda, 0xc4, 0x56, 0xa7, 0xd4, 0xaa,
            0xb8, 0x60, 0x8a, 0x05,
        ],
    };

    /// A_SCALAR * basepoint, computed with ed25519.py
    pub static A_TIMES_BASEPOINT: CompressedEdwardsY = CompressedEdwardsY([
        0xea, 0x27, 0xe2, 0x60, 0x53, 0xdf, 0x1b, 0x59, 0x56, 0xf1, 0x4d, 0x5d, 0xec, 0x3c, 0x34,
        0xc3, 0x84, 0xa2, 0x69, 0xb7, 0x4c, 0xc3, 0x80, 0x3e, 0xa8, 0xe2, 0xe7, 0xc9, 0x42, 0x5e,
        0x40, 0xa5,
    ]);

    /// A_SCALAR * (A_TIMES_BASEPOINT) + B_SCALAR * BASEPOINT
    /// computed with ed25519.py
    static DOUBLE_SCALAR_MULT_RESULT: CompressedEdwardsY = CompressedEdwardsY([
        0x7d, 0xfd, 0x6c, 0x45, 0xaf, 0x6d, 0x6e, 0x0e, 0xba, 0x20, 0x37, 0x1a, 0x23, 0x64, 0x59,
        0xc4, 0xc0, 0x46, 0x83, 0x43, 0xde, 0x70, 0x4b, 0x85, 0x09, 0x6f, 0xfe, 0x35, 0x4f, 0x13,
        0x2b, 0x42,
    ]);

    /// Test round-trip decompression for the basepoint.
    #[test]
    fn basepoint_decompression_compression() {
        let base_X = FieldElement::from_bytes(&BASE_X_COORD_BYTES);
        let bp = constants::ED25519_BASEPOINT_COMPRESSED
            .decompress()
            .unwrap();
        assert!(bp.is_valid());
        // Check that decompression actually gives the correct X coordinate
        assert_eq!(base_X, bp.X);
        assert_eq!(bp.compress(), constants::ED25519_BASEPOINT_COMPRESSED);
    }

    /// Test sign handling in decompression
    #[test]
    fn decompression_sign_handling() {
        // Manually set the high bit of the last byte to flip the sign
        let mut minus_basepoint_bytes = *constants::ED25519_BASEPOINT_COMPRESSED.as_bytes();
        minus_basepoint_bytes[31] |= 1 << 7;
        let minus_basepoint = CompressedEdwardsY(minus_basepoint_bytes)
            .decompress()
            .unwrap();
        // Test projective coordinates exactly since we know they should
        // only differ by a flipped sign.
        assert_eq!(minus_basepoint.X, -(&constants::ED25519_BASEPOINT_POINT.X));
        assert_eq!(minus_basepoint.Y, constants::ED25519_BASEPOINT_POINT.Y);
        assert_eq!(minus_basepoint.Z, constants::ED25519_BASEPOINT_POINT.Z);
        assert_eq!(minus_basepoint.T, -(&constants::ED25519_BASEPOINT_POINT.T));
    }

    /// Test that computing 1*basepoint gives the correct basepoint.
    #[cfg(feature = "precomputed-tables")]
    #[test]
    fn basepoint_mult_one_vs_basepoint() {
        let bp = ED25519_BASEPOINT_TABLE * &Scalar::ONE;
        let compressed = bp.compress();
        assert_eq!(compressed, constants::ED25519_BASEPOINT_COMPRESSED);
    }

    /// Test that `EdwardsBasepointTable::basepoint()` gives the correct basepoint.
    #[cfg(feature = "precomputed-tables")]
    #[test]
    fn basepoint_table_basepoint_function_correct() {
        let bp = ED25519_BASEPOINT_TABLE.basepoint();
        assert_eq!(bp.compress(), constants::ED25519_BASEPOINT_COMPRESSED);
    }

    /// Test `impl Add<EdwardsPoint> for EdwardsPoint`
    /// using basepoint + basepoint versus the 2*basepoint constant.
    #[test]
    fn basepoint_plus_basepoint_vs_basepoint2() {
        let bp = constants::ED25519_BASEPOINT_POINT;
        let bp_added = bp + bp;
        assert_eq!(bp_added.compress(), BASE2_CMPRSSD);
    }

    /// Test `impl Add<ProjectiveNielsPoint> for EdwardsPoint`
    /// using the basepoint, basepoint2 constants
    #[test]
    fn basepoint_plus_basepoint_projective_niels_vs_basepoint2() {
        let bp = constants::ED25519_BASEPOINT_POINT;
        let bp_added = (&bp + &bp.as_projective_niels()).as_extended();
        assert_eq!(bp_added.compress(), BASE2_CMPRSSD);
    }

    /// Test `impl Add<AffineNielsPoint> for EdwardsPoint`
    /// using the basepoint, basepoint2 constants
    #[test]
    fn basepoint_plus_basepoint_affine_niels_vs_basepoint2() {
        let bp = constants::ED25519_BASEPOINT_POINT;
        let bp_affine_niels = bp.as_affine_niels();
        let bp_added = (&bp + &bp_affine_niels).as_extended();
        assert_eq!(bp_added.compress(), BASE2_CMPRSSD);
    }

    /// Check that equality of `EdwardsPoints` handles projective
    /// coordinates correctly.
    #[test]
    fn extended_point_equality_handles_scaling() {
        let mut two_bytes = [0u8; 32];
        two_bytes[0] = 2;
        let id1 = EdwardsPoint::identity();
        let id2 = EdwardsPoint {
            X: FieldElement::ZERO,
            Y: FieldElement::from_bytes(&two_bytes),
            Z: FieldElement::from_bytes(&two_bytes),
            T: FieldElement::ZERO,
        };
        assert!(bool::from(id1.ct_eq(&id2)));
    }

    /// Sanity check for conversion to precomputed points
    #[cfg(feature = "precomputed-tables")]
    #[test]
    fn to_affine_niels_clears_denominators() {
        // construct a point as aB so it has denominators (ie. Z != 1)
        let aB = ED25519_BASEPOINT_TABLE * &A_SCALAR;
        let aB_affine_niels = aB.as_affine_niels();
        let also_aB = (&EdwardsPoint::identity() + &aB_affine_niels).as_extended();
        assert_eq!(aB.compress(), also_aB.compress());
    }

    /// Test mul_base versus a known scalar multiple from ed25519.py
    #[test]
    fn basepoint_mult_vs_ed25519py() {
        let aB = EdwardsPoint::mul_base(&A_SCALAR);
        assert_eq!(aB.compress(), A_TIMES_BASEPOINT);
    }

    /// Test that multiplication by the basepoint order kills the basepoint
    #[test]
    fn basepoint_mult_by_basepoint_order() {
        let should_be_id = EdwardsPoint::mul_base(&constants::BASEPOINT_ORDER_PRIVATE);
        assert!(should_be_id.is_identity());
    }

    /// Test precomputed basepoint mult
    #[cfg(feature = "precomputed-tables")]
    #[test]
    fn test_precomputed_basepoint_mult() {
        let aB_1 = ED25519_BASEPOINT_TABLE * &A_SCALAR;
        let aB_2 = constants::ED25519_BASEPOINT_POINT * A_SCALAR;
        assert_eq!(aB_1.compress(), aB_2.compress());
    }

    /// Test scalar_mul versus a known scalar multiple from ed25519.py
    #[test]
    fn scalar_mul_vs_ed25519py() {
        let aB = constants::ED25519_BASEPOINT_POINT * A_SCALAR;
        assert_eq!(aB.compress(), A_TIMES_BASEPOINT);
    }

    /// Test basepoint.double() versus the 2*basepoint constant.
    #[test]
    fn basepoint_double_vs_basepoint2() {
        assert_eq!(
            constants::ED25519_BASEPOINT_POINT.double().compress(),
            BASE2_CMPRSSD
        );
    }

    /// Test that computing 2*basepoint is the same as basepoint.double()
    #[test]
    fn basepoint_mult_two_vs_basepoint2() {
        let two = Scalar::from(2u64);
        let bp2 = EdwardsPoint::mul_base(&two);
        assert_eq!(bp2.compress(), BASE2_CMPRSSD);
    }

    /// Test that all the basepoint table types compute the same results.
    #[cfg(feature = "precomputed-tables")]
    #[test]
    fn basepoint_tables() {
        let P = &constants::ED25519_BASEPOINT_POINT;
        let a = A_SCALAR;

        let table_radix16 = EdwardsBasepointTableRadix16::create(P);
        let table_radix32 = EdwardsBasepointTableRadix32::create(P);
        let table_radix64 = EdwardsBasepointTableRadix64::create(P);
        let table_radix128 = EdwardsBasepointTableRadix128::create(P);
        let table_radix256 = EdwardsBasepointTableRadix256::create(P);

        let aP = (ED25519_BASEPOINT_TABLE * &a).compress();
        let aP16 = (&table_radix16 * &a).compress();
        let aP32 = (&table_radix32 * &a).compress();
        let aP64 = (&table_radix64 * &a).compress();
        let aP128 = (&table_radix128 * &a).compress();
        let aP256 = (&table_radix256 * &a).compress();

        assert_eq!(aP, aP16);
        assert_eq!(aP16, aP32);
        assert_eq!(aP32, aP64);
        assert_eq!(aP64, aP128);
        assert_eq!(aP128, aP256);
    }

    /// Check unreduced scalar multiplication by the basepoint tables is the same no matter what
    /// radix the table is.
    #[cfg(feature = "precomputed-tables")]
    #[test]
    fn basepoint_tables_unreduced_scalar() {
        let P = &constants::ED25519_BASEPOINT_POINT;
        let a = crate::scalar::test::LARGEST_UNREDUCED_SCALAR;

        let table_radix16 = EdwardsBasepointTableRadix16::create(P);
        let table_radix32 = EdwardsBasepointTableRadix32::create(P);
        let table_radix64 = EdwardsBasepointTableRadix64::create(P);
        let table_radix128 = EdwardsBasepointTableRadix128::create(P);
        let table_radix256 = EdwardsBasepointTableRadix256::create(P);

        let aP = (ED25519_BASEPOINT_TABLE * &a).compress();
        let aP16 = (&table_radix16 * &a).compress();
        let aP32 = (&table_radix32 * &a).compress();
        let aP64 = (&table_radix64 * &a).compress();
        let aP128 = (&table_radix128 * &a).compress();
        let aP256 = (&table_radix256 * &a).compress();

        assert_eq!(aP, aP16);
        assert_eq!(aP16, aP32);
        assert_eq!(aP32, aP64);
        assert_eq!(aP64, aP128);
        assert_eq!(aP128, aP256);
    }

    /// Check that converting to projective and then back to extended round-trips.
    #[test]
    fn basepoint_projective_extended_round_trip() {
        assert_eq!(
            constants::ED25519_BASEPOINT_POINT
                .as_projective()
                .as_extended()
                .compress(),
            constants::ED25519_BASEPOINT_COMPRESSED
        );
    }

    /// Test computing 16*basepoint vs mul_by_pow_2(4)
    #[test]
    fn basepoint16_vs_mul_by_pow_2_4() {
        let bp16 = constants::ED25519_BASEPOINT_POINT.mul_by_pow_2(4);
        assert_eq!(bp16.compress(), BASE16_CMPRSSD);
    }

    /// Check that mul_base_clamped and mul_clamped agree
    #[test]
    fn mul_base_clamped() {
        let mut csprng = rand_core::OsRng;

        // Make a random curve point in the curve. Give it torsion to make things interesting.
        #[cfg(feature = "precomputed-tables")]
        let random_point = {
            let mut b = [0u8; 32];
            csprng.fill_bytes(&mut b);
            EdwardsPoint::mul_base_clamped(b) + constants::EIGHT_TORSION[1]
        };
        // Make a basepoint table from the random point. We'll use this with mul_base_clamped
        #[cfg(feature = "precomputed-tables")]
        let random_table = EdwardsBasepointTableRadix256::create(&random_point);

        // Now test scalar mult. agreement on the default basepoint as well as random_point

        // Test that mul_base_clamped and mul_clamped agree on a large integer. Even after
        // clamping, this integer is not reduced mod l.
        let a_bytes = [0xff; 32];
        assert_eq!(
            EdwardsPoint::mul_base_clamped(a_bytes),
            constants::ED25519_BASEPOINT_POINT.mul_clamped(a_bytes)
        );
        #[cfg(feature = "precomputed-tables")]
        assert_eq!(
            random_table.mul_base_clamped(a_bytes),
            random_point.mul_clamped(a_bytes)
        );

        // Test agreement on random integers
        for _ in 0..100 {
            // This will be reduced mod l with probability l / 2^256 ≈ 6.25%
            let mut a_bytes = [0u8; 32];
            csprng.fill_bytes(&mut a_bytes);

            assert_eq!(
                EdwardsPoint::mul_base_clamped(a_bytes),
                constants::ED25519_BASEPOINT_POINT.mul_clamped(a_bytes)
            );
            #[cfg(feature = "precomputed-tables")]
            assert_eq!(
                random_table.mul_base_clamped(a_bytes),
                random_point.mul_clamped(a_bytes)
            );
        }
    }

    #[test]
    #[cfg(feature = "alloc")]
    fn impl_sum() {
        // Test that sum works for non-empty iterators
        let BASE = constants::ED25519_BASEPOINT_POINT;

        let s1 = Scalar::from(999u64);
        let P1 = BASE * s1;

        let s2 = Scalar::from(333u64);
        let P2 = BASE * s2;

        let vec = vec![P1, P2];
        let sum: EdwardsPoint = vec.iter().sum();

        assert_eq!(sum, P1 + P2);

        // Test that sum works for the empty iterator
        let empty_vector: Vec<EdwardsPoint> = vec![];
        let sum: EdwardsPoint = empty_vector.iter().sum();

        assert_eq!(sum, EdwardsPoint::identity());

        // Test that sum works on owning iterators
        let s = Scalar::from(2u64);
        let mapped = vec.iter().map(|x| x * s);
        let sum: EdwardsPoint = mapped.sum();

        assert_eq!(sum, P1 * s + P2 * s);
    }

    /// Test that the conditional assignment trait works for AffineNielsPoints.
    #[test]
    fn conditional_assign_for_affine_niels_point() {
        let id = AffineNielsPoint::identity();
        let mut p1 = AffineNielsPoint::identity();
        let bp = constants::ED25519_BASEPOINT_POINT.as_affine_niels();

        p1.conditional_assign(&bp, Choice::from(0));
        assert_eq!(p1, id);
        p1.conditional_assign(&bp, Choice::from(1));
        assert_eq!(p1, bp);
    }

    #[test]
    fn is_small_order() {
        // The basepoint has large prime order
        assert!(!constants::ED25519_BASEPOINT_POINT.is_small_order());
        // constants::EIGHT_TORSION has all points of small order.
        for torsion_point in &constants::EIGHT_TORSION {
            assert!(torsion_point.is_small_order());
        }
    }

    #[test]
    fn compressed_identity() {
        assert_eq!(
            EdwardsPoint::identity().compress(),
            CompressedEdwardsY::identity()
        );
    }

    #[test]
    fn is_identity() {
        assert!(EdwardsPoint::identity().is_identity());
        assert!(!constants::ED25519_BASEPOINT_POINT.is_identity());
    }

    /// Rust's debug builds have overflow and underflow trapping,
    /// and enable `debug_assert!()`.  This performs many scalar
    /// multiplications to attempt to trigger possible overflows etc.
    ///
    /// For instance, the `u64` `Mul` implementation for
    /// `FieldElements` requires the input `Limb`s to be bounded by
    /// 2^54, but we cannot enforce this dynamically at runtime, or
    /// statically at compile time (until Rust gets type-level
    /// integers, at which point we can encode "bits of headroom" into
    /// the type system and prove correctness).
    #[test]
    fn monte_carlo_overflow_underflow_debug_assert_test() {
        let mut P = constants::ED25519_BASEPOINT_POINT;
        // N.B. each scalar_mul does 1407 field mults, 1024 field squarings,
        // so this does ~ 1M of each operation.
        for _ in 0..1_000 {
            P *= &A_SCALAR;
        }
    }

    #[test]
    fn scalarmult_extended_point_works_both_ways() {
        let G: EdwardsPoint = constants::ED25519_BASEPOINT_POINT;
        let s: Scalar = A_SCALAR;

        let P1 = G * s;
        let P2 = s * G;

        assert!(P1.compress().to_bytes() == P2.compress().to_bytes());
    }

    // A single iteration of a consistency check for MSM.
    #[cfg(feature = "alloc")]
    fn multiscalar_consistency_iter(n: usize) {
        let mut rng = rand::thread_rng();

        // Construct random coefficients x0, ..., x_{n-1},
        // followed by some extra hardcoded ones.
        let xs = (0..n).map(|_| Scalar::random(&mut rng)).collect::<Vec<_>>();
        let check = xs.iter().map(|xi| xi * xi).sum::<Scalar>();

        // Construct points G_i = x_i * B
        let Gs = xs.iter().map(EdwardsPoint::mul_base).collect::<Vec<_>>();

        // Compute H1 = <xs, Gs> (consttime)
        let H1 = EdwardsPoint::multiscalar_mul(&xs, &Gs);
        // Compute H2 = <xs, Gs> (vartime)
        let H2 = EdwardsPoint::vartime_multiscalar_mul(&xs, &Gs);
        // Compute H3 = <xs, Gs> = sum(xi^2) * B
        let H3 = EdwardsPoint::mul_base(&check);

        assert_eq!(H1, H3);
        assert_eq!(H2, H3);
    }

    // Use different multiscalar sizes to hit different internal
    // parameters.

    #[test]
    #[cfg(feature = "alloc")]
    fn multiscalar_consistency_n_100() {
        let iters = 50;
        for _ in 0..iters {
            multiscalar_consistency_iter(100);
        }
    }

    #[test]
    #[cfg(feature = "alloc")]
    fn multiscalar_consistency_n_250() {
        let iters = 50;
        for _ in 0..iters {
            multiscalar_consistency_iter(250);
        }
    }

    #[test]
    #[cfg(feature = "alloc")]
    fn multiscalar_consistency_n_500() {
        let iters = 50;
        for _ in 0..iters {
            multiscalar_consistency_iter(500);
        }
    }

    #[test]
    #[cfg(feature = "alloc")]
    fn multiscalar_consistency_n_1000() {
        let iters = 50;
        for _ in 0..iters {
            multiscalar_consistency_iter(1000);
        }
    }

    #[test]
    #[cfg(feature = "alloc")]
    fn vartime_precomputed_vs_nonprecomputed_multiscalar() {
        let mut rng = rand::thread_rng();

        let static_scalars = (0..128)
            .map(|_| Scalar::random(&mut rng))
            .collect::<Vec<_>>();

        let dynamic_scalars = (0..128)
            .map(|_| Scalar::random(&mut rng))
            .collect::<Vec<_>>();

        let check_scalar: Scalar = static_scalars
            .iter()
            .chain(dynamic_scalars.iter())
            .map(|s| s * s)
            .sum();

        let static_points = static_scalars
            .iter()
            .map(EdwardsPoint::mul_base)
            .collect::<Vec<_>>();
        let dynamic_points = dynamic_scalars
            .iter()
            .map(EdwardsPoint::mul_base)
            .collect::<Vec<_>>();

        let precomputation = VartimeEdwardsPrecomputation::new(static_points.iter());

        let P = precomputation.vartime_mixed_multiscalar_mul(
            &static_scalars,
            &dynamic_scalars,
            &dynamic_points,
        );

        use crate::traits::VartimeMultiscalarMul;
        let Q = EdwardsPoint::vartime_multiscalar_mul(
            static_scalars.iter().chain(dynamic_scalars.iter()),
            static_points.iter().chain(dynamic_points.iter()),
        );

        let R = EdwardsPoint::mul_base(&check_scalar);

        assert_eq!(P.compress(), R.compress());
        assert_eq!(Q.compress(), R.compress());
    }

    mod vartime {
        use super::super::*;
        use super::{A_SCALAR, A_TIMES_BASEPOINT, B_SCALAR, DOUBLE_SCALAR_MULT_RESULT};

        /// Test double_scalar_mul_vartime vs ed25519.py
        #[test]
        fn double_scalar_mul_basepoint_vs_ed25519py() {
            let A = A_TIMES_BASEPOINT.decompress().unwrap();
            let result =
                EdwardsPoint::vartime_double_scalar_mul_basepoint(&A_SCALAR, &A, &B_SCALAR);
            assert_eq!(result.compress(), DOUBLE_SCALAR_MULT_RESULT);
        }

        #[test]
        #[cfg(feature = "alloc")]
        fn multiscalar_mul_vs_ed25519py() {
            let A = A_TIMES_BASEPOINT.decompress().unwrap();
            let result = EdwardsPoint::vartime_multiscalar_mul(
                &[A_SCALAR, B_SCALAR],
                &[A, constants::ED25519_BASEPOINT_POINT],
            );
            assert_eq!(result.compress(), DOUBLE_SCALAR_MULT_RESULT);
        }

        #[test]
        #[cfg(feature = "alloc")]
        fn multiscalar_mul_vartime_vs_consttime() {
            let A = A_TIMES_BASEPOINT.decompress().unwrap();
            let result_vartime = EdwardsPoint::vartime_multiscalar_mul(
                &[A_SCALAR, B_SCALAR],
                &[A, constants::ED25519_BASEPOINT_POINT],
            );
            let result_consttime = EdwardsPoint::multiscalar_mul(
                &[A_SCALAR, B_SCALAR],
                &[A, constants::ED25519_BASEPOINT_POINT],
            );

            assert_eq!(result_vartime.compress(), result_consttime.compress());
        }
    }

    #[test]
    #[cfg(feature = "serde")]
    fn serde_bincode_basepoint_roundtrip() {
        use bincode;

        let encoded = bincode::serialize(&constants::ED25519_BASEPOINT_POINT).unwrap();
        let enc_compressed = bincode::serialize(&constants::ED25519_BASEPOINT_COMPRESSED).unwrap();
        assert_eq!(encoded, enc_compressed);

        // Check that the encoding is 32 bytes exactly
        assert_eq!(encoded.len(), 32);

        let dec_uncompressed: EdwardsPoint = bincode::deserialize(&encoded).unwrap();
        let dec_compressed: CompressedEdwardsY = bincode::deserialize(&encoded).unwrap();

        assert_eq!(dec_uncompressed, constants::ED25519_BASEPOINT_POINT);
        assert_eq!(dec_compressed, constants::ED25519_BASEPOINT_COMPRESSED);

        // Check that the encoding itself matches the usual one
        let raw_bytes = constants::ED25519_BASEPOINT_COMPRESSED.as_bytes();
        let bp: EdwardsPoint = bincode::deserialize(raw_bytes).unwrap();
        assert_eq!(bp, constants::ED25519_BASEPOINT_POINT);
    }

    ////////////////////////////////////////////////////////////
    // Signal tests from                                      //
    //     https://github.com/signalapp/libsignal-protocol-c/ //
    ////////////////////////////////////////////////////////////

    #[cfg(all(feature = "alloc", feature = "digest"))]
    fn test_vectors() -> Vec<Vec<&'static str>> {
        vec![
            vec![
                "214f306e1576f5a7577636fe303ca2c625b533319f52442b22a9fa3b7ede809f",
                "c95becf0f93595174633b9d4d6bbbeb88e16fa257176f877ce426e1424626052",
            ],
            vec![
                "2eb10d432702ea7f79207da95d206f82d5a3b374f5f89f17a199531f78d3bea6",
                "d8f8b508edffbb8b6dab0f602f86a9dd759f800fe18f782fdcac47c234883e7f",
            ],
            vec![
                "84cbe9accdd32b46f4a8ef51c85fd39d028711f77fb00e204a613fc235fd68b9",
                "93c73e0289afd1d1fc9e4e78a505d5d1b2642fbdf91a1eff7d281930654b1453",
            ],
            vec![
                "c85165952490dc1839cb69012a3d9f2cc4b02343613263ab93a26dc89fd58267",
                "43cbe8685fd3c90665b91835debb89ff1477f906f5170f38a192f6a199556537",
            ],
            vec![
                "26e7fc4a78d863b1a4ccb2ce0951fbcd021e106350730ee4157bacb4502e1b76",
                "b6fc3d738c2c40719479b2f23818180cdafa72a14254d4016bbed8f0b788a835",
            ],
            vec![
                "1618c08ef0233f94f0f163f9435ec7457cd7a8cd4bb6b160315d15818c30f7a2",
                "da0b703593b29dbcd28ebd6e7baea17b6f61971f3641cae774f6a5137a12294c",
            ],
            vec![
                "48b73039db6fcdcb6030c4a38e8be80b6390d8ae46890e77e623f87254ef149c",
                "ca11b25acbc80566603eabeb9364ebd50e0306424c61049e1ce9385d9f349966",
            ],
            vec![
                "a744d582b3a34d14d311b7629da06d003045ae77cebceeb4e0e72734d63bd07d",
                "fad25a5ea15d4541258af8785acaf697a886c1b872c793790e60a6837b1adbc0",
            ],
            vec![
                "80a6ff33494c471c5eff7efb9febfbcf30a946fe6535b3451cda79f2154a7095",
                "57ac03913309b3f8cd3c3d4c49d878bb21f4d97dc74a1eaccbe5c601f7f06f47",
            ],
            vec![
                "f06fc939bc10551a0fd415aebf107ef0b9c4ee1ef9a164157bdd089127782617",
                "785b2a6a00a5579cc9da1ff997ce8339b6f9fb46c6f10cf7a12ff2986341a6e0",
            ],
        ]
    }

    #[test]
    #[allow(deprecated)]
    #[cfg(all(feature = "alloc", feature = "digest"))]
    fn elligator_signal_test_vectors() {
        for vector in test_vectors().iter() {
            let input = hex::decode(vector[0]).unwrap();
            let output = hex::decode(vector[1]).unwrap();

            let point = EdwardsPoint::nonspec_map_to_curve::<sha2::Sha512>(&input);
            assert_eq!(point.compress().to_bytes(), output[..]);
        }
    }
}
