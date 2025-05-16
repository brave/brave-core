//! Implementation of hash-to-curve for the G1 group.

use subtle::{Choice, ConditionallyNegatable, ConditionallySelectable, ConstantTimeEq};

use super::chain::chain_pm3div4;
use super::{HashToField, MapToCurve, Sgn0};
use crate::fp::Fp;
use crate::g1::G1Projective;
use crate::generic_array::{typenum::U64, GenericArray};

/// Coefficients of the 11-isogeny x map's numerator
const ISO11_XNUM: [Fp; 12] = [
    Fp::from_raw_unchecked([
        0x4d18_b6f3_af00_131c,
        0x19fa_2197_93fe_e28c,
        0x3f28_85f1_467f_19ae,
        0x23dc_ea34_f2ff_b304,
        0xd15b_58d2_ffc0_0054,
        0x0913_be20_0a20_bef4,
    ]),
    Fp::from_raw_unchecked([
        0x8989_8538_5cdb_bd8b,
        0x3c79_e43c_c7d9_66aa,
        0x1597_e193_f4cd_233a,
        0x8637_ef1e_4d66_23ad,
        0x11b2_2dee_d20d_827b,
        0x0709_7bc5_9987_84ad,
    ]),
    Fp::from_raw_unchecked([
        0xa542_583a_480b_664b,
        0xfc71_69c0_26e5_68c6,
        0x5ba2_ef31_4ed8_b5a6,
        0x5b54_91c0_5102_f0e7,
        0xdf6e_9970_7d2a_0079,
        0x0784_151e_d760_5524,
    ]),
    Fp::from_raw_unchecked([
        0x494e_2128_70f7_2741,
        0xab9b_e52f_bda4_3021,
        0x26f5_5779_94e3_4c3d,
        0x049d_fee8_2aef_bd60,
        0x65da_dd78_2850_5289,
        0x0e93_d431_ea01_1aeb,
    ]),
    Fp::from_raw_unchecked([
        0x90ee_774b_d6a7_4d45,
        0x7ada_1c8a_41bf_b185,
        0x0f1a_8953_b325_f464,
        0x104c_2421_1be4_805c,
        0x1691_39d3_19ea_7a8f,
        0x09f2_0ead_8e53_2bf6,
    ]),
    Fp::from_raw_unchecked([
        0x6ddd_93e2_f436_26b7,
        0xa548_2c9a_a1cc_d7bd,
        0x1432_4563_1883_f4bd,
        0x2e0a_94cc_f77e_c0db,
        0xb028_2d48_0e56_489f,
        0x18f4_bfcb_b436_8929,
    ]),
    Fp::from_raw_unchecked([
        0x23c5_f0c9_5340_2dfd,
        0x7a43_ff69_58ce_4fe9,
        0x2c39_0d3d_2da5_df63,
        0xd0df_5c98_e1f9_d70f,
        0xffd8_9869_a572_b297,
        0x1277_ffc7_2f25_e8fe,
    ]),
    Fp::from_raw_unchecked([
        0x79f4_f049_0f06_a8a6,
        0x85f8_94a8_8030_fd81,
        0x12da_3054_b18b_6410,
        0xe2a5_7f65_0588_0d65,
        0xbba0_74f2_60e4_00f1,
        0x08b7_6279_f621_d028,
    ]),
    Fp::from_raw_unchecked([
        0xe672_45ba_78d5_b00b,
        0x8456_ba9a_1f18_6475,
        0x7888_bff6_e6b3_3bb4,
        0xe215_85b9_a30f_86cb,
        0x05a6_9cdc_ef55_feee,
        0x09e6_99dd_9adf_a5ac,
    ]),
    Fp::from_raw_unchecked([
        0x0de5_c357_bff5_7107,
        0x0a0d_b4ae_6b1a_10b2,
        0xe256_bb67_b3b3_cd8d,
        0x8ad4_5657_4e9d_b24f,
        0x0443_915f_50fd_4179,
        0x098c_4bf7_de8b_6375,
    ]),
    Fp::from_raw_unchecked([
        0xe6b0_617e_7dd9_29c7,
        0xfe6e_37d4_4253_7375,
        0x1daf_deda_137a_489e,
        0xe4ef_d1ad_3f76_7ceb,
        0x4a51_d866_7f0f_e1cf,
        0x054f_df4b_bf1d_821c,
    ]),
    Fp::from_raw_unchecked([
        0x72db_2a50_658d_767b,
        0x8abf_91fa_a257_b3d5,
        0xe969_d683_3764_ab47,
        0x4641_7014_2a10_09eb,
        0xb14f_01aa_db30_be2f,
        0x18ae_6a85_6f40_715d,
    ]),
];

