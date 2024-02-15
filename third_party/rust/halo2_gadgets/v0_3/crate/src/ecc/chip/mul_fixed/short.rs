use std::convert::TryInto;

use super::super::{EccPoint, EccScalarFixedShort, FixedPoints, L_SCALAR_SHORT, NUM_WINDOWS_SHORT};
use crate::{ecc::chip::MagnitudeSign, utilities::bool_check};

use halo2_proofs::{
    circuit::{Layouter, Region},
    plonk::{ConstraintSystem, Constraints, Error, Expression, Selector},
    poly::Rotation,
};
use pasta_curves::pallas;

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Config<Fixed: FixedPoints<pallas::Affine>> {
    // Selector used for fixed-base scalar mul with short signed exponent.
    q_mul_fixed_short: Selector,
    super_config: super::Config<Fixed>,
}

impl<Fixed: FixedPoints<pallas::Affine>> Config<Fixed> {
    pub(crate) fn configure(
        meta: &mut ConstraintSystem<pallas::Base>,
        super_config: super::Config<Fixed>,
    ) -> Self {
        let config = Self {
            q_mul_fixed_short: meta.selector(),
            super_config,
        };

        config.create_gate(meta);

        config
    }

    fn create_gate(&self, meta: &mut ConstraintSystem<pallas::Base>) {
        // Gate contains the following constraints:
        // - https://p.z.cash/halo2-0.1:ecc-fixed-mul-short-msb
        // - https://p.z.cash/halo2-0.1:ecc-fixed-mul-short-conditional-neg
        meta.create_gate("Short fixed-base mul gate", |meta| {
            let q_mul_fixed_short = meta.query_selector(self.q_mul_fixed_short);
            let y_p = meta.query_advice(self.super_config.add_config.y_p, Rotation::cur());
            let y_a = meta.query_advice(self.super_config.add_config.y_qr, Rotation::cur());
            // z_21 = k_21
            let last_window = meta.query_advice(self.super_config.u, Rotation::cur());
            let sign = meta.query_advice(self.super_config.window, Rotation::cur());

            let one = Expression::Constant(pallas::Base::one());

            // Check that last window is either 0 or 1.
            let last_window_check = bool_check(last_window);
            // Check that sign is either 1 or -1.
            let sign_check = sign.clone().square() - one;

            // `(x_a, y_a)` is the result of `[m]B`, where `m` is the magnitude.
            // We conditionally negate this result using `y_p = y_a * s`, where `s` is the sign.

            // Check that the final `y_p = y_a` or `y_p = -y_a`
            //
            // This constraint is redundant / unnecessary, because `sign` is constrained
            // to -1 or 1 by `sign_check`, and `negation_check` therefore permits a strict
            // subset of the cases that this constraint permits.
            let y_check = (y_p.clone() - y_a.clone()) * (y_p.clone() + y_a.clone());

            // Check that the correct sign is witnessed s.t. sign * y_p = y_a
            let negation_check = sign * y_p - y_a;

            Constraints::with_selector(
                q_mul_fixed_short,
                [
                    ("last_window_check", last_window_check),
                    ("sign_check", sign_check),
                    ("y_check", y_check),
                    ("negation_check", negation_check),
                ],
            )
        });
    }

    /// Constraints `magnitude` to be at most 66 bits.
    ///
    /// The final window is separately constrained to be a single bit, which completes the
    /// 64-bit range constraint.
    fn decompose(
        &self,
        region: &mut Region<'_, pallas::Base>,
        offset: usize,
        magnitude_sign: MagnitudeSign,
    ) -> Result<EccScalarFixedShort, Error> {
        let (magnitude, sign) = magnitude_sign;

        // Decompose magnitude
        let running_sum = self.super_config.running_sum_config.copy_decompose(
            region,
            offset,
            magnitude.clone(),
            true,
            L_SCALAR_SHORT,
            NUM_WINDOWS_SHORT,
        )?;

        Ok(EccScalarFixedShort {
            magnitude,
            sign,
            running_sum: Some((*running_sum).as_slice().try_into().unwrap()),
        })
    }

