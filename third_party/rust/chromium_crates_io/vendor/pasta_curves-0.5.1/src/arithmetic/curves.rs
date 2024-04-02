//! This module contains the `Curve`/`CurveAffine` abstractions that allow us to
//! write code that generalizes over a pair of groups.

#[cfg(feature = "alloc")]
use group::prime::{PrimeCurve, PrimeCurveAffine};
#[cfg(feature = "alloc")]
use subtle::{Choice, ConditionallySelectable, ConstantTimeEq, CtOption};

#[cfg(feature = "alloc")]
use alloc::boxed::Box;
#[cfg(feature = "alloc")]
use core::ops::{Add, Mul, Sub};

/// This trait is a common interface for dealing with elements of an elliptic
/// curve group in a "projective" form, where that arithmetic is usually more
/// efficient.
///
/// Requires the `alloc` feature flag because of `hash_to_curve`.
#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
pub trait CurveExt:
    PrimeCurve<Affine = <Self as CurveExt>::AffineExt>
    + group::Group<Scalar = <Self as CurveExt>::ScalarExt>
    + Default
    + ConditionallySelectable
    + ConstantTimeEq
    + From<<Self as PrimeCurve>::Affine>
{
    /// The scalar field of this elliptic curve.
    type ScalarExt: ff::WithSmallOrderMulGroup<3>;
    /// The base field over which this elliptic curve is constructed.
    type Base: ff::WithSmallOrderMulGroup<3>;
    /// The affine version of the curve
    type AffineExt: CurveAffine<CurveExt = Self, ScalarExt = <Self as CurveExt>::ScalarExt>
        + Mul<Self::ScalarExt, Output = Self>
        + for<'r> Mul<Self::ScalarExt, Output = Self>;

    /// CURVE_ID used for hash-to-curve.
    const CURVE_ID: &'static str;

    /// Apply the curve endomorphism by multiplying the x-coordinate
    /// by an element of multiplicative order 3.
    fn endo(&self) -> Self;

    /// Return the Jacobian coordinates of this point.
    fn jacobian_coordinates(&self) -> (Self::Base, Self::Base, Self::Base);

    /// Requests a hasher that accepts messages and returns near-uniformly
    /// distributed elements in the group, given domain prefix `domain_prefix`.
    ///
    /// This method is suitable for use as a random oracle.
    ///
    /// # Example
    ///
    /// ```
    /// use pasta_curves::arithmetic::CurveExt;
    /// fn pedersen_commitment<C: CurveExt>(
    ///     x: C::ScalarExt,
    ///     r: C::ScalarExt,
    /// ) -> C::Affine {
    ///     let hasher = C::hash_to_curve("z.cash:example_pedersen_commitment");
    ///     let g = hasher(b"g");
    ///     let h = hasher(b"h");
    ///     (g * x + &(h * r)).to_affine()
    /// }
    /// ```
    fn hash_to_curve<'a>(domain_prefix: &'a str) -> Box<dyn Fn(&[u8]) -> Self + 'a>;

    /// Returns whether or not this element is on the curve; should
    /// always be true unless an "unchecked" API was used.
    fn is_on_curve(&self) -> Choice;

    /// Returns the curve constant a.
    fn a() -> Self::Base;

    /// Returns the curve constant b.
    fn b() -> Self::Base;

    /// Obtains a point given Jacobian coordinates $X : Y : Z$, failing
    /// if the coordinates are not on the curve.
    fn new_jacobian(x: Self::Base, y: Self::Base, z: Self::Base) -> CtOption<Self>;
}

/// This trait is the affine counterpart to `Curve` and is used for
/// serialization, storage in memory, and inspection of $x$ and $y$ coordinates.
///
/// Requires the `alloc` feature flag because of `hash_to_curve` on [`CurveExt`].
#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
pub trait CurveAffine:
    PrimeCurveAffine<
        Scalar = <Self as CurveAffine>::ScalarExt,
        Curve = <Self as CurveAffine>::CurveExt,
    > + Default
    + Add<Output = <Self as PrimeCurveAffine>::Curve>
    + Sub<Output = <Self as PrimeCurveAffine>::Curve>
    + ConditionallySelectable
    + ConstantTimeEq
    + From<<Self as PrimeCurveAffine>::Curve>
{
    /// The scalar field of this elliptic curve.
    type ScalarExt: ff::WithSmallOrderMulGroup<3> + Ord;
    /// The base field over which this elliptic curve is constructed.
    type Base: ff::WithSmallOrderMulGroup<3> + Ord;
    /// The projective form of the curve
    type CurveExt: CurveExt<AffineExt = Self, ScalarExt = <Self as CurveAffine>::ScalarExt>;

    /// Gets the coordinates of this point.
    ///
    /// Returns None if this is the identity.
    fn coordinates(&self) -> CtOption<Coordinates<Self>>;

    /// Obtains a point given $(x, y)$, failing if it is not on the
    /// curve.
    fn from_xy(x: Self::Base, y: Self::Base) -> CtOption<Self>;

    /// Returns whether or not this element is on the curve; should
    /// always be true unless an "unchecked" API was used.
    fn is_on_curve(&self) -> Choice;

    /// Returns the curve constant $a$.
    fn a() -> Self::Base;

    /// Returns the curve constant $b$.
    fn b() -> Self::Base;
}

/// The affine coordinates of a point on an elliptic curve.
#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
#[derive(Clone, Copy, Debug, Default)]
pub struct Coordinates<C: CurveAffine> {
    pub(crate) x: C::Base,
    pub(crate) y: C::Base,
}

#[cfg(feature = "alloc")]
impl<C: CurveAffine> Coordinates<C> {
    /// Obtains a `Coordinates` value given $(x, y)$, failing if it is not on the curve.
    pub fn from_xy(x: C::Base, y: C::Base) -> CtOption<Self> {
        // We use CurveAffine::from_xy to validate the coordinates.
        C::from_xy(x, y).map(|_| Coordinates { x, y })
    }
    /// Returns the x-coordinate.
    ///
    /// Equivalent to `Coordinates::u`.
    pub fn x(&self) -> &C::Base {
        &self.x
    }

    /// Returns the y-coordinate.
    ///
    /// Equivalent to `Coordinates::v`.
    pub fn y(&self) -> &C::Base {
        &self.y
    }

    /// Returns the u-coordinate.
    ///
    /// Equivalent to `Coordinates::x`.
    pub fn u(&self) -> &C::Base {
        &self.x
    }

    /// Returns the v-coordinate.
    ///
    /// Equivalent to `Coordinates::y`.
    pub fn v(&self) -> &C::Base {
        &self.y
    }
}

#[cfg(feature = "alloc")]
impl<C: CurveAffine> ConditionallySelectable for Coordinates<C> {
    fn conditional_select(a: &Self, b: &Self, choice: Choice) -> Self {
        Coordinates {
            x: C::Base::conditional_select(&a.x, &b.x, choice),
            y: C::Base::conditional_select(&a.y, &b.y, choice),
        }
    }
}
