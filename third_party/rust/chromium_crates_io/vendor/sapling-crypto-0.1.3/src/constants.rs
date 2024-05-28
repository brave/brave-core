//! Various constants used by the Sapling protocol.

use ff::PrimeField;
use group::Group;
use jubjub::SubgroupPoint;
use lazy_static::lazy_static;

/// First 64 bytes of the BLAKE2s input during group hash.
/// This is chosen to be some random string that we couldn't have anticipated when we designed
/// the algorithm, for rigidity purposes.
/// We deliberately use an ASCII hex string of 32 bytes here.
pub const GH_FIRST_BLOCK: &[u8; 64] =
    b"096b36a5804bfacef1691e173c366a47ff5ba84a44f26ddd7e8d9f79d5b42df0";

// BLAKE2s invocation personalizations
/// BLAKE2s Personalization for CRH^ivk = BLAKE2s(ak | nk)
pub const CRH_IVK_PERSONALIZATION: &[u8; 8] = b"Zcashivk";

/// BLAKE2s Personalization for PRF^nf = BLAKE2s(nk | rho)
pub const PRF_NF_PERSONALIZATION: &[u8; 8] = b"Zcash_nf";

// Group hash personalizations
/// BLAKE2s Personalization for Pedersen hash generators.
pub const PEDERSEN_HASH_GENERATORS_PERSONALIZATION: &[u8; 8] = b"Zcash_PH";

/// BLAKE2s Personalization for the group hash for key diversification
pub const KEY_DIVERSIFICATION_PERSONALIZATION: &[u8; 8] = b"Zcash_gd";

/// BLAKE2s Personalization for the spending key base point
pub const SPENDING_KEY_GENERATOR_PERSONALIZATION: &[u8; 8] = b"Zcash_G_";

/// BLAKE2s Personalization for the proof generation key base point
pub const PROOF_GENERATION_KEY_BASE_GENERATOR_PERSONALIZATION: &[u8; 8] = b"Zcash_H_";

/// BLAKE2s Personalization for the value commitment generator for the value
pub const VALUE_COMMITMENT_GENERATOR_PERSONALIZATION: &[u8; 8] = b"Zcash_cv";

/// BLAKE2s Personalization for the nullifier position generator (for computing rho)
pub const NULLIFIER_POSITION_IN_TREE_GENERATOR_PERSONALIZATION: &[u8; 8] = b"Zcash_J_";

/// The prover will demonstrate knowledge of discrete log with respect to this base when
/// they are constructing a proof, in order to authorize proof construction.
pub const PROOF_GENERATION_KEY_GENERATOR: SubgroupPoint = SubgroupPoint::from_raw_unchecked(
    bls12_381::Scalar::from_raw([
        0x3af2_dbef_b96e_2571,
        0xadf2_d038_f2fb_b820,
        0x7043_03f1_e890_6081,
        0x1457_a502_31cd_e2df,
    ]),
    bls12_381::Scalar::from_raw([
        0x467a_f9f7_e05d_e8e7,
        0x50df_51ea_f5a1_49d2,
        0xdec9_0184_0f49_48cc,
        0x54b6_d107_18df_2a7a,
    ]),
);

/// The note commitment is randomized over this generator.
pub const NOTE_COMMITMENT_RANDOMNESS_GENERATOR: SubgroupPoint = SubgroupPoint::from_raw_unchecked(
    bls12_381::Scalar::from_raw([
        0xa514_3b34_a8e3_6462,
        0xf091_9d06_ffb1_ecda,
        0xa140_9aa1_f33b_ec2c,
        0x26eb_9f8a_9ec7_2a8c,
    ]),
    bls12_381::Scalar::from_raw([
        0xd4fc_6365_796c_77ac,
        0x96b7_8bea_fa9c_c44c,
        0x949d_7747_6e26_2c95,
        0x114b_7501_ad10_4c57,
    ]),
);

