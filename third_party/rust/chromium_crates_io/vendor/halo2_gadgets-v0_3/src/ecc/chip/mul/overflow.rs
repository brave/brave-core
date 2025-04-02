use super::{T_Q, Z};
use crate::{
    sinsemilla::primitives as sinsemilla, utilities::lookup_range_check::LookupRangeCheckConfig,
};

use group::ff::PrimeField;
use halo2_proofs::circuit::AssignedCell;
use halo2_proofs::{
    circuit::Layouter,
    plonk::{Advice, Assigned, Column, ConstraintSystem, Constraints, Error, Expression, Selector},
    poly::Rotation,
};
use pasta_curves::pallas;

use std::iter;

#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub struct Config {
    // Selector to check z_0 = alpha + t_q (mod p)
    q_mul_overflow: Selector,
    // 10-bit lookup table
    lookup_config: LookupRangeCheckConfig<pallas::Base, { sinsemilla::K }>,
    // Advice columns
    advices: [Column<Advice>; 3],
}

impl Config {
    pub(super) fn configure(
        meta: &mut ConstraintSystem<pallas::Base>,
        lookup_config: LookupRangeCheckConfig<pallas::Base, { sinsemilla::K }>,
        advices: [Column<Advice>; 3],
    ) -> Self {
        for advice in advices.iter() {
            meta.enable_equality(*advice);
        }

        let config = Self {
            q_mul_overflow: meta.selector(),
            lookup_config,
            advices,
        };

        config.create_gate(meta);

        config
    }

    fn create_gate(&self, meta: &mut ConstraintSystem<pallas::Base>) {
        // https://p.z.cash/halo2-0.1:ecc-var-mul-overflow
        meta.create_gate("overflow checks", |meta| {
            let q_mul_overflow = meta.query_selector(self.q_mul_overflow);

            // Constant expressions
            let one = Expression::Constant(pallas::Base::one());
            let two_pow_124 = Expression::Constant(pallas::Base::from_u128(1 << 124));
            let two_pow_130 =
                two_pow_124.clone() * Expression::Constant(pallas::Base::from_u128(1 << 6));

            let z_0 = meta.query_advice(self.advices[0], Rotation::prev());
            let z_130 = meta.query_advice(self.advices[0], Rotation::cur());
            let eta = meta.query_advice(self.advices[0], Rotation::next());

            let k_254 = meta.query_advice(self.advices[1], Rotation::prev());
            let alpha = meta.query_advice(self.advices[1], Rotation::cur());

            // s_minus_lo_130 = s - sum_{i = 0}^{129} 2^i ⋅ s_i
            let s_minus_lo_130 = meta.query_advice(self.advices[1], Rotation::next());

            let s = meta.query_advice(self.advices[2], Rotation::cur());
            let s_check = s - (alpha.clone() + k_254.clone() * two_pow_130);

            // q = 2^254 + t_q is the Pallas scalar field modulus.
            // We cast t_q into the base field to check alpha + t_q (mod p).
            let t_q = Expression::Constant(pallas::Base::from_u128(T_Q));

            // z_0 - alpha - t_q = 0 (mod p)
            let recovery = z_0 - alpha - t_q;

            // k_254 * (z_130 - 2^124) = 0
            let lo_zero = k_254.clone() * (z_130.clone() - two_pow_124);

            // k_254 * s_minus_lo_130 = 0
            let s_minus_lo_130_check = k_254.clone() * s_minus_lo_130.clone();

            // (1 - k_254) * (1 - z_130 * eta) * s_minus_lo_130 = 0
            let canonicity = (one.clone() - k_254) * (one - z_130 * eta) * s_minus_lo_130;

            Constraints::with_selector(
                q_mul_overflow,
                iter::empty()
                    .chain(Some(("s_check", s_check)))
                    .chain(Some(("recovery", recovery)))
                    .chain(Some(("lo_zero", lo_zero)))
                    .chain(Some(("s_minus_lo_130_check", s_minus_lo_130_check)))
                    .chain(Some(("canonicity", canonicity))),
            )
        });
    }

