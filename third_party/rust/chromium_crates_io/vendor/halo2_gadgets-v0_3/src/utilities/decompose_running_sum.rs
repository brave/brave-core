//! Decomposes an $n$-bit field element $\alpha$ into $W$ windows, each window
//! being a $K$-bit word, using a running sum $z$.
//! We constrain $K \leq 3$ for this helper.
//!     $$\alpha = k_0 + (2^K) k_1 + (2^{2K}) k_2 + ... + (2^{(W-1)K}) k_{W-1}$$
//!
//! $z_0$ is initialized as $\alpha$. Each successive $z_{i+1}$ is computed as
//!                $$z_{i+1} = (z_{i} - k_i) / (2^K).$$
//! $z_W$ is constrained to be zero.
//! The difference between each interstitial running sum output is constrained
//! to be $K$ bits, i.e.
//!                      `range_check`($k_i$, $2^K$),
//! where
//! ```text
//!   range_check(word, range)
//!     = word * (1 - word) * (2 - word) * ... * ((range - 1) - word)
//! ```
//!
//! Given that the `range_check` constraint will be toggled by a selector, in
//! practice we will have a `selector * range_check(word, range)` expression
//! of degree `range + 1`.
//!
//! This means that $2^K$ has to be at most `degree_bound - 1` in order for
//! the range check constraint to stay within the degree bound.

use ff::PrimeFieldBits;
use halo2_proofs::{
    circuit::{AssignedCell, Region, Value},
    plonk::{Advice, Column, ConstraintSystem, Constraints, Error, Selector},
    poly::Rotation,
};

use super::range_check;

use std::marker::PhantomData;

/// The running sum $[z_0, ..., z_W]$. If created in strict mode, $z_W = 0$.
#[derive(Debug)]
pub struct RunningSum<F: PrimeFieldBits>(Vec<AssignedCell<F, F>>);
impl<F: PrimeFieldBits> std::ops::Deref for RunningSum<F> {
    type Target = Vec<AssignedCell<F, F>>;

    fn deref(&self) -> &Vec<AssignedCell<F, F>> {
        &self.0
    }
}

/// Configuration that provides methods for running sum decomposition.
#[derive(Debug, Clone, Copy, Eq, PartialEq)]
pub struct RunningSumConfig<F: PrimeFieldBits, const WINDOW_NUM_BITS: usize> {
    q_range_check: Selector,
    z: Column<Advice>,
    _marker: PhantomData<F>,
}

impl<F: PrimeFieldBits, const WINDOW_NUM_BITS: usize> RunningSumConfig<F, WINDOW_NUM_BITS> {
    /// Returns the q_range_check selector of this [`RunningSumConfig`].
    pub(crate) fn q_range_check(&self) -> Selector {
        self.q_range_check
    }

    /// `perm` MUST include the advice column `z`.
    ///
    /// # Panics
    ///
    /// Panics if WINDOW_NUM_BITS > 3.
    ///
    /// # Side-effects
    ///
    /// `z` will be equality-enabled.
    pub fn configure(
        meta: &mut ConstraintSystem<F>,
        q_range_check: Selector,
        z: Column<Advice>,
    ) -> Self {
        assert!(WINDOW_NUM_BITS <= 3);

        meta.enable_equality(z);

        let config = Self {
            q_range_check,
            z,
            _marker: PhantomData,
        };

        // https://p.z.cash/halo2-0.1:decompose-short-range
        meta.create_gate("range check", |meta| {
            let q_range_check = meta.query_selector(config.q_range_check);
            let z_cur = meta.query_advice(config.z, Rotation::cur());
            let z_next = meta.query_advice(config.z, Rotation::next());
            //    z_i = 2^{K}⋅z_{i + 1} + k_i
            // => k_i = z_i - 2^{K}⋅z_{i + 1}
            let word = z_cur - z_next * F::from(1 << WINDOW_NUM_BITS);

            Constraints::with_selector(q_range_check, Some(range_check(word, 1 << WINDOW_NUM_BITS)))
        });

        config
    }

    /// Decompose a field element alpha that is witnessed in this helper.
    ///
    /// `strict` = true constrains the final running sum to be zero, i.e.
    /// constrains alpha to be within WINDOW_NUM_BITS * num_windows bits.
    pub fn witness_decompose(
        &self,
        region: &mut Region<'_, F>,
        offset: usize,
        alpha: Value<F>,
        strict: bool,
        word_num_bits: usize,
        num_windows: usize,
    ) -> Result<RunningSum<F>, Error> {
        let z_0 = region.assign_advice(|| "z_0 = alpha", self.z, offset, || alpha)?;
        self.decompose(region, offset, z_0, strict, word_num_bits, num_windows)
    }