/// The node commitment is randomized again by the position in order to supply the
/// nullifier computation with a unique input w.r.t. the note being spent, to prevent
/// Faerie gold attacks.
pub const NULLIFIER_POSITION_GENERATOR: SubgroupPoint = SubgroupPoint::from_raw_unchecked(
    bls12_381::Scalar::from_raw([
        0x2ce3_3921_888d_30db,
        0xe81c_ee09_a561_229e,
        0xdb56_b6db_8d80_75ed,
        0x2400_c2e2_e336_2644,
    ]),
    bls12_381::Scalar::from_raw([
        0xa3f7_fa36_c72b_0065,
        0xe155_b8e8_ffff_2e42,
        0xfc9e_8a15_a096_ba8f,
        0x6136_9d54_40bf_84a5,
    ]),
);

/// The value commitment is used to check balance between inputs and outputs. The value is
/// placed over this generator.
pub const VALUE_COMMITMENT_VALUE_GENERATOR: SubgroupPoint = SubgroupPoint::from_raw_unchecked(
    bls12_381::Scalar::from_raw([
        0x3618_3b2c_b4d7_ef51,
        0x9472_c89a_c043_042d,
        0xd861_8ed1_d15f_ef4e,
        0x273f_910d_9ecc_1615,
    ]),
    bls12_381::Scalar::from_raw([
        0xa77a_81f5_0667_c8d7,
        0xbc33_32d0_fa1c_cd18,
        0xd322_94fd_8977_4ad6,
        0x466a_7e3a_82f6_7ab1,
    ]),
);

/// The value commitment is randomized over this generator, for privacy.
pub const VALUE_COMMITMENT_RANDOMNESS_GENERATOR: SubgroupPoint = SubgroupPoint::from_raw_unchecked(
    bls12_381::Scalar::from_raw([
        0x3bce_3b77_9366_4337,
        0xd1d8_da41_af03_744e,
        0x7ff6_826a_d580_04b4,
        0x6800_f4fa_0f00_1cfc,
    ]),
    bls12_381::Scalar::from_raw([
        0x3cae_fab9_380b_6a8b,
        0xad46_f1b0_473b_803b,
        0xe6fb_2a6e_1e22_ab50,
        0x6d81_d3a9_cb45_dedb,
    ]),
);

/// The spender proves discrete log with respect to this base at spend time.
pub const SPENDING_KEY_GENERATOR: SubgroupPoint = SubgroupPoint::from_raw_unchecked(
    bls12_381::Scalar::from_raw([
        0x47bf_4692_0a95_a753,
        0xd5b9_a7d3_ef8e_2827,
        0xd418_a7ff_2675_3b6a,
        0x0926_d4f3_2059_c712,
    ]),
    bls12_381::Scalar::from_raw([
        0x3056_32ad_aaf2_b530,
        0x6d65_674d_cedb_ddbc,
        0x53bb_37d0_c21c_fd05,
        0x57a1_019e_6de9_b675,
    ]),
);

