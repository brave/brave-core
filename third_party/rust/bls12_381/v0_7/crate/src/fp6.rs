use crate::fp::*;
use crate::fp2::*;

use core::fmt;
use core::ops::{Add, AddAssign, Mul, MulAssign, Neg, Sub, SubAssign};
use subtle::{Choice, ConditionallySelectable, ConstantTimeEq, CtOption};

#[cfg(feature = "pairings")]
use rand_core::RngCore;

/// This represents an element $c_0 + c_1 v + c_2 v^2$ of $\mathbb{F}_{p^6} = \mathbb{F}_{p^2} / v^3 - u - 1$.
pub struct Fp6 {
    pub c0: Fp2,
    pub c1: Fp2,
    pub c2: Fp2,
}

impl From<Fp> for Fp6 {
    fn from(f: Fp) -> Fp6 {
        Fp6 {
            c0: Fp2::from(f),
            c1: Fp2::zero(),
            c2: Fp2::zero(),
        }
    }
}

impl From<Fp2> for Fp6 {
    fn from(f: Fp2) -> Fp6 {
        Fp6 {
            c0: f,
            c1: Fp2::zero(),
            c2: Fp2::zero(),
        }
    }
}

impl PartialEq for Fp6 {
    fn eq(&self, other: &Fp6) -> bool {
        self.ct_eq(other).into()
    }
}

impl Copy for Fp6 {}
impl Clone for Fp6 {
    #[inline]
    fn clone(&self) -> Self {
        *self
    }
}

impl Default for Fp6 {
    fn default() -> Self {
        Fp6::zero()
    }
}

#[cfg(feature = "zeroize")]
impl zeroize::DefaultIsZeroes for Fp6 {}

impl fmt::Debug for Fp6 {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?} + ({:?})*v + ({:?})*v^2", self.c0, self.c1, self.c2)
    }
}

impl ConditionallySelectable for Fp6 {
    #[inline(always)]
    fn conditional_select(a: &Self, b: &Self, choice: Choice) -> Self {
        Fp6 {
            c0: Fp2::conditional_select(&a.c0, &b.c0, choice),
            c1: Fp2::conditional_select(&a.c1, &b.c1, choice),
            c2: Fp2::conditional_select(&a.c2, &b.c2, choice),
        }
    }
}

impl ConstantTimeEq for Fp6 {
    #[inline(always)]
    fn ct_eq(&self, other: &Self) -> Choice {
        self.c0.ct_eq(&other.c0) & self.c1.ct_eq(&other.c1) & self.c2.ct_eq(&other.c2)
    }
}

impl Fp6 {
    #[inline]
    pub fn zero() -> Self {
        Fp6 {
            c0: Fp2::zero(),
            c1: Fp2::zero(),
            c2: Fp2::zero(),
        }
    }

    #[inline]
    pub fn one() -> Self {
        Fp6 {
            c0: Fp2::one(),
            c1: Fp2::zero(),
            c2: Fp2::zero(),
        }
    }

    #[cfg(feature = "pairings")]
    pub(crate) fn random(mut rng: impl RngCore) -> Self {
        Fp6 {
            c0: Fp2::random(&mut rng),
            c1: Fp2::random(&mut rng),
            c2: Fp2::random(&mut rng),
        }
    }

    pub fn mul_by_1(&self, c1: &Fp2) -> Fp6 {
        let b_b = self.c1 * c1;

        let t1 = (self.c1 + self.c2) * c1 - b_b;
        let t1 = t1.mul_by_nonresidue();

        let t2 = (self.c0 + self.c1) * c1 - b_b;

        Fp6 {
            c0: t1,
            c1: t2,
            c2: b_b,
        }
    }

    pub fn mul_by_01(&self, c0: &Fp2, c1: &Fp2) -> Fp6 {
        let a_a = self.c0 * c0;
        let b_b = self.c1 * c1;

        let t1 = (self.c1 + self.c2) * c1 - b_b;
        let t1 = t1.mul_by_nonresidue() + a_a;

        let t2 = (c0 + c1) * (self.c0 + self.c1) - a_a - b_b;

        let t3 = (self.c0 + self.c2) * c0 - a_a + b_b;

        Fp6 {
            c0: t1,
            c1: t2,
            c2: t3,
        }
    }

    /// Multiply by quadratic nonresidue v.
    pub fn mul_by_nonresidue(&self) -> Self {
        // Given a + bv + cv^2, this produces
        //     av + bv^2 + cv^3
        // but because v^3 = u + 1, we have
        //     c(u + 1) + av + v^2

        Fp6 {
            c0: self.c2.mul_by_nonresidue(),
            c1: self.c0,
            c2: self.c1,
        }
    }

