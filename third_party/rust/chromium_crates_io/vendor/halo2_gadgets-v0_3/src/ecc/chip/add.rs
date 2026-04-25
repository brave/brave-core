use super::EccPoint;

use group::ff::PrimeField;
use halo2_proofs::{
    circuit::Region,
    plonk::{Advice, Assigned, Column, ConstraintSystem, Constraints, Error, Expression, Selector},
    poly::Rotation,
};
use pasta_curves::pallas;

use std::collections::HashSet;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Config {
    q_add: Selector,
    // lambda
    lambda: Column<Advice>,
    // x-coordinate of P in P + Q = R
    pub x_p: Column<Advice>,
    // y-coordinate of P in P + Q = R
    pub y_p: Column<Advice>,
    // x-coordinate of Q or R in P + Q = R
    pub x_qr: Column<Advice>,
    // y-coordinate of Q or R in P + Q = R
    pub y_qr: Column<Advice>,
    // α = inv0(x_q - x_p)
    alpha: Column<Advice>,
    // β = inv0(x_p)
    beta: Column<Advice>,
    // γ = inv0(x_q)
    gamma: Column<Advice>,
    // δ = inv0(y_p + y_q) if x_q = x_p, 0 otherwise
    delta: Column<Advice>,
}

impl Config {
    #[allow(clippy::too_many_arguments)]
    pub(super) fn configure(
        meta: &mut ConstraintSystem<pallas::Base>,
        x_p: Column<Advice>,
        y_p: Column<Advice>,
        x_qr: Column<Advice>,
        y_qr: Column<Advice>,
        lambda: Column<Advice>,
        alpha: Column<Advice>,
        beta: Column<Advice>,
        gamma: Column<Advice>,
        delta: Column<Advice>,
    ) -> Self {
        meta.enable_equality(x_p);
        meta.enable_equality(y_p);
        meta.enable_equality(x_qr);
        meta.enable_equality(y_qr);

        let config = Self {
            q_add: meta.selector(),
            x_p,
            y_p,
            x_qr,
            y_qr,
            lambda,
            alpha,
            beta,
            gamma,
            delta,
        };

        config.create_gate(meta);

        config
    }

    pub(crate) fn output_columns(&self) -> HashSet<Column<Advice>> {
        [self.x_qr, self.y_qr].into_iter().collect()
    }