/// The generators (for each segment) used in all Pedersen commitments.
pub const PEDERSEN_HASH_GENERATORS: &[SubgroupPoint] = &[
    SubgroupPoint::from_raw_unchecked(
        bls12_381::Scalar::from_raw([
            0x194e_4292_6f66_1b51,
            0x2f0c_718f_6f0f_badd,
            0xb5ea_25de_7ec0_e378,
            0x73c0_16a4_2ded_9578,
        ]),
        bls12_381::Scalar::from_raw([
            0x77bf_abd4_3224_3cca,
            0xf947_2e8b_c04e_4632,
            0x79c9_166b_837e_dc5e,
            0x289e_87a2_d352_1b57,
        ]),
    ),
    SubgroupPoint::from_raw_unchecked(
        bls12_381::Scalar::from_raw([
            0xb981_9dc8_2d90_607e,
            0xa361_ee3f_d48f_df77,
            0x52a3_5a8c_1908_dd87,
            0x15a3_6d1f_0f39_0d88,
        ]),
        bls12_381::Scalar::from_raw([
            0x7b0d_c53c_4ebf_1891,
            0x1f3a_beeb_98fa_d3e8,
            0xf789_1142_c001_d925,
            0x015d_8c7f_5b43_fe33,
        ]),
    ),
    SubgroupPoint::from_raw_unchecked(
        bls12_381::Scalar::from_raw([
            0x76d6_f7c2_b67f_c475,
            0xbae8_e5c4_6641_ae5c,
            0xeb69_ae39_f5c8_4210,
            0x6643_21a5_8246_e2f6,
        ]),
        bls12_381::Scalar::from_raw([
            0x80ed_502c_9793_d457,
            0x8bb2_2a7f_1784_b498,
            0xe000_a46c_8e8c_e853,
            0x362e_1500_d24e_ee9e,
        ]),
    ),
    SubgroupPoint::from_raw_unchecked(
        bls12_381::Scalar::from_raw([
            0x4c76_7804_c1c4_a2cc,
            0x7d02_d50e_654b_87f2,
            0xedc5_f4a9_cff2_9fd5,
            0x323a_6548_ce9d_9876,
        ]),
        bls12_381::Scalar::from_raw([
            0x8471_4bec_a335_70e9,
            0x5103_afa1_a11f_6a85,
            0x9107_0acb_d8d9_47b7,
            0x2f7e_e40c_4b56_cad8,
        ]),
    ),
    SubgroupPoint::from_raw_unchecked(
        bls12_381::Scalar::from_raw([
            0x4680_9430_657f_82d1,
            0xefd5_9313_05f2_f0bf,
            0x89b6_4b4e_0336_2796,
            0x3bd2_6660_00b5_4796,
        ]),
        bls12_381::Scalar::from_raw([
            0x9996_8299_c365_8aef,
            0xb3b9_d809_5859_d14c,
            0x3978_3238_1406_c9e5,
            0x494b_c521_03ab_9d0a,
        ]),
    ),
    SubgroupPoint::from_raw_unchecked(
        bls12_381::Scalar::from_raw([
            0xcb3c_0232_58d3_2079,
            0x1d9e_5ca2_1135_ff6f,
            0xda04_9746_d76d_3ee5,
            0x6344_7b2b_a31b_b28a,
        ]),
        bls12_381::Scalar::from_raw([
            0x4360_8211_9f8d_629a,
            0xa802_00d2_c66b_13a7,
            0x64cd_b107_0a13_6a28,
            0x64ec_4689_e8bf_b6e5,
        ]),
    ),
];

/// The maximum number of chunks per segment of the Pedersen hash.
pub const PEDERSEN_HASH_CHUNKS_PER_GENERATOR: usize = 63;

/// The window size for exponentiation of Pedersen hash generators outside the circuit.
pub const PEDERSEN_HASH_EXP_WINDOW_SIZE: u32 = 8;

lazy_static! {
    /// The exp table for [`PEDERSEN_HASH_GENERATORS`].
    pub static ref PEDERSEN_HASH_EXP_TABLE: Vec<Vec<Vec<SubgroupPoint>>> =
        generate_pedersen_hash_exp_table();
}

/// Creates the exp table for the Pedersen hash generators.
fn generate_pedersen_hash_exp_table() -> Vec<Vec<Vec<SubgroupPoint>>> {
    let window = PEDERSEN_HASH_EXP_WINDOW_SIZE;

    PEDERSEN_HASH_GENERATORS
        .iter()
        .cloned()
        .map(|mut g| {
            let mut tables = vec![];

            let mut num_bits = 0;
            while num_bits <= jubjub::Fr::NUM_BITS {
                let mut table = Vec::with_capacity(1 << window);
                let mut base = SubgroupPoint::identity();

                for _ in 0..(1 << window) {
                    table.push(base);
                    base += g;
                }

                tables.push(table);
                num_bits += window;

                for _ in 0..window {
                    g = g.double();
                }
            }

            tables
        })
        .collect()
}

#[cfg(test)]
mod tests {
    use jubjub::SubgroupPoint;

    use super::*;
    use crate::group_hash::group_hash;

    fn find_group_hash(m: &[u8], personalization: &[u8; 8]) -> SubgroupPoint {
        let mut tag = m.to_vec();
        let i = tag.len();
        tag.push(0u8);

        loop {
            let gh = group_hash(&tag, personalization);

            // We don't want to overflow and start reusing generators
            assert!(tag[i] != u8::max_value());
            tag[i] += 1;

            if let Some(gh) = gh {
                break gh;
            }
        }
    }

    #[test]
    fn proof_generation_key_base_generator() {
        assert_eq!(
            find_group_hash(&[], PROOF_GENERATION_KEY_BASE_GENERATOR_PERSONALIZATION),
            PROOF_GENERATION_KEY_GENERATOR,
        );
    }

