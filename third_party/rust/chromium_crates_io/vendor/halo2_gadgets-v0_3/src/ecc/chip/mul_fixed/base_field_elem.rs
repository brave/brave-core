use super::super::{EccBaseFieldElemFixed, EccPoint, FixedPoints, NUM_WINDOWS, T_P};
use super::H_BASE;

use crate::utilities::bool_check;
use crate::{
    sinsemilla::primitives as sinsemilla,
    utilities::{bitrange_subset, lookup_range_check::LookupRangeCheckConfig, range_check},
};

use group::ff::PrimeField;
use halo2_proofs::{
    circuit::{AssignedCell, Layouter},
    plonk::{Advice, Column, ConstraintSystem, Constraints, Error, Expression, Selector},
    poly::Rotation,
};
use pasta_curves::pallas;

use std::convert::TryInto;

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Config<Fixed: FixedPoints<pallas::Affine>> {
    q_mul_fixed_base_field: Selector,
    canon_advices: [Column<Advice>; 3],
    lookup_config: LookupRangeCheckConfig<pallas::Base, { sinsemilla::K }>,
    super_config: super::Config<Fixed>,
}

impl<Fixed: FixedPoints<pallas::Affine>> Config<Fixed> {
    pub(crate) fn configure(
        meta: &mut ConstraintSystem<pallas::Base>,
        canon_advices: [Column<Advice>; 3],
        lookup_config: LookupRangeCheckConfig<pallas::Base, { sinsemilla::K }>,
        super_config: super::Config<Fixed>,
    ) -> Self {
        for advice in canon_advices.iter() {
            meta.enable_equality(*advice);
        }

        let config = Self {
            q_mul_fixed_base_field: meta.selector(),
            canon_advices,
            lookup_config,
            super_config,
        };

        let add_incomplete_advices = config.super_config.add_incomplete_config.advice_columns();
        for canon_advice in config.canon_advices.iter() {
            assert!(
                !add_incomplete_advices.contains(canon_advice),
                "Deconflict canon_advice columns with incomplete addition columns."
            );
        }

        config.create_gate(meta);

        config
    }