    fn create_gate(&self, meta: &mut ConstraintSystem<pallas::Base>) {
        // https://p.z.cash/halo2-0.1:ecc-complete-addition
        meta.create_gate("complete addition", |meta| {
            let q_add = meta.query_selector(self.q_add);
            let x_p = meta.query_advice(self.x_p, Rotation::cur());
            let y_p = meta.query_advice(self.y_p, Rotation::cur());
            let x_q = meta.query_advice(self.x_qr, Rotation::cur());
            let y_q = meta.query_advice(self.y_qr, Rotation::cur());
            let x_r = meta.query_advice(self.x_qr, Rotation::next());
            let y_r = meta.query_advice(self.y_qr, Rotation::next());
            let lambda = meta.query_advice(self.lambda, Rotation::cur());

            // α = inv0(x_q - x_p)
            let alpha = meta.query_advice(self.alpha, Rotation::cur());
            // β = inv0(x_p)
            let beta = meta.query_advice(self.beta, Rotation::cur());
            // γ = inv0(x_q)
            let gamma = meta.query_advice(self.gamma, Rotation::cur());
            // δ = inv0(y_p + y_q) if x_q = x_p, 0 otherwise
            let delta = meta.query_advice(self.delta, Rotation::cur());

            // Useful composite expressions
            // (x_q − x_p)
            let x_q_minus_x_p = x_q.clone() - x_p.clone();
            // (x_p - x_r)
            let x_p_minus_x_r = x_p.clone() - x_r.clone();
            // (y_q + y_p)
            let y_q_plus_y_p = y_q.clone() + y_p.clone();
            // α ⋅(x_q - x_p)
            let if_alpha = x_q_minus_x_p.clone() * alpha;
            // β ⋅ x_p
            let if_beta = x_p.clone() * beta;
            // γ ⋅ x_q
            let if_gamma = x_q.clone() * gamma;
            // δ ⋅(y_q + y_p)
            let if_delta = y_q_plus_y_p.clone() * delta;

            // Useful constants
            let one = Expression::Constant(pallas::Base::one());
            let two = Expression::Constant(pallas::Base::from(2));
            let three = Expression::Constant(pallas::Base::from(3));

            // (x_q − x_p)⋅((x_q − x_p)⋅λ − (y_q−y_p)) = 0
            let poly1 = {
                let y_q_minus_y_p = y_q.clone() - y_p.clone(); // (y_q − y_p)
                let incomplete = x_q_minus_x_p.clone() * lambda.clone() - y_q_minus_y_p; // (x_q − x_p)⋅λ − (y_q−y_p)

                // q_add ⋅(x_q − x_p)⋅((x_q − x_p)⋅λ − (y_q−y_p))
                x_q_minus_x_p.clone() * incomplete
            };

            // (1 - (x_q - x_p)⋅α)⋅(2y_p ⋅λ - 3x_p^2) = 0
            let poly2 = {
                let three_x_p_sq = three * x_p.clone().square(); // 3x_p^2
                let two_y_p = two * y_p.clone(); // 2y_p
                let tangent_line = two_y_p * lambda.clone() - three_x_p_sq; // (2y_p ⋅λ - 3x_p^2)

                // q_add ⋅(1 - (x_q - x_p)⋅α)⋅(2y_p ⋅λ - 3x_p^2)
                (one.clone() - if_alpha.clone()) * tangent_line
            };

            // (λ^2 - x_p - x_q - x_r)
            let nonexceptional_x_r =
                lambda.clone().square() - x_p.clone() - x_q.clone() - x_r.clone();
            // (λ ⋅(x_p - x_r) - y_p - y_r)
            let nonexceptional_y_r = lambda * x_p_minus_x_r - y_p.clone() - y_r.clone();

            // x_p⋅x_q⋅(x_q - x_p)⋅(λ^2 - x_p - x_q - x_r) = 0
            let poly3a =
                x_p.clone() * x_q.clone() * x_q_minus_x_p.clone() * nonexceptional_x_r.clone();

            // x_p⋅x_q⋅(x_q - x_p)⋅(λ ⋅(x_p - x_r) - y_p - y_r) = 0
            let poly3b = x_p.clone() * x_q.clone() * x_q_minus_x_p * nonexceptional_y_r.clone();

            // x_p⋅x_q⋅(y_q + y_p)⋅(λ^2 - x_p - x_q - x_r) = 0
            let poly3c = x_p.clone() * x_q.clone() * y_q_plus_y_p.clone() * nonexceptional_x_r;

            // x_p⋅x_q⋅(y_q + y_p)⋅(λ ⋅(x_p - x_r) - y_p - y_r) = 0
            let poly3d = x_p.clone() * x_q.clone() * y_q_plus_y_p * nonexceptional_y_r;

            // (1 - x_p * β) * (x_r - x_q) = 0
            let poly4a = (one.clone() - if_beta.clone()) * (x_r.clone() - x_q);

            // (1 - x_p * β) * (y_r - y_q) = 0
            let poly4b = (one.clone() - if_beta) * (y_r.clone() - y_q);

            // (1 - x_q * γ) * (x_r - x_p) = 0
            let poly5a = (one.clone() - if_gamma.clone()) * (x_r.clone() - x_p);

            // (1 - x_q * γ) * (y_r - y_p) = 0
            let poly5b = (one.clone() - if_gamma) * (y_r.clone() - y_p);

            // ((1 - (x_q - x_p) * α - (y_q + y_p) * δ)) * x_r
            let poly6a = (one.clone() - if_alpha.clone() - if_delta.clone()) * x_r;

            // ((1 - (x_q - x_p) * α - (y_q + y_p) * δ)) * y_r
            let poly6b = (one - if_alpha - if_delta) * y_r;

            Constraints::with_selector(
                q_add,
                [
                    ("1", poly1),
                    ("2", poly2),
                    ("3a", poly3a),
                    ("3b", poly3b),
                    ("3c", poly3c),
                    ("3d", poly3d),
                    ("4a", poly4a),
                    ("4b", poly4b),
                    ("5a", poly5a),
                    ("5b", poly5b),
                    ("6a", poly6a),
                    ("6b", poly6b),
                ],
            )
        });
    }

