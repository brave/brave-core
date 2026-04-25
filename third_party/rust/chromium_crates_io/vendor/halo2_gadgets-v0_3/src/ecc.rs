//! Elliptic curve operations.

use std::fmt::Debug;

use halo2_proofs::{
    arithmetic::CurveAffine,
    circuit::{Chip, Layouter, Value},
    plonk::Error,
};

use crate::utilities::UtilitiesInstructions;

pub mod chip;

/// The set of circuit instructions required to use the ECC gadgets.
pub trait EccInstructions<C: CurveAffine>:
    Chip<C::Base> + UtilitiesInstructions<C::Base> + Clone + Debug + Eq
{
    /// Variable representing a scalar used in variable-base scalar mul.
    ///
    /// This type is treated as a full-width scalar. However, if `Self` implements
    /// [`BaseFitsInScalarInstructions`] then this may also be constructed from an element
    /// of the base field.
    type ScalarVar: Clone + Debug;
    /// Variable representing a full-width element of the elliptic curve's
    /// scalar field, to be used for fixed-base scalar mul.
    type ScalarFixed: Clone + Debug;
    /// Variable representing a signed short element of the elliptic curve's
    /// scalar field, to be used for fixed-base scalar mul.
    ///
    /// A `ScalarFixedShort` must be in the range [-(2^64 - 1), 2^64 - 1].
    type ScalarFixedShort: Clone + Debug;
    /// Variable representing an elliptic curve point.
    type Point: From<Self::NonIdentityPoint> + Clone + Debug;
    /// Variable representing a non-identity elliptic curve point.
    type NonIdentityPoint: Clone + Debug;
    /// Variable representing the affine short Weierstrass x-coordinate of an
    /// elliptic curve point.
    type X: Clone + Debug;
    /// Enumeration of the set of fixed bases to be used in scalar mul.
    /// TODO: When associated consts can be used as const generics, introduce
    /// `Self::NUM_WINDOWS`, `Self::NUM_WINDOWS_BASE_FIELD`, `Self::NUM_WINDOWS_SHORT`
    /// and use them to differentiate `FixedPoints` types.
    type FixedPoints: FixedPoints<C>;

    /// Constrains point `a` to be equal in value to point `b`.
    fn constrain_equal(
        &self,
        layouter: &mut impl Layouter<C::Base>,
        a: &Self::Point,
        b: &Self::Point,
    ) -> Result<(), Error>;

    /// Witnesses the given point as a private input to the circuit.
    /// This allows the point to be the identity, mapped to (0, 0) in
    /// affine coordinates.
    fn witness_point(
        &self,
        layouter: &mut impl Layouter<C::Base>,
        value: Value<C>,
    ) -> Result<Self::Point, Error>;

    /// Witnesses the given point as a private input to the circuit.
    /// This returns an error if the point is the identity.
    fn witness_point_non_id(
        &self,
        layouter: &mut impl Layouter<C::Base>,
        value: Value<C>,
    ) -> Result<Self::NonIdentityPoint, Error>;

    /// Witnesses a full-width scalar to be used in variable-base multiplication.
    fn witness_scalar_var(
        &self,
        layouter: &mut impl Layouter<C::Base>,
        value: Value<C::Scalar>,
    ) -> Result<Self::ScalarVar, Error>;

    /// Witnesses a full-width scalar to be used in fixed-base multiplication.
    fn witness_scalar_fixed(
        &self,
        layouter: &mut impl Layouter<C::Base>,
        value: Value<C::Scalar>,
    ) -> Result<Self::ScalarFixed, Error>;

    /// Converts a magnitude and sign that exists as variables in the circuit into a
    /// signed short scalar to be used in fixed-base scalar multiplication.
    fn scalar_fixed_from_signed_short(
        &self,
        layouter: &mut impl Layouter<C::Base>,
        magnitude_sign: (Self::Var, Self::Var),
    ) -> Result<Self::ScalarFixedShort, Error>;

    /// Extracts the x-coordinate of a point.
    fn extract_p<Point: Into<Self::Point> + Clone>(point: &Point) -> Self::X;

    /// Performs incomplete point addition, returning `a + b`.
    ///
    /// This returns an error in exceptional cases.
    fn add_incomplete(
        &self,
        layouter: &mut impl Layouter<C::Base>,
        a: &Self::NonIdentityPoint,
        b: &Self::NonIdentityPoint,
    ) -> Result<Self::NonIdentityPoint, Error>;

    /// Performs complete point addition, returning `a + b`.
    fn add<A: Into<Self::Point> + Clone, B: Into<Self::Point> + Clone>(
        &self,
        layouter: &mut impl Layouter<C::Base>,
        a: &A,
        b: &B,
    ) -> Result<Self::Point, Error>;

    /// Performs variable-base scalar multiplication, returning `[scalar] base`.
    fn mul(
        &self,
        layouter: &mut impl Layouter<C::Base>,
        scalar: &Self::ScalarVar,
        base: &Self::NonIdentityPoint,
    ) -> Result<(Self::Point, Self::ScalarVar), Error>;

    /// Performs fixed-base scalar multiplication using a full-width scalar, returning `[scalar] base`.
    fn mul_fixed(
        &self,
        layouter: &mut impl Layouter<C::Base>,
        scalar: &Self::ScalarFixed,
        base: &<Self::FixedPoints as FixedPoints<C>>::FullScalar,
    ) -> Result<(Self::Point, Self::ScalarFixed), Error>;

    /// Performs fixed-base scalar multiplication using a short signed scalar, returning
    /// `[scalar] base`.
    fn mul_fixed_short(
        &self,
        layouter: &mut impl Layouter<C::Base>,
        scalar: &Self::ScalarFixedShort,
        base: &<Self::FixedPoints as FixedPoints<C>>::ShortScalar,
    ) -> Result<(Self::Point, Self::ScalarFixedShort), Error>;

    /// Performs fixed-base scalar multiplication using a base field element as the scalar.
    /// In the current implementation, this base field element must be output from another
    /// instruction.
    fn mul_fixed_base_field_elem(
        &self,
        layouter: &mut impl Layouter<C::Base>,
        base_field_elem: Self::Var,
        base: &<Self::FixedPoints as FixedPoints<C>>::Base,
    ) -> Result<Self::Point, Error>;
}