    pub(super) fn overflow_check(
        &self,
        mut layouter: impl Layouter<pallas::Base>,
        alpha: AssignedCell<pallas::Base, pallas::Base>,
        zs: &[Z<pallas::Base>], // [z_0, z_1, ..., z_{254}, z_{255}]
    ) -> Result<(), Error> {
        // s = alpha + k_254 ⋅ 2^130 is witnessed here, and then copied into
        // the decomposition as well as the overflow check gate.
        // In the overflow check gate, we check that s is properly derived
        // from alpha and k_254.
        let s = {
            let k_254 = zs[254].clone();
            let s_val = alpha
                .value()
                .zip(k_254.value())
                .map(|(alpha, k_254)| alpha + k_254 * pallas::Base::from_u128(1 << 65).square());

            layouter.assign_region(
                || "s = alpha + k_254 ⋅ 2^130",
                |mut region| {
                    region.assign_advice(
                        || "s = alpha + k_254 ⋅ 2^130",
                        self.advices[0],
                        0,
                        || s_val,
                    )
                },
            )?
        };

        // Subtract the first 130 low bits of s = alpha + k_254 ⋅ 2^130
        // using thirteen 10-bit lookups, s_{0..=129}
        let s_minus_lo_130 =
            self.s_minus_lo_130(layouter.namespace(|| "decompose s_{0..=129}"), s.clone())?;

        layouter.assign_region(
            || "overflow check",
            |mut region| {
                let offset = 0;

                // Enable overflow check gate
                self.q_mul_overflow.enable(&mut region, offset + 1)?;

                // Copy `z_0`
                zs[0].copy_advice(|| "copy z_0", &mut region, self.advices[0], offset)?;

                // Copy `z_130`
                zs[130].copy_advice(|| "copy z_130", &mut region, self.advices[0], offset + 1)?;

                // Witness η = inv0(z_130), where inv0(x) = 0 if x = 0, 1/x otherwise
                {
                    let eta = zs[130].value().map(|z_130| Assigned::from(z_130).invert());
                    region.assign_advice(
                        || "η = inv0(z_130)",
                        self.advices[0],
                        offset + 2,
                        || eta,
                    )?;
                }

                // Copy `k_254` = z_254
                zs[254].copy_advice(|| "copy k_254", &mut region, self.advices[1], offset)?;

                // Copy original alpha
                alpha.copy_advice(
                    || "copy original alpha",
                    &mut region,
                    self.advices[1],
                    offset + 1,
                )?;

                // Copy weighted sum of the decomposition of s = alpha + k_254 ⋅ 2^130.
                s_minus_lo_130.copy_advice(
                    || "copy s_minus_lo_130",
                    &mut region,
                    self.advices[1],
                    offset + 2,
                )?;

                // Copy witnessed s to check that it was properly derived from alpha and k_254.
                s.copy_advice(|| "copy s", &mut region, self.advices[2], offset + 1)?;

                Ok(())
            },
        )?;

        Ok(())
    }

    fn s_minus_lo_130(
        &self,
        mut layouter: impl Layouter<pallas::Base>,
        s: AssignedCell<pallas::Base, pallas::Base>,
    ) -> Result<AssignedCell<pallas::Base, pallas::Base>, Error> {
        // Number of k-bit words we can use in the lookup decomposition.
        let num_words = 130 / sinsemilla::K;
        assert!(num_words * sinsemilla::K == 130);

        // Decompose the low 130 bits of `s` using thirteen 10-bit lookups.
        let zs = self.lookup_config.copy_check(
            layouter.namespace(|| "Decompose low 130 bits of s"),
            s,
            num_words,
            false,
        )?;
        // (s - (2^0 s_0 + 2^1 s_1 + ... + 2^129 s_129)) / 2^130
        Ok(zs[zs.len() - 1].clone())
    }
}
