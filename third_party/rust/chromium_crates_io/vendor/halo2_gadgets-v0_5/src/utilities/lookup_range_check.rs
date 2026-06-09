//! Make use of a K-bit lookup table to decompose a field element into K-bit
//! words.

use halo2_proofs::{
    circuit::{AssignedCell, Layouter, Region},
    plonk::{Advice, Column, ConstraintSystem, Constraints, Error, Selector, TableColumn},
    poly::Rotation,
};
use std::{convert::TryInto, fmt::Debug, marker::PhantomData};

use ff::PrimeFieldBits;

use crate::sinsemilla::{chip::generator_table::GeneratorTableConfig, primitives as sinsemilla};
use pasta_curves::pallas;
use sinsemilla::SINSEMILLA_S;

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
    pub fn witness_short<const K: usize, Lookup: LookupRangeCheck<F, K>>(
        lookup_config: &Lookup,
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
                _phantom: PhantomData,
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

/// Trait that provides common methods for a lookup range check.
pub trait LookupRangeCheck<F: PrimeFieldBits, const K: usize>: Eq + Copy + Debug {
    /// Returns a reference to the `LookupRangeCheckConfig` instance.
    fn config(&self) -> &LookupRangeCheckConfig<F, K>;

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
    fn configure(
        meta: &mut ConstraintSystem<F>,
        running_sum: Column<Advice>,
        table_idx: TableColumn,
    ) -> Self
    where
        Self: Sized;

    /// Load the generator table into the circuit.
    fn load(
        &self,
        generator_table_config: &GeneratorTableConfig,
        layouter: &mut impl Layouter<pallas::Base>,
    ) -> Result<(), Error>;

    #[cfg(test)]
    /// Fill `table_idx` and `table_range_check_tag`.
    ///
    /// This is only used in testing for now, since the Sinsemilla chip provides a pre-loaded table
    /// in the Orchard context.
    fn load_range_check_table(&self, layouter: &mut impl Layouter<F>) -> Result<(), Error>;

    /// Constrain `x` to be a NUM_BITS word.
    ///
    /// `element` must have been assigned to `self.config().running_sum` at offset 0.
    fn short_range_check(
        &self,
        region: &mut Region<'_, F>,
        element: AssignedCell<F, F>,
        num_bits: usize,
    ) -> Result<(), Error>;

    /// Range check on an existing cell that is copied into this helper.
    ///
    /// Returns an error if `element` is not in a column that was passed to
    /// [`ConstraintSystem::enable_equality`] during circuit configuration.
    fn copy_check(
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
                let z_0 =
                    element.copy_advice(|| "z_0", &mut region, self.config().running_sum, 0)?;
                self.range_check(&mut region, z_0, num_words, strict)
            },
        )
    }

    /// Range check on a value that is witnessed in this helper.
    fn witness_check(
        &self,
        mut layouter: impl Layouter<F>,
        value: Value<F>,
        num_words: usize,
        strict: bool,
    ) -> Result<RunningSum<F>, Error> {
        layouter.assign_region(
            || "Witness element",
            |mut region| {
                let z_0 = region.assign_advice(
                    || "Witness element",
                    self.config().running_sum,
                    0,
                    || value,
                )?;
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
            self.config().q_lookup.enable(region, idx)?;
            // Enable q_running on this row
            self.config().q_running.enable(region, idx)?;

            // z_next = (z_cur - m_cur) / 2^K
            z = {
                let z_val = z
                    .value()
                    .zip(*word)
                    .map(|(z, word)| (*z - word) * inv_two_pow_k);

                // Assign z_next
                region.assign_advice(
                    || format!("z_{:?}", idx + 1),
                    self.config().running_sum,
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
    fn copy_short_check(
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
                    element.copy_advice(|| "element", &mut region, self.config().running_sum, 0)?;
                self.short_range_check(&mut region, element, num_bits)
            },
        )
    }

    /// Short range check on value that is witnessed in this helper.
    ///
    /// # Panics
    ///
    /// Panics if num_bits is larger than K.
    fn witness_short_check(
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
                let element = region.assign_advice(
                    || "Witness element",
                    self.config().running_sum,
                    0,
                    || element,
                )?;

                self.short_range_check(&mut region, element.clone(), num_bits)?;

                Ok(element)
            },
        )
    }
}