    /// Raises this element to p.
    #[inline(always)]
    pub fn frobenius_map(&self) -> Self {
        let c0 = self.c0.frobenius_map();
        let c1 = self.c1.frobenius_map();
        let c2 = self.c2.frobenius_map();

        // c1 = c1 * (u + 1)^((p - 1) / 3)
        let c1 = c1
            * Fp2 {
                c0: Fp::zero(),
                c1: Fp::from_raw_unchecked([
                    0xcd03_c9e4_8671_f071,
                    0x5dab_2246_1fcd_a5d2,
                    0x5870_42af_d385_1b95,
                    0x8eb6_0ebe_01ba_cb9e,
                    0x03f9_7d6e_83d0_50d2,
                    0x18f0_2065_5463_8741,
                ]),
            };

        // c2 = c2 * (u + 1)^((2p - 2) / 3)
        let c2 = c2
            * Fp2 {
                c0: Fp::from_raw_unchecked([
                    0x890d_c9e4_8675_45c3,
                    0x2af3_2253_3285_a5d5,
                    0x5088_0866_309b_7e2c,
                    0xa20d_1b8c_7e88_1024,
                    0x14e4_f04f_e2db_9068,
                    0x14e5_6d3f_1564_853a,
                ]),
                c1: Fp::zero(),
            };

        Fp6 { c0, c1, c2 }
    }

    #[inline(always)]
    pub fn is_zero(&self) -> Choice {
        self.c0.is_zero() & self.c1.is_zero() & self.c2.is_zero()
    }