    pub fn assign(
        &self,
        mut layouter: impl Layouter<pallas::Base>,
        scalar: &EccScalarFixedShort,
        base: &<Fixed as FixedPoints<pallas::Affine>>::ShortScalar,
    ) -> Result<(EccPoint, EccScalarFixedShort), Error>
    where
        <Fixed as FixedPoints<pallas::Affine>>::ShortScalar:
            super::super::FixedPoint<pallas::Affine>,
    {
        let (scalar, acc, mul_b) = layouter.assign_region(
            || "Short fixed-base mul (incomplete addition)",
            |mut region| {
                let offset = 0;

                // Decompose the scalar
                let scalar = match scalar.running_sum {
                    None => self.decompose(
                        &mut region,
                        offset,
                        (scalar.magnitude.clone(), scalar.sign.clone()),
                    ),
                    Some(_) => todo!("unimplemented for halo2_gadgets v0.1.0"),
                }?;

                let (acc, mul_b) = self
                    .super_config
                    .assign_region_inner::<_, NUM_WINDOWS_SHORT>(
                        &mut region,
                        offset,
                        &(&scalar).into(),
                        base,
                        self.super_config.running_sum_config.q_range_check(),
                    )?;

                Ok((scalar, acc, mul_b))
            },
        )?;

        // Last window
        let result = layouter.assign_region(
            || "Short fixed-base mul (most significant word)",
            |mut region| {
                let offset = 0;
                // Add to the cumulative sum to get `[magnitude]B`.
                let magnitude_mul = self.super_config.add_config.assign_region(
                    &mul_b.clone().into(),
                    &acc.clone().into(),
                    offset,
                    &mut region,
                )?;

                // Increase offset by 1 after complete addition
                let offset = offset + 1;

                // Copy sign to `window` column
                let sign = scalar.sign.copy_advice(
                    || "sign",
                    &mut region,
                    self.super_config.window,
                    offset,
                )?;

                // Copy last window to `u` column.
                // (Although the last window is not a `u` value; we are copying it into the `u`
                // column because there is an available cell there.)
                let z_21 = scalar.running_sum.as_ref().unwrap()[21].clone();
                z_21.copy_advice(|| "last_window", &mut region, self.super_config.u, offset)?;

                // Conditionally negate `y`-coordinate
                let y_val = sign.value().and_then(|sign| {
                    if sign == &-pallas::Base::one() {
                        -magnitude_mul.y.value()
                    } else {
                        magnitude_mul.y.value().cloned()
                    }
                });

                // Enable mul_fixed_short selector on final row
                self.q_mul_fixed_short.enable(&mut region, offset)?;

                // Assign final `y` to `y_p` column and return final point
                let y_var = region.assign_advice(
                    || "y_var",
                    self.super_config.add_config.y_p,
                    offset,
                    || y_val,
                )?;

                Ok(EccPoint::from_coordinates_unchecked(magnitude_mul.x, y_var))
            },
        )?;

        #[cfg(test)]
        // Check that the correct multiple is obtained.
        // This inlined test is only done for valid 64-bit magnitudes
        // and valid +/- 1 signs.
        // Invalid values result in constraint failures which are
        // tested at the circuit-level.
        {
            use super::super::FixedPoint;
            use group::{ff::PrimeField, Curve};

            scalar
                .magnitude
                .value()
                .zip(scalar.sign.value())
                .zip(result.point())
                .assert_if_known(|((magnitude, sign), result)| {
                    let magnitude_is_valid =
                        magnitude <= &&pallas::Base::from(0xFFFF_FFFF_FFFF_FFFFu64);
                    let sign_is_valid = sign.square() == pallas::Base::one();
                    // Only check the result if the magnitude and sign are valid.
                    !(magnitude_is_valid && sign_is_valid) || {
                        let scalar = {
                            // Move magnitude from base field into scalar field (which always fits
                            // for Pallas).
                            let magnitude = pallas::Scalar::from_repr(magnitude.to_repr()).unwrap();

                            let sign = if sign == &&pallas::Base::one() {
                                pallas::Scalar::one()
                            } else {
                                -pallas::Scalar::one()
                            };

                            magnitude * sign
                        };
                        let real_mul = base.generator() * scalar;

                        &real_mul.to_affine() == result
                    }
                });
        }

        Ok((result, scalar))
    }
}

