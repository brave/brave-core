//! Gadget and chip for conditional swap and mux utilities.

use super::{bool_check, ternary, UtilitiesInstructions};

use crate::ecc::chip::{EccPoint, NonIdentityEccPoint};
use group::ff::{Field, PrimeField};
use halo2_proofs::{
    circuit::{AssignedCell, Chip, Layouter, Value},
    plonk::{self, Advice, Column, ConstraintSystem, Constraints, Error, Selector},
    poly::Rotation,
};
use pasta_curves::pallas;
use std::marker::PhantomData;

/// Instructions for conditional swap and mux gadgets.
pub trait CondSwapInstructions<F: Field>: UtilitiesInstructions<F> {
    #[allow(clippy::type_complexity)]
    /// Given an input pair (a,b) and a `swap` boolean flag, returns
    /// (b,a) if `swap` is set, else (a,b) if `swap` is not set.
    ///
    /// The second element of the pair is required to be a witnessed
    /// value, not a variable that already exists in the circuit.
    fn swap(
        &self,
        layouter: impl Layouter<F>,
        pair: (Self::Var, Value<F>),
        swap: Value<bool>,
    ) -> Result<(Self::Var, Self::Var), Error>;

    /// Given an input `(choice, left, right)` where `choice` is a boolean flag,
    /// returns `left` if `choice` is not set and `right` if `choice` is set.
    fn mux(
        &self,
        layouter: &mut impl Layouter<F>,
        choice: Self::Var,
        left: Self::Var,
        right: Self::Var,
    ) -> Result<Self::Var, Error>;
}

/// A chip implementing a conditional swap.
#[derive(Clone, Debug)]
pub struct CondSwapChip<F> {
    config: CondSwapConfig,
    _marker: PhantomData<F>,
}

impl<F: Field> Chip<F> for CondSwapChip<F> {
    type Config = CondSwapConfig;
    type Loaded = ();

    fn config(&self) -> &Self::Config {
        &self.config
    }

    fn loaded(&self) -> &Self::Loaded {
        &()
    }
}

/// Configuration for the [`CondSwapChip`].
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct CondSwapConfig {
    q_swap: Selector,
    a: Column<Advice>,
    b: Column<Advice>,
    a_swapped: Column<Advice>,
    b_swapped: Column<Advice>,
    swap: Column<Advice>,
}

#[cfg(test)]
impl CondSwapConfig {
    pub(crate) fn a(&self) -> Column<Advice> {
        self.a
    }
}

impl<F: Field> UtilitiesInstructions<F> for CondSwapChip<F> {
    type Var = AssignedCell<F, F>;
}

impl<F: PrimeField> CondSwapInstructions<F> for CondSwapChip<F> {
    #[allow(clippy::type_complexity)]
    fn swap(
        &self,
        mut layouter: impl Layouter<F>,
        pair: (Self::Var, Value<F>),
        swap: Value<bool>,
    ) -> Result<(Self::Var, Self::Var), Error> {
        let config = self.config();

        layouter.assign_region(
            || "swap",
            |mut region| {
                // Enable `q_swap` selector
                config.q_swap.enable(&mut region, 0)?;

                // Copy in `a` value
                let a = pair.0.copy_advice(|| "copy a", &mut region, config.a, 0)?;

                // Witness `b` value
                let b = region.assign_advice(|| "witness b", config.b, 0, || pair.1)?;

                // Witness `swap` value
                let swap_val = swap.map(|swap| F::from(swap as u64));
                region.assign_advice(|| "swap", config.swap, 0, || swap_val)?;

                // Conditionally swap a
                let a_swapped = {
                    let a_swapped = a
                        .value()
                        .zip(b.value())
                        .zip(swap)
                        .map(|((a, b), swap)| if swap { b } else { a })
                        .cloned();
                    region.assign_advice(|| "a_swapped", config.a_swapped, 0, || a_swapped)?
                };

                // Conditionally swap b
                let b_swapped = {
                    let b_swapped = a
                        .value()
                        .zip(b.value())
                        .zip(swap)
                        .map(|((a, b), swap)| if swap { a } else { b })
                        .cloned();
                    region.assign_advice(|| "b_swapped", config.b_swapped, 0, || b_swapped)?
                };

                // Return swapped pair
                Ok((a_swapped, b_swapped))
            },
        )
    }

