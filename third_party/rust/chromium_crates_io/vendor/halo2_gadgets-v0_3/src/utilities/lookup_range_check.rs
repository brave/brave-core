//! Make use of a K-bit lookup table to decompose a field element into K-bit
//! words.

use halo2_proofs::{
    circuit::{AssignedCell, Layouter, Region},
    plonk::{Advice, Column, ConstraintSystem, Constraints, Error, Selector, TableColumn},
    poly::Rotation,
};
use std::{convert::TryInto, marker::PhantomData};

use ff::PrimeFieldBits;

use super::*;

/// The running sum $[z_0, ..., z_W]$. If created in strict mode, $z_W = 0$.
#[derive(Debug)]
pub struct RunningSum<F: PrimeFieldBits>(Vec<AssignedCell<F, F>>);
impl<F: PrimeFieldBits> std::ops::Deref for RunningSum<F> {
    type Target = Vec<AssignedCell<F, F>>;

    fn deref(&self) -> &Vec<AssignedCell<F, F>> {
        &self.0
    }
}

impl<F: PrimeFieldBits> RangeConstrained<F, AssignedCell<F, F>> {
    /// Witnesses a subset of the bits in `value` and constrains them to be the correct
    /// number of bits.
    ///
    /// # Panics
    ///
    /// Panics if `bitrange.len() >= K`.
    pub fn witness_short<const K: usize>(
        lookup_config: &LookupRangeCheckConfig<F, K>,
        layouter: impl Layouter<F>,
        value: Value<&F>,
        bitrange: Range<usize>,
    ) -> Result<Self, Error> {
        let num_bits = bitrange.len();
        assert!(num_bits < K);

        // Witness the subset and constrain it to be the correct number of bits.
        lookup_config
            .witness_short_check(
                layouter,
                value.map(|value| bitrange_subset(value, bitrange)),
                num_bits,
            )
            .map(|inner| Self {
                inner,
                num_bits,
                _phantom: PhantomData::default(),
            })
    }
}

/// Configuration that provides methods for a lookup range check.
#[derive(Eq, PartialEq, Debug, Clone, Copy)]
pub struct LookupRangeCheckConfig<F: PrimeFieldBits, const K: usize> {
    q_lookup: Selector,
    q_running: Selector,
    q_bitshift: Selector,
    running_sum: Column<Advice>,
    table_idx: TableColumn,
    _marker: PhantomData<F>,
}

impl<F: PrimeFieldBits, const K: usize> LookupRangeCheckConfig<F, K> {
    /// The `running_sum` advice column breaks the field element into `K`-bit
    /// words. It is used to construct the input expression to the lookup
    /// argument.
    ///
    /// The `table_idx` fixed column contains values from [0..2^K). Looking up
    /// a value in `table_idx` constrains it to be within this range. The table
    /// can be loaded outside this helper.
    ///
    /// # Side-effects
    ///
    /// Both the `running_sum` and `constants` columns will be equality-enabled.
    pub fn configure(
        meta: &mut ConstraintSystem<F>,
        running_sum: Column<Advice>,
        table_idx: TableColumn,
    ) -> Self {
        meta.enable_equality(running_sum);

        let q_lookup = meta.complex_selector();
        let q_running = meta.complex_selector();
        let q_bitshift = meta.selector();
        let config = LookupRangeCheckConfig {
            q_lookup,
            q_running,
            q_bitshift,
            running_sum,
            table_idx,
            _marker: PhantomData,
        };

        // https://p.z.cash/halo2-0.1:decompose-combined-lookup
        meta.lookup(|meta| {
            let q_lookup = meta.query_selector(config.q_lookup);
            let q_running = meta.query_selector(config.q_running);
            let z_cur = meta.query_advice(config.running_sum, Rotation::cur());

            // In the case of a running sum decomposition, we recover the word from
            // the difference of the running sums:
            //    z_i = 2^{K}⋅z_{i + 1} + a_i
            // => a_i = z_i - 2^{K}⋅z_{i + 1}
            let running_sum_lookup = {
                let running_sum_word = {
                    let z_next = meta.query_advice(config.running_sum, Rotation::next());
                    z_cur.clone() - z_next * F::from(1 << K)
                };

                q_running.clone() * running_sum_word
            };

            // In the short range check, the word is directly witnessed.
            let short_lookup = {
                let short_word = z_cur;
                let q_short = Expression::Constant(F::ONE) - q_running;

                q_short * short_word
            };

            // Combine the running sum and short lookups:
            vec![(
                q_lookup * (running_sum_lookup + short_lookup),
                config.table_idx,
            )]
        });

        // For short lookups, check that the word has been shifted by the correct number of bits.
        // https://p.z.cash/halo2-0.1:decompose-short-lookup
        meta.create_gate("Short lookup bitshift", |meta| {
            let q_bitshift = meta.query_selector(config.q_bitshift);
            let word = meta.query_advice(config.running_sum, Rotation::prev());
            let shifted_word = meta.query_advice(config.running_sum, Rotation::cur());
            let inv_two_pow_s = meta.query_advice(config.running_sum, Rotation::next());

            let two_pow_k = F::from(1 << K);

            // shifted_word = word * 2^{K-s}
            //              = word * 2^K * inv_two_pow_s
            Constraints::with_selector(
                q_bitshift,
                Some(word * two_pow_k * inv_two_pow_s - shifted_word),
            )
        });

        config
    }

