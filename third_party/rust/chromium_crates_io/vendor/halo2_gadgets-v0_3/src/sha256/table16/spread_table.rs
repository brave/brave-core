use super::{util::*, AssignedBits};

use group::ff::{Field, PrimeField};
use halo2_proofs::{
    circuit::{Chip, Layouter, Region, Value},
    pasta::pallas,
    plonk::{Advice, Column, ConstraintSystem, Error, TableColumn},
    poly::Rotation,
};
use std::convert::TryInto;
use std::marker::PhantomData;

const BITS_7: usize = 1 << 7;
const BITS_10: usize = 1 << 10;
const BITS_11: usize = 1 << 11;
const BITS_13: usize = 1 << 13;
const BITS_14: usize = 1 << 14;

/// An input word into a lookup, containing (tag, dense, spread)
#[derive(Copy, Clone, Debug)]
pub(super) struct SpreadWord<const DENSE: usize, const SPREAD: usize> {
    pub tag: u8,
    pub dense: [bool; DENSE],
    pub spread: [bool; SPREAD],
}

/// Helper function that returns tag of 16-bit input
pub fn get_tag(input: u16) -> u8 {
    let input = input as usize;
    if input < BITS_7 {
        0
    } else if input < BITS_10 {
        1
    } else if input < BITS_11 {
        2
    } else if input < BITS_13 {
        3
    } else if input < BITS_14 {
        4
    } else {
        5
    }
}

impl<const DENSE: usize, const SPREAD: usize> SpreadWord<DENSE, SPREAD> {
    pub(super) fn new(dense: [bool; DENSE]) -> Self {
        assert!(DENSE <= 16);
        SpreadWord {
            tag: get_tag(lebs2ip(&dense) as u16),
            dense,
            spread: spread_bits(dense),
        }
    }

    pub(super) fn try_new<T: TryInto<[bool; DENSE]> + std::fmt::Debug>(dense: T) -> Self
    where
        <T as TryInto<[bool; DENSE]>>::Error: std::fmt::Debug,
    {
        assert!(DENSE <= 16);
        let dense: [bool; DENSE] = dense.try_into().unwrap();
        SpreadWord {
            tag: get_tag(lebs2ip(&dense) as u16),
            dense,
            spread: spread_bits(dense),
        }
    }
}

/// A variable stored in advice columns corresponding to a row of [`SpreadTableConfig`].
#[derive(Clone, Debug)]
pub(super) struct SpreadVar<const DENSE: usize, const SPREAD: usize> {
    pub _tag: Value<u8>,
    pub dense: AssignedBits<DENSE>,
    pub spread: AssignedBits<SPREAD>,
}

impl<const DENSE: usize, const SPREAD: usize> SpreadVar<DENSE, SPREAD> {
    pub(super) fn with_lookup(
        region: &mut Region<'_, pallas::Base>,
        cols: &SpreadInputs,
        row: usize,
        word: Value<SpreadWord<DENSE, SPREAD>>,
    ) -> Result<Self, Error> {
        let tag = word.map(|word| word.tag);
        let dense_val = word.map(|word| word.dense);
        let spread_val = word.map(|word| word.spread);

        region.assign_advice(
            || "tag",
            cols.tag,
            row,
            || tag.map(|tag| pallas::Base::from(tag as u64)),
        )?;

        let dense =
            AssignedBits::<DENSE>::assign_bits(region, || "dense", cols.dense, row, dense_val)?;

        let spread =
            AssignedBits::<SPREAD>::assign_bits(region, || "spread", cols.spread, row, spread_val)?;

        Ok(SpreadVar {
            _tag: tag,
            dense,
            spread,
        })
    }

    pub(super) fn without_lookup(
        region: &mut Region<'_, pallas::Base>,
        dense_col: Column<Advice>,
        dense_row: usize,
        spread_col: Column<Advice>,
        spread_row: usize,
        word: Value<SpreadWord<DENSE, SPREAD>>,
    ) -> Result<Self, Error> {
        let tag = word.map(|word| word.tag);
        let dense_val = word.map(|word| word.dense);
        let spread_val = word.map(|word| word.spread);

        let dense = AssignedBits::<DENSE>::assign_bits(
            region,
            || "dense",
            dense_col,
            dense_row,
            dense_val,
        )?;

        let spread = AssignedBits::<SPREAD>::assign_bits(
            region,
            || "spread",
            spread_col,
            spread_row,
            spread_val,
        )?;

        Ok(SpreadVar {
            _tag: tag,
            dense,
            spread,
        })
    }
}

