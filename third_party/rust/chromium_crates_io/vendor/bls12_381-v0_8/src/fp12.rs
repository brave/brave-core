use crate::fp::*;
use crate::fp2::*;
use crate::fp6::*;

use core::fmt;
use core::ops::{Add, AddAssign, Mul, MulAssign, Neg, Sub, SubAssign};
use subtle::{Choice, ConditionallySelectable, ConstantTimeEq, CtOption};

#[cfg(feature = "pairings")]
use rand_core::RngCore;

/// This represents an element $c_0 + c_1 w$ of $\mathbb{F}_{p^12} = \mathbb{F}_{p^6} / w^2 - v$.
pub struct Fp12 {
    pub c0: Fp6,
    pub c1: Fp6,
}

impl From<Fp> for Fp12 {
    fn from(f: Fp) -> Fp12 {
        Fp12 {
            c0: Fp6::from(f),
            c1: Fp6::zero(),
        }
    }
}

impl From<Fp2> for Fp12 {
    fn from(f: Fp2) -> Fp12 {
        Fp12 {
            c0: Fp6::from(f),
            c1: Fp6::zero(),
        }
    }
}

impl From<Fp6> for Fp12 {
    fn from(f: Fp6) -> Fp12 {
        Fp12 {
            c0: f,
            c1: Fp6::zero(),
        }
    }
}

impl PartialEq for Fp12 {
    fn eq(&self, other: &Fp12) -> bool {
        self.ct_eq(other).into()
    }
}

impl Copy for Fp12 {}
impl Clone for Fp12 {
    #[inline]
    fn clone(&self) -> Self {
        *self
    }
}

impl Default for Fp12 {
    fn default() -> Self {
        Fp12::zero()
    }
}

#[cfg(feature = "zeroize")]
impl zeroize::DefaultIsZeroes for Fp12 {}

impl fmt::Debug for Fp12 {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?} + ({:?})*w", self.c0, self.c1)
    }
}

impl ConditionallySelectable for Fp12 {
    #[inline(always)]
    fn conditional_select(a: &Self, b: &Self, choice: Choice) -> Self {
        Fp12 {
            c0: Fp6::conditional_select(&a.c0, &b.c0, choice),
            c1: Fp6::conditional_select(&a.c1, &b.c1, choice),
        }
    }
}

impl ConstantTimeEq for Fp12 {
    #[inline(always)]
    fn ct_eq(&self, other: &Self) -> Choice {
        self.c0.ct_eq(&other.c0) & self.c1.ct_eq(&other.c1)
    }
}

impl Fp12 {
    #[inline]
    pub fn zero() -> Self {
        Fp12 {
            c0: Fp6::zero(),
            c1: Fp6::zero(),
        }
    }

    #[inline]
    pub fn one() -> Self {
        Fp12 {
            c0: Fp6::one(),
            c1: Fp6::zero(),
        }
    }

    #[cfg(feature = "pairings")]
    pub(crate) fn random(mut rng: impl RngCore) -> Self {
        Fp12 {
            c0: Fp6::random(&mut rng),
            c1: Fp6::random(&mut rng),
        }
    }

    pub fn mul_by_014(&self, c0: &Fp2, c1: &Fp2, c4: &Fp2) -> Fp12 {
        let aa = self.c0.mul_by_01(c0, c1);
        let bb = self.c1.mul_by_1(c4);
        let o = c1 + c4;
        let c1 = self.c1 + self.c0;
        let c1 = c1.mul_by_01(c0, &o);
        let c1 = c1 - aa - bb;
        let c0 = bb;
        let c0 = c0.mul_by_nonresidue();
        let c0 = c0 + aa;

        Fp12 { c0, c1 }
    }

    #[inline(always)]
    pub fn is_zero(&self) -> Choice {
        self.c0.is_zero() & self.c1.is_zero()
    }

    #[inline(always)]
    pub fn conjugate(&self) -> Self {
        Fp12 {
            c0: self.c0,
            c1: -self.c1,
        }
    }

    /// Raises this element to p.
    #[inline(always)]
    pub fn frobenius_map(&self) -> Self {
        let c0 = self.c0.frobenius_map();
        let c1 = self.c1.frobenius_map();

        // c1 = c1 * (u + 1)^((p - 1) / 6)
        let c1 = c1
            * Fp6::from(Fp2 {
                c0: Fp::from_raw_unchecked([
                    0x0708_9552_b319_d465,
                    0xc669_5f92_b50a_8313,
                    0x97e8_3ccc_d117_228f,
                    0xa35b_aeca_b2dc_29ee,
                    0x1ce3_93ea_5daa_ce4d,
                    0x08f2_220f_b0fb_66eb,
                ]),
                c1: Fp::from_raw_unchecked([
                    0xb2f6_6aad_4ce5_d646,
                    0x5842_a06b_fc49_7cec,
                    0xcf48_95d4_2599_d394,
                    0xc11b_9cba_40a8_e8d0,
                    0x2e38_13cb_e5a0_de89,
                    0x110e_efda_8884_7faf,
                ]),
            });

        Fp12 { c0, c1 }
    }

    #[inline]
    pub fn square(&self) -> Self {
        let ab = self.c0 * self.c1;
        let c0c1 = self.c0 + self.c1;
        let c0 = self.c1.mul_by_nonresidue();
        let c0 = c0 + self.c0;
        let c0 = c0 * c0c1;
        let c0 = c0 - ab;
        let c1 = ab + ab;
        let c0 = c0 - ab.mul_by_nonresidue();

        Fp12 { c0, c1 }
    }

