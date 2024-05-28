//! Gadget for Zcash's Pedersen hash.

use super::ecc::{EdwardsPoint, MontgomeryPoint};
pub use crate::pedersen_hash::Personalization;

use bellman::gadgets::boolean::Boolean;
use bellman::gadgets::lookup::*;
use bellman::{ConstraintSystem, SynthesisError};

use super::constants::PEDERSEN_CIRCUIT_GENERATORS;

fn get_constant_bools(person: &Personalization) -> Vec<Boolean> {
    person
        .get_bits()
        .into_iter()
        .map(Boolean::constant)
        .collect()
}

pub fn pedersen_hash<CS>(
    mut cs: CS,
    personalization: Personalization,
    bits: &[Boolean],
) -> Result<EdwardsPoint, SynthesisError>
where
    CS: ConstraintSystem<bls12_381::Scalar>,
{
    let personalization = get_constant_bools(&personalization);
    assert_eq!(personalization.len(), 6);

    let mut edwards_result = None;
    let mut bits = personalization.iter().chain(bits.iter()).peekable();
    let mut segment_generators = PEDERSEN_CIRCUIT_GENERATORS.iter();
    let boolean_false = Boolean::constant(false);

    let mut segment_i = 0;
    while bits.peek().is_some() {
        let mut segment_result = None;
        let mut segment_windows = &segment_generators.next().expect("enough segments")[..];

        let mut window_i = 0;
        while let Some(a) = bits.next() {
            let b = bits.next().unwrap_or(&boolean_false);
            let c = bits.next().unwrap_or(&boolean_false);

            let tmp = lookup3_xy_with_conditional_negation(
                cs.namespace(|| format!("segment {}, window {}", segment_i, window_i)),
                &[a.clone(), b.clone(), c.clone()],
                &segment_windows[0],
            )?;

            let tmp = MontgomeryPoint::interpret_unchecked(tmp.0, tmp.1);

            match segment_result {
                None => {
                    segment_result = Some(tmp);
                }
                Some(ref mut segment_result) => {
                    *segment_result = tmp.add(
                        cs.namespace(|| {
                            format!("addition of segment {}, window {}", segment_i, window_i)
                        }),
                        segment_result,
                    )?;
                }
            }

            segment_windows = &segment_windows[1..];

            if segment_windows.is_empty() {
                break;
            }

            window_i += 1;
        }

        let segment_result = segment_result.expect(
            "bits is not exhausted due to while condition;
                    thus there must be a segment window;
                    thus there must be a segment result",
        );

        // Convert this segment into twisted Edwards form.
        let segment_result = segment_result.into_edwards(
            cs.namespace(|| format!("conversion of segment {} into edwards", segment_i)),
        )?;

        match edwards_result {
            Some(ref mut edwards_result) => {
                *edwards_result = segment_result.add(
                    cs.namespace(|| format!("addition of segment {} to accumulator", segment_i)),
                    edwards_result,
                )?;
            }
            None => {
                edwards_result = Some(segment_result);
            }
        }

        segment_i += 1;
    }

    Ok(edwards_result.unwrap())
}

#[cfg(test)]
mod test {
    use super::*;
    use crate::pedersen_hash;

    use bellman::gadgets::boolean::{AllocatedBit, Boolean};
    use bellman::gadgets::test::*;
    use group::{ff::PrimeField, Curve};
    use rand_core::{RngCore, SeedableRng};
    use rand_xorshift::XorShiftRng;

    /// Predict the number of constraints of a Pedersen hash
    fn ph_num_constraints(input_bits: usize) -> usize {
        // Account for the 6 personalization bits.
        let personalized_bits = 6 + input_bits;
        // Constant booleans in the personalization and padding don't need lookup "precomp" constraints.
        let precomputed_booleans = 2 + (personalized_bits % 3 == 1) as usize;

        // Count chunks and segments with ceiling division
        let chunks = (personalized_bits + 3 - 1) / 3;
        let segments = (chunks + 63 - 1) / 63;
        let all_but_last_segments = segments - 1;
        let last_chunks = chunks - all_but_last_segments * 63;

        // Constraints per operation
        let lookup_chunk = 2;
        let add_chunks = 3; // Montgomery addition
        let convert_segment = 2; // Conversion to Edwards
        let add_segments = 6; // Edwards addition

        (chunks) * lookup_chunk - precomputed_booleans
            + segments * convert_segment
            + all_but_last_segments * ((63 - 1) * add_chunks + add_segments)
            + (last_chunks - 1) * add_chunks
    }

