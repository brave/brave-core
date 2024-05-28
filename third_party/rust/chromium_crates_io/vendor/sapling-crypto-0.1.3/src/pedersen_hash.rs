//! Implementation of the Pedersen hash function used in Sapling.

#[cfg(test)]
pub(crate) mod test_vectors;

use byteorder::{ByteOrder, LittleEndian};
use ff::PrimeField;
use group::Group;
use std::ops::{AddAssign, Neg};

use super::constants::{
    PEDERSEN_HASH_CHUNKS_PER_GENERATOR, PEDERSEN_HASH_EXP_TABLE, PEDERSEN_HASH_EXP_WINDOW_SIZE,
};

#[derive(Copy, Clone)]
pub enum Personalization {
    NoteCommitment,
    MerkleTree(usize),
}

impl Personalization {
    pub fn get_bits(&self) -> Vec<bool> {
        match *self {
            Personalization::NoteCommitment => vec![true, true, true, true, true, true],
            Personalization::MerkleTree(num) => {
                assert!(num < 63);

                (0..6).map(|i| (num >> i) & 1 == 1).collect()
            }
        }
    }
}

pub fn pedersen_hash<I>(personalization: Personalization, bits: I) -> jubjub::SubgroupPoint
where
    I: IntoIterator<Item = bool>,
{
    let mut bits = personalization
        .get_bits()
        .into_iter()
        .chain(bits.into_iter());

    let mut result = jubjub::SubgroupPoint::identity();
    let mut generators = PEDERSEN_HASH_EXP_TABLE.iter();

    loop {
        let mut acc = jubjub::Fr::zero();
        let mut cur = jubjub::Fr::one();
        let mut chunks_remaining = PEDERSEN_HASH_CHUNKS_PER_GENERATOR;
        let mut encountered_bits = false;

        // Grab three bits from the input
        while let Some(a) = bits.next() {
            encountered_bits = true;

            let b = bits.next().unwrap_or(false);
            let c = bits.next().unwrap_or(false);

            // Start computing this portion of the scalar
            let mut tmp = cur;
            if a {
                tmp.add_assign(&cur);
            }
            cur = cur.double(); // 2^1 * cur
            if b {
                tmp.add_assign(&cur);
            }

            // conditionally negate
            if c {
                tmp = tmp.neg();
            }

            acc.add_assign(&tmp);

            chunks_remaining -= 1;

            if chunks_remaining == 0 {
                break;
            } else {
                cur = cur.double().double().double(); // 2^4 * cur
            }
        }

        if !encountered_bits {
            break;
        }

        let mut table: &[Vec<jubjub::SubgroupPoint>] =
            generators.next().expect("we don't have enough generators");
        let window = PEDERSEN_HASH_EXP_WINDOW_SIZE as usize;
        let window_mask = (1u64 << window) - 1;

        let acc = acc.to_repr();
        let num_limbs: usize = acc.as_ref().len() / 8;
        let mut limbs = vec![0u64; num_limbs + 1];
        LittleEndian::read_u64_into(acc.as_ref(), &mut limbs[..num_limbs]);

        let mut tmp = jubjub::SubgroupPoint::identity();

        let mut pos = 0;
        while pos < jubjub::Fr::NUM_BITS as usize {
            let u64_idx = pos / 64;
            let bit_idx = pos % 64;
            let i = (if bit_idx + window < 64 {
                // This window's bits are contained in a single u64.
                limbs[u64_idx] >> bit_idx
            } else {
                // Combine the current u64's bits with the bits from the next u64.
                (limbs[u64_idx] >> bit_idx) | (limbs[u64_idx + 1] << (64 - bit_idx))
            } & window_mask) as usize;

            tmp += table[0][i];

            pos += window;
            table = &table[1..];
        }

        result += tmp;
    }

    result
}

#[cfg(test)]
pub mod test {
    use group::Curve;

    use super::*;

    pub struct TestVector<'a> {
        pub personalization: Personalization,
        pub input_bits: Vec<u8>,
        pub hash_u: &'a str,
        pub hash_v: &'a str,
    }

    #[test]
    fn test_pedersen_hash_points() {
        let test_vectors = test_vectors::get_vectors();

        assert!(!test_vectors.is_empty());

        for v in test_vectors.iter() {
            let input_bools: Vec<bool> = v.input_bits.iter().map(|&i| i == 1).collect();

            // The 6 bits prefix is handled separately
            assert_eq!(v.personalization.get_bits(), &input_bools[..6]);

            let p = jubjub::ExtendedPoint::from(pedersen_hash(
                v.personalization,
                input_bools.into_iter().skip(6),
            ))
            .to_affine();

            assert_eq!(p.get_u().to_string(), v.hash_u);
            assert_eq!(p.get_v().to_string(), v.hash_v);
        }
    }
}