    #[test]
    fn note_commitment_randomness_generator() {
        assert_eq!(
            find_group_hash(b"r", PEDERSEN_HASH_GENERATORS_PERSONALIZATION),
            NOTE_COMMITMENT_RANDOMNESS_GENERATOR,
        );
    }

    #[test]
    fn nullifier_position_generator() {
        assert_eq!(
            find_group_hash(&[], NULLIFIER_POSITION_IN_TREE_GENERATOR_PERSONALIZATION),
            NULLIFIER_POSITION_GENERATOR,
        );
    }

    #[test]
    fn value_commitment_value_generator() {
        assert_eq!(
            find_group_hash(b"v", VALUE_COMMITMENT_GENERATOR_PERSONALIZATION),
            VALUE_COMMITMENT_VALUE_GENERATOR,
        );
    }

    #[test]
    fn value_commitment_randomness_generator() {
        assert_eq!(
            find_group_hash(b"r", VALUE_COMMITMENT_GENERATOR_PERSONALIZATION),
            VALUE_COMMITMENT_RANDOMNESS_GENERATOR,
        );
    }

    #[test]
    fn spending_key_generator() {
        assert_eq!(
            find_group_hash(&[], SPENDING_KEY_GENERATOR_PERSONALIZATION),
            SPENDING_KEY_GENERATOR,
        );
    }

    #[test]
    fn pedersen_hash_generators() {
        for (m, actual) in PEDERSEN_HASH_GENERATORS.iter().enumerate() {
            assert_eq!(
                &find_group_hash(
                    &(m as u32).to_le_bytes(),
                    PEDERSEN_HASH_GENERATORS_PERSONALIZATION
                ),
                actual
            );
        }
    }

    #[test]
    fn no_duplicate_fixed_base_generators() {
        let fixed_base_generators = [
            PROOF_GENERATION_KEY_GENERATOR,
            NOTE_COMMITMENT_RANDOMNESS_GENERATOR,
            NULLIFIER_POSITION_GENERATOR,
            VALUE_COMMITMENT_VALUE_GENERATOR,
            VALUE_COMMITMENT_RANDOMNESS_GENERATOR,
            SPENDING_KEY_GENERATOR,
        ];

        // Check for duplicates, far worse than spec inconsistencies!
        for (i, p1) in fixed_base_generators.iter().enumerate() {
            if p1.is_identity().into() {
                panic!("Neutral element!");
            }

            for p2 in fixed_base_generators.iter().skip(i + 1) {
                if p1 == p2 {
                    panic!("Duplicate generator!");
                }
            }
        }
    }

    /// Check for simple relations between the generators, that make finding collisions easy;
    /// far worse than spec inconsistencies!
    fn check_consistency_of_pedersen_hash_generators(
        pedersen_hash_generators: &[jubjub::SubgroupPoint],
    ) {
        for (i, p1) in pedersen_hash_generators.iter().enumerate() {
            if p1.is_identity().into() {
                panic!("Neutral element!");
            }
            for p2 in pedersen_hash_generators.iter().skip(i + 1) {
                if p1 == p2 {
                    panic!("Duplicate generator!");
                }
                if *p1 == -p2 {
                    panic!("Inverse generator!");
                }
            }

            // check for a generator being the sum of any other two
            for (j, p2) in pedersen_hash_generators.iter().enumerate() {
                if j == i {
                    continue;
                }
                for (k, p3) in pedersen_hash_generators.iter().enumerate() {
                    if k == j || k == i {
                        continue;
                    }
                    let sum = p2 + p3;
                    if sum == *p1 {
                        panic!("Linear relation between generators!");
                    }
                }
            }
        }
    }

    #[test]
    fn pedersen_hash_generators_consistency() {
        check_consistency_of_pedersen_hash_generators(PEDERSEN_HASH_GENERATORS);
    }

    #[test]
    #[should_panic(expected = "Linear relation between generators!")]
    fn test_jubjub_bls12_pedersen_hash_generators_consistency_check_linear_relation() {
        let mut pedersen_hash_generators = PEDERSEN_HASH_GENERATORS.to_vec();

        // Test for linear relation
        pedersen_hash_generators.push(PEDERSEN_HASH_GENERATORS[0] + PEDERSEN_HASH_GENERATORS[1]);

        check_consistency_of_pedersen_hash_generators(&pedersen_hash_generators);
    }
}
