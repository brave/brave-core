//! Utility gadgets.

use ff::{Field, PrimeField, PrimeFieldBits};
use halo2_proofs::{
    circuit::{AssignedCell, Cell, Layouter, Value},
    plonk::{Advice, Column, Error, Expression},
};

use std::marker::PhantomData;
use std::ops::Range;

pub mod cond_swap;
pub mod decompose_running_sum;
pub mod lookup_range_check;

/// A type that has a value at either keygen or proving time.
pub trait FieldValue<F: Field> {
    /// Returns the value of this type.
    fn value(&self) -> Value<&F>;
}

impl<F: Field> FieldValue<F> for Value<F> {
    fn value(&self) -> Value<&F> {
        self.as_ref()
    }
}

impl<F: Field> FieldValue<F> for AssignedCell<F, F> {
    fn value(&self) -> Value<&F> {
        self.value()
    }
}

/// Trait for a variable in the circuit.
pub trait Var<F: Field>: Clone + std::fmt::Debug + From<AssignedCell<F, F>> {
    /// The cell at which this variable was allocated.
    fn cell(&self) -> Cell;

    /// The value allocated to this variable.
    fn value(&self) -> Value<F>;
}

impl<F: Field> Var<F> for AssignedCell<F, F> {
    fn cell(&self) -> Cell {
        self.cell()
    }

    fn value(&self) -> Value<F> {
        self.value().cloned()
    }
}

/// Trait for utilities used across circuits.
pub trait UtilitiesInstructions<F: Field> {
    /// Variable in the circuit.
    type Var: Var<F>;

    /// Load a variable.
    fn load_private(
        &self,
        mut layouter: impl Layouter<F>,
        column: Column<Advice>,
        value: Value<F>,
    ) -> Result<Self::Var, Error> {
        layouter.assign_region(
            || "load private",
            |mut region| {
                region
                    .assign_advice(|| "load private", column, 0, || value)
                    .map(Self::Var::from)
            },
        )
    }
}

/// A type representing a range-constrained field element.
#[derive(Clone, Copy, Debug)]
pub struct RangeConstrained<F: Field, T: FieldValue<F>> {
    inner: T,
    num_bits: usize,
    _phantom: PhantomData<F>,
}

impl<F: Field, T: FieldValue<F>> RangeConstrained<F, T> {
    /// Returns the range-constrained inner type.
    pub fn inner(&self) -> &T {
        &self.inner
    }

    /// Returns the number of bits to which this cell is constrained.
    pub fn num_bits(&self) -> usize {
        self.num_bits
    }
}

impl<F: PrimeFieldBits> RangeConstrained<F, Value<F>> {
    /// Constructs a `RangeConstrained<Value<F>>` as a bitrange of the given value.
    pub fn bitrange_of(value: Value<&F>, bitrange: Range<usize>) -> Self {
        let num_bits = bitrange.len();
        Self {
            inner: value.map(|value| bitrange_subset(value, bitrange)),
            num_bits,
            _phantom: PhantomData::default(),
        }
    }
}

impl<F: Field> RangeConstrained<F, AssignedCell<F, F>> {
    /// Constructs a `RangeConstrained<AssignedCell<F, F>>` without verifying that the
    /// cell is correctly range constrained.
    ///
    /// This API only exists to ease with integrating this type into existing circuits,
    /// and will likely be removed in future.
    pub fn unsound_unchecked(cell: AssignedCell<F, F>, num_bits: usize) -> Self {
        Self {
            inner: cell,
            num_bits,
            _phantom: PhantomData::default(),
        }
    }

    /// Extracts the range-constrained value from this range-constrained cell.
    pub fn value(&self) -> RangeConstrained<F, Value<F>> {
        RangeConstrained {
            inner: self.inner.value().copied(),
            num_bits: self.num_bits,
            _phantom: PhantomData::default(),
        }
    }
}

/// Checks that an expression is either 1 or 0.
pub fn bool_check<F: PrimeField>(value: Expression<F>) -> Expression<F> {
    range_check(value, 2)
}