    pub(super) fn assign_region(
        &self,
        p: &EccPoint,
        q: &EccPoint,
        offset: usize,
        region: &mut Region<'_, pallas::Base>,
    ) -> Result<EccPoint, Error> {
        // Enable `q_add` selector
        self.q_add.enable(region, offset)?;

        // Copy point `p` into `x_p`, `y_p` columns
        p.x.copy_advice(|| "x_p", region, self.x_p, offset)?;
        p.y.copy_advice(|| "y_p", region, self.y_p, offset)?;

        // Copy point `q` into `x_qr`, `y_qr` columns
        q.x.copy_advice(|| "x_q", region, self.x_qr, offset)?;
        q.y.copy_advice(|| "y_q", region, self.y_qr, offset)?;

        let (x_p, y_p) = (p.x.value(), p.y.value());
        let (x_q, y_q) = (q.x.value(), q.y.value());

        // Assign α = inv0(x_q - x_p)
        let alpha = (x_q - x_p).invert();
        region.assign_advice(|| "α", self.alpha, offset, || alpha)?;

        // Assign β = inv0(x_p)
        let beta = x_p.invert();
        region.assign_advice(|| "β", self.beta, offset, || beta)?;

        // Assign γ = inv0(x_q)
        let gamma = x_q.invert();
        region.assign_advice(|| "γ", self.gamma, offset, || gamma)?;

        // Assign δ = inv0(y_q + y_p) if x_q = x_p, 0 otherwise
        let delta = x_p
            .zip(x_q)
            .zip(y_p)
            .zip(y_q)
            .map(|(((x_p, x_q), y_p), y_q)| {
                if x_q == x_p {
                    (y_q + y_p).invert()
                } else {
                    Assigned::Zero
                }
            });
        region.assign_advice(|| "δ", self.delta, offset, || delta)?;

        #[allow(clippy::collapsible_else_if)]
        // Assign lambda
        let lambda =
            x_p.zip(y_p)
                .zip(x_q)
                .zip(y_q)
                .zip(alpha)
                .map(|((((x_p, y_p), x_q), y_q), alpha)| {
                    if x_q != x_p {
                        // λ = (y_q - y_p)/(x_q - x_p)
                        // Here, alpha = inv0(x_q - x_p), which suffices since we
                        // know that x_q != x_p in this branch.
                        (y_q - y_p) * alpha
                    } else {
                        if !y_p.is_zero_vartime() {
                            // 3(x_p)^2
                            let three_x_p_sq = x_p.square() * pallas::Base::from(3);
                            // 1 / 2(y_p)
                            let inv_two_y_p = y_p.invert() * pallas::Base::TWO_INV;
                            // λ = 3(x_p)^2 / 2(y_p)
                            three_x_p_sq * inv_two_y_p
                        } else {
                            Assigned::Zero
                        }
                    }
                });
        region.assign_advice(|| "λ", self.lambda, offset, || lambda)?;

        // Calculate (x_r, y_r)
        let r =
            x_p.zip(y_p)
                .zip(x_q)
                .zip(y_q)
                .zip(lambda)
                .map(|((((x_p, y_p), x_q), y_q), lambda)| {
                    {
                        if x_p.is_zero_vartime() {
                            // 0 + Q = Q
                            (*x_q, *y_q)
                        } else if x_q.is_zero_vartime() {
                            // P + 0 = P
                            (*x_p, *y_p)
                        } else if (x_q == x_p) && (*y_q == -y_p) {
                            // P + (-P) maps to (0,0)
                            (Assigned::Zero, Assigned::Zero)
                        } else {
                            // x_r = λ^2 - x_p - x_q
                            let x_r = lambda.square() - x_p - x_q;
                            // y_r = λ(x_p - x_r) - y_p
                            let y_r = lambda * (x_p - x_r) - y_p;
                            (x_r, y_r)
                        }
                    }
                });

        // Assign x_r
        let x_r = r.map(|r| r.0);
        let x_r_cell = region.assign_advice(|| "x_r", self.x_qr, offset + 1, || x_r)?;

        // Assign y_r
        let y_r = r.map(|r| r.1);
        let y_r_cell = region.assign_advice(|| "y_r", self.y_qr, offset + 1, || y_r)?;

        let result = EccPoint::from_coordinates_unchecked(x_r_cell, y_r_cell);

        #[cfg(test)]
        // Check that the correct sum is obtained.
        {
            use group::Curve;

            let p = p.point();
            let q = q.point();
            let real_sum = p.zip(q).map(|(p, q)| p + q);
            let result = result.point();

            real_sum
                .zip(result)
                .assert_if_known(|(real_sum, result)| &real_sum.to_affine() == result);
        }

        Ok(result)
    }
}