/// Instructions that can be implemented for a curve whose base field fits into
/// its scalar field.
pub trait BaseFitsInScalarInstructions<C: CurveAffine>: EccInstructions<C> {
    /// Converts a base field element that exists as a variable in the circuit
    /// into a scalar to be used in variable-base scalar multiplication.
    fn scalar_var_from_base(
        &self,
        layouter: &mut impl Layouter<C::Base>,
        base: &Self::Var,
    ) -> Result<Self::ScalarVar, Error>;
}

/// Defines the fixed points for a given instantiation of the ECC chip.
pub trait FixedPoints<C: CurveAffine>: Debug + Eq + Clone {
    /// Fixed points that can be used with full-width scalar multiplication.
    type FullScalar: Debug + Eq + Clone;
    /// Fixed points that can be used with short scalar multiplication.
    type ShortScalar: Debug + Eq + Clone;
    /// Fixed points that can be multiplied by base field elements.
    type Base: Debug + Eq + Clone;
}

/// An integer representing an element of the scalar field for a specific elliptic curve.
#[derive(Debug)]
pub struct ScalarVar<C: CurveAffine, EccChip: EccInstructions<C>> {
    chip: EccChip,
    inner: EccChip::ScalarVar,
}

impl<C: CurveAffine, EccChip: EccInstructions<C>> ScalarVar<C, EccChip> {
    /// Witnesses the given full-width scalar.
    ///
    /// Depending on the `EccChip` implementation, this may either witness the scalar
    /// immediately, or delay witnessing until its first use in [`NonIdentityPoint::mul`].
    pub fn new(
        chip: EccChip,
        mut layouter: impl Layouter<C::Base>,
        value: Value<C::Scalar>,
    ) -> Result<Self, Error> {
        let scalar = chip.witness_scalar_var(&mut layouter, value);
        scalar.map(|inner| ScalarVar { chip, inner })
    }
}