    /// Returns `c = self * b`.
    ///
    /// Implements the full-tower interleaving strategy from
    /// [ePrint 2022-376](https://eprint.iacr.org/2022/367).
    #[inline]
    fn mul_interleaved(&self, b: &Self) -> Self {
        // The intuition for this algorithm is that we can look at F_p^6 as a direct
        // extension of F_p^2, and express the overall operations down to the base field
        // F_p instead of only over F_p^2. This enables us to interleave multiplications
        // and reductions, ensuring that we don't require double-width intermediate
        // representations (with around twice as many limbs as F_p elements).

        // We want to express the multiplication c = a x b, where a = (a_0, a_1, a_2) is
        // an element of F_p^6, and a_i = (a_i,0, a_i,1) is an element of F_p^2. The fully
        // expanded multiplication is given by (2022-376 §5):
        //
        //   c_0,0 = a_0,0 b_0,0 - a_0,1 b_0,1 + a_1,0 b_2,0 - a_1,1 b_2,1 + a_2,0 b_1,0 - a_2,1 b_1,1
        //                                     - a_1,0 b_2,1 - a_1,1 b_2,0 - a_2,0 b_1,1 - a_2,1 b_1,0.
        //         = a_0,0 b_0,0 - a_0,1 b_0,1 + a_1,0 (b_2,0 - b_2,1) - a_1,1 (b_2,0 + b_2,1)
        //                                     + a_2,0 (b_1,0 - b_1,1) - a_2,1 (b_1,0 + b_1,1).
        //
        //   c_0,1 = a_0,0 b_0,1 + a_0,1 b_0,0 + a_1,0 b_2,1 + a_1,1 b_2,0 + a_2,0 b_1,1 + a_2,1 b_1,0
        //                                     + a_1,0 b_2,0 - a_1,1 b_2,1 + a_2,0 b_1,0 - a_2,1 b_1,1.
        //         = a_0,0 b_0,1 + a_0,1 b_0,0 + a_1,0(b_2,0 + b_2,1) + a_1,1(b_2,0 - b_2,1)
        //                                     + a_2,0(b_1,0 + b_1,1) + a_2,1(b_1,0 - b_1,1).
        //
        //   c_1,0 = a_0,0 b_1,0 - a_0,1 b_1,1 + a_1,0 b_0,0 - a_1,1 b_0,1 + a_2,0 b_2,0 - a_2,1 b_2,1
        //                                                                 - a_2,0 b_2,1 - a_2,1 b_2,0.
        //         = a_0,0 b_1,0 - a_0,1 b_1,1 + a_1,0 b_0,0 - a_1,1 b_0,1 + a_2,0(b_2,0 - b_2,1)
        //                                                                 - a_2,1(b_2,0 + b_2,1).
        //
        //   c_1,1 = a_0,0 b_1,1 + a_0,1 b_1,0 + a_1,0 b_0,1 + a_1,1 b_0,0 + a_2,0 b_2,1 + a_2,1 b_2,0
        //                                                                 + a_2,0 b_2,0 - a_2,1 b_2,1
        //         = a_0,0 b_1,1 + a_0,1 b_1,0 + a_1,0 b_0,1 + a_1,1 b_0,0 + a_2,0(b_2,0 + b_2,1)
        //                                                                 + a_2,1(b_2,0 - b_2,1).
        //
        //   c_2,0 = a_0,0 b_2,0 - a_0,1 b_2,1 + a_1,0 b_1,0 - a_1,1 b_1,1 + a_2,0 b_0,0 - a_2,1 b_0,1.
        //   c_2,1 = a_0,0 b_2,1 + a_0,1 b_2,0 + a_1,0 b_1,1 + a_1,1 b_1,0 + a_2,0 b_0,1 + a_2,1 b_0,0.
        //
        // Each of these is a "sum of products", which we can compute efficiently.

        let a = self;
        let b10_p_b11 = b.c1.c0 + b.c1.c1;
        let b10_m_b11 = b.c1.c0 - b.c1.c1;
        let b20_p_b21 = b.c2.c0 + b.c2.c1;
        let b20_m_b21 = b.c2.c0 - b.c2.c1;

        Fp6 {
            c0: Fp2 {
                c0: Fp::sum_of_products(
                    [a.c0.c0, -a.c0.c1, a.c1.c0, -a.c1.c1, a.c2.c0, -a.c2.c1],
                    [b.c0.c0, b.c0.c1, b20_m_b21, b20_p_b21, b10_m_b11, b10_p_b11],
                ),
                c1: Fp::sum_of_products(
                    [a.c0.c0, a.c0.c1, a.c1.c0, a.c1.c1, a.c2.c0, a.c2.c1],
                    [b.c0.c1, b.c0.c0, b20_p_b21, b20_m_b21, b10_p_b11, b10_m_b11],
                ),
            },
            c1: Fp2 {
                c0: Fp::sum_of_products(
                    [a.c0.c0, -a.c0.c1, a.c1.c0, -a.c1.c1, a.c2.c0, -a.c2.c1],
                    [b.c1.c0, b.c1.c1, b.c0.c0, b.c0.c1, b20_m_b21, b20_p_b21],
                ),
                c1: Fp::sum_of_products(
                    [a.c0.c0, a.c0.c1, a.c1.c0, a.c1.c1, a.c2.c0, a.c2.c1],
                    [b.c1.c1, b.c1.c0, b.c0.c1, b.c0.c0, b20_p_b21, b20_m_b21],
                ),
            },
            c2: Fp2 {
                c0: Fp::sum_of_products(
                    [a.c0.c0, -a.c0.c1, a.c1.c0, -a.c1.c1, a.c2.c0, -a.c2.c1],
                    [b.c2.c0, b.c2.c1, b.c1.c0, b.c1.c1, b.c0.c0, b.c0.c1],
                ),
                c1: Fp::sum_of_products(
                    [a.c0.c0, a.c0.c1, a.c1.c0, a.c1.c1, a.c2.c0, a.c2.c1],
                    [b.c2.c1, b.c2.c0, b.c1.c1, b.c1.c0, b.c0.c1, b.c0.c0],
                ),
            },
        }
    }

    #[inline]
    pub fn square(&self) -> Self {
        let s0 = self.c0.square();
        let ab = self.c0 * self.c1;
        let s1 = ab + ab;
        let s2 = (self.c0 - self.c1 + self.c2).square();
        let bc = self.c1 * self.c2;
        let s3 = bc + bc;
        let s4 = self.c2.square();

        Fp6 {
            c0: s3.mul_by_nonresidue() + s0,
            c1: s4.mul_by_nonresidue() + s1,
            c2: s1 + s2 + s3 - s0 - s4,
        }
    }

    #[inline]
    pub fn invert(&self) -> CtOption<Self> {
        let c0 = (self.c1 * self.c2).mul_by_nonresidue();
        let c0 = self.c0.square() - c0;

        let c1 = self.c2.square().mul_by_nonresidue();
        let c1 = c1 - (self.c0 * self.c1);

        let c2 = self.c1.square();
        let c2 = c2 - (self.c0 * self.c2);

        let tmp = ((self.c1 * c2) + (self.c2 * c1)).mul_by_nonresidue();
        let tmp = tmp + (self.c0 * c0);

        tmp.invert().map(|t| Fp6 {
            c0: t * c0,
            c1: t * c1,
            c2: t * c2,
        })
    }
}