/// If `a` then `b`, else `c`. Returns (a * b) + (1 - a) * c.
///
/// `a` must be a boolean-constrained expression.
pub fn ternary<F: Field>(a: Expression<F>, b: Expression<F>, c: Expression<F>) -> Expression<F> {
    let one_minus_a = Expression::Constant(F::ONE) - a.clone();
    a * b + one_minus_a * c
}

/// Takes a specified subsequence of the little-endian bit representation of a field element.
/// The bits are numbered from 0 for the LSB.
pub fn bitrange_subset<F: PrimeFieldBits>(field_elem: &F, bitrange: Range<usize>) -> F {
    // We can allow a subsequence of length NUM_BITS, because
    // field_elem.to_le_bits() returns canonical bitstrings.
    assert!(bitrange.end <= F::NUM_BITS as usize);

    field_elem
        .to_le_bits()
        .iter()
        .by_vals()
        .skip(bitrange.start)
        .take(bitrange.end - bitrange.start)
        .rev()
        .fold(F::ZERO, |acc, bit| {
            if bit {
                acc.double() + F::ONE
            } else {
                acc.double()
            }
        })
}

/// Check that an expression is in the small range [0..range),
/// i.e. 0 ≤ word < range.
pub fn range_check<F: PrimeField>(word: Expression<F>, range: usize) -> Expression<F> {
    (1..range).fold(word.clone(), |acc, i| {
        acc * (Expression::Constant(F::from(i as u64)) - word.clone())
    })
}

/// Decompose a word `alpha` into `window_num_bits` bits (little-endian)
/// For a window size of `w`, this returns [k_0, ..., k_n] where each `k_i`
/// is a `w`-bit value, and `scalar = k_0 + k_1 * w + k_n * w^n`.
///
/// # Panics
///
/// We are returning a `Vec<u8>` which means the window size is limited to
/// <= 8 bits.
pub fn decompose_word<F: PrimeFieldBits>(
    word: &F,
    word_num_bits: usize,
    window_num_bits: usize,
) -> Vec<u8> {
    assert!(window_num_bits <= 8);

    // Pad bits to multiple of window_num_bits
    let padding = (window_num_bits - (word_num_bits % window_num_bits)) % window_num_bits;
    let bits: Vec<bool> = word
        .to_le_bits()
        .into_iter()
        .take(word_num_bits)
        .chain(std::iter::repeat(false).take(padding))
        .collect();
    assert_eq!(bits.len(), word_num_bits + padding);

    bits.chunks_exact(window_num_bits)
        .map(|chunk| chunk.iter().rev().fold(0, |acc, b| (acc << 1) + (*b as u8)))
        .collect()
}

/// The u64 integer represented by an L-bit little-endian bitstring.
///
/// # Panics
///
/// Panics if the bitstring is longer than 64 bits.
pub fn lebs2ip<const L: usize>(bits: &[bool; L]) -> u64 {
    assert!(L <= 64);
    bits.iter()
        .enumerate()
        .fold(0u64, |acc, (i, b)| acc + if *b { 1 << i } else { 0 })
}

/// The sequence of bits representing a u64 in little-endian order.
///
/// # Panics
///
/// Panics if the expected length of the sequence `NUM_BITS` exceeds
/// 64.
pub fn i2lebsp<const NUM_BITS: usize>(int: u64) -> [bool; NUM_BITS] {
    /// Takes in an FnMut closure and returns a constant-length array with elements of
    /// type `Output`.
    fn gen_const_array<Output: Copy + Default, const LEN: usize>(
        closure: impl FnMut(usize) -> Output,
    ) -> [Output; LEN] {
        let mut ret: [Output; LEN] = [Default::default(); LEN];
        for (bit, val) in ret.iter_mut().zip((0..LEN).map(closure)) {
            *bit = val;
        }
        ret
    }
    assert!(NUM_BITS <= 64);
    gen_const_array(|mask: usize| (int & (1 << mask)) != 0)
}

#[cfg(test)]
mod tests {
    use super::*;
    use group::ff::{Field, FromUniformBytes, PrimeField};
    use halo2_proofs::{
        circuit::{Layouter, SimpleFloorPlanner},
        dev::{FailureLocation, MockProver, VerifyFailure},
        plonk::{Any, Circuit, ConstraintSystem, Constraints, Error, Selector},
        poly::Rotation,
    };
    use pasta_curves::pallas;
    use proptest::prelude::*;
    use rand::rngs::OsRng;
    use std::convert::TryInto;
    use std::iter;
    use uint::construct_uint;