impl<C: CurveAffine, EccChip: BaseFitsInScalarInstructions<C>> ScalarVar<C, EccChip> {
    /// Constructs a scalar from an existing base-field element.
    pub fn from_base(
        chip: EccChip,
        mut layouter: impl Layouter<C::Base>,
        base: &EccChip::Var,
    ) -> Result<Self, Error> {
        let scalar = chip.scalar_var_from_base(&mut layouter, base);
        scalar.map(|inner| ScalarVar { chip, inner })
    }
}

/// An integer representing an element of the scalar field for a specific elliptic curve,
/// for [`FixedPoint`] scalar multiplication.
#[derive(Debug)]
pub struct ScalarFixed<C: CurveAffine, EccChip: EccInstructions<C>> {
    chip: EccChip,
    inner: EccChip::ScalarFixed,
}

impl<C: CurveAffine, EccChip: EccInstructions<C>> ScalarFixed<C, EccChip> {
    /// Witnesses the given full-width scalar.
    ///
    /// Depending on the `EccChip` implementation, this may either witness the scalar
    /// immediately, or delay witnessing until its first use in [`FixedPoint::mul`].
    pub fn new(
        chip: EccChip,
        mut layouter: impl Layouter<C::Base>,
        value: Value<C::Scalar>,
    ) -> Result<Self, Error> {
        let scalar = chip.witness_scalar_fixed(&mut layouter, value);
        scalar.map(|inner| ScalarFixed { chip, inner })
    }
}

/// A signed short (64-bit) integer represented as an element of the scalar field for a
/// specific elliptic curve, to be used for [`FixedPointShort`] scalar multiplication.
#[derive(Debug)]
pub struct ScalarFixedShort<C: CurveAffine, EccChip: EccInstructions<C>> {
    chip: EccChip,
    inner: EccChip::ScalarFixedShort,
}

impl<C: CurveAffine, EccChip: EccInstructions<C>> ScalarFixedShort<C, EccChip> {
    /// Converts the given signed short scalar.
    ///
    /// `magnitude_sign` must be a tuple of two circuit-assigned values:
    /// - An unsigned integer of at most 64 bits.
    /// - A sign value that is either 1 or -1.
    ///
    /// Depending on the `EccChip` implementation, the scalar may either be constrained
    /// immediately by this constructor, or lazily constrained when it is first used in
    /// [`FixedPointShort::mul`].
    pub fn new(
        chip: EccChip,
        mut layouter: impl Layouter<C::Base>,
        magnitude_sign: (EccChip::Var, EccChip::Var),
    ) -> Result<Self, Error> {
        let scalar = chip.scalar_fixed_from_signed_short(&mut layouter, magnitude_sign);
        scalar.map(|inner| ScalarFixedShort { chip, inner })
    }
}

/// A point on a specific elliptic curve that is guaranteed to not be the identity.
#[derive(Copy, Clone, Debug)]
pub struct NonIdentityPoint<C: CurveAffine, EccChip: EccInstructions<C>> {
    chip: EccChip,
    inner: EccChip::NonIdentityPoint,
}

impl<C: CurveAffine, EccChip: EccInstructions<C>> NonIdentityPoint<C, EccChip> {
    /// Constructs a new point with the given value.
    pub fn new(
        chip: EccChip,
        mut layouter: impl Layouter<C::Base>,
        value: Value<C>,
    ) -> Result<Self, Error> {
        let point = chip.witness_point_non_id(&mut layouter, value);
        point.map(|inner| NonIdentityPoint { chip, inner })
    }

    /// Constrains this point to be equal in value to another point.
    pub fn constrain_equal<Other: Into<Point<C, EccChip>> + Clone>(
        &self,
        mut layouter: impl Layouter<C::Base>,
        other: &Other,
    ) -> Result<(), Error> {
        let other: Point<C, EccChip> = (other.clone()).into();
        self.chip.constrain_equal(
            &mut layouter,
            &Point::<C, EccChip>::from(self.clone()).inner,
            &other.inner,
        )
    }