/// Coefficients of the 11-isogeny x map's denominator
const ISO11_XDEN: [Fp; 11] = [
    Fp::from_raw_unchecked([
        0xb962_a077_fdb0_f945,
        0xa6a9_740f_efda_13a0,
        0xc14d_568c_3ed6_c544,
        0xb43f_c37b_908b_133e,
        0x9c0b_3ac9_2959_9016,
        0x0165_aa6c_93ad_115f,
    ]),
    Fp::from_raw_unchecked([
        0x2327_9a3b_a506_c1d9,
        0x92cf_ca0a_9465_176a,
        0x3b29_4ab1_3755_f0ff,
        0x116d_da1c_5070_ae93,
        0xed45_3092_4cec_2045,
        0x0833_83d6_ed81_f1ce,
    ]),
    Fp::from_raw_unchecked([
        0x9885_c2a6_449f_ecfc,
        0x4a2b_54cc_d377_33f0,
        0x17da_9ffd_8738_c142,
        0xa0fb_a727_32b3_fafd,
        0xff36_4f36_e54b_6812,
        0x0f29_c13c_6605_23e2,
    ]),
    Fp::from_raw_unchecked([
        0xe349_cc11_8278_f041,
        0xd487_228f_2f32_04fb,
        0xc9d3_2584_9ade_5150,
        0x43a9_2bd6_9c15_c2df,
        0x1c2c_7844_bc41_7be4,
        0x1202_5184_f407_440c,
    ]),
    Fp::from_raw_unchecked([
        0x587f_65ae_6acb_057b,
        0x1444_ef32_5140_201f,
        0xfbf9_95e7_1270_da49,
        0xccda_0660_7243_6a42,
        0x7408_904f_0f18_6bb2,
        0x13b9_3c63_edf6_c015,
    ]),
    Fp::from_raw_unchecked([
        0xfb91_8622_cd14_1920,
        0x4a4c_6442_3eca_ddb4,
        0x0beb_2329_27f7_fb26,
        0x30f9_4df6_f83a_3dc2,
        0xaeed_d424_d780_f388,
        0x06cc_402d_d594_bbeb,
    ]),
    Fp::from_raw_unchecked([
        0xd41f_7611_51b2_3f8f,
        0x32a9_2465_4357_19b3,
        0x64f4_36e8_88c6_2cb9,
        0xdf70_a9a1_f757_c6e4,
        0x6933_a38d_5b59_4c81,
        0x0c6f_7f72_37b4_6606,
    ]),
    Fp::from_raw_unchecked([
        0x693c_0874_7876_c8f7,
        0x22c9_850b_f9cf_80f0,
        0x8e90_71da_b950_c124,
        0x89bc_62d6_1c7b_af23,
        0xbc6b_e2d8_dad5_7c23,
        0x1791_6987_aa14_a122,
    ]),
    Fp::from_raw_unchecked([
        0x1be3_ff43_9c13_16fd,
        0x9965_243a_7571_dfa7,
        0xc7f7_f629_62f5_cd81,
        0x32c6_aa9a_f394_361c,
        0xbbc2_ee18_e1c2_27f4,
        0x0c10_2cba_c531_bb34,
    ]),
    Fp::from_raw_unchecked([
        0x9976_14c9_7bac_bf07,
        0x61f8_6372_b991_92c0,
        0x5b8c_95fc_1435_3fc3,
        0xca2b_066c_2a87_492f,
        0x1617_8f5b_bf69_8711,
        0x12a6_dcd7_f0f4_e0e8,
    ]),
    Fp::from_raw_unchecked([
        0x7609_0000_0002_fffd,
        0xebf4_000b_c40c_0002,
        0x5f48_9857_53c7_58ba,
        0x77ce_5853_7052_5745,
        0x5c07_1a97_a256_ec6d,
        0x15f6_5ec3_fa80_e493,
    ]),
];