    fn create_gate(&self, meta: &mut ConstraintSystem<pallas::Base>) {
        // Check that the base field element is canonical.
        // https://p.z.cash/halo2-0.1:ecc-fixed-mul-base-canonicity
        meta.create_gate("Canonicity checks", |meta| {
            let q_mul_fixed_base_field = meta.query_selector(self.q_mul_fixed_base_field);

            let alpha = meta.query_advice(self.canon_advices[0], Rotation::prev());
            // The last three bits of α.
            let z_84_alpha = meta.query_advice(self.canon_advices[2], Rotation::prev());

            // Decompose α into three pieces, in little-endian order:
            //            α = α_0 (252 bits)  || α_1 (2 bits) || α_2 (1 bit).
            //
            // α_0 is derived, not witnessed.
            let alpha_0 = {
                let two_pow_252 = pallas::Base::from_u128(1 << 126).square();
                alpha - (z_84_alpha.clone() * two_pow_252)
            };
            let alpha_1 = meta.query_advice(self.canon_advices[1], Rotation::cur());
            let alpha_2 = meta.query_advice(self.canon_advices[2], Rotation::cur());

            let alpha_0_prime = meta.query_advice(self.canon_advices[0], Rotation::cur());
            let z_13_alpha_0_prime = meta.query_advice(self.canon_advices[0], Rotation::next());
            let z_44_alpha = meta.query_advice(self.canon_advices[1], Rotation::next());
            let z_43_alpha = meta.query_advice(self.canon_advices[2], Rotation::next());

            let decomposition_checks = {
                // Range-constrain α_1 to be 2 bits
                let alpha_1_range_check = range_check(alpha_1.clone(), 1 << 2);
                // Boolean-constrain α_2
                let alpha_2_range_check = bool_check(alpha_2.clone());
                // Check that α_1 + 2^2 α_2 = z_84_alpha
                let z_84_alpha_check = z_84_alpha.clone()
                    - (alpha_1.clone() + alpha_2.clone() * pallas::Base::from(1 << 2));

                std::iter::empty()
                    .chain(Some(("alpha_1_range_check", alpha_1_range_check)))
                    .chain(Some(("alpha_2_range_check", alpha_2_range_check)))
                    .chain(Some(("z_84_alpha_check", z_84_alpha_check)))
            };

            // Check α_0_prime = α_0 + 2^130 - t_p
            let alpha_0_prime_check = {
                let two_pow_130 = Expression::Constant(pallas::Base::from_u128(1 << 65).square());
                let t_p = Expression::Constant(pallas::Base::from_u128(T_P));
                alpha_0_prime - (alpha_0 + two_pow_130 - t_p)
            };

            // We want to enforce canonicity of a 255-bit base field element, α.
            // That is, we want to check that 0 ≤ α < p, where p is Pallas base
            // field modulus p = 2^254 + t_p
            //                 = 2^254 + 45560315531419706090280762371685220353.
            // Note that t_p < 2^130.
            //
            // α has been decomposed into three pieces in little-endian order:
            //            α = α_0 (252 bits)  || α_1 (2 bits) || α_2 (1 bit).
            //              = α_0 + 2^252 α_1 + 2^254 α_2.
            //
            // If the MSB α_2 = 1, then:
            //      - α_2 = 1 => α_1 = 0, and
            //      - α_2 = 1 => α_0 < t_p. To enforce this:
            //          - α_2 = 1 => 0 ≤ α_0 < 2^130
            //                - alpha_0_hi_120 = 0 (constrain α_0 to be 132 bits)
            //                - a_43 = 0 or 1 (constrain α_0[130..=131] to be 0)
            //          - α_2 = 1 => 0 ≤ α_0 + 2^130 - t_p < 2^130
            //                    => 13 ten-bit lookups of α_0 + 2^130 - t_p
            //                    => z_13_alpha_0_prime = 0
            //
            let canon_checks = {
                // alpha_0_hi_120 = z_44 - 2^120 z_84
                let alpha_0_hi_120 = {
                    let two_pow_120 =
                        Expression::Constant(pallas::Base::from_u128(1 << 60).square());
                    z_44_alpha.clone() - z_84_alpha * two_pow_120
                };
                // a_43 = z_43 - (2^3)z_44
                let a_43 = z_43_alpha - z_44_alpha * *H_BASE;

                std::iter::empty()
                    .chain(Some(("MSB = 1 => alpha_1 = 0", alpha_2.clone() * alpha_1)))
                    .chain(Some((
                        "MSB = 1 => alpha_0_hi_120 = 0",
                        alpha_2.clone() * alpha_0_hi_120,
                    )))
                    .chain(Some((
                        "MSB = 1 => a_43 = 0 or 1",
                        alpha_2.clone() * bool_check(a_43),
                    )))
                    .chain(Some((
                        "MSB = 1 => z_13_alpha_0_prime = 0",
                        alpha_2 * z_13_alpha_0_prime,
                    )))
            };

            Constraints::with_selector(
                q_mul_fixed_base_field,
                canon_checks
                    .chain(decomposition_checks)
                    .chain(Some(("alpha_0_prime check", alpha_0_prime_check))),
            )
        });
    }