#[cfg(test)]
pub mod tests {
    use group::{prime::PrimeCurveAffine, Curve};
    use halo2_proofs::{
        circuit::{Layouter, Value},
        plonk::Error,
    };
    use pasta_curves::{arithmetic::CurveExt, pallas};

    use crate::ecc::{chip::EccPoint, EccInstructions, NonIdentityPoint};

    #[allow(clippy::too_many_arguments)]
    pub fn test_add<
        EccChip: EccInstructions<pallas::Affine, Point = EccPoint> + Clone + Eq + std::fmt::Debug,
    >(
        chip: EccChip,
        mut layouter: impl Layouter<pallas::Base>,
        p_val: pallas::Affine,
        p: &NonIdentityPoint<pallas::Affine, EccChip>,
        q_val: pallas::Affine,
        q: &NonIdentityPoint<pallas::Affine, EccChip>,
        p_neg: &NonIdentityPoint<pallas::Affine, EccChip>,
    ) -> Result<(), Error> {
        // Make sure P and Q are not the same point.
        assert_ne!(p_val, q_val);

        // Check complete addition P + (-P)
        let zero = {
            let result = p.add(layouter.namespace(|| "P + (-P)"), p_neg)?;
            result
                .inner()
                .is_identity()
                .assert_if_known(|is_identity| *is_identity);
            result
        };

        // Check complete addition 𝒪 + 𝒪
        {
            let result = zero.add(layouter.namespace(|| "𝒪 + 𝒪"), &zero)?;
            result.constrain_equal(layouter.namespace(|| "𝒪 + 𝒪 = 𝒪"), &zero)?;
        }

        // Check P + Q
        {
            let result = p.add(layouter.namespace(|| "P + Q"), q)?;
            let witnessed_result = NonIdentityPoint::new(
                chip.clone(),
                layouter.namespace(|| "witnessed P + Q"),
                Value::known((p_val + q_val).to_affine()),
            )?;
            result.constrain_equal(layouter.namespace(|| "constrain P + Q"), &witnessed_result)?;
        }

        // P + P
        {
            let result = p.add(layouter.namespace(|| "P + P"), p)?;
            let witnessed_result = NonIdentityPoint::new(
                chip.clone(),
                layouter.namespace(|| "witnessed P + P"),
                Value::known((p_val + p_val).to_affine()),
            )?;
            result.constrain_equal(layouter.namespace(|| "constrain P + P"), &witnessed_result)?;
        }

        // P + 𝒪
        {
            let result = p.add(layouter.namespace(|| "P + 𝒪"), &zero)?;
            result.constrain_equal(layouter.namespace(|| "P + 𝒪 = P"), p)?;
        }

        // 𝒪 + P
        {
            let result = zero.add(layouter.namespace(|| "𝒪 + P"), p)?;
            result.constrain_equal(layouter.namespace(|| "𝒪 + P = P"), p)?;
        }

        // (x, y) + (ζx, y) should behave like normal P + Q.
        let endo_p = p_val.to_curve().endo();
        let endo_p = NonIdentityPoint::new(
            chip.clone(),
            layouter.namespace(|| "endo(P)"),
            Value::known(endo_p.to_affine()),
        )?;
        p.add(layouter.namespace(|| "P + endo(P)"), &endo_p)?;

        // (x, y) + (ζx, -y) should also behave like normal P + Q.
        let endo_p_neg = (-p_val).to_curve().endo();
        let endo_p_neg = NonIdentityPoint::new(
            chip.clone(),
            layouter.namespace(|| "endo(-P)"),
            Value::known(endo_p_neg.to_affine()),
        )?;
        p.add(layouter.namespace(|| "P + endo(-P)"), &endo_p_neg)?;

        // (x, y) + ((ζ^2)x, y)
        let endo_2_p = p_val.to_curve().endo().endo();
        let endo_2_p = NonIdentityPoint::new(
            chip.clone(),
            layouter.namespace(|| "endo^2(P)"),
            Value::known(endo_2_p.to_affine()),
        )?;
        p.add(layouter.namespace(|| "P + endo^2(P)"), &endo_2_p)?;

        // (x, y) + ((ζ^2)x, -y)
        let endo_2_p_neg = (-p_val).to_curve().endo().endo();
        let endo_2_p_neg = NonIdentityPoint::new(
            chip,
            layouter.namespace(|| "endo^2(-P)"),
            Value::known(endo_2_p_neg.to_affine()),
        )?;
        p.add(layouter.namespace(|| "P + endo^2(-P)"), &endo_2_p_neg)?;

        Ok(())
    }
}