impl<F: PrimeFieldBits, const K: usize> LookupRangeCheck<F, K> for LookupRangeCheckConfig<F, K> {
    fn config(&self) -> &LookupRangeCheckConfig<F, K> {
        self
    }

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
    fn configure(
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
            let one = Expression::Constant(F::ONE);

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
                let q_short = one - q_running;

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

    /// Load the generator table into the circuit.
    ///
    /// | table_idx |     table_x    |     table_y    |
    /// ------------------------------------------------
    /// |     0     |    X(S\[0\])   |    Y(S\[0\])   |
    /// |     1     |    X(S\[1\])   |    Y(S\[1\])   |
    /// |    ...    |      ...       |       ...      |
    /// |   2^10-1  | X(S\[2^10-1\]) | Y(S\[2^10-1\]) |
    fn load(
        &self,
        generator_table_config: &GeneratorTableConfig,
        layouter: &mut impl Layouter<pallas::Base>,
    ) -> Result<(), Error> {
        layouter.assign_table(
            || "generator_table",
            |mut table| {
                for (index, (x, y)) in SINSEMILLA_S.iter().enumerate() {
                    table.assign_cell(
                        || "table_idx",
                        generator_table_config.table_idx,
                        index,
                        || Value::known(pallas::Base::from(index as u64)),
                    )?;
                    table.assign_cell(
                        || "table_x",
                        generator_table_config.table_x,
                        index,
                        || Value::known(*x),
                    )?;
                    table.assign_cell(
                        || "table_y",
                        generator_table_config.table_y,
                        index,
                        || Value::known(*y),
                    )?;
                }
                Ok(())
            },
        )
    }

    #[cfg(test)]
    // Fill `table_idx` and `table_range_check_tag`.
    // This is only used in testing for now, since the Sinsemilla chip provides a pre-loaded table
    // in the Orchard context.
    fn load_range_check_table(&self, layouter: &mut impl Layouter<F>) -> Result<(), Error> {
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

    /// Constrain `x` to be a NUM_BITS word.
    ///
    /// `element` must have been assigned to `self.running_sum` at offset 0.
    fn short_range_check(
        &self,
        region: &mut Region<'_, F>,
        element: AssignedCell<F, F>,
        num_bits: usize,
    ) -> Result<(), Error> {
        // Enable lookup for `element`.
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

/// `PallasLookupRangeCheck` a shorthand for `LookupRangeCheck` specialized with `pallas::Base` and
/// `sinsemilla::K` and used to improve readability. In addition, it extends
/// the `LookupRangeCheck` with additional standard traits.
pub trait PallasLookupRangeCheck: LookupRangeCheck<pallas::Base, { sinsemilla::K }> {}

/// `PallasLookupRangeCheckConfig` is a shorthand for `LookupRangeCheckConfig` specialized with
/// `pallas::Base` and `sinsemilla::K` and used to improve readability```
pub type PallasLookupRangeCheckConfig = LookupRangeCheckConfig<pallas::Base, { sinsemilla::K }>;

impl PallasLookupRangeCheck for PallasLookupRangeCheckConfig {}

/// Configuration that provides methods for an efficient 4, 5, and K-bit lookup range check.
#[derive(Eq, PartialEq, Debug, Clone, Copy)]
pub struct LookupRangeCheck4_5BConfig<F: PrimeFieldBits, const K: usize> {
    base: LookupRangeCheckConfig<F, K>,
    q_range_check_4: Selector,
    q_range_check_5: Selector,
    table_range_check_tag: TableColumn,
}

impl<F: PrimeFieldBits, const K: usize> LookupRangeCheck4_5BConfig<F, K> {
    /// The `running_sum` advice column breaks the field element into `K`-bit
    /// words. It is used to construct the input expression to the lookup
    /// argument.
    ///
    /// The `table_idx` fixed column contains values from [0..2^K). Looking up
    /// a value in `table_idx` constrains it to be within this range. The table
    /// can be loaded outside this helper.
    ///
    /// # Side effects
    ///
    /// Both the `running_sum` and `constants` columns will be equality-enabled.
    pub fn configure_with_tag(
        meta: &mut ConstraintSystem<F>,
        running_sum: Column<Advice>,
        table_idx: TableColumn,
        table_range_check_tag: TableColumn,
    ) -> Self {
        meta.enable_equality(running_sum);

        let q_lookup = meta.complex_selector();
        let q_running = meta.complex_selector();
        let q_bitshift = meta.selector();
        let q_range_check_4 = meta.complex_selector();
        let q_range_check_5 = meta.complex_selector();

        let config = LookupRangeCheck4_5BConfig {
            base: LookupRangeCheckConfig {
                q_lookup,
                q_running,
                q_bitshift,
                running_sum,
                table_idx,
                _marker: PhantomData,
            },
            q_range_check_4,
            q_range_check_5,
            table_range_check_tag,
        };

        // https://p.z.cash/halo2-0.1:decompose-combined-lookup
        meta.lookup(|meta| {
            let q_lookup = meta.query_selector(config.base.q_lookup);
            let q_running = meta.query_selector(config.base.q_running);
            let q_range_check_4 = meta.query_selector(config.q_range_check_4);
            let q_range_check_5 = meta.query_selector(config.q_range_check_5);

            let z_cur = meta.query_advice(config.base.running_sum, Rotation::cur());
            let one = Expression::Constant(F::ONE);

            // In the case of a running sum decomposition, we recover the word from
            // the difference of the running sums:
            //    z_i = 2^{K}⋅z_{i + 1} + a_i
            // => a_i = z_i - 2^{K}⋅z_{i + 1}
            let running_sum_lookup = {
                let running_sum_word = {
                    let z_next = meta.query_advice(config.base.running_sum, Rotation::next());
                    z_cur.clone() - z_next * F::from(1 << K)
                };

                q_running.clone() * running_sum_word
            };

            // In the short range check, the word is directly witnessed.
            let short_lookup = {
                let short_word = z_cur.clone();
                let q_short = one.clone() - q_running;

                q_short * short_word
            };

            // q_range_check is equal to
            // - 1 if q_range_check_4 = 1 or q_range_check_5 = 1
            // - 0 otherwise
            let q_range_check = one.clone()
                - (one.clone() - q_range_check_4.clone()) * (one.clone() - q_range_check_5.clone());

            // num_bits is equal to
            // - 5 if q_range_check_5 = 1
            // - 4 if q_range_check_4 = 1 and q_range_check_5 = 0
            // - 0 otherwise
            let num_bits = q_range_check_5.clone() * Expression::Constant(F::from(5_u64))
                + (one.clone() - q_range_check_5)
                    * q_range_check_4
                    * Expression::Constant(F::from(4_u64));

            // Combine the running sum, short lookups and optimized range checks:
            vec![
                (
                    q_lookup.clone()
                        * ((one - q_range_check.clone()) * (running_sum_lookup + short_lookup)
                            + q_range_check.clone() * z_cur),
                    config.base.table_idx,
                ),
                (
                    q_lookup * q_range_check * num_bits,
                    config.table_range_check_tag,
                ),
            ]
        });

        // For short lookups, check that the word has been shifted by the correct number of bits.
        // https://p.z.cash/halo2-0.1:decompose-short-lookup
        meta.create_gate("Short lookup bitshift", |meta| {
            let q_bitshift = meta.query_selector(config.base.q_bitshift);
            let word = meta.query_advice(config.base.running_sum, Rotation::prev());
            let shifted_word = meta.query_advice(config.base.running_sum, Rotation::cur());
            let inv_two_pow_s = meta.query_advice(config.base.running_sum, Rotation::next());

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
}

impl<F: PrimeFieldBits, const K: usize> LookupRangeCheck<F, K>
    for LookupRangeCheck4_5BConfig<F, K>
{
    fn config(&self) -> &LookupRangeCheckConfig<F, K> {
        &self.base
    }

    fn configure(
        meta: &mut ConstraintSystem<F>,
        running_sum: Column<Advice>,
        table_idx: TableColumn,
    ) -> Self {
        let table_range_check_tag = meta.lookup_table_column();
        Self::configure_with_tag(meta, running_sum, table_idx, table_range_check_tag)
    }

    /// Load the generator table into the circuit.
    ///
    /// | table_idx |     table_x    |     table_y    | table_range_check_tag |
    /// -------------------------------------------------------------------
    /// |     0     |    X(S\[0\])   |    Y(S\[0\])   |           0           |
    /// |     1     |    X(S\[1\])   |    Y(S\[1\])   |           0           |
    /// |    ...    |      ...       |       ...      |           0           |
    /// |   2^10-1  | X(S\[2^10-1\]) | Y(S\[2^10-1\]) |           0           |
    /// |     0     |    X(S\[0\])   |    Y(S\[0\])   |           4           |
    /// |     1     |    X(S\[1\])   |    Y(S\[1\])   |           4           |
    /// |    ...    |       ...      |       ...      |           4           |
    /// |   2^4-1   | X(S\[2^4-1\])  | Y(S\[2^4-1\])  |           4           |
    /// |     0     |    X(S\[0\])   |    Y(S\[0\])   |           5           |
    /// |     1     |    X(S\[1\])   |    Y(S\[1\])   |           5           |
    /// |    ...    |       ...      |       ...      |           5           |
    /// |   2^5-1   | X(S\[2^5-1\])  | Y(S\[2^5-1\])  |           5           |
    ///
    /// # Rationale
    ///
    /// While it is possible to avoid duplicated rows by using tags and storing them in an advice
    /// column, as done in the spread table of the SHA-256 gadget, applying this technique to the
    /// range check lookup table would significantly increase complexity.
    ///
    /// Indeed, in the range check lookup tables, tags are currently computed on the fly and are not
    /// stored in advice columns. Adopting the no duplicated rows approach would require modifying
    /// the lookup chip to include an advice column for tags and updating the public API accordingly.
    /// This tag column would need to be populated each time a range check lookup is performed.
    /// Additionally, the current lookup range check implementation involves combinations of
    /// multiple lookups, further complicating this approach.
    ///
    /// The performance benefit would be minimal: our current table has 1072 rows (2^10 + 2^4 + 2^5),
    /// and switching to the no duplicated-rows method would reduce it only slightly to 1024 rows
    /// (2^10), while adding more logic and infrastructure to manage the tags.
    ///
    /// Therefore, we retain the simpler duplicated-rows format for clarity and maintainability.
    fn load(
        &self,
        generator_table_config: &GeneratorTableConfig,
        layouter: &mut impl Layouter<pallas::Base>,
    ) -> Result<(), Error> {
        layouter.assign_table(
            || "generator_table",
            |mut table| {
                for (index, (x, y)) in SINSEMILLA_S.iter().enumerate() {
                    table.assign_cell(
                        || "table_idx",
                        generator_table_config.table_idx,
                        index,
                        || Value::known(pallas::Base::from(index as u64)),
                    )?;
                    table.assign_cell(
                        || "table_x",
                        generator_table_config.table_x,
                        index,
                        || Value::known(*x),
                    )?;
                    table.assign_cell(
                        || "table_y",
                        generator_table_config.table_y,
                        index,
                        || Value::known(*y),
                    )?;

                    table.assign_cell(
                        || "table_range_check_tag",
                        self.table_range_check_tag,
                        index,
                        || Value::known(pallas::Base::zero()),
                    )?;
                    if index < (1 << 4) {
                        let new_index = index + (1 << sinsemilla::K);
                        table.assign_cell(
                            || "table_idx",
                            generator_table_config.table_idx,
                            new_index,
                            || Value::known(pallas::Base::from(index as u64)),
                        )?;
                        table.assign_cell(
                            || "table_x",
                            generator_table_config.table_x,
                            new_index,
                            || Value::known(*x),
                        )?;
                        table.assign_cell(
                            || "table_y",
                            generator_table_config.table_y,
                            new_index,
                            || Value::known(*y),
                        )?;
                        table.assign_cell(
                            || "table_range_check_tag",
                            self.table_range_check_tag,
                            new_index,
                            || Value::known(pallas::Base::from(4_u64)),
                        )?;
                    }
                    if index < (1 << 5) {
                        let new_index = index + (1 << 10) + (1 << 4);
                        table.assign_cell(
                            || "table_idx",
                            generator_table_config.table_idx,
                            new_index,
                            || Value::known(pallas::Base::from(index as u64)),
                        )?;
                        table.assign_cell(
                            || "table_x",
                            generator_table_config.table_x,
                            new_index,
                            || Value::known(*x),
                        )?;
                        table.assign_cell(
                            || "table_y",
                            generator_table_config.table_y,
                            new_index,
                            || Value::known(*y),
                        )?;
                        table.assign_cell(
                            || "table_range_check_tag",
                            self.table_range_check_tag,
                            new_index,
                            || Value::known(pallas::Base::from(5_u64)),
                        )?;
                    }
                }
                Ok(())
            },
        )
    }

    #[cfg(test)]
    // Fill `table_idx` and `table_range_check_tag`.
    // This is only used in testing for now, since the Sinsemilla chip provides a pre-loaded table
    // in the Orchard context.
    fn load_range_check_table(&self, layouter: &mut impl Layouter<F>) -> Result<(), Error> {
        layouter.assign_table(
            || "table_idx",
            |mut table| {
                let mut assign_cells =
                    |offset: usize, table_size, value: u64| -> Result<usize, Error> {
                        for index in 0..table_size {
                            let new_index = index + offset;
                            table.assign_cell(
                                || "table_idx",
                                self.base.table_idx,
                                new_index,
                                || Value::known(F::from(index as u64)),
                            )?;
                            table.assign_cell(
                                || "table_range_check_tag",
                                self.table_range_check_tag,
                                new_index,
                                || Value::known(F::from(value)),
                            )?;
                        }
                        Ok(offset + table_size)
                    };

                // We generate the row values lazily (we only need them during keygen).
                let mut offset = 0;

                //annotation: &'v (dyn Fn() -> String + 'v),
                //column: TableColumn,
                //offset: usize,
                //to: &'v mut (dyn FnMut() -> Value<Assigned<F>> + 'v),

                offset = assign_cells(offset, 1 << K, 0)?;
                offset = assign_cells(offset, 1 << 4, 4)?;
                assign_cells(offset, 1 << 5, 5)?;

                Ok(())
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
        match num_bits {
            4 => {
                self.base.q_lookup.enable(region, 0)?;
                self.q_range_check_4.enable(region, 0)?;
                Ok(())
            }

            5 => {
                self.base.q_lookup.enable(region, 0)?;
                self.q_range_check_5.enable(region, 0)?;
                Ok(())
            }

            _ => self.base.short_range_check(region, element, num_bits),
        }
    }
}

/// In this crate, `LookupRangeCheck4_5BConfig` is always used with `pallas::Base` as the prime field
/// and the constant `K` from the `sinsemilla` module. To reduce verbosity and improve readability,
/// we introduce this alias and use it instead of that long construction.
pub type PallasLookupRangeCheck4_5BConfig =
    LookupRangeCheck4_5BConfig<pallas::Base, { sinsemilla::K }>;

impl PallasLookupRangeCheck for PallasLookupRangeCheck4_5BConfig {}

#[cfg(test)]
mod tests {
    use super::super::lebs2ip;

    use ff::{Field, PrimeFieldBits};
    use halo2_proofs::{
        circuit::{Layouter, SimpleFloorPlanner, Value},
        dev::{FailureLocation, MockProver, VerifyFailure},
        plonk::{Circuit, ConstraintSystem, Error},
    };
    use pasta_curves::pallas;

    use crate::{
        sinsemilla::primitives::K,
        test_circuits::test_utils::test_against_stored_circuit,
        utilities::lookup_range_check::{
            LookupRangeCheck, PallasLookupRangeCheck, PallasLookupRangeCheck4_5BConfig,
            PallasLookupRangeCheckConfig,
        },
    };
    use std::{convert::TryInto, marker::PhantomData};

    #[derive(Clone, Copy)]
    struct MyLookupCircuit<F: PrimeFieldBits, Lookup: LookupRangeCheck<F, K>> {
        num_words: usize,
        _marker: PhantomData<(F, Lookup)>,
    }

    impl<F: PrimeFieldBits, Lookup: LookupRangeCheck<F, K>> MyLookupCircuit<F, Lookup> {
        fn new(num_words: usize) -> Self {
            MyLookupCircuit {
                num_words,
                _marker: PhantomData,
            }
        }
    }

    impl<F: PrimeFieldBits, Lookup: LookupRangeCheck<F, K> + std::clone::Clone> Circuit<F>
        for MyLookupCircuit<F, Lookup>
    {
        type Config = Lookup;
        type FloorPlanner = SimpleFloorPlanner;

        fn without_witnesses(&self) -> Self {
            MyLookupCircuit::new(self.num_words)
        }

        fn configure(meta: &mut ConstraintSystem<F>) -> Self::Config {
            let running_sum = meta.advice_column();
            let table_idx = meta.lookup_table_column();
            let constants = meta.fixed_column();
            meta.enable_constant(constants);

            Lookup::configure(meta, running_sum, table_idx)
        }

        fn synthesize(
            &self,
            config: Self::Config,
            mut layouter: impl Layouter<F>,
        ) -> Result<(), Error> {
            // Load table_idx
            config.load_range_check_table(&mut layouter)?;

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

    #[test]
    fn lookup_range_check() {
        let circuit = MyLookupCircuit::<pallas::Base, PallasLookupRangeCheckConfig>::new(6);
        let prover = MockProver::<pallas::Base>::run(11, &circuit, vec![]).unwrap();
        assert_eq!(prover.verify(), Ok(()));
    }

    #[test]
    fn test_lookup_range_check_against_stored_circuit() {
        let circuit = MyLookupCircuit::<pallas::Base, PallasLookupRangeCheckConfig>::new(6);
        test_against_stored_circuit(circuit, "lookup_range_check", 1888);
    }

    #[test]
    fn lookup_range_check_4_5b() {
        let circuit = MyLookupCircuit::<pallas::Base, PallasLookupRangeCheck4_5BConfig>::new(6);
        let prover = MockProver::<pallas::Base>::run(11, &circuit, vec![]).unwrap();
        assert_eq!(prover.verify(), Ok(()));
    }

    #[test]
    fn test_lookup_range_check_against_stored_circuit_4_5b() {
        let circuit = MyLookupCircuit::<pallas::Base, PallasLookupRangeCheck4_5BConfig>::new(6);
        test_against_stored_circuit(circuit, "lookup_range_check_4_5b", 2048);
    }

    #[derive(Clone, Copy)]
    struct MyShortRangeCheckCircuit<F: PrimeFieldBits, Lookup: LookupRangeCheck<F, K>> {
        element: Value<F>,
        num_bits: usize,
        _lookup_marker: PhantomData<Lookup>,
    }

    impl<F: PrimeFieldBits, Lookup: LookupRangeCheck<F, K>> MyShortRangeCheckCircuit<F, Lookup> {
        fn new(element: Value<F>, num_bits: usize) -> Self {
            MyShortRangeCheckCircuit {
                element,
                num_bits,
                _lookup_marker: PhantomData,
            }
        }
    }

    impl<F: PrimeFieldBits, Lookup: LookupRangeCheck<F, K> + std::clone::Clone> Circuit<F>
        for MyShortRangeCheckCircuit<F, Lookup>
    {
        type Config = Lookup;
        type FloorPlanner = SimpleFloorPlanner;

        fn without_witnesses(&self) -> Self {
            MyShortRangeCheckCircuit::new(Value::unknown(), self.num_bits)
        }

        fn configure(meta: &mut ConstraintSystem<F>) -> Self::Config {
            let running_sum = meta.advice_column();
            let table_idx = meta.lookup_table_column();
            let constants = meta.fixed_column();
            meta.enable_constant(constants);

            Lookup::configure(meta, running_sum, table_idx)
        }

        fn synthesize(
            &self,
            config: Self::Config,
            mut layouter: impl Layouter<F>,
        ) -> Result<(), Error> {
            // Load table_idx
            config.load_range_check_table(&mut layouter)?;

            // Lookup constraining element to be no longer than num_bits.
            config.witness_short_check(
                layouter.namespace(|| format!("Lookup {:?} bits", self.num_bits)),
                self.element,
                self.num_bits,
            )?;

            Ok(())
        }
    }

    fn test_short_range_check<Lookup: PallasLookupRangeCheck>(
        element: pallas::Base,
        num_bits: usize,
        proof_result: &Result<(), Vec<VerifyFailure>>,
        circuit_name: &str,
        expected_proof_size: usize,
    ) {
        let circuit =
            MyShortRangeCheckCircuit::<pallas::Base, Lookup>::new(Value::known(element), num_bits);
        let prover = MockProver::<pallas::Base>::run(11, &circuit, vec![]).unwrap();
        assert_eq!(prover.verify(), *proof_result);

        if proof_result.is_ok() {
            test_against_stored_circuit(circuit, circuit_name, expected_proof_size);
        }
    }

    #[test]
    fn short_range_check() {
        let proof_size_10_bits = 1888;
        let proof_size_4_5_10_bits = 2048;

        // Edge case: zero bits (case 0)
        let element = pallas::Base::ZERO;
        let num_bits = 0;
        test_short_range_check::<PallasLookupRangeCheckConfig>(
            element,
            num_bits,
            &Ok(()),
            "short_range_check_case0",
            proof_size_10_bits,
        );
        test_short_range_check::<PallasLookupRangeCheck4_5BConfig>(
            element,
            num_bits,
            &Ok(()),
            "short_range_check_4_5b_case0",
            proof_size_4_5_10_bits,
        );

        // Edge case: K bits (case 1)
        let element = pallas::Base::from((1 << K) - 1);
        let num_bits = K;
        test_short_range_check::<PallasLookupRangeCheckConfig>(
            element,
            num_bits,
            &Ok(()),
            "short_range_check_case1",
            proof_size_10_bits,
        );
        test_short_range_check::<PallasLookupRangeCheck4_5BConfig>(
            element,
            num_bits,
            &Ok(()),
            "short_range_check_4_5b_case1",
            proof_size_4_5_10_bits,
        );

        // Element within `num_bits` (case 2)
        let element = pallas::Base::from((1 << 6) - 1);
        let num_bits = 6;
        test_short_range_check::<PallasLookupRangeCheckConfig>(
            element,
            num_bits,
            &Ok(()),
            "short_range_check_case2",
            proof_size_10_bits,
        );
        test_short_range_check::<PallasLookupRangeCheck4_5BConfig>(
            element,
            num_bits,
            &Ok(()),
            "short_range_check_4_5b_case2",
            proof_size_4_5_10_bits,
        );

        // Element larger than `num_bits` but within K bits
        let element = pallas::Base::from(1 << 6);
        let num_bits = 6;
        let error = Err(vec![VerifyFailure::Lookup {
            lookup_index: 0,
            location: FailureLocation::InRegion {
                region: (1, "Range check 6 bits").into(),
                offset: 1,
            },
        }]);
        test_short_range_check::<PallasLookupRangeCheckConfig>(
            element,
            num_bits,
            &error,
            "not_saved",
            proof_size_10_bits,
        );
        test_short_range_check::<PallasLookupRangeCheck4_5BConfig>(
            element,
            num_bits,
            &error,
            "not_saved",
            proof_size_4_5_10_bits,
        );

        // Element larger than K bits
        let element = pallas::Base::from(1 << K);
        let num_bits = 6;
        let error = Err(vec![
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
        ]);
        test_short_range_check::<PallasLookupRangeCheckConfig>(
            element,
            num_bits,
            &error,
            "not_saved",
            proof_size_10_bits,
        );
        test_short_range_check::<PallasLookupRangeCheck4_5BConfig>(
            element,
            num_bits,
            &error,
            "not_saved",
            proof_size_4_5_10_bits,
        );

        // Element which is not within `num_bits`, but which has a shifted value within num_bits
        let num_bits = 6;
        let shifted = pallas::Base::from((1 << num_bits) - 1);
        // Recall that shifted = element * 2^{K-s}
        //          => element = shifted * 2^{s-K}
        let element = shifted
            * pallas::Base::from(1 << (K as u64 - num_bits))
                .invert()
                .unwrap();
        let error = Err(vec![VerifyFailure::Lookup {
            lookup_index: 0,
            location: FailureLocation::InRegion {
                region: (1, "Range check 6 bits").into(),
                offset: 0,
            },
        }]);
        test_short_range_check::<PallasLookupRangeCheckConfig>(
            element,
            num_bits as usize,
            &error,
            "not_saved",
            proof_size_10_bits,
        );
        test_short_range_check::<PallasLookupRangeCheck4_5BConfig>(
            element,
            num_bits as usize,
            &error,
            "not_saved",
            proof_size_4_5_10_bits,
        );

        // Element within 4 bits
        test_short_range_check::<PallasLookupRangeCheck4_5BConfig>(
            pallas::Base::from((1 << 4) - 1),
            4,
            &Ok(()),
            "short_range_check_4_5b_case3",
            proof_size_4_5_10_bits,
        );

        // Element larger than 5 bits
        test_short_range_check::<PallasLookupRangeCheck4_5BConfig>(
            pallas::Base::from(1 << 5),
            5,
            &Err(vec![VerifyFailure::Lookup {
                lookup_index: 0,
                location: FailureLocation::InRegion {
                    region: (1, "Range check 5 bits").into(),
                    offset: 0,
                },
            }]),
            "not_saved",
            proof_size_4_5_10_bits,
        );
    }
}