    fn mux(
        &self,
        layouter: &mut impl Layouter<F>,
        choice: Self::Var,
        left: Self::Var,
        right: Self::Var,
    ) -> Result<Self::Var, Error> {
        let config = self.config();

        layouter.assign_region(
            || "mux",
            |mut region| {
                // Enable `q_swap` selector
                config.q_swap.enable(&mut region, 0)?;

                // Copy in `a` value
                let left = left.copy_advice(|| "copy left", &mut region, config.a, 0)?;

                // Copy in `b` value
                let right = right.copy_advice(|| "copy right", &mut region, config.b, 0)?;

                // Copy `choice` value
                let choice = choice.copy_advice(|| "copy choice", &mut region, config.swap, 0)?;

                let a_swapped = left
                    .value()
                    .zip(right.value())
                    .zip(choice.value())
                    .map(|((left, right), choice)| {
                        if *choice == F::from(0_u64) {
                            left
                        } else {
                            right
                        }
                    })
                    .cloned();
                let b_swapped = left
                    .value()
                    .zip(right.value())
                    .zip(choice.value())
                    .map(|((left, right), choice)| {
                        if *choice == F::from(0_u64) {
                            right
                        } else {
                            left
                        }
                    })
                    .cloned();

                region.assign_advice(|| "out b_swap", self.config.b_swapped, 0, || b_swapped)?;
                region.assign_advice(|| "out a_swap", self.config.a_swapped, 0, || a_swapped)
            },
        )
    }
}

impl CondSwapChip<pallas::Base> {
    /// Given an input `(choice, left, right)` where `choice` is a boolean flag and `left` and `right` are `EccPoint`,
    /// returns `left` if `choice` is not set and `right` if `choice` is set.
    pub fn mux_on_points(
        &self,
        mut layouter: impl Layouter<pallas::Base>,
        choice: &AssignedCell<pallas::Base, pallas::Base>,
        left: &EccPoint,
        right: &EccPoint,
    ) -> Result<EccPoint, plonk::Error> {
        let x_cell = self.mux(&mut layouter, choice.clone(), left.x(), right.x())?;
        let y_cell = self.mux(&mut layouter, choice.clone(), left.y(), right.y())?;
        Ok(EccPoint::from_coordinates_unchecked(
            x_cell.into(),
            y_cell.into(),
        ))
    }

    /// Given an input `(choice, left, right)` where `choice` is a boolean flag and `left` and `right` are
    /// `NonIdentityEccPoint`, returns `left` if `choice` is not set and `right` if `choice` is set.
    pub fn mux_on_non_identity_points(
        &self,
        mut layouter: impl Layouter<pallas::Base>,
        choice: &AssignedCell<pallas::Base, pallas::Base>,
        left: &NonIdentityEccPoint,
        right: &NonIdentityEccPoint,
    ) -> Result<NonIdentityEccPoint, plonk::Error> {
        let x_cell = self.mux(&mut layouter, choice.clone(), left.x(), right.x())?;
        let y_cell = self.mux(&mut layouter, choice.clone(), left.y(), right.y())?;
        Ok(NonIdentityEccPoint::from_coordinates_unchecked(
            x_cell.into(),
            y_cell.into(),
        ))
    }
}