impl<'a, 'b> Mul<&'b Fp6> for &'a Fp6 {
    type Output = Fp6;

    #[inline]
    fn mul(self, other: &'b Fp6) -> Self::Output {
        self.mul_interleaved(other)
    }
}

impl<'a, 'b> Add<&'b Fp6> for &'a Fp6 {
    type Output = Fp6;

    #[inline]
    fn add(self, rhs: &'b Fp6) -> Self::Output {
        Fp6 {
            c0: self.c0 + rhs.c0,
            c1: self.c1 + rhs.c1,
            c2: self.c2 + rhs.c2,
        }
    }
}

impl<'a> Neg for &'a Fp6 {
    type Output = Fp6;

    #[inline]
    fn neg(self) -> Self::Output {
        Fp6 {
            c0: -self.c0,
            c1: -self.c1,
            c2: -self.c2,
        }
    }
}

impl Neg for Fp6 {
    type Output = Fp6;

    #[inline]
    fn neg(self) -> Self::Output {
        -&self
    }
}

impl<'a, 'b> Sub<&'b Fp6> for &'a Fp6 {
    type Output = Fp6;

    #[inline]
    fn sub(self, rhs: &'b Fp6) -> Self::Output {
        Fp6 {
            c0: self.c0 - rhs.c0,
            c1: self.c1 - rhs.c1,
            c2: self.c2 - rhs.c2,
        }
    }
}

impl_binops_additive!(Fp6, Fp6);
impl_binops_multiplicative!(Fp6, Fp6);