    /// Returns the inner point.
    pub fn inner(&self) -> &EccChip::NonIdentityPoint {
        &self.inner
    }

    /// Extracts the x-coordinate of a point.
    pub fn extract_p(&self) -> X<C, EccChip> {
        X::from_inner(self.chip.clone(), EccChip::extract_p(&self.inner))
    }

    /// Wraps the given point (obtained directly from an instruction) in a gadget.
    pub fn from_inner(chip: EccChip, inner: EccChip::NonIdentityPoint) -> Self {
        NonIdentityPoint { chip, inner }
    }

    /// Returns `self + other` using complete addition.
    pub fn add<Other: Into<Point<C, EccChip>> + Clone>(
        &self,
        mut layouter: impl Layouter<C::Base>,
        other: &Other,
    ) -> Result<Point<C, EccChip>, Error> {
        let other: Point<C, EccChip> = (other.clone()).into();

        assert_eq!(self.chip, other.chip);
        self.chip
            .add(&mut layouter, &self.inner, &other.inner)
            .map(|inner| Point {
                chip: self.chip.clone(),
                inner,
            })
    }

    /// Returns `self + other` using incomplete addition.
    /// The arguments are type-constrained not to be the identity point,
    /// and since exceptional cases return an Error, the result also cannot
    /// be the identity point.
    pub fn add_incomplete(
        &self,
        mut layouter: impl Layouter<C::Base>,
        other: &Self,
    ) -> Result<Self, Error> {
        assert_eq!(self.chip, other.chip);
        self.chip
            .add_incomplete(&mut layouter, &self.inner, &other.inner)
            .map(|inner| NonIdentityPoint {
                chip: self.chip.clone(),
                inner,
            })
    }

    /// Returns `[by] self`.
    #[allow(clippy::type_complexity)]
    pub fn mul(
        &self,
        mut layouter: impl Layouter<C::Base>,
        by: ScalarVar<C, EccChip>,
    ) -> Result<(Point<C, EccChip>, ScalarVar<C, EccChip>), Error> {
        assert_eq!(self.chip, by.chip);
        self.chip
            .mul(&mut layouter, &by.inner, &self.inner.clone())
            .map(|(point, scalar)| {
                (
                    Point {
                        chip: self.chip.clone(),
                        inner: point,
                    },
                    ScalarVar {
                        chip: self.chip.clone(),
                        inner: scalar,
                    },
                )
            })
    }
}

impl<C: CurveAffine, EccChip: EccInstructions<C> + Clone + Debug + Eq>
    From<NonIdentityPoint<C, EccChip>> for Point<C, EccChip>
{
    fn from(non_id_point: NonIdentityPoint<C, EccChip>) -> Self {
        Self {
            chip: non_id_point.chip,
            inner: non_id_point.inner.into(),
        }
    }
}

/// A point on a specific elliptic curve.
#[derive(Copy, Clone, Debug)]
pub struct Point<C: CurveAffine, EccChip: EccInstructions<C> + Clone + Debug + Eq> {
    chip: EccChip,
    inner: EccChip::Point,
}

impl<C: CurveAffine, EccChip: EccInstructions<C> + Clone + Debug + Eq> Point<C, EccChip> {
    /// Constructs a new point with the given value.
    pub fn new(
        chip: EccChip,
        mut layouter: impl Layouter<C::Base>,
        value: Value<C>,
    ) -> Result<Self, Error> {
        let point = chip.witness_point(&mut layouter, value);
        point.map(|inner| Point { chip, inner })
    }

    /// Constrains this point to be equal in value to another point.
    pub fn constrain_equal<Other: Into<Point<C, EccChip>> + Clone>(
        &self,
        mut layouter: impl Layouter<C::Base>,
        other: &Other,
    ) -> Result<(), Error> {
        let other: Point<C, EccChip> = (other.clone()).into();
        self.chip
            .constrain_equal(&mut layouter, &self.inner, &other.inner)
    }

    /// Returns the inner point.
    pub fn inner(&self) -> &EccChip::Point {
        &self.inner
    }