impl<F: PrimeField> CondSwapChip<F> {
    /// Configures this chip for use in a circuit.
    ///
    /// # Side-effects
    ///
    /// `advices[0]` will be equality-enabled.
    pub fn configure(
        meta: &mut ConstraintSystem<F>,
        advices: [Column<Advice>; 5],
    ) -> CondSwapConfig {
        let a = advices[0];
        // Only column a is used in an equality constraint directly by this chip.
        meta.enable_equality(a);

        let q_swap = meta.selector();

        let config = CondSwapConfig {
            q_swap,
            a,
            b: advices[1],
            a_swapped: advices[2],
            b_swapped: advices[3],
            swap: advices[4],
        };

        // TODO: optimise shape of gate for Merkle path validation

        meta.create_gate("a' = b ⋅ swap + a ⋅ (1-swap)", |meta| {
            let q_swap = meta.query_selector(q_swap);

            let a = meta.query_advice(config.a, Rotation::cur());
            let b = meta.query_advice(config.b, Rotation::cur());
            let a_swapped = meta.query_advice(config.a_swapped, Rotation::cur());
            let b_swapped = meta.query_advice(config.b_swapped, Rotation::cur());
            let swap = meta.query_advice(config.swap, Rotation::cur());

            // This checks that `a_swapped` is equal to `b` when `swap` is set,
            // but remains as `a` when `swap` is not set.
            let a_check = a_swapped - ternary(swap.clone(), b.clone(), a.clone());

            // This checks that `b_swapped` is equal to `a` when `swap` is set,
            // but remains as `b` when `swap` is not set.
            let b_check = b_swapped - ternary(swap.clone(), a, b);

            // Check `swap` is boolean.
            let bool_check = bool_check(swap);

            Constraints::with_selector(
                q_swap,
                [
                    ("a check", a_check),
                    ("b check", b_check),
                    ("swap is bool", bool_check),
                ],
            )
        });

        config
    }

    /// Constructs a [`CondSwapChip`] given a [`CondSwapConfig`].
    pub fn construct(config: CondSwapConfig) -> Self {
        CondSwapChip {
            config,
            _marker: PhantomData,
        }
    }
}

#[cfg(test)]
mod tests {
    use crate::utilities::{
        cond_swap::{CondSwapChip, CondSwapConfig, CondSwapInstructions},
        lookup_range_check::{
            PallasLookupRangeCheck, PallasLookupRangeCheck4_5BConfig, PallasLookupRangeCheckConfig,
        },
        UtilitiesInstructions,
    };
    use group::ff::{Field, PrimeField};
    use halo2_proofs::{
        circuit::{Layouter, SimpleFloorPlanner, Value},
        dev::MockProver,
        plonk::{Circuit, ConstraintSystem, Error},
    };
    use pasta_curves::pallas::Base;
    use rand::rngs::OsRng;
    use std::marker::PhantomData;

    #[test]
    fn cond_swap() {
        #[derive(Default)]
        struct MyCondSwapCircuit<F: Field> {
            a: Value<F>,
            b: Value<F>,
            swap: Value<bool>,
        }

        impl<F: PrimeField> Circuit<F> for MyCondSwapCircuit<F> {
            type Config = CondSwapConfig;
            type FloorPlanner = SimpleFloorPlanner;

            fn without_witnesses(&self) -> Self {
                Self::default()
            }

            fn configure(meta: &mut ConstraintSystem<F>) -> Self::Config {
                let advices = [
                    meta.advice_column(),
                    meta.advice_column(),
                    meta.advice_column(),
                    meta.advice_column(),
                    meta.advice_column(),
                ];

                CondSwapChip::<F>::configure(meta, advices)
            }

            fn synthesize(
                &self,
                config: Self::Config,
                mut layouter: impl Layouter<F>,
            ) -> Result<(), Error> {
                let chip = CondSwapChip::<F>::construct(config.clone());

                // Load the pair and the swap flag into the circuit.
                let a = chip.load_private(layouter.namespace(|| "a"), config.a, self.a)?;
                // Return the swapped pair.
                let swapped_pair = chip.swap(
                    layouter.namespace(|| "swap"),
                    (a.clone(), self.b),
                    self.swap,
                )?;

                self.swap
                    .zip(a.value().zip(self.b.as_ref()))
                    .zip(swapped_pair.0.value().zip(swapped_pair.1.value()))
                    .assert_if_known(|((swap, (a, b)), (a_swapped, b_swapped))| {
                        if *swap {
                            // Check that `a` and `b` have been swapped
                            (a_swapped == b) && (b_swapped == a)
                        } else {
                            // Check that `a` and `b` have not been swapped
                            (a_swapped == a) && (b_swapped == b)
                        }
                    });

                Ok(())
            }
        }

        let rng = OsRng;

        // Test swap case
        {
            let circuit: MyCondSwapCircuit<Base> = MyCondSwapCircuit {
                a: Value::known(Base::random(rng)),
                b: Value::known(Base::random(rng)),
                swap: Value::known(true),
            };
            let prover = MockProver::<Base>::run(3, &circuit, vec![]).unwrap();
            assert_eq!(prover.verify(), Ok(()));
        }

        // Test non-swap case
        {
            let circuit: MyCondSwapCircuit<Base> = MyCondSwapCircuit {
                a: Value::known(Base::random(rng)),
                b: Value::known(Base::random(rng)),
                swap: Value::known(false),
            };
            let prover = MockProver::<Base>::run(3, &circuit, vec![]).unwrap();
            assert_eq!(prover.verify(), Ok(()));
        }
    }