    pub fn assign(
        &self,
        mut layouter: impl Layouter<pallas::Base>,
        scalar: AssignedCell<pallas::Base, pallas::Base>,
        base: &<Fixed as FixedPoints<pallas::Affine>>::Base,
    ) -> Result<EccPoint, Error>
    where
        <Fixed as FixedPoints<pallas::Affine>>::Base: super::super::FixedPoint<pallas::Affine>,
    {
        let (scalar, acc, mul_b) = layouter.assign_region(
            || "Base-field elem fixed-base mul (incomplete addition)",
            |mut region| {
                let offset = 0;

                // Decompose scalar
                let scalar = {
                    let running_sum = self.super_config.running_sum_config.copy_decompose(
                        &mut region,
                        offset,
                        scalar.clone(),
                        true,
                        pallas::Base::NUM_BITS as usize,
                        NUM_WINDOWS,
                    )?;
                    EccBaseFieldElemFixed {
                        base_field_elem: running_sum[0].clone(),
                        running_sum: (*running_sum).as_slice().try_into().unwrap(),
                    }
                };

                let (acc, mul_b) = self.super_config.assign_region_inner::<_, NUM_WINDOWS>(
                    &mut region,
                    offset,
                    &(&scalar).into(),
                    base,
                    self.super_config.running_sum_config.q_range_check(),
                )?;

                Ok((scalar, acc, mul_b))
            },
        )?;

        // Add to the accumulator and return the final result as `[scalar]B`.
        let result = layouter.assign_region(
            || "Base-field elem fixed-base mul (complete addition)",
            |mut region| {
                self.super_config.add_config.assign_region(
                    &mul_b.clone().into(),
                    &acc.clone().into(),
                    0,
                    &mut region,
                )
            },
        )?;

        #[cfg(test)]
        // Check that the correct multiple is obtained.
        {
            use super::super::FixedPoint;
            use group::Curve;

            let scalar = &scalar
                .base_field_elem()
                .value()
                .map(|scalar| pallas::Scalar::from_repr(scalar.to_repr()).unwrap());
            let real_mul = scalar.map(|scalar| base.generator() * scalar);
            let result = result.point();

            real_mul
                .zip(result)
                .assert_if_known(|(real_mul, result)| &real_mul.to_affine() == result);
        }

        // We want to enforce canonicity of a 255-bit base field element, α.
        // That is, we want to check that 0 ≤ α < p, where p is Pallas base
        // field modulus p = 2^254 + t_p
        //                 = 2^254 + 45560315531419706090280762371685220353.
        // Note that t_p < 2^130.
        //
        // α has been decomposed into three pieces in little-endian order:
        //            α = α_0 (252 bits)  || α_1 (2 bits) || α_2 (1 bit).
        //              = α_0 + 2^252 α_1 + 2^254 α_2.
        //
        // If the MSB α_2 = 1, then:
        //      - α_2 = 1 => α_1 = 0, and
        //      - α_2 = 1 => α_0 < t_p. To enforce this:
        //      - α_2 = 1 => 0 ≤ α_0 < 2^130
        //                => 13 ten-bit lookups of α_0
        //      - α_2 = 1 => 0 ≤ α_0 + 2^130 - t_p < 2^130
        //                => 13 ten-bit lookups of α_0 + 2^130 - t_p
        //                => z_13_alpha_0_prime = 0
        //
        let (alpha, running_sum) = (scalar.base_field_elem, &scalar.running_sum);
        let z_43_alpha = running_sum[43].clone();
        let z_44_alpha = running_sum[44].clone();
        let z_84_alpha = running_sum[84].clone();

        // α_0 = α - z_84_alpha * 2^252
        let alpha_0 = alpha
            .value()
            .zip(z_84_alpha.value())
            .map(|(alpha, z_84_alpha)| {
                let two_pow_252 = pallas::Base::from_u128(1 << 126).square();
                alpha - z_84_alpha * two_pow_252
            });

        let (alpha_0_prime, z_13_alpha_0_prime) = {
            // alpha_0_prime = alpha + 2^130 - t_p.
            let alpha_0_prime = alpha_0.map(|alpha_0| {
                let two_pow_130 = pallas::Base::from_u128(1 << 65).square();
                let t_p = pallas::Base::from_u128(T_P);
                alpha_0 + two_pow_130 - t_p
            });
            let zs = self.lookup_config.witness_check(
                layouter.namespace(|| "Lookup range check alpha_0 + 2^130 - t_p"),
                alpha_0_prime,
                13,
                false,
            )?;
            let alpha_0_prime = zs[0].clone();

            (alpha_0_prime, zs[13].clone())
        };

        layouter.assign_region(
            || "Canonicity checks",
            |mut region| {
                // Activate canonicity check gate
                self.q_mul_fixed_base_field.enable(&mut region, 1)?;

                // Offset 0
                {
                    let offset = 0;

                    // Copy α
                    alpha.copy_advice(|| "Copy α", &mut region, self.canon_advices[0], offset)?;

                    // z_84_alpha = the top three bits of alpha.
                    z_84_alpha.copy_advice(
                        || "Copy z_84_alpha",
                        &mut region,
                        self.canon_advices[2],
                        offset,
                    )?;
                }

                // Offset 1
                {
                    let offset = 1;
                    // Copy alpha_0_prime = alpha_0 + 2^130 - t_p.
                    // We constrain this in the custom gate to be derived correctly.
                    alpha_0_prime.copy_advice(
                        || "Copy α_0 + 2^130 - t_p",
                        &mut region,
                        self.canon_advices[0],
                        offset,
                    )?;

                    // Decompose α into three pieces,
                    //     α = α_0 (252 bits)  || α_1 (2 bits) || α_2 (1 bit).
                    // We only need to witness α_1 and α_2. α_0 is derived in the gate.
                    // Witness α_1 = α[252..=253]
                    let alpha_1 = alpha.value().map(|alpha| bitrange_subset(alpha, 252..254));
                    region.assign_advice(
                        || "α_1 = α[252..=253]",
                        self.canon_advices[1],
                        offset,
                        || alpha_1,
                    )?;

                    // Witness the MSB α_2 = α[254]
                    let alpha_2 = alpha.value().map(|alpha| bitrange_subset(alpha, 254..255));
                    region.assign_advice(
                        || "α_2 = α[254]",
                        self.canon_advices[2],
                        offset,
                        || alpha_2,
                    )?;
                }

                // Offset 2
                {
                    let offset = 2;
                    // Copy z_13_alpha_0_prime
                    z_13_alpha_0_prime.copy_advice(
                        || "Copy z_13_alpha_0_prime",
                        &mut region,
                        self.canon_advices[0],
                        offset,
                    )?;

                    // Copy z_44_alpha
                    z_44_alpha.copy_advice(
                        || "Copy z_44_alpha",
                        &mut region,
                        self.canon_advices[1],
                        offset,
                    )?;

                    // Copy z_43_alpha
                    z_43_alpha.copy_advice(
                        || "Copy z_43_alpha",
                        &mut region,
                        self.canon_advices[2],
                        offset,
                    )?;
                }

                Ok(())
            },
        )?;

        Ok(result)
    }
}