    /// Extracts the x-coordinate of a point.
    pub fn extract_p(&self) -> X<C, EccChip> {
        X::from_inner(self.chip.clone(), EccChip::extract_p(&self.inner))
    }

    /// Wraps the given point (obtained directly from an instruction) in a gadget.
    pub fn from_inner(chip: EccChip, inner: EccChip::Point) -> Self {
        Point { chip, inner }
    }

    /// Returns `self + other` using complete addition.
    pub fn add<Other: Into<Point<C, EccChip>> + Clone>(
        &self,
        mut layouter: impl Layouter<C::Base>,
        other: &Other,
    ) -> Result<Point<C, EccChip>, Error> {
        let other: Point<C, EccChip> = (other.clone()).into();

        assert_eq!(self.chip, other.chip);
        self.chip
            .add(&mut layouter, &self.inner, &other.inner)
            .map(|inner| Point {
                chip: self.chip.clone(),
                inner,
            })
    }
}

/// The affine short Weierstrass x-coordinate of a point on a specific elliptic curve.
#[derive(Debug)]
pub struct X<C: CurveAffine, EccChip: EccInstructions<C>> {
    inner: EccChip::X,
}

impl<C: CurveAffine, EccChip: EccInstructions<C>> X<C, EccChip> {
    /// Wraps the given x-coordinate (obtained directly from an instruction) in a gadget.
    pub fn from_inner(chip: EccChip, inner: EccChip::X) -> Self {
        let _ = chip; // unused
        X { inner }
    }

    /// Returns the inner x-coordinate.
    pub fn inner(&self) -> &EccChip::X {
        &self.inner
    }
}

/// Precomputed multiples of a fixed point, for full-width scalar multiplication.
///
/// Fixing the curve point enables window tables to be baked into the circuit, making
/// scalar multiplication more efficient. These window tables are tuned to full-width
/// scalar multiplication.
#[derive(Clone, Debug)]
pub struct FixedPoint<C: CurveAffine, EccChip: EccInstructions<C>> {
    chip: EccChip,
    inner: <EccChip::FixedPoints as FixedPoints<C>>::FullScalar,
}

/// Precomputed multiples of a fixed point, that can be multiplied by base-field elements.
///
/// Fixing the curve point enables window tables to be baked into the circuit, making
/// scalar multiplication more efficient. These window tables are tuned to scalar
/// multiplication by base-field elements.
#[derive(Clone, Debug)]
pub struct FixedPointBaseField<C: CurveAffine, EccChip: EccInstructions<C>> {
    chip: EccChip,
    inner: <EccChip::FixedPoints as FixedPoints<C>>::Base,
}

/// Precomputed multiples of a fixed point, for short signed scalar multiplication.
#[derive(Clone, Debug)]
pub struct FixedPointShort<C: CurveAffine, EccChip: EccInstructions<C>> {
    chip: EccChip,
    inner: <EccChip::FixedPoints as FixedPoints<C>>::ShortScalar,
}

impl<C: CurveAffine, EccChip: EccInstructions<C>> FixedPoint<C, EccChip> {
    #[allow(clippy::type_complexity)]
    /// Returns `[by] self`.
    pub fn mul(
        &self,
        mut layouter: impl Layouter<C::Base>,
        by: ScalarFixed<C, EccChip>,
    ) -> Result<(Point<C, EccChip>, ScalarFixed<C, EccChip>), Error> {
        assert_eq!(self.chip, by.chip);
        self.chip
            .mul_fixed(&mut layouter, &by.inner, &self.inner)
            .map(|(point, scalar)| {
                (
                    Point {
                        chip: self.chip.clone(),
                        inner: point,
                    },
                    ScalarFixed {
                        chip: self.chip.clone(),
                        inner: scalar,
                    },
                )
            })
    }

    /// Wraps the given fixed base (obtained directly from an instruction) in a gadget.
    pub fn from_inner(
        chip: EccChip,
        inner: <EccChip::FixedPoints as FixedPoints<C>>::FullScalar,
    ) -> Self {
        Self { chip, inner }
    }
}

