//! Gadget and chip for a conditional swap utility.

use super::{bool_check, ternary, UtilitiesInstructions};

use group::ff::{Field, PrimeField};
use halo2_proofs::{
    circuit::{AssignedCell, Chip, Layouter, Value},
    plonk::{Advice, Column, ConstraintSystem, Constraints, Error, Selector},
    poly::Rotation,
};
use std::marker::PhantomData;

/// Instructions for a conditional swap gadget.
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
    use super::super::UtilitiesInstructions;
    use super::{CondSwapChip, CondSwapConfig, CondSwapInstructions};
    use group::ff::{Field, PrimeField};
    use halo2_proofs::{
        circuit::{Layouter, SimpleFloorPlanner, Value},
        dev::MockProver,
        plonk::{Circuit, ConstraintSystem, Error},
    };
    use pasta_curves::pallas::Base;
    use rand::rngs::OsRng;

    #[test]
    fn cond_swap() {
        #[derive(Default)]
        struct MyCircuit<F: Field> {
            a: Value<F>,
            b: Value<F>,
            swap: Value<bool>,
        }

        impl<F: PrimeField> Circuit<F> for MyCircuit<F> {
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
            let circuit: MyCircuit<Base> = MyCircuit {
                a: Value::known(Base::random(rng)),
                b: Value::known(Base::random(rng)),
                swap: Value::known(true),
            };
            let prover = MockProver::<Base>::run(3, &circuit, vec![]).unwrap();
            assert_eq!(prover.verify(), Ok(()));
        }

        // Test non-swap case
        {
            let circuit: MyCircuit<Base> = MyCircuit {
                a: Value::known(Base::random(rng)),
                b: Value::known(Base::random(rng)),
                swap: Value::known(false),
            };
            let prover = MockProver::<Base>::run(3, &circuit, vec![]).unwrap();
            assert_eq!(prover.verify(), Ok(()));
        }
    }
}