#[cfg(test)]
pub mod tests {
    use group::{
        ff::{Field, PrimeField},
        Curve,
    };
    use halo2_proofs::{
        circuit::{Chip, Layouter, Value},
        plonk::Error,
    };
    use pasta_curves::pallas;
    use rand::rngs::OsRng;

    use crate::{
        ecc::{
            chip::{EccChip, FixedPoint, H},
            tests::{BaseField, TestFixedBases},
            FixedPointBaseField, NonIdentityPoint, Point,
        },
        utilities::UtilitiesInstructions,
    };

    pub(crate) fn test_mul_fixed_base_field(
        chip: EccChip<TestFixedBases>,
        mut layouter: impl Layouter<pallas::Base>,
    ) -> Result<(), Error> {
        test_single_base(
            chip.clone(),
            layouter.namespace(|| "base_field_elem"),
            FixedPointBaseField::from_inner(chip, BaseField),
            BaseField.generator(),
        )
    }

    #[allow(clippy::op_ref)]
    fn test_single_base(
        chip: EccChip<TestFixedBases>,
        mut layouter: impl Layouter<pallas::Base>,
        base: FixedPointBaseField<pallas::Affine, EccChip<TestFixedBases>>,
        base_val: pallas::Affine,
    ) -> Result<(), Error> {
        let rng = OsRng;

        let column = chip.config().advices[0];

        fn constrain_equal_non_id(
            chip: EccChip<TestFixedBases>,
            mut layouter: impl Layouter<pallas::Base>,
            base_val: pallas::Affine,
            scalar_val: pallas::Base,
            result: Point<pallas::Affine, EccChip<TestFixedBases>>,
        ) -> Result<(), Error> {
            // Move scalar from base field into scalar field (which always fits for Pallas).
            let scalar = pallas::Scalar::from_repr(scalar_val.to_repr()).unwrap();
            let expected = NonIdentityPoint::new(
                chip,
                layouter.namespace(|| "expected point"),
                Value::known((base_val * scalar).to_affine()),
            )?;
            result.constrain_equal(layouter.namespace(|| "constrain result"), &expected)
        }

        // [a]B
        {
            let scalar_fixed = pallas::Base::random(rng);
            let result = {
                let scalar_fixed = chip.load_private(
                    layouter.namespace(|| "random base field element"),
                    column,
                    Value::known(scalar_fixed),
                )?;
                base.mul(layouter.namespace(|| "random [a]B"), scalar_fixed)?
            };
            constrain_equal_non_id(
                chip.clone(),
                layouter.namespace(|| "random [a]B"),
                base_val,
                scalar_fixed,
                result,
            )?;
        }

        // There is a single canonical sequence of window values for which a doubling occurs on the last step:
        // 1333333333333333333333333333333333333333333333333333333333333333333333333333333333334 in octal.
        // (There is another *non-canonical* sequence
        // 5333333333333333333333333333333333333333332711161673731021062440252244051273333333333 in octal.)
        {
            let h = pallas::Base::from(H as u64);
            let scalar_fixed = "1333333333333333333333333333333333333333333333333333333333333333333333333333333333334"
                        .chars()
                        .fold(pallas::Base::zero(), |acc, c| {
                            acc * &h + &pallas::Base::from(c.to_digit(8).unwrap() as u64)
                        });
            let result = {
                let scalar_fixed = chip.load_private(
                    layouter.namespace(|| "mul with double"),
                    column,
                    Value::known(scalar_fixed),
                )?;
                base.mul(layouter.namespace(|| "mul with double"), scalar_fixed)?
            };
            constrain_equal_non_id(
                chip.clone(),
                layouter.namespace(|| "mul with double"),
                base_val,
                scalar_fixed,
                result,
            )?;
        }

        // [0]B should return (0,0) since it uses complete addition
        // on the last step.
        {
            let scalar_fixed = pallas::Base::zero();
            let result = {
                let scalar_fixed = chip.load_private(
                    layouter.namespace(|| "zero"),
                    column,
                    Value::known(scalar_fixed),
                )?;
                base.mul(layouter.namespace(|| "mul by zero"), scalar_fixed)?
            };
            result
                .inner()
                .is_identity()
                .assert_if_known(|is_identity| *is_identity);
        }

        // [-1]B is the largest base field element
        {
            let scalar_fixed = -pallas::Base::one();
            let result = {
                let scalar_fixed = chip.load_private(
                    layouter.namespace(|| "-1"),
                    column,
                    Value::known(scalar_fixed),
                )?;
                base.mul(layouter.namespace(|| "mul by -1"), scalar_fixed)?
            };
            constrain_equal_non_id(
                chip,
                layouter.namespace(|| "mul by -1"),
                base_val,
                scalar_fixed,
                result,
            )?;
        }

        Ok(())
    }
}