    #[cfg(test)]
    // Loads the values [0..2^K) into `table_idx`. This is only used in testing
    // for now, since the Sinsemilla chip provides a pre-loaded table in the
    // Orchard context.
    pub fn load(&self, layouter: &mut impl Layouter<F>) -> Result<(), Error> {
        layouter.assign_table(
            || "table_idx",
            |mut table| {
                // We generate the row values lazily (we only need them during keygen).
                for index in 0..(1 << K) {
                    table.assign_cell(
                        || "table_idx",
                        self.table_idx,
                        index,
                        || Value::known(F::from(index as u64)),
                    )?;
                }
                Ok(())
            },
        )
    }

    /// Range check on an existing cell that is copied into this helper.
    ///
    /// Returns an error if `element` is not in a column that was passed to
    /// [`ConstraintSystem::enable_equality`] during circuit configuration.
    pub fn copy_check(
        &self,
        mut layouter: impl Layouter<F>,
        element: AssignedCell<F, F>,
        num_words: usize,
        strict: bool,
    ) -> Result<RunningSum<F>, Error> {
        layouter.assign_region(
            || format!("{:?} words range check", num_words),
            |mut region| {
                // Copy `element` and initialize running sum `z_0 = element` to decompose it.
                let z_0 = element.copy_advice(|| "z_0", &mut region, self.running_sum, 0)?;
                self.range_check(&mut region, z_0, num_words, strict)
            },
        )
    }

    /// Range check on a value that is witnessed in this helper.
    pub fn witness_check(
        &self,
        mut layouter: impl Layouter<F>,
        value: Value<F>,
        num_words: usize,
        strict: bool,
    ) -> Result<RunningSum<F>, Error> {
        layouter.assign_region(
            || "Witness element",
            |mut region| {
                let z_0 =
                    region.assign_advice(|| "Witness element", self.running_sum, 0, || value)?;
                self.range_check(&mut region, z_0, num_words, strict)
            },
        )
    }

