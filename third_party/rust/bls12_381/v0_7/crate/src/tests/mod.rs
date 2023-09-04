use super::*;

macro_rules! test_vectors {
    ($projective:ident, $affine:ident, $serialize:ident, $deserialize:ident, $expected:ident) => {
        let mut e = $projective::identity();

        let mut v = vec![];
        {
            let mut expected = $expected;
            for _ in 0..1000 {
                let e_affine = $affine::from(e);
                let encoded = e_affine.$serialize();
                v.extend_from_slice(&encoded[..]);

                let mut decoded = encoded;
                let len_of_encoding = decoded.len();
                (&mut decoded[..]).copy_from_slice(&expected[0..len_of_encoding]);
                expected = &expected[len_of_encoding..];
                let decoded = $affine::$deserialize(&decoded).unwrap();
                assert_eq!(e_affine, decoded);

                e = &e + &$projective::generator();
            }
        }

        assert_eq!(&v[..], $expected);
    };
}

#[test]
fn g1_uncompressed_valid_test_vectors() {
    let bytes: &'static [u8] = include_bytes!("g1_uncompressed_valid_test_vectors.dat");
    test_vectors!(
        G1Projective,
        G1Affine,
        to_uncompressed,
        from_uncompressed,
        bytes
    );
}

#[test]
fn g1_compressed_valid_test_vectors() {
    let bytes: &'static [u8] = include_bytes!("g1_compressed_valid_test_vectors.dat");
    test_vectors!(
        G1Projective,
        G1Affine,
        to_compressed,
        from_compressed,
        bytes
    );
}

#[test]
fn g2_uncompressed_valid_test_vectors() {
    let bytes: &'static [u8] = include_bytes!("g2_uncompressed_valid_test_vectors.dat");
    test_vectors!(
        G2Projective,
        G2Affine,
        to_uncompressed,
        from_uncompressed,
        bytes
    );
}

#[test]
fn g2_compressed_valid_test_vectors() {
    let bytes: &'static [u8] = include_bytes!("g2_compressed_valid_test_vectors.dat");
    test_vectors!(
        G2Projective,
        G2Affine,
        to_compressed,
        from_compressed,
        bytes
    );
}

