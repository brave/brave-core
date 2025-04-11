use std::marker::PhantomData;

use crate::arithmetic::CurveAffine;

mod prover;
mod verifier;

/// A vanishing argument.
pub(crate) struct Argument<C: CurveAffine> {
    _marker: PhantomData<C>,
}