    #[test]
    fn test_pedersen_hash_constraints() {
        let mut rng = XorShiftRng::from_seed([
            0x59, 0x62, 0xbe, 0x3d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06,
            0xbc, 0xe5,
        ]);

        let leaves_len = 2 * 255;
        let note_len = 64 + 256 + 256;

        for &n_bits in [
            0,
            3 * 63 - 6,
            3 * 63 - 6 + 1,
            3 * 63 - 6 + 2,
            leaves_len,
            note_len,
        ]
        .iter()
        {
            let mut cs = TestConstraintSystem::new();

            let input: Vec<bool> = (0..n_bits).map(|_| rng.next_u32() % 2 != 0).collect();

            let input_bools: Vec<Boolean> = input
                .iter()
                .enumerate()
                .map(|(i, b)| {
                    Boolean::from(
                        AllocatedBit::alloc(cs.namespace(|| format!("input {}", i)), Some(*b))
                            .unwrap(),
                    )
                })
                .collect();

            pedersen_hash(
                cs.namespace(|| "pedersen hash"),
                Personalization::NoteCommitment,
                &input_bools,
            )
            .unwrap();

            assert!(cs.is_satisfied());

            let bitness_constraints = n_bits;
            let ph_constraints = ph_num_constraints(n_bits);
            assert_eq!(cs.num_constraints(), bitness_constraints + ph_constraints);
            // The actual usages
            if n_bits == leaves_len {
                assert_eq!(cs.num_constraints(), leaves_len + 867)
            };
            if n_bits == note_len {
                assert_eq!(cs.num_constraints(), note_len + 982)
            };
        }
    }

    #[test]
    fn test_pedersen_hash() {
        let mut rng = XorShiftRng::from_seed([
            0x59, 0x62, 0xbe, 0x3d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06,
            0xbc, 0xe5,
        ]);

        for length in 0..751 {
            for _ in 0..5 {
                let input: Vec<bool> = (0..length).map(|_| rng.next_u32() % 2 != 0).collect();

                let mut cs = TestConstraintSystem::new();

                let input_bools: Vec<Boolean> = input
                    .iter()
                    .enumerate()
                    .map(|(i, b)| {
                        Boolean::from(
                            AllocatedBit::alloc(cs.namespace(|| format!("input {}", i)), Some(*b))
                                .unwrap(),
                        )
                    })
                    .collect();

                let res = pedersen_hash(
                    cs.namespace(|| "pedersen hash"),
                    Personalization::MerkleTree(1),
                    &input_bools,
                )
                .unwrap();

                assert!(cs.is_satisfied());

                let expected = jubjub::ExtendedPoint::from(pedersen_hash::pedersen_hash(
                    Personalization::MerkleTree(1),
                    input.clone().into_iter(),
                ))
                .to_affine();

                assert_eq!(res.get_u().get_value().unwrap(), expected.get_u());
                assert_eq!(res.get_v().get_value().unwrap(), expected.get_v());

                // Test against the output of a different personalization
                let unexpected = jubjub::ExtendedPoint::from(pedersen_hash::pedersen_hash(
                    Personalization::MerkleTree(0),
                    input.into_iter(),
                ))
                .to_affine();

                assert!(res.get_u().get_value().unwrap() != unexpected.get_u());
                assert!(res.get_v().get_value().unwrap() != unexpected.get_v());
            }
        }
    }

    #[test]
    fn test_pedersen_hash_external_test_vectors() {
        let mut rng = XorShiftRng::from_seed([
            0x59, 0x62, 0xbe, 0x3d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06,
            0xbc, 0xe5,
        ]);

        let expected_us = [
            "28161926966428986673895580777285905189725480206811328272001879986576840909576",
            "39669831794597628158501766225645040955899576179071014703006420393381978263045",
        ];
        let expected_vs = [
            "26869991781071974894722407757894142583682396277979904369818887810555917099932",
            "2112827187110048608327330788910224944044097981650120385961435904443901436107",
        ];
        for length in 300..302 {
            let input: Vec<bool> = (0..length).map(|_| rng.next_u32() % 2 != 0).collect();

            let mut cs = TestConstraintSystem::new();

            let input_bools: Vec<Boolean> = input
                .iter()
                .enumerate()
                .map(|(i, b)| {
                    Boolean::from(
                        AllocatedBit::alloc(cs.namespace(|| format!("input {}", i)), Some(*b))
                            .unwrap(),
                    )
                })
                .collect();

            let res = pedersen_hash(
                cs.namespace(|| "pedersen hash"),
                Personalization::MerkleTree(1),
                &input_bools,
            )
            .unwrap();

            assert!(cs.is_satisfied());

            assert_eq!(
                res.get_u().get_value().unwrap(),
                bls12_381::Scalar::from_str_vartime(expected_us[length - 300]).unwrap()
            );
            assert_eq!(
                res.get_v().get_value().unwrap(),
                bls12_381::Scalar::from_str_vartime(expected_vs[length - 300]).unwrap()
            );
        }
    }
}
