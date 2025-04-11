//! A library for working with pairing-friendly curves.

#![no_std]
// `clippy` is a code linting tool for improving code quality by catching
// common mistakes or strange code patterns. If the `cargo-clippy` feature
// is provided, all compiler warnings are prohibited.
#![cfg_attr(feature = "cargo-clippy", deny(warnings))]
#![cfg_attr(feature = "cargo-clippy", allow(clippy::inline_always))]
#![cfg_attr(feature = "cargo-clippy", allow(clippy::too_many_arguments))]
#![cfg_attr(feature = "cargo-clippy", allow(clippy::unreadable_literal))]
#![cfg_attr(feature = "cargo-clippy", allow(clippy::many_single_char_names))]
#![cfg_attr(feature = "cargo-clippy", allow(clippy::new_without_default))]
#![cfg_attr(feature = "cargo-clippy", allow(clippy::write_literal))]
// Catch documentation errors caused by code changes.
#![deny(rustdoc::broken_intra_doc_links)]
// Force public structures to implement Debug
#![deny(missing_debug_implementations)]

// Re-export group to make version-matching easier.
pub use group;

use core::ops::{Add, AddAssign, Mul};
use group::{
    ff::PrimeField,
    prime::{PrimeCurve, PrimeCurveAffine},
    Group, GroupOps, GroupOpsOwned, ScalarMul, ScalarMulOwned, UncompressedEncoding,
};

/// An "engine" is a collection of types (fields, elliptic curve groups, etc.)
/// with well-defined relationships. In particular, the G1/G2 curve groups are
/// of prime order `r`, and are equipped with a bilinear pairing function.
pub trait Engine: Sized + 'static + Clone + Sync + Send + core::fmt::Debug {
    /// This is the scalar field of the engine's groups.
    type Fr: PrimeField;

    /// The projective representation of an element in G1.
    type G1: PrimeCurve<Scalar = Self::Fr, Affine = Self::G1Affine>
        + From<Self::G1Affine>
        + GroupOps<Self::G1Affine>
        + GroupOpsOwned<Self::G1Affine>
        + ScalarMul<Self::Fr>
        + ScalarMulOwned<Self::Fr>;

    /// The affine representation of an element in G1.
    type G1Affine: PairingCurveAffine<
            Scalar = Self::Fr,
            Curve = Self::G1,
            Pair = Self::G2Affine,
            PairingResult = Self::Gt,
        > + From<Self::G1>
        + Mul<Self::Fr, Output = Self::G1>
        + for<'a> Mul<&'a Self::Fr, Output = Self::G1>;

    /// The projective representation of an element in G2.
    type G2: PrimeCurve<Scalar = Self::Fr, Affine = Self::G2Affine>
        + From<Self::G2Affine>
        + GroupOps<Self::G2Affine>
        + GroupOpsOwned<Self::G2Affine>
        + ScalarMul<Self::Fr>
        + ScalarMulOwned<Self::Fr>;

    /// The affine representation of an element in G2.
    type G2Affine: PairingCurveAffine<
            Scalar = Self::Fr,
            Curve = Self::G2,
            Pair = Self::G1Affine,
            PairingResult = Self::Gt,
        > + From<Self::G2>
        + Mul<Self::Fr, Output = Self::G2>
        + for<'a> Mul<&'a Self::Fr, Output = Self::G2>;

    /// The extension field that hosts the target group of the pairing.
    type Gt: Group<Scalar = Self::Fr> + ScalarMul<Self::Fr> + ScalarMulOwned<Self::Fr>;

    /// Invoke the pairing function `G1 x G2 -> Gt` without the use of precomputation and
    /// other optimizations.
    fn pairing(p: &Self::G1Affine, q: &Self::G2Affine) -> Self::Gt;
}

/// Affine representation of an elliptic curve point that can be used
/// to perform pairings.
pub trait PairingCurveAffine: PrimeCurveAffine + UncompressedEncoding {
    type Pair: PairingCurveAffine<Pair = Self>;
    type PairingResult: Group;

    /// Perform a pairing
    fn pairing_with(&self, other: &Self::Pair) -> Self::PairingResult;
}

/// An engine that can compute sums of pairings in an efficient way.
pub trait MultiMillerLoop: Engine {
    /// The prepared form of `Self::G2Affine`.
    type G2Prepared: Clone + Send + Sync + From<Self::G2Affine>;

    /// The type returned by `Engine::miller_loop`.
    type Result: MillerLoopResult<Gt = Self::Gt>;

    /// Computes $$\sum_{i=1}^n \textbf{ML}(a_i, b_i)$$ given a series of terms
    /// $$(a_1, b_1), (a_2, b_2), ..., (a_n, b_n).$$
    fn multi_miller_loop(terms: &[(&Self::G1Affine, &Self::G2Prepared)]) -> Self::Result;
}

/// Represents results of a Miller loop, one of the most expensive portions of the pairing
/// function.
///
/// `MillerLoopResult`s cannot be compared with each other until
/// [`MillerLoopResult::final_exponentiation`] is called, which is also expensive.
pub trait MillerLoopResult:
    Clone
    + Copy
    + Default
    + core::fmt::Debug
    + Send
    + Sync
    + Add<Output = Self>
    + for<'a> Add<&'a Self, Output = Self>
    + AddAssign
    + for<'a> AddAssign<&'a Self>
{
    /// The extension field that hosts the target group of the pairing.
    type Gt: Group;

    /// This performs a "final exponentiation" routine to convert the result of a Miller
    /// loop into an element of [`MillerLoopResult::Gt`], so that it can be compared with
    /// other elements of `Gt`.
    fn final_exponentiation(&self) -> Self::Gt;
}