#[cfg(test)]
pub mod tests {
    use group::{ff::PrimeField, Curve};
    use halo2_proofs::{
        arithmetic::CurveAffine,
        circuit::{AssignedCell, Chip, Layouter, Value},
        plonk::{Any, Error},
    };
    use pasta_curves::pallas;

    use crate::{
        ecc::{
            chip::{EccChip, FixedPoint, MagnitudeSign},
            tests::{Short, TestFixedBases},
            FixedPointShort, NonIdentityPoint, Point, ScalarFixedShort,
        },
        utilities::{lookup_range_check::LookupRangeCheckConfig, UtilitiesInstructions},
    };

    #[allow(clippy::op_ref)]
    pub(crate) fn test_mul_fixed_short(
        chip: EccChip<TestFixedBases>,
        mut layouter: impl Layouter<pallas::Base>,
    ) -> Result<(), Error> {
        // test_short
        let base_val = Short.generator();
        let test_short = FixedPointShort::from_inner(chip.clone(), Short);

        fn load_magnitude_sign(
            chip: EccChip<TestFixedBases>,
            mut layouter: impl Layouter<pallas::Base>,
            magnitude: pallas::Base,
            sign: pallas::Base,
        ) -> Result<MagnitudeSign, Error> {
            let column = chip.config().advices[0];
            let magnitude = chip.load_private(
                layouter.namespace(|| "magnitude"),
                column,
                Value::known(magnitude),
            )?;
            let sign =
                chip.load_private(layouter.namespace(|| "sign"), column, Value::known(sign))?;

            Ok((magnitude, sign))
        }

        fn constrain_equal_non_id(
            chip: EccChip<TestFixedBases>,
            mut layouter: impl Layouter<pallas::Base>,
            base_val: pallas::Affine,
            scalar_val: pallas::Scalar,
            result: Point<pallas::Affine, EccChip<TestFixedBases>>,
        ) -> Result<(), Error> {
            let expected = NonIdentityPoint::new(
                chip,
                layouter.namespace(|| "expected point"),
                Value::known((base_val * scalar_val).to_affine()),
            )?;
            result.constrain_equal(layouter.namespace(|| "constrain result"), &expected)
        }

        let magnitude_signs = [
            ("random [a]B", pallas::Base::from(rand::random::<u64>()), {
                let mut random_sign = pallas::Base::one();
                if rand::random::<bool>() {
                    random_sign = -random_sign;
                }
                random_sign
            }),
            (
                "[2^64 - 1]B",
                pallas::Base::from(0xFFFF_FFFF_FFFF_FFFFu64),
                pallas::Base::one(),
            ),
            (
                "-[2^64 - 1]B",
                pallas::Base::from(0xFFFF_FFFF_FFFF_FFFFu64),
                -pallas::Base::one(),
            ),
            // There is a single canonical sequence of window values for which a doubling occurs on the last step:
            // 1333333333333333333334 in octal.
            // [0xB6DB_6DB6_DB6D_B6DC] B
            (
                "mul_with_double",
                pallas::Base::from(0xB6DB_6DB6_DB6D_B6DCu64),
                pallas::Base::one(),
            ),
            (
                "mul_with_double negative",
                pallas::Base::from(0xB6DB_6DB6_DB6D_B6DCu64),
                -pallas::Base::one(),
            ),
        ];

        for (name, magnitude, sign) in magnitude_signs.iter() {
            let (result, _) = {
                let magnitude_sign = load_magnitude_sign(
                    chip.clone(),
                    layouter.namespace(|| *name),
                    *magnitude,
                    *sign,
                )?;
                let by = ScalarFixedShort::new(
                    chip.clone(),
                    layouter.namespace(|| "signed short scalar"),
                    magnitude_sign,
                )?;
                test_short.mul(layouter.namespace(|| *name), by)?
            };
            // Move from base field into scalar field
            let scalar = {
                let magnitude = pallas::Scalar::from_repr(magnitude.to_repr()).unwrap();
                let sign = if *sign == pallas::Base::one() {
                    pallas::Scalar::one()
                } else {
                    -pallas::Scalar::one()
                };
                magnitude * sign
            };
            constrain_equal_non_id(
                chip.clone(),
                layouter.namespace(|| *name),
                base_val,
                scalar,
                result,
            )?;
        }

        let zero_magnitude_signs = [
            ("mul by +zero", pallas::Base::zero(), pallas::Base::one()),
            ("mul by -zero", pallas::Base::zero(), -pallas::Base::one()),
        ];

        for (name, magnitude, sign) in zero_magnitude_signs.iter() {
            let (result, _) = {
                let magnitude_sign = load_magnitude_sign(
                    chip.clone(),
                    layouter.namespace(|| *name),
                    *magnitude,
                    *sign,
                )?;
                let by = ScalarFixedShort::new(
                    chip.clone(),
                    layouter.namespace(|| "signed short scalar"),
                    magnitude_sign,
                )?;
                test_short.mul(layouter.namespace(|| *name), by)?
            };
            result
                .inner()
                .is_identity()
                .assert_if_known(|is_identity| *is_identity);
        }

        Ok(())
    }