    #[test]
    fn test_range_check() {
        struct MyCircuit<const RANGE: usize>(u8);

        impl<const RANGE: usize> UtilitiesInstructions<pallas::Base> for MyCircuit<RANGE> {
            type Var = AssignedCell<pallas::Base, pallas::Base>;
        }

        #[derive(Clone)]
        struct Config {
            selector: Selector,
            advice: Column<Advice>,
        }

        impl<const RANGE: usize> Circuit<pallas::Base> for MyCircuit<RANGE> {
            type Config = Config;
            type FloorPlanner = SimpleFloorPlanner;

            fn without_witnesses(&self) -> Self {
                MyCircuit(self.0)
            }

            fn configure(meta: &mut ConstraintSystem<pallas::Base>) -> Self::Config {
                let selector = meta.selector();
                let advice = meta.advice_column();

                meta.create_gate("range check", |meta| {
                    let selector = meta.query_selector(selector);
                    let advice = meta.query_advice(advice, Rotation::cur());

                    Constraints::with_selector(selector, Some(range_check(advice, RANGE)))
                });

                Config { selector, advice }
            }

            fn synthesize(
                &self,
                config: Self::Config,
                mut layouter: impl Layouter<pallas::Base>,
            ) -> Result<(), Error> {
                layouter.assign_region(
                    || "range constrain",
                    |mut region| {
                        config.selector.enable(&mut region, 0)?;
                        region.assign_advice(
                            || format!("witness {}", self.0),
                            config.advice,
                            0,
                            || Value::known(pallas::Base::from(self.0 as u64)),
                        )?;

                        Ok(())
                    },
                )
            }
        }

        for i in 0..8 {
            let circuit: MyCircuit<8> = MyCircuit(i);
            let prover = MockProver::<pallas::Base>::run(3, &circuit, vec![]).unwrap();
            assert_eq!(prover.verify(), Ok(()));
        }

        {
            let circuit: MyCircuit<8> = MyCircuit(8);
            let prover = MockProver::<pallas::Base>::run(3, &circuit, vec![]).unwrap();
            assert_eq!(
                prover.verify(),
                Err(vec![VerifyFailure::ConstraintNotSatisfied {
                    constraint: ((0, "range check").into(), 0, "").into(),
                    location: FailureLocation::InRegion {
                        region: (0, "range constrain").into(),
                        offset: 0,
                    },
                    cell_values: vec![(((Any::Advice, 0).into(), 0).into(), "0x8".to_string())],
                }])
            );
        }
    }

    #[allow(clippy::assign_op_pattern)]
    #[allow(clippy::ptr_offset_with_cast)]
    #[test]
    fn test_bitrange_subset() {
        let rng = OsRng;

        construct_uint! {
            struct U256(4);
        }

        // Subset full range.
        {
            let field_elem = pallas::Base::random(rng);
            let bitrange = 0..(pallas::Base::NUM_BITS as usize);
            let subset = bitrange_subset(&field_elem, bitrange);
            assert_eq!(field_elem, subset);
        }

        // Subset zero bits
        {
            let field_elem = pallas::Base::random(rng);
            let bitrange = 0..0;
            let subset = bitrange_subset(&field_elem, bitrange);
            assert_eq!(pallas::Base::ZERO, subset);
        }

        // Closure to decompose field element into pieces using consecutive ranges,
        // and check that we recover the original.
        let decompose = |field_elem: pallas::Base, ranges: &[Range<usize>]| {
            assert_eq!(
                ranges.iter().map(|range| range.len()).sum::<usize>(),
                pallas::Base::NUM_BITS as usize
            );
            assert_eq!(ranges[0].start, 0);
            assert_eq!(ranges.last().unwrap().end, pallas::Base::NUM_BITS as usize);

            // Check ranges are contiguous
            #[allow(unused_assignments)]
            {
                let mut ranges = ranges.iter();
                let mut range = ranges.next().unwrap();
                if let Some(next_range) = ranges.next() {
                    assert_eq!(range.end, next_range.start);
                    range = next_range;
                }
            }

            let subsets = ranges
                .iter()
                .map(|range| bitrange_subset(&field_elem, range.clone()))
                .collect::<Vec<_>>();

            let mut sum = subsets[0];
            let mut num_bits = 0;
            for (idx, subset) in subsets.iter().skip(1).enumerate() {
                // 2^num_bits
                let range_shift: [u8; 32] = {
                    num_bits += ranges[idx].len();
                    let mut range_shift = [0u8; 32];
                    U256([2, 0, 0, 0])
                        .pow(U256([num_bits as u64, 0, 0, 0]))
                        .to_little_endian(&mut range_shift);
                    range_shift
                };
                sum += subset * pallas::Base::from_repr(range_shift).unwrap();
            }
            assert_eq!(field_elem, sum);
        };

        decompose(pallas::Base::random(rng), &[0..255]);
        decompose(pallas::Base::random(rng), &[0..1, 1..255]);
        decompose(pallas::Base::random(rng), &[0..254, 254..255]);
        decompose(pallas::Base::random(rng), &[0..127, 127..255]);
        decompose(pallas::Base::random(rng), &[0..128, 128..255]);
        decompose(
            pallas::Base::random(rng),
            &[0..50, 50..100, 100..150, 150..200, 200..255],
        );
    }