/// Coefficients of the 11-isogeny y map's numerator
const ISO11_YNUM: [Fp; 16] = [
    Fp::from_raw_unchecked([
        0x2b56_7ff3_e283_7267,
        0x1d4d_9e57_b958_a767,
        0xce02_8fea_04bd_7373,
        0xcc31_a30a_0b6c_d3df,
        0x7d7b_18a6_8269_2693,
        0x0d30_0744_d42a_0310,
    ]),
    Fp::from_raw_unchecked([
        0x99c2_555f_a542_493f,
        0xfe7f_53cc_4874_f878,
        0x5df0_608b_8f97_608a,
        0x14e0_3832_052b_49c8,
        0x7063_26a6_957d_d5a4,
        0x0a8d_add9_c241_4555,
    ]),
    Fp::from_raw_unchecked([
        0x13d9_4292_2a5c_f63a,
        0x357e_33e3_6e26_1e7d,
        0xcf05_a27c_8456_088d,
        0x0000_bd1d_e7ba_50f0,
        0x83d0_c753_2f8c_1fde,
        0x13f7_0bf3_8bbf_2905,
    ]),
    Fp::from_raw_unchecked([
        0x5c57_fd95_bfaf_bdbb,
        0x28a3_59a6_5e54_1707,
        0x3983_ceb4_f636_0b6d,
        0xafe1_9ff6_f97e_6d53,
        0xb346_8f45_5019_2bf7,
        0x0bb6_cde4_9d8b_a257,
    ]),
    Fp::from_raw_unchecked([
        0x590b_62c7_ff8a_513f,
        0x314b_4ce3_72ca_cefd,
        0x6bef_32ce_94b8_a800,
        0x6ddf_84a0_9571_3d5f,
        0x64ea_ce4c_b098_2191,
        0x0386_213c_651b_888d,
    ]),
    Fp::from_raw_unchecked([
        0xa531_0a31_111b_bcdd,
        0xa14a_c0f5_da14_8982,
        0xf9ad_9cc9_5423_d2e9,
        0xaa6e_c095_283e_e4a7,
        0xcf5b_1f02_2e1c_9107,
        0x01fd_df5a_ed88_1793,
    ]),
    Fp::from_raw_unchecked([
        0x65a5_72b0_d7a7_d950,
        0xe25c_2d81_8347_3a19,
        0xc2fc_ebe7_cb87_7dbd,
        0x05b2_d36c_769a_89b0,
        0xba12_961b_e86e_9efb,
        0x07eb_1b29_c1df_de1f,
    ]),
    Fp::from_raw_unchecked([
        0x93e0_9572_f7c4_cd24,
        0x364e_9290_7679_5091,
        0x8569_467e_68af_51b5,
        0xa47d_a894_39f5_340f,
        0xf4fa_9180_82e4_4d64,
        0x0ad5_2ba3_e669_5a79,
    ]),
    Fp::from_raw_unchecked([
        0x9114_2984_4e0d_5f54,
        0xd03f_51a3_516b_b233,
        0x3d58_7e56_4053_6e66,
        0xfa86_d2a3_a9a7_3482,
        0xa90e_d5ad_f1ed_5537,
        0x149c_9c32_6a5e_7393,
    ]),
    Fp::from_raw_unchecked([
        0x462b_beb0_3c12_921a,
        0xdc9a_f5fa_0a27_4a17,
        0x9a55_8ebd_e836_ebed,
        0x649e_f8f1_1a4f_ae46,
        0x8100_e165_2b3c_dc62,
        0x1862_bd62_c291_dacb,
    ]),
    Fp::from_raw_unchecked([
        0x05c9_b8ca_89f1_2c26,
        0x0194_160f_a9b9_ac4f,
        0x6a64_3d5a_6879_fa2c,
        0x1466_5bdd_8846_e19d,
        0xbb1d_0d53_af3f_f6bf,
        0x12c7_e1c3_b289_62e5,
    ]),
    Fp::from_raw_unchecked([
        0xb55e_bf90_0b8a_3e17,
        0xfedc_77ec_1a92_01c4,
        0x1f07_db10_ea1a_4df4,
        0x0dfb_d15d_c41a_594d,
        0x3895_47f2_334a_5391,
        0x0241_9f98_1658_71a4,
    ]),
    Fp::from_raw_unchecked([
        0xb416_af00_0745_fc20,
        0x8e56_3e9d_1ea6_d0f5,
        0x7c76_3e17_763a_0652,
        0x0145_8ef0_159e_bbef,
        0x8346_fe42_1f96_bb13,
        0x0d2d_7b82_9ce3_24d2,
    ]),
    Fp::from_raw_unchecked([
        0x9309_6bb5_38d6_4615,
        0x6f2a_2619_951d_823a,
        0x8f66_b3ea_5951_4fa4,
        0xf563_e637_04f7_092f,
        0x724b_136c_4cf2_d9fa,
        0x0469_59cf_cfd0_bf49,
    ]),
    Fp::from_raw_unchecked([
        0xea74_8d4b_6e40_5346,
        0x91e9_079c_2c02_d58f,
        0x4106_4965_946d_9b59,
        0xa067_31f1_d2bb_e1ee,
        0x07f8_97e2_67a3_3f1b,
        0x1017_2909_1921_0e5f,
    ]),
    Fp::from_raw_unchecked([
        0x872a_a6c1_7d98_5097,
        0xeecc_5316_1264_562a,
        0x07af_e37a_fff5_5002,
        0x5475_9078_e5be_6838,
        0xc4b9_2d15_db8a_cca8,
        0x106d_87d1_b51d_13b9,
    ]),
];