    #[test]
    fn test_mux() {
        use crate::ecc::{
            chip::{EccChip, EccConfig},
            tests::TestFixedBases,
            CircuitVersion, NonIdentityPoint, Point,
        };

        use group::{cofactor::CofactorCurveAffine, Curve, Group};
        use halo2_proofs::{
            circuit::{Layouter, SimpleFloorPlanner, Value},
            dev::MockProver,
            plonk::{Advice, Circuit, Column, ConstraintSystem, Error, Instance},
        };
        use pasta_curves::{arithmetic::CurveAffine, pallas, EpAffine};

        use rand::rngs::OsRng;

        #[derive(Clone, Debug)]
        pub struct MuxConfig<Lookup: PallasLookupRangeCheck> {
            primary: Column<Instance>,
            advice: Column<Advice>,
            cond_swap_config: CondSwapConfig,
            ecc_config: EccConfig<TestFixedBases, Lookup>,
        }

        #[derive(Default)]
        struct MyMuxCircuit<Lookup: PallasLookupRangeCheck> {
            left_point: Value<EpAffine>,
            right_point: Value<EpAffine>,
            choice: Value<pallas::Base>,
            _lookup_marker: PhantomData<Lookup>,
        }

        impl<Lookup: PallasLookupRangeCheck> MyMuxCircuit<Lookup> {
            fn new(
                left_point: Value<EpAffine>,
                right_point: Value<EpAffine>,
                choice: Value<pallas::Base>,
            ) -> Self {
                MyMuxCircuit {
                    left_point,
                    right_point,
                    choice,
                    _lookup_marker: PhantomData,
                }
            }
        }

        impl<Lookup: PallasLookupRangeCheck> Circuit<pallas::Base> for MyMuxCircuit<Lookup> {
            type Config = MuxConfig<Lookup>;
            type FloorPlanner = SimpleFloorPlanner;

            fn without_witnesses(&self) -> Self {
                MyMuxCircuit::<Lookup>::new(Value::default(), Value::default(), Value::default())
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

                for advice in advices.iter() {
                    meta.enable_equality(*advice);
                }

                // Instance column used for public inputs
                let primary = meta.instance_column();
                meta.enable_equality(primary);

                let cond_swap_config =
                    CondSwapChip::configure(meta, advices[0..5].try_into().unwrap());

                let table_idx = meta.lookup_table_column();

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
                meta.enable_constant(lagrange_coeffs[0]);

                let range_check = Lookup::configure(meta, advices[9], table_idx);

                let ecc_config = EccChip::<TestFixedBases, Lookup>::configure(
                    meta,
                    advices,
                    lagrange_coeffs,
                    range_check,
                );

                MuxConfig {
                    primary,
                    advice: advices[0],
                    cond_swap_config,
                    ecc_config,
                }
            }

            fn synthesize(
                &self,
                config: Self::Config,
                mut layouter: impl Layouter<pallas::Base>,
            ) -> Result<(), Error> {
                // Construct a CondSwap chip
                let cond_swap_chip = CondSwapChip::construct(config.cond_swap_config);

                // Construct an ECC chip
                let ecc_chip = EccChip::<TestFixedBases, Lookup>::construct(
                    config.ecc_config,
                    CircuitVersion::AnchoredBase,
                );

                // Assign choice
                let choice = layouter.assign_region(
                    || "load private",
                    |mut region| {
                        region.assign_advice(|| "load private", config.advice, 0, || self.choice)
                    },
                )?;

                // Test mux on non identity points
                // Assign left point
                let left_non_identity_point = NonIdentityPoint::new(
                    ecc_chip.clone(),
                    layouter.namespace(|| "left point"),
                    self.left_point.map(|left_point| left_point),
                )?;

                // Assign right point
                let right_non_identity_point = NonIdentityPoint::new(
                    ecc_chip.clone(),
                    layouter.namespace(|| "right point"),
                    self.right_point.map(|right_point| right_point),
                )?;

                // Apply mux
                let result_non_identity_point = cond_swap_chip.mux_on_non_identity_points(
                    layouter.namespace(|| "MUX"),
                    &choice,
                    left_non_identity_point.inner(),
                    right_non_identity_point.inner(),
                )?;

                // Check equality with instance
                layouter.constrain_instance(
                    result_non_identity_point.x().cell(),
                    config.primary,
                    0,
                )?;
                layouter.constrain_instance(
                    result_non_identity_point.y().cell(),
                    config.primary,
                    1,
                )?;

                // Test mux on points
                // Assign left point
                let left_point = Point::new(
                    ecc_chip.clone(),
                    layouter.namespace(|| "left point"),
                    self.left_point.map(|left_point| left_point),
                )?;

                // Assign right point
                let right_point = Point::new(
                    ecc_chip,
                    layouter.namespace(|| "right point"),
                    self.right_point.map(|right_point| right_point),
                )?;

                // Apply mux
                let result = cond_swap_chip.mux_on_points(
                    layouter.namespace(|| "MUX"),
                    &choice,
                    left_point.inner(),
                    right_point.inner(),
                )?;

                // Check equality with instance
                layouter.constrain_instance(result.x().cell(), config.primary, 0)?;
                layouter.constrain_instance(result.y().cell(), config.primary, 1)
            }
        }

        impl<Lookup: PallasLookupRangeCheck> MyMuxCircuit<Lookup> {
            fn test_mux_circuits() {
                // Test different circuits
                let mut circuits = vec![];
                let mut instances = vec![];
                for choice in [false, true] {
                    let choice_value = if choice {
                        pallas::Base::one()
                    } else {
                        pallas::Base::zero()
                    };
                    let left_point = pallas::Point::random(OsRng).to_affine();
                    let right_point = pallas::Point::random(OsRng).to_affine();
                    circuits.push(MyMuxCircuit::<Lookup> {
                        left_point: Value::known(left_point),
                        right_point: Value::known(right_point),
                        choice: Value::known(choice_value),
                        _lookup_marker: PhantomData,
                    });
                    let expected_output = if choice { right_point } else { left_point };
                    let (expected_x, expected_y) = if bool::from(expected_output.is_identity()) {
                        (pallas::Base::zero(), pallas::Base::zero())
                    } else {
                        let coords = expected_output.coordinates().unwrap();
                        (*coords.x(), *coords.y())
                    };
                    instances.push([[expected_x, expected_y]]);
                }

                for (circuit, instance) in circuits.iter().zip(instances.iter()) {
                    let prover = MockProver::<pallas::Base>::run(
                        5,
                        circuit,
                        instance.iter().map(|p| p.to_vec()).collect(),
                    )
                    .unwrap();
                    assert_eq!(prover.verify(), Ok(()));
                }
            }
        }

        MyMuxCircuit::<PallasLookupRangeCheckConfig>::test_mux_circuits();
        MyMuxCircuit::<PallasLookupRangeCheck4_5BConfig>::test_mux_circuits();
    }
}
