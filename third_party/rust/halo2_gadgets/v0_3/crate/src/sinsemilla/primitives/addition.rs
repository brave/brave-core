use std::ops::Add;

use group::{cofactor::CofactorCurveAffine, Group};
use pasta_curves::pallas;
use subtle::{ConstantTimeEq, CtOption};

/// P ∪ {⊥}
///
/// Simulated incomplete addition built over complete addition.
#[derive(Clone, Copy, Debug)]
pub(super) struct IncompletePoint(CtOption<pallas::Point>);

impl From<pallas::Point> for IncompletePoint {
    fn from(p: pallas::Point) -> Self {
        IncompletePoint(CtOption::new(p, 1.into()))
    }
}

impl From<IncompletePoint> for CtOption<pallas::Point> {
    fn from(p: IncompletePoint) -> Self {
        p.0
    }
}

impl Add for IncompletePoint {
    type Output = IncompletePoint;

    #[allow(clippy::suspicious_arithmetic_impl)]
    fn add(self, rhs: Self) -> Self::Output {
        // ⊥ ⸭ ⊥ = ⊥
        // ⊥ ⸭ P = ⊥
        IncompletePoint(self.0.and_then(|p| {
            // P ⸭ ⊥ = ⊥
            rhs.0.and_then(|q| {
                // 0 ⸭ 0 = ⊥
                // 0 ⸭ P = ⊥
                // P ⸭ 0 = ⊥
                // (x, y) ⸭ (x', y') = ⊥ if x == x'
                // (x, y) ⸭ (x', y') = (x, y) + (x', y') if x != x'
                CtOption::new(
                    p + q,
                    !(p.is_identity() | q.is_identity() | p.ct_eq(&q) | p.ct_eq(&-q)),
                )
            })
        }))
    }
}

impl Add<pallas::Affine> for IncompletePoint {
    type Output = IncompletePoint;

    /// Specialisation of incomplete addition for mixed addition.
    #[allow(clippy::suspicious_arithmetic_impl)]
    fn add(self, rhs: pallas::Affine) -> Self::Output {
        // ⊥ ⸭ ⊥ = ⊥
        // ⊥ ⸭ P = ⊥
        IncompletePoint(self.0.and_then(|p| {
            // P ⸭ ⊥ = ⊥ is satisfied by definition.
            let q = rhs.to_curve();

            // 0 ⸭ 0 = ⊥
            // 0 ⸭ P = ⊥
            // P ⸭ 0 = ⊥
            // (x, y) ⸭ (x', y') = ⊥ if x == x'
            // (x, y) ⸭ (x', y') = (x, y) + (x', y') if x != x'
            CtOption::new(
                // Use mixed addition for efficiency.
                p + rhs,
                !(p.is_identity() | q.is_identity() | p.ct_eq(&q) | p.ct_eq(&-q)),
            )
        }))
    }
}