/// Coefficients of the 11-isogeny y map's denominator
const ISO11_YDEN: [Fp; 16] = [
    Fp::from_raw_unchecked([
        0xeb6c_359d_47e5_2b1c,
        0x18ef_5f8a_1063_4d60,
        0xddfa_71a0_889d_5b7e,
        0x723e_71dc_c5fc_1323,
        0x52f4_5700_b70d_5c69,
        0x0a8b_981e_e476_91f1,
    ]),
    Fp::from_raw_unchecked([
        0x616a_3c4f_5535_b9fb,
        0x6f5f_0373_95db_d911,
        0xf25f_4cc5_e35c_65da,
        0x3e50_dffe_a3c6_2658,
        0x6a33_dca5_2356_0776,
        0x0fad_eff7_7b6b_fe3e,
    ]),
    Fp::from_raw_unchecked([
        0x2be9_b66d_f470_059c,
        0x24a2_c159_a3d3_6742,
        0x115d_be7a_d10c_2a37,
        0xb663_4a65_2ee5_884d,
        0x04fe_8bb2_b8d8_1af4,
        0x01c2_a7a2_56fe_9c41,
    ]),
    Fp::from_raw_unchecked([
        0xf27b_f8ef_3b75_a386,
        0x898b_3674_76c9_073f,
        0x2448_2e6b_8c2f_4e5f,
        0xc8e0_bbd6_fe11_0806,
        0x59b0_c17f_7631_448a,
        0x1103_7cd5_8b3d_bfbd,
    ]),
    Fp::from_raw_unchecked([
        0x31c7_912e_a267_eec6,
        0x1dbf_6f1c_5fcd_b700,
        0xd30d_4fe3_ba86_fdb1,
        0x3cae_528f_bee9_a2a4,
        0xb1cc_e69b_6aa9_ad9a,
        0x0443_93bb_632d_94fb,
    ]),
    Fp::from_raw_unchecked([
        0xc66e_f6ef_eeb5_c7e8,
        0x9824_c289_dd72_bb55,
        0x71b1_a4d2_f119_981d,
        0x104f_c1aa_fb09_19cc,
        0x0e49_df01_d942_a628,
        0x096c_3a09_7732_72d4,
    ]),
    Fp::from_raw_unchecked([
        0x9abc_11eb_5fad_eff4,
        0x32dc_a50a_8857_28f0,
        0xfb1f_a372_1569_734c,
        0xc4b7_6271_ea65_06b3,
        0xd466_a755_99ce_728e,
        0x0c81_d464_5f4c_b6ed,
    ]),
    Fp::from_raw_unchecked([
        0x4199_f10e_5b8b_e45b,
        0xda64_e495_b1e8_7930,
        0xcb35_3efe_9b33_e4ff,
        0x9e9e_fb24_aa64_24c6,
        0xf08d_3368_0a23_7465,
        0x0d33_7802_3e4c_7406,
    ]),
    Fp::from_raw_unchecked([
        0x7eb4_ae92_ec74_d3a5,
        0xc341_b4aa_9fac_3497,
        0x5be6_0389_9e90_7687,
        0x03bf_d9cc_a75c_bdeb,
        0x564c_2935_a96b_fa93,
        0x0ef3_c333_71e2_fdb5,
    ]),
    Fp::from_raw_unchecked([
        0x7ee9_1fd4_49f6_ac2e,
        0xe5d5_bd5c_b935_7a30,
        0x773a_8ca5_196b_1380,
        0xd0fd_a172_174e_d023,
        0x6cb9_5e0f_a776_aead,
        0x0d22_d5a4_0cec_7cff,
    ]),
    Fp::from_raw_unchecked([
        0xf727_e092_85fd_8519,
        0xdc9d_55a8_3017_897b,
        0x7549_d8bd_0578_94ae,
        0x1784_1961_3d90_d8f8,
        0xfce9_5ebd_eb5b_490a,
        0x0467_ffae_f23f_c49e,
    ]),
    Fp::from_raw_unchecked([
        0xc176_9e6a_7c38_5f1b,
        0x79bc_930d_eac0_1c03,
        0x5461_c75a_23ed_e3b5,
        0x6e20_829e_5c23_0c45,
        0x828e_0f1e_772a_53cd,
        0x116a_efa7_4912_7bff,
    ]),
    Fp::from_raw_unchecked([
        0x101c_10bf_2744_c10a,
        0xbbf1_8d05_3a6a_3154,
        0xa0ec_f39e_f026_f602,
        0xfc00_9d49_96dc_5153,
        0xb900_0209_d5bd_08d3,
        0x189e_5fe4_470c_d73c,
    ]),
    Fp::from_raw_unchecked([
        0x7ebd_546c_a157_5ed2,
        0xe47d_5a98_1d08_1b55,
        0x57b2_b625_b6d4_ca21,
        0xb0a1_ba04_2285_20cc,
        0x9873_8983_c210_7ff3,
        0x13dd_dbc4_799d_81d6,
    ]),
    Fp::from_raw_unchecked([
        0x0931_9f2e_3983_4935,
        0x039e_952c_bdb0_5c21,
        0x55ba_77a9_a2f7_6493,
        0xfd04_e3df_c608_6467,
        0xfb95_832e_7d78_742e,
        0x0ef9_c24e_ccaf_5e0e,
    ]),
    Fp::from_raw_unchecked([
        0x7609_0000_0002_fffd,
        0xebf4_000b_c40c_0002,
        0x5f48_9857_53c7_58ba,
        0x77ce_5853_7052_5745,
        0x5c07_1a97_a256_ec6d,
        0x15f6_5ec3_fa80_e493,
    ]),
];

const SSWU_ELLP_A: Fp = Fp::from_raw_unchecked([
    0x2f65_aa0e_9af5_aa51,
    0x8646_4c2d_1e84_16c3,
    0xb85c_e591_b7bd_31e2,
    0x27e1_1c91_b5f2_4e7c,
    0x2837_6eda_6bfc_1835,
    0x1554_55c3_e507_1d85,
]);