impl<C: CurveAffine, EccChip: EccInstructions<C>> FixedPointBaseField<C, EccChip> {
    #[allow(clippy::type_complexity)]
    /// Returns `[by] self`.
    pub fn mul(
        &self,
        mut layouter: impl Layouter<C::Base>,
        by: EccChip::Var,
    ) -> Result<Point<C, EccChip>, Error> {
        self.chip
            .mul_fixed_base_field_elem(&mut layouter, by, &self.inner)
            .map(|inner| Point {
                chip: self.chip.clone(),
                inner,
            })
    }

    /// Wraps the given fixed base (obtained directly from an instruction) in a gadget.
    pub fn from_inner(
        chip: EccChip,
        inner: <EccChip::FixedPoints as FixedPoints<C>>::Base,
    ) -> Self {
        Self { chip, inner }
    }
}

impl<C: CurveAffine, EccChip: EccInstructions<C>> FixedPointShort<C, EccChip> {
    #[allow(clippy::type_complexity)]
    /// Returns `[by] self`.
    pub fn mul(
        &self,
        mut layouter: impl Layouter<C::Base>,
        by: ScalarFixedShort<C, EccChip>,
    ) -> Result<(Point<C, EccChip>, ScalarFixedShort<C, EccChip>), Error> {
        assert_eq!(self.chip, by.chip);
        self.chip
            .mul_fixed_short(&mut layouter, &by.inner, &self.inner)
            .map(|(point, scalar)| {
                (
                    Point {
                        chip: self.chip.clone(),
                        inner: point,
                    },
                    ScalarFixedShort {
                        chip: self.chip.clone(),
                        inner: scalar,
                    },
                )
            })
    }

    /// Wraps the given fixed base (obtained directly from an instruction) in a gadget.
    pub fn from_inner(
        chip: EccChip,
        inner: <EccChip::FixedPoints as FixedPoints<C>>::ShortScalar,
    ) -> Self {
        Self { chip, inner }
    }
}

#[cfg(test)]
pub(crate) mod tests {
    use ff::PrimeField;
    use group::{prime::PrimeCurveAffine, Curve, Group};

    use halo2_proofs::{
        circuit::{Layouter, SimpleFloorPlanner, Value},
        dev::MockProver,
        plonk::{Circuit, ConstraintSystem, Error},
    };
    use lazy_static::lazy_static;
    use pasta_curves::pallas;

    use super::{
        chip::{
            find_zs_and_us, BaseFieldElem, EccChip, EccConfig, FixedPoint, FullScalar, ShortScalar,
            H, NUM_WINDOWS, NUM_WINDOWS_SHORT,
        },
        FixedPoints,
    };
    use crate::utilities::lookup_range_check::LookupRangeCheckConfig;