    #[test]
    fn invalid_magnitude_sign() {
        use crate::{
            ecc::chip::{EccConfig, FixedPoint},
            utilities::UtilitiesInstructions,
        };
        use halo2_proofs::{
            circuit::{Layouter, SimpleFloorPlanner},
            dev::{FailureLocation, MockProver, VerifyFailure},
            plonk::{Circuit, ConstraintSystem, Error},
        };

        #[derive(Default)]
        struct MyCircuit {
            magnitude: Value<pallas::Base>,
            sign: Value<pallas::Base>,
            // For test checking
            magnitude_error: Value<pallas::Base>,
        }

        impl UtilitiesInstructions<pallas::Base> for MyCircuit {
            type Var = AssignedCell<pallas::Base, pallas::Base>;
        }

        impl Circuit<pallas::Base> for MyCircuit {
            type Config = EccConfig<TestFixedBases>;
            type FloorPlanner = SimpleFloorPlanner;

            fn without_witnesses(&self) -> Self {
                Self::default()
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
                let column = config.advices[0];

                let short_config = config.mul_fixed_short.clone();
                let magnitude_sign = {
                    let magnitude = self.load_private(
                        layouter.namespace(|| "load magnitude"),
                        column,
                        self.magnitude,
                    )?;
                    let sign =
                        self.load_private(layouter.namespace(|| "load sign"), column, self.sign)?;
                    ScalarFixedShort::new(
                        EccChip::construct(config),
                        layouter.namespace(|| "signed short scalar"),
                        (magnitude, sign),
                    )?
                };

                short_config.assign(layouter, &magnitude_sign.inner, &Short)?;

                Ok(())
            }
        }

        // Copied from halo2_proofs::dev::util
        fn format_value(v: pallas::Base) -> String {
            use ff::Field;
            if v.is_zero_vartime() {
                "0".into()
            } else if v == pallas::Base::one() {
                "1".into()
            } else if v == -pallas::Base::one() {
                "-1".into()
            } else {
                // Format value as hex.
                let s = format!("{:?}", v);
                // Remove leading zeroes.
                let s = s.strip_prefix("0x").unwrap();
                let s = s.trim_start_matches('0');
                format!("0x{}", s)
            }
        }

        // Magnitude larger than 64 bits should fail
        {
            let circuits = [
                // 2^64
                MyCircuit {
                    magnitude: Value::known(pallas::Base::from_u128(1 << 64)),
                    sign: Value::known(pallas::Base::one()),
                    magnitude_error: Value::known(pallas::Base::from(1 << 1)),
                },
                // -2^64
                MyCircuit {
                    magnitude: Value::known(pallas::Base::from_u128(1 << 64)),
                    sign: Value::known(-pallas::Base::one()),
                    magnitude_error: Value::known(pallas::Base::from(1 << 1)),
                },
                // 2^66
                MyCircuit {
                    magnitude: Value::known(pallas::Base::from_u128(1 << 66)),
                    sign: Value::known(pallas::Base::one()),
                    magnitude_error: Value::known(pallas::Base::from(1 << 3)),
                },
                // -2^66
                MyCircuit {
                    magnitude: Value::known(pallas::Base::from_u128(1 << 66)),
                    sign: Value::known(-pallas::Base::one()),
                    magnitude_error: Value::known(pallas::Base::from(1 << 3)),
                },
                // 2^254
                MyCircuit {
                    magnitude: Value::known(pallas::Base::from_u128(1 << 127).square()),
                    sign: Value::known(pallas::Base::one()),
                    magnitude_error: Value::known(
                        pallas::Base::from_u128(1 << 95).square() * pallas::Base::from(2),
                    ),
                },
                // -2^254
                MyCircuit {
                    magnitude: Value::known(pallas::Base::from_u128(1 << 127).square()),
                    sign: Value::known(-pallas::Base::one()),
                    magnitude_error: Value::known(
                        pallas::Base::from_u128(1 << 95).square() * pallas::Base::from(2),
                    ),
                },
            ];

            for circuit in circuits.iter() {
                let prover = MockProver::<pallas::Base>::run(11, circuit, vec![]).unwrap();
                circuit.magnitude_error.assert_if_known(|magnitude_error| {
                    assert_eq!(
                        prover.verify(),
                        Err(vec![
                            VerifyFailure::ConstraintNotSatisfied {
                                constraint: (
                                    (17, "Short fixed-base mul gate").into(),
                                    0,
                                    "last_window_check",
                                )
                                    .into(),
                                location: FailureLocation::InRegion {
                                    region: (3, "Short fixed-base mul (most significant word)")
                                        .into(),
                                    offset: 1,
                                },
                                cell_values: vec![(
                                    ((Any::Advice, 5).into(), 0).into(),
                                    format_value(*magnitude_error),
                                )],
                            },
                            VerifyFailure::Permutation {
                                column: (Any::Fixed, 9).into(),
                                location: FailureLocation::OutsideRegion { row: 0 },
                            },
                            VerifyFailure::Permutation {
                                column: (Any::Advice, 4).into(),
                                location: FailureLocation::InRegion {
                                    region: (2, "Short fixed-base mul (incomplete addition)")
                                        .into(),
                                    offset: 22,
                                },
                            },
                        ])
                    );
                    true
                });
            }
        }

        // Sign that is not +/- 1 should fail
        {
            let magnitude_u64 = rand::random::<u64>();
            let circuit = MyCircuit {
                magnitude: Value::known(pallas::Base::from(magnitude_u64)),
                sign: Value::known(pallas::Base::zero()),
                magnitude_error: Value::unknown(),
            };

            let negation_check_y = {
                *(Short.generator() * pallas::Scalar::from(magnitude_u64))
                    .to_affine()
                    .coordinates()
                    .unwrap()
                    .y()
            };

            let prover = MockProver::<pallas::Base>::run(11, &circuit, vec![]).unwrap();
            assert_eq!(
                prover.verify(),
                Err(vec![
                    VerifyFailure::ConstraintNotSatisfied {
                        constraint: ((17, "Short fixed-base mul gate").into(), 1, "sign_check")
                            .into(),
                        location: FailureLocation::InRegion {
                            region: (3, "Short fixed-base mul (most significant word)").into(),
                            offset: 1,
                        },
                        cell_values: vec![(((Any::Advice, 4).into(), 0).into(), "0".to_string())],
                    },
                    VerifyFailure::ConstraintNotSatisfied {
                        constraint: (
                            (17, "Short fixed-base mul gate").into(),
                            3,
                            "negation_check"
                        )
                            .into(),
                        location: FailureLocation::InRegion {
                            region: (3, "Short fixed-base mul (most significant word)").into(),
                            offset: 1,
                        },
                        cell_values: vec![
                            (
                                ((Any::Advice, 1).into(), 0).into(),
                                format_value(negation_check_y),
                            ),
                            (
                                ((Any::Advice, 3).into(), 0).into(),
                                format_value(negation_check_y),
                            ),
                            (((Any::Advice, 4).into(), 0).into(), "0".to_string()),
                        ],
                    }
                ])
            );
        }
    }
}