    /// Decompose an existing variable alpha that is copied into this helper.
    ///
    /// `strict` = true constrains the final running sum to be zero, i.e.
    /// constrains alpha to be within WINDOW_NUM_BITS * num_windows bits.
    pub fn copy_decompose(
        &self,
        region: &mut Region<'_, F>,
        offset: usize,
        alpha: AssignedCell<F, F>,
        strict: bool,
        word_num_bits: usize,
        num_windows: usize,
    ) -> Result<RunningSum<F>, Error> {
        let z_0 = alpha.copy_advice(|| "copy z_0 = alpha", region, self.z, offset)?;
        self.decompose(region, offset, z_0, strict, word_num_bits, num_windows)
    }

    /// `z_0` must be the cell at `(self.z, offset)` in `region`.
    ///
    /// # Panics
    ///
    /// Panics if there are too many windows for the given word size.
    fn decompose(
        &self,
        region: &mut Region<'_, F>,
        offset: usize,
        z_0: AssignedCell<F, F>,
        strict: bool,
        word_num_bits: usize,
        num_windows: usize,
    ) -> Result<RunningSum<F>, Error> {
        // Make sure that we do not have more windows than required for the number
        // of bits in the word. In other words, every window must contain at least
        // one bit of the word (no empty windows).
        //
        // For example, let:
        //      - word_num_bits = 64
        //      - WINDOW_NUM_BITS = 3
        // In this case, the maximum allowed num_windows is 22:
        //                    3 * 22 < 64 + 3
        //
        assert!(WINDOW_NUM_BITS * num_windows < word_num_bits + WINDOW_NUM_BITS);

        // Enable selectors
        for idx in 0..num_windows {
            self.q_range_check.enable(region, offset + idx)?;
        }

        // Decompose base field element into K-bit words.
        let words = z_0
            .value()
            .map(|word| super::decompose_word::<F>(word, word_num_bits, WINDOW_NUM_BITS))
            .transpose_vec(num_windows);

        // Initialize empty vector to store running sum values [z_0, ..., z_W].
        let mut zs: Vec<AssignedCell<F, F>> = vec![z_0.clone()];
        let mut z = z_0;

        // Assign running sum `z_{i+1}` = (z_i - k_i) / (2^K) for i = 0..=n-1.
        // Outside of this helper, z_0 = alpha must have already been loaded into the
        // `z` column at `offset`.
        let two_pow_k_inv = Value::known(F::from(1 << WINDOW_NUM_BITS as u64).invert().unwrap());
        for (i, word) in words.iter().enumerate() {
            // z_next = (z_cur - word) / (2^K)
            let z_next = {
                let z_cur_val = z.value().copied();
                let word = word.map(|word| F::from(word as u64));
                let z_next_val = (z_cur_val - word) * two_pow_k_inv;
                region.assign_advice(
                    || format!("z_{:?}", i + 1),
                    self.z,
                    offset + i + 1,
                    || z_next_val,
                )?
            };

            // Update `z`.
            z = z_next;
            zs.push(z.clone());
        }
        assert_eq!(zs.len(), num_windows + 1);

        if strict {
            // Constrain the final running sum output to be zero.
            region.constrain_constant(zs.last().unwrap().cell(), F::ZERO)?;
        }

        Ok(RunningSum(zs))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use group::ff::{Field, PrimeField};
    use halo2_proofs::{
        circuit::{Layouter, SimpleFloorPlanner},
        dev::{FailureLocation, MockProver, VerifyFailure},
        plonk::{Any, Circuit, ConstraintSystem, Error},
    };
    use pasta_curves::pallas;
    use rand::rngs::OsRng;

    use crate::ecc::chip::{
        FIXED_BASE_WINDOW_SIZE, L_SCALAR_SHORT as L_SHORT, NUM_WINDOWS, NUM_WINDOWS_SHORT,
    };

    const L_BASE: usize = pallas::Base::NUM_BITS as usize;

    #[test]
    fn test_running_sum() {
        struct MyCircuit<
            F: PrimeFieldBits,
            const WORD_NUM_BITS: usize,
            const WINDOW_NUM_BITS: usize,
            const NUM_WINDOWS: usize,
        > {
            alpha: Value<F>,
            strict: bool,
        }

        impl<
                F: PrimeFieldBits,
                const WORD_NUM_BITS: usize,
                const WINDOW_NUM_BITS: usize,
                const NUM_WINDOWS: usize,
            > Circuit<F> for MyCircuit<F, WORD_NUM_BITS, WINDOW_NUM_BITS, NUM_WINDOWS>
        {
            type Config = RunningSumConfig<F, WINDOW_NUM_BITS>;
            type FloorPlanner = SimpleFloorPlanner;

            fn without_witnesses(&self) -> Self {
                Self {
                    alpha: Value::unknown(),
                    strict: self.strict,
                }
            }

            fn configure(meta: &mut ConstraintSystem<F>) -> Self::Config {
                let z = meta.advice_column();
                let q_range_check = meta.selector();
                let constants = meta.fixed_column();
                meta.enable_constant(constants);

                RunningSumConfig::<F, WINDOW_NUM_BITS>::configure(meta, q_range_check, z)
            }

            fn synthesize(
                &self,
                config: Self::Config,
                mut layouter: impl Layouter<F>,
            ) -> Result<(), Error> {
                layouter.assign_region(
                    || "decompose",
                    |mut region| {
                        let offset = 0;
                        let zs = config.witness_decompose(
                            &mut region,
                            offset,
                            self.alpha,
                            self.strict,
                            WORD_NUM_BITS,
                            NUM_WINDOWS,
                        )?;
                        let alpha = zs[0].clone();

                        let offset = offset + NUM_WINDOWS + 1;

                        config.copy_decompose(
                            &mut region,
                            offset,
                            alpha,
                            self.strict,
                            WORD_NUM_BITS,
                            NUM_WINDOWS,
                        )?;

                        Ok(())
                    },
                )
            }
        }

        // Random base field element
        {
            let alpha = pallas::Base::random(OsRng);

            // Strict full decomposition should pass.
            let circuit: MyCircuit<pallas::Base, L_BASE, FIXED_BASE_WINDOW_SIZE, { NUM_WINDOWS }> =
                MyCircuit {
                    alpha: Value::known(alpha),
                    strict: true,
                };
            let prover = MockProver::<pallas::Base>::run(8, &circuit, vec![]).unwrap();
            assert_eq!(prover.verify(), Ok(()));
        }

        // Random 64-bit word
        {
            let alpha = pallas::Base::from(rand::random::<u64>());

            // Strict full decomposition should pass.
            let circuit: MyCircuit<
                pallas::Base,
                L_SHORT,
                FIXED_BASE_WINDOW_SIZE,
                { NUM_WINDOWS_SHORT },
            > = MyCircuit {
                alpha: Value::known(alpha),
                strict: true,
            };
            let prover = MockProver::<pallas::Base>::run(8, &circuit, vec![]).unwrap();
            assert_eq!(prover.verify(), Ok(()));
        }

        // 2^66
        {
            let alpha = pallas::Base::from_u128(1 << 66);

            // Strict partial decomposition should fail.
            let circuit: MyCircuit<
                pallas::Base,
                L_SHORT,
                FIXED_BASE_WINDOW_SIZE,
                { NUM_WINDOWS_SHORT },
            > = MyCircuit {
                alpha: Value::known(alpha),
                strict: true,
            };
            let prover = MockProver::<pallas::Base>::run(8, &circuit, vec![]).unwrap();
            assert_eq!(
                prover.verify(),
                Err(vec![
                    VerifyFailure::Permutation {
                        column: (Any::Fixed, 0).into(),
                        location: FailureLocation::OutsideRegion { row: 0 },
                    },
                    VerifyFailure::Permutation {
                        column: (Any::Fixed, 0).into(),
                        location: FailureLocation::OutsideRegion { row: 1 },
                    },
                    VerifyFailure::Permutation {
                        column: (Any::Advice, 0).into(),
                        location: FailureLocation::InRegion {
                            region: (0, "decompose").into(),
                            offset: 22,
                        },
                    },
                    VerifyFailure::Permutation {
                        column: (Any::Advice, 0).into(),
                        location: FailureLocation::InRegion {
                            region: (0, "decompose").into(),
                            offset: 45,
                        },
                    },
                ])
            );

            // Non-strict partial decomposition should pass.
            let circuit: MyCircuit<
                pallas::Base,
                { L_SHORT },
                FIXED_BASE_WINDOW_SIZE,
                { NUM_WINDOWS_SHORT },
            > = MyCircuit {
                alpha: Value::known(alpha),
                strict: false,
            };
            let prover = MockProver::<pallas::Base>::run(8, &circuit, vec![]).unwrap();
            assert_eq!(prover.verify(), Ok(()));
        }
    }
}