const SSWU_ELLP_B: Fp = Fp::from_raw_unchecked([
    0xfb99_6971_fe22_a1e0,
    0x9aa9_3eb3_5b74_2d6f,
    0x8c47_6013_de99_c5c4,
    0x873e_27c3_a221_e571,
    0xca72_b5e4_5a52_d888,
    0x0682_4061_418a_386b,
]);

const SSWU_XI: Fp = Fp::from_raw_unchecked([
    0x886c_0000_0023_ffdc,
    0x0f70_008d_3090_001d,
    0x7767_2417_ed58_28c3,
    0x9dac_23e9_43dc_1740,
    0x5055_3f1b_9c13_1521,
    0x078c_712f_be0a_b6e8,
]);

const SQRT_M_XI_CUBED: Fp = Fp::from_raw_unchecked([
    0x43b5_71ca_d321_5f1f,
    0xccb4_60ef_1c70_2dc2,
    0x742d_884f_4f97_100b,
    0xdb2c_3e32_38a3_382b,
    0xe40f_3fa1_3fce_8f88,
    0x0073_a2af_9892_a2ff,
]);

impl HashToField for Fp {
    // ceil(log2(p)) = 381, m = 1, k = 128.
    type InputLength = U64;

    fn from_okm(okm: &GenericArray<u8, U64>) -> Fp {
        const F_2_256: Fp = Fp::from_raw_unchecked([
            0x075b_3cd7_c5ce_820f,
            0x3ec6_ba62_1c3e_db0b,
            0x168a_13d8_2bff_6bce,
            0x8766_3c4b_f8c4_49d2,
            0x15f3_4c83_ddc8_d830,
            0x0f96_28b4_9caa_2e85,
        ]);

        let mut bs = [0u8; 48];
        bs[16..].copy_from_slice(&okm[..32]);
        let db = Fp::from_bytes(&bs).unwrap();

        bs[16..].copy_from_slice(&okm[32..]);
        let da = Fp::from_bytes(&bs).unwrap();

        db * F_2_256 + da
    }
}

impl Sgn0 for Fp {
    fn sgn0(&self) -> Choice {
        // Turn into canonical form by computing
        // (a.R) / R = a
        let tmp = Fp::montgomery_reduce(
            self.0[0], self.0[1], self.0[2], self.0[3], self.0[4], self.0[5], 0, 0, 0, 0, 0, 0,
        );
        Choice::from((tmp.0[0] & 1) as u8)
    }
}

/// Maps an element of [`Fp`] to a point on iso-G1.
///
/// Implements [section 6.6.2 of `draft-irtf-cfrg-hash-to-curve-12`][sswu].
///
/// [sswu]: https://datatracker.ietf.org/doc/html/draft-irtf-cfrg-hash-to-curve-12#section-6.6.2
fn map_to_curve_simple_swu(u: &Fp) -> G1Projective {
    let usq = u.square();
    let xi_usq = SSWU_XI * usq;
    let xisq_u4 = xi_usq.square();
    let nd_common = xisq_u4 + xi_usq; // XI^2 * u^4 + XI * u^2
    let x_den = SSWU_ELLP_A * Fp::conditional_select(&(-nd_common), &SSWU_XI, nd_common.is_zero());
    let x0_num = SSWU_ELLP_B * (Fp::one() + nd_common); // B * (1 + (XI^2 * u^4 + XI * u^2))

    // compute g(x0(u))
    let x_densq = x_den.square();
    let gx_den = x_densq * x_den;
    // x0_num^3 + A * x0_num * x_den^2 + B * x_den^3
    let gx0_num = (x0_num.square() + SSWU_ELLP_A * x_densq) * x0_num + SSWU_ELLP_B * gx_den;

    // compute g(X0(u)) ^ ((p - 3) // 4)
    let sqrt_candidate = {
        let u_v = gx0_num * gx_den; // u*v
        let vsq = gx_den.square(); // v^2
        u_v * chain_pm3div4(&(u_v * vsq)) // u v (u v^3) ^ ((p - 3) // 4)
    };

    let gx0_square = (sqrt_candidate.square() * gx_den).ct_eq(&gx0_num); // g(x0) is square
    let x1_num = x0_num * xi_usq;
    // sqrt(-XI**3) * u^3 g(x0) ^ ((p - 3) // 4)
    let y1 = SQRT_M_XI_CUBED * usq * u * sqrt_candidate;

    let x_num = Fp::conditional_select(&x1_num, &x0_num, gx0_square);
    let mut y = Fp::conditional_select(&y1, &sqrt_candidate, gx0_square);
    // ensure sign of y and sign of u agree
    y.conditional_negate(y.sgn0() ^ u.sgn0());

    G1Projective {
        x: x_num,
        y: y * x_den,
        z: x_den,
    }
}