    /// If `strict` is set to "true", the field element must fit into
    /// `num_words * K` bits. In other words, the the final cumulative sum `z_{num_words}`
    /// must be zero.
    ///
    /// If `strict` is set to "false", the final `z_{num_words}` is not constrained.
    ///
    /// `element` must have been assigned to `self.running_sum` at offset 0.
    fn range_check(
        &self,
        region: &mut Region<'_, F>,
        element: AssignedCell<F, F>,
        num_words: usize,
        strict: bool,
    ) -> Result<RunningSum<F>, Error> {
        // `num_words` must fit into a single field element.
        assert!(num_words * K <= F::CAPACITY as usize);
        let num_bits = num_words * K;

        // Chunk the first num_bits bits into K-bit words.
        let words = {
            // Take first num_bits bits of `element`.
            let bits = element.value().map(|element| {
                element
                    .to_le_bits()
                    .into_iter()
                    .take(num_bits)
                    .collect::<Vec<_>>()
            });

            bits.map(|bits| {
                bits.chunks_exact(K)
                    .map(|word| F::from(lebs2ip::<K>(&(word.try_into().unwrap()))))
                    .collect::<Vec<_>>()
            })
            .transpose_vec(num_words)
        };

        let mut zs = vec![element.clone()];

        // Assign cumulative sum such that
        //          z_i = 2^{K}⋅z_{i + 1} + a_i
        // => z_{i + 1} = (z_i - a_i) / (2^K)
        //
        // For `element` = a_0 + 2^10 a_1 + ... + 2^{120} a_{12}}, initialize z_0 = `element`.
        // If `element` fits in 130 bits, we end up with z_{13} = 0.
        let mut z = element;
        let inv_two_pow_k = F::from(1u64 << K).invert().unwrap();
        for (idx, word) in words.iter().enumerate() {
            // Enable q_lookup on this row
            self.q_lookup.enable(region, idx)?;
            // Enable q_running on this row
            self.q_running.enable(region, idx)?;

            // z_next = (z_cur - m_cur) / 2^K
            z = {
                let z_val = z
                    .value()
                    .zip(*word)
                    .map(|(z, word)| (*z - word) * inv_two_pow_k);

                // Assign z_next
                region.assign_advice(
                    || format!("z_{:?}", idx + 1),
                    self.running_sum,
                    idx + 1,
                    || z_val,
                )?
            };
            zs.push(z.clone());
        }

        if strict {
            // Constrain the final `z` to be zero.
            region.constrain_constant(zs.last().unwrap().cell(), F::ZERO)?;
        }

        Ok(RunningSum(zs))
    }

    /// Short range check on an existing cell that is copied into this helper.
    ///
    /// # Panics
    ///
    /// Panics if NUM_BITS is equal to or larger than K.
    pub fn copy_short_check(
        &self,
        mut layouter: impl Layouter<F>,
        element: AssignedCell<F, F>,
        num_bits: usize,
    ) -> Result<(), Error> {
        assert!(num_bits < K);
        layouter.assign_region(
            || format!("Range check {:?} bits", num_bits),
            |mut region| {
                // Copy `element` to use in the k-bit lookup.
                let element =
                    element.copy_advice(|| "element", &mut region, self.running_sum, 0)?;

                self.short_range_check(&mut region, element, num_bits)
            },
        )
    }

    /// Short range check on value that is witnessed in this helper.
    ///
    /// # Panics
    ///
    /// Panics if num_bits is larger than K.
    pub fn witness_short_check(
        &self,
        mut layouter: impl Layouter<F>,
        element: Value<F>,
        num_bits: usize,
    ) -> Result<AssignedCell<F, F>, Error> {
        assert!(num_bits <= K);
        layouter.assign_region(
            || format!("Range check {:?} bits", num_bits),
            |mut region| {
                // Witness `element` to use in the k-bit lookup.
                let element =
                    region.assign_advice(|| "Witness element", self.running_sum, 0, || element)?;

                self.short_range_check(&mut region, element.clone(), num_bits)?;

                Ok(element)
            },
        )
    }