    #[derive(Debug, Eq, PartialEq, Clone)]
    pub(crate) struct TestFixedBases;
    #[derive(Debug, Eq, PartialEq, Clone)]
    pub(crate) struct FullWidth(pallas::Affine, &'static [(u64, [pallas::Base; H])]);
    #[derive(Debug, Eq, PartialEq, Clone)]
    pub(crate) struct BaseField;
    #[derive(Debug, Eq, PartialEq, Clone)]
    pub(crate) struct Short;

    lazy_static! {
        static ref BASE: pallas::Affine = pallas::Point::generator().to_affine();
        static ref ZS_AND_US: Vec<(u64, [pallas::Base; H])> =
            find_zs_and_us(*BASE, NUM_WINDOWS).unwrap();
        static ref ZS_AND_US_SHORT: Vec<(u64, [pallas::Base; H])> =
            find_zs_and_us(*BASE, NUM_WINDOWS_SHORT).unwrap();
    }

    impl FullWidth {
        pub(crate) fn from_pallas_generator() -> Self {
            FullWidth(*BASE, &ZS_AND_US)
        }

        pub(crate) fn from_parts(
            base: pallas::Affine,
            zs_and_us: &'static [(u64, [pallas::Base; H])],
        ) -> Self {
            FullWidth(base, zs_and_us)
        }
    }

    impl FixedPoint<pallas::Affine> for FullWidth {
        type FixedScalarKind = FullScalar;

        fn generator(&self) -> pallas::Affine {
            self.0
        }

        fn u(&self) -> Vec<[[u8; 32]; H]> {
            self.1
                .iter()
                .map(|(_, us)| {
                    [
                        us[0].to_repr(),
                        us[1].to_repr(),
                        us[2].to_repr(),
                        us[3].to_repr(),
                        us[4].to_repr(),
                        us[5].to_repr(),
                        us[6].to_repr(),
                        us[7].to_repr(),
                    ]
                })
                .collect()
        }

        fn z(&self) -> Vec<u64> {
            self.1.iter().map(|(z, _)| *z).collect()
        }
    }

    impl FixedPoint<pallas::Affine> for BaseField {
        type FixedScalarKind = BaseFieldElem;

        fn generator(&self) -> pallas::Affine {
            *BASE
        }

        fn u(&self) -> Vec<[[u8; 32]; H]> {
            ZS_AND_US
                .iter()
                .map(|(_, us)| {
                    [
                        us[0].to_repr(),
                        us[1].to_repr(),
                        us[2].to_repr(),
                        us[3].to_repr(),
                        us[4].to_repr(),
                        us[5].to_repr(),
                        us[6].to_repr(),
                        us[7].to_repr(),
                    ]
                })
                .collect()
        }

        fn z(&self) -> Vec<u64> {
            ZS_AND_US.iter().map(|(z, _)| *z).collect()
        }
    }

    impl FixedPoint<pallas::Affine> for Short {
        type FixedScalarKind = ShortScalar;

        fn generator(&self) -> pallas::Affine {
            *BASE
        }

        fn u(&self) -> Vec<[[u8; 32]; H]> {
            ZS_AND_US_SHORT
                .iter()
                .map(|(_, us)| {
                    [
                        us[0].to_repr(),
                        us[1].to_repr(),
                        us[2].to_repr(),
                        us[3].to_repr(),
                        us[4].to_repr(),
                        us[5].to_repr(),
                        us[6].to_repr(),
                        us[7].to_repr(),
                    ]
                })
                .collect()
        }

        fn z(&self) -> Vec<u64> {
            ZS_AND_US_SHORT.iter().map(|(z, _)| *z).collect()
        }
    }

    impl FixedPoints<pallas::Affine> for TestFixedBases {
        type FullScalar = FullWidth;
        type ShortScalar = Short;
        type Base = BaseField;
    }

    struct MyCircuit {
        test_errors: bool,
    }

    #[allow(non_snake_case)]
    impl Circuit<pallas::Base> for MyCircuit {
        type Config = EccConfig<TestFixedBases>;
        type FloorPlanner = SimpleFloorPlanner;

        fn without_witnesses(&self) -> Self {
            MyCircuit { test_errors: false }
        }

        fn configure(meta: &mut ConstraintSystem<pallas::Base>) -> Self::Config {
            let advices = [
                meta.advice_column(),
                meta.advice_column(),
                meta.advice_column(),
                meta.advice_column(),
                meta.advice_column(),
                meta.advice_column(),
                meta.advice_column(),
                meta.advice_column(),
                meta.advice_column(),
                meta.advice_column(),
            ];
            let lookup_table = meta.lookup_table_column();
            let lagrange_coeffs = [
                meta.fixed_column(),
                meta.fixed_column(),
                meta.fixed_column(),
                meta.fixed_column(),
                meta.fixed_column(),
                meta.fixed_column(),
                meta.fixed_column(),
                meta.fixed_column(),
            ];
            // Shared fixed column for loading constants
            let constants = meta.fixed_column();
            meta.enable_constant(constants);

            let range_check = LookupRangeCheckConfig::configure(meta, advices[9], lookup_table);
            EccChip::<TestFixedBases>::configure(meta, advices, lagrange_coeffs, range_check)
        }

        fn synthesize(
            &self,
            config: Self::Config,
            mut layouter: impl Layouter<pallas::Base>,
        ) -> Result<(), Error> {
            let chip = EccChip::construct(config.clone());

            // Load 10-bit lookup table. In the Action circuit, this will be
            // provided by the Sinsemilla chip.
            config.lookup_config.load(&mut layouter)?;

            // Generate a random non-identity point P
            let p_val = pallas::Point::random(rand::rngs::OsRng).to_affine(); // P
            let p = super::NonIdentityPoint::new(
                chip.clone(),
                layouter.namespace(|| "P"),
                Value::known(p_val),
            )?;
            let p_neg = -p_val;
            let p_neg = super::NonIdentityPoint::new(
                chip.clone(),
                layouter.namespace(|| "-P"),
                Value::known(p_neg),
            )?;

            // Generate a random non-identity point Q
            let q_val = pallas::Point::random(rand::rngs::OsRng).to_affine(); // Q
            let q = super::NonIdentityPoint::new(
                chip.clone(),
                layouter.namespace(|| "Q"),
                Value::known(q_val),
            )?;

            // Make sure P and Q are not the same point.
            assert_ne!(p_val, q_val);

            // Test that we can witness the identity as a point, but not as a non-identity point.
            {
                let _ = super::Point::new(
                    chip.clone(),
                    layouter.namespace(|| "identity"),
                    Value::known(pallas::Affine::identity()),
                )?;

                super::NonIdentityPoint::new(
                    chip.clone(),
                    layouter.namespace(|| "identity"),
                    Value::known(pallas::Affine::identity()),
                )
                .expect_err("Trying to witness the identity should return an error");
            }

            // Test witness non-identity point
            {
                super::chip::witness_point::tests::test_witness_non_id(
                    chip.clone(),
                    layouter.namespace(|| "witness non-identity point"),
                )
            }

            // Test complete addition
            {
                super::chip::add::tests::test_add(
                    chip.clone(),
                    layouter.namespace(|| "complete addition"),
                    p_val,
                    &p,
                    q_val,
                    &q,
                    &p_neg,
                )?;
            }

            // Test incomplete addition
            {
                super::chip::add_incomplete::tests::test_add_incomplete(
                    chip.clone(),
                    layouter.namespace(|| "incomplete addition"),
                    p_val,
                    &p,
                    q_val,
                    &q,
                    &p_neg,
                    self.test_errors,
                )?;
            }

            // Test variable-base scalar multiplication
            {
                super::chip::mul::tests::test_mul(
                    chip.clone(),
                    layouter.namespace(|| "variable-base scalar mul"),
                    &p,
                    p_val,
                )?;
            }

            // Test full-width fixed-base scalar multiplication
            {
                super::chip::mul_fixed::full_width::tests::test_mul_fixed(
                    chip.clone(),
                    layouter.namespace(|| "full-width fixed-base scalar mul"),
                )?;
            }

            // Test signed short fixed-base scalar multiplication
            {
                super::chip::mul_fixed::short::tests::test_mul_fixed_short(
                    chip.clone(),
                    layouter.namespace(|| "signed short fixed-base scalar mul"),
                )?;
            }

            // Test fixed-base scalar multiplication with a base field element
            {
                super::chip::mul_fixed::base_field_elem::tests::test_mul_fixed_base_field(
                    chip,
                    layouter.namespace(|| "fixed-base scalar mul with base field element"),
                )?;
            }

            Ok(())
        }
    }

    #[test]
    fn ecc_chip() {
        let k = 13;
        let circuit = MyCircuit { test_errors: true };
        let prover = MockProver::run(k, &circuit, vec![]).unwrap();
        assert_eq!(prover.verify(), Ok(()))
    }

    #[cfg(feature = "test-dev-graph")]
    #[test]
    fn print_ecc_chip() {
        use plotters::prelude::*;

        let root = BitMapBackend::new("ecc-chip-layout.png", (1024, 7680)).into_drawing_area();
        root.fill(&WHITE).unwrap();
        let root = root.titled("Ecc Chip Layout", ("sans-serif", 60)).unwrap();

        let circuit = MyCircuit { test_errors: false };
        halo2_proofs::dev::CircuitLayout::default()
            .render(13, &circuit, &root)
            .unwrap();
    }
}