#[test]
fn test_pairing_result_against_relic() {
    /*
    Sent to me from Diego Aranha (author of RELIC library):
    1250EBD871FC0A92 A7B2D83168D0D727 272D441BEFA15C50 3DD8E90CE98DB3E7 B6D194F60839C508 A84305AACA1789B6
    089A1C5B46E5110B 86750EC6A5323488 68A84045483C92B7 AF5AF689452EAFAB F1A8943E50439F1D 59882A98EAA0170F
    1368BB445C7C2D20 9703F239689CE34C 0378A68E72A6B3B2 16DA0E22A5031B54 DDFF57309396B38C 881C4C849EC23E87
    193502B86EDB8857 C273FA075A505129 37E0794E1E65A761 7C90D8BD66065B1F FFE51D7A579973B1 315021EC3C19934F
    01B2F522473D1713 91125BA84DC4007C FBF2F8DA752F7C74 185203FCCA589AC7 19C34DFFBBAAD843 1DAD1C1FB597AAA5
    018107154F25A764 BD3C79937A45B845 46DA634B8F6BE14A 8061E55CCEBA478B 23F7DACAA35C8CA7 8BEAE9624045B4B6
    19F26337D205FB46 9CD6BD15C3D5A04D C88784FBB3D0B2DB DEA54D43B2B73F2C BB12D58386A8703E 0F948226E47EE89D
    06FBA23EB7C5AF0D 9F80940CA771B6FF D5857BAAF222EB95 A7D2809D61BFE02E 1BFD1B68FF02F0B8 102AE1C2D5D5AB1A
    11B8B424CD48BF38 FCEF68083B0B0EC5 C81A93B330EE1A67 7D0D15FF7B984E89 78EF48881E32FAC9 1B93B47333E2BA57
    03350F55A7AEFCD3 C31B4FCB6CE5771C C6A0E9786AB59733 20C806AD36082910 7BA810C5A09FFDD9 BE2291A0C25A99A2
    04C581234D086A99 02249B64728FFD21 A189E87935A95405 1C7CDBA7B3872629 A4FAFC05066245CB 9108F0242D0FE3EF
    0F41E58663BF08CF 068672CBD01A7EC7 3BACA4D72CA93544 DEFF686BFD6DF543 D48EAA24AFE47E1E FDE449383B676631
    */

    let a = G1Affine::generator();
    let b = G2Affine::generator();

    use super::fp::Fp;
    use super::fp12::Fp12;
    use super::fp2::Fp2;
    use super::fp6::Fp6;

    let res = pairing(&a, &b);

    let prep = G2Prepared::from(b);

    assert_eq!(
        res,
        multi_miller_loop(&[(&a, &prep)]).final_exponentiation()
    );

    assert_eq!(
        res.0,
        Fp12 {
            c0: Fp6 {
                c0: Fp2 {
                    c0: Fp::from_raw_unchecked([
                        0x1972_e433_a01f_85c5,
                        0x97d3_2b76_fd77_2538,
                        0xc8ce_546f_c96b_cdf9,
                        0xcef6_3e73_66d4_0614,
                        0xa611_3427_8184_3780,
                        0x13f3_448a_3fc6_d825,
                    ]),
                    c1: Fp::from_raw_unchecked([
                        0xd263_31b0_2e9d_6995,
                        0x9d68_a482_f779_7e7d,
                        0x9c9b_2924_8d39_ea92,
                        0xf480_1ca2_e131_07aa,
                        0xa16c_0732_bdbc_b066,
                        0x083c_a4af_ba36_0478,
                    ])
                },
                c1: Fp2 {
                    c0: Fp::from_raw_unchecked([
                        0x59e2_61db_0916_b641,
                        0x2716_b6f4_b23e_960d,
                        0xc8e5_5b10_a0bd_9c45,
                        0x0bdb_0bd9_9c4d_eda8,
                        0x8cf8_9ebf_57fd_aac5,
                        0x12d6_b792_9e77_7a5e,
                    ]),
                    c1: Fp::from_raw_unchecked([
                        0x5fc8_5188_b0e1_5f35,
                        0x34a0_6e3a_8f09_6365,
                        0xdb31_26a6_e02a_d62c,
                        0xfc6f_5aa9_7d9a_990b,
                        0xa12f_55f5_eb89_c210,
                        0x1723_703a_926f_8889,
                    ])
                },
                c2: Fp2 {
                    c0: Fp::from_raw_unchecked([
                        0x9358_8f29_7182_8778,
                        0x43f6_5b86_11ab_7585,
                        0x3183_aaf5_ec27_9fdf,
                        0xfa73_d7e1_8ac9_9df6,
                        0x64e1_76a6_a64c_99b0,
                        0x179f_a78c_5838_8f1f,
                    ]),
                    c1: Fp::from_raw_unchecked([
                        0x672a_0a11_ca2a_ef12,
                        0x0d11_b9b5_2aa3_f16b,
                        0xa444_12d0_699d_056e,
                        0xc01d_0177_221a_5ba5,
                        0x66e0_cede_6c73_5529,
                        0x05f5_a71e_9fdd_c339,
                    ])
                }
            },
            c1: Fp6 {
                c0: Fp2 {
                    c0: Fp::from_raw_unchecked([
                        0xd30a_88a1_b062_c679,
                        0x5ac5_6a5d_35fc_8304,
                        0xd0c8_34a6_a81f_290d,
                        0xcd54_30c2_da37_07c7,
                        0xf0c2_7ff7_8050_0af0,
                        0x0924_5da6_e2d7_2eae,
                    ]),
                    c1: Fp::from_raw_unchecked([
                        0x9f2e_0676_791b_5156,
                        0xe2d1_c823_4918_fe13,
                        0x4c9e_459f_3c56_1bf4,
                        0xa3e8_5e53_b9d3_e3c1,
                        0x820a_121e_21a7_0020,
                        0x15af_6183_41c5_9acc,
                    ])
                },
                c1: Fp2 {
                    c0: Fp::from_raw_unchecked([
                        0x7c95_658c_2499_3ab1,
                        0x73eb_3872_1ca8_86b9,
                        0x5256_d749_4774_34bc,
                        0x8ba4_1902_ea50_4a8b,
                        0x04a3_d3f8_0c86_ce6d,
                        0x18a6_4a87_fb68_6eaa,
                    ]),
                    c1: Fp::from_raw_unchecked([
                        0xbb83_e71b_b920_cf26,
                        0x2a52_77ac_92a7_3945,
                        0xfc0e_e59f_94f0_46a0,
                        0x7158_cdf3_7860_58f7,
                        0x7cc1_061b_82f9_45f6,
                        0x03f8_47aa_9fdb_e567,
                    ])
                },
                c2: Fp2 {
                    c0: Fp::from_raw_unchecked([
                        0x8078_dba5_6134_e657,
                        0x1cd7_ec9a_4399_8a6e,
                        0xb1aa_599a_1a99_3766,
                        0xc9a0_f62f_0842_ee44,
                        0x8e15_9be3_b605_dffa,
                        0x0c86_ba0d_4af1_3fc2,
                    ]),
                    c1: Fp::from_raw_unchecked([
                        0xe80f_f2a0_6a52_ffb1,
                        0x7694_ca48_721a_906c,
                        0x7583_183e_03b0_8514,
                        0xf567_afdd_40ce_e4e2,
                        0x9a6d_96d2_e526_a5fc,
                        0x197e_9f49_861f_2242,
                    ])
                }
            }
        }
    );
}