#[derive(Clone, Debug)]
pub(super) struct SpreadInputs {
    pub(super) tag: Column<Advice>,
    pub(super) dense: Column<Advice>,
    pub(super) spread: Column<Advice>,
}

#[derive(Clone, Debug)]
pub(super) struct SpreadTable {
    pub(super) tag: TableColumn,
    pub(super) dense: TableColumn,
    pub(super) spread: TableColumn,
}

#[derive(Clone, Debug)]
pub(super) struct SpreadTableConfig {
    pub input: SpreadInputs,
    pub table: SpreadTable,
}

#[derive(Clone, Debug)]
pub(super) struct SpreadTableChip<F: Field> {
    config: SpreadTableConfig,
    _marker: PhantomData<F>,
}

impl<F: Field> Chip<F> for SpreadTableChip<F> {
    type Config = SpreadTableConfig;
    type Loaded = ();

    fn config(&self) -> &Self::Config {
        &self.config
    }

    fn loaded(&self) -> &Self::Loaded {
        &()
    }
}

impl<F: PrimeField> SpreadTableChip<F> {
    pub fn configure(
        meta: &mut ConstraintSystem<F>,
        input_tag: Column<Advice>,
        input_dense: Column<Advice>,
        input_spread: Column<Advice>,
    ) -> <Self as Chip<F>>::Config {
        let table_tag = meta.lookup_table_column();
        let table_dense = meta.lookup_table_column();
        let table_spread = meta.lookup_table_column();

        meta.lookup(|meta| {
            let tag_cur = meta.query_advice(input_tag, Rotation::cur());
            let dense_cur = meta.query_advice(input_dense, Rotation::cur());
            let spread_cur = meta.query_advice(input_spread, Rotation::cur());

            vec![
                (tag_cur, table_tag),
                (dense_cur, table_dense),
                (spread_cur, table_spread),
            ]
        });

        SpreadTableConfig {
            input: SpreadInputs {
                tag: input_tag,
                dense: input_dense,
                spread: input_spread,
            },
            table: SpreadTable {
                tag: table_tag,
                dense: table_dense,
                spread: table_spread,
            },
        }
    }

    pub fn load(
        config: SpreadTableConfig,
        layouter: &mut impl Layouter<F>,
    ) -> Result<<Self as Chip<F>>::Loaded, Error> {
        layouter.assign_table(
            || "spread table",
            |mut table| {
                // We generate the row values lazily (we only need them during keygen).
                let mut rows = SpreadTableConfig::generate::<F>();

                for index in 0..(1 << 16) {
                    let mut row = None;
                    table.assign_cell(
                        || "tag",
                        config.table.tag,
                        index,
                        || {
                            row = rows.next();
                            Value::known(row.map(|(tag, _, _)| tag).unwrap())
                        },
                    )?;
                    table.assign_cell(
                        || "dense",
                        config.table.dense,
                        index,
                        || Value::known(row.map(|(_, dense, _)| dense).unwrap()),
                    )?;
                    table.assign_cell(
                        || "spread",
                        config.table.spread,
                        index,
                        || Value::known(row.map(|(_, _, spread)| spread).unwrap()),
                    )?;
                }

                Ok(())
            },
        )
    }
}

impl SpreadTableConfig {
    fn generate<F: PrimeField>() -> impl Iterator<Item = (F, F, F)> {
        (1..=(1 << 16)).scan((F::ZERO, F::ZERO, F::ZERO), |(tag, dense, spread), i| {
            // We computed this table row in the previous iteration.
            let res = (*tag, *dense, *spread);

            // i holds the zero-indexed row number for the next table row.
            match i {
                BITS_7 | BITS_10 | BITS_11 | BITS_13 | BITS_14 => *tag += F::ONE,
                _ => (),
            }
            *dense += F::ONE;
            if i & 1 == 0 {
                // On even-numbered rows we recompute the spread.
                *spread = F::ZERO;
                for b in 0..16 {
                    if (i >> b) & 1 != 0 {
                        *spread += F::from(1 << (2 * b));
                    }
                }
            } else {
                // On odd-numbered rows we add one.
                *spread += F::ONE;
            }

            Some(res)
        })
    }
}

#[cfg(test)]
mod tests {
    use super::{get_tag, SpreadTableChip, SpreadTableConfig};
    use rand::Rng;

    use group::ff::PrimeField;
    use halo2_proofs::{
        circuit::{Layouter, SimpleFloorPlanner, Value},
        dev::MockProver,
        pasta::Fp,
        plonk::{Advice, Circuit, Column, ConstraintSystem, Error},
    };