/// Maps an iso-G1 point to a G1 point.
fn iso_map(u: &G1Projective) -> G1Projective {
    const COEFFS: [&[Fp]; 4] = [&ISO11_XNUM, &ISO11_XDEN, &ISO11_YNUM, &ISO11_YDEN];

    // unpack input point
    let G1Projective { x, y, z } = *u;

    // xnum, xden, ynum, yden
    let mut mapvals = [Fp::zero(); 4];

    // pre-compute powers of z
    let zpows = {
        let mut zpows = [Fp::zero(); 15];
        zpows[0] = z;
        for idx in 1..zpows.len() {
            zpows[idx] = zpows[idx - 1] * z;
        }
        zpows
    };

    // compute map value by Horner's rule
    for idx in 0..4 {
        let coeff = COEFFS[idx];
        let clast = coeff.len() - 1;
        mapvals[idx] = coeff[clast];
        for jdx in 0..clast {
            mapvals[idx] = mapvals[idx] * x + zpows[jdx] * coeff[clast - 1 - jdx];
        }
    }

    // x denominator is order 1 less than x numerator, so we need an extra factor of z
    mapvals[1] *= z;

    // multiply result of Y map by the y-coord, y / z
    mapvals[2] *= y;
    mapvals[3] *= z;

    G1Projective {
        x: mapvals[0] * mapvals[3], // xnum * yden,
        y: mapvals[2] * mapvals[1], // ynum * xden,
        z: mapvals[1] * mapvals[3], // xden * yden
    }
}

impl MapToCurve for G1Projective {
    type Field = Fp;

    fn map_to_curve(u: &Fp) -> G1Projective {
        let pt = map_to_curve_simple_swu(u);
        iso_map(&pt)
    }

    fn clear_h(&self) -> Self {
        self.clear_cofactor()
    }
}

#[cfg(test)]
fn check_g1_prime(pt: &G1Projective) -> bool {
    // (X : Y : Z)==(X/Z, Y/Z) is on E': y^2 = x^3 + A * x + B.
    // y^2 z = (x^3) + A (x z^2) + B z^3
    let zsq = pt.z.square();
    (pt.y.square() * pt.z)
        == (pt.x.square() * pt.x + SSWU_ELLP_A * pt.x * zsq + SSWU_ELLP_B * zsq * pt.z)
}

#[test]
fn test_simple_swu_expected() {
    // exceptional case: zero
    let p = map_to_curve_simple_swu(&Fp::zero());
    let G1Projective { x, y, z } = &p;
    let xo = Fp::from_raw_unchecked([
        0xfb99_6971_fe22_a1e0,
        0x9aa9_3eb3_5b74_2d6f,
        0x8c47_6013_de99_c5c4,
        0x873e_27c3_a221_e571,
        0xca72_b5e4_5a52_d888,
        0x0682_4061_418a_386b,
    ]);
    let yo = Fp::from_raw_unchecked([
        0xfd6f_ced8_7a7f_11a3,
        0x9a6b_314b_03c8_db31,
        0x41f8_5416_e0ea_b593,
        0xfeeb_089f_7e6e_c4d7,
        0x85a1_34c3_7ed1_278f,
        0x0575_c525_bb9f_74bb,
    ]);
    let zo = Fp::from_raw_unchecked([
        0x7f67_4ea0_a891_5178,
        0xb0f9_45fc_13b8_fa65,
        0x4b46_759a_38e8_7d76,
        0x2e7a_9296_41bb_b6a1,
        0x1668_ddfa_462b_f6b6,
        0x0096_0e2e_d1cf_294c,
    ]);
    assert_eq!(x, &xo);
    assert_eq!(y, &yo);
    assert_eq!(z, &zo);
    assert!(check_g1_prime(&p));

    // exceptional case: sqrt(-1/XI) (positive)
    let excp = Fp::from_raw_unchecked([
        0x00f3_d047_7e91_edbf,
        0x08d6_621e_4ca8_dc69,
        0xb9cf_7927_b19b_9726,
        0xba13_3c99_6caf_a2ec,
        0xed2a_5ccd_5ca7_bb68,
        0x19cb_022f_8ee9_d73b,
    ]);
    let p = map_to_curve_simple_swu(&excp);
    let G1Projective { x, y, z } = &p;
    assert_eq!(x, &xo);
    assert_eq!(y, &yo);
    assert_eq!(z, &zo);
    assert!(check_g1_prime(&p));

    // exceptional case: sqrt(-1/XI) (negative)
    let excp = Fp::from_raw_unchecked([
        0xb90b_2fb8_816d_bcec,
        0x15d5_9de0_64ab_2396,
        0xad61_5979_4515_5efe,
        0xaa64_0eeb_86d5_6fd2,
        0x5df1_4ae8_e6a3_f16e,
        0x0036_0fba_aa96_0f5e,
    ]);
    let p = map_to_curve_simple_swu(&excp);
    let G1Projective { x, y, z } = &p;
    let myo = -yo;
    assert_eq!(x, &xo);
    assert_eq!(y, &myo);
    assert_eq!(z, &zo);
    assert!(check_g1_prime(&p));

    let u = Fp::from_raw_unchecked([
        0xa618_fa19_f7e2_eadc,
        0x93c7_f1fc_876b_a245,
        0xe2ed_4cc4_7b5c_0ae0,
        0xd49e_fa74_e4a8_d000,
        0xa0b2_3ba6_92b5_431c,
        0x0d15_51f2_d7d8_d193,
    ]);
    let xo = Fp::from_raw_unchecked([
        0x2197_ca55_fab3_ba48,
        0x591d_eb39_f434_949a,
        0xf9df_7fb4_f1fa_6a08,
        0x59e3_c16a_9dfa_8fa5,
        0xe592_9b19_4aad_5f7a,
        0x130a_46a4_c61b_44ed,
    ]);
    let yo = Fp::from_raw_unchecked([
        0xf721_5b58_c720_0ad0,
        0x8905_1631_3a4e_66bf,
        0xc903_1acc_8a36_19a8,
        0xea1f_9978_fde3_ffec,
        0x0548_f02d_6cfb_f472,
        0x1693_7557_3529_163f,
    ]);
    let zo = Fp::from_raw_unchecked([
        0xf36f_eb2e_1128_ade0,
        0x42e2_2214_250b_cd94,
        0xb94f_6ba2_dddf_62d6,
        0xf56d_4392_782b_f0a2,
        0xb2d7_ce1e_c263_09e7,
        0x182b_57ed_6b99_f0a1,
    ]);
    let p = map_to_curve_simple_swu(&u);
    let G1Projective { x, y, z } = &p;
    assert_eq!(x, &xo);
    assert_eq!(y, &yo);
    assert_eq!(z, &zo);
    assert!(check_g1_prime(&p));
}