    /// Constrain `x` to be a NUM_BITS word.
    ///
    /// `element` must have been assigned to `self.running_sum` at offset 0.
    fn short_range_check(
        &self,
        region: &mut Region<'_, F>,
        element: AssignedCell<F, F>,
        num_bits: usize,
    ) -> Result<(), Error> {
        // Enable lookup for `element`, to constrain it to 10 bits.
        self.q_lookup.enable(region, 0)?;

        // Enable lookup for shifted element, to constrain it to 10 bits.
        self.q_lookup.enable(region, 1)?;

        // Check element has been shifted by the correct number of bits.
        self.q_bitshift.enable(region, 1)?;

        // Assign shifted `element * 2^{K - num_bits}`
        let shifted = element.value().into_field() * F::from(1 << (K - num_bits));

        region.assign_advice(
            || format!("element * 2^({}-{})", K, num_bits),
            self.running_sum,
            1,
            || shifted,
        )?;

        // Assign 2^{-num_bits} from a fixed column.
        let inv_two_pow_s = F::from(1 << num_bits).invert().unwrap();
        region.assign_advice_from_constant(
            || format!("2^(-{})", num_bits),
            self.running_sum,
            2,
            inv_two_pow_s,
        )?;

        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::LookupRangeCheckConfig;

    use super::super::lebs2ip;
    use crate::sinsemilla::primitives::K;

    use ff::{Field, PrimeFieldBits};
    use halo2_proofs::{
        circuit::{Layouter, SimpleFloorPlanner, Value},
        dev::{FailureLocation, MockProver, VerifyFailure},
        plonk::{Circuit, ConstraintSystem, Error},
    };
    use pasta_curves::pallas;

    use std::{convert::TryInto, marker::PhantomData};

    #[test]
    fn lookup_range_check() {
        #[derive(Clone, Copy)]
        struct MyCircuit<F: PrimeFieldBits> {
            num_words: usize,
            _marker: PhantomData<F>,
        }

        impl<F: PrimeFieldBits> Circuit<F> for MyCircuit<F> {
            type Config = LookupRangeCheckConfig<F, K>;
            type FloorPlanner = SimpleFloorPlanner;

            fn without_witnesses(&self) -> Self {
                *self
            }

            fn configure(meta: &mut ConstraintSystem<F>) -> Self::Config {
                let running_sum = meta.advice_column();
                let table_idx = meta.lookup_table_column();
                let constants = meta.fixed_column();
                meta.enable_constant(constants);

                LookupRangeCheckConfig::<F, K>::configure(meta, running_sum, table_idx)
            }

            fn synthesize(
                &self,
                config: Self::Config,
                mut layouter: impl Layouter<F>,
            ) -> Result<(), Error> {
                // Load table_idx
                config.load(&mut layouter)?;

                // Lookup constraining element to be no longer than num_words * K bits.
                let elements_and_expected_final_zs = [
                    (F::from((1 << (self.num_words * K)) - 1), F::ZERO, true), // a word that is within self.num_words * K bits long
                    (F::from(1 << (self.num_words * K)), F::ONE, false), // a word that is just over self.num_words * K bits long
                ];

                fn expected_zs<F: PrimeFieldBits, const K: usize>(
                    element: F,
                    num_words: usize,
                ) -> Vec<F> {
                    let chunks = {
                        element
                            .to_le_bits()
                            .iter()
                            .by_vals()
                            .take(num_words * K)
                            .collect::<Vec<_>>()
                            .chunks_exact(K)
                            .map(|chunk| F::from(lebs2ip::<K>(chunk.try_into().unwrap())))
                            .collect::<Vec<_>>()
                    };
                    let expected_zs = {
                        let inv_two_pow_k = F::from(1 << K).invert().unwrap();
                        chunks.iter().fold(vec![element], |mut zs, a_i| {
                            // z_{i + 1} = (z_i - a_i) / 2^{K}
                            let z = (zs[zs.len() - 1] - a_i) * inv_two_pow_k;
                            zs.push(z);
                            zs
                        })
                    };
                    expected_zs
                }

                for (element, expected_final_z, strict) in elements_and_expected_final_zs.iter() {
                    let expected_zs = expected_zs::<F, K>(*element, self.num_words);

                    let zs = config.witness_check(
                        layouter.namespace(|| format!("Lookup {:?}", self.num_words)),
                        Value::known(*element),
                        self.num_words,
                        *strict,
                    )?;

                    assert_eq!(*expected_zs.last().unwrap(), *expected_final_z);

                    for (expected_z, z) in expected_zs.into_iter().zip(zs.iter()) {
                        z.value().assert_if_known(|z| &&expected_z == z);
                    }
                }
                Ok(())
            }
        }

        {
            let circuit: MyCircuit<pallas::Base> = MyCircuit {
                num_words: 6,
                _marker: PhantomData,
            };

            let prover = MockProver::<pallas::Base>::run(11, &circuit, vec![]).unwrap();
            assert_eq!(prover.verify(), Ok(()));
        }
    }

    #[test]
    fn short_range_check() {
        struct MyCircuit<F: PrimeFieldBits> {
            element: Value<F>,
            num_bits: usize,
        }

        impl<F: PrimeFieldBits> Circuit<F> for MyCircuit<F> {
            type Config = LookupRangeCheckConfig<F, K>;
            type FloorPlanner = SimpleFloorPlanner;

            fn without_witnesses(&self) -> Self {
                MyCircuit {
                    element: Value::unknown(),
                    num_bits: self.num_bits,
                }
            }

            fn configure(meta: &mut ConstraintSystem<F>) -> Self::Config {
                let running_sum = meta.advice_column();
                let table_idx = meta.lookup_table_column();
                let constants = meta.fixed_column();
                meta.enable_constant(constants);

                LookupRangeCheckConfig::<F, K>::configure(meta, running_sum, table_idx)
            }

            fn synthesize(
                &self,
                config: Self::Config,
                mut layouter: impl Layouter<F>,
            ) -> Result<(), Error> {
                // Load table_idx
                config.load(&mut layouter)?;

                // Lookup constraining element to be no longer than num_bits.
                config.witness_short_check(
                    layouter.namespace(|| format!("Lookup {:?} bits", self.num_bits)),
                    self.element,
                    self.num_bits,
                )?;

                Ok(())
            }
        }

        // Edge case: zero bits
        {
            let circuit: MyCircuit<pallas::Base> = MyCircuit {
                element: Value::known(pallas::Base::ZERO),
                num_bits: 0,
            };
            let prover = MockProver::<pallas::Base>::run(11, &circuit, vec![]).unwrap();
            assert_eq!(prover.verify(), Ok(()));
        }

        // Edge case: K bits
        {
            let circuit: MyCircuit<pallas::Base> = MyCircuit {
                element: Value::known(pallas::Base::from((1 << K) - 1)),
                num_bits: K,
            };
            let prover = MockProver::<pallas::Base>::run(11, &circuit, vec![]).unwrap();
            assert_eq!(prover.verify(), Ok(()));
        }

        // Element within `num_bits`
        {
            let circuit: MyCircuit<pallas::Base> = MyCircuit {
                element: Value::known(pallas::Base::from((1 << 6) - 1)),
                num_bits: 6,
            };
            let prover = MockProver::<pallas::Base>::run(11, &circuit, vec![]).unwrap();
            assert_eq!(prover.verify(), Ok(()));
        }

        // Element larger than `num_bits` but within K bits
        {
            let circuit: MyCircuit<pallas::Base> = MyCircuit {
                element: Value::known(pallas::Base::from(1 << 6)),
                num_bits: 6,
            };
            let prover = MockProver::<pallas::Base>::run(11, &circuit, vec![]).unwrap();
            assert_eq!(
                prover.verify(),
                Err(vec![VerifyFailure::Lookup {
                    lookup_index: 0,
                    location: FailureLocation::InRegion {
                        region: (1, "Range check 6 bits").into(),
                        offset: 1,
                    },
                }])
            );
        }

        // Element larger than K bits
        {
            let circuit: MyCircuit<pallas::Base> = MyCircuit {
                element: Value::known(pallas::Base::from(1 << K)),
                num_bits: 6,
            };
            let prover = MockProver::<pallas::Base>::run(11, &circuit, vec![]).unwrap();
            assert_eq!(
                prover.verify(),
                Err(vec![
                    VerifyFailure::Lookup {
                        lookup_index: 0,
                        location: FailureLocation::InRegion {
                            region: (1, "Range check 6 bits").into(),
                            offset: 0,
                        },
                    },
                    VerifyFailure::Lookup {
                        lookup_index: 0,
                        location: FailureLocation::InRegion {
                            region: (1, "Range check 6 bits").into(),
                            offset: 1,
                        },
                    },
                ])
            );
        }

        // Element which is not within `num_bits`, but which has a shifted value within
        // num_bits
        {
            let num_bits = 6;
            let shifted = pallas::Base::from((1 << num_bits) - 1);
            // Recall that shifted = element * 2^{K-s}
            //          => element = shifted * 2^{s-K}
            let element = shifted
                * pallas::Base::from(1 << (K as u64 - num_bits))
                    .invert()
                    .unwrap();
            let circuit: MyCircuit<pallas::Base> = MyCircuit {
                element: Value::known(element),
                num_bits: num_bits as usize,
            };
            let prover = MockProver::<pallas::Base>::run(11, &circuit, vec![]).unwrap();
            assert_eq!(
                prover.verify(),
                Err(vec![VerifyFailure::Lookup {
                    lookup_index: 0,
                    location: FailureLocation::InRegion {
                        region: (1, "Range check 6 bits").into(),
                        offset: 0,
                    },
                }])
            );
        }
    }
}