    #[test]
    fn lookup_table() {
        /// This represents an advice column at a certain row in the ConstraintSystem
        #[derive(Copy, Clone, Debug)]
        pub struct Variable(Column<Advice>, usize);

        struct MyCircuit {}

        impl<F: PrimeField> Circuit<F> for MyCircuit {
            type Config = SpreadTableConfig;
            type FloorPlanner = SimpleFloorPlanner;

            fn without_witnesses(&self) -> Self {
                MyCircuit {}
            }

            fn configure(meta: &mut ConstraintSystem<F>) -> Self::Config {
                let input_tag = meta.advice_column();
                let input_dense = meta.advice_column();
                let input_spread = meta.advice_column();

                SpreadTableChip::configure(meta, input_tag, input_dense, input_spread)
            }

            fn synthesize(
                &self,
                config: Self::Config,
                mut layouter: impl Layouter<F>,
            ) -> Result<(), Error> {
                SpreadTableChip::load(config.clone(), &mut layouter)?;

                layouter.assign_region(
                    || "spread_test",
                    |mut gate| {
                        let mut row = 0;
                        let mut add_row = |tag, dense, spread| -> Result<(), Error> {
                            gate.assign_advice(
                                || "tag",
                                config.input.tag,
                                row,
                                || Value::known(tag),
                            )?;
                            gate.assign_advice(
                                || "dense",
                                config.input.dense,
                                row,
                                || Value::known(dense),
                            )?;
                            gate.assign_advice(
                                || "spread",
                                config.input.spread,
                                row,
                                || Value::known(spread),
                            )?;
                            row += 1;
                            Ok(())
                        };

                        // Test the first few small values.
                        add_row(F::ZERO, F::from(0b000), F::from(0b000000))?;
                        add_row(F::ZERO, F::from(0b001), F::from(0b000001))?;
                        add_row(F::ZERO, F::from(0b010), F::from(0b000100))?;
                        add_row(F::ZERO, F::from(0b011), F::from(0b000101))?;
                        add_row(F::ZERO, F::from(0b100), F::from(0b010000))?;
                        add_row(F::ZERO, F::from(0b101), F::from(0b010001))?;

                        // Test the tag boundaries:
                        // 7-bit
                        add_row(F::ZERO, F::from(0b1111111), F::from(0b01010101010101))?;
                        add_row(F::ONE, F::from(0b10000000), F::from(0b0100000000000000))?;
                        // - 10-bit
                        add_row(
                            F::ONE,
                            F::from(0b1111111111),
                            F::from(0b01010101010101010101),
                        )?;
                        add_row(
                            F::from(2),
                            F::from(0b10000000000),
                            F::from(0b0100000000000000000000),
                        )?;
                        // - 11-bit
                        add_row(
                            F::from(2),
                            F::from(0b11111111111),
                            F::from(0b0101010101010101010101),
                        )?;
                        add_row(
                            F::from(3),
                            F::from(0b100000000000),
                            F::from(0b010000000000000000000000),
                        )?;
                        // - 13-bit
                        add_row(
                            F::from(3),
                            F::from(0b1111111111111),
                            F::from(0b01010101010101010101010101),
                        )?;
                        add_row(
                            F::from(4),
                            F::from(0b10000000000000),
                            F::from(0b0100000000000000000000000000),
                        )?;
                        // - 14-bit
                        add_row(
                            F::from(4),
                            F::from(0b11111111111111),
                            F::from(0b0101010101010101010101010101),
                        )?;
                        add_row(
                            F::from(5),
                            F::from(0b100000000000000),
                            F::from(0b010000000000000000000000000000),
                        )?;

                        // Test random lookup values
                        let mut rng = rand::thread_rng();

                        fn interleave_u16_with_zeros(word: u16) -> u32 {
                            let mut word: u32 = word.into();
                            word = (word ^ (word << 8)) & 0x00ff00ff;
                            word = (word ^ (word << 4)) & 0x0f0f0f0f;
                            word = (word ^ (word << 2)) & 0x33333333;
                            word = (word ^ (word << 1)) & 0x55555555;
                            word
                        }

                        for _ in 0..10 {
                            let word: u16 = rng.gen();
                            add_row(
                                F::from(u64::from(get_tag(word))),
                                F::from(u64::from(word)),
                                F::from(u64::from(interleave_u16_with_zeros(word))),
                            )?;
                        }

                        Ok(())
                    },
                )
            }
        }

        let circuit: MyCircuit = MyCircuit {};

        let prover = match MockProver::<Fp>::run(17, &circuit, vec![]) {
            Ok(prover) => prover,
            Err(e) => panic!("{:?}", e),
        };
        assert_eq!(prover.verify(), Ok(()));
    }
}