#[test]
fn test_osswu_semirandom() {
    use rand_core::SeedableRng;
    let mut rng = rand_xorshift::XorShiftRng::from_seed([
        0x59, 0x62, 0xbe, 0x5d, 0x76, 0x3d, 0x31, 0x8d, 0x17, 0xdb, 0x37, 0x32, 0x54, 0x06, 0xbc,
        0xe5,
    ]);
    for _ in 0..32 {
        let input = Fp::random(&mut rng);
        let p = map_to_curve_simple_swu(&input);
        assert!(check_g1_prime(&p));

        let p_iso = iso_map(&p);
        assert!(bool::from(p_iso.is_on_curve()));
    }
}

// test vectors from the draft 10 RFC
#[test]
fn test_encode_to_curve_10() {
    use crate::{
        g1::G1Affine,
        hash_to_curve::{ExpandMsgXmd, HashToCurve},
    };
    use std::string::{String, ToString};

    struct TestCase {
        msg: &'static [u8],
        expected: [&'static str; 2],
    }
    impl TestCase {
        fn expected(&self) -> String {
            self.expected[0].to_string() + self.expected[1]
        }
    }

    const DOMAIN: &[u8] = b"QUUX-V01-CS02-with-BLS12381G1_XMD:SHA-256_SSWU_NU_";

    let cases = vec![
        TestCase {
            msg: b"",
            expected: [
                "184bb665c37ff561a89ec2122dd343f20e0f4cbcaec84e3c3052ea81d1834e192c426074b02ed3dca4e7676ce4ce48ba",
                "04407b8d35af4dacc809927071fc0405218f1401a6d15af775810e4e460064bcc9468beeba82fdc751be70476c888bf3",
            ],
        },
        TestCase {
            msg: b"abc",
            expected: [
                "009769f3ab59bfd551d53a5f846b9984c59b97d6842b20a2c565baa167945e3d026a3755b6345df8ec7e6acb6868ae6d",
                "1532c00cf61aa3d0ce3e5aa20c3b531a2abd2c770a790a2613818303c6b830ffc0ecf6c357af3317b9575c567f11cd2c",
            ],
        },
        TestCase {
            msg: b"abcdef0123456789",
            expected: [
                "1974dbb8e6b5d20b84df7e625e2fbfecb2cdb5f77d5eae5fb2955e5ce7313cae8364bc2fff520a6c25619739c6bdcb6a",
                "15f9897e11c6441eaa676de141c8d83c37aab8667173cbe1dfd6de74d11861b961dccebcd9d289ac633455dfcc7013a3",
            ]
        },
        TestCase {
            msg: b"q128_qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq\
                   qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq\
                   qqqqqqqqqqqqqqqqqqqqqqqqq",
            expected: [
        "0a7a047c4a8397b3446450642c2ac64d7239b61872c9ae7a59707a8f4f950f101e766afe58223b3bff3a19a7f754027c",
        "1383aebba1e4327ccff7cf9912bda0dbc77de048b71ef8c8a81111d71dc33c5e3aa6edee9cf6f5fe525d50cc50b77cc9",
            ]
        },
        TestCase {
            msg: b"a512_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
                   aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
                   aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
                   aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
                   aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
                   aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
                   aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
                   aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
                   aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
                   aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
            expected: [
                "0e7a16a975904f131682edbb03d9560d3e48214c9986bd50417a77108d13dc957500edf96462a3d01e62dc6cd468ef11",
                "0ae89e677711d05c30a48d6d75e76ca9fb70fe06c6dd6ff988683d89ccde29ac7d46c53bb97a59b1901abf1db66052db",
            ]
        }
    ];

    for case in cases {
        let g = <G1Projective as HashToCurve<ExpandMsgXmd<sha2::Sha256>>>::encode_to_curve(
            &case.msg, DOMAIN,
        );
        let aff = G1Affine::from(g);
        let g_uncompressed = aff.to_uncompressed();

        assert_eq!(case.expected(), hex::encode(&g_uncompressed[..]));
    }
}

// test vectors from the draft 10 RFC
#[test]
fn test_hash_to_curve_10() {
    use crate::{
        g1::G1Affine,
        hash_to_curve::{ExpandMsgXmd, HashToCurve},
    };
    use std::string::{String, ToString};

    struct TestCase {
        msg: &'static [u8],
        expected: [&'static str; 2],
    }
    impl TestCase {
        fn expected(&self) -> String {
            self.expected[0].to_string() + self.expected[1]
        }
    }

    const DOMAIN: &[u8] = b"QUUX-V01-CS02-with-BLS12381G1_XMD:SHA-256_SSWU_RO_";

    let cases = vec![
        TestCase {
            msg: b"",
            expected: [
                "052926add2207b76ca4fa57a8734416c8dc95e24501772c814278700eed6d1e4e8cf62d9c09db0fac349612b759e79a1",
                "08ba738453bfed09cb546dbb0783dbb3a5f1f566ed67bb6be0e8c67e2e81a4cc68ee29813bb7994998f3eae0c9c6a265",
            ],
        },
        TestCase {
            msg: b"abc",
            expected: [
                "03567bc5ef9c690c2ab2ecdf6a96ef1c139cc0b2f284dca0a9a7943388a49a3aee664ba5379a7655d3c68900be2f6903",
                "0b9c15f3fe6e5cf4211f346271d7b01c8f3b28be689c8429c85b67af215533311f0b8dfaaa154fa6b88176c229f2885d"
            ],
        },
        TestCase {
            msg: b"abcdef0123456789",
            expected: [
                "11e0b079dea29a68f0383ee94fed1b940995272407e3bb916bbf268c263ddd57a6a27200a784cbc248e84f357ce82d98",
                "03a87ae2caf14e8ee52e51fa2ed8eefe80f02457004ba4d486d6aa1f517c0889501dc7413753f9599b099ebcbbd2d709"
            ]
        },
        TestCase {
            msg: b"q128_qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq\
                   qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq\
                   qqqqqqqqqqqqqqqqqqqqqqqqq",
            expected: [
                "15f68eaa693b95ccb85215dc65fa81038d69629f70aeee0d0f677cf22285e7bf58d7cb86eefe8f2e9bc3f8cb84fac488",
                "1807a1d50c29f430b8cafc4f8638dfeeadf51211e1602a5f184443076715f91bb90a48ba1e370edce6ae1062f5e6dd38"
            ]
        },
        TestCase {
            msg: b"a512_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
                   aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
                   aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
                   aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
                   aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
                   aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
                   aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
                   aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
                   aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
                   aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
            expected: [
                "082aabae8b7dedb0e78aeb619ad3bfd9277a2f77ba7fad20ef6aabdc6c31d19ba5a6d12283553294c1825c4b3ca2dcfe",
                "05b84ae5a942248eea39e1d91030458c40153f3b654ab7872d779ad1e942856a20c438e8d99bc8abfbf74729ce1f7ac8"
            ]
        }
    ];

    for case in cases {
        let g = <G1Projective as HashToCurve<ExpandMsgXmd<sha2::Sha256>>>::hash_to_curve(
            &case.msg, DOMAIN,
        );
        let g_uncompressed = G1Affine::from(g).to_uncompressed();

        assert_eq!(case.expected(), hex::encode(&g_uncompressed[..]));
    }
}

#[cfg(test)]
// p-1 / 2
pub const P_M1_OVER2: Fp = Fp::from_raw_unchecked([
    0xa1fa_ffff_fffe_5557,
    0x995b_fff9_76a3_fffe,
    0x03f4_1d24_d174_ceb4,
    0xf654_7998_c199_5dbd,
    0x778a_468f_507a_6034,
    0x0205_5993_1f7f_8103,
]);

#[test]
fn test_sgn0() {
    assert_eq!(bool::from(Fp::zero().sgn0()), false);
    assert_eq!(bool::from(Fp::one().sgn0()), true);
    assert_eq!(bool::from((-Fp::one()).sgn0()), false);
    assert_eq!(bool::from((-Fp::zero()).sgn0()), false);
    assert_eq!(bool::from(P_M1_OVER2.sgn0()), true);

    let p_p1_over2 = P_M1_OVER2 + Fp::one();
    assert_eq!(bool::from(p_p1_over2.sgn0()), false);

    let neg_p_p1_over2 = {
        let mut tmp = p_p1_over2;
        tmp.conditional_negate(Choice::from(1u8));
        tmp
    };
    assert_eq!(neg_p_p1_over2, P_M1_OVER2);
}