    prop_compose! {
        fn arb_scalar()(bytes in prop::array::uniform32(0u8..)) -> pallas::Scalar {
            // Instead of rejecting out-of-range bytes, let's reduce them.
            let mut buf = [0; 64];
            buf[..32].copy_from_slice(&bytes);
            pallas::Scalar::from_uniform_bytes(&buf)
        }
    }

    proptest! {
        #[test]
        fn test_decompose_word(
            scalar in arb_scalar(),
            window_num_bits in 1u8..9
        ) {
            // Get decomposition into `window_num_bits` bits
            let decomposed = decompose_word(&scalar, pallas::Scalar::NUM_BITS as usize, window_num_bits as usize);

            // Flatten bits
            let bits = decomposed
                .iter()
                .flat_map(|window| (0..window_num_bits).map(move |mask| (window & (1 << mask)) != 0));

            // Ensure this decomposition contains 256 or fewer set bits.
            assert!(!bits.clone().skip(32*8).any(|b| b));

            // Pad or truncate bits to 32 bytes
            let bits: Vec<bool> = bits.chain(iter::repeat(false)).take(32*8).collect();

            let bytes: Vec<u8> = bits.chunks_exact(8).map(|chunk| chunk.iter().rev().fold(0, |acc, b| (acc << 1) + (*b as u8))).collect();

            // Check that original scalar is recovered from decomposition
            assert_eq!(scalar, pallas::Scalar::from_repr(bytes.try_into().unwrap()).unwrap());
        }
    }

    #[test]
    fn lebs2ip_round_trip() {
        use rand::rngs::OsRng;

        let mut rng = OsRng;
        {
            let int = rng.next_u64();
            assert_eq!(lebs2ip::<64>(&i2lebsp(int)), int);
        }

        assert_eq!(lebs2ip::<64>(&i2lebsp(0)), 0);
        assert_eq!(
            lebs2ip::<64>(&i2lebsp(0xFFFFFFFFFFFFFFFF)),
            0xFFFFFFFFFFFFFFFF
        );
    }

    #[test]
    fn i2lebsp_round_trip() {
        {
            let bitstring = (0..64).map(|_| rand::random()).collect::<Vec<_>>();
            assert_eq!(
                i2lebsp::<64>(lebs2ip::<64>(&bitstring.clone().try_into().unwrap())).to_vec(),
                bitstring
            );
        }

        {
            let bitstring = [false; 64];
            assert_eq!(i2lebsp(lebs2ip(&bitstring)), bitstring);
        }

        {
            let bitstring = [true; 64];
            assert_eq!(i2lebsp(lebs2ip(&bitstring)), bitstring);
        }

        {
            let bitstring = [];
            assert_eq!(i2lebsp(lebs2ip(&bitstring)), bitstring);
        }
    }
}
