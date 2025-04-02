use super::{EccPoint, NonIdentityEccPoint};

use group::prime::PrimeCurveAffine;

use halo2_proofs::{
    circuit::{AssignedCell, Region, Value},
    plonk::{
        Advice, Assigned, Column, ConstraintSystem, Constraints, Error, Expression, Selector,
        VirtualCells,
    },
    poly::Rotation,
};
use pasta_curves::{arithmetic::CurveAffine, pallas};

type Coordinates = (
    AssignedCell<Assigned<pallas::Base>, pallas::Base>,
    AssignedCell<Assigned<pallas::Base>, pallas::Base>,
);

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Config {
    q_point: Selector,
    q_point_non_id: Selector,
    // x-coordinate
    pub x: Column<Advice>,
    // y-coordinate
    pub y: Column<Advice>,
}

impl Config {
    pub(super) fn configure(
        meta: &mut ConstraintSystem<pallas::Base>,
        x: Column<Advice>,
        y: Column<Advice>,
    ) -> Self {
        let config = Self {
            q_point: meta.selector(),
            q_point_non_id: meta.selector(),
            x,
            y,
        };

        config.create_gate(meta);

        config
    }

    fn create_gate(&self, meta: &mut ConstraintSystem<pallas::Base>) {
        let curve_eqn = |meta: &mut VirtualCells<pallas::Base>| {
            let x = meta.query_advice(self.x, Rotation::cur());
            let y = meta.query_advice(self.y, Rotation::cur());

            // y^2 = x^3 + b
            y.square() - (x.clone().square() * x) - Expression::Constant(pallas::Affine::b())
        };

        // https://p.z.cash/halo2-0.1:ecc-witness-point
        meta.create_gate("witness point", |meta| {
            // Check that the point being witnessed is either:
            // - the identity, which is mapped to (0, 0) in affine coordinates; or
            // - a valid curve point y^2 = x^3 + b, where b = 5 in the Pallas equation

            let q_point = meta.query_selector(self.q_point);
            let x = meta.query_advice(self.x, Rotation::cur());
            let y = meta.query_advice(self.y, Rotation::cur());

            // We can't use `Constraints::with_selector` because that creates constraints
            // of the form `q_point * (x * curve_eqn)`, but this was implemented without
            // parentheses, and thus evaluates as `(q_point * x) * curve_eqn`, which is
            // structurally different in the pinned verifying key.
            [
                ("x == 0 v on_curve", q_point.clone() * x * curve_eqn(meta)),
                ("y == 0 v on_curve", q_point * y * curve_eqn(meta)),
            ]
        });

        // https://p.z.cash/halo2-0.1:ecc-witness-non-identity-point
        meta.create_gate("witness non-identity point", |meta| {
            // Check that the point being witnessed is a valid curve point y^2 = x^3 + b,
            // where b = 5 in the Pallas equation

            let q_point_non_id = meta.query_selector(self.q_point_non_id);

            Constraints::with_selector(q_point_non_id, Some(("on_curve", curve_eqn(meta))))
        });
    }

    fn assign_xy(
        &self,
        value: Value<(Assigned<pallas::Base>, Assigned<pallas::Base>)>,
        offset: usize,
        region: &mut Region<'_, pallas::Base>,
    ) -> Result<Coordinates, Error> {
        // Assign `x` value
        let x_val = value.map(|value| value.0);
        let x_var = region.assign_advice(|| "x", self.x, offset, || x_val)?;

        // Assign `y` value
        let y_val = value.map(|value| value.1);
        let y_var = region.assign_advice(|| "y", self.y, offset, || y_val)?;

        Ok((x_var, y_var))
    }

    /// Assigns a point that can be the identity.
    pub(super) fn point(
        &self,
        value: Value<pallas::Affine>,
        offset: usize,
        region: &mut Region<'_, pallas::Base>,
    ) -> Result<EccPoint, Error> {
        // Enable `q_point` selector
        self.q_point.enable(region, offset)?;

        let value = value.map(|value| {
            // Map the identity to (0, 0).
            if value == pallas::Affine::identity() {
                (Assigned::Zero, Assigned::Zero)
            } else {
                let value = value.coordinates().unwrap();
                (value.x().into(), value.y().into())
            }
        });

        self.assign_xy(value, offset, region)
            .map(|(x, y)| EccPoint::from_coordinates_unchecked(x, y))
    }

    /// Assigns a non-identity point.
    pub(super) fn point_non_id(
        &self,
        value: Value<pallas::Affine>,
        offset: usize,
        region: &mut Region<'_, pallas::Base>,
    ) -> Result<NonIdentityEccPoint, Error> {
        // Enable `q_point_non_id` selector
        self.q_point_non_id.enable(region, offset)?;

        // Return an error if the point is the identity.
        value.error_if_known_and(|value| value == &pallas::Affine::identity())?;

        let value = value.map(|value| {
            let value = value.coordinates().unwrap();
            (value.x().into(), value.y().into())
        });

        self.assign_xy(value, offset, region)
            .map(|(x, y)| NonIdentityEccPoint::from_coordinates_unchecked(x, y))
    }
}

#[cfg(test)]
pub mod tests {
    use halo2_proofs::circuit::Layouter;
    use pasta_curves::pallas;

    use super::*;
    use crate::ecc::{EccInstructions, NonIdentityPoint};

    pub fn test_witness_non_id<
        EccChip: EccInstructions<pallas::Affine> + Clone + Eq + std::fmt::Debug,
    >(
        chip: EccChip,
        mut layouter: impl Layouter<pallas::Base>,
    ) {
        // Witnessing the identity should return an error.
        NonIdentityPoint::new(
            chip,
            layouter.namespace(|| "witness identity"),
            Value::known(pallas::Affine::identity()),
        )
        .expect_err("witnessing 𝒪 should return an error");
    }
}