    pub fn invert(&self) -> CtOption<Self> {
        (self.c0.square() - self.c1.square().mul_by_nonresidue())
            .invert()
            .map(|t| Fp12 {
                c0: self.c0 * t,
                c1: self.c1 * -t,
            })
    }
}

impl<'a, 'b> Mul<&'b Fp12> for &'a Fp12 {
    type Output = Fp12;

    #[inline]
    fn mul(self, other: &'b Fp12) -> Self::Output {
        let aa = self.c0 * other.c0;
        let bb = self.c1 * other.c1;
        let o = other.c0 + other.c1;
        let c1 = self.c1 + self.c0;
        let c1 = c1 * o;
        let c1 = c1 - aa;
        let c1 = c1 - bb;
        let c0 = bb.mul_by_nonresidue();
        let c0 = c0 + aa;

        Fp12 { c0, c1 }
    }
}

impl<'a, 'b> Add<&'b Fp12> for &'a Fp12 {
    type Output = Fp12;

    #[inline]
    fn add(self, rhs: &'b Fp12) -> Self::Output {
        Fp12 {
            c0: self.c0 + rhs.c0,
            c1: self.c1 + rhs.c1,
        }
    }
}

impl<'a> Neg for &'a Fp12 {
    type Output = Fp12;

    #[inline]
    fn neg(self) -> Self::Output {
        Fp12 {
            c0: -self.c0,
            c1: -self.c1,
        }
    }
}

impl Neg for Fp12 {
    type Output = Fp12;

    #[inline]
    fn neg(self) -> Self::Output {
        -&self
    }
}

impl<'a, 'b> Sub<&'b Fp12> for &'a Fp12 {
    type Output = Fp12;

    #[inline]
    fn sub(self, rhs: &'b Fp12) -> Self::Output {
        Fp12 {
            c0: self.c0 - rhs.c0,
            c1: self.c1 - rhs.c1,
        }
    }
}

impl_binops_additive!(Fp12, Fp12);
impl_binops_multiplicative!(Fp12, Fp12);

#[test]
fn test_arithmetic() {
    use crate::fp::*;
    use crate::fp2::*;

    let a = Fp12 {
        c0: Fp6 {
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
        },
        c1: Fp6 {
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
        },
    };

    let b = Fp12 {
        c0: Fp6 {
            c0: Fp2 {
                c0: Fp::from_raw_unchecked([
                    0x47f9_cb98_b1b8_2d58,
                    0x5fe9_11eb_a3aa_1d9d,
                    0x96bf_1b5f_4dd8_1db3,
                    0x8100_d272_c925_9f5b,
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
                    0x0acd_b8e1_58bf_e348,
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
        },
        c1: Fp6 {
            c0: Fp2 {
                c0: Fp::from_raw_unchecked([
                    0x47f9_cb98_b1b8_2d58,
                    0x5fe9_11eb_a3aa_1d9d,
                    0x96bf_1b5f_4dd2_1db3,
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
                    0xcf79_f69f_a117_df3b,
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
        },
    };

    let c = Fp12 {
        c0: Fp6 {
            c0: Fp2 {
                c0: Fp::from_raw_unchecked([
                    0x47f9_cb98_71b8_2d58,
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
                    0x7791_bc55_fece_41d2,
                    0xf84c_5770_4e38_5ec2,
                    0xcb49_c1d9_c010_e60f,
                    0x0acd_b8e1_58bf_e3c8,
                ]),
                c1: Fp::from_raw_unchecked([
                    0x8aef_cb98_b15f_8306,
                    0x3ea1_108f_e4f2_1d54,
                    0xcf79_f69f_a1b7_df3b,
                    0xe4f5_4aa1_d16b_133c,
                    0xba5e_4ef8_6105_a679,
                    0x0ed8_6c07_97be_e5cf,
                ]),
            },
            c2: Fp2 {
                c0: Fp::from_raw_unchecked([
                    0xcee5_cb98_b15c_2db4,
                    0x7159_1082_d23a_1d51,
                    0xd762_40e9_44a1_7ca4,
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
                    0x1099_4b0c_1744_c040,
                ]),
            },
        },
        c1: Fp6 {
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
                    0xcb49_c1d3_c010_e60f,
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
                    0x1099_4b0c_5744_1040,
                ]),
            },
        },
    };

    // because a and b and c are similar to each other and
    // I was lazy, this is just some arbitrary way to make
    // them a little more different
    let a = a.square().invert().unwrap().square() + c;
    let b = b.square().invert().unwrap().square() + a;
    let c = c.square().invert().unwrap().square() + b;

    assert_eq!(a.square(), a * a);
    assert_eq!(b.square(), b * b);
    assert_eq!(c.square(), c * c);

    assert_eq!((a + b) * c.square(), (c * c * a) + (c * c * b));

    assert_eq!(
        a.invert().unwrap() * b.invert().unwrap(),
        (a * b).invert().unwrap()
    );
    assert_eq!(a.invert().unwrap() * a, Fp12::one());

    assert!(a != a.frobenius_map());
    assert_eq!(
        a,
        a.frobenius_map()
            .frobenius_map()
            .frobenius_map()
            .frobenius_map()
            .frobenius_map()
            .frobenius_map()
            .frobenius_map()
            .frobenius_map()
            .frobenius_map()
            .frobenius_map()
            .frobenius_map()
            .frobenius_map()
    );
}

#[cfg(feature = "zeroize")]
#[test]
fn test_zeroize() {
    use zeroize::Zeroize;

    let mut a = Fp12::one();
    a.zeroize();
    assert!(bool::from(a.is_zero()));
}