#[test]
fn test_arithmetic() {
    use crate::fp::*;

    let a = Fp6 {
        c0: Fp2 {
            c0: Fp::from_raw_unchecked([
                0x47f9_cb98_b1b8_2d58,
                0x5fe9_11eb_a3aa_1d9d,
                0x96bf_1b5f_4dd8_1db3,
                0x8100_d27c_c925_9f5b,
                0xafa2_0b96_7464_0eab,
                0x09bb_cea7_d8d9_497d,
            ]),
            c1: Fp::from_raw_unchecked([
                0x0303_cb98_b166_2daa,
                0xd931_10aa_0a62_1d5a,
                0xbfa9_820c_5be4_a468,
                0x0ba3_643e_cb05_a348,
                0xdc35_34bb_1f1c_25a6,
                0x06c3_05bb_19c0_e1c1,
            ]),
        },
        c1: Fp2 {
            c0: Fp::from_raw_unchecked([
                0x46f9_cb98_b162_d858,
                0x0be9_109c_f7aa_1d57,
                0xc791_bc55_fece_41d2,
                0xf84c_5770_4e38_5ec2,
                0xcb49_c1d9_c010_e60f,
                0x0acd_b8e1_58bf_e3c8,
            ]),
            c1: Fp::from_raw_unchecked([
                0x8aef_cb98_b15f_8306,
                0x3ea1_108f_e4f2_1d54,
                0xcf79_f69f_a1b7_df3b,
                0xe4f5_4aa1_d16b_1a3c,
                0xba5e_4ef8_6105_a679,
                0x0ed8_6c07_97be_e5cf,
            ]),
        },
        c2: Fp2 {
            c0: Fp::from_raw_unchecked([
                0xcee5_cb98_b15c_2db4,
                0x7159_1082_d23a_1d51,
                0xd762_30e9_44a1_7ca4,
                0xd19e_3dd3_549d_d5b6,
                0xa972_dc17_01fa_66e3,
                0x12e3_1f2d_d6bd_e7d6,
            ]),
            c1: Fp::from_raw_unchecked([
                0xad2a_cb98_b173_2d9d,
                0x2cfd_10dd_0696_1d64,
                0x0739_6b86_c6ef_24e8,
                0xbd76_e2fd_b1bf_c820,
                0x6afe_a7f6_de94_d0d5,
                0x1099_4b0c_5744_c040,
            ]),
        },
    };

    let b = Fp6 {
        c0: Fp2 {
            c0: Fp::from_raw_unchecked([
                0xf120_cb98_b16f_d84b,
                0x5fb5_10cf_f3de_1d61,
                0x0f21_a5d0_69d8_c251,
                0xaa1f_d62f_34f2_839a,
                0x5a13_3515_7f89_913f,
                0x14a3_fe32_9643_c247,
            ]),
            c1: Fp::from_raw_unchecked([
                0x3516_cb98_b16c_82f9,
                0x926d_10c2_e126_1d5f,
                0x1709_e01a_0cc2_5fba,
                0x96c8_c960_b825_3f14,
                0x4927_c234_207e_51a9,
                0x18ae_b158_d542_c44e,
            ]),
        },
        c1: Fp2 {
            c0: Fp::from_raw_unchecked([
                0xbf0d_cb98_b169_82fc,
                0xa679_10b7_1d1a_1d5c,
                0xb7c1_47c2_b8fb_06ff,
                0x1efa_710d_47d2_e7ce,
                0xed20_a79c_7e27_653c,
                0x02b8_5294_dac1_dfba,
            ]),
            c1: Fp::from_raw_unchecked([
                0x9d52_cb98_b180_82e5,
                0x621d_1111_5176_1d6f,
                0xe798_8260_3b48_af43,
                0x0ad3_1637_a4f4_da37,
                0xaeac_737c_5ac1_cf2e,
                0x006e_7e73_5b48_b824,
            ]),
        },
        c2: Fp2 {
            c0: Fp::from_raw_unchecked([
                0xe148_cb98_b17d_2d93,
                0x94d5_1104_3ebe_1d6c,
                0xef80_bca9_de32_4cac,
                0xf77c_0969_2827_95b1,
                0x9dc1_009a_fbb6_8f97,
                0x0479_3199_9a47_ba2b,
            ]),
            c1: Fp::from_raw_unchecked([
                0x253e_cb98_b179_d841,
                0xc78d_10f7_2c06_1d6a,
                0xf768_f6f3_811b_ea15,
                0xe424_fc9a_ab5a_512b,
                0x8cd5_8db9_9cab_5001,
                0x0883_e4bf_d946_bc32,
            ]),
        },
    };

    let c = Fp6 {
        c0: Fp2 {
            c0: Fp::from_raw_unchecked([
                0x6934_cb98_b176_82ef,
                0xfa45_10ea_194e_1d67,
                0xff51_313d_2405_877e,
                0xd0cd_efcc_2e8d_0ca5,
                0x7bea_1ad8_3da0_106b,
                0x0c8e_97e6_1845_be39,
            ]),
            c1: Fp::from_raw_unchecked([
                0x4779_cb98_b18d_82d8,
                0xb5e9_1144_4daa_1d7a,
                0x2f28_6bda_a653_2fc2,
                0xbca6_94f6_8bae_ff0f,
                0x3d75_e6b8_1a3a_7a5d,
                0x0a44_c3c4_98cc_96a3,
            ]),
        },
        c1: Fp2 {
            c0: Fp::from_raw_unchecked([
                0x8b6f_cb98_b18a_2d86,
                0xe8a1_1137_3af2_1d77,
                0x3710_a624_493c_cd2b,
                0xa94f_8828_0ee1_ba89,
                0x2c8a_73d6_bb2f_3ac7,
                0x0e4f_76ea_d7cb_98aa,
            ]),
            c1: Fp::from_raw_unchecked([
                0xcf65_cb98_b186_d834,
                0x1b59_112a_283a_1d74,
                0x3ef8_e06d_ec26_6a95,
                0x95f8_7b59_9214_7603,
                0x1b9f_00f5_5c23_fb31,
                0x125a_2a11_16ca_9ab1,
            ]),
        },
        c2: Fp2 {
            c0: Fp::from_raw_unchecked([
                0x135b_cb98_b183_82e2,
                0x4e11_111d_1582_1d72,
                0x46e1_1ab7_8f10_07fe,
                0x82a1_6e8b_1547_317d,
                0x0ab3_8e13_fd18_bb9b,
                0x1664_dd37_55c9_9cb8,
            ]),
            c1: Fp::from_raw_unchecked([
                0xce65_cb98_b131_8334,
                0xc759_0fdb_7c3a_1d2e,
                0x6fcb_8164_9d1c_8eb3,
                0x0d44_004d_1727_356a,
                0x3746_b738_a7d0_d296,
                0x136c_144a_96b1_34fc,
            ]),
        },
    };

    assert_eq!(a.square(), a * a);
    assert_eq!(b.square(), b * b);
    assert_eq!(c.square(), c * c);

    assert_eq!((a + b) * c.square(), (c * c * a) + (c * c * b));

    assert_eq!(
        a.invert().unwrap() * b.invert().unwrap(),
        (a * b).invert().unwrap()
    );
    assert_eq!(a.invert().unwrap() * a, Fp6::one());
}

#[cfg(feature = "zeroize")]
#[test]
fn test_zeroize() {
    use zeroize::Zeroize;

    let mut a = Fp6::one();
    a.zeroize();
    assert!(bool::from(a.is_zero()));
}
