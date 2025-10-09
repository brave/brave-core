use super::arch;
use arch::*;

macro_rules! __impl {
    ($name: ident, $feature: tt) => {
        #[derive(Clone, Copy)]
        #[repr(transparent)]
        pub struct $name {
            __private: (),
        }

        impl $name {
            #[inline(always)]
            pub fn new_unchecked() -> Self {
                Self { __private: () }
            }
            #[inline(always)]
            pub fn try_new() -> Option<Self> {
                if feature_detected!($feature) {
                    Some(Self { __private: () })
                } else {
                    None
                }
            }

            #[inline(always)]
            pub fn is_available() -> bool {
                feature_detected!($feature)
            }
        }

        impl ::core::fmt::Debug for $name {
            #[inline]
            fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> core::fmt::Result {
                f.write_str(stringify!($name))
            }
        }
    };
}

__impl!(Neon, "neon");
__impl!(Pmull, "pmull");
__impl!(Fp, "fp");
__impl!(Fp16, "fp16");
__impl!(Sve, "sve");
__impl!(Crc, "crc");
__impl!(Lse, "lse");
__impl!(Lse2, "lse2");
__impl!(Rdm, "rdm");
__impl!(Rcpc, "rcpc");
__impl!(Rcpc2, "rcpc2");
__impl!(Dotprod, "dotprod");
__impl!(Tme, "tme");
__impl!(Fhm, "fhm");
__impl!(Dit, "dit");
__impl!(Flagm, "flagm");
__impl!(Ssbs, "ssbs");
__impl!(Sb, "sb");
__impl!(Paca, "paca");
__impl!(Pacg, "pacg");
__impl!(Dpb, "dpb");
__impl!(Dpb2, "dpb2");
__impl!(Sve2, "sve2");
__impl!(Sve2Aes, "sve2-aes");
__impl!(Sve2Sm4, "sve2-sm4");
__impl!(Sve2Sha3, "sve2-sha3");
__impl!(Sve2Bitperm, "sve2-bitperm");
__impl!(Frintts, "frintts");
__impl!(I8mm, "i8mm");
__impl!(F32mm, "f32mm");
__impl!(F64mm, "f64mm");
__impl!(Bf16, "bf16");
__impl!(Rand, "rand");
__impl!(Bti, "bti");
__impl!(Mte, "mte");
__impl!(Jsconv, "jsconv");
__impl!(Fcma, "fcma");
__impl!(Aes, "aes");
__impl!(Sha2, "sha2");
__impl!(Sha3, "sha3");
__impl!(Sm4, "sm4");
__impl!(Asimd, "asimd");

#[derive(Debug, Clone, Copy)]
#[repr(transparent)]
pub struct Neon_Aes {
    pub neon: Neon,
    pub aes: Aes,
}

#[cfg(feature = "nightly")]
#[cfg_attr(docsrs, doc(cfg(feature = "nightly")))]
#[derive(Debug, Clone, Copy)]
#[repr(transparent)]
pub struct Neon_Sha3 {
    pub neon: Neon,
    pub sha3: Sha3,
}

#[cfg(feature = "nightly")]
#[cfg_attr(docsrs, doc(cfg(feature = "nightly")))]
#[derive(Debug, Clone, Copy)]
#[repr(transparent)]
pub struct Neon_Sm4 {
    pub neon: Neon,
    pub sm4: Sm4,
}

#[cfg(feature = "nightly")]
#[cfg_attr(docsrs, doc(cfg(feature = "nightly")))]
#[derive(Debug, Clone, Copy)]
#[repr(transparent)]
pub struct Neon_Fcma {
    pub neon: Neon,
    pub fcma: Fcma,
}

#[cfg(feature = "nightly")]
#[cfg_attr(docsrs, doc(cfg(feature = "nightly")))]
#[derive(Debug, Clone, Copy)]
#[repr(transparent)]
pub struct Neon_Dotprod {
    pub neon: Neon,
    pub dotprod: Dotprod,
}

#[cfg(feature = "nightly")]
#[cfg_attr(docsrs, doc(cfg(feature = "nightly")))]
#[derive(Debug, Clone, Copy)]
#[repr(transparent)]
pub struct Neon_I8mm {
    pub neon: Neon,
    pub i8mm: I8mm,
}

type p8 = u8;
type p16 = u16;
type p64 = u64;
type p128 = u128;

impl Neon {
    delegate! {
        fn vand_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vandq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vand_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vandq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vand_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vandq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vand_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vandq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vand_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vandq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vand_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vandq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vand_s64(a: int64x1_t, b: int64x1_t) -> int64x1_t;
        fn vandq_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vand_u64(a: uint64x1_t, b: uint64x1_t) -> uint64x1_t;
        fn vandq_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vorr_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vorrq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vorr_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vorrq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vorr_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vorrq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vorr_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vorrq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vorr_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vorrq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vorr_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vorrq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vorr_s64(a: int64x1_t, b: int64x1_t) -> int64x1_t;
        fn vorrq_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vorr_u64(a: uint64x1_t, b: uint64x1_t) -> uint64x1_t;
        fn vorrq_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn veor_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn veorq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn veor_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn veorq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn veor_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn veorq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn veor_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn veorq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn veor_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn veorq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn veor_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn veorq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn veor_s64(a: int64x1_t, b: int64x1_t) -> int64x1_t;
        fn veorq_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn veor_u64(a: uint64x1_t, b: uint64x1_t) -> uint64x1_t;
        fn veorq_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vabd_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vabdq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vabd_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vabdq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vabd_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vabdq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vabd_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vabdq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vabd_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vabdq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vabd_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vabdq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vabd_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vabdq_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vabdl_u8(a: uint8x8_t, b: uint8x8_t) -> uint16x8_t;
        fn vabdl_u16(a: uint16x4_t, b: uint16x4_t) -> uint32x4_t;
        fn vabdl_u32(a: uint32x2_t, b: uint32x2_t) -> uint64x2_t;
        fn vabdl_s8(a: int8x8_t, b: int8x8_t) -> int16x8_t;
        fn vabdl_s16(a: int16x4_t, b: int16x4_t) -> int32x4_t;
        fn vabdl_s32(a: int32x2_t, b: int32x2_t) -> int64x2_t;
        fn vceq_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vceqq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vceq_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vceqq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vceq_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vceqq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vceq_s8(a: int8x8_t, b: int8x8_t) -> uint8x8_t;
        fn vceqq_s8(a: int8x16_t, b: int8x16_t) -> uint8x16_t;
        fn vceq_s16(a: int16x4_t, b: int16x4_t) -> uint16x4_t;
        fn vceqq_s16(a: int16x8_t, b: int16x8_t) -> uint16x8_t;
        fn vceq_s32(a: int32x2_t, b: int32x2_t) -> uint32x2_t;
        fn vceqq_s32(a: int32x4_t, b: int32x4_t) -> uint32x4_t;
        fn vceq_p8(a: poly8x8_t, b: poly8x8_t) -> uint8x8_t;
        fn vceqq_p8(a: poly8x16_t, b: poly8x16_t) -> uint8x16_t;
        fn vceq_f32(a: float32x2_t, b: float32x2_t) -> uint32x2_t;
        fn vceqq_f32(a: float32x4_t, b: float32x4_t) -> uint32x4_t;
        fn vtst_s8(a: int8x8_t, b: int8x8_t) -> uint8x8_t;
        fn vtstq_s8(a: int8x16_t, b: int8x16_t) -> uint8x16_t;
        fn vtst_s16(a: int16x4_t, b: int16x4_t) -> uint16x4_t;
        fn vtstq_s16(a: int16x8_t, b: int16x8_t) -> uint16x8_t;
        fn vtst_s32(a: int32x2_t, b: int32x2_t) -> uint32x2_t;
        fn vtstq_s32(a: int32x4_t, b: int32x4_t) -> uint32x4_t;
        fn vtst_p8(a: poly8x8_t, b: poly8x8_t) -> uint8x8_t;
        fn vtstq_p8(a: poly8x16_t, b: poly8x16_t) -> uint8x16_t;
        fn vtst_p16(a: poly16x4_t, b: poly16x4_t) -> uint16x4_t;
        fn vtstq_p16(a: poly16x8_t, b: poly16x8_t) -> uint16x8_t;
        fn vtst_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vtstq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vtst_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vtstq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vtst_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vtstq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vabs_f32(a: float32x2_t) -> float32x2_t;
        fn vabsq_f32(a: float32x4_t) -> float32x4_t;
        fn vcgt_s8(a: int8x8_t, b: int8x8_t) -> uint8x8_t;
        fn vcgtq_s8(a: int8x16_t, b: int8x16_t) -> uint8x16_t;
        fn vcgt_s16(a: int16x4_t, b: int16x4_t) -> uint16x4_t;
        fn vcgtq_s16(a: int16x8_t, b: int16x8_t) -> uint16x8_t;
        fn vcgt_s32(a: int32x2_t, b: int32x2_t) -> uint32x2_t;
        fn vcgtq_s32(a: int32x4_t, b: int32x4_t) -> uint32x4_t;
        fn vcgt_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vcgtq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vcgt_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vcgtq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vcgt_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vcgtq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vcgt_f32(a: float32x2_t, b: float32x2_t) -> uint32x2_t;
        fn vcgtq_f32(a: float32x4_t, b: float32x4_t) -> uint32x4_t;
        fn vclt_s8(a: int8x8_t, b: int8x8_t) -> uint8x8_t;
        fn vcltq_s8(a: int8x16_t, b: int8x16_t) -> uint8x16_t;
        fn vclt_s16(a: int16x4_t, b: int16x4_t) -> uint16x4_t;
        fn vcltq_s16(a: int16x8_t, b: int16x8_t) -> uint16x8_t;
        fn vclt_s32(a: int32x2_t, b: int32x2_t) -> uint32x2_t;
        fn vcltq_s32(a: int32x4_t, b: int32x4_t) -> uint32x4_t;
        fn vclt_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vcltq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vclt_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vcltq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vclt_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vcltq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vclt_f32(a: float32x2_t, b: float32x2_t) -> uint32x2_t;
        fn vcltq_f32(a: float32x4_t, b: float32x4_t) -> uint32x4_t;
        fn vcle_s8(a: int8x8_t, b: int8x8_t) -> uint8x8_t;
        fn vcleq_s8(a: int8x16_t, b: int8x16_t) -> uint8x16_t;
        fn vcle_s16(a: int16x4_t, b: int16x4_t) -> uint16x4_t;
        fn vcleq_s16(a: int16x8_t, b: int16x8_t) -> uint16x8_t;
        fn vcle_s32(a: int32x2_t, b: int32x2_t) -> uint32x2_t;
        fn vcleq_s32(a: int32x4_t, b: int32x4_t) -> uint32x4_t;
        fn vcle_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vcleq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vcle_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vcleq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vcle_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vcleq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vcle_f32(a: float32x2_t, b: float32x2_t) -> uint32x2_t;
        fn vcleq_f32(a: float32x4_t, b: float32x4_t) -> uint32x4_t;
        fn vcge_s8(a: int8x8_t, b: int8x8_t) -> uint8x8_t;
        fn vcgeq_s8(a: int8x16_t, b: int8x16_t) -> uint8x16_t;
        fn vcge_s16(a: int16x4_t, b: int16x4_t) -> uint16x4_t;
        fn vcgeq_s16(a: int16x8_t, b: int16x8_t) -> uint16x8_t;
        fn vcge_s32(a: int32x2_t, b: int32x2_t) -> uint32x2_t;
        fn vcgeq_s32(a: int32x4_t, b: int32x4_t) -> uint32x4_t;
        fn vcge_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vcgeq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vcge_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vcgeq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vcge_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vcgeq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vcge_f32(a: float32x2_t, b: float32x2_t) -> uint32x2_t;
        fn vcgeq_f32(a: float32x4_t, b: float32x4_t) -> uint32x4_t;
        fn vcls_s8(a: int8x8_t) -> int8x8_t;
        fn vclsq_s8(a: int8x16_t) -> int8x16_t;
        fn vcls_s16(a: int16x4_t) -> int16x4_t;
        fn vclsq_s16(a: int16x8_t) -> int16x8_t;
        fn vcls_s32(a: int32x2_t) -> int32x2_t;
        fn vclsq_s32(a: int32x4_t) -> int32x4_t;
        fn vcls_u8(a: uint8x8_t) -> int8x8_t;
        fn vclsq_u8(a: uint8x16_t) -> int8x16_t;
        fn vcls_u16(a: uint16x4_t) -> int16x4_t;
        fn vclsq_u16(a: uint16x8_t) -> int16x8_t;
        fn vcls_u32(a: uint32x2_t) -> int32x2_t;
        fn vclsq_u32(a: uint32x4_t) -> int32x4_t;
        fn vclz_s8(a: int8x8_t) -> int8x8_t;
        fn vclzq_s8(a: int8x16_t) -> int8x16_t;
        fn vclz_s16(a: int16x4_t) -> int16x4_t;
        fn vclzq_s16(a: int16x8_t) -> int16x8_t;
        fn vclz_s32(a: int32x2_t) -> int32x2_t;
        fn vclzq_s32(a: int32x4_t) -> int32x4_t;
        fn vclz_u8(a: uint8x8_t) -> uint8x8_t;
        fn vclzq_u8(a: uint8x16_t) -> uint8x16_t;
        fn vclz_u16(a: uint16x4_t) -> uint16x4_t;
        fn vclzq_u16(a: uint16x8_t) -> uint16x8_t;
        fn vclz_u32(a: uint32x2_t) -> uint32x2_t;
        fn vclzq_u32(a: uint32x4_t) -> uint32x4_t;
        fn vcagt_f32(a: float32x2_t, b: float32x2_t) -> uint32x2_t;
        fn vcagtq_f32(a: float32x4_t, b: float32x4_t) -> uint32x4_t;
        fn vcage_f32(a: float32x2_t, b: float32x2_t) -> uint32x2_t;
        fn vcageq_f32(a: float32x4_t, b: float32x4_t) -> uint32x4_t;
        fn vcalt_f32(a: float32x2_t, b: float32x2_t) -> uint32x2_t;
        fn vcaltq_f32(a: float32x4_t, b: float32x4_t) -> uint32x4_t;
        fn vcale_f32(a: float32x2_t, b: float32x2_t) -> uint32x2_t;
        fn vcaleq_f32(a: float32x4_t, b: float32x4_t) -> uint32x4_t;
        fn vcreate_s8(a: u64) -> int8x8_t;
        fn vcreate_s16(a: u64) -> int16x4_t;
        fn vcreate_s32(a: u64) -> int32x2_t;
        fn vcreate_s64(a: u64) -> int64x1_t;
        fn vcreate_u8(a: u64) -> uint8x8_t;
        fn vcreate_u16(a: u64) -> uint16x4_t;
        fn vcreate_u32(a: u64) -> uint32x2_t;
        fn vcreate_u64(a: u64) -> uint64x1_t;
        fn vcreate_p8(a: u64) -> poly8x8_t;
        fn vcreate_p16(a: u64) -> poly16x4_t;
        fn vcreate_f32(a: u64) -> float32x2_t;
        fn vcvt_f32_s32(a: int32x2_t) -> float32x2_t;
        fn vcvtq_f32_s32(a: int32x4_t) -> float32x4_t;
        fn vcvt_f32_u32(a: uint32x2_t) -> float32x2_t;
        fn vcvtq_f32_u32(a: uint32x4_t) -> float32x4_t;
        fn vcvt_n_f32_s32<const N: i32>(a: int32x2_t) -> float32x2_t;
        fn vcvtq_n_f32_s32<const N: i32>(a: int32x4_t) -> float32x4_t;
        fn vcvt_n_f32_u32<const N: i32>(a: uint32x2_t) -> float32x2_t;
        fn vcvtq_n_f32_u32<const N: i32>(a: uint32x4_t) -> float32x4_t;
        fn vcvt_n_s32_f32<const N: i32>(a: float32x2_t) -> int32x2_t;
        fn vcvtq_n_s32_f32<const N: i32>(a: float32x4_t) -> int32x4_t;
        fn vcvt_n_u32_f32<const N: i32>(a: float32x2_t) -> uint32x2_t;
        fn vcvtq_n_u32_f32<const N: i32>(a: float32x4_t) -> uint32x4_t;
        fn vcvt_s32_f32(a: float32x2_t) -> int32x2_t;
        fn vcvtq_s32_f32(a: float32x4_t) -> int32x4_t;
        fn vcvt_u32_f32(a: float32x2_t) -> uint32x2_t;
        fn vcvtq_u32_f32(a: float32x4_t) -> uint32x4_t;
        fn vdup_lane_s8<const N: i32>(a: int8x8_t) -> int8x8_t;
        fn vdupq_laneq_s8<const N: i32>(a: int8x16_t) -> int8x16_t;
        fn vdup_lane_s16<const N: i32>(a: int16x4_t) -> int16x4_t;
        fn vdupq_laneq_s16<const N: i32>(a: int16x8_t) -> int16x8_t;
        fn vdup_lane_s32<const N: i32>(a: int32x2_t) -> int32x2_t;
        fn vdupq_laneq_s32<const N: i32>(a: int32x4_t) -> int32x4_t;
        fn vdup_laneq_s8<const N: i32>(a: int8x16_t) -> int8x8_t;
        fn vdup_laneq_s16<const N: i32>(a: int16x8_t) -> int16x4_t;
        fn vdup_laneq_s32<const N: i32>(a: int32x4_t) -> int32x2_t;
        fn vdupq_lane_s8<const N: i32>(a: int8x8_t) -> int8x16_t;
        fn vdupq_lane_s16<const N: i32>(a: int16x4_t) -> int16x8_t;
        fn vdupq_lane_s32<const N: i32>(a: int32x2_t) -> int32x4_t;
        fn vdup_lane_u8<const N: i32>(a: uint8x8_t) -> uint8x8_t;
        fn vdupq_laneq_u8<const N: i32>(a: uint8x16_t) -> uint8x16_t;
        fn vdup_lane_u16<const N: i32>(a: uint16x4_t) -> uint16x4_t;
        fn vdupq_laneq_u16<const N: i32>(a: uint16x8_t) -> uint16x8_t;
        fn vdup_lane_u32<const N: i32>(a: uint32x2_t) -> uint32x2_t;
        fn vdupq_laneq_u32<const N: i32>(a: uint32x4_t) -> uint32x4_t;
        fn vdup_laneq_u8<const N: i32>(a: uint8x16_t) -> uint8x8_t;
        fn vdup_laneq_u16<const N: i32>(a: uint16x8_t) -> uint16x4_t;
        fn vdup_laneq_u32<const N: i32>(a: uint32x4_t) -> uint32x2_t;
        fn vdupq_lane_u8<const N: i32>(a: uint8x8_t) -> uint8x16_t;
        fn vdupq_lane_u16<const N: i32>(a: uint16x4_t) -> uint16x8_t;
        fn vdupq_lane_u32<const N: i32>(a: uint32x2_t) -> uint32x4_t;
        fn vdup_lane_p8<const N: i32>(a: poly8x8_t) -> poly8x8_t;
        fn vdupq_laneq_p8<const N: i32>(a: poly8x16_t) -> poly8x16_t;
        fn vdup_lane_p16<const N: i32>(a: poly16x4_t) -> poly16x4_t;
        fn vdupq_laneq_p16<const N: i32>(a: poly16x8_t) -> poly16x8_t;
        fn vdup_laneq_p8<const N: i32>(a: poly8x16_t) -> poly8x8_t;
        fn vdup_laneq_p16<const N: i32>(a: poly16x8_t) -> poly16x4_t;
        fn vdupq_lane_p8<const N: i32>(a: poly8x8_t) -> poly8x16_t;
        fn vdupq_lane_p16<const N: i32>(a: poly16x4_t) -> poly16x8_t;
        fn vdupq_laneq_s64<const N: i32>(a: int64x2_t) -> int64x2_t;
        fn vdupq_lane_s64<const N: i32>(a: int64x1_t) -> int64x2_t;
        fn vdupq_laneq_u64<const N: i32>(a: uint64x2_t) -> uint64x2_t;
        fn vdupq_lane_u64<const N: i32>(a: uint64x1_t) -> uint64x2_t;
        fn vdup_lane_f32<const N: i32>(a: float32x2_t) -> float32x2_t;
        fn vdupq_laneq_f32<const N: i32>(a: float32x4_t) -> float32x4_t;
        fn vdup_laneq_f32<const N: i32>(a: float32x4_t) -> float32x2_t;
        fn vdupq_lane_f32<const N: i32>(a: float32x2_t) -> float32x4_t;
        fn vdup_lane_s64<const N: i32>(a: int64x1_t) -> int64x1_t;
        fn vdup_lane_u64<const N: i32>(a: uint64x1_t) -> uint64x1_t;
        fn vdup_laneq_s64<const N: i32>(a: int64x2_t) -> int64x1_t;
        fn vdup_laneq_u64<const N: i32>(a: uint64x2_t) -> uint64x1_t;
        fn vext_s8<const N: i32>(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vextq_s8<const N: i32>(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vext_s16<const N: i32>(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vextq_s16<const N: i32>(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vext_s32<const N: i32>(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vextq_s32<const N: i32>(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vext_u8<const N: i32>(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vextq_u8<const N: i32>(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vext_u16<const N: i32>(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vextq_u16<const N: i32>(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vext_u32<const N: i32>(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vextq_u32<const N: i32>(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vext_p8<const N: i32>(a: poly8x8_t, b: poly8x8_t) -> poly8x8_t;
        fn vextq_p8<const N: i32>(a: poly8x16_t, b: poly8x16_t) -> poly8x16_t;
        fn vext_p16<const N: i32>(a: poly16x4_t, b: poly16x4_t) -> poly16x4_t;
        fn vextq_p16<const N: i32>(a: poly16x8_t, b: poly16x8_t) -> poly16x8_t;
        fn vextq_s64<const N: i32>(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vextq_u64<const N: i32>(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vext_f32<const N: i32>(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vextq_f32<const N: i32>(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vmla_s8(a: int8x8_t, b: int8x8_t, c: int8x8_t) -> int8x8_t;
        fn vmlaq_s8(a: int8x16_t, b: int8x16_t, c: int8x16_t) -> int8x16_t;
        fn vmla_s16(a: int16x4_t, b: int16x4_t, c: int16x4_t) -> int16x4_t;
        fn vmlaq_s16(a: int16x8_t, b: int16x8_t, c: int16x8_t) -> int16x8_t;
        fn vmla_s32(a: int32x2_t, b: int32x2_t, c: int32x2_t) -> int32x2_t;
        fn vmlaq_s32(a: int32x4_t, b: int32x4_t, c: int32x4_t) -> int32x4_t;
        fn vmla_u8(a: uint8x8_t, b: uint8x8_t, c: uint8x8_t) -> uint8x8_t;
        fn vmlaq_u8(a: uint8x16_t, b: uint8x16_t, c: uint8x16_t) -> uint8x16_t;
        fn vmla_u16(a: uint16x4_t, b: uint16x4_t, c: uint16x4_t) -> uint16x4_t;
        fn vmlaq_u16(a: uint16x8_t, b: uint16x8_t, c: uint16x8_t) -> uint16x8_t;
        fn vmla_u32(a: uint32x2_t, b: uint32x2_t, c: uint32x2_t) -> uint32x2_t;
        fn vmlaq_u32(a: uint32x4_t, b: uint32x4_t, c: uint32x4_t) -> uint32x4_t;
        fn vmla_f32(a: float32x2_t, b: float32x2_t, c: float32x2_t) -> float32x2_t;
        fn vmlaq_f32(a: float32x4_t, b: float32x4_t, c: float32x4_t) -> float32x4_t;
        fn vmla_n_s16(a: int16x4_t, b: int16x4_t, c: i16) -> int16x4_t;
        fn vmlaq_n_s16(a: int16x8_t, b: int16x8_t, c: i16) -> int16x8_t;
        fn vmla_n_s32(a: int32x2_t, b: int32x2_t, c: i32) -> int32x2_t;
        fn vmlaq_n_s32(a: int32x4_t, b: int32x4_t, c: i32) -> int32x4_t;
        fn vmla_n_u16(a: uint16x4_t, b: uint16x4_t, c: u16) -> uint16x4_t;
        fn vmlaq_n_u16(a: uint16x8_t, b: uint16x8_t, c: u16) -> uint16x8_t;
        fn vmla_n_u32(a: uint32x2_t, b: uint32x2_t, c: u32) -> uint32x2_t;
        fn vmlaq_n_u32(a: uint32x4_t, b: uint32x4_t, c: u32) -> uint32x4_t;
        fn vmla_n_f32(a: float32x2_t, b: float32x2_t, c: f32) -> float32x2_t;
        fn vmlaq_n_f32(a: float32x4_t, b: float32x4_t, c: f32) -> float32x4_t;
        fn vmla_lane_s16<const LANE: i32>(
            a: int16x4_t,
            b: int16x4_t,
            c: int16x4_t,
        ) -> int16x4_t;
        fn vmla_laneq_s16<const LANE: i32>(
            a: int16x4_t,
            b: int16x4_t,
            c: int16x8_t,
        ) -> int16x4_t;
        fn vmlaq_lane_s16<const LANE: i32>(
            a: int16x8_t,
            b: int16x8_t,
            c: int16x4_t,
        ) -> int16x8_t;
        fn vmlaq_laneq_s16<const LANE: i32>(
            a: int16x8_t,
            b: int16x8_t,
            c: int16x8_t,
        ) -> int16x8_t;
        fn vmla_lane_s32<const LANE: i32>(
            a: int32x2_t,
            b: int32x2_t,
            c: int32x2_t,
        ) -> int32x2_t;
        fn vmla_laneq_s32<const LANE: i32>(
            a: int32x2_t,
            b: int32x2_t,
            c: int32x4_t,
        ) -> int32x2_t;
        fn vmlaq_lane_s32<const LANE: i32>(
            a: int32x4_t,
            b: int32x4_t,
            c: int32x2_t,
        ) -> int32x4_t;
        fn vmlaq_laneq_s32<const LANE: i32>(
            a: int32x4_t,
            b: int32x4_t,
            c: int32x4_t,
        ) -> int32x4_t;
        fn vmla_lane_u16<const LANE: i32>(
            a: uint16x4_t,
            b: uint16x4_t,
            c: uint16x4_t,
        ) -> uint16x4_t;
        fn vmla_laneq_u16<const LANE: i32>(
            a: uint16x4_t,
            b: uint16x4_t,
            c: uint16x8_t,
        ) -> uint16x4_t;
        fn vmlaq_lane_u16<const LANE: i32>(
            a: uint16x8_t,
            b: uint16x8_t,
            c: uint16x4_t,
        ) -> uint16x8_t;
        fn vmlaq_laneq_u16<const LANE: i32>(
            a: uint16x8_t,
            b: uint16x8_t,
            c: uint16x8_t,
        ) -> uint16x8_t;
        fn vmla_lane_u32<const LANE: i32>(
            a: uint32x2_t,
            b: uint32x2_t,
            c: uint32x2_t,
        ) -> uint32x2_t;
        fn vmla_laneq_u32<const LANE: i32>(
            a: uint32x2_t,
            b: uint32x2_t,
            c: uint32x4_t,
        ) -> uint32x2_t;
        fn vmlaq_lane_u32<const LANE: i32>(
            a: uint32x4_t,
            b: uint32x4_t,
            c: uint32x2_t,
        ) -> uint32x4_t;
        fn vmlaq_laneq_u32<const LANE: i32>(
            a: uint32x4_t,
            b: uint32x4_t,
            c: uint32x4_t,
        ) -> uint32x4_t;
        fn vmla_lane_f32<const LANE: i32>(
            a: float32x2_t,
            b: float32x2_t,
            c: float32x2_t,
        ) -> float32x2_t;
        fn vmla_laneq_f32<const LANE: i32>(
            a: float32x2_t,
            b: float32x2_t,
            c: float32x4_t,
        ) -> float32x2_t;
        fn vmlaq_lane_f32<const LANE: i32>(
            a: float32x4_t,
            b: float32x4_t,
            c: float32x2_t,
        ) -> float32x4_t;
        fn vmlaq_laneq_f32<const LANE: i32>(
            a: float32x4_t,
            b: float32x4_t,
            c: float32x4_t,
        ) -> float32x4_t;
        fn vmlal_s8(a: int16x8_t, b: int8x8_t, c: int8x8_t) -> int16x8_t;
        fn vmlal_s16(a: int32x4_t, b: int16x4_t, c: int16x4_t) -> int32x4_t;
        fn vmlal_s32(a: int64x2_t, b: int32x2_t, c: int32x2_t) -> int64x2_t;
        fn vmlal_u8(a: uint16x8_t, b: uint8x8_t, c: uint8x8_t) -> uint16x8_t;
        fn vmlal_u16(a: uint32x4_t, b: uint16x4_t, c: uint16x4_t) -> uint32x4_t;
        fn vmlal_u32(a: uint64x2_t, b: uint32x2_t, c: uint32x2_t) -> uint64x2_t;
        fn vmlal_n_s16(a: int32x4_t, b: int16x4_t, c: i16) -> int32x4_t;
        fn vmlal_n_s32(a: int64x2_t, b: int32x2_t, c: i32) -> int64x2_t;
        fn vmlal_n_u16(a: uint32x4_t, b: uint16x4_t, c: u16) -> uint32x4_t;
        fn vmlal_n_u32(a: uint64x2_t, b: uint32x2_t, c: u32) -> uint64x2_t;
        fn vmlal_lane_s16<const LANE: i32>(
            a: int32x4_t,
            b: int16x4_t,
            c: int16x4_t,
        ) -> int32x4_t;
        fn vmlal_laneq_s16<const LANE: i32>(
            a: int32x4_t,
            b: int16x4_t,
            c: int16x8_t,
        ) -> int32x4_t;
        fn vmlal_lane_s32<const LANE: i32>(
            a: int64x2_t,
            b: int32x2_t,
            c: int32x2_t,
        ) -> int64x2_t;
        fn vmlal_laneq_s32<const LANE: i32>(
            a: int64x2_t,
            b: int32x2_t,
            c: int32x4_t,
        ) -> int64x2_t;
        fn vmlal_lane_u16<const LANE: i32>(
            a: uint32x4_t,
            b: uint16x4_t,
            c: uint16x4_t,
        ) -> uint32x4_t;
        fn vmlal_laneq_u16<const LANE: i32>(
            a: uint32x4_t,
            b: uint16x4_t,
            c: uint16x8_t,
        ) -> uint32x4_t;
        fn vmlal_lane_u32<const LANE: i32>(
            a: uint64x2_t,
            b: uint32x2_t,
            c: uint32x2_t,
        ) -> uint64x2_t;
        fn vmlal_laneq_u32<const LANE: i32>(
            a: uint64x2_t,
            b: uint32x2_t,
            c: uint32x4_t,
        ) -> uint64x2_t;
        fn vmls_s8(a: int8x8_t, b: int8x8_t, c: int8x8_t) -> int8x8_t;
        fn vmlsq_s8(a: int8x16_t, b: int8x16_t, c: int8x16_t) -> int8x16_t;
        fn vmls_s16(a: int16x4_t, b: int16x4_t, c: int16x4_t) -> int16x4_t;
        fn vmlsq_s16(a: int16x8_t, b: int16x8_t, c: int16x8_t) -> int16x8_t;
        fn vmls_s32(a: int32x2_t, b: int32x2_t, c: int32x2_t) -> int32x2_t;
        fn vmlsq_s32(a: int32x4_t, b: int32x4_t, c: int32x4_t) -> int32x4_t;
        fn vmls_u8(a: uint8x8_t, b: uint8x8_t, c: uint8x8_t) -> uint8x8_t;
        fn vmlsq_u8(a: uint8x16_t, b: uint8x16_t, c: uint8x16_t) -> uint8x16_t;
        fn vmls_u16(a: uint16x4_t, b: uint16x4_t, c: uint16x4_t) -> uint16x4_t;
        fn vmlsq_u16(a: uint16x8_t, b: uint16x8_t, c: uint16x8_t) -> uint16x8_t;
        fn vmls_u32(a: uint32x2_t, b: uint32x2_t, c: uint32x2_t) -> uint32x2_t;
        fn vmlsq_u32(a: uint32x4_t, b: uint32x4_t, c: uint32x4_t) -> uint32x4_t;
        fn vmls_f32(a: float32x2_t, b: float32x2_t, c: float32x2_t) -> float32x2_t;
        fn vmlsq_f32(a: float32x4_t, b: float32x4_t, c: float32x4_t) -> float32x4_t;
        fn vmls_n_s16(a: int16x4_t, b: int16x4_t, c: i16) -> int16x4_t;
        fn vmlsq_n_s16(a: int16x8_t, b: int16x8_t, c: i16) -> int16x8_t;
        fn vmls_n_s32(a: int32x2_t, b: int32x2_t, c: i32) -> int32x2_t;
        fn vmlsq_n_s32(a: int32x4_t, b: int32x4_t, c: i32) -> int32x4_t;
        fn vmls_n_u16(a: uint16x4_t, b: uint16x4_t, c: u16) -> uint16x4_t;
        fn vmlsq_n_u16(a: uint16x8_t, b: uint16x8_t, c: u16) -> uint16x8_t;
        fn vmls_n_u32(a: uint32x2_t, b: uint32x2_t, c: u32) -> uint32x2_t;
        fn vmlsq_n_u32(a: uint32x4_t, b: uint32x4_t, c: u32) -> uint32x4_t;
        fn vmls_n_f32(a: float32x2_t, b: float32x2_t, c: f32) -> float32x2_t;
        fn vmlsq_n_f32(a: float32x4_t, b: float32x4_t, c: f32) -> float32x4_t;
        fn vmls_lane_s16<const LANE: i32>(
            a: int16x4_t,
            b: int16x4_t,
            c: int16x4_t,
        ) -> int16x4_t;
        fn vmls_laneq_s16<const LANE: i32>(
            a: int16x4_t,
            b: int16x4_t,
            c: int16x8_t,
        ) -> int16x4_t;
        fn vmlsq_lane_s16<const LANE: i32>(
            a: int16x8_t,
            b: int16x8_t,
            c: int16x4_t,
        ) -> int16x8_t;
        fn vmlsq_laneq_s16<const LANE: i32>(
            a: int16x8_t,
            b: int16x8_t,
            c: int16x8_t,
        ) -> int16x8_t;
        fn vmls_lane_s32<const LANE: i32>(
            a: int32x2_t,
            b: int32x2_t,
            c: int32x2_t,
        ) -> int32x2_t;
        fn vmls_laneq_s32<const LANE: i32>(
            a: int32x2_t,
            b: int32x2_t,
            c: int32x4_t,
        ) -> int32x2_t;
        fn vmlsq_lane_s32<const LANE: i32>(
            a: int32x4_t,
            b: int32x4_t,
            c: int32x2_t,
        ) -> int32x4_t;
        fn vmlsq_laneq_s32<const LANE: i32>(
            a: int32x4_t,
            b: int32x4_t,
            c: int32x4_t,
        ) -> int32x4_t;
        fn vmls_lane_u16<const LANE: i32>(
            a: uint16x4_t,
            b: uint16x4_t,
            c: uint16x4_t,
        ) -> uint16x4_t;
        fn vmls_laneq_u16<const LANE: i32>(
            a: uint16x4_t,
            b: uint16x4_t,
            c: uint16x8_t,
        ) -> uint16x4_t;
        fn vmlsq_lane_u16<const LANE: i32>(
            a: uint16x8_t,
            b: uint16x8_t,
            c: uint16x4_t,
        ) -> uint16x8_t;
        fn vmlsq_laneq_u16<const LANE: i32>(
            a: uint16x8_t,
            b: uint16x8_t,
            c: uint16x8_t,
        ) -> uint16x8_t;
        fn vmls_lane_u32<const LANE: i32>(
            a: uint32x2_t,
            b: uint32x2_t,
            c: uint32x2_t,
        ) -> uint32x2_t;
        fn vmls_laneq_u32<const LANE: i32>(
            a: uint32x2_t,
            b: uint32x2_t,
            c: uint32x4_t,
        ) -> uint32x2_t;
        fn vmlsq_lane_u32<const LANE: i32>(
            a: uint32x4_t,
            b: uint32x4_t,
            c: uint32x2_t,
        ) -> uint32x4_t;
        fn vmlsq_laneq_u32<const LANE: i32>(
            a: uint32x4_t,
            b: uint32x4_t,
            c: uint32x4_t,
        ) -> uint32x4_t;
        fn vmls_lane_f32<const LANE: i32>(
            a: float32x2_t,
            b: float32x2_t,
            c: float32x2_t,
        ) -> float32x2_t;
        fn vmls_laneq_f32<const LANE: i32>(
            a: float32x2_t,
            b: float32x2_t,
            c: float32x4_t,
        ) -> float32x2_t;
        fn vmlsq_lane_f32<const LANE: i32>(
            a: float32x4_t,
            b: float32x4_t,
            c: float32x2_t,
        ) -> float32x4_t;
        fn vmlsq_laneq_f32<const LANE: i32>(
            a: float32x4_t,
            b: float32x4_t,
            c: float32x4_t,
        ) -> float32x4_t;
        fn vmlsl_s8(a: int16x8_t, b: int8x8_t, c: int8x8_t) -> int16x8_t;
        fn vmlsl_s16(a: int32x4_t, b: int16x4_t, c: int16x4_t) -> int32x4_t;
        fn vmlsl_s32(a: int64x2_t, b: int32x2_t, c: int32x2_t) -> int64x2_t;
        fn vmlsl_u8(a: uint16x8_t, b: uint8x8_t, c: uint8x8_t) -> uint16x8_t;
        fn vmlsl_u16(a: uint32x4_t, b: uint16x4_t, c: uint16x4_t) -> uint32x4_t;
        fn vmlsl_u32(a: uint64x2_t, b: uint32x2_t, c: uint32x2_t) -> uint64x2_t;
        fn vmlsl_n_s16(a: int32x4_t, b: int16x4_t, c: i16) -> int32x4_t;
        fn vmlsl_n_s32(a: int64x2_t, b: int32x2_t, c: i32) -> int64x2_t;
        fn vmlsl_n_u16(a: uint32x4_t, b: uint16x4_t, c: u16) -> uint32x4_t;
        fn vmlsl_n_u32(a: uint64x2_t, b: uint32x2_t, c: u32) -> uint64x2_t;
        fn vmlsl_lane_s16<const LANE: i32>(
            a: int32x4_t,
            b: int16x4_t,
            c: int16x4_t,
        ) -> int32x4_t;
        fn vmlsl_laneq_s16<const LANE: i32>(
            a: int32x4_t,
            b: int16x4_t,
            c: int16x8_t,
        ) -> int32x4_t;
        fn vmlsl_lane_s32<const LANE: i32>(
            a: int64x2_t,
            b: int32x2_t,
            c: int32x2_t,
        ) -> int64x2_t;
        fn vmlsl_laneq_s32<const LANE: i32>(
            a: int64x2_t,
            b: int32x2_t,
            c: int32x4_t,
        ) -> int64x2_t;
        fn vmlsl_lane_u16<const LANE: i32>(
            a: uint32x4_t,
            b: uint16x4_t,
            c: uint16x4_t,
        ) -> uint32x4_t;
        fn vmlsl_laneq_u16<const LANE: i32>(
            a: uint32x4_t,
            b: uint16x4_t,
            c: uint16x8_t,
        ) -> uint32x4_t;
        fn vmlsl_lane_u32<const LANE: i32>(
            a: uint64x2_t,
            b: uint32x2_t,
            c: uint32x2_t,
        ) -> uint64x2_t;
        fn vmlsl_laneq_u32<const LANE: i32>(
            a: uint64x2_t,
            b: uint32x2_t,
            c: uint32x4_t,
        ) -> uint64x2_t;
        fn vneg_s8(a: int8x8_t) -> int8x8_t;
        fn vnegq_s8(a: int8x16_t) -> int8x16_t;
        fn vneg_s16(a: int16x4_t) -> int16x4_t;
        fn vnegq_s16(a: int16x8_t) -> int16x8_t;
        fn vneg_s32(a: int32x2_t) -> int32x2_t;
        fn vnegq_s32(a: int32x4_t) -> int32x4_t;
        fn vneg_f32(a: float32x2_t) -> float32x2_t;
        fn vnegq_f32(a: float32x4_t) -> float32x4_t;
        fn vqneg_s8(a: int8x8_t) -> int8x8_t;
        fn vqnegq_s8(a: int8x16_t) -> int8x16_t;
        fn vqneg_s16(a: int16x4_t) -> int16x4_t;
        fn vqnegq_s16(a: int16x8_t) -> int16x8_t;
        fn vqneg_s32(a: int32x2_t) -> int32x2_t;
        fn vqnegq_s32(a: int32x4_t) -> int32x4_t;
        fn vqsub_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vqsubq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vqsub_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vqsubq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vqsub_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vqsubq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vqsub_u64(a: uint64x1_t, b: uint64x1_t) -> uint64x1_t;
        fn vqsubq_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vqsub_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vqsubq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vqsub_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vqsubq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vqsub_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vqsubq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vqsub_s64(a: int64x1_t, b: int64x1_t) -> int64x1_t;
        fn vqsubq_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vhadd_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vhaddq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vhadd_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vhaddq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vhadd_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vhaddq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vhadd_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vhaddq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vhadd_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vhaddq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vhadd_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vhaddq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vrhadd_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vrhaddq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vrhadd_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vrhaddq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vrhadd_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vrhaddq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vrhadd_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vrhaddq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vrhadd_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vrhaddq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vrhadd_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vrhaddq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vrndn_f32(a: float32x2_t) -> float32x2_t;
        fn vrndnq_f32(a: float32x4_t) -> float32x4_t;
        fn vqadd_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vqaddq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vqadd_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vqaddq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vqadd_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vqaddq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vqadd_u64(a: uint64x1_t, b: uint64x1_t) -> uint64x1_t;
        fn vqaddq_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vqadd_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vqaddq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vqadd_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vqaddq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vqadd_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vqaddq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vqadd_s64(a: int64x1_t, b: int64x1_t) -> int64x1_t;
        fn vqaddq_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        unsafe fn vld1_s8_x2(a: *const i8) -> int8x8x2_t;
        unsafe fn vld1_s16_x2(a: *const i16) -> int16x4x2_t;
        unsafe fn vld1_s32_x2(a: *const i32) -> int32x2x2_t;
        unsafe fn vld1_s64_x2(a: *const i64) -> int64x1x2_t;
        unsafe fn vld1q_s8_x2(a: *const i8) -> int8x16x2_t;
        unsafe fn vld1q_s16_x2(a: *const i16) -> int16x8x2_t;
        unsafe fn vld1q_s32_x2(a: *const i32) -> int32x4x2_t;
        unsafe fn vld1q_s64_x2(a: *const i64) -> int64x2x2_t;
        unsafe fn vld1_s8_x3(a: *const i8) -> int8x8x3_t;
        unsafe fn vld1_s16_x3(a: *const i16) -> int16x4x3_t;
        unsafe fn vld1_s32_x3(a: *const i32) -> int32x2x3_t;
        unsafe fn vld1_s64_x3(a: *const i64) -> int64x1x3_t;
        unsafe fn vld1q_s8_x3(a: *const i8) -> int8x16x3_t;
        unsafe fn vld1q_s16_x3(a: *const i16) -> int16x8x3_t;
        unsafe fn vld1q_s32_x3(a: *const i32) -> int32x4x3_t;
        unsafe fn vld1q_s64_x3(a: *const i64) -> int64x2x3_t;
        unsafe fn vld1_s8_x4(a: *const i8) -> int8x8x4_t;
        unsafe fn vld1_s16_x4(a: *const i16) -> int16x4x4_t;
        unsafe fn vld1_s32_x4(a: *const i32) -> int32x2x4_t;
        unsafe fn vld1_s64_x4(a: *const i64) -> int64x1x4_t;
        unsafe fn vld1q_s8_x4(a: *const i8) -> int8x16x4_t;
        unsafe fn vld1q_s16_x4(a: *const i16) -> int16x8x4_t;
        unsafe fn vld1q_s32_x4(a: *const i32) -> int32x4x4_t;
        unsafe fn vld1q_s64_x4(a: *const i64) -> int64x2x4_t;
        unsafe fn vld1_u8_x2(a: *const u8) -> uint8x8x2_t;
        unsafe fn vld1_u16_x2(a: *const u16) -> uint16x4x2_t;
        unsafe fn vld1_u32_x2(a: *const u32) -> uint32x2x2_t;
        unsafe fn vld1_u64_x2(a: *const u64) -> uint64x1x2_t;
        unsafe fn vld1q_u8_x2(a: *const u8) -> uint8x16x2_t;
        unsafe fn vld1q_u16_x2(a: *const u16) -> uint16x8x2_t;
        unsafe fn vld1q_u32_x2(a: *const u32) -> uint32x4x2_t;
        unsafe fn vld1q_u64_x2(a: *const u64) -> uint64x2x2_t;
        unsafe fn vld1_u8_x3(a: *const u8) -> uint8x8x3_t;
        unsafe fn vld1_u16_x3(a: *const u16) -> uint16x4x3_t;
        unsafe fn vld1_u32_x3(a: *const u32) -> uint32x2x3_t;
        unsafe fn vld1_u64_x3(a: *const u64) -> uint64x1x3_t;
        unsafe fn vld1q_u8_x3(a: *const u8) -> uint8x16x3_t;
        unsafe fn vld1q_u16_x3(a: *const u16) -> uint16x8x3_t;
        unsafe fn vld1q_u32_x3(a: *const u32) -> uint32x4x3_t;
        unsafe fn vld1q_u64_x3(a: *const u64) -> uint64x2x3_t;
        unsafe fn vld1_u8_x4(a: *const u8) -> uint8x8x4_t;
        unsafe fn vld1_u16_x4(a: *const u16) -> uint16x4x4_t;
        unsafe fn vld1_u32_x4(a: *const u32) -> uint32x2x4_t;
        unsafe fn vld1_u64_x4(a: *const u64) -> uint64x1x4_t;
        unsafe fn vld1q_u8_x4(a: *const u8) -> uint8x16x4_t;
        unsafe fn vld1q_u16_x4(a: *const u16) -> uint16x8x4_t;
        unsafe fn vld1q_u32_x4(a: *const u32) -> uint32x4x4_t;
        unsafe fn vld1q_u64_x4(a: *const u64) -> uint64x2x4_t;
        unsafe fn vld1_p8_x2(a: *const p8) -> poly8x8x2_t;
        unsafe fn vld1_p8_x3(a: *const p8) -> poly8x8x3_t;
        unsafe fn vld1_p8_x4(a: *const p8) -> poly8x8x4_t;
        unsafe fn vld1q_p8_x2(a: *const p8) -> poly8x16x2_t;
        unsafe fn vld1q_p8_x3(a: *const p8) -> poly8x16x3_t;
        unsafe fn vld1q_p8_x4(a: *const p8) -> poly8x16x4_t;
        unsafe fn vld1_p16_x2(a: *const p16) -> poly16x4x2_t;
        unsafe fn vld1_p16_x3(a: *const p16) -> poly16x4x3_t;
        unsafe fn vld1_p16_x4(a: *const p16) -> poly16x4x4_t;
        unsafe fn vld1q_p16_x2(a: *const p16) -> poly16x8x2_t;
        unsafe fn vld1q_p16_x3(a: *const p16) -> poly16x8x3_t;
        unsafe fn vld1q_p16_x4(a: *const p16) -> poly16x8x4_t;
        unsafe fn vld1_f32_x2(a: *const f32) -> float32x2x2_t;
        unsafe fn vld1q_f32_x2(a: *const f32) -> float32x4x2_t;
        unsafe fn vld1_f32_x3(a: *const f32) -> float32x2x3_t;
        unsafe fn vld1q_f32_x3(a: *const f32) -> float32x4x3_t;
        unsafe fn vld1_f32_x4(a: *const f32) -> float32x2x4_t;
        unsafe fn vld1q_f32_x4(a: *const f32) -> float32x4x4_t;
        unsafe fn vld2_s8(a: *const i8) -> int8x8x2_t;
        unsafe fn vld2_s16(a: *const i16) -> int16x4x2_t;
        unsafe fn vld2_s32(a: *const i32) -> int32x2x2_t;
        unsafe fn vld2q_s8(a: *const i8) -> int8x16x2_t;
        unsafe fn vld2q_s16(a: *const i16) -> int16x8x2_t;
        unsafe fn vld2q_s32(a: *const i32) -> int32x4x2_t;
        unsafe fn vld2_s64(a: *const i64) -> int64x1x2_t;
        unsafe fn vld2_u8(a: *const u8) -> uint8x8x2_t;
        unsafe fn vld2_u16(a: *const u16) -> uint16x4x2_t;
        unsafe fn vld2_u32(a: *const u32) -> uint32x2x2_t;
        unsafe fn vld2q_u8(a: *const u8) -> uint8x16x2_t;
        unsafe fn vld2q_u16(a: *const u16) -> uint16x8x2_t;
        unsafe fn vld2q_u32(a: *const u32) -> uint32x4x2_t;
        unsafe fn vld2_p8(a: *const p8) -> poly8x8x2_t;
        unsafe fn vld2_p16(a: *const p16) -> poly16x4x2_t;
        unsafe fn vld2q_p8(a: *const p8) -> poly8x16x2_t;
        unsafe fn vld2q_p16(a: *const p16) -> poly16x8x2_t;
        unsafe fn vld2_u64(a: *const u64) -> uint64x1x2_t;
        unsafe fn vld2_f32(a: *const f32) -> float32x2x2_t;
        unsafe fn vld2q_f32(a: *const f32) -> float32x4x2_t;
        unsafe fn vld2_dup_s8(a: *const i8) -> int8x8x2_t;
        unsafe fn vld2_dup_s16(a: *const i16) -> int16x4x2_t;
        unsafe fn vld2_dup_s32(a: *const i32) -> int32x2x2_t;
        unsafe fn vld2q_dup_s8(a: *const i8) -> int8x16x2_t;
        unsafe fn vld2q_dup_s16(a: *const i16) -> int16x8x2_t;
        unsafe fn vld2q_dup_s32(a: *const i32) -> int32x4x2_t;
        unsafe fn vld2_dup_s64(a: *const i64) -> int64x1x2_t;
        unsafe fn vld2_dup_u8(a: *const u8) -> uint8x8x2_t;
        unsafe fn vld2_dup_u16(a: *const u16) -> uint16x4x2_t;
        unsafe fn vld2_dup_u32(a: *const u32) -> uint32x2x2_t;
        unsafe fn vld2q_dup_u8(a: *const u8) -> uint8x16x2_t;
        unsafe fn vld2q_dup_u16(a: *const u16) -> uint16x8x2_t;
        unsafe fn vld2q_dup_u32(a: *const u32) -> uint32x4x2_t;
        unsafe fn vld2_dup_p8(a: *const p8) -> poly8x8x2_t;
        unsafe fn vld2_dup_p16(a: *const p16) -> poly16x4x2_t;
        unsafe fn vld2q_dup_p8(a: *const p8) -> poly8x16x2_t;
        unsafe fn vld2q_dup_p16(a: *const p16) -> poly16x8x2_t;
        unsafe fn vld2_dup_u64(a: *const u64) -> uint64x1x2_t;
        unsafe fn vld2_dup_f32(a: *const f32) -> float32x2x2_t;
        unsafe fn vld2q_dup_f32(a: *const f32) -> float32x4x2_t;
        unsafe fn vld2_lane_s8<const LANE: i32>(a: *const i8, b: int8x8x2_t) -> int8x8x2_t;
        unsafe fn vld2_lane_s16<const LANE: i32>(a: *const i16, b: int16x4x2_t) -> int16x4x2_t;
        unsafe fn vld2_lane_s32<const LANE: i32>(a: *const i32, b: int32x2x2_t) -> int32x2x2_t;
        unsafe fn vld2q_lane_s16<const LANE: i32>(a: *const i16, b: int16x8x2_t) -> int16x8x2_t;
        unsafe fn vld2q_lane_s32<const LANE: i32>(a: *const i32, b: int32x4x2_t) -> int32x4x2_t;
        unsafe fn vld2_lane_u8<const LANE: i32>(a: *const u8, b: uint8x8x2_t) -> uint8x8x2_t;
        unsafe fn vld2_lane_u16<const LANE: i32>(a: *const u16, b: uint16x4x2_t) -> uint16x4x2_t;
        unsafe fn vld2_lane_u32<const LANE: i32>(a: *const u32, b: uint32x2x2_t) -> uint32x2x2_t;
        unsafe fn vld2q_lane_u16<const LANE: i32>(a: *const u16, b: uint16x8x2_t) -> uint16x8x2_t;
        unsafe fn vld2q_lane_u32<const LANE: i32>(a: *const u32, b: uint32x4x2_t) -> uint32x4x2_t;
        unsafe fn vld2_lane_p8<const LANE: i32>(a: *const p8, b: poly8x8x2_t) -> poly8x8x2_t;
        unsafe fn vld2_lane_p16<const LANE: i32>(a: *const p16, b: poly16x4x2_t) -> poly16x4x2_t;
        unsafe fn vld2q_lane_p16<const LANE: i32>(a: *const p16, b: poly16x8x2_t) -> poly16x8x2_t;
        unsafe fn vld2_lane_f32<const LANE: i32>(a: *const f32, b: float32x2x2_t) -> float32x2x2_t;
        unsafe fn vld2q_lane_f32<const LANE: i32>(
            a: *const f32,
            b: float32x4x2_t,
        ) -> float32x4x2_t;
        unsafe fn vld3_s8(a: *const i8) -> int8x8x3_t;
        unsafe fn vld3_s16(a: *const i16) -> int16x4x3_t;
        unsafe fn vld3_s32(a: *const i32) -> int32x2x3_t;
        unsafe fn vld3q_s8(a: *const i8) -> int8x16x3_t;
        unsafe fn vld3q_s16(a: *const i16) -> int16x8x3_t;
        unsafe fn vld3q_s32(a: *const i32) -> int32x4x3_t;
        unsafe fn vld3_s64(a: *const i64) -> int64x1x3_t;
        unsafe fn vld3_u8(a: *const u8) -> uint8x8x3_t;
        unsafe fn vld3_u16(a: *const u16) -> uint16x4x3_t;
        unsafe fn vld3_u32(a: *const u32) -> uint32x2x3_t;
        unsafe fn vld3q_u8(a: *const u8) -> uint8x16x3_t;
        unsafe fn vld3q_u16(a: *const u16) -> uint16x8x3_t;
        unsafe fn vld3q_u32(a: *const u32) -> uint32x4x3_t;
        unsafe fn vld3_p8(a: *const p8) -> poly8x8x3_t;
        unsafe fn vld3_p16(a: *const p16) -> poly16x4x3_t;
        unsafe fn vld3q_p8(a: *const p8) -> poly8x16x3_t;
        unsafe fn vld3q_p16(a: *const p16) -> poly16x8x3_t;
        unsafe fn vld3_u64(a: *const u64) -> uint64x1x3_t;
        unsafe fn vld3_f32(a: *const f32) -> float32x2x3_t;
        unsafe fn vld3q_f32(a: *const f32) -> float32x4x3_t;
        unsafe fn vld3_dup_s8(a: *const i8) -> int8x8x3_t;
        unsafe fn vld3_dup_s16(a: *const i16) -> int16x4x3_t;
        unsafe fn vld3_dup_s32(a: *const i32) -> int32x2x3_t;
        unsafe fn vld3q_dup_s8(a: *const i8) -> int8x16x3_t;
        unsafe fn vld3q_dup_s16(a: *const i16) -> int16x8x3_t;
        unsafe fn vld3q_dup_s32(a: *const i32) -> int32x4x3_t;
        unsafe fn vld3_dup_s64(a: *const i64) -> int64x1x3_t;
        unsafe fn vld3_dup_u8(a: *const u8) -> uint8x8x3_t;
        unsafe fn vld3_dup_u16(a: *const u16) -> uint16x4x3_t;
        unsafe fn vld3_dup_u32(a: *const u32) -> uint32x2x3_t;
        unsafe fn vld3q_dup_u8(a: *const u8) -> uint8x16x3_t;
        unsafe fn vld3q_dup_u16(a: *const u16) -> uint16x8x3_t;
        unsafe fn vld3q_dup_u32(a: *const u32) -> uint32x4x3_t;
        unsafe fn vld3_dup_p8(a: *const p8) -> poly8x8x3_t;
        unsafe fn vld3_dup_p16(a: *const p16) -> poly16x4x3_t;
        unsafe fn vld3q_dup_p8(a: *const p8) -> poly8x16x3_t;
        unsafe fn vld3q_dup_p16(a: *const p16) -> poly16x8x3_t;
        unsafe fn vld3_dup_u64(a: *const u64) -> uint64x1x3_t;
        unsafe fn vld3_dup_f32(a: *const f32) -> float32x2x3_t;
        unsafe fn vld3q_dup_f32(a: *const f32) -> float32x4x3_t;
        unsafe fn vld3_lane_s8<const LANE: i32>(a: *const i8, b: int8x8x3_t) -> int8x8x3_t;
        unsafe fn vld3_lane_s16<const LANE: i32>(a: *const i16, b: int16x4x3_t) -> int16x4x3_t;
        unsafe fn vld3_lane_s32<const LANE: i32>(a: *const i32, b: int32x2x3_t) -> int32x2x3_t;
        unsafe fn vld3q_lane_s16<const LANE: i32>(a: *const i16, b: int16x8x3_t) -> int16x8x3_t;
        unsafe fn vld3q_lane_s32<const LANE: i32>(a: *const i32, b: int32x4x3_t) -> int32x4x3_t;
        unsafe fn vld3_lane_u8<const LANE: i32>(a: *const u8, b: uint8x8x3_t) -> uint8x8x3_t;
        unsafe fn vld3_lane_u16<const LANE: i32>(a: *const u16, b: uint16x4x3_t) -> uint16x4x3_t;
        unsafe fn vld3_lane_u32<const LANE: i32>(a: *const u32, b: uint32x2x3_t) -> uint32x2x3_t;
        unsafe fn vld3q_lane_u16<const LANE: i32>(a: *const u16, b: uint16x8x3_t) -> uint16x8x3_t;
        unsafe fn vld3q_lane_u32<const LANE: i32>(a: *const u32, b: uint32x4x3_t) -> uint32x4x3_t;
        unsafe fn vld3_lane_p8<const LANE: i32>(a: *const p8, b: poly8x8x3_t) -> poly8x8x3_t;
        unsafe fn vld3_lane_p16<const LANE: i32>(a: *const p16, b: poly16x4x3_t) -> poly16x4x3_t;
        unsafe fn vld3q_lane_p16<const LANE: i32>(a: *const p16, b: poly16x8x3_t) -> poly16x8x3_t;
        unsafe fn vld3_lane_f32<const LANE: i32>(a: *const f32, b: float32x2x3_t) -> float32x2x3_t;
        unsafe fn vld3q_lane_f32<const LANE: i32>(
            a: *const f32,
            b: float32x4x3_t,
        ) -> float32x4x3_t;
        unsafe fn vld4_s8(a: *const i8) -> int8x8x4_t;
        unsafe fn vld4_s16(a: *const i16) -> int16x4x4_t;
        unsafe fn vld4_s32(a: *const i32) -> int32x2x4_t;
        unsafe fn vld4q_s8(a: *const i8) -> int8x16x4_t;
        unsafe fn vld4q_s16(a: *const i16) -> int16x8x4_t;
        unsafe fn vld4q_s32(a: *const i32) -> int32x4x4_t;
        unsafe fn vld4_s64(a: *const i64) -> int64x1x4_t;
        unsafe fn vld4_u8(a: *const u8) -> uint8x8x4_t;
        unsafe fn vld4_u16(a: *const u16) -> uint16x4x4_t;
        unsafe fn vld4_u32(a: *const u32) -> uint32x2x4_t;
        unsafe fn vld4q_u8(a: *const u8) -> uint8x16x4_t;
        unsafe fn vld4q_u16(a: *const u16) -> uint16x8x4_t;
        unsafe fn vld4q_u32(a: *const u32) -> uint32x4x4_t;
        unsafe fn vld4_p8(a: *const p8) -> poly8x8x4_t;
        unsafe fn vld4_p16(a: *const p16) -> poly16x4x4_t;
        unsafe fn vld4q_p8(a: *const p8) -> poly8x16x4_t;
        unsafe fn vld4q_p16(a: *const p16) -> poly16x8x4_t;
        unsafe fn vld4_u64(a: *const u64) -> uint64x1x4_t;
        unsafe fn vld4_f32(a: *const f32) -> float32x2x4_t;
        unsafe fn vld4q_f32(a: *const f32) -> float32x4x4_t;
        unsafe fn vld4_dup_s8(a: *const i8) -> int8x8x4_t;
        unsafe fn vld4_dup_s16(a: *const i16) -> int16x4x4_t;
        unsafe fn vld4_dup_s32(a: *const i32) -> int32x2x4_t;
        unsafe fn vld4q_dup_s8(a: *const i8) -> int8x16x4_t;
        unsafe fn vld4q_dup_s16(a: *const i16) -> int16x8x4_t;
        unsafe fn vld4q_dup_s32(a: *const i32) -> int32x4x4_t;
        unsafe fn vld4_dup_s64(a: *const i64) -> int64x1x4_t;
        unsafe fn vld4_dup_u8(a: *const u8) -> uint8x8x4_t;
        unsafe fn vld4_dup_u16(a: *const u16) -> uint16x4x4_t;
        unsafe fn vld4_dup_u32(a: *const u32) -> uint32x2x4_t;
        unsafe fn vld4q_dup_u8(a: *const u8) -> uint8x16x4_t;
        unsafe fn vld4q_dup_u16(a: *const u16) -> uint16x8x4_t;
        unsafe fn vld4q_dup_u32(a: *const u32) -> uint32x4x4_t;
        unsafe fn vld4_dup_p8(a: *const p8) -> poly8x8x4_t;
        unsafe fn vld4_dup_p16(a: *const p16) -> poly16x4x4_t;
        unsafe fn vld4q_dup_p8(a: *const p8) -> poly8x16x4_t;
        unsafe fn vld4q_dup_p16(a: *const p16) -> poly16x8x4_t;
        unsafe fn vld4_dup_u64(a: *const u64) -> uint64x1x4_t;
        unsafe fn vld4_dup_f32(a: *const f32) -> float32x2x4_t;
        unsafe fn vld4q_dup_f32(a: *const f32) -> float32x4x4_t;
        unsafe fn vld4_lane_s8<const LANE: i32>(a: *const i8, b: int8x8x4_t) -> int8x8x4_t;
        unsafe fn vld4_lane_s16<const LANE: i32>(a: *const i16, b: int16x4x4_t) -> int16x4x4_t;
        unsafe fn vld4_lane_s32<const LANE: i32>(a: *const i32, b: int32x2x4_t) -> int32x2x4_t;
        unsafe fn vld4q_lane_s16<const LANE: i32>(a: *const i16, b: int16x8x4_t) -> int16x8x4_t;
        unsafe fn vld4q_lane_s32<const LANE: i32>(a: *const i32, b: int32x4x4_t) -> int32x4x4_t;
        unsafe fn vld4_lane_u8<const LANE: i32>(a: *const u8, b: uint8x8x4_t) -> uint8x8x4_t;
        unsafe fn vld4_lane_u16<const LANE: i32>(a: *const u16, b: uint16x4x4_t) -> uint16x4x4_t;
        unsafe fn vld4_lane_u32<const LANE: i32>(a: *const u32, b: uint32x2x4_t) -> uint32x2x4_t;
        unsafe fn vld4q_lane_u16<const LANE: i32>(a: *const u16, b: uint16x8x4_t) -> uint16x8x4_t;
        unsafe fn vld4q_lane_u32<const LANE: i32>(a: *const u32, b: uint32x4x4_t) -> uint32x4x4_t;
        unsafe fn vld4_lane_p8<const LANE: i32>(a: *const p8, b: poly8x8x4_t) -> poly8x8x4_t;
        unsafe fn vld4_lane_p16<const LANE: i32>(a: *const p16, b: poly16x4x4_t) -> poly16x4x4_t;
        unsafe fn vld4q_lane_p16<const LANE: i32>(a: *const p16, b: poly16x8x4_t) -> poly16x8x4_t;
        unsafe fn vld4_lane_f32<const LANE: i32>(a: *const f32, b: float32x2x4_t) -> float32x2x4_t;
        unsafe fn vld4q_lane_f32<const LANE: i32>(
            a: *const f32,
            b: float32x4x4_t,
        ) -> float32x4x4_t;
        unsafe fn vst1_lane_s8<const LANE: i32>(a: *mut i8, b: int8x8_t);
        unsafe fn vst1_lane_s16<const LANE: i32>(a: *mut i16, b: int16x4_t);
        unsafe fn vst1_lane_s32<const LANE: i32>(a: *mut i32, b: int32x2_t);
        unsafe fn vst1_lane_s64<const LANE: i32>(a: *mut i64, b: int64x1_t);
        unsafe fn vst1q_lane_s8<const LANE: i32>(a: *mut i8, b: int8x16_t);
        unsafe fn vst1q_lane_s16<const LANE: i32>(a: *mut i16, b: int16x8_t);
        unsafe fn vst1q_lane_s32<const LANE: i32>(a: *mut i32, b: int32x4_t);
        unsafe fn vst1q_lane_s64<const LANE: i32>(a: *mut i64, b: int64x2_t);
        unsafe fn vst1_lane_u8<const LANE: i32>(a: *mut u8, b: uint8x8_t);
        unsafe fn vst1_lane_u16<const LANE: i32>(a: *mut u16, b: uint16x4_t);
        unsafe fn vst1_lane_u32<const LANE: i32>(a: *mut u32, b: uint32x2_t);
        unsafe fn vst1_lane_u64<const LANE: i32>(a: *mut u64, b: uint64x1_t);
        unsafe fn vst1q_lane_u8<const LANE: i32>(a: *mut u8, b: uint8x16_t);
        unsafe fn vst1q_lane_u16<const LANE: i32>(a: *mut u16, b: uint16x8_t);
        unsafe fn vst1q_lane_u32<const LANE: i32>(a: *mut u32, b: uint32x4_t);
        unsafe fn vst1q_lane_u64<const LANE: i32>(a: *mut u64, b: uint64x2_t);
        unsafe fn vst1_lane_p8<const LANE: i32>(a: *mut p8, b: poly8x8_t);
        unsafe fn vst1_lane_p16<const LANE: i32>(a: *mut p16, b: poly16x4_t);
        unsafe fn vst1q_lane_p8<const LANE: i32>(a: *mut p8, b: poly8x16_t);
        unsafe fn vst1q_lane_p16<const LANE: i32>(a: *mut p16, b: poly16x8_t);
        unsafe fn vst1q_lane_p64<const LANE: i32>(a: *mut p64, b: poly64x2_t);
        unsafe fn vst1_lane_f32<const LANE: i32>(a: *mut f32, b: float32x2_t);
        unsafe fn vst1q_lane_f32<const LANE: i32>(a: *mut f32, b: float32x4_t);
        unsafe fn vst1_s8_x2(a: *mut i8, b: int8x8x2_t);
        unsafe fn vst1_s16_x2(a: *mut i16, b: int16x4x2_t);
        unsafe fn vst1_s32_x2(a: *mut i32, b: int32x2x2_t);
        unsafe fn vst1_s64_x2(a: *mut i64, b: int64x1x2_t);
        unsafe fn vst1q_s8_x2(a: *mut i8, b: int8x16x2_t);
        unsafe fn vst1q_s16_x2(a: *mut i16, b: int16x8x2_t);
        unsafe fn vst1q_s32_x2(a: *mut i32, b: int32x4x2_t);
        unsafe fn vst1q_s64_x2(a: *mut i64, b: int64x2x2_t);
        unsafe fn vst1_s8_x3(a: *mut i8, b: int8x8x3_t);
        unsafe fn vst1_s16_x3(a: *mut i16, b: int16x4x3_t);
        unsafe fn vst1_s32_x3(a: *mut i32, b: int32x2x3_t);
        unsafe fn vst1_s64_x3(a: *mut i64, b: int64x1x3_t);
        unsafe fn vst1q_s8_x3(a: *mut i8, b: int8x16x3_t);
        unsafe fn vst1q_s16_x3(a: *mut i16, b: int16x8x3_t);
        unsafe fn vst1q_s32_x3(a: *mut i32, b: int32x4x3_t);
        unsafe fn vst1q_s64_x3(a: *mut i64, b: int64x2x3_t);
        unsafe fn vst1_s8_x4(a: *mut i8, b: int8x8x4_t);
        unsafe fn vst1_s16_x4(a: *mut i16, b: int16x4x4_t);
        unsafe fn vst1_s32_x4(a: *mut i32, b: int32x2x4_t);
        unsafe fn vst1_s64_x4(a: *mut i64, b: int64x1x4_t);
        unsafe fn vst1q_s8_x4(a: *mut i8, b: int8x16x4_t);
        unsafe fn vst1q_s16_x4(a: *mut i16, b: int16x8x4_t);
        unsafe fn vst1q_s32_x4(a: *mut i32, b: int32x4x4_t);
        unsafe fn vst1q_s64_x4(a: *mut i64, b: int64x2x4_t);
        unsafe fn vst1_u8_x2(a: *mut u8, b: uint8x8x2_t);
        unsafe fn vst1_u16_x2(a: *mut u16, b: uint16x4x2_t);
        unsafe fn vst1_u32_x2(a: *mut u32, b: uint32x2x2_t);
        unsafe fn vst1_u64_x2(a: *mut u64, b: uint64x1x2_t);
        unsafe fn vst1q_u8_x2(a: *mut u8, b: uint8x16x2_t);
        unsafe fn vst1q_u16_x2(a: *mut u16, b: uint16x8x2_t);
        unsafe fn vst1q_u32_x2(a: *mut u32, b: uint32x4x2_t);
        unsafe fn vst1q_u64_x2(a: *mut u64, b: uint64x2x2_t);
        unsafe fn vst1_u8_x3(a: *mut u8, b: uint8x8x3_t);
        unsafe fn vst1_u16_x3(a: *mut u16, b: uint16x4x3_t);
        unsafe fn vst1_u32_x3(a: *mut u32, b: uint32x2x3_t);
        unsafe fn vst1_u64_x3(a: *mut u64, b: uint64x1x3_t);
        unsafe fn vst1q_u8_x3(a: *mut u8, b: uint8x16x3_t);
        unsafe fn vst1q_u16_x3(a: *mut u16, b: uint16x8x3_t);
        unsafe fn vst1q_u32_x3(a: *mut u32, b: uint32x4x3_t);
        unsafe fn vst1q_u64_x3(a: *mut u64, b: uint64x2x3_t);
        unsafe fn vst1_u8_x4(a: *mut u8, b: uint8x8x4_t);
        unsafe fn vst1_u16_x4(a: *mut u16, b: uint16x4x4_t);
        unsafe fn vst1_u32_x4(a: *mut u32, b: uint32x2x4_t);
        unsafe fn vst1_u64_x4(a: *mut u64, b: uint64x1x4_t);
        unsafe fn vst1q_u8_x4(a: *mut u8, b: uint8x16x4_t);
        unsafe fn vst1q_u16_x4(a: *mut u16, b: uint16x8x4_t);
        unsafe fn vst1q_u32_x4(a: *mut u32, b: uint32x4x4_t);
        unsafe fn vst1q_u64_x4(a: *mut u64, b: uint64x2x4_t);
        unsafe fn vst1_p8_x2(a: *mut p8, b: poly8x8x2_t);
        unsafe fn vst1_p8_x3(a: *mut p8, b: poly8x8x3_t);
        unsafe fn vst1_p8_x4(a: *mut p8, b: poly8x8x4_t);
        unsafe fn vst1q_p8_x2(a: *mut p8, b: poly8x16x2_t);
        unsafe fn vst1q_p8_x3(a: *mut p8, b: poly8x16x3_t);
        unsafe fn vst1q_p8_x4(a: *mut p8, b: poly8x16x4_t);
        unsafe fn vst1_p16_x2(a: *mut p16, b: poly16x4x2_t);
        unsafe fn vst1_p16_x3(a: *mut p16, b: poly16x4x3_t);
        unsafe fn vst1_p16_x4(a: *mut p16, b: poly16x4x4_t);
        unsafe fn vst1q_p16_x2(a: *mut p16, b: poly16x8x2_t);
        unsafe fn vst1q_p16_x3(a: *mut p16, b: poly16x8x3_t);
        unsafe fn vst1q_p16_x4(a: *mut p16, b: poly16x8x4_t);
        unsafe fn vst1_f32_x2(a: *mut f32, b: float32x2x2_t);
        unsafe fn vst1q_f32_x2(a: *mut f32, b: float32x4x2_t);
        unsafe fn vst1_f32_x3(a: *mut f32, b: float32x2x3_t);
        unsafe fn vst1q_f32_x3(a: *mut f32, b: float32x4x3_t);
        unsafe fn vst1_f32_x4(a: *mut f32, b: float32x2x4_t);
        unsafe fn vst1q_f32_x4(a: *mut f32, b: float32x4x4_t);
        unsafe fn vst2_s8(a: *mut i8, b: int8x8x2_t);
        unsafe fn vst2_s16(a: *mut i16, b: int16x4x2_t);
        unsafe fn vst2_s32(a: *mut i32, b: int32x2x2_t);
        unsafe fn vst2q_s8(a: *mut i8, b: int8x16x2_t);
        unsafe fn vst2q_s16(a: *mut i16, b: int16x8x2_t);
        unsafe fn vst2q_s32(a: *mut i32, b: int32x4x2_t);
        unsafe fn vst2_s64(a: *mut i64, b: int64x1x2_t);
        unsafe fn vst2_u8(a: *mut u8, b: uint8x8x2_t);
        unsafe fn vst2_u16(a: *mut u16, b: uint16x4x2_t);
        unsafe fn vst2_u32(a: *mut u32, b: uint32x2x2_t);
        unsafe fn vst2q_u8(a: *mut u8, b: uint8x16x2_t);
        unsafe fn vst2q_u16(a: *mut u16, b: uint16x8x2_t);
        unsafe fn vst2q_u32(a: *mut u32, b: uint32x4x2_t);
        unsafe fn vst2_p8(a: *mut p8, b: poly8x8x2_t);
        unsafe fn vst2_p16(a: *mut p16, b: poly16x4x2_t);
        unsafe fn vst2q_p8(a: *mut p8, b: poly8x16x2_t);
        unsafe fn vst2q_p16(a: *mut p16, b: poly16x8x2_t);
        unsafe fn vst2_u64(a: *mut u64, b: uint64x1x2_t);
        unsafe fn vst2_f32(a: *mut f32, b: float32x2x2_t);
        unsafe fn vst2q_f32(a: *mut f32, b: float32x4x2_t);
        unsafe fn vst2_lane_s8<const LANE: i32>(a: *mut i8, b: int8x8x2_t);
        unsafe fn vst2_lane_s16<const LANE: i32>(a: *mut i16, b: int16x4x2_t);
        unsafe fn vst2_lane_s32<const LANE: i32>(a: *mut i32, b: int32x2x2_t);
        unsafe fn vst2q_lane_s16<const LANE: i32>(a: *mut i16, b: int16x8x2_t);
        unsafe fn vst2q_lane_s32<const LANE: i32>(a: *mut i32, b: int32x4x2_t);
        unsafe fn vst2_lane_u8<const LANE: i32>(a: *mut u8, b: uint8x8x2_t);
        unsafe fn vst2_lane_u16<const LANE: i32>(a: *mut u16, b: uint16x4x2_t);
        unsafe fn vst2_lane_u32<const LANE: i32>(a: *mut u32, b: uint32x2x2_t);
        unsafe fn vst2q_lane_u16<const LANE: i32>(a: *mut u16, b: uint16x8x2_t);
        unsafe fn vst2q_lane_u32<const LANE: i32>(a: *mut u32, b: uint32x4x2_t);
        unsafe fn vst2_lane_p8<const LANE: i32>(a: *mut p8, b: poly8x8x2_t);
        unsafe fn vst2_lane_p16<const LANE: i32>(a: *mut p16, b: poly16x4x2_t);
        unsafe fn vst2q_lane_p16<const LANE: i32>(a: *mut p16, b: poly16x8x2_t);
        unsafe fn vst2_lane_f32<const LANE: i32>(a: *mut f32, b: float32x2x2_t);
        unsafe fn vst2q_lane_f32<const LANE: i32>(a: *mut f32, b: float32x4x2_t);
        unsafe fn vst3_s8(a: *mut i8, b: int8x8x3_t);
        unsafe fn vst3_s16(a: *mut i16, b: int16x4x3_t);
        unsafe fn vst3_s32(a: *mut i32, b: int32x2x3_t);
        unsafe fn vst3q_s8(a: *mut i8, b: int8x16x3_t);
        unsafe fn vst3q_s16(a: *mut i16, b: int16x8x3_t);
        unsafe fn vst3q_s32(a: *mut i32, b: int32x4x3_t);
        unsafe fn vst3_s64(a: *mut i64, b: int64x1x3_t);
        unsafe fn vst3_u8(a: *mut u8, b: uint8x8x3_t);
        unsafe fn vst3_u16(a: *mut u16, b: uint16x4x3_t);
        unsafe fn vst3_u32(a: *mut u32, b: uint32x2x3_t);
        unsafe fn vst3q_u8(a: *mut u8, b: uint8x16x3_t);
        unsafe fn vst3q_u16(a: *mut u16, b: uint16x8x3_t);
        unsafe fn vst3q_u32(a: *mut u32, b: uint32x4x3_t);
        unsafe fn vst3_p8(a: *mut p8, b: poly8x8x3_t);
        unsafe fn vst3_p16(a: *mut p16, b: poly16x4x3_t);
        unsafe fn vst3q_p8(a: *mut p8, b: poly8x16x3_t);
        unsafe fn vst3q_p16(a: *mut p16, b: poly16x8x3_t);
        unsafe fn vst3_u64(a: *mut u64, b: uint64x1x3_t);
        unsafe fn vst3_f32(a: *mut f32, b: float32x2x3_t);
        unsafe fn vst3q_f32(a: *mut f32, b: float32x4x3_t);
        unsafe fn vst3_lane_s8<const LANE: i32>(a: *mut i8, b: int8x8x3_t);
        unsafe fn vst3_lane_s16<const LANE: i32>(a: *mut i16, b: int16x4x3_t);
        unsafe fn vst3_lane_s32<const LANE: i32>(a: *mut i32, b: int32x2x3_t);
        unsafe fn vst3q_lane_s16<const LANE: i32>(a: *mut i16, b: int16x8x3_t);
        unsafe fn vst3q_lane_s32<const LANE: i32>(a: *mut i32, b: int32x4x3_t);
        unsafe fn vst3_lane_u8<const LANE: i32>(a: *mut u8, b: uint8x8x3_t);
        unsafe fn vst3_lane_u16<const LANE: i32>(a: *mut u16, b: uint16x4x3_t);
        unsafe fn vst3_lane_u32<const LANE: i32>(a: *mut u32, b: uint32x2x3_t);
        unsafe fn vst3q_lane_u16<const LANE: i32>(a: *mut u16, b: uint16x8x3_t);
        unsafe fn vst3q_lane_u32<const LANE: i32>(a: *mut u32, b: uint32x4x3_t);
        unsafe fn vst3_lane_p8<const LANE: i32>(a: *mut p8, b: poly8x8x3_t);
        unsafe fn vst3_lane_p16<const LANE: i32>(a: *mut p16, b: poly16x4x3_t);
        unsafe fn vst3q_lane_p16<const LANE: i32>(a: *mut p16, b: poly16x8x3_t);
        unsafe fn vst3_lane_f32<const LANE: i32>(a: *mut f32, b: float32x2x3_t);
        unsafe fn vst3q_lane_f32<const LANE: i32>(a: *mut f32, b: float32x4x3_t);
        unsafe fn vst4_s8(a: *mut i8, b: int8x8x4_t);
        unsafe fn vst4_s16(a: *mut i16, b: int16x4x4_t);
        unsafe fn vst4_s32(a: *mut i32, b: int32x2x4_t);
        unsafe fn vst4q_s8(a: *mut i8, b: int8x16x4_t);
        unsafe fn vst4q_s16(a: *mut i16, b: int16x8x4_t);
        unsafe fn vst4q_s32(a: *mut i32, b: int32x4x4_t);
        unsafe fn vst4_s64(a: *mut i64, b: int64x1x4_t);
        unsafe fn vst4_u8(a: *mut u8, b: uint8x8x4_t);
        unsafe fn vst4_u16(a: *mut u16, b: uint16x4x4_t);
        unsafe fn vst4_u32(a: *mut u32, b: uint32x2x4_t);
        unsafe fn vst4q_u8(a: *mut u8, b: uint8x16x4_t);
        unsafe fn vst4q_u16(a: *mut u16, b: uint16x8x4_t);
        unsafe fn vst4q_u32(a: *mut u32, b: uint32x4x4_t);
        unsafe fn vst4_p8(a: *mut p8, b: poly8x8x4_t);
        unsafe fn vst4_p16(a: *mut p16, b: poly16x4x4_t);
        unsafe fn vst4q_p8(a: *mut p8, b: poly8x16x4_t);
        unsafe fn vst4q_p16(a: *mut p16, b: poly16x8x4_t);
        unsafe fn vst4_u64(a: *mut u64, b: uint64x1x4_t);
        unsafe fn vst4_f32(a: *mut f32, b: float32x2x4_t);
        unsafe fn vst4q_f32(a: *mut f32, b: float32x4x4_t);
        unsafe fn vst4_lane_s8<const LANE: i32>(a: *mut i8, b: int8x8x4_t);
        unsafe fn vst4_lane_s16<const LANE: i32>(a: *mut i16, b: int16x4x4_t);
        unsafe fn vst4_lane_s32<const LANE: i32>(a: *mut i32, b: int32x2x4_t);
        unsafe fn vst4q_lane_s16<const LANE: i32>(a: *mut i16, b: int16x8x4_t);
        unsafe fn vst4q_lane_s32<const LANE: i32>(a: *mut i32, b: int32x4x4_t);
        unsafe fn vst4_lane_u8<const LANE: i32>(a: *mut u8, b: uint8x8x4_t);
        unsafe fn vst4_lane_u16<const LANE: i32>(a: *mut u16, b: uint16x4x4_t);
        unsafe fn vst4_lane_u32<const LANE: i32>(a: *mut u32, b: uint32x2x4_t);
        unsafe fn vst4q_lane_u16<const LANE: i32>(a: *mut u16, b: uint16x8x4_t);
        unsafe fn vst4q_lane_u32<const LANE: i32>(a: *mut u32, b: uint32x4x4_t);
        unsafe fn vst4_lane_p8<const LANE: i32>(a: *mut p8, b: poly8x8x4_t);
        unsafe fn vst4_lane_p16<const LANE: i32>(a: *mut p16, b: poly16x4x4_t);
        unsafe fn vst4q_lane_p16<const LANE: i32>(a: *mut p16, b: poly16x8x4_t);
        unsafe fn vst4_lane_f32<const LANE: i32>(a: *mut f32, b: float32x2x4_t);
        unsafe fn vst4q_lane_f32<const LANE: i32>(a: *mut f32, b: float32x4x4_t);
        fn vmul_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vmulq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vmul_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vmulq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vmul_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vmulq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vmul_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vmulq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vmul_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vmulq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vmul_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vmulq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vmul_p8(a: poly8x8_t, b: poly8x8_t) -> poly8x8_t;
        fn vmulq_p8(a: poly8x16_t, b: poly8x16_t) -> poly8x16_t;
        fn vmul_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vmulq_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vmul_n_s16(a: int16x4_t, b: i16) -> int16x4_t;
        fn vmulq_n_s16(a: int16x8_t, b: i16) -> int16x8_t;
        fn vmul_n_s32(a: int32x2_t, b: i32) -> int32x2_t;
        fn vmulq_n_s32(a: int32x4_t, b: i32) -> int32x4_t;
        fn vmul_n_u16(a: uint16x4_t, b: u16) -> uint16x4_t;
        fn vmulq_n_u16(a: uint16x8_t, b: u16) -> uint16x8_t;
        fn vmul_n_u32(a: uint32x2_t, b: u32) -> uint32x2_t;
        fn vmulq_n_u32(a: uint32x4_t, b: u32) -> uint32x4_t;
        fn vmul_n_f32(a: float32x2_t, b: f32) -> float32x2_t;
        fn vmulq_n_f32(a: float32x4_t, b: f32) -> float32x4_t;
        fn vmul_lane_s16<const LANE: i32>(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vmul_laneq_s16<const LANE: i32>(a: int16x4_t, b: int16x8_t) -> int16x4_t;
        fn vmulq_lane_s16<const LANE: i32>(a: int16x8_t, b: int16x4_t) -> int16x8_t;
        fn vmulq_laneq_s16<const LANE: i32>(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vmul_lane_s32<const LANE: i32>(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vmul_laneq_s32<const LANE: i32>(a: int32x2_t, b: int32x4_t) -> int32x2_t;
        fn vmulq_lane_s32<const LANE: i32>(a: int32x4_t, b: int32x2_t) -> int32x4_t;
        fn vmulq_laneq_s32<const LANE: i32>(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vmul_lane_u16<const LANE: i32>(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vmul_laneq_u16<const LANE: i32>(a: uint16x4_t, b: uint16x8_t) -> uint16x4_t;
        fn vmulq_lane_u16<const LANE: i32>(a: uint16x8_t, b: uint16x4_t) -> uint16x8_t;
        fn vmulq_laneq_u16<const LANE: i32>(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vmul_lane_u32<const LANE: i32>(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vmul_laneq_u32<const LANE: i32>(a: uint32x2_t, b: uint32x4_t) -> uint32x2_t;
        fn vmulq_lane_u32<const LANE: i32>(a: uint32x4_t, b: uint32x2_t) -> uint32x4_t;
        fn vmulq_laneq_u32<const LANE: i32>(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vmul_lane_f32<const LANE: i32>(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vmul_laneq_f32<const LANE: i32>(a: float32x2_t, b: float32x4_t) -> float32x2_t;
        fn vmulq_lane_f32<const LANE: i32>(a: float32x4_t, b: float32x2_t) -> float32x4_t;
        fn vmulq_laneq_f32<const LANE: i32>(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vmull_s8(a: int8x8_t, b: int8x8_t) -> int16x8_t;
        fn vmull_s16(a: int16x4_t, b: int16x4_t) -> int32x4_t;
        fn vmull_s32(a: int32x2_t, b: int32x2_t) -> int64x2_t;
        fn vmull_u8(a: uint8x8_t, b: uint8x8_t) -> uint16x8_t;
        fn vmull_u16(a: uint16x4_t, b: uint16x4_t) -> uint32x4_t;
        fn vmull_u32(a: uint32x2_t, b: uint32x2_t) -> uint64x2_t;
        fn vmull_p8(a: poly8x8_t, b: poly8x8_t) -> poly16x8_t;
        fn vmull_n_s16(a: int16x4_t, b: i16) -> int32x4_t;
        fn vmull_n_s32(a: int32x2_t, b: i32) -> int64x2_t;
        fn vmull_n_u16(a: uint16x4_t, b: u16) -> uint32x4_t;
        fn vmull_n_u32(a: uint32x2_t, b: u32) -> uint64x2_t;
        fn vmull_lane_s16<const LANE: i32>(a: int16x4_t, b: int16x4_t) -> int32x4_t;
        fn vmull_laneq_s16<const LANE: i32>(a: int16x4_t, b: int16x8_t) -> int32x4_t;
        fn vmull_lane_s32<const LANE: i32>(a: int32x2_t, b: int32x2_t) -> int64x2_t;
        fn vmull_laneq_s32<const LANE: i32>(a: int32x2_t, b: int32x4_t) -> int64x2_t;
        fn vmull_lane_u16<const LANE: i32>(a: uint16x4_t, b: uint16x4_t) -> uint32x4_t;
        fn vmull_laneq_u16<const LANE: i32>(a: uint16x4_t, b: uint16x8_t) -> uint32x4_t;
        fn vmull_lane_u32<const LANE: i32>(a: uint32x2_t, b: uint32x2_t) -> uint64x2_t;
        fn vmull_laneq_u32<const LANE: i32>(a: uint32x2_t, b: uint32x4_t) -> uint64x2_t;
        fn vfma_f32(a: float32x2_t, b: float32x2_t, c: float32x2_t) -> float32x2_t;
        fn vfmaq_f32(a: float32x4_t, b: float32x4_t, c: float32x4_t) -> float32x4_t;
        fn vfma_n_f32(a: float32x2_t, b: float32x2_t, c: f32) -> float32x2_t;
        fn vfmaq_n_f32(a: float32x4_t, b: float32x4_t, c: f32) -> float32x4_t;
        fn vfms_f32(a: float32x2_t, b: float32x2_t, c: float32x2_t) -> float32x2_t;
        fn vfmsq_f32(a: float32x4_t, b: float32x4_t, c: float32x4_t) -> float32x4_t;
        fn vfms_n_f32(a: float32x2_t, b: float32x2_t, c: f32) -> float32x2_t;
        fn vfmsq_n_f32(a: float32x4_t, b: float32x4_t, c: f32) -> float32x4_t;
        fn vsub_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vsubq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vsub_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vsubq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vsub_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vsubq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vsub_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vsubq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vsub_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vsubq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vsub_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vsubq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vsub_s64(a: int64x1_t, b: int64x1_t) -> int64x1_t;
        fn vsubq_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vsub_u64(a: uint64x1_t, b: uint64x1_t) -> uint64x1_t;
        fn vsubq_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vsub_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vsubq_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vadd_p8(a: poly8x8_t, b: poly8x8_t) -> poly8x8_t;
        fn vadd_p16(a: poly16x4_t, b: poly16x4_t) -> poly16x4_t;
        fn vaddq_p8(a: poly8x16_t, b: poly8x16_t) -> poly8x16_t;
        fn vaddq_p16(a: poly16x8_t, b: poly16x8_t) -> poly16x8_t;
        fn vadd_p64(a: poly64x1_t, b: poly64x1_t) -> poly64x1_t;
        fn vaddq_p64(a: poly64x2_t, b: poly64x2_t) -> poly64x2_t;
        fn vaddq_p128(a: p128, b: p128) -> p128;
        fn vsubhn_s16(a: int16x8_t, b: int16x8_t) -> int8x8_t;
        fn vsubhn_s32(a: int32x4_t, b: int32x4_t) -> int16x4_t;
        fn vsubhn_s64(a: int64x2_t, b: int64x2_t) -> int32x2_t;
        fn vsubhn_u16(a: uint16x8_t, b: uint16x8_t) -> uint8x8_t;
        fn vsubhn_u32(a: uint32x4_t, b: uint32x4_t) -> uint16x4_t;
        fn vsubhn_u64(a: uint64x2_t, b: uint64x2_t) -> uint32x2_t;
        fn vsubhn_high_s16(a: int8x8_t, b: int16x8_t, c: int16x8_t) -> int8x16_t;
        fn vsubhn_high_s32(a: int16x4_t, b: int32x4_t, c: int32x4_t) -> int16x8_t;
        fn vsubhn_high_s64(a: int32x2_t, b: int64x2_t, c: int64x2_t) -> int32x4_t;
        fn vsubhn_high_u16(a: uint8x8_t, b: uint16x8_t, c: uint16x8_t) -> uint8x16_t;
        fn vsubhn_high_u32(a: uint16x4_t, b: uint32x4_t, c: uint32x4_t) -> uint16x8_t;
        fn vsubhn_high_u64(a: uint32x2_t, b: uint64x2_t, c: uint64x2_t) -> uint32x4_t;
        fn vhsub_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vhsubq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vhsub_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vhsubq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vhsub_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vhsubq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vhsub_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vhsubq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vhsub_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vhsubq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vhsub_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vhsubq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vsubw_s8(a: int16x8_t, b: int8x8_t) -> int16x8_t;
        fn vsubw_s16(a: int32x4_t, b: int16x4_t) -> int32x4_t;
        fn vsubw_s32(a: int64x2_t, b: int32x2_t) -> int64x2_t;
        fn vsubw_u8(a: uint16x8_t, b: uint8x8_t) -> uint16x8_t;
        fn vsubw_u16(a: uint32x4_t, b: uint16x4_t) -> uint32x4_t;
        fn vsubw_u32(a: uint64x2_t, b: uint32x2_t) -> uint64x2_t;
        fn vsubl_s8(a: int8x8_t, b: int8x8_t) -> int16x8_t;
        fn vsubl_s16(a: int16x4_t, b: int16x4_t) -> int32x4_t;
        fn vsubl_s32(a: int32x2_t, b: int32x2_t) -> int64x2_t;
        fn vsubl_u8(a: uint8x8_t, b: uint8x8_t) -> uint16x8_t;
        fn vsubl_u16(a: uint16x4_t, b: uint16x4_t) -> uint32x4_t;
        fn vsubl_u32(a: uint32x2_t, b: uint32x2_t) -> uint64x2_t;
        fn vmax_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vmaxq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vmax_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vmaxq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vmax_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vmaxq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vmax_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vmaxq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vmax_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vmaxq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vmax_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vmaxq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vmax_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vmaxq_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vmaxnm_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vmaxnmq_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vmin_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vminq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vmin_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vminq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vmin_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vminq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vmin_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vminq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vmin_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vminq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vmin_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vminq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vmin_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vminq_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vminnm_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vminnmq_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vpadd_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vqdmull_s16(a: int16x4_t, b: int16x4_t) -> int32x4_t;
        fn vqdmull_s32(a: int32x2_t, b: int32x2_t) -> int64x2_t;
        fn vqdmull_n_s16(a: int16x4_t, b: i16) -> int32x4_t;
        fn vqdmull_n_s32(a: int32x2_t, b: i32) -> int64x2_t;
        fn vqdmull_lane_s16<const N: i32>(a: int16x4_t, b: int16x4_t) -> int32x4_t;
        fn vqdmull_lane_s32<const N: i32>(a: int32x2_t, b: int32x2_t) -> int64x2_t;
        fn vqdmlal_s16(a: int32x4_t, b: int16x4_t, c: int16x4_t) -> int32x4_t;
        fn vqdmlal_s32(a: int64x2_t, b: int32x2_t, c: int32x2_t) -> int64x2_t;
        fn vqdmlal_n_s16(a: int32x4_t, b: int16x4_t, c: i16) -> int32x4_t;
        fn vqdmlal_n_s32(a: int64x2_t, b: int32x2_t, c: i32) -> int64x2_t;
        fn vqdmlal_lane_s16<const N: i32>(
            a: int32x4_t,
            b: int16x4_t,
            c: int16x4_t,
        ) -> int32x4_t;
        fn vqdmlal_lane_s32<const N: i32>(
            a: int64x2_t,
            b: int32x2_t,
            c: int32x2_t,
        ) -> int64x2_t;
        fn vqdmlsl_s16(a: int32x4_t, b: int16x4_t, c: int16x4_t) -> int32x4_t;
        fn vqdmlsl_s32(a: int64x2_t, b: int32x2_t, c: int32x2_t) -> int64x2_t;
        fn vqdmlsl_n_s16(a: int32x4_t, b: int16x4_t, c: i16) -> int32x4_t;
        fn vqdmlsl_n_s32(a: int64x2_t, b: int32x2_t, c: i32) -> int64x2_t;
        fn vqdmlsl_lane_s16<const N: i32>(
            a: int32x4_t,
            b: int16x4_t,
            c: int16x4_t,
        ) -> int32x4_t;
        fn vqdmlsl_lane_s32<const N: i32>(
            a: int64x2_t,
            b: int32x2_t,
            c: int32x2_t,
        ) -> int64x2_t;
        fn vqdmulh_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vqdmulhq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vqdmulh_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vqdmulhq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vqdmulh_n_s16(a: int16x4_t, b: i16) -> int16x4_t;
        fn vqdmulh_n_s32(a: int32x2_t, b: i32) -> int32x2_t;
        fn vqdmulhq_n_s16(a: int16x8_t, b: i16) -> int16x8_t;
        fn vqdmulhq_n_s32(a: int32x4_t, b: i32) -> int32x4_t;
        fn vqdmulhq_laneq_s16<const LANE: i32>(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vqdmulh_laneq_s16<const LANE: i32>(a: int16x4_t, b: int16x8_t) -> int16x4_t;
        fn vqdmulhq_laneq_s32<const LANE: i32>(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vqdmulh_laneq_s32<const LANE: i32>(a: int32x2_t, b: int32x4_t) -> int32x2_t;
        fn vqmovn_s16(a: int16x8_t) -> int8x8_t;
        fn vqmovn_s32(a: int32x4_t) -> int16x4_t;
        fn vqmovn_s64(a: int64x2_t) -> int32x2_t;
        fn vqmovn_u16(a: uint16x8_t) -> uint8x8_t;
        fn vqmovn_u32(a: uint32x4_t) -> uint16x4_t;
        fn vqmovn_u64(a: uint64x2_t) -> uint32x2_t;
        fn vqmovun_s16(a: int16x8_t) -> uint8x8_t;
        fn vqmovun_s32(a: int32x4_t) -> uint16x4_t;
        fn vqmovun_s64(a: int64x2_t) -> uint32x2_t;
        fn vqrdmulh_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vqrdmulhq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vqrdmulh_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vqrdmulhq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vqrdmulh_n_s16(a: int16x4_t, b: i16) -> int16x4_t;
        fn vqrdmulhq_n_s16(a: int16x8_t, b: i16) -> int16x8_t;
        fn vqrdmulh_n_s32(a: int32x2_t, b: i32) -> int32x2_t;
        fn vqrdmulhq_n_s32(a: int32x4_t, b: i32) -> int32x4_t;
        fn vqrdmulh_lane_s16<const LANE: i32>(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vqrdmulh_laneq_s16<const LANE: i32>(a: int16x4_t, b: int16x8_t) -> int16x4_t;
        fn vqrdmulhq_lane_s16<const LANE: i32>(a: int16x8_t, b: int16x4_t) -> int16x8_t;
        fn vqrdmulhq_laneq_s16<const LANE: i32>(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vqrdmulh_lane_s32<const LANE: i32>(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vqrdmulh_laneq_s32<const LANE: i32>(a: int32x2_t, b: int32x4_t) -> int32x2_t;
        fn vqrdmulhq_lane_s32<const LANE: i32>(a: int32x4_t, b: int32x2_t) -> int32x4_t;
        fn vqrdmulhq_laneq_s32<const LANE: i32>(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vqrshl_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vqrshlq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vqrshl_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vqrshlq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vqrshl_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vqrshlq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vqrshl_s64(a: int64x1_t, b: int64x1_t) -> int64x1_t;
        fn vqrshlq_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vqrshl_u8(a: uint8x8_t, b: int8x8_t) -> uint8x8_t;
        fn vqrshlq_u8(a: uint8x16_t, b: int8x16_t) -> uint8x16_t;
        fn vqrshl_u16(a: uint16x4_t, b: int16x4_t) -> uint16x4_t;
        fn vqrshlq_u16(a: uint16x8_t, b: int16x8_t) -> uint16x8_t;
        fn vqrshl_u32(a: uint32x2_t, b: int32x2_t) -> uint32x2_t;
        fn vqrshlq_u32(a: uint32x4_t, b: int32x4_t) -> uint32x4_t;
        fn vqrshl_u64(a: uint64x1_t, b: int64x1_t) -> uint64x1_t;
        fn vqrshlq_u64(a: uint64x2_t, b: int64x2_t) -> uint64x2_t;
        fn vqrshrn_n_s16<const N: i32>(a: int16x8_t) -> int8x8_t;
        fn vqrshrn_n_s32<const N: i32>(a: int32x4_t) -> int16x4_t;
        fn vqrshrn_n_s64<const N: i32>(a: int64x2_t) -> int32x2_t;
        fn vqrshrn_n_u16<const N: i32>(a: uint16x8_t) -> uint8x8_t;
        fn vqrshrn_n_u32<const N: i32>(a: uint32x4_t) -> uint16x4_t;
        fn vqrshrn_n_u64<const N: i32>(a: uint64x2_t) -> uint32x2_t;
        fn vqrshrun_n_s16<const N: i32>(a: int16x8_t) -> uint8x8_t;
        fn vqrshrun_n_s32<const N: i32>(a: int32x4_t) -> uint16x4_t;
        fn vqrshrun_n_s64<const N: i32>(a: int64x2_t) -> uint32x2_t;
        fn vqshl_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vqshlq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vqshl_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vqshlq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vqshl_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vqshlq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vqshl_s64(a: int64x1_t, b: int64x1_t) -> int64x1_t;
        fn vqshlq_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vqshl_u8(a: uint8x8_t, b: int8x8_t) -> uint8x8_t;
        fn vqshlq_u8(a: uint8x16_t, b: int8x16_t) -> uint8x16_t;
        fn vqshl_u16(a: uint16x4_t, b: int16x4_t) -> uint16x4_t;
        fn vqshlq_u16(a: uint16x8_t, b: int16x8_t) -> uint16x8_t;
        fn vqshl_u32(a: uint32x2_t, b: int32x2_t) -> uint32x2_t;
        fn vqshlq_u32(a: uint32x4_t, b: int32x4_t) -> uint32x4_t;
        fn vqshl_u64(a: uint64x1_t, b: int64x1_t) -> uint64x1_t;
        fn vqshlq_u64(a: uint64x2_t, b: int64x2_t) -> uint64x2_t;
        fn vqshl_n_s8<const N: i32>(a: int8x8_t) -> int8x8_t;
        fn vqshlq_n_s8<const N: i32>(a: int8x16_t) -> int8x16_t;
        fn vqshl_n_s16<const N: i32>(a: int16x4_t) -> int16x4_t;
        fn vqshlq_n_s16<const N: i32>(a: int16x8_t) -> int16x8_t;
        fn vqshl_n_s32<const N: i32>(a: int32x2_t) -> int32x2_t;
        fn vqshlq_n_s32<const N: i32>(a: int32x4_t) -> int32x4_t;
        fn vqshl_n_s64<const N: i32>(a: int64x1_t) -> int64x1_t;
        fn vqshlq_n_s64<const N: i32>(a: int64x2_t) -> int64x2_t;
        fn vqshl_n_u8<const N: i32>(a: uint8x8_t) -> uint8x8_t;
        fn vqshlq_n_u8<const N: i32>(a: uint8x16_t) -> uint8x16_t;
        fn vqshl_n_u16<const N: i32>(a: uint16x4_t) -> uint16x4_t;
        fn vqshlq_n_u16<const N: i32>(a: uint16x8_t) -> uint16x8_t;
        fn vqshl_n_u32<const N: i32>(a: uint32x2_t) -> uint32x2_t;
        fn vqshlq_n_u32<const N: i32>(a: uint32x4_t) -> uint32x4_t;
        fn vqshl_n_u64<const N: i32>(a: uint64x1_t) -> uint64x1_t;
        fn vqshlq_n_u64<const N: i32>(a: uint64x2_t) -> uint64x2_t;
        fn vqshlu_n_s8<const N: i32>(a: int8x8_t) -> uint8x8_t;
        fn vqshlu_n_s16<const N: i32>(a: int16x4_t) -> uint16x4_t;
        fn vqshlu_n_s32<const N: i32>(a: int32x2_t) -> uint32x2_t;
        fn vqshlu_n_s64<const N: i32>(a: int64x1_t) -> uint64x1_t;
        fn vqshluq_n_s8<const N: i32>(a: int8x16_t) -> uint8x16_t;
        fn vqshluq_n_s16<const N: i32>(a: int16x8_t) -> uint16x8_t;
        fn vqshluq_n_s32<const N: i32>(a: int32x4_t) -> uint32x4_t;
        fn vqshluq_n_s64<const N: i32>(a: int64x2_t) -> uint64x2_t;
        fn vqshrn_n_s16<const N: i32>(a: int16x8_t) -> int8x8_t;
        fn vqshrn_n_s32<const N: i32>(a: int32x4_t) -> int16x4_t;
        fn vqshrn_n_s64<const N: i32>(a: int64x2_t) -> int32x2_t;
        fn vqshrn_n_u16<const N: i32>(a: uint16x8_t) -> uint8x8_t;
        fn vqshrn_n_u32<const N: i32>(a: uint32x4_t) -> uint16x4_t;
        fn vqshrn_n_u64<const N: i32>(a: uint64x2_t) -> uint32x2_t;
        fn vqshrun_n_s16<const N: i32>(a: int16x8_t) -> uint8x8_t;
        fn vqshrun_n_s32<const N: i32>(a: int32x4_t) -> uint16x4_t;
        fn vqshrun_n_s64<const N: i32>(a: int64x2_t) -> uint32x2_t;
        fn vrsqrte_f32(a: float32x2_t) -> float32x2_t;
        fn vrsqrteq_f32(a: float32x4_t) -> float32x4_t;
        fn vrsqrte_u32(a: uint32x2_t) -> uint32x2_t;
        fn vrsqrteq_u32(a: uint32x4_t) -> uint32x4_t;
        fn vrsqrts_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vrsqrtsq_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vrecpe_f32(a: float32x2_t) -> float32x2_t;
        fn vrecpeq_f32(a: float32x4_t) -> float32x4_t;
        fn vrecpe_u32(a: uint32x2_t) -> uint32x2_t;
        fn vrecpeq_u32(a: uint32x4_t) -> uint32x4_t;
        fn vrecps_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vrecpsq_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vreinterpret_s8_u8(a: uint8x8_t) -> int8x8_t;
        fn vreinterpret_s8_p8(a: poly8x8_t) -> int8x8_t;
        fn vreinterpret_s16_p16(a: poly16x4_t) -> int16x4_t;
        fn vreinterpret_s16_u16(a: uint16x4_t) -> int16x4_t;
        fn vreinterpret_s32_u32(a: uint32x2_t) -> int32x2_t;
        fn vreinterpret_s64_u64(a: uint64x1_t) -> int64x1_t;
        fn vreinterpretq_s8_u8(a: uint8x16_t) -> int8x16_t;
        fn vreinterpretq_s8_p8(a: poly8x16_t) -> int8x16_t;
        fn vreinterpretq_s16_p16(a: poly16x8_t) -> int16x8_t;
        fn vreinterpretq_s16_u16(a: uint16x8_t) -> int16x8_t;
        fn vreinterpretq_s32_u32(a: uint32x4_t) -> int32x4_t;
        fn vreinterpretq_s64_u64(a: uint64x2_t) -> int64x2_t;
        fn vreinterpret_u8_p8(a: poly8x8_t) -> uint8x8_t;
        fn vreinterpret_u8_s8(a: int8x8_t) -> uint8x8_t;
        fn vreinterpret_u16_p16(a: poly16x4_t) -> uint16x4_t;
        fn vreinterpret_u16_s16(a: int16x4_t) -> uint16x4_t;
        fn vreinterpret_u32_s32(a: int32x2_t) -> uint32x2_t;
        fn vreinterpret_u64_s64(a: int64x1_t) -> uint64x1_t;
        fn vreinterpretq_u8_p8(a: poly8x16_t) -> uint8x16_t;
        fn vreinterpretq_u8_s8(a: int8x16_t) -> uint8x16_t;
        fn vreinterpretq_u16_p16(a: poly16x8_t) -> uint16x8_t;
        fn vreinterpretq_u16_s16(a: int16x8_t) -> uint16x8_t;
        fn vreinterpretq_u32_s32(a: int32x4_t) -> uint32x4_t;
        fn vreinterpretq_u64_s64(a: int64x2_t) -> uint64x2_t;
        fn vreinterpret_p8_s8(a: int8x8_t) -> poly8x8_t;
        fn vreinterpret_p8_u8(a: uint8x8_t) -> poly8x8_t;
        fn vreinterpret_p16_s16(a: int16x4_t) -> poly16x4_t;
        fn vreinterpret_p16_u16(a: uint16x4_t) -> poly16x4_t;
        fn vreinterpretq_p8_s8(a: int8x16_t) -> poly8x16_t;
        fn vreinterpretq_p8_u8(a: uint8x16_t) -> poly8x16_t;
        fn vreinterpretq_p16_s16(a: int16x8_t) -> poly16x8_t;
        fn vreinterpretq_p16_u16(a: uint16x8_t) -> poly16x8_t;
        fn vreinterpret_s8_s16(a: int16x4_t) -> int8x8_t;
        fn vreinterpret_s8_u16(a: uint16x4_t) -> int8x8_t;
        fn vreinterpret_s8_p16(a: poly16x4_t) -> int8x8_t;
        fn vreinterpret_s16_s32(a: int32x2_t) -> int16x4_t;
        fn vreinterpret_s16_u32(a: uint32x2_t) -> int16x4_t;
        fn vreinterpret_s32_s64(a: int64x1_t) -> int32x2_t;
        fn vreinterpret_s32_u64(a: uint64x1_t) -> int32x2_t;
        fn vreinterpretq_s8_s16(a: int16x8_t) -> int8x16_t;
        fn vreinterpretq_s8_u16(a: uint16x8_t) -> int8x16_t;
        fn vreinterpretq_s8_p16(a: poly16x8_t) -> int8x16_t;
        fn vreinterpretq_s16_s32(a: int32x4_t) -> int16x8_t;
        fn vreinterpretq_s16_u32(a: uint32x4_t) -> int16x8_t;
        fn vreinterpretq_s32_s64(a: int64x2_t) -> int32x4_t;
        fn vreinterpretq_s32_u64(a: uint64x2_t) -> int32x4_t;
        fn vreinterpret_u8_p16(a: poly16x4_t) -> uint8x8_t;
        fn vreinterpret_u8_s16(a: int16x4_t) -> uint8x8_t;
        fn vreinterpret_u8_u16(a: uint16x4_t) -> uint8x8_t;
        fn vreinterpret_u16_s32(a: int32x2_t) -> uint16x4_t;
        fn vreinterpret_u16_u32(a: uint32x2_t) -> uint16x4_t;
        fn vreinterpret_u32_s64(a: int64x1_t) -> uint32x2_t;
        fn vreinterpret_u32_u64(a: uint64x1_t) -> uint32x2_t;
        fn vreinterpretq_u8_p16(a: poly16x8_t) -> uint8x16_t;
        fn vreinterpretq_u8_s16(a: int16x8_t) -> uint8x16_t;
        fn vreinterpretq_u8_u16(a: uint16x8_t) -> uint8x16_t;
        fn vreinterpretq_u16_s32(a: int32x4_t) -> uint16x8_t;
        fn vreinterpretq_u16_u32(a: uint32x4_t) -> uint16x8_t;
        fn vreinterpretq_u32_s64(a: int64x2_t) -> uint32x4_t;
        fn vreinterpretq_u32_u64(a: uint64x2_t) -> uint32x4_t;
        fn vreinterpret_p8_p16(a: poly16x4_t) -> poly8x8_t;
        fn vreinterpret_p8_s16(a: int16x4_t) -> poly8x8_t;
        fn vreinterpret_p8_u16(a: uint16x4_t) -> poly8x8_t;
        fn vreinterpret_p16_s32(a: int32x2_t) -> poly16x4_t;
        fn vreinterpret_p16_u32(a: uint32x2_t) -> poly16x4_t;
        fn vreinterpretq_p8_p16(a: poly16x8_t) -> poly8x16_t;
        fn vreinterpretq_p8_s16(a: int16x8_t) -> poly8x16_t;
        fn vreinterpretq_p8_u16(a: uint16x8_t) -> poly8x16_t;
        fn vreinterpretq_p16_s32(a: int32x4_t) -> poly16x8_t;
        fn vreinterpretq_p16_u32(a: uint32x4_t) -> poly16x8_t;
        fn vreinterpret_u32_p64(a: poly64x1_t) -> uint32x2_t;
        fn vreinterpret_s16_p8(a: poly8x8_t) -> int16x4_t;
        fn vreinterpret_s16_s8(a: int8x8_t) -> int16x4_t;
        fn vreinterpret_s16_u8(a: uint8x8_t) -> int16x4_t;
        fn vreinterpret_s32_p16(a: poly16x4_t) -> int32x2_t;
        fn vreinterpret_s32_s16(a: int16x4_t) -> int32x2_t;
        fn vreinterpret_s32_u16(a: uint16x4_t) -> int32x2_t;
        fn vreinterpret_s64_s32(a: int32x2_t) -> int64x1_t;
        fn vreinterpret_s64_u32(a: uint32x2_t) -> int64x1_t;
        fn vreinterpretq_s16_p8(a: poly8x16_t) -> int16x8_t;
        fn vreinterpretq_s16_s8(a: int8x16_t) -> int16x8_t;
        fn vreinterpretq_s16_u8(a: uint8x16_t) -> int16x8_t;
        fn vreinterpretq_s32_p16(a: poly16x8_t) -> int32x4_t;
        fn vreinterpretq_s32_s16(a: int16x8_t) -> int32x4_t;
        fn vreinterpretq_s32_u16(a: uint16x8_t) -> int32x4_t;
        fn vreinterpretq_s64_s32(a: int32x4_t) -> int64x2_t;
        fn vreinterpretq_s64_u32(a: uint32x4_t) -> int64x2_t;
        fn vreinterpret_u16_p8(a: poly8x8_t) -> uint16x4_t;
        fn vreinterpret_u16_s8(a: int8x8_t) -> uint16x4_t;
        fn vreinterpret_u16_u8(a: uint8x8_t) -> uint16x4_t;
        fn vreinterpret_u32_p16(a: poly16x4_t) -> uint32x2_t;
        fn vreinterpret_u32_s16(a: int16x4_t) -> uint32x2_t;
        fn vreinterpret_u32_u16(a: uint16x4_t) -> uint32x2_t;
        fn vreinterpret_u64_s32(a: int32x2_t) -> uint64x1_t;
        fn vreinterpret_u64_u32(a: uint32x2_t) -> uint64x1_t;
        fn vreinterpretq_u16_p8(a: poly8x16_t) -> uint16x8_t;
        fn vreinterpretq_u16_s8(a: int8x16_t) -> uint16x8_t;
        fn vreinterpretq_u16_u8(a: uint8x16_t) -> uint16x8_t;
        fn vreinterpretq_u32_p16(a: poly16x8_t) -> uint32x4_t;
        fn vreinterpretq_u32_s16(a: int16x8_t) -> uint32x4_t;
        fn vreinterpretq_u32_u16(a: uint16x8_t) -> uint32x4_t;
        fn vreinterpretq_u64_s32(a: int32x4_t) -> uint64x2_t;
        fn vreinterpretq_u64_u32(a: uint32x4_t) -> uint64x2_t;
        fn vreinterpret_p16_p8(a: poly8x8_t) -> poly16x4_t;
        fn vreinterpret_p16_s8(a: int8x8_t) -> poly16x4_t;
        fn vreinterpret_p16_u8(a: uint8x8_t) -> poly16x4_t;
        fn vreinterpretq_p16_p8(a: poly8x16_t) -> poly16x8_t;
        fn vreinterpretq_p16_s8(a: int8x16_t) -> poly16x8_t;
        fn vreinterpretq_p16_u8(a: uint8x16_t) -> poly16x8_t;
        fn vreinterpret_s8_s32(a: int32x2_t) -> int8x8_t;
        fn vreinterpret_s8_u32(a: uint32x2_t) -> int8x8_t;
        fn vreinterpret_s16_s64(a: int64x1_t) -> int16x4_t;
        fn vreinterpret_s16_u64(a: uint64x1_t) -> int16x4_t;
        fn vreinterpretq_s8_s32(a: int32x4_t) -> int8x16_t;
        fn vreinterpretq_s8_u32(a: uint32x4_t) -> int8x16_t;
        fn vreinterpretq_s16_s64(a: int64x2_t) -> int16x8_t;
        fn vreinterpretq_s16_u64(a: uint64x2_t) -> int16x8_t;
        fn vreinterpret_u8_s32(a: int32x2_t) -> uint8x8_t;
        fn vreinterpret_u8_u32(a: uint32x2_t) -> uint8x8_t;
        fn vreinterpret_u16_s64(a: int64x1_t) -> uint16x4_t;
        fn vreinterpret_u16_u64(a: uint64x1_t) -> uint16x4_t;
        fn vreinterpretq_u8_s32(a: int32x4_t) -> uint8x16_t;
        fn vreinterpretq_u8_u32(a: uint32x4_t) -> uint8x16_t;
        fn vreinterpretq_u16_s64(a: int64x2_t) -> uint16x8_t;
        fn vreinterpretq_u16_u64(a: uint64x2_t) -> uint16x8_t;
        fn vreinterpret_p8_s32(a: int32x2_t) -> poly8x8_t;
        fn vreinterpret_p8_u32(a: uint32x2_t) -> poly8x8_t;
        fn vreinterpret_p16_s64(a: int64x1_t) -> poly16x4_t;
        fn vreinterpret_p16_u64(a: uint64x1_t) -> poly16x4_t;
        fn vreinterpretq_p8_s32(a: int32x4_t) -> poly8x16_t;
        fn vreinterpretq_p8_u32(a: uint32x4_t) -> poly8x16_t;
        fn vreinterpretq_p16_s64(a: int64x2_t) -> poly16x8_t;
        fn vreinterpretq_p16_u64(a: uint64x2_t) -> poly16x8_t;
        fn vreinterpret_s32_p8(a: poly8x8_t) -> int32x2_t;
        fn vreinterpret_s32_s8(a: int8x8_t) -> int32x2_t;
        fn vreinterpret_s32_u8(a: uint8x8_t) -> int32x2_t;
        fn vreinterpret_s64_p16(a: poly16x4_t) -> int64x1_t;
        fn vreinterpret_s64_s16(a: int16x4_t) -> int64x1_t;
        fn vreinterpret_s64_u16(a: uint16x4_t) -> int64x1_t;
        fn vreinterpretq_s32_p8(a: poly8x16_t) -> int32x4_t;
        fn vreinterpretq_s32_s8(a: int8x16_t) -> int32x4_t;
        fn vreinterpretq_s32_u8(a: uint8x16_t) -> int32x4_t;
        fn vreinterpretq_s64_p16(a: poly16x8_t) -> int64x2_t;
        fn vreinterpretq_s64_s16(a: int16x8_t) -> int64x2_t;
        fn vreinterpretq_s64_u16(a: uint16x8_t) -> int64x2_t;
        fn vreinterpret_u32_p8(a: poly8x8_t) -> uint32x2_t;
        fn vreinterpret_u32_s8(a: int8x8_t) -> uint32x2_t;
        fn vreinterpret_u32_u8(a: uint8x8_t) -> uint32x2_t;
        fn vreinterpret_u64_p16(a: poly16x4_t) -> uint64x1_t;
        fn vreinterpret_u64_s16(a: int16x4_t) -> uint64x1_t;
        fn vreinterpret_u64_u16(a: uint16x4_t) -> uint64x1_t;
        fn vreinterpretq_u32_p8(a: poly8x16_t) -> uint32x4_t;
        fn vreinterpretq_u32_s8(a: int8x16_t) -> uint32x4_t;
        fn vreinterpretq_u32_u8(a: uint8x16_t) -> uint32x4_t;
        fn vreinterpretq_u64_p16(a: poly16x8_t) -> uint64x2_t;
        fn vreinterpretq_u64_s16(a: int16x8_t) -> uint64x2_t;
        fn vreinterpretq_u64_u16(a: uint16x8_t) -> uint64x2_t;
        fn vreinterpret_s8_s64(a: int64x1_t) -> int8x8_t;
        fn vreinterpret_s8_u64(a: uint64x1_t) -> int8x8_t;
        fn vreinterpret_u8_s64(a: int64x1_t) -> uint8x8_t;
        fn vreinterpret_u8_u64(a: uint64x1_t) -> uint8x8_t;
        fn vreinterpret_p8_s64(a: int64x1_t) -> poly8x8_t;
        fn vreinterpret_p8_u64(a: uint64x1_t) -> poly8x8_t;
        fn vreinterpretq_s8_s64(a: int64x2_t) -> int8x16_t;
        fn vreinterpretq_s8_u64(a: uint64x2_t) -> int8x16_t;
        fn vreinterpretq_u8_s64(a: int64x2_t) -> uint8x16_t;
        fn vreinterpretq_u8_u64(a: uint64x2_t) -> uint8x16_t;
        fn vreinterpretq_p8_s64(a: int64x2_t) -> poly8x16_t;
        fn vreinterpretq_p8_u64(a: uint64x2_t) -> poly8x16_t;
        fn vreinterpret_s64_p8(a: poly8x8_t) -> int64x1_t;
        fn vreinterpret_s64_s8(a: int8x8_t) -> int64x1_t;
        fn vreinterpret_s64_u8(a: uint8x8_t) -> int64x1_t;
        fn vreinterpret_u64_p8(a: poly8x8_t) -> uint64x1_t;
        fn vreinterpret_u64_s8(a: int8x8_t) -> uint64x1_t;
        fn vreinterpret_u64_u8(a: uint8x8_t) -> uint64x1_t;
        fn vreinterpretq_s64_p8(a: poly8x16_t) -> int64x2_t;
        fn vreinterpretq_s64_s8(a: int8x16_t) -> int64x2_t;
        fn vreinterpretq_s64_u8(a: uint8x16_t) -> int64x2_t;
        fn vreinterpretq_u64_p8(a: poly8x16_t) -> uint64x2_t;
        fn vreinterpretq_u64_s8(a: int8x16_t) -> uint64x2_t;
        fn vreinterpretq_u64_u8(a: uint8x16_t) -> uint64x2_t;
        fn vreinterpret_s8_f32(a: float32x2_t) -> int8x8_t;
        fn vreinterpret_s16_f32(a: float32x2_t) -> int16x4_t;
        fn vreinterpret_s32_f32(a: float32x2_t) -> int32x2_t;
        fn vreinterpret_s64_f32(a: float32x2_t) -> int64x1_t;
        fn vreinterpretq_s8_f32(a: float32x4_t) -> int8x16_t;
        fn vreinterpretq_s16_f32(a: float32x4_t) -> int16x8_t;
        fn vreinterpretq_s32_f32(a: float32x4_t) -> int32x4_t;
        fn vreinterpretq_s64_f32(a: float32x4_t) -> int64x2_t;
        fn vreinterpret_u8_f32(a: float32x2_t) -> uint8x8_t;
        fn vreinterpret_u16_f32(a: float32x2_t) -> uint16x4_t;
        fn vreinterpret_u32_f32(a: float32x2_t) -> uint32x2_t;
        fn vreinterpret_u64_f32(a: float32x2_t) -> uint64x1_t;
        fn vreinterpretq_u8_f32(a: float32x4_t) -> uint8x16_t;
        fn vreinterpretq_u16_f32(a: float32x4_t) -> uint16x8_t;
        fn vreinterpretq_u32_f32(a: float32x4_t) -> uint32x4_t;
        fn vreinterpretq_u64_f32(a: float32x4_t) -> uint64x2_t;
        fn vreinterpret_p8_f32(a: float32x2_t) -> poly8x8_t;
        fn vreinterpret_p16_f32(a: float32x2_t) -> poly16x4_t;
        fn vreinterpretq_p8_f32(a: float32x4_t) -> poly8x16_t;
        fn vreinterpretq_p16_f32(a: float32x4_t) -> poly16x8_t;
        fn vreinterpretq_p128_f32(a: float32x4_t) -> p128;
        fn vreinterpret_f32_s8(a: int8x8_t) -> float32x2_t;
        fn vreinterpret_f32_s16(a: int16x4_t) -> float32x2_t;
        fn vreinterpret_f32_s32(a: int32x2_t) -> float32x2_t;
        fn vreinterpret_f32_s64(a: int64x1_t) -> float32x2_t;
        fn vreinterpretq_f32_s8(a: int8x16_t) -> float32x4_t;
        fn vreinterpretq_f32_s16(a: int16x8_t) -> float32x4_t;
        fn vreinterpretq_f32_s32(a: int32x4_t) -> float32x4_t;
        fn vreinterpretq_f32_s64(a: int64x2_t) -> float32x4_t;
        fn vreinterpret_f32_u8(a: uint8x8_t) -> float32x2_t;
        fn vreinterpret_f32_u16(a: uint16x4_t) -> float32x2_t;
        fn vreinterpret_f32_u32(a: uint32x2_t) -> float32x2_t;
        fn vreinterpret_f32_u64(a: uint64x1_t) -> float32x2_t;
        fn vreinterpretq_f32_u8(a: uint8x16_t) -> float32x4_t;
        fn vreinterpretq_f32_u16(a: uint16x8_t) -> float32x4_t;
        fn vreinterpretq_f32_u32(a: uint32x4_t) -> float32x4_t;
        fn vreinterpretq_f32_u64(a: uint64x2_t) -> float32x4_t;
        fn vreinterpret_f32_p8(a: poly8x8_t) -> float32x2_t;
        fn vreinterpret_f32_p16(a: poly16x4_t) -> float32x2_t;
        fn vreinterpretq_f32_p8(a: poly8x16_t) -> float32x4_t;
        fn vreinterpretq_f32_p16(a: poly16x8_t) -> float32x4_t;
        fn vreinterpretq_f32_p128(a: p128) -> float32x4_t;
        fn vrshl_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vrshlq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vrshl_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vrshlq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vrshl_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vrshlq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vrshl_s64(a: int64x1_t, b: int64x1_t) -> int64x1_t;
        fn vrshlq_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vrshl_u8(a: uint8x8_t, b: int8x8_t) -> uint8x8_t;
        fn vrshlq_u8(a: uint8x16_t, b: int8x16_t) -> uint8x16_t;
        fn vrshl_u16(a: uint16x4_t, b: int16x4_t) -> uint16x4_t;
        fn vrshlq_u16(a: uint16x8_t, b: int16x8_t) -> uint16x8_t;
        fn vrshl_u32(a: uint32x2_t, b: int32x2_t) -> uint32x2_t;
        fn vrshlq_u32(a: uint32x4_t, b: int32x4_t) -> uint32x4_t;
        fn vrshl_u64(a: uint64x1_t, b: int64x1_t) -> uint64x1_t;
        fn vrshlq_u64(a: uint64x2_t, b: int64x2_t) -> uint64x2_t;
        fn vrshr_n_s8<const N: i32>(a: int8x8_t) -> int8x8_t;
        fn vrshrq_n_s8<const N: i32>(a: int8x16_t) -> int8x16_t;
        fn vrshr_n_s16<const N: i32>(a: int16x4_t) -> int16x4_t;
        fn vrshrq_n_s16<const N: i32>(a: int16x8_t) -> int16x8_t;
        fn vrshr_n_s32<const N: i32>(a: int32x2_t) -> int32x2_t;
        fn vrshrq_n_s32<const N: i32>(a: int32x4_t) -> int32x4_t;
        fn vrshr_n_s64<const N: i32>(a: int64x1_t) -> int64x1_t;
        fn vrshrq_n_s64<const N: i32>(a: int64x2_t) -> int64x2_t;
        fn vrshr_n_u8<const N: i32>(a: uint8x8_t) -> uint8x8_t;
        fn vrshrq_n_u8<const N: i32>(a: uint8x16_t) -> uint8x16_t;
        fn vrshr_n_u16<const N: i32>(a: uint16x4_t) -> uint16x4_t;
        fn vrshrq_n_u16<const N: i32>(a: uint16x8_t) -> uint16x8_t;
        fn vrshr_n_u32<const N: i32>(a: uint32x2_t) -> uint32x2_t;
        fn vrshrq_n_u32<const N: i32>(a: uint32x4_t) -> uint32x4_t;
        fn vrshr_n_u64<const N: i32>(a: uint64x1_t) -> uint64x1_t;
        fn vrshrq_n_u64<const N: i32>(a: uint64x2_t) -> uint64x2_t;
        fn vrshrn_n_s16<const N: i32>(a: int16x8_t) -> int8x8_t;
        fn vrshrn_n_s32<const N: i32>(a: int32x4_t) -> int16x4_t;
        fn vrshrn_n_s64<const N: i32>(a: int64x2_t) -> int32x2_t;
        fn vrshrn_n_u16<const N: i32>(a: uint16x8_t) -> uint8x8_t;
        fn vrshrn_n_u32<const N: i32>(a: uint32x4_t) -> uint16x4_t;
        fn vrshrn_n_u64<const N: i32>(a: uint64x2_t) -> uint32x2_t;
        fn vrsra_n_s8<const N: i32>(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vrsraq_n_s8<const N: i32>(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vrsra_n_s16<const N: i32>(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vrsraq_n_s16<const N: i32>(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vrsra_n_s32<const N: i32>(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vrsraq_n_s32<const N: i32>(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vrsra_n_s64<const N: i32>(a: int64x1_t, b: int64x1_t) -> int64x1_t;
        fn vrsraq_n_s64<const N: i32>(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vrsra_n_u8<const N: i32>(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vrsraq_n_u8<const N: i32>(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vrsra_n_u16<const N: i32>(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vrsraq_n_u16<const N: i32>(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vrsra_n_u32<const N: i32>(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vrsraq_n_u32<const N: i32>(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vrsra_n_u64<const N: i32>(a: uint64x1_t, b: uint64x1_t) -> uint64x1_t;
        fn vrsraq_n_u64<const N: i32>(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vrsubhn_s16(a: int16x8_t, b: int16x8_t) -> int8x8_t;
        fn vrsubhn_s32(a: int32x4_t, b: int32x4_t) -> int16x4_t;
        fn vrsubhn_s64(a: int64x2_t, b: int64x2_t) -> int32x2_t;
        fn vrsubhn_u16(a: uint16x8_t, b: uint16x8_t) -> uint8x8_t;
        fn vrsubhn_u32(a: uint32x4_t, b: uint32x4_t) -> uint16x4_t;
        fn vrsubhn_u64(a: uint64x2_t, b: uint64x2_t) -> uint32x2_t;
        fn vset_lane_s8<const LANE: i32>(a: i8, b: int8x8_t) -> int8x8_t;
        fn vset_lane_s16<const LANE: i32>(a: i16, b: int16x4_t) -> int16x4_t;
        fn vset_lane_s32<const LANE: i32>(a: i32, b: int32x2_t) -> int32x2_t;
        fn vset_lane_s64<const LANE: i32>(a: i64, b: int64x1_t) -> int64x1_t;
        fn vset_lane_u8<const LANE: i32>(a: u8, b: uint8x8_t) -> uint8x8_t;
        fn vset_lane_u16<const LANE: i32>(a: u16, b: uint16x4_t) -> uint16x4_t;
        fn vset_lane_u32<const LANE: i32>(a: u32, b: uint32x2_t) -> uint32x2_t;
        fn vset_lane_u64<const LANE: i32>(a: u64, b: uint64x1_t) -> uint64x1_t;
        fn vset_lane_p8<const LANE: i32>(a: p8, b: poly8x8_t) -> poly8x8_t;
        fn vset_lane_p16<const LANE: i32>(a: p16, b: poly16x4_t) -> poly16x4_t;
        fn vsetq_lane_s8<const LANE: i32>(a: i8, b: int8x16_t) -> int8x16_t;
        fn vsetq_lane_s16<const LANE: i32>(a: i16, b: int16x8_t) -> int16x8_t;
        fn vsetq_lane_s32<const LANE: i32>(a: i32, b: int32x4_t) -> int32x4_t;
        fn vsetq_lane_s64<const LANE: i32>(a: i64, b: int64x2_t) -> int64x2_t;
        fn vsetq_lane_u8<const LANE: i32>(a: u8, b: uint8x16_t) -> uint8x16_t;
        fn vsetq_lane_u16<const LANE: i32>(a: u16, b: uint16x8_t) -> uint16x8_t;
        fn vsetq_lane_u32<const LANE: i32>(a: u32, b: uint32x4_t) -> uint32x4_t;
        fn vsetq_lane_u64<const LANE: i32>(a: u64, b: uint64x2_t) -> uint64x2_t;
        fn vsetq_lane_p8<const LANE: i32>(a: p8, b: poly8x16_t) -> poly8x16_t;
        fn vsetq_lane_p16<const LANE: i32>(a: p16, b: poly16x8_t) -> poly16x8_t;
        fn vset_lane_f32<const LANE: i32>(a: f32, b: float32x2_t) -> float32x2_t;
        fn vsetq_lane_f32<const LANE: i32>(a: f32, b: float32x4_t) -> float32x4_t;
        fn vshl_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vshlq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vshl_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vshlq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vshl_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vshlq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vshl_s64(a: int64x1_t, b: int64x1_t) -> int64x1_t;
        fn vshlq_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vshl_u8(a: uint8x8_t, b: int8x8_t) -> uint8x8_t;
        fn vshlq_u8(a: uint8x16_t, b: int8x16_t) -> uint8x16_t;
        fn vshl_u16(a: uint16x4_t, b: int16x4_t) -> uint16x4_t;
        fn vshlq_u16(a: uint16x8_t, b: int16x8_t) -> uint16x8_t;
        fn vshl_u32(a: uint32x2_t, b: int32x2_t) -> uint32x2_t;
        fn vshlq_u32(a: uint32x4_t, b: int32x4_t) -> uint32x4_t;
        fn vshl_u64(a: uint64x1_t, b: int64x1_t) -> uint64x1_t;
        fn vshlq_u64(a: uint64x2_t, b: int64x2_t) -> uint64x2_t;
        fn vshl_n_s8<const N: i32>(a: int8x8_t) -> int8x8_t;
        fn vshlq_n_s8<const N: i32>(a: int8x16_t) -> int8x16_t;
        fn vshl_n_s16<const N: i32>(a: int16x4_t) -> int16x4_t;
        fn vshlq_n_s16<const N: i32>(a: int16x8_t) -> int16x8_t;
        fn vshl_n_s32<const N: i32>(a: int32x2_t) -> int32x2_t;
        fn vshlq_n_s32<const N: i32>(a: int32x4_t) -> int32x4_t;
        fn vshl_n_u8<const N: i32>(a: uint8x8_t) -> uint8x8_t;
        fn vshlq_n_u8<const N: i32>(a: uint8x16_t) -> uint8x16_t;
        fn vshl_n_u16<const N: i32>(a: uint16x4_t) -> uint16x4_t;
        fn vshlq_n_u16<const N: i32>(a: uint16x8_t) -> uint16x8_t;
        fn vshl_n_u32<const N: i32>(a: uint32x2_t) -> uint32x2_t;
        fn vshlq_n_u32<const N: i32>(a: uint32x4_t) -> uint32x4_t;
        fn vshl_n_s64<const N: i32>(a: int64x1_t) -> int64x1_t;
        fn vshlq_n_s64<const N: i32>(a: int64x2_t) -> int64x2_t;
        fn vshl_n_u64<const N: i32>(a: uint64x1_t) -> uint64x1_t;
        fn vshlq_n_u64<const N: i32>(a: uint64x2_t) -> uint64x2_t;
        fn vshll_n_s8<const N: i32>(a: int8x8_t) -> int16x8_t;
        fn vshll_n_s16<const N: i32>(a: int16x4_t) -> int32x4_t;
        fn vshll_n_s32<const N: i32>(a: int32x2_t) -> int64x2_t;
        fn vshll_n_u8<const N: i32>(a: uint8x8_t) -> uint16x8_t;
        fn vshll_n_u16<const N: i32>(a: uint16x4_t) -> uint32x4_t;
        fn vshll_n_u32<const N: i32>(a: uint32x2_t) -> uint64x2_t;
        fn vshr_n_s8<const N: i32>(a: int8x8_t) -> int8x8_t;
        fn vshrq_n_s8<const N: i32>(a: int8x16_t) -> int8x16_t;
        fn vshr_n_s16<const N: i32>(a: int16x4_t) -> int16x4_t;
        fn vshrq_n_s16<const N: i32>(a: int16x8_t) -> int16x8_t;
        fn vshr_n_s32<const N: i32>(a: int32x2_t) -> int32x2_t;
        fn vshrq_n_s32<const N: i32>(a: int32x4_t) -> int32x4_t;
        fn vshr_n_s64<const N: i32>(a: int64x1_t) -> int64x1_t;
        fn vshrq_n_s64<const N: i32>(a: int64x2_t) -> int64x2_t;
        fn vshr_n_u8<const N: i32>(a: uint8x8_t) -> uint8x8_t;
        fn vshrq_n_u8<const N: i32>(a: uint8x16_t) -> uint8x16_t;
        fn vshr_n_u16<const N: i32>(a: uint16x4_t) -> uint16x4_t;
        fn vshrq_n_u16<const N: i32>(a: uint16x8_t) -> uint16x8_t;
        fn vshr_n_u32<const N: i32>(a: uint32x2_t) -> uint32x2_t;
        fn vshrq_n_u32<const N: i32>(a: uint32x4_t) -> uint32x4_t;
        fn vshr_n_u64<const N: i32>(a: uint64x1_t) -> uint64x1_t;
        fn vshrq_n_u64<const N: i32>(a: uint64x2_t) -> uint64x2_t;
        fn vshrn_n_s16<const N: i32>(a: int16x8_t) -> int8x8_t;
        fn vshrn_n_s32<const N: i32>(a: int32x4_t) -> int16x4_t;
        fn vshrn_n_s64<const N: i32>(a: int64x2_t) -> int32x2_t;
        fn vshrn_n_u16<const N: i32>(a: uint16x8_t) -> uint8x8_t;
        fn vshrn_n_u32<const N: i32>(a: uint32x4_t) -> uint16x4_t;
        fn vshrn_n_u64<const N: i32>(a: uint64x2_t) -> uint32x2_t;
        fn vsra_n_s8<const N: i32>(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vsraq_n_s8<const N: i32>(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vsra_n_s16<const N: i32>(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vsraq_n_s16<const N: i32>(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vsra_n_s32<const N: i32>(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vsraq_n_s32<const N: i32>(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vsra_n_s64<const N: i32>(a: int64x1_t, b: int64x1_t) -> int64x1_t;
        fn vsraq_n_s64<const N: i32>(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vsra_n_u8<const N: i32>(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vsraq_n_u8<const N: i32>(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vsra_n_u16<const N: i32>(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vsraq_n_u16<const N: i32>(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vsra_n_u32<const N: i32>(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vsraq_n_u32<const N: i32>(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vsra_n_u64<const N: i32>(a: uint64x1_t, b: uint64x1_t) -> uint64x1_t;
        fn vsraq_n_u64<const N: i32>(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vtrn_s8(a: int8x8_t, b: int8x8_t) -> int8x8x2_t;
        fn vtrn_s16(a: int16x4_t, b: int16x4_t) -> int16x4x2_t;
        fn vtrnq_s8(a: int8x16_t, b: int8x16_t) -> int8x16x2_t;
        fn vtrnq_s16(a: int16x8_t, b: int16x8_t) -> int16x8x2_t;
        fn vtrnq_s32(a: int32x4_t, b: int32x4_t) -> int32x4x2_t;
        fn vtrn_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8x2_t;
        fn vtrn_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4x2_t;
        fn vtrnq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16x2_t;
        fn vtrnq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8x2_t;
        fn vtrnq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4x2_t;
        fn vtrn_p8(a: poly8x8_t, b: poly8x8_t) -> poly8x8x2_t;
        fn vtrn_p16(a: poly16x4_t, b: poly16x4_t) -> poly16x4x2_t;
        fn vtrnq_p8(a: poly8x16_t, b: poly8x16_t) -> poly8x16x2_t;
        fn vtrnq_p16(a: poly16x8_t, b: poly16x8_t) -> poly16x8x2_t;
        fn vtrn_s32(a: int32x2_t, b: int32x2_t) -> int32x2x2_t;
        fn vtrn_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2x2_t;
        fn vtrn_f32(a: float32x2_t, b: float32x2_t) -> float32x2x2_t;
        fn vtrnq_f32(a: float32x4_t, b: float32x4_t) -> float32x4x2_t;
        fn vzip_s8(a: int8x8_t, b: int8x8_t) -> int8x8x2_t;
        fn vzip_s16(a: int16x4_t, b: int16x4_t) -> int16x4x2_t;
        fn vzip_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8x2_t;
        fn vzip_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4x2_t;
        fn vzip_p8(a: poly8x8_t, b: poly8x8_t) -> poly8x8x2_t;
        fn vzip_p16(a: poly16x4_t, b: poly16x4_t) -> poly16x4x2_t;
        fn vzip_s32(a: int32x2_t, b: int32x2_t) -> int32x2x2_t;
        fn vzip_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2x2_t;
        fn vzipq_s8(a: int8x16_t, b: int8x16_t) -> int8x16x2_t;
        fn vzipq_s16(a: int16x8_t, b: int16x8_t) -> int16x8x2_t;
        fn vzipq_s32(a: int32x4_t, b: int32x4_t) -> int32x4x2_t;
        fn vzipq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16x2_t;
        fn vzipq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8x2_t;
        fn vzipq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4x2_t;
        fn vzipq_p8(a: poly8x16_t, b: poly8x16_t) -> poly8x16x2_t;
        fn vzipq_p16(a: poly16x8_t, b: poly16x8_t) -> poly16x8x2_t;
        fn vzip_f32(a: float32x2_t, b: float32x2_t) -> float32x2x2_t;
        fn vzipq_f32(a: float32x4_t, b: float32x4_t) -> float32x4x2_t;
        fn vuzp_s8(a: int8x8_t, b: int8x8_t) -> int8x8x2_t;
        fn vuzp_s16(a: int16x4_t, b: int16x4_t) -> int16x4x2_t;
        fn vuzpq_s8(a: int8x16_t, b: int8x16_t) -> int8x16x2_t;
        fn vuzpq_s16(a: int16x8_t, b: int16x8_t) -> int16x8x2_t;
        fn vuzpq_s32(a: int32x4_t, b: int32x4_t) -> int32x4x2_t;
        fn vuzp_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8x2_t;
        fn vuzp_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4x2_t;
        fn vuzpq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16x2_t;
        fn vuzpq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8x2_t;
        fn vuzpq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4x2_t;
        fn vuzp_p8(a: poly8x8_t, b: poly8x8_t) -> poly8x8x2_t;
        fn vuzp_p16(a: poly16x4_t, b: poly16x4_t) -> poly16x4x2_t;
        fn vuzpq_p8(a: poly8x16_t, b: poly8x16_t) -> poly8x16x2_t;
        fn vuzpq_p16(a: poly16x8_t, b: poly16x8_t) -> poly16x8x2_t;
        fn vuzp_s32(a: int32x2_t, b: int32x2_t) -> int32x2x2_t;
        fn vuzp_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2x2_t;
        fn vuzp_f32(a: float32x2_t, b: float32x2_t) -> float32x2x2_t;
        fn vuzpq_f32(a: float32x4_t, b: float32x4_t) -> float32x4x2_t;
        fn vabal_u8(a: uint16x8_t, b: uint8x8_t, c: uint8x8_t) -> uint16x8_t;
        fn vabal_u16(a: uint32x4_t, b: uint16x4_t, c: uint16x4_t) -> uint32x4_t;
        fn vabal_u32(a: uint64x2_t, b: uint32x2_t, c: uint32x2_t) -> uint64x2_t;
        fn vabal_s8(a: int16x8_t, b: int8x8_t, c: int8x8_t) -> int16x8_t;
        fn vabal_s16(a: int32x4_t, b: int16x4_t, c: int16x4_t) -> int32x4_t;
        fn vabal_s32(a: int64x2_t, b: int32x2_t, c: int32x2_t) -> int64x2_t;
        fn vqabs_s8(a: int8x8_t) -> int8x8_t;
        fn vqabsq_s8(a: int8x16_t) -> int8x16_t;
        fn vqabs_s16(a: int16x4_t) -> int16x4_t;
        fn vqabsq_s16(a: int16x8_t) -> int16x8_t;
        fn vqabs_s32(a: int32x2_t) -> int32x2_t;
        fn vqabsq_s32(a: int32x4_t) -> int32x4_t;
    }

    delegate! {
        unsafe fn vld1_lane_s8<const LANE: i32>(ptr: *const i8, src: int8x8_t) -> int8x8_t;
        unsafe fn vld1q_lane_s8<const LANE: i32>(ptr: *const i8, src: int8x16_t) -> int8x16_t;
        unsafe fn vld1_lane_s16<const LANE: i32>(ptr: *const i16, src: int16x4_t) -> int16x4_t;
        unsafe fn vld1q_lane_s16<const LANE: i32>(ptr: *const i16, src: int16x8_t) -> int16x8_t;
        unsafe fn vld1_lane_s32<const LANE: i32>(ptr: *const i32, src: int32x2_t) -> int32x2_t;
        unsafe fn vld1q_lane_s32<const LANE: i32>(ptr: *const i32, src: int32x4_t) -> int32x4_t;
        unsafe fn vld1_lane_s64<const LANE: i32>(ptr: *const i64, src: int64x1_t) -> int64x1_t;
        unsafe fn vld1q_lane_s64<const LANE: i32>(ptr: *const i64, src: int64x2_t) -> int64x2_t;
        unsafe fn vld1_lane_u8<const LANE: i32>(ptr: *const u8, src: uint8x8_t) -> uint8x8_t;
        unsafe fn vld1q_lane_u8<const LANE: i32>(ptr: *const u8, src: uint8x16_t) -> uint8x16_t;
        unsafe fn vld1_lane_u16<const LANE: i32>(ptr: *const u16, src: uint16x4_t) -> uint16x4_t;
        unsafe fn vld1q_lane_u16<const LANE: i32>(ptr: *const u16, src: uint16x8_t) -> uint16x8_t;
        unsafe fn vld1_lane_u32<const LANE: i32>(ptr: *const u32, src: uint32x2_t) -> uint32x2_t;
        unsafe fn vld1q_lane_u32<const LANE: i32>(ptr: *const u32, src: uint32x4_t) -> uint32x4_t;
        unsafe fn vld1_lane_u64<const LANE: i32>(ptr: *const u64, src: uint64x1_t) -> uint64x1_t;
        unsafe fn vld1q_lane_u64<const LANE: i32>(ptr: *const u64, src: uint64x2_t) -> uint64x2_t;
        unsafe fn vld1_lane_p8<const LANE: i32>(ptr: *const p8, src: poly8x8_t) -> poly8x8_t;
        unsafe fn vld1q_lane_p8<const LANE: i32>(ptr: *const p8, src: poly8x16_t) -> poly8x16_t;
        unsafe fn vld1_lane_p16<const LANE: i32>(ptr: *const p16, src: poly16x4_t) -> poly16x4_t;
        unsafe fn vld1q_lane_p16<const LANE: i32>(ptr: *const p16, src: poly16x8_t) -> poly16x8_t;
        unsafe fn vld1_lane_f32<const LANE: i32>(ptr: *const f32, src: float32x2_t) -> float32x2_t;
        unsafe fn vld1q_lane_f32<const LANE: i32>(ptr: *const f32, src: float32x4_t) -> float32x4_t;
        unsafe fn vld1_dup_s8(ptr: *const i8) -> int8x8_t;
        unsafe fn vld1q_dup_s8(ptr: *const i8) -> int8x16_t;
        unsafe fn vld1_dup_s16(ptr: *const i16) -> int16x4_t;
        unsafe fn vld1q_dup_s16(ptr: *const i16) -> int16x8_t;
        unsafe fn vld1_dup_s32(ptr: *const i32) -> int32x2_t;
        unsafe fn vld1q_dup_s32(ptr: *const i32) -> int32x4_t;
        unsafe fn vld1_dup_s64(ptr: *const i64) -> int64x1_t;
        unsafe fn vld1q_dup_s64(ptr: *const i64) -> int64x2_t;
        unsafe fn vld1_dup_u8(ptr: *const u8) -> uint8x8_t;
        unsafe fn vld1q_dup_u8(ptr: *const u8) -> uint8x16_t;
        unsafe fn vld1_dup_u16(ptr: *const u16) -> uint16x4_t;
        unsafe fn vld1q_dup_u16(ptr: *const u16) -> uint16x8_t;
        unsafe fn vld1_dup_u32(ptr: *const u32) -> uint32x2_t;
        unsafe fn vld1q_dup_u32(ptr: *const u32) -> uint32x4_t;
        unsafe fn vld1_dup_u64(ptr: *const u64) -> uint64x1_t;
        unsafe fn vld1q_dup_u64(ptr: *const u64) -> uint64x2_t;
        unsafe fn vld1_dup_p8(ptr: *const p8) -> poly8x8_t;
        unsafe fn vld1q_dup_p8(ptr: *const p8) -> poly8x16_t;
        unsafe fn vld1_dup_p16(ptr: *const p16) -> poly16x4_t;
        unsafe fn vld1q_dup_p16(ptr: *const p16) -> poly16x8_t;
        unsafe fn vld1_dup_f32(ptr: *const f32) -> float32x2_t;
        unsafe fn vld1q_dup_f32(ptr: *const f32) -> float32x4_t;
        fn vaba_s8(a: int8x8_t, b: int8x8_t, c: int8x8_t) -> int8x8_t;
        fn vaba_s16(a: int16x4_t, b: int16x4_t, c: int16x4_t) -> int16x4_t;
        fn vaba_s32(a: int32x2_t, b: int32x2_t, c: int32x2_t) -> int32x2_t;
        fn vaba_u8(a: uint8x8_t, b: uint8x8_t, c: uint8x8_t) -> uint8x8_t;
        fn vaba_u16(a: uint16x4_t, b: uint16x4_t, c: uint16x4_t) -> uint16x4_t;
        fn vaba_u32(a: uint32x2_t, b: uint32x2_t, c: uint32x2_t) -> uint32x2_t;
        fn vabaq_s8(a: int8x16_t, b: int8x16_t, c: int8x16_t) -> int8x16_t;
        fn vabaq_s16(a: int16x8_t, b: int16x8_t, c: int16x8_t) -> int16x8_t;
        fn vabaq_s32(a: int32x4_t, b: int32x4_t, c: int32x4_t) -> int32x4_t;
        fn vabaq_u8(a: uint8x16_t, b: uint8x16_t, c: uint8x16_t) -> uint8x16_t;
        fn vabaq_u16(a: uint16x8_t, b: uint16x8_t, c: uint16x8_t) -> uint16x8_t;
        fn vabaq_u32(a: uint32x4_t, b: uint32x4_t, c: uint32x4_t) -> uint32x4_t;
        fn vabs_s8(a: int8x8_t) -> int8x8_t;
        fn vabs_s16(a: int16x4_t) -> int16x4_t;
        fn vabs_s32(a: int32x2_t) -> int32x2_t;
        fn vabsq_s8(a: int8x16_t) -> int8x16_t;
        fn vabsq_s16(a: int16x8_t) -> int16x8_t;
        fn vabsq_s32(a: int32x4_t) -> int32x4_t;
        fn vpadd_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vpadd_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vpadd_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vpadd_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vpadd_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vpadd_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vadd_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vaddq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vadd_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vaddq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vadd_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vaddq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vaddq_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vadd_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vaddq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vadd_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vaddq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vadd_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vaddq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vaddq_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vadd_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vaddq_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vaddl_s8(a: int8x8_t, b: int8x8_t) -> int16x8_t;
        fn vaddl_s16(a: int16x4_t, b: int16x4_t) -> int32x4_t;
        fn vaddl_s32(a: int32x2_t, b: int32x2_t) -> int64x2_t;
        fn vaddl_u8(a: uint8x8_t, b: uint8x8_t) -> uint16x8_t;
        fn vaddl_u16(a: uint16x4_t, b: uint16x4_t) -> uint32x4_t;
        fn vaddl_u32(a: uint32x2_t, b: uint32x2_t) -> uint64x2_t;
        fn vaddl_high_s8(a: int8x16_t, b: int8x16_t) -> int16x8_t;
        fn vaddl_high_s16(a: int16x8_t, b: int16x8_t) -> int32x4_t;
        fn vaddl_high_s32(a: int32x4_t, b: int32x4_t) -> int64x2_t;
        fn vaddl_high_u8(a: uint8x16_t, b: uint8x16_t) -> uint16x8_t;
        fn vaddl_high_u16(a: uint16x8_t, b: uint16x8_t) -> uint32x4_t;
        fn vaddl_high_u32(a: uint32x4_t, b: uint32x4_t) -> uint64x2_t;
        fn vaddw_s8(a: int16x8_t, b: int8x8_t) -> int16x8_t;
        fn vaddw_s16(a: int32x4_t, b: int16x4_t) -> int32x4_t;
        fn vaddw_s32(a: int64x2_t, b: int32x2_t) -> int64x2_t;
        fn vaddw_u8(a: uint16x8_t, b: uint8x8_t) -> uint16x8_t;
        fn vaddw_u16(a: uint32x4_t, b: uint16x4_t) -> uint32x4_t;
        fn vaddw_u32(a: uint64x2_t, b: uint32x2_t) -> uint64x2_t;
        fn vaddw_high_s8(a: int16x8_t, b: int8x16_t) -> int16x8_t;
        fn vaddw_high_s16(a: int32x4_t, b: int16x8_t) -> int32x4_t;
        fn vaddw_high_s32(a: int64x2_t, b: int32x4_t) -> int64x2_t;
        fn vaddw_high_u8(a: uint16x8_t, b: uint8x16_t) -> uint16x8_t;
        fn vaddw_high_u16(a: uint32x4_t, b: uint16x8_t) -> uint32x4_t;
        fn vaddw_high_u32(a: uint64x2_t, b: uint32x4_t) -> uint64x2_t;
        fn vaddhn_s16(a: int16x8_t, b: int16x8_t) -> int8x8_t;
        fn vaddhn_s32(a: int32x4_t, b: int32x4_t) -> int16x4_t;
        fn vaddhn_s64(a: int64x2_t, b: int64x2_t) -> int32x2_t;
        fn vaddhn_u16(a: uint16x8_t, b: uint16x8_t) -> uint8x8_t;
        fn vaddhn_u32(a: uint32x4_t, b: uint32x4_t) -> uint16x4_t;
        fn vaddhn_u64(a: uint64x2_t, b: uint64x2_t) -> uint32x2_t;
        fn vaddhn_high_s16(r: int8x8_t, a: int16x8_t, b: int16x8_t) -> int8x16_t;
        fn vaddhn_high_s32(r: int16x4_t, a: int32x4_t, b: int32x4_t) -> int16x8_t;
        fn vaddhn_high_s64(r: int32x2_t, a: int64x2_t, b: int64x2_t) -> int32x4_t;
        fn vaddhn_high_u16(r: uint8x8_t, a: uint16x8_t, b: uint16x8_t) -> uint8x16_t;
        fn vaddhn_high_u32(r: uint16x4_t, a: uint32x4_t, b: uint32x4_t) -> uint16x8_t;
        fn vaddhn_high_u64(r: uint32x2_t, a: uint64x2_t, b: uint64x2_t) -> uint32x4_t;
        fn vraddhn_s16(a: int16x8_t, b: int16x8_t) -> int8x8_t;
        fn vraddhn_s32(a: int32x4_t, b: int32x4_t) -> int16x4_t;
        fn vraddhn_s64(a: int64x2_t, b: int64x2_t) -> int32x2_t;
        fn vraddhn_u16(a: uint16x8_t, b: uint16x8_t) -> uint8x8_t;
        fn vraddhn_u32(a: uint32x4_t, b: uint32x4_t) -> uint16x4_t;
        fn vraddhn_u64(a: uint64x2_t, b: uint64x2_t) -> uint32x2_t;
        fn vraddhn_high_s16(r: int8x8_t, a: int16x8_t, b: int16x8_t) -> int8x16_t;
        fn vraddhn_high_s32(r: int16x4_t, a: int32x4_t, b: int32x4_t) -> int16x8_t;
        fn vraddhn_high_s64(r: int32x2_t, a: int64x2_t, b: int64x2_t) -> int32x4_t;
        fn vraddhn_high_u16(r: uint8x8_t, a: uint16x8_t, b: uint16x8_t) -> uint8x16_t;
        fn vraddhn_high_u32(r: uint16x4_t, a: uint32x4_t, b: uint32x4_t) -> uint16x8_t;
        fn vraddhn_high_u64(r: uint32x2_t, a: uint64x2_t, b: uint64x2_t) -> uint32x4_t;
        fn vpaddl_s8(a: int8x8_t) -> int16x4_t;
        fn vpaddl_s16(a: int16x4_t) -> int32x2_t;
        fn vpaddl_s32(a: int32x2_t) -> int64x1_t;
        fn vpaddlq_s8(a: int8x16_t) -> int16x8_t;
        fn vpaddlq_s16(a: int16x8_t) -> int32x4_t;
        fn vpaddlq_s32(a: int32x4_t) -> int64x2_t;
        fn vpaddl_u8(a: uint8x8_t) -> uint16x4_t;
        fn vpaddl_u16(a: uint16x4_t) -> uint32x2_t;
        fn vpaddl_u32(a: uint32x2_t) -> uint64x1_t;
        fn vpaddlq_u8(a: uint8x16_t) -> uint16x8_t;
        fn vpaddlq_u16(a: uint16x8_t) -> uint32x4_t;
        fn vpaddlq_u32(a: uint32x4_t) -> uint64x2_t;
        fn vmovn_s16(a: int16x8_t) -> int8x8_t;
        fn vmovn_s32(a: int32x4_t) -> int16x4_t;
        fn vmovn_s64(a: int64x2_t) -> int32x2_t;
        fn vmovn_u16(a: uint16x8_t) -> uint8x8_t;
        fn vmovn_u32(a: uint32x4_t) -> uint16x4_t;
        fn vmovn_u64(a: uint64x2_t) -> uint32x2_t;
        fn vmovl_s8(a: int8x8_t) -> int16x8_t;
        fn vmovl_s16(a: int16x4_t) -> int32x4_t;
        fn vmovl_s32(a: int32x2_t) -> int64x2_t;
        fn vmovl_u8(a: uint8x8_t) -> uint16x8_t;
        fn vmovl_u16(a: uint16x4_t) -> uint32x4_t;
        fn vmovl_u32(a: uint32x2_t) -> uint64x2_t;
        fn vmvn_s8(a: int8x8_t) -> int8x8_t;
        fn vmvnq_s8(a: int8x16_t) -> int8x16_t;
        fn vmvn_s16(a: int16x4_t) -> int16x4_t;
        fn vmvnq_s16(a: int16x8_t) -> int16x8_t;
        fn vmvn_s32(a: int32x2_t) -> int32x2_t;
        fn vmvnq_s32(a: int32x4_t) -> int32x4_t;
        fn vmvn_u8(a: uint8x8_t) -> uint8x8_t;
        fn vmvnq_u8(a: uint8x16_t) -> uint8x16_t;
        fn vmvn_u16(a: uint16x4_t) -> uint16x4_t;
        fn vmvnq_u16(a: uint16x8_t) -> uint16x8_t;
        fn vmvn_u32(a: uint32x2_t) -> uint32x2_t;
        fn vmvnq_u32(a: uint32x4_t) -> uint32x4_t;
        fn vmvn_p8(a: poly8x8_t) -> poly8x8_t;
        fn vmvnq_p8(a: poly8x16_t) -> poly8x16_t;
        fn vbic_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vbicq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vbic_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vbicq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vbic_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vbicq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vbic_s64(a: int64x1_t, b: int64x1_t) -> int64x1_t;
        fn vbicq_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vbic_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vbicq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vbic_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vbicq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vbic_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vbicq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vbic_u64(a: uint64x1_t, b: uint64x1_t) -> uint64x1_t;
        fn vbicq_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vbsl_s8(a: uint8x8_t, b: int8x8_t, c: int8x8_t) -> int8x8_t;
        fn vbsl_s16(a: uint16x4_t, b: int16x4_t, c: int16x4_t) -> int16x4_t;
        fn vbsl_s32(a: uint32x2_t, b: int32x2_t, c: int32x2_t) -> int32x2_t;
        fn vbsl_s64(a: uint64x1_t, b: int64x1_t, c: int64x1_t) -> int64x1_t;
        fn vbsl_u8(a: uint8x8_t, b: uint8x8_t, c: uint8x8_t) -> uint8x8_t;
        fn vbsl_u16(a: uint16x4_t, b: uint16x4_t, c: uint16x4_t) -> uint16x4_t;
        fn vbsl_u32(a: uint32x2_t, b: uint32x2_t, c: uint32x2_t) -> uint32x2_t;
        fn vbsl_u64(a: uint64x1_t, b: uint64x1_t, c: uint64x1_t) -> uint64x1_t;
        fn vbsl_f32(a: uint32x2_t, b: float32x2_t, c: float32x2_t) -> float32x2_t;
        fn vbsl_p8(a: uint8x8_t, b: poly8x8_t, c: poly8x8_t) -> poly8x8_t;
        fn vbsl_p16(a: uint16x4_t, b: poly16x4_t, c: poly16x4_t) -> poly16x4_t;
        fn vbslq_s8(a: uint8x16_t, b: int8x16_t, c: int8x16_t) -> int8x16_t;
        fn vbslq_s16(a: uint16x8_t, b: int16x8_t, c: int16x8_t) -> int16x8_t;
        fn vbslq_s32(a: uint32x4_t, b: int32x4_t, c: int32x4_t) -> int32x4_t;
        fn vbslq_s64(a: uint64x2_t, b: int64x2_t, c: int64x2_t) -> int64x2_t;
        fn vbslq_u8(a: uint8x16_t, b: uint8x16_t, c: uint8x16_t) -> uint8x16_t;
        fn vbslq_u16(a: uint16x8_t, b: uint16x8_t, c: uint16x8_t) -> uint16x8_t;
        fn vbslq_u32(a: uint32x4_t, b: uint32x4_t, c: uint32x4_t) -> uint32x4_t;
        fn vbslq_u64(a: uint64x2_t, b: uint64x2_t, c: uint64x2_t) -> uint64x2_t;
        fn vbslq_p8(a: uint8x16_t, b: poly8x16_t, c: poly8x16_t) -> poly8x16_t;
        fn vbslq_p16(a: uint16x8_t, b: poly16x8_t, c: poly16x8_t) -> poly16x8_t;
        fn vbslq_f32(a: uint32x4_t, b: float32x4_t, c: float32x4_t) -> float32x4_t;
        fn vorn_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vornq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vorn_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vornq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vorn_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vornq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vorn_s64(a: int64x1_t, b: int64x1_t) -> int64x1_t;
        fn vornq_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vorn_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vornq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vorn_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vornq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vorn_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vornq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vorn_u64(a: uint64x1_t, b: uint64x1_t) -> uint64x1_t;
        fn vornq_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vpmin_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vpmin_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vpmin_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vpmin_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vpmin_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vpmin_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vpmin_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vpmax_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vpmax_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vpmax_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vpmax_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vpmax_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vpmax_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vpmax_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vgetq_lane_u64<const IMM5: i32>(v: uint64x2_t) -> u64;
        fn vget_lane_u64<const IMM5: i32>(v: uint64x1_t) -> u64;
        fn vget_lane_u16<const IMM5: i32>(v: uint16x4_t) -> u16;
        fn vget_lane_s16<const IMM5: i32>(v: int16x4_t) -> i16;
        fn vget_lane_p16<const IMM5: i32>(v: poly16x4_t) -> p16;
        fn vget_lane_u32<const IMM5: i32>(v: uint32x2_t) -> u32;
        fn vget_lane_s32<const IMM5: i32>(v: int32x2_t) -> i32;
        fn vget_lane_f32<const IMM5: i32>(v: float32x2_t) -> f32;
        fn vgetq_lane_f32<const IMM5: i32>(v: float32x4_t) -> f32;
        fn vget_lane_p64<const IMM5: i32>(v: poly64x1_t) -> p64;
        fn vgetq_lane_p64<const IMM5: i32>(v: poly64x2_t) -> p64;
        fn vget_lane_s64<const IMM5: i32>(v: int64x1_t) -> i64;
        fn vgetq_lane_s64<const IMM5: i32>(v: int64x2_t) -> i64;
        fn vgetq_lane_u16<const IMM5: i32>(v: uint16x8_t) -> u16;
        fn vgetq_lane_u32<const IMM5: i32>(v: uint32x4_t) -> u32;
        fn vgetq_lane_s16<const IMM5: i32>(v: int16x8_t) -> i16;
        fn vgetq_lane_p16<const IMM5: i32>(v: poly16x8_t) -> p16;
        fn vgetq_lane_s32<const IMM5: i32>(v: int32x4_t) -> i32;
        fn vget_lane_u8<const IMM5: i32>(v: uint8x8_t) -> u8;
        fn vget_lane_s8<const IMM5: i32>(v: int8x8_t) -> i8;
        fn vget_lane_p8<const IMM5: i32>(v: poly8x8_t) -> p8;
        fn vgetq_lane_u8<const IMM5: i32>(v: uint8x16_t) -> u8;
        fn vgetq_lane_s8<const IMM5: i32>(v: int8x16_t) -> i8;
        fn vgetq_lane_p8<const IMM5: i32>(v: poly8x16_t) -> p8;
        fn vget_high_s8(a: int8x16_t) -> int8x8_t;
        fn vget_high_s16(a: int16x8_t) -> int16x4_t;
        fn vget_high_s32(a: int32x4_t) -> int32x2_t;
        fn vget_high_s64(a: int64x2_t) -> int64x1_t;
        fn vget_high_u8(a: uint8x16_t) -> uint8x8_t;
        fn vget_high_u16(a: uint16x8_t) -> uint16x4_t;
        fn vget_high_u32(a: uint32x4_t) -> uint32x2_t;
        fn vget_high_u64(a: uint64x2_t) -> uint64x1_t;
        fn vget_high_p8(a: poly8x16_t) -> poly8x8_t;
        fn vget_high_p16(a: poly16x8_t) -> poly16x4_t;
        fn vget_high_f32(a: float32x4_t) -> float32x2_t;
        fn vget_low_s8(a: int8x16_t) -> int8x8_t;
        fn vget_low_s16(a: int16x8_t) -> int16x4_t;
        fn vget_low_s32(a: int32x4_t) -> int32x2_t;
        fn vget_low_s64(a: int64x2_t) -> int64x1_t;
        fn vget_low_u8(a: uint8x16_t) -> uint8x8_t;
        fn vget_low_u16(a: uint16x8_t) -> uint16x4_t;
        fn vget_low_u32(a: uint32x4_t) -> uint32x2_t;
        fn vget_low_u64(a: uint64x2_t) -> uint64x1_t;
        fn vget_low_p8(a: poly8x16_t) -> poly8x8_t;
        fn vget_low_p16(a: poly16x8_t) -> poly16x4_t;
        fn vget_low_f32(a: float32x4_t) -> float32x2_t;
        fn vdupq_n_s8(value: i8) -> int8x16_t;
        fn vdupq_n_s16(value: i16) -> int16x8_t;
        fn vdupq_n_s32(value: i32) -> int32x4_t;
        fn vdupq_n_s64(value: i64) -> int64x2_t;
        fn vdupq_n_u8(value: u8) -> uint8x16_t;
        fn vdupq_n_u16(value: u16) -> uint16x8_t;
        fn vdupq_n_u32(value: u32) -> uint32x4_t;
        fn vdupq_n_u64(value: u64) -> uint64x2_t;
        fn vdupq_n_p8(value: p8) -> poly8x16_t;
        fn vdupq_n_p16(value: p16) -> poly16x8_t;
        fn vdupq_n_f32(value: f32) -> float32x4_t;
        fn vdup_n_s8(value: i8) -> int8x8_t;
        fn vdup_n_s16(value: i16) -> int16x4_t;
        fn vdup_n_s32(value: i32) -> int32x2_t;
        fn vdup_n_s64(value: i64) -> int64x1_t;
        fn vdup_n_u8(value: u8) -> uint8x8_t;
        fn vdup_n_u16(value: u16) -> uint16x4_t;
        fn vdup_n_u32(value: u32) -> uint32x2_t;
        fn vdup_n_u64(value: u64) -> uint64x1_t;
        fn vdup_n_p8(value: p8) -> poly8x8_t;
        fn vdup_n_p16(value: p16) -> poly16x4_t;
        fn vdup_n_f32(value: f32) -> float32x2_t;
        fn vldrq_p128(a: *const p128) -> p128;
        fn vstrq_p128(a: *mut p128, b: p128);
        fn vmov_n_s8(value: i8) -> int8x8_t;
        fn vmov_n_s16(value: i16) -> int16x4_t;
        fn vmov_n_s32(value: i32) -> int32x2_t;
        fn vmov_n_s64(value: i64) -> int64x1_t;
        fn vmov_n_u8(value: u8) -> uint8x8_t;
        fn vmov_n_u16(value: u16) -> uint16x4_t;
        fn vmov_n_u32(value: u32) -> uint32x2_t;
        fn vmov_n_u64(value: u64) -> uint64x1_t;
        fn vmov_n_p8(value: p8) -> poly8x8_t;
        fn vmov_n_p16(value: p16) -> poly16x4_t;
        fn vmov_n_f32(value: f32) -> float32x2_t;
        fn vmovq_n_s8(value: i8) -> int8x16_t;
        fn vmovq_n_s16(value: i16) -> int16x8_t;
        fn vmovq_n_s32(value: i32) -> int32x4_t;
        fn vmovq_n_s64(value: i64) -> int64x2_t;
        fn vmovq_n_u8(value: u8) -> uint8x16_t;
        fn vmovq_n_u16(value: u16) -> uint16x8_t;
        fn vmovq_n_u32(value: u32) -> uint32x4_t;
        fn vmovq_n_u64(value: u64) -> uint64x2_t;
        fn vmovq_n_p8(value: p8) -> poly8x16_t;
        fn vmovq_n_p16(value: p16) -> poly16x8_t;
        fn vmovq_n_f32(value: f32) -> float32x4_t;
        fn vext_s64<const N: i32>(a: int64x1_t, _b: int64x1_t) -> int64x1_t;
        fn vext_u64<const N: i32>(a: uint64x1_t, _b: uint64x1_t) -> uint64x1_t;
        fn vcnt_s8(a: int8x8_t) -> int8x8_t;
        fn vcntq_s8(a: int8x16_t) -> int8x16_t;
        fn vcnt_u8(a: uint8x8_t) -> uint8x8_t;
        fn vcntq_u8(a: uint8x16_t) -> uint8x16_t;
        fn vcnt_p8(a: poly8x8_t) -> poly8x8_t;
        fn vcntq_p8(a: poly8x16_t) -> poly8x16_t;
        fn vrev16_s8(a: int8x8_t) -> int8x8_t;
        fn vrev16q_s8(a: int8x16_t) -> int8x16_t;
        fn vrev16_u8(a: uint8x8_t) -> uint8x8_t;
        fn vrev16q_u8(a: uint8x16_t) -> uint8x16_t;
        fn vrev16_p8(a: poly8x8_t) -> poly8x8_t;
        fn vrev16q_p8(a: poly8x16_t) -> poly8x16_t;
        fn vrev32_s8(a: int8x8_t) -> int8x8_t;
        fn vrev32q_s8(a: int8x16_t) -> int8x16_t;
        fn vrev32_u8(a: uint8x8_t) -> uint8x8_t;
        fn vrev32q_u8(a: uint8x16_t) -> uint8x16_t;
        fn vrev32_s16(a: int16x4_t) -> int16x4_t;
        fn vrev32q_s16(a: int16x8_t) -> int16x8_t;
        fn vrev32_p16(a: poly16x4_t) -> poly16x4_t;
        fn vrev32q_p16(a: poly16x8_t) -> poly16x8_t;
        fn vrev32_u16(a: uint16x4_t) -> uint16x4_t;
        fn vrev32q_u16(a: uint16x8_t) -> uint16x8_t;
        fn vrev32_p8(a: poly8x8_t) -> poly8x8_t;
        fn vrev32q_p8(a: poly8x16_t) -> poly8x16_t;
        fn vrev64_s8(a: int8x8_t) -> int8x8_t;
        fn vrev64q_s8(a: int8x16_t) -> int8x16_t;
        fn vrev64_s16(a: int16x4_t) -> int16x4_t;
        fn vrev64q_s16(a: int16x8_t) -> int16x8_t;
        fn vrev64_s32(a: int32x2_t) -> int32x2_t;
        fn vrev64q_s32(a: int32x4_t) -> int32x4_t;
        fn vrev64_u8(a: uint8x8_t) -> uint8x8_t;
        fn vrev64q_u8(a: uint8x16_t) -> uint8x16_t;
        fn vrev64_u16(a: uint16x4_t) -> uint16x4_t;
        fn vrev64q_u16(a: uint16x8_t) -> uint16x8_t;
        fn vrev64_u32(a: uint32x2_t) -> uint32x2_t;
        fn vrev64q_u32(a: uint32x4_t) -> uint32x4_t;
        fn vrev64_f32(a: float32x2_t) -> float32x2_t;
        fn vrev64q_f32(a: float32x4_t) -> float32x4_t;
        fn vrev64_p8(a: poly8x8_t) -> poly8x8_t;
        fn vrev64q_p8(a: poly8x16_t) -> poly8x16_t;
        fn vrev64_p16(a: poly16x4_t) -> poly16x4_t;
        fn vrev64q_p16(a: poly16x8_t) -> poly16x8_t;
        fn vpadal_s8(a: int16x4_t, b: int8x8_t) -> int16x4_t;
        fn vpadal_s16(a: int32x2_t, b: int16x4_t) -> int32x2_t;
        fn vpadal_s32(a: int64x1_t, b: int32x2_t) -> int64x1_t;
        fn vpadalq_s8(a: int16x8_t, b: int8x16_t) -> int16x8_t;
        fn vpadalq_s16(a: int32x4_t, b: int16x8_t) -> int32x4_t;
        fn vpadalq_s32(a: int64x2_t, b: int32x4_t) -> int64x2_t;
        fn vpadal_u8(a: uint16x4_t, b: uint8x8_t) -> uint16x4_t;
        fn vpadal_u16(a: uint32x2_t, b: uint16x4_t) -> uint32x2_t;
        fn vpadal_u32(a: uint64x1_t, b: uint32x2_t) -> uint64x1_t;
        fn vpadalq_u8(a: uint16x8_t, b: uint8x16_t) -> uint16x8_t;
        fn vpadalq_u16(a: uint32x4_t, b: uint16x8_t) -> uint32x4_t;
        fn vpadalq_u32(a: uint64x2_t, b: uint32x4_t) -> uint64x2_t;
        fn vcombine_f32(low: float32x2_t, high: float32x2_t) -> float32x4_t;
        fn vcombine_p8(low: poly8x8_t, high: poly8x8_t) -> poly8x16_t;
        fn vcombine_p16(low: poly16x4_t, high: poly16x4_t) -> poly16x8_t;
        fn vcombine_s8(low: int8x8_t, high: int8x8_t) -> int8x16_t;
        fn vcombine_s16(low: int16x4_t, high: int16x4_t) -> int16x8_t;
        fn vcombine_s32(low: int32x2_t, high: int32x2_t) -> int32x4_t;
        fn vcombine_s64(low: int64x1_t, high: int64x1_t) -> int64x2_t;
        fn vcombine_u8(low: uint8x8_t, high: uint8x8_t) -> uint8x16_t;
        fn vcombine_u16(low: uint16x4_t, high: uint16x4_t) -> uint16x8_t;
        fn vcombine_u32(low: uint32x2_t, high: uint32x2_t) -> uint32x4_t;
        fn vcombine_u64(low: uint64x1_t, high: uint64x1_t) -> uint64x2_t;
        fn vcombine_p64(low: poly64x1_t, high: poly64x1_t) -> poly64x2_t;
    }

    delegate! {
        fn vabd_f64(a: float64x1_t, b: float64x1_t) -> float64x1_t;
        fn vabdq_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vabds_f32(a: f32, b: f32) -> f32;
        fn vabdd_f64(a: f64, b: f64) -> f64;
        fn vabdl_high_u8(a: uint8x16_t, b: uint8x16_t) -> uint16x8_t;
        fn vabdl_high_u16(a: uint16x8_t, b: uint16x8_t) -> uint32x4_t;
        fn vabdl_high_u32(a: uint32x4_t, b: uint32x4_t) -> uint64x2_t;
        fn vabdl_high_s8(a: int8x16_t, b: int8x16_t) -> int16x8_t;
        fn vabdl_high_s16(a: int16x8_t, b: int16x8_t) -> int32x4_t;
        fn vabdl_high_s32(a: int32x4_t, b: int32x4_t) -> int64x2_t;
        fn vceq_u64(a: uint64x1_t, b: uint64x1_t) -> uint64x1_t;
        fn vceqq_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vceq_s64(a: int64x1_t, b: int64x1_t) -> uint64x1_t;
        fn vceqq_s64(a: int64x2_t, b: int64x2_t) -> uint64x2_t;
        fn vceq_p64(a: poly64x1_t, b: poly64x1_t) -> uint64x1_t;
        fn vceqq_p64(a: poly64x2_t, b: poly64x2_t) -> uint64x2_t;
        fn vceq_f64(a: float64x1_t, b: float64x1_t) -> uint64x1_t;
        fn vceqq_f64(a: float64x2_t, b: float64x2_t) -> uint64x2_t;
        fn vceqd_s64(a: i64, b: i64) -> u64;
        fn vceqd_u64(a: u64, b: u64) -> u64;
        fn vceqs_f32(a: f32, b: f32) -> u32;
        fn vceqd_f64(a: f64, b: f64) -> u64;
        fn vceqz_s8(a: int8x8_t) -> uint8x8_t;
        fn vceqzq_s8(a: int8x16_t) -> uint8x16_t;
        fn vceqz_s16(a: int16x4_t) -> uint16x4_t;
        fn vceqzq_s16(a: int16x8_t) -> uint16x8_t;
        fn vceqz_s32(a: int32x2_t) -> uint32x2_t;
        fn vceqzq_s32(a: int32x4_t) -> uint32x4_t;
        fn vceqz_s64(a: int64x1_t) -> uint64x1_t;
        fn vceqzq_s64(a: int64x2_t) -> uint64x2_t;
        fn vceqz_p8(a: poly8x8_t) -> uint8x8_t;
        fn vceqzq_p8(a: poly8x16_t) -> uint8x16_t;
        fn vceqz_p64(a: poly64x1_t) -> uint64x1_t;
        fn vceqzq_p64(a: poly64x2_t) -> uint64x2_t;
        fn vceqz_u8(a: uint8x8_t) -> uint8x8_t;
        fn vceqzq_u8(a: uint8x16_t) -> uint8x16_t;
        fn vceqz_u16(a: uint16x4_t) -> uint16x4_t;
        fn vceqzq_u16(a: uint16x8_t) -> uint16x8_t;
        fn vceqz_u32(a: uint32x2_t) -> uint32x2_t;
        fn vceqzq_u32(a: uint32x4_t) -> uint32x4_t;
        fn vceqz_u64(a: uint64x1_t) -> uint64x1_t;
        fn vceqzq_u64(a: uint64x2_t) -> uint64x2_t;
        fn vceqz_f32(a: float32x2_t) -> uint32x2_t;
        fn vceqzq_f32(a: float32x4_t) -> uint32x4_t;
        fn vceqz_f64(a: float64x1_t) -> uint64x1_t;
        fn vceqzq_f64(a: float64x2_t) -> uint64x2_t;
        fn vceqzd_s64(a: i64) -> u64;
        fn vceqzd_u64(a: u64) -> u64;
        fn vceqzs_f32(a: f32) -> u32;
        fn vceqzd_f64(a: f64) -> u64;
        fn vtst_s64(a: int64x1_t, b: int64x1_t) -> uint64x1_t;
        fn vtstq_s64(a: int64x2_t, b: int64x2_t) -> uint64x2_t;
        fn vtst_p64(a: poly64x1_t, b: poly64x1_t) -> uint64x1_t;
        fn vtstq_p64(a: poly64x2_t, b: poly64x2_t) -> uint64x2_t;
        fn vtst_u64(a: uint64x1_t, b: uint64x1_t) -> uint64x1_t;
        fn vtstq_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vtstd_s64(a: i64, b: i64) -> u64;
        fn vtstd_u64(a: u64, b: u64) -> u64;
        fn vuqadds_s32(a: i32, b: u32) -> i32;
        fn vuqaddd_s64(a: i64, b: u64) -> i64;
        fn vuqaddb_s8(a: i8, b: u8) -> i8;
        fn vuqaddh_s16(a: i16, b: u16) -> i16;
        fn vabs_f64(a: float64x1_t) -> float64x1_t;
        fn vabsq_f64(a: float64x2_t) -> float64x2_t;
        fn vcgt_s64(a: int64x1_t, b: int64x1_t) -> uint64x1_t;
        fn vcgtq_s64(a: int64x2_t, b: int64x2_t) -> uint64x2_t;
        fn vcgt_u64(a: uint64x1_t, b: uint64x1_t) -> uint64x1_t;
        fn vcgtq_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vcgt_f64(a: float64x1_t, b: float64x1_t) -> uint64x1_t;
        fn vcgtq_f64(a: float64x2_t, b: float64x2_t) -> uint64x2_t;
        fn vcgtd_s64(a: i64, b: i64) -> u64;
        fn vcgtd_u64(a: u64, b: u64) -> u64;
        fn vcgts_f32(a: f32, b: f32) -> u32;
        fn vcgtd_f64(a: f64, b: f64) -> u64;
        fn vclt_s64(a: int64x1_t, b: int64x1_t) -> uint64x1_t;
        fn vcltq_s64(a: int64x2_t, b: int64x2_t) -> uint64x2_t;
        fn vclt_u64(a: uint64x1_t, b: uint64x1_t) -> uint64x1_t;
        fn vcltq_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vclt_f64(a: float64x1_t, b: float64x1_t) -> uint64x1_t;
        fn vcltq_f64(a: float64x2_t, b: float64x2_t) -> uint64x2_t;
        fn vcltd_s64(a: i64, b: i64) -> u64;
        fn vcltd_u64(a: u64, b: u64) -> u64;
        fn vclts_f32(a: f32, b: f32) -> u32;
        fn vcltd_f64(a: f64, b: f64) -> u64;
        fn vcle_s64(a: int64x1_t, b: int64x1_t) -> uint64x1_t;
        fn vcleq_s64(a: int64x2_t, b: int64x2_t) -> uint64x2_t;
        fn vcged_s64(a: i64, b: i64) -> u64;
        fn vcged_u64(a: u64, b: u64) -> u64;
        fn vcges_f32(a: f32, b: f32) -> u32;
        fn vcged_f64(a: f64, b: f64) -> u64;
        fn vcle_u64(a: uint64x1_t, b: uint64x1_t) -> uint64x1_t;
        fn vcleq_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vcle_f64(a: float64x1_t, b: float64x1_t) -> uint64x1_t;
        fn vcleq_f64(a: float64x2_t, b: float64x2_t) -> uint64x2_t;
        fn vcled_s64(a: i64, b: i64) -> u64;
        fn vcled_u64(a: u64, b: u64) -> u64;
        fn vcles_f32(a: f32, b: f32) -> u32;
        fn vcled_f64(a: f64, b: f64) -> u64;
        fn vcge_s64(a: int64x1_t, b: int64x1_t) -> uint64x1_t;
        fn vcgeq_s64(a: int64x2_t, b: int64x2_t) -> uint64x2_t;
        fn vcge_u64(a: uint64x1_t, b: uint64x1_t) -> uint64x1_t;
        fn vcgeq_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vcge_f64(a: float64x1_t, b: float64x1_t) -> uint64x1_t;
        fn vcgeq_f64(a: float64x2_t, b: float64x2_t) -> uint64x2_t;
        fn vcgez_s8(a: int8x8_t) -> uint8x8_t;
        fn vcgezq_s8(a: int8x16_t) -> uint8x16_t;
        fn vcgez_s16(a: int16x4_t) -> uint16x4_t;
        fn vcgezq_s16(a: int16x8_t) -> uint16x8_t;
        fn vcgez_s32(a: int32x2_t) -> uint32x2_t;
        fn vcgezq_s32(a: int32x4_t) -> uint32x4_t;
        fn vcgez_s64(a: int64x1_t) -> uint64x1_t;
        fn vcgezq_s64(a: int64x2_t) -> uint64x2_t;
        fn vcgez_f32(a: float32x2_t) -> uint32x2_t;
        fn vcgezq_f32(a: float32x4_t) -> uint32x4_t;
        fn vcgez_f64(a: float64x1_t) -> uint64x1_t;
        fn vcgezq_f64(a: float64x2_t) -> uint64x2_t;
        fn vcgezd_s64(a: i64) -> u64;
        fn vcgezs_f32(a: f32) -> u32;
        fn vcgezd_f64(a: f64) -> u64;
        fn vcgtz_s8(a: int8x8_t) -> uint8x8_t;
        fn vcgtzq_s8(a: int8x16_t) -> uint8x16_t;
        fn vcgtz_s16(a: int16x4_t) -> uint16x4_t;
        fn vcgtzq_s16(a: int16x8_t) -> uint16x8_t;
        fn vcgtz_s32(a: int32x2_t) -> uint32x2_t;
        fn vcgtzq_s32(a: int32x4_t) -> uint32x4_t;
        fn vcgtz_s64(a: int64x1_t) -> uint64x1_t;
        fn vcgtzq_s64(a: int64x2_t) -> uint64x2_t;
        fn vcgtz_f32(a: float32x2_t) -> uint32x2_t;
        fn vcgtzq_f32(a: float32x4_t) -> uint32x4_t;
        fn vcgtz_f64(a: float64x1_t) -> uint64x1_t;
        fn vcgtzq_f64(a: float64x2_t) -> uint64x2_t;
        fn vcgtzd_s64(a: i64) -> u64;
        fn vcgtzs_f32(a: f32) -> u32;
        fn vcgtzd_f64(a: f64) -> u64;
        fn vclez_s8(a: int8x8_t) -> uint8x8_t;
        fn vclezq_s8(a: int8x16_t) -> uint8x16_t;
        fn vclez_s16(a: int16x4_t) -> uint16x4_t;
        fn vclezq_s16(a: int16x8_t) -> uint16x8_t;
        fn vclez_s32(a: int32x2_t) -> uint32x2_t;
        fn vclezq_s32(a: int32x4_t) -> uint32x4_t;
        fn vclez_s64(a: int64x1_t) -> uint64x1_t;
        fn vclezq_s64(a: int64x2_t) -> uint64x2_t;
        fn vclez_f32(a: float32x2_t) -> uint32x2_t;
        fn vclezq_f32(a: float32x4_t) -> uint32x4_t;
        fn vclez_f64(a: float64x1_t) -> uint64x1_t;
        fn vclezq_f64(a: float64x2_t) -> uint64x2_t;
        fn vclezd_s64(a: i64) -> u64;
        fn vclezs_f32(a: f32) -> u32;
        fn vclezd_f64(a: f64) -> u64;
        fn vcltz_s8(a: int8x8_t) -> uint8x8_t;
        fn vcltzq_s8(a: int8x16_t) -> uint8x16_t;
        fn vcltz_s16(a: int16x4_t) -> uint16x4_t;
        fn vcltzq_s16(a: int16x8_t) -> uint16x8_t;
        fn vcltz_s32(a: int32x2_t) -> uint32x2_t;
        fn vcltzq_s32(a: int32x4_t) -> uint32x4_t;
        fn vcltz_s64(a: int64x1_t) -> uint64x1_t;
        fn vcltzq_s64(a: int64x2_t) -> uint64x2_t;
        fn vcltz_f32(a: float32x2_t) -> uint32x2_t;
        fn vcltzq_f32(a: float32x4_t) -> uint32x4_t;
        fn vcltz_f64(a: float64x1_t) -> uint64x1_t;
        fn vcltzq_f64(a: float64x2_t) -> uint64x2_t;
        fn vcltzd_s64(a: i64) -> u64;
        fn vcltzs_f32(a: f32) -> u32;
        fn vcltzd_f64(a: f64) -> u64;
        fn vcagt_f64(a: float64x1_t, b: float64x1_t) -> uint64x1_t;
        fn vcagtq_f64(a: float64x2_t, b: float64x2_t) -> uint64x2_t;
        fn vcagts_f32(a: f32, b: f32) -> u32;
        fn vcagtd_f64(a: f64, b: f64) -> u64;
        fn vcage_f64(a: float64x1_t, b: float64x1_t) -> uint64x1_t;
        fn vcageq_f64(a: float64x2_t, b: float64x2_t) -> uint64x2_t;
        fn vcages_f32(a: f32, b: f32) -> u32;
        fn vcaged_f64(a: f64, b: f64) -> u64;
        fn vcalt_f64(a: float64x1_t, b: float64x1_t) -> uint64x1_t;
        fn vcaltq_f64(a: float64x2_t, b: float64x2_t) -> uint64x2_t;
        fn vcalts_f32(a: f32, b: f32) -> u32;
        fn vcaltd_f64(a: f64, b: f64) -> u64;
        fn vcale_f64(a: float64x1_t, b: float64x1_t) -> uint64x1_t;
        fn vcaleq_f64(a: float64x2_t, b: float64x2_t) -> uint64x2_t;
        fn vcales_f32(a: f32, b: f32) -> u32;
        fn vcaled_f64(a: f64, b: f64) -> u64;
        fn vcopy_lane_s8<const LANE1: i32, const LANE2: i32>(
            a: int8x8_t,
            b: int8x8_t,
        ) -> int8x8_t;
        fn vcopyq_laneq_s8<const LANE1: i32, const LANE2: i32>(
            a: int8x16_t,
            b: int8x16_t,
        ) -> int8x16_t;
        fn vcopy_lane_s16<const LANE1: i32, const LANE2: i32>(
            a: int16x4_t,
            b: int16x4_t,
        ) -> int16x4_t;
        fn vcopyq_laneq_s16<const LANE1: i32, const LANE2: i32>(
            a: int16x8_t,
            b: int16x8_t,
        ) -> int16x8_t;
        fn vcopy_lane_s32<const LANE1: i32, const LANE2: i32>(
            a: int32x2_t,
            b: int32x2_t,
        ) -> int32x2_t;
        fn vcopyq_laneq_s32<const LANE1: i32, const LANE2: i32>(
            a: int32x4_t,
            b: int32x4_t,
        ) -> int32x4_t;
        fn vcopyq_laneq_s64<const LANE1: i32, const LANE2: i32>(
            a: int64x2_t,
            b: int64x2_t,
        ) -> int64x2_t;
        fn vcopy_lane_u8<const LANE1: i32, const LANE2: i32>(
            a: uint8x8_t,
            b: uint8x8_t,
        ) -> uint8x8_t;
        fn vcopyq_laneq_u8<const LANE1: i32, const LANE2: i32>(
            a: uint8x16_t,
            b: uint8x16_t,
        ) -> uint8x16_t;
        fn vcopy_lane_u16<const LANE1: i32, const LANE2: i32>(
            a: uint16x4_t,
            b: uint16x4_t,
        ) -> uint16x4_t;
        fn vcopyq_laneq_u16<const LANE1: i32, const LANE2: i32>(
            a: uint16x8_t,
            b: uint16x8_t,
        ) -> uint16x8_t;
        fn vcopy_lane_u32<const LANE1: i32, const LANE2: i32>(
            a: uint32x2_t,
            b: uint32x2_t,
        ) -> uint32x2_t;
        fn vcopyq_laneq_u32<const LANE1: i32, const LANE2: i32>(
            a: uint32x4_t,
            b: uint32x4_t,
        ) -> uint32x4_t;
        fn vcopyq_laneq_u64<const LANE1: i32, const LANE2: i32>(
            a: uint64x2_t,
            b: uint64x2_t,
        ) -> uint64x2_t;
        fn vcopy_lane_p8<const LANE1: i32, const LANE2: i32>(
            a: poly8x8_t,
            b: poly8x8_t,
        ) -> poly8x8_t;
        fn vcopyq_laneq_p8<const LANE1: i32, const LANE2: i32>(
            a: poly8x16_t,
            b: poly8x16_t,
        ) -> poly8x16_t;
        fn vcopy_lane_p16<const LANE1: i32, const LANE2: i32>(
            a: poly16x4_t,
            b: poly16x4_t,
        ) -> poly16x4_t;
        fn vcopyq_laneq_p16<const LANE1: i32, const LANE2: i32>(
            a: poly16x8_t,
            b: poly16x8_t,
        ) -> poly16x8_t;
        fn vcopyq_laneq_p64<const LANE1: i32, const LANE2: i32>(
            a: poly64x2_t,
            b: poly64x2_t,
        ) -> poly64x2_t;
        fn vcopy_lane_f32<const LANE1: i32, const LANE2: i32>(
            a: float32x2_t,
            b: float32x2_t,
        ) -> float32x2_t;
        fn vcopyq_laneq_f32<const LANE1: i32, const LANE2: i32>(
            a: float32x4_t,
            b: float32x4_t,
        ) -> float32x4_t;
        fn vcopyq_laneq_f64<const LANE1: i32, const LANE2: i32>(
            a: float64x2_t,
            b: float64x2_t,
        ) -> float64x2_t;
        fn vcopy_laneq_s8<const LANE1: i32, const LANE2: i32>(
            a: int8x8_t,
            b: int8x16_t,
        ) -> int8x8_t;
        fn vcopy_laneq_s16<const LANE1: i32, const LANE2: i32>(
            a: int16x4_t,
            b: int16x8_t,
        ) -> int16x4_t;
        fn vcopy_laneq_s32<const LANE1: i32, const LANE2: i32>(
            a: int32x2_t,
            b: int32x4_t,
        ) -> int32x2_t;
        fn vcopy_laneq_u8<const LANE1: i32, const LANE2: i32>(
            a: uint8x8_t,
            b: uint8x16_t,
        ) -> uint8x8_t;
        fn vcopy_laneq_u16<const LANE1: i32, const LANE2: i32>(
            a: uint16x4_t,
            b: uint16x8_t,
        ) -> uint16x4_t;
        fn vcopy_laneq_u32<const LANE1: i32, const LANE2: i32>(
            a: uint32x2_t,
            b: uint32x4_t,
        ) -> uint32x2_t;
        fn vcopy_laneq_p8<const LANE1: i32, const LANE2: i32>(
            a: poly8x8_t,
            b: poly8x16_t,
        ) -> poly8x8_t;
        fn vcopy_laneq_p16<const LANE1: i32, const LANE2: i32>(
            a: poly16x4_t,
            b: poly16x8_t,
        ) -> poly16x4_t;
        fn vcopy_laneq_f32<const LANE1: i32, const LANE2: i32>(
            a: float32x2_t,
            b: float32x4_t,
        ) -> float32x2_t;
        fn vcopyq_lane_s8<const LANE1: i32, const LANE2: i32>(
            a: int8x16_t,
            b: int8x8_t,
        ) -> int8x16_t;
        fn vcopyq_lane_s16<const LANE1: i32, const LANE2: i32>(
            a: int16x8_t,
            b: int16x4_t,
        ) -> int16x8_t;
        fn vcopyq_lane_s32<const LANE1: i32, const LANE2: i32>(
            a: int32x4_t,
            b: int32x2_t,
        ) -> int32x4_t;
        fn vcopyq_lane_u8<const LANE1: i32, const LANE2: i32>(
            a: uint8x16_t,
            b: uint8x8_t,
        ) -> uint8x16_t;
        fn vcopyq_lane_u16<const LANE1: i32, const LANE2: i32>(
            a: uint16x8_t,
            b: uint16x4_t,
        ) -> uint16x8_t;
        fn vcopyq_lane_u32<const LANE1: i32, const LANE2: i32>(
            a: uint32x4_t,
            b: uint32x2_t,
        ) -> uint32x4_t;
        fn vcopyq_lane_p8<const LANE1: i32, const LANE2: i32>(
            a: poly8x16_t,
            b: poly8x8_t,
        ) -> poly8x16_t;
        fn vcopyq_lane_p16<const LANE1: i32, const LANE2: i32>(
            a: poly16x8_t,
            b: poly16x4_t,
        ) -> poly16x8_t;
        fn vcopyq_lane_s64<const LANE1: i32, const LANE2: i32>(
            a: int64x2_t,
            b: int64x1_t,
        ) -> int64x2_t;
        fn vcopyq_lane_u64<const LANE1: i32, const LANE2: i32>(
            a: uint64x2_t,
            b: uint64x1_t,
        ) -> uint64x2_t;
        fn vcopyq_lane_p64<const LANE1: i32, const LANE2: i32>(
            a: poly64x2_t,
            b: poly64x1_t,
        ) -> poly64x2_t;
        fn vcopyq_lane_f32<const LANE1: i32, const LANE2: i32>(
            a: float32x4_t,
            b: float32x2_t,
        ) -> float32x4_t;
        fn vcopyq_lane_f64<const LANE1: i32, const LANE2: i32>(
            a: float64x2_t,
            b: float64x1_t,
        ) -> float64x2_t;
        fn vcreate_f64(a: u64) -> float64x1_t;
        fn vcvt_f64_s64(a: int64x1_t) -> float64x1_t;
        fn vcvtq_f64_s64(a: int64x2_t) -> float64x2_t;
        fn vcvt_f64_u64(a: uint64x1_t) -> float64x1_t;
        fn vcvtq_f64_u64(a: uint64x2_t) -> float64x2_t;
        fn vcvt_f64_f32(a: float32x2_t) -> float64x2_t;
        fn vcvt_high_f64_f32(a: float32x4_t) -> float64x2_t;
        fn vcvt_f32_f64(a: float64x2_t) -> float32x2_t;
        fn vcvt_high_f32_f64(a: float32x2_t, b: float64x2_t) -> float32x4_t;
        fn vcvtx_f32_f64(a: float64x2_t) -> float32x2_t;
        fn vcvtxd_f32_f64(a: f64) -> f32;
        fn vcvtx_high_f32_f64(a: float32x2_t, b: float64x2_t) -> float32x4_t;
        fn vcvt_n_f64_s64<const N: i32>(a: int64x1_t) -> float64x1_t;
        fn vcvtq_n_f64_s64<const N: i32>(a: int64x2_t) -> float64x2_t;
        fn vcvts_n_f32_s32<const N: i32>(a: i32) -> f32;
        fn vcvtd_n_f64_s64<const N: i32>(a: i64) -> f64;
        fn vcvt_n_f64_u64<const N: i32>(a: uint64x1_t) -> float64x1_t;
        fn vcvtq_n_f64_u64<const N: i32>(a: uint64x2_t) -> float64x2_t;
        fn vcvts_n_f32_u32<const N: i32>(a: u32) -> f32;
        fn vcvtd_n_f64_u64<const N: i32>(a: u64) -> f64;
        fn vcvt_n_s64_f64<const N: i32>(a: float64x1_t) -> int64x1_t;
        fn vcvtq_n_s64_f64<const N: i32>(a: float64x2_t) -> int64x2_t;
        fn vcvts_n_s32_f32<const N: i32>(a: f32) -> i32;
        fn vcvtd_n_s64_f64<const N: i32>(a: f64) -> i64;
        fn vcvt_n_u64_f64<const N: i32>(a: float64x1_t) -> uint64x1_t;
        fn vcvtq_n_u64_f64<const N: i32>(a: float64x2_t) -> uint64x2_t;
        fn vcvts_n_u32_f32<const N: i32>(a: f32) -> u32;
        fn vcvtd_n_u64_f64<const N: i32>(a: f64) -> u64;
        fn vcvts_f32_s32(a: i32) -> f32;
        fn vcvtd_f64_s64(a: i64) -> f64;
        fn vcvts_f32_u32(a: u32) -> f32;
        fn vcvtd_f64_u64(a: u64) -> f64;
        fn vcvts_s32_f32(a: f32) -> i32;
        fn vcvtd_s64_f64(a: f64) -> i64;
        fn vcvts_u32_f32(a: f32) -> u32;
        fn vcvtd_u64_f64(a: f64) -> u64;
        fn vcvt_s64_f64(a: float64x1_t) -> int64x1_t;
        fn vcvtq_s64_f64(a: float64x2_t) -> int64x2_t;
        fn vcvt_u64_f64(a: float64x1_t) -> uint64x1_t;
        fn vcvtq_u64_f64(a: float64x2_t) -> uint64x2_t;
        fn vcvta_s32_f32(a: float32x2_t) -> int32x2_t;
        fn vcvtaq_s32_f32(a: float32x4_t) -> int32x4_t;
        fn vcvta_s64_f64(a: float64x1_t) -> int64x1_t;
        fn vcvtaq_s64_f64(a: float64x2_t) -> int64x2_t;
        fn vcvtas_s32_f32(a: f32) -> i32;
        fn vcvtad_s64_f64(a: f64) -> i64;
        fn vcvtas_u32_f32(a: f32) -> u32;
        fn vcvtad_u64_f64(a: f64) -> u64;
        fn vcvtn_s32_f32(a: float32x2_t) -> int32x2_t;
        fn vcvtnq_s32_f32(a: float32x4_t) -> int32x4_t;
        fn vcvtn_s64_f64(a: float64x1_t) -> int64x1_t;
        fn vcvtnq_s64_f64(a: float64x2_t) -> int64x2_t;
        fn vcvtns_s32_f32(a: f32) -> i32;
        fn vcvtnd_s64_f64(a: f64) -> i64;
        fn vcvtm_s32_f32(a: float32x2_t) -> int32x2_t;
        fn vcvtmq_s32_f32(a: float32x4_t) -> int32x4_t;
        fn vcvtm_s64_f64(a: float64x1_t) -> int64x1_t;
        fn vcvtmq_s64_f64(a: float64x2_t) -> int64x2_t;
        fn vcvtms_s32_f32(a: f32) -> i32;
        fn vcvtmd_s64_f64(a: f64) -> i64;
        fn vcvtp_s32_f32(a: float32x2_t) -> int32x2_t;
        fn vcvtpq_s32_f32(a: float32x4_t) -> int32x4_t;
        fn vcvtp_s64_f64(a: float64x1_t) -> int64x1_t;
        fn vcvtpq_s64_f64(a: float64x2_t) -> int64x2_t;
        fn vcvtps_s32_f32(a: f32) -> i32;
        fn vcvtpd_s64_f64(a: f64) -> i64;
        fn vcvta_u32_f32(a: float32x2_t) -> uint32x2_t;
        fn vcvtaq_u32_f32(a: float32x4_t) -> uint32x4_t;
        fn vcvta_u64_f64(a: float64x1_t) -> uint64x1_t;
        fn vcvtaq_u64_f64(a: float64x2_t) -> uint64x2_t;
        fn vcvtn_u32_f32(a: float32x2_t) -> uint32x2_t;
        fn vcvtnq_u32_f32(a: float32x4_t) -> uint32x4_t;
        fn vcvtn_u64_f64(a: float64x1_t) -> uint64x1_t;
        fn vcvtnq_u64_f64(a: float64x2_t) -> uint64x2_t;
        fn vcvtns_u32_f32(a: f32) -> u32;
        fn vcvtnd_u64_f64(a: f64) -> u64;
        fn vcvtm_u32_f32(a: float32x2_t) -> uint32x2_t;
        fn vcvtmq_u32_f32(a: float32x4_t) -> uint32x4_t;
        fn vcvtm_u64_f64(a: float64x1_t) -> uint64x1_t;
        fn vcvtmq_u64_f64(a: float64x2_t) -> uint64x2_t;
        fn vcvtms_u32_f32(a: f32) -> u32;
        fn vcvtmd_u64_f64(a: f64) -> u64;
        fn vcvtp_u32_f32(a: float32x2_t) -> uint32x2_t;
        fn vcvtpq_u32_f32(a: float32x4_t) -> uint32x4_t;
        fn vcvtp_u64_f64(a: float64x1_t) -> uint64x1_t;
        fn vcvtpq_u64_f64(a: float64x2_t) -> uint64x2_t;
        fn vcvtps_u32_f32(a: f32) -> u32;
        fn vcvtpd_u64_f64(a: f64) -> u64;
        fn vdupq_laneq_p64<const N: i32>(a: poly64x2_t) -> poly64x2_t;
        fn vdupq_lane_p64<const N: i32>(a: poly64x1_t) -> poly64x2_t;
        fn vdupq_laneq_f64<const N: i32>(a: float64x2_t) -> float64x2_t;
        fn vdupq_lane_f64<const N: i32>(a: float64x1_t) -> float64x2_t;
        fn vdup_lane_p64<const N: i32>(a: poly64x1_t) -> poly64x1_t;
        fn vdup_lane_f64<const N: i32>(a: float64x1_t) -> float64x1_t;
        fn vdup_laneq_p64<const N: i32>(a: poly64x2_t) -> poly64x1_t;
        fn vdup_laneq_f64<const N: i32>(a: float64x2_t) -> float64x1_t;
        fn vdupb_lane_s8<const N: i32>(a: int8x8_t) -> i8;
        fn vdupb_laneq_s8<const N: i32>(a: int8x16_t) -> i8;
        fn vduph_lane_s16<const N: i32>(a: int16x4_t) -> i16;
        fn vduph_laneq_s16<const N: i32>(a: int16x8_t) -> i16;
        fn vdups_lane_s32<const N: i32>(a: int32x2_t) -> i32;
        fn vdups_laneq_s32<const N: i32>(a: int32x4_t) -> i32;
        fn vdupd_lane_s64<const N: i32>(a: int64x1_t) -> i64;
        fn vdupd_laneq_s64<const N: i32>(a: int64x2_t) -> i64;
        fn vdupb_lane_u8<const N: i32>(a: uint8x8_t) -> u8;
        fn vdupb_laneq_u8<const N: i32>(a: uint8x16_t) -> u8;
        fn vduph_lane_u16<const N: i32>(a: uint16x4_t) -> u16;
        fn vduph_laneq_u16<const N: i32>(a: uint16x8_t) -> u16;
        fn vdups_lane_u32<const N: i32>(a: uint32x2_t) -> u32;
        fn vdups_laneq_u32<const N: i32>(a: uint32x4_t) -> u32;
        fn vdupd_lane_u64<const N: i32>(a: uint64x1_t) -> u64;
        fn vdupd_laneq_u64<const N: i32>(a: uint64x2_t) -> u64;
        fn vdupb_lane_p8<const N: i32>(a: poly8x8_t) -> p8;
        fn vdupb_laneq_p8<const N: i32>(a: poly8x16_t) -> p8;
        fn vduph_lane_p16<const N: i32>(a: poly16x4_t) -> p16;
        fn vduph_laneq_p16<const N: i32>(a: poly16x8_t) -> p16;
        fn vdups_lane_f32<const N: i32>(a: float32x2_t) -> f32;
        fn vdups_laneq_f32<const N: i32>(a: float32x4_t) -> f32;
        fn vdupd_lane_f64<const N: i32>(a: float64x1_t) -> f64;
        fn vdupd_laneq_f64<const N: i32>(a: float64x2_t) -> f64;
        fn vextq_p64<const N: i32>(a: poly64x2_t, b: poly64x2_t) -> poly64x2_t;
        fn vextq_f64<const N: i32>(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vmla_f64(a: float64x1_t, b: float64x1_t, c: float64x1_t) -> float64x1_t;
        fn vmlaq_f64(a: float64x2_t, b: float64x2_t, c: float64x2_t) -> float64x2_t;
        fn vmlal_high_s8(a: int16x8_t, b: int8x16_t, c: int8x16_t) -> int16x8_t;
        fn vmlal_high_s16(a: int32x4_t, b: int16x8_t, c: int16x8_t) -> int32x4_t;
        fn vmlal_high_s32(a: int64x2_t, b: int32x4_t, c: int32x4_t) -> int64x2_t;
        fn vmlal_high_u8(a: uint16x8_t, b: uint8x16_t, c: uint8x16_t) -> uint16x8_t;
        fn vmlal_high_u16(a: uint32x4_t, b: uint16x8_t, c: uint16x8_t) -> uint32x4_t;
        fn vmlal_high_u32(a: uint64x2_t, b: uint32x4_t, c: uint32x4_t) -> uint64x2_t;
        fn vmlal_high_n_s16(a: int32x4_t, b: int16x8_t, c: i16) -> int32x4_t;
        fn vmlal_high_n_s32(a: int64x2_t, b: int32x4_t, c: i32) -> int64x2_t;
        fn vmlal_high_n_u16(a: uint32x4_t, b: uint16x8_t, c: u16) -> uint32x4_t;
        fn vmlal_high_n_u32(a: uint64x2_t, b: uint32x4_t, c: u32) -> uint64x2_t;
        fn vmlal_high_lane_s16<const LANE: i32>(
            a: int32x4_t,
            b: int16x8_t,
            c: int16x4_t,
        ) -> int32x4_t;
        fn vmlal_high_laneq_s16<const LANE: i32>(
            a: int32x4_t,
            b: int16x8_t,
            c: int16x8_t,
        ) -> int32x4_t;
        fn vmlal_high_lane_s32<const LANE: i32>(
            a: int64x2_t,
            b: int32x4_t,
            c: int32x2_t,
        ) -> int64x2_t;
        fn vmlal_high_laneq_s32<const LANE: i32>(
            a: int64x2_t,
            b: int32x4_t,
            c: int32x4_t,
        ) -> int64x2_t;
        fn vmlal_high_lane_u16<const LANE: i32>(
            a: uint32x4_t,
            b: uint16x8_t,
            c: uint16x4_t,
        ) -> uint32x4_t;
        fn vmlal_high_laneq_u16<const LANE: i32>(
            a: uint32x4_t,
            b: uint16x8_t,
            c: uint16x8_t,
        ) -> uint32x4_t;
        fn vmlal_high_lane_u32<const LANE: i32>(
            a: uint64x2_t,
            b: uint32x4_t,
            c: uint32x2_t,
        ) -> uint64x2_t;
        fn vmlal_high_laneq_u32<const LANE: i32>(
            a: uint64x2_t,
            b: uint32x4_t,
            c: uint32x4_t,
        ) -> uint64x2_t;
        fn vmls_f64(a: float64x1_t, b: float64x1_t, c: float64x1_t) -> float64x1_t;
        fn vmlsq_f64(a: float64x2_t, b: float64x2_t, c: float64x2_t) -> float64x2_t;
        fn vmlsl_high_s8(a: int16x8_t, b: int8x16_t, c: int8x16_t) -> int16x8_t;
        fn vmlsl_high_s16(a: int32x4_t, b: int16x8_t, c: int16x8_t) -> int32x4_t;
        fn vmlsl_high_s32(a: int64x2_t, b: int32x4_t, c: int32x4_t) -> int64x2_t;
        fn vmlsl_high_u8(a: uint16x8_t, b: uint8x16_t, c: uint8x16_t) -> uint16x8_t;
        fn vmlsl_high_u16(a: uint32x4_t, b: uint16x8_t, c: uint16x8_t) -> uint32x4_t;
        fn vmlsl_high_u32(a: uint64x2_t, b: uint32x4_t, c: uint32x4_t) -> uint64x2_t;
        fn vmlsl_high_n_s16(a: int32x4_t, b: int16x8_t, c: i16) -> int32x4_t;
        fn vmlsl_high_n_s32(a: int64x2_t, b: int32x4_t, c: i32) -> int64x2_t;
        fn vmlsl_high_n_u16(a: uint32x4_t, b: uint16x8_t, c: u16) -> uint32x4_t;
        fn vmlsl_high_n_u32(a: uint64x2_t, b: uint32x4_t, c: u32) -> uint64x2_t;
        fn vmlsl_high_lane_s16<const LANE: i32>(
            a: int32x4_t,
            b: int16x8_t,
            c: int16x4_t,
        ) -> int32x4_t;
        fn vmlsl_high_laneq_s16<const LANE: i32>(
            a: int32x4_t,
            b: int16x8_t,
            c: int16x8_t,
        ) -> int32x4_t;
        fn vmlsl_high_lane_s32<const LANE: i32>(
            a: int64x2_t,
            b: int32x4_t,
            c: int32x2_t,
        ) -> int64x2_t;
        fn vmlsl_high_laneq_s32<const LANE: i32>(
            a: int64x2_t,
            b: int32x4_t,
            c: int32x4_t,
        ) -> int64x2_t;
        fn vmlsl_high_lane_u16<const LANE: i32>(
            a: uint32x4_t,
            b: uint16x8_t,
            c: uint16x4_t,
        ) -> uint32x4_t;
        fn vmlsl_high_laneq_u16<const LANE: i32>(
            a: uint32x4_t,
            b: uint16x8_t,
            c: uint16x8_t,
        ) -> uint32x4_t;
        fn vmlsl_high_lane_u32<const LANE: i32>(
            a: uint64x2_t,
            b: uint32x4_t,
            c: uint32x2_t,
        ) -> uint64x2_t;
        fn vmlsl_high_laneq_u32<const LANE: i32>(
            a: uint64x2_t,
            b: uint32x4_t,
            c: uint32x4_t,
        ) -> uint64x2_t;
        fn vmovn_high_s16(a: int8x8_t, b: int16x8_t) -> int8x16_t;
        fn vmovn_high_s32(a: int16x4_t, b: int32x4_t) -> int16x8_t;
        fn vmovn_high_s64(a: int32x2_t, b: int64x2_t) -> int32x4_t;
        fn vmovn_high_u16(a: uint8x8_t, b: uint16x8_t) -> uint8x16_t;
        fn vmovn_high_u32(a: uint16x4_t, b: uint32x4_t) -> uint16x8_t;
        fn vmovn_high_u64(a: uint32x2_t, b: uint64x2_t) -> uint32x4_t;
        fn vneg_s64(a: int64x1_t) -> int64x1_t;
        fn vnegq_s64(a: int64x2_t) -> int64x2_t;
        fn vnegd_s64(a: i64) -> i64;
        fn vneg_f64(a: float64x1_t) -> float64x1_t;
        fn vnegq_f64(a: float64x2_t) -> float64x2_t;
        fn vqneg_s64(a: int64x1_t) -> int64x1_t;
        fn vqnegq_s64(a: int64x2_t) -> int64x2_t;
        fn vqnegb_s8(a: i8) -> i8;
        fn vqnegh_s16(a: i16) -> i16;
        fn vqnegs_s32(a: i32) -> i32;
        fn vqnegd_s64(a: i64) -> i64;
        fn vqsubb_s8(a: i8, b: i8) -> i8;
        fn vqsubh_s16(a: i16, b: i16) -> i16;
        fn vqsubb_u8(a: u8, b: u8) -> u8;
        fn vqsubh_u16(a: u16, b: u16) -> u16;
        fn vqsubs_u32(a: u32, b: u32) -> u32;
        fn vqsubd_u64(a: u64, b: u64) -> u64;
        fn vqsubs_s32(a: i32, b: i32) -> i32;
        fn vqsubd_s64(a: i64, b: i64) -> i64;
        fn vrbit_s8(a: int8x8_t) -> int8x8_t;
        fn vrbitq_s8(a: int8x16_t) -> int8x16_t;
        fn vrbit_u8(a: uint8x8_t) -> uint8x8_t;
        fn vrbitq_u8(a: uint8x16_t) -> uint8x16_t;
        fn vrbit_p8(a: poly8x8_t) -> poly8x8_t;
        fn vrbitq_p8(a: poly8x16_t) -> poly8x16_t;
        fn vrndx_f32(a: float32x2_t) -> float32x2_t;
        fn vrndxq_f32(a: float32x4_t) -> float32x4_t;
        fn vrndx_f64(a: float64x1_t) -> float64x1_t;
        fn vrndxq_f64(a: float64x2_t) -> float64x2_t;
        fn vrnda_f32(a: float32x2_t) -> float32x2_t;
        fn vrndaq_f32(a: float32x4_t) -> float32x4_t;
        fn vrnda_f64(a: float64x1_t) -> float64x1_t;
        fn vrndaq_f64(a: float64x2_t) -> float64x2_t;
        fn vrndn_f64(a: float64x1_t) -> float64x1_t;
        fn vrndnq_f64(a: float64x2_t) -> float64x2_t;
        fn vrndns_f32(a: f32) -> f32;
        fn vrndm_f32(a: float32x2_t) -> float32x2_t;
        fn vrndmq_f32(a: float32x4_t) -> float32x4_t;
        fn vrndm_f64(a: float64x1_t) -> float64x1_t;
        fn vrndmq_f64(a: float64x2_t) -> float64x2_t;
        fn vrndp_f32(a: float32x2_t) -> float32x2_t;
        fn vrndpq_f32(a: float32x4_t) -> float32x4_t;
        fn vrndp_f64(a: float64x1_t) -> float64x1_t;
        fn vrndpq_f64(a: float64x2_t) -> float64x2_t;
        fn vrnd_f32(a: float32x2_t) -> float32x2_t;
        fn vrndq_f32(a: float32x4_t) -> float32x4_t;
        fn vrnd_f64(a: float64x1_t) -> float64x1_t;
        fn vrndq_f64(a: float64x2_t) -> float64x2_t;
        fn vrndi_f32(a: float32x2_t) -> float32x2_t;
        fn vrndiq_f32(a: float32x4_t) -> float32x4_t;
        fn vrndi_f64(a: float64x1_t) -> float64x1_t;
        fn vrndiq_f64(a: float64x2_t) -> float64x2_t;
        fn vqaddb_s8(a: i8, b: i8) -> i8;
        fn vqaddh_s16(a: i16, b: i16) -> i16;
        fn vqaddb_u8(a: u8, b: u8) -> u8;
        fn vqaddh_u16(a: u16, b: u16) -> u16;
        fn vqadds_u32(a: u32, b: u32) -> u32;
        fn vqaddd_u64(a: u64, b: u64) -> u64;
        fn vqadds_s32(a: i32, b: i32) -> i32;
        fn vqaddd_s64(a: i64, b: i64) -> i64;
        unsafe fn vld1_f64_x2(a: *const f64) -> float64x1x2_t;
        unsafe fn vld1q_f64_x2(a: *const f64) -> float64x2x2_t;
        unsafe fn vld1_f64_x3(a: *const f64) -> float64x1x3_t;
        unsafe fn vld1q_f64_x3(a: *const f64) -> float64x2x3_t;
        unsafe fn vld1_f64_x4(a: *const f64) -> float64x1x4_t;
        unsafe fn vld1q_f64_x4(a: *const f64) -> float64x2x4_t;
        unsafe fn vld2q_s64(a: *const i64) -> int64x2x2_t;
        unsafe fn vld2q_u64(a: *const u64) -> uint64x2x2_t;
        unsafe fn vld2_f64(a: *const f64) -> float64x1x2_t;
        unsafe fn vld2q_f64(a: *const f64) -> float64x2x2_t;
        unsafe fn vld2q_dup_s64(a: *const i64) -> int64x2x2_t;
        unsafe fn vld2q_dup_u64(a: *const u64) -> uint64x2x2_t;
        unsafe fn vld2_dup_f64(a: *const f64) -> float64x1x2_t;
        unsafe fn vld2q_dup_f64(a: *const f64) -> float64x2x2_t;
        unsafe fn vld2q_lane_s8<const LANE: i32>(a: *const i8, b: int8x16x2_t) -> int8x16x2_t;
        unsafe fn vld2_lane_s64<const LANE: i32>(a: *const i64, b: int64x1x2_t) -> int64x1x2_t;
        unsafe fn vld2q_lane_s64<const LANE: i32>(a: *const i64, b: int64x2x2_t) -> int64x2x2_t;
        unsafe fn vld2q_lane_u8<const LANE: i32>(a: *const u8, b: uint8x16x2_t) -> uint8x16x2_t;
        unsafe fn vld2_lane_u64<const LANE: i32>(a: *const u64, b: uint64x1x2_t) -> uint64x1x2_t;
        unsafe fn vld2q_lane_u64<const LANE: i32>(a: *const u64, b: uint64x2x2_t) -> uint64x2x2_t;
        unsafe fn vld2q_lane_p8<const LANE: i32>(a: *const p8, b: poly8x16x2_t) -> poly8x16x2_t;
        unsafe fn vld2_lane_f64<const LANE: i32>(a: *const f64, b: float64x1x2_t) -> float64x1x2_t;
        unsafe fn vld2q_lane_f64<const LANE: i32>(a: *const f64, b: float64x2x2_t) -> float64x2x2_t;
        unsafe fn vld3q_s64(a: *const i64) -> int64x2x3_t;
        unsafe fn vld3q_u64(a: *const u64) -> uint64x2x3_t;
        unsafe fn vld3_f64(a: *const f64) -> float64x1x3_t;
        unsafe fn vld3q_f64(a: *const f64) -> float64x2x3_t;
        unsafe fn vld3q_dup_s64(a: *const i64) -> int64x2x3_t;
        unsafe fn vld3q_dup_u64(a: *const u64) -> uint64x2x3_t;
        unsafe fn vld3_dup_f64(a: *const f64) -> float64x1x3_t;
        unsafe fn vld3q_dup_f64(a: *const f64) -> float64x2x3_t;
        unsafe fn vld3q_lane_s8<const LANE: i32>(a: *const i8, b: int8x16x3_t) -> int8x16x3_t;
        unsafe fn vld3_lane_s64<const LANE: i32>(a: *const i64, b: int64x1x3_t) -> int64x1x3_t;
        unsafe fn vld3q_lane_s64<const LANE: i32>(a: *const i64, b: int64x2x3_t) -> int64x2x3_t;
        unsafe fn vld3q_lane_p8<const LANE: i32>(a: *const p8, b: poly8x16x3_t) -> poly8x16x3_t;
        unsafe fn vld3q_lane_u8<const LANE: i32>(a: *const u8, b: uint8x16x3_t) -> uint8x16x3_t;
        unsafe fn vld3_lane_u64<const LANE: i32>(a: *const u64, b: uint64x1x3_t) -> uint64x1x3_t;
        unsafe fn vld3q_lane_u64<const LANE: i32>(a: *const u64, b: uint64x2x3_t) -> uint64x2x3_t;
        unsafe fn vld3_lane_f64<const LANE: i32>(a: *const f64, b: float64x1x3_t) -> float64x1x3_t;
        unsafe fn vld3q_lane_f64<const LANE: i32>(a: *const f64, b: float64x2x3_t) -> float64x2x3_t;
        unsafe fn vld4q_s64(a: *const i64) -> int64x2x4_t;
        unsafe fn vld4q_u64(a: *const u64) -> uint64x2x4_t;
        unsafe fn vld4_f64(a: *const f64) -> float64x1x4_t;
        unsafe fn vld4q_f64(a: *const f64) -> float64x2x4_t;
        unsafe fn vld4q_dup_s64(a: *const i64) -> int64x2x4_t;
        unsafe fn vld4q_dup_u64(a: *const u64) -> uint64x2x4_t;
        unsafe fn vld4_dup_f64(a: *const f64) -> float64x1x4_t;
        unsafe fn vld4q_dup_f64(a: *const f64) -> float64x2x4_t;
        unsafe fn vld4q_lane_s8<const LANE: i32>(a: *const i8, b: int8x16x4_t) -> int8x16x4_t;
        unsafe fn vld4_lane_s64<const LANE: i32>(a: *const i64, b: int64x1x4_t) -> int64x1x4_t;
        unsafe fn vld4q_lane_s64<const LANE: i32>(a: *const i64, b: int64x2x4_t) -> int64x2x4_t;
        unsafe fn vld4q_lane_p8<const LANE: i32>(a: *const p8, b: poly8x16x4_t) -> poly8x16x4_t;
        unsafe fn vld4q_lane_u8<const LANE: i32>(a: *const u8, b: uint8x16x4_t) -> uint8x16x4_t;
        unsafe fn vld4_lane_u64<const LANE: i32>(a: *const u64, b: uint64x1x4_t) -> uint64x1x4_t;
        unsafe fn vld4q_lane_u64<const LANE: i32>(a: *const u64, b: uint64x2x4_t) -> uint64x2x4_t;
        unsafe fn vld4_lane_f64<const LANE: i32>(a: *const f64, b: float64x1x4_t) -> float64x1x4_t;
        unsafe fn vld4q_lane_f64<const LANE: i32>(a: *const f64, b: float64x2x4_t) -> float64x2x4_t;
        unsafe fn vst1_lane_f64<const LANE: i32>(a: *mut f64, b: float64x1_t);
        unsafe fn vst1q_lane_f64<const LANE: i32>(a: *mut f64, b: float64x2_t);
        unsafe fn vst1_f64_x2(a: *mut f64, b: float64x1x2_t);
        unsafe fn vst1q_f64_x2(a: *mut f64, b: float64x2x2_t);
        unsafe fn vst1_f64_x3(a: *mut f64, b: float64x1x3_t);
        unsafe fn vst1q_f64_x3(a: *mut f64, b: float64x2x3_t);
        unsafe fn vst1_f64_x4(a: *mut f64, b: float64x1x4_t);
        unsafe fn vst1q_f64_x4(a: *mut f64, b: float64x2x4_t);
        unsafe fn vst2q_s64(a: *mut i64, b: int64x2x2_t);
        unsafe fn vst2q_u64(a: *mut u64, b: uint64x2x2_t);
        unsafe fn vst2_f64(a: *mut f64, b: float64x1x2_t);
        unsafe fn vst2q_f64(a: *mut f64, b: float64x2x2_t);
        unsafe fn vst2q_lane_s8<const LANE: i32>(a: *mut i8, b: int8x16x2_t);
        unsafe fn vst2_lane_s64<const LANE: i32>(a: *mut i64, b: int64x1x2_t);
        unsafe fn vst2q_lane_s64<const LANE: i32>(a: *mut i64, b: int64x2x2_t);
        unsafe fn vst2q_lane_u8<const LANE: i32>(a: *mut u8, b: uint8x16x2_t);
        unsafe fn vst2_lane_u64<const LANE: i32>(a: *mut u64, b: uint64x1x2_t);
        unsafe fn vst2q_lane_u64<const LANE: i32>(a: *mut u64, b: uint64x2x2_t);
        unsafe fn vst2q_lane_p8<const LANE: i32>(a: *mut p8, b: poly8x16x2_t);
        unsafe fn vst2_lane_f64<const LANE: i32>(a: *mut f64, b: float64x1x2_t);
        unsafe fn vst2q_lane_f64<const LANE: i32>(a: *mut f64, b: float64x2x2_t);
        unsafe fn vst3q_s64(a: *mut i64, b: int64x2x3_t);
        unsafe fn vst3q_u64(a: *mut u64, b: uint64x2x3_t);
        unsafe fn vst3_f64(a: *mut f64, b: float64x1x3_t);
        unsafe fn vst3q_f64(a: *mut f64, b: float64x2x3_t);
        unsafe fn vst3q_lane_s8<const LANE: i32>(a: *mut i8, b: int8x16x3_t);
        unsafe fn vst3_lane_s64<const LANE: i32>(a: *mut i64, b: int64x1x3_t);
        unsafe fn vst3q_lane_s64<const LANE: i32>(a: *mut i64, b: int64x2x3_t);
        unsafe fn vst3q_lane_u8<const LANE: i32>(a: *mut u8, b: uint8x16x3_t);
        unsafe fn vst3_lane_u64<const LANE: i32>(a: *mut u64, b: uint64x1x3_t);
        unsafe fn vst3q_lane_u64<const LANE: i32>(a: *mut u64, b: uint64x2x3_t);
        unsafe fn vst3q_lane_p8<const LANE: i32>(a: *mut p8, b: poly8x16x3_t);
        unsafe fn vst3_lane_f64<const LANE: i32>(a: *mut f64, b: float64x1x3_t);
        unsafe fn vst3q_lane_f64<const LANE: i32>(a: *mut f64, b: float64x2x3_t);
        unsafe fn vst4q_s64(a: *mut i64, b: int64x2x4_t);
        unsafe fn vst4q_u64(a: *mut u64, b: uint64x2x4_t);
        unsafe fn vst4_f64(a: *mut f64, b: float64x1x4_t);
        unsafe fn vst4q_f64(a: *mut f64, b: float64x2x4_t);
        unsafe fn vst4q_lane_s8<const LANE: i32>(a: *mut i8, b: int8x16x4_t);
        unsafe fn vst4_lane_s64<const LANE: i32>(a: *mut i64, b: int64x1x4_t);
        unsafe fn vst4q_lane_s64<const LANE: i32>(a: *mut i64, b: int64x2x4_t);
        unsafe fn vst4q_lane_u8<const LANE: i32>(a: *mut u8, b: uint8x16x4_t);
        unsafe fn vst4_lane_u64<const LANE: i32>(a: *mut u64, b: uint64x1x4_t);
        unsafe fn vst4q_lane_u64<const LANE: i32>(a: *mut u64, b: uint64x2x4_t);
        unsafe fn vst4q_lane_p8<const LANE: i32>(a: *mut p8, b: poly8x16x4_t);
        unsafe fn vst4_lane_f64<const LANE: i32>(a: *mut f64, b: float64x1x4_t);
        unsafe fn vst4q_lane_f64<const LANE: i32>(a: *mut f64, b: float64x2x4_t);
        fn vmul_f64(a: float64x1_t, b: float64x1_t) -> float64x1_t;
        fn vmulq_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vmul_n_f64(a: float64x1_t, b: f64) -> float64x1_t;
        fn vmulq_n_f64(a: float64x2_t, b: f64) -> float64x2_t;
        fn vmul_lane_f64<const LANE: i32>(a: float64x1_t, b: float64x1_t) -> float64x1_t;
        fn vmul_laneq_f64<const LANE: i32>(a: float64x1_t, b: float64x2_t) -> float64x1_t;
        fn vmulq_lane_f64<const LANE: i32>(a: float64x2_t, b: float64x1_t) -> float64x2_t;
        fn vmulq_laneq_f64<const LANE: i32>(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vmuls_lane_f32<const LANE: i32>(a: f32, b: float32x2_t) -> f32;
        fn vmuls_laneq_f32<const LANE: i32>(a: f32, b: float32x4_t) -> f32;
        fn vmuld_lane_f64<const LANE: i32>(a: f64, b: float64x1_t) -> f64;
        fn vmuld_laneq_f64<const LANE: i32>(a: f64, b: float64x2_t) -> f64;
        fn vmull_high_s8(a: int8x16_t, b: int8x16_t) -> int16x8_t;
        fn vmull_high_s16(a: int16x8_t, b: int16x8_t) -> int32x4_t;
        fn vmull_high_s32(a: int32x4_t, b: int32x4_t) -> int64x2_t;
        fn vmull_high_u8(a: uint8x16_t, b: uint8x16_t) -> uint16x8_t;
        fn vmull_high_u16(a: uint16x8_t, b: uint16x8_t) -> uint32x4_t;
        fn vmull_high_u32(a: uint32x4_t, b: uint32x4_t) -> uint64x2_t;
        fn vmull_high_p8(a: poly8x16_t, b: poly8x16_t) -> poly16x8_t;
        fn vmull_high_n_s16(a: int16x8_t, b: i16) -> int32x4_t;
        fn vmull_high_n_s32(a: int32x4_t, b: i32) -> int64x2_t;
        fn vmull_high_n_u16(a: uint16x8_t, b: u16) -> uint32x4_t;
        fn vmull_high_n_u32(a: uint32x4_t, b: u32) -> uint64x2_t;
        fn vmull_high_lane_s16<const LANE: i32>(a: int16x8_t, b: int16x4_t) -> int32x4_t;
        fn vmull_high_laneq_s16<const LANE: i32>(a: int16x8_t, b: int16x8_t) -> int32x4_t;
        fn vmull_high_lane_s32<const LANE: i32>(a: int32x4_t, b: int32x2_t) -> int64x2_t;
        fn vmull_high_laneq_s32<const LANE: i32>(a: int32x4_t, b: int32x4_t) -> int64x2_t;
        fn vmull_high_lane_u16<const LANE: i32>(a: uint16x8_t, b: uint16x4_t) -> uint32x4_t;
        fn vmull_high_laneq_u16<const LANE: i32>(a: uint16x8_t, b: uint16x8_t) -> uint32x4_t;
        fn vmull_high_lane_u32<const LANE: i32>(a: uint32x4_t, b: uint32x2_t) -> uint64x2_t;
        fn vmull_high_laneq_u32<const LANE: i32>(a: uint32x4_t, b: uint32x4_t) -> uint64x2_t;
        fn vmulx_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vmulxq_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vmulx_f64(a: float64x1_t, b: float64x1_t) -> float64x1_t;
        fn vmulxq_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vmulx_lane_f64<const LANE: i32>(a: float64x1_t, b: float64x1_t) -> float64x1_t;
        fn vmulx_laneq_f64<const LANE: i32>(a: float64x1_t, b: float64x2_t) -> float64x1_t;
        fn vmulx_lane_f32<const LANE: i32>(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vmulx_laneq_f32<const LANE: i32>(a: float32x2_t, b: float32x4_t) -> float32x2_t;
        fn vmulxq_lane_f32<const LANE: i32>(a: float32x4_t, b: float32x2_t) -> float32x4_t;
        fn vmulxq_laneq_f32<const LANE: i32>(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vmulxq_lane_f64<const LANE: i32>(a: float64x2_t, b: float64x1_t) -> float64x2_t;
        fn vmulxq_laneq_f64<const LANE: i32>(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vmulxs_f32(a: f32, b: f32) -> f32;
        fn vmulxd_f64(a: f64, b: f64) -> f64;
        fn vmulxs_lane_f32<const LANE: i32>(a: f32, b: float32x2_t) -> f32;
        fn vmulxs_laneq_f32<const LANE: i32>(a: f32, b: float32x4_t) -> f32;
        fn vmulxd_lane_f64<const LANE: i32>(a: f64, b: float64x1_t) -> f64;
        fn vmulxd_laneq_f64<const LANE: i32>(a: f64, b: float64x2_t) -> f64;
        fn vfma_f64(a: float64x1_t, b: float64x1_t, c: float64x1_t) -> float64x1_t;
        fn vfmaq_f64(a: float64x2_t, b: float64x2_t, c: float64x2_t) -> float64x2_t;
        fn vfma_n_f64(a: float64x1_t, b: float64x1_t, c: f64) -> float64x1_t;
        fn vfmaq_n_f64(a: float64x2_t, b: float64x2_t, c: f64) -> float64x2_t;
        fn vfma_lane_f32<const LANE: i32>(
            a: float32x2_t,
            b: float32x2_t,
            c: float32x2_t,
        ) -> float32x2_t;
        fn vfma_laneq_f32<const LANE: i32>(
            a: float32x2_t,
            b: float32x2_t,
            c: float32x4_t,
        ) -> float32x2_t;
        fn vfmaq_lane_f32<const LANE: i32>(
            a: float32x4_t,
            b: float32x4_t,
            c: float32x2_t,
        ) -> float32x4_t;
        fn vfmaq_laneq_f32<const LANE: i32>(
            a: float32x4_t,
            b: float32x4_t,
            c: float32x4_t,
        ) -> float32x4_t;
        fn vfma_lane_f64<const LANE: i32>(
            a: float64x1_t,
            b: float64x1_t,
            c: float64x1_t,
        ) -> float64x1_t;
        fn vfma_laneq_f64<const LANE: i32>(
            a: float64x1_t,
            b: float64x1_t,
            c: float64x2_t,
        ) -> float64x1_t;
        fn vfmaq_lane_f64<const LANE: i32>(
            a: float64x2_t,
            b: float64x2_t,
            c: float64x1_t,
        ) -> float64x2_t;
        fn vfmaq_laneq_f64<const LANE: i32>(
            a: float64x2_t,
            b: float64x2_t,
            c: float64x2_t,
        ) -> float64x2_t;
        fn vfmas_lane_f32<const LANE: i32>(a: f32, b: f32, c: float32x2_t) -> f32;
        fn vfmas_laneq_f32<const LANE: i32>(a: f32, b: f32, c: float32x4_t) -> f32;
        fn vfmad_lane_f64<const LANE: i32>(a: f64, b: f64, c: float64x1_t) -> f64;
        fn vfmad_laneq_f64<const LANE: i32>(a: f64, b: f64, c: float64x2_t) -> f64;
        fn vfms_f64(a: float64x1_t, b: float64x1_t, c: float64x1_t) -> float64x1_t;
        fn vfmsq_f64(a: float64x2_t, b: float64x2_t, c: float64x2_t) -> float64x2_t;
        fn vfms_n_f64(a: float64x1_t, b: float64x1_t, c: f64) -> float64x1_t;
        fn vfmsq_n_f64(a: float64x2_t, b: float64x2_t, c: f64) -> float64x2_t;
        fn vfms_lane_f32<const LANE: i32>(
            a: float32x2_t,
            b: float32x2_t,
            c: float32x2_t,
        ) -> float32x2_t;
        fn vfms_laneq_f32<const LANE: i32>(
            a: float32x2_t,
            b: float32x2_t,
            c: float32x4_t,
        ) -> float32x2_t;
        fn vfmsq_lane_f32<const LANE: i32>(
            a: float32x4_t,
            b: float32x4_t,
            c: float32x2_t,
        ) -> float32x4_t;
        fn vfmsq_laneq_f32<const LANE: i32>(
            a: float32x4_t,
            b: float32x4_t,
            c: float32x4_t,
        ) -> float32x4_t;
        fn vfms_lane_f64<const LANE: i32>(
            a: float64x1_t,
            b: float64x1_t,
            c: float64x1_t,
        ) -> float64x1_t;
        fn vfms_laneq_f64<const LANE: i32>(
            a: float64x1_t,
            b: float64x1_t,
            c: float64x2_t,
        ) -> float64x1_t;
        fn vfmsq_lane_f64<const LANE: i32>(
            a: float64x2_t,
            b: float64x2_t,
            c: float64x1_t,
        ) -> float64x2_t;
        fn vfmsq_laneq_f64<const LANE: i32>(
            a: float64x2_t,
            b: float64x2_t,
            c: float64x2_t,
        ) -> float64x2_t;
        fn vfmss_lane_f32<const LANE: i32>(a: f32, b: f32, c: float32x2_t) -> f32;
        fn vfmss_laneq_f32<const LANE: i32>(a: f32, b: f32, c: float32x4_t) -> f32;
        fn vfmsd_lane_f64<const LANE: i32>(a: f64, b: f64, c: float64x1_t) -> f64;
        fn vfmsd_laneq_f64<const LANE: i32>(a: f64, b: f64, c: float64x2_t) -> f64;
        fn vdiv_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vdivq_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vdiv_f64(a: float64x1_t, b: float64x1_t) -> float64x1_t;
        fn vdivq_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vsub_f64(a: float64x1_t, b: float64x1_t) -> float64x1_t;
        fn vsubq_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vsubd_s64(a: i64, b: i64) -> i64;
        fn vsubd_u64(a: u64, b: u64) -> u64;
        fn vaddv_f32(a: float32x2_t) -> f32;
        fn vaddvq_f32(a: float32x4_t) -> f32;
        fn vaddvq_f64(a: float64x2_t) -> f64;
        fn vaddlv_s16(a: int16x4_t) -> i32;
        fn vaddlvq_s16(a: int16x8_t) -> i32;
        fn vaddlv_s32(a: int32x2_t) -> i64;
        fn vaddlvq_s32(a: int32x4_t) -> i64;
        fn vaddlv_u16(a: uint16x4_t) -> u32;
        fn vaddlvq_u16(a: uint16x8_t) -> u32;
        fn vaddlv_u32(a: uint32x2_t) -> u64;
        fn vaddlvq_u32(a: uint32x4_t) -> u64;
        fn vsubw_high_s8(a: int16x8_t, b: int8x16_t) -> int16x8_t;
        fn vsubw_high_s16(a: int32x4_t, b: int16x8_t) -> int32x4_t;
        fn vsubw_high_s32(a: int64x2_t, b: int32x4_t) -> int64x2_t;
        fn vsubw_high_u8(a: uint16x8_t, b: uint8x16_t) -> uint16x8_t;
        fn vsubw_high_u16(a: uint32x4_t, b: uint16x8_t) -> uint32x4_t;
        fn vsubw_high_u32(a: uint64x2_t, b: uint32x4_t) -> uint64x2_t;
        fn vsubl_high_s8(a: int8x16_t, b: int8x16_t) -> int16x8_t;
        fn vsubl_high_s16(a: int16x8_t, b: int16x8_t) -> int32x4_t;
        fn vsubl_high_s32(a: int32x4_t, b: int32x4_t) -> int64x2_t;
        fn vsubl_high_u8(a: uint8x16_t, b: uint8x16_t) -> uint16x8_t;
        fn vsubl_high_u16(a: uint16x8_t, b: uint16x8_t) -> uint32x4_t;
        fn vsubl_high_u32(a: uint32x4_t, b: uint32x4_t) -> uint64x2_t;
        fn vmax_f64(a: float64x1_t, b: float64x1_t) -> float64x1_t;
        fn vmaxq_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vmaxnm_f64(a: float64x1_t, b: float64x1_t) -> float64x1_t;
        fn vmaxnmq_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vmaxnmv_f32(a: float32x2_t) -> f32;
        fn vmaxnmvq_f64(a: float64x2_t) -> f64;
        fn vmaxnmvq_f32(a: float32x4_t) -> f32;
        fn vpmaxnm_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vpmaxnmq_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vpmaxnmq_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vpmaxnms_f32(a: float32x2_t) -> f32;
        fn vpmaxnmqd_f64(a: float64x2_t) -> f64;
        fn vpmaxs_f32(a: float32x2_t) -> f32;
        fn vpmaxqd_f64(a: float64x2_t) -> f64;
        fn vmin_f64(a: float64x1_t, b: float64x1_t) -> float64x1_t;
        fn vminq_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vminnm_f64(a: float64x1_t, b: float64x1_t) -> float64x1_t;
        fn vminnmq_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vminnmv_f32(a: float32x2_t) -> f32;
        fn vminnmvq_f64(a: float64x2_t) -> f64;
        fn vminnmvq_f32(a: float32x4_t) -> f32;
        fn vmovl_high_s8(a: int8x16_t) -> int16x8_t;
        fn vmovl_high_s16(a: int16x8_t) -> int32x4_t;
        fn vmovl_high_s32(a: int32x4_t) -> int64x2_t;
        fn vmovl_high_u8(a: uint8x16_t) -> uint16x8_t;
        fn vmovl_high_u16(a: uint16x8_t) -> uint32x4_t;
        fn vmovl_high_u32(a: uint32x4_t) -> uint64x2_t;
        fn vpaddq_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vpaddq_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vpadds_f32(a: float32x2_t) -> f32;
        fn vpaddd_f64(a: float64x2_t) -> f64;
        fn vpminnm_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vpminnmq_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vpminnmq_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vpminnms_f32(a: float32x2_t) -> f32;
        fn vpminnmqd_f64(a: float64x2_t) -> f64;
        fn vpmins_f32(a: float32x2_t) -> f32;
        fn vpminqd_f64(a: float64x2_t) -> f64;
        fn vqdmullh_s16(a: i16, b: i16) -> i32;
        fn vqdmulls_s32(a: i32, b: i32) -> i64;
        fn vqdmull_high_s16(a: int16x8_t, b: int16x8_t) -> int32x4_t;
        fn vqdmull_high_s32(a: int32x4_t, b: int32x4_t) -> int64x2_t;
        fn vqdmull_high_n_s16(a: int16x8_t, b: i16) -> int32x4_t;
        fn vqdmull_high_n_s32(a: int32x4_t, b: i32) -> int64x2_t;
        fn vqdmull_laneq_s16<const N: i32>(a: int16x4_t, b: int16x8_t) -> int32x4_t;
        fn vqdmull_laneq_s32<const N: i32>(a: int32x2_t, b: int32x4_t) -> int64x2_t;
        fn vqdmullh_lane_s16<const N: i32>(a: i16, b: int16x4_t) -> i32;
        fn vqdmullh_laneq_s16<const N: i32>(a: i16, b: int16x8_t) -> i32;
        fn vqdmulls_lane_s32<const N: i32>(a: i32, b: int32x2_t) -> i64;
        fn vqdmulls_laneq_s32<const N: i32>(a: i32, b: int32x4_t) -> i64;
        fn vqdmull_high_lane_s16<const N: i32>(a: int16x8_t, b: int16x4_t) -> int32x4_t;
        fn vqdmull_high_lane_s32<const N: i32>(a: int32x4_t, b: int32x2_t) -> int64x2_t;
        fn vqdmull_high_laneq_s16<const N: i32>(a: int16x8_t, b: int16x8_t) -> int32x4_t;
        fn vqdmull_high_laneq_s32<const N: i32>(a: int32x4_t, b: int32x4_t) -> int64x2_t;
        fn vqdmlal_high_s16(a: int32x4_t, b: int16x8_t, c: int16x8_t) -> int32x4_t;
        fn vqdmlal_high_s32(a: int64x2_t, b: int32x4_t, c: int32x4_t) -> int64x2_t;
        fn vqdmlal_high_n_s16(a: int32x4_t, b: int16x8_t, c: i16) -> int32x4_t;
        fn vqdmlal_high_n_s32(a: int64x2_t, b: int32x4_t, c: i32) -> int64x2_t;
        fn vqdmlal_laneq_s16<const N: i32>(
            a: int32x4_t,
            b: int16x4_t,
            c: int16x8_t,
        ) -> int32x4_t;
        fn vqdmlal_laneq_s32<const N: i32>(
            a: int64x2_t,
            b: int32x2_t,
            c: int32x4_t,
        ) -> int64x2_t;
        fn vqdmlal_high_lane_s16<const N: i32>(
            a: int32x4_t,
            b: int16x8_t,
            c: int16x4_t,
        ) -> int32x4_t;
        fn vqdmlal_high_laneq_s16<const N: i32>(
            a: int32x4_t,
            b: int16x8_t,
            c: int16x8_t,
        ) -> int32x4_t;
        fn vqdmlal_high_lane_s32<const N: i32>(
            a: int64x2_t,
            b: int32x4_t,
            c: int32x2_t,
        ) -> int64x2_t;
        fn vqdmlal_high_laneq_s32<const N: i32>(
            a: int64x2_t,
            b: int32x4_t,
            c: int32x4_t,
        ) -> int64x2_t;
        fn vqdmlalh_s16(a: i32, b: i16, c: i16) -> i32;
        fn vqdmlals_s32(a: i64, b: i32, c: i32) -> i64;
        fn vqdmlalh_lane_s16<const LANE: i32>(a: i32, b: i16, c: int16x4_t) -> i32;
        fn vqdmlalh_laneq_s16<const LANE: i32>(a: i32, b: i16, c: int16x8_t) -> i32;
        fn vqdmlals_lane_s32<const LANE: i32>(a: i64, b: i32, c: int32x2_t) -> i64;
        fn vqdmlals_laneq_s32<const LANE: i32>(a: i64, b: i32, c: int32x4_t) -> i64;
        fn vqdmlsl_high_s16(a: int32x4_t, b: int16x8_t, c: int16x8_t) -> int32x4_t;
        fn vqdmlsl_high_s32(a: int64x2_t, b: int32x4_t, c: int32x4_t) -> int64x2_t;
        fn vqdmlsl_high_n_s16(a: int32x4_t, b: int16x8_t, c: i16) -> int32x4_t;
        fn vqdmlsl_high_n_s32(a: int64x2_t, b: int32x4_t, c: i32) -> int64x2_t;
        fn vqdmlsl_laneq_s16<const N: i32>(
            a: int32x4_t,
            b: int16x4_t,
            c: int16x8_t,
        ) -> int32x4_t;
        fn vqdmlsl_laneq_s32<const N: i32>(
            a: int64x2_t,
            b: int32x2_t,
            c: int32x4_t,
        ) -> int64x2_t;
        fn vqdmlsl_high_lane_s16<const N: i32>(
            a: int32x4_t,
            b: int16x8_t,
            c: int16x4_t,
        ) -> int32x4_t;
        fn vqdmlsl_high_laneq_s16<const N: i32>(
            a: int32x4_t,
            b: int16x8_t,
            c: int16x8_t,
        ) -> int32x4_t;
        fn vqdmlsl_high_lane_s32<const N: i32>(
            a: int64x2_t,
            b: int32x4_t,
            c: int32x2_t,
        ) -> int64x2_t;
        fn vqdmlsl_high_laneq_s32<const N: i32>(
            a: int64x2_t,
            b: int32x4_t,
            c: int32x4_t,
        ) -> int64x2_t;
        fn vqdmlslh_s16(a: i32, b: i16, c: i16) -> i32;
        fn vqdmlsls_s32(a: i64, b: i32, c: i32) -> i64;
        fn vqdmlslh_lane_s16<const LANE: i32>(a: i32, b: i16, c: int16x4_t) -> i32;
        fn vqdmlslh_laneq_s16<const LANE: i32>(a: i32, b: i16, c: int16x8_t) -> i32;
        fn vqdmlsls_lane_s32<const LANE: i32>(a: i64, b: i32, c: int32x2_t) -> i64;
        fn vqdmlsls_laneq_s32<const LANE: i32>(a: i64, b: i32, c: int32x4_t) -> i64;
        fn vqdmulhh_s16(a: i16, b: i16) -> i16;
        fn vqdmulhs_s32(a: i32, b: i32) -> i32;
        fn vqdmulhh_lane_s16<const N: i32>(a: i16, b: int16x4_t) -> i16;
        fn vqdmulhh_laneq_s16<const N: i32>(a: i16, b: int16x8_t) -> i16;
        fn vqdmulhs_lane_s32<const N: i32>(a: i32, b: int32x2_t) -> i32;
        fn vqdmulhs_laneq_s32<const N: i32>(a: i32, b: int32x4_t) -> i32;
        fn vqdmulh_lane_s16<const LANE: i32>(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vqdmulhq_lane_s16<const LANE: i32>(a: int16x8_t, b: int16x4_t) -> int16x8_t;
        fn vqdmulh_lane_s32<const LANE: i32>(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vqdmulhq_lane_s32<const LANE: i32>(a: int32x4_t, b: int32x2_t) -> int32x4_t;
        fn vqmovnh_s16(a: i16) -> i8;
        fn vqmovns_s32(a: i32) -> i16;
        fn vqmovnh_u16(a: u16) -> u8;
        fn vqmovns_u32(a: u32) -> u16;
        fn vqmovnd_s64(a: i64) -> i32;
        fn vqmovnd_u64(a: u64) -> u32;
        fn vqmovn_high_s16(a: int8x8_t, b: int16x8_t) -> int8x16_t;
        fn vqmovn_high_s32(a: int16x4_t, b: int32x4_t) -> int16x8_t;
        fn vqmovn_high_s64(a: int32x2_t, b: int64x2_t) -> int32x4_t;
        fn vqmovn_high_u16(a: uint8x8_t, b: uint16x8_t) -> uint8x16_t;
        fn vqmovn_high_u32(a: uint16x4_t, b: uint32x4_t) -> uint16x8_t;
        fn vqmovn_high_u64(a: uint32x2_t, b: uint64x2_t) -> uint32x4_t;
        fn vqmovunh_s16(a: i16) -> u8;
        fn vqmovuns_s32(a: i32) -> u16;
        fn vqmovund_s64(a: i64) -> u32;
        fn vqmovun_high_s16(a: uint8x8_t, b: int16x8_t) -> uint8x16_t;
        fn vqmovun_high_s32(a: uint16x4_t, b: int32x4_t) -> uint16x8_t;
        fn vqmovun_high_s64(a: uint32x2_t, b: int64x2_t) -> uint32x4_t;
        fn vqrdmulhh_s16(a: i16, b: i16) -> i16;
        fn vqrdmulhs_s32(a: i32, b: i32) -> i32;
        fn vqrdmulhh_lane_s16<const LANE: i32>(a: i16, b: int16x4_t) -> i16;
        fn vqrdmulhh_laneq_s16<const LANE: i32>(a: i16, b: int16x8_t) -> i16;
        fn vqrdmulhs_lane_s32<const LANE: i32>(a: i32, b: int32x2_t) -> i32;
        fn vqrdmulhs_laneq_s32<const LANE: i32>(a: i32, b: int32x4_t) -> i32;
        fn vqrdmlah_s16(a: int16x4_t, b: int16x4_t, c: int16x4_t) -> int16x4_t;
        fn vqrdmlahq_s16(a: int16x8_t, b: int16x8_t, c: int16x8_t) -> int16x8_t;
        fn vqrdmlah_s32(a: int32x2_t, b: int32x2_t, c: int32x2_t) -> int32x2_t;
        fn vqrdmlahq_s32(a: int32x4_t, b: int32x4_t, c: int32x4_t) -> int32x4_t;
        fn vqrdmlahh_s16(a: i16, b: i16, c: i16) -> i16;
        fn vqrdmlahs_s32(a: i32, b: i32, c: i32) -> i32;
        fn vqrdmlah_lane_s16<const LANE: i32>(
            a: int16x4_t,
            b: int16x4_t,
            c: int16x4_t,
        ) -> int16x4_t;
        fn vqrdmlah_laneq_s16<const LANE: i32>(
            a: int16x4_t,
            b: int16x4_t,
            c: int16x8_t,
        ) -> int16x4_t;
        fn vqrdmlahq_lane_s16<const LANE: i32>(
            a: int16x8_t,
            b: int16x8_t,
            c: int16x4_t,
        ) -> int16x8_t;
        fn vqrdmlahq_laneq_s16<const LANE: i32>(
            a: int16x8_t,
            b: int16x8_t,
            c: int16x8_t,
        ) -> int16x8_t;
        fn vqrdmlah_lane_s32<const LANE: i32>(
            a: int32x2_t,
            b: int32x2_t,
            c: int32x2_t,
        ) -> int32x2_t;
        fn vqrdmlah_laneq_s32<const LANE: i32>(
            a: int32x2_t,
            b: int32x2_t,
            c: int32x4_t,
        ) -> int32x2_t;
        fn vqrdmlahq_lane_s32<const LANE: i32>(
            a: int32x4_t,
            b: int32x4_t,
            c: int32x2_t,
        ) -> int32x4_t;
        fn vqrdmlahq_laneq_s32<const LANE: i32>(
            a: int32x4_t,
            b: int32x4_t,
            c: int32x4_t,
        ) -> int32x4_t;
        fn vqrdmlahh_lane_s16<const LANE: i32>(a: i16, b: i16, c: int16x4_t) -> i16;
        fn vqrdmlahh_laneq_s16<const LANE: i32>(a: i16, b: i16, c: int16x8_t) -> i16;
        fn vqrdmlahs_lane_s32<const LANE: i32>(a: i32, b: i32, c: int32x2_t) -> i32;
        fn vqrdmlahs_laneq_s32<const LANE: i32>(a: i32, b: i32, c: int32x4_t) -> i32;
        fn vqrdmlsh_s16(a: int16x4_t, b: int16x4_t, c: int16x4_t) -> int16x4_t;
        fn vqrdmlshq_s16(a: int16x8_t, b: int16x8_t, c: int16x8_t) -> int16x8_t;
        fn vqrdmlsh_s32(a: int32x2_t, b: int32x2_t, c: int32x2_t) -> int32x2_t;
        fn vqrdmlshq_s32(a: int32x4_t, b: int32x4_t, c: int32x4_t) -> int32x4_t;
        fn vqrdmlshh_s16(a: i16, b: i16, c: i16) -> i16;
        fn vqrdmlshs_s32(a: i32, b: i32, c: i32) -> i32;
        fn vqrdmlsh_lane_s16<const LANE: i32>(
            a: int16x4_t,
            b: int16x4_t,
            c: int16x4_t,
        ) -> int16x4_t;
        fn vqrdmlsh_laneq_s16<const LANE: i32>(
            a: int16x4_t,
            b: int16x4_t,
            c: int16x8_t,
        ) -> int16x4_t;
        fn vqrdmlshq_lane_s16<const LANE: i32>(
            a: int16x8_t,
            b: int16x8_t,
            c: int16x4_t,
        ) -> int16x8_t;
        fn vqrdmlshq_laneq_s16<const LANE: i32>(
            a: int16x8_t,
            b: int16x8_t,
            c: int16x8_t,
        ) -> int16x8_t;
        fn vqrdmlsh_lane_s32<const LANE: i32>(
            a: int32x2_t,
            b: int32x2_t,
            c: int32x2_t,
        ) -> int32x2_t;
        fn vqrdmlsh_laneq_s32<const LANE: i32>(
            a: int32x2_t,
            b: int32x2_t,
            c: int32x4_t,
        ) -> int32x2_t;
        fn vqrdmlshq_lane_s32<const LANE: i32>(
            a: int32x4_t,
            b: int32x4_t,
            c: int32x2_t,
        ) -> int32x4_t;
        fn vqrdmlshq_laneq_s32<const LANE: i32>(
            a: int32x4_t,
            b: int32x4_t,
            c: int32x4_t,
        ) -> int32x4_t;
        fn vqrdmlshh_lane_s16<const LANE: i32>(a: i16, b: i16, c: int16x4_t) -> i16;
        fn vqrdmlshh_laneq_s16<const LANE: i32>(a: i16, b: i16, c: int16x8_t) -> i16;
        fn vqrdmlshs_lane_s32<const LANE: i32>(a: i32, b: i32, c: int32x2_t) -> i32;
        fn vqrdmlshs_laneq_s32<const LANE: i32>(a: i32, b: i32, c: int32x4_t) -> i32;
        fn vqrshls_s32(a: i32, b: i32) -> i32;
        fn vqrshld_s64(a: i64, b: i64) -> i64;
        fn vqrshlb_s8(a: i8, b: i8) -> i8;
        fn vqrshlh_s16(a: i16, b: i16) -> i16;
        fn vqrshls_u32(a: u32, b: i32) -> u32;
        fn vqrshld_u64(a: u64, b: i64) -> u64;
        fn vqrshlb_u8(a: u8, b: i8) -> u8;
        fn vqrshlh_u16(a: u16, b: i16) -> u16;
        fn vqrshrnh_n_s16<const N: i32>(a: i16) -> i8;
        fn vqrshrns_n_s32<const N: i32>(a: i32) -> i16;
        fn vqrshrnd_n_s64<const N: i32>(a: i64) -> i32;
        fn vqrshrn_high_n_s16<const N: i32>(a: int8x8_t, b: int16x8_t) -> int8x16_t;
        fn vqrshrn_high_n_s32<const N: i32>(a: int16x4_t, b: int32x4_t) -> int16x8_t;
        fn vqrshrn_high_n_s64<const N: i32>(a: int32x2_t, b: int64x2_t) -> int32x4_t;
        fn vqrshrnh_n_u16<const N: i32>(a: u16) -> u8;
        fn vqrshrns_n_u32<const N: i32>(a: u32) -> u16;
        fn vqrshrnd_n_u64<const N: i32>(a: u64) -> u32;
        fn vqrshrn_high_n_u16<const N: i32>(a: uint8x8_t, b: uint16x8_t) -> uint8x16_t;
        fn vqrshrn_high_n_u32<const N: i32>(a: uint16x4_t, b: uint32x4_t) -> uint16x8_t;
        fn vqrshrn_high_n_u64<const N: i32>(a: uint32x2_t, b: uint64x2_t) -> uint32x4_t;
        fn vqrshrunh_n_s16<const N: i32>(a: i16) -> u8;
        fn vqrshruns_n_s32<const N: i32>(a: i32) -> u16;
        fn vqrshrund_n_s64<const N: i32>(a: i64) -> u32;
        fn vqrshrun_high_n_s16<const N: i32>(a: uint8x8_t, b: int16x8_t) -> uint8x16_t;
        fn vqrshrun_high_n_s32<const N: i32>(a: uint16x4_t, b: int32x4_t) -> uint16x8_t;
        fn vqrshrun_high_n_s64<const N: i32>(a: uint32x2_t, b: int64x2_t) -> uint32x4_t;
        fn vqshld_s64(a: i64, b: i64) -> i64;
        fn vqshlb_s8(a: i8, b: i8) -> i8;
        fn vqshlh_s16(a: i16, b: i16) -> i16;
        fn vqshls_s32(a: i32, b: i32) -> i32;
        fn vqshld_u64(a: u64, b: i64) -> u64;
        fn vqshlb_u8(a: u8, b: i8) -> u8;
        fn vqshlh_u16(a: u16, b: i16) -> u16;
        fn vqshls_u32(a: u32, b: i32) -> u32;
        fn vqshlb_n_s8<const N: i32>(a: i8) -> i8;
        fn vqshlh_n_s16<const N: i32>(a: i16) -> i16;
        fn vqshls_n_s32<const N: i32>(a: i32) -> i32;
        fn vqshld_n_s64<const N: i32>(a: i64) -> i64;
        fn vqshlb_n_u8<const N: i32>(a: u8) -> u8;
        fn vqshlh_n_u16<const N: i32>(a: u16) -> u16;
        fn vqshls_n_u32<const N: i32>(a: u32) -> u32;
        fn vqshld_n_u64<const N: i32>(a: u64) -> u64;
        fn vqshlub_n_s8<const N: i32>(a: i8) -> u8;
        fn vqshluh_n_s16<const N: i32>(a: i16) -> u16;
        fn vqshlus_n_s32<const N: i32>(a: i32) -> u32;
        fn vqshlud_n_s64<const N: i32>(a: i64) -> u64;
        fn vqshrnd_n_s64<const N: i32>(a: i64) -> i32;
        fn vqshrnh_n_s16<const N: i32>(a: i16) -> i8;
        fn vqshrns_n_s32<const N: i32>(a: i32) -> i16;
        fn vqshrn_high_n_s16<const N: i32>(a: int8x8_t, b: int16x8_t) -> int8x16_t;
        fn vqshrn_high_n_s32<const N: i32>(a: int16x4_t, b: int32x4_t) -> int16x8_t;
        fn vqshrn_high_n_s64<const N: i32>(a: int32x2_t, b: int64x2_t) -> int32x4_t;
        fn vqshrnd_n_u64<const N: i32>(a: u64) -> u32;
        fn vqshrnh_n_u16<const N: i32>(a: u16) -> u8;
        fn vqshrns_n_u32<const N: i32>(a: u32) -> u16;
        fn vqshrn_high_n_u16<const N: i32>(a: uint8x8_t, b: uint16x8_t) -> uint8x16_t;
        fn vqshrn_high_n_u32<const N: i32>(a: uint16x4_t, b: uint32x4_t) -> uint16x8_t;
        fn vqshrn_high_n_u64<const N: i32>(a: uint32x2_t, b: uint64x2_t) -> uint32x4_t;
        fn vqshrunh_n_s16<const N: i32>(a: i16) -> u8;
        fn vqshruns_n_s32<const N: i32>(a: i32) -> u16;
        fn vqshrund_n_s64<const N: i32>(a: i64) -> u32;
        fn vqshrun_high_n_s16<const N: i32>(a: uint8x8_t, b: int16x8_t) -> uint8x16_t;
        fn vqshrun_high_n_s32<const N: i32>(a: uint16x4_t, b: int32x4_t) -> uint16x8_t;
        fn vqshrun_high_n_s64<const N: i32>(a: uint32x2_t, b: int64x2_t) -> uint32x4_t;
        fn vsqaddb_u8(a: u8, b: i8) -> u8;
        fn vsqaddh_u16(a: u16, b: i16) -> u16;
        fn vsqadds_u32(a: u32, b: i32) -> u32;
        fn vsqaddd_u64(a: u64, b: i64) -> u64;
        fn vsqrt_f32(a: float32x2_t) -> float32x2_t;
        fn vsqrtq_f32(a: float32x4_t) -> float32x4_t;
        fn vsqrt_f64(a: float64x1_t) -> float64x1_t;
        fn vsqrtq_f64(a: float64x2_t) -> float64x2_t;
        fn vrsqrte_f64(a: float64x1_t) -> float64x1_t;
        fn vrsqrteq_f64(a: float64x2_t) -> float64x2_t;
        fn vrsqrtes_f32(a: f32) -> f32;
        fn vrsqrted_f64(a: f64) -> f64;
        fn vrsqrts_f64(a: float64x1_t, b: float64x1_t) -> float64x1_t;
        fn vrsqrtsq_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vrsqrtss_f32(a: f32, b: f32) -> f32;
        fn vrsqrtsd_f64(a: f64, b: f64) -> f64;
        fn vrecpe_f64(a: float64x1_t) -> float64x1_t;
        fn vrecpeq_f64(a: float64x2_t) -> float64x2_t;
        fn vrecpes_f32(a: f32) -> f32;
        fn vrecped_f64(a: f64) -> f64;
        fn vrecps_f64(a: float64x1_t, b: float64x1_t) -> float64x1_t;
        fn vrecpsq_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vrecpss_f32(a: f32, b: f32) -> f32;
        fn vrecpsd_f64(a: f64, b: f64) -> f64;
        fn vrecpxs_f32(a: f32) -> f32;
        fn vrecpxd_f64(a: f64) -> f64;
        fn vreinterpret_s64_p64(a: poly64x1_t) -> int64x1_t;
        fn vreinterpret_u64_p64(a: poly64x1_t) -> uint64x1_t;
        fn vreinterpret_p64_s64(a: int64x1_t) -> poly64x1_t;
        fn vreinterpret_p64_u64(a: uint64x1_t) -> poly64x1_t;
        fn vreinterpretq_s64_p64(a: poly64x2_t) -> int64x2_t;
        fn vreinterpretq_u64_p64(a: poly64x2_t) -> uint64x2_t;
        fn vreinterpretq_p64_s64(a: int64x2_t) -> poly64x2_t;
        fn vreinterpretq_p64_u64(a: uint64x2_t) -> poly64x2_t;
        fn vreinterpret_s8_f64(a: float64x1_t) -> int8x8_t;
        fn vreinterpret_s16_f64(a: float64x1_t) -> int16x4_t;
        fn vreinterpret_s32_f64(a: float64x1_t) -> int32x2_t;
        fn vreinterpret_s64_f64(a: float64x1_t) -> int64x1_t;
        fn vreinterpretq_s8_f64(a: float64x2_t) -> int8x16_t;
        fn vreinterpretq_s16_f64(a: float64x2_t) -> int16x8_t;
        fn vreinterpretq_s32_f64(a: float64x2_t) -> int32x4_t;
        fn vreinterpretq_s64_f64(a: float64x2_t) -> int64x2_t;
        fn vreinterpret_u8_f64(a: float64x1_t) -> uint8x8_t;
        fn vreinterpret_u16_f64(a: float64x1_t) -> uint16x4_t;
        fn vreinterpret_u32_f64(a: float64x1_t) -> uint32x2_t;
        fn vreinterpret_u64_f64(a: float64x1_t) -> uint64x1_t;
        fn vreinterpretq_u8_f64(a: float64x2_t) -> uint8x16_t;
        fn vreinterpretq_u16_f64(a: float64x2_t) -> uint16x8_t;
        fn vreinterpretq_u32_f64(a: float64x2_t) -> uint32x4_t;
        fn vreinterpretq_u64_f64(a: float64x2_t) -> uint64x2_t;
        fn vreinterpret_p8_f64(a: float64x1_t) -> poly8x8_t;
        fn vreinterpret_p16_f64(a: float64x1_t) -> poly16x4_t;
        fn vreinterpret_p64_f32(a: float32x2_t) -> poly64x1_t;
        fn vreinterpret_p64_f64(a: float64x1_t) -> poly64x1_t;
        fn vreinterpretq_p8_f64(a: float64x2_t) -> poly8x16_t;
        fn vreinterpretq_p16_f64(a: float64x2_t) -> poly16x8_t;
        fn vreinterpretq_p64_f32(a: float32x4_t) -> poly64x2_t;
        fn vreinterpretq_p64_f64(a: float64x2_t) -> poly64x2_t;
        fn vreinterpretq_p128_f64(a: float64x2_t) -> p128;
        fn vreinterpret_f64_s8(a: int8x8_t) -> float64x1_t;
        fn vreinterpret_f64_s16(a: int16x4_t) -> float64x1_t;
        fn vreinterpret_f64_s32(a: int32x2_t) -> float64x1_t;
        fn vreinterpret_f64_s64(a: int64x1_t) -> float64x1_t;
        fn vreinterpretq_f64_s8(a: int8x16_t) -> float64x2_t;
        fn vreinterpretq_f64_s16(a: int16x8_t) -> float64x2_t;
        fn vreinterpretq_f64_s32(a: int32x4_t) -> float64x2_t;
        fn vreinterpretq_f64_s64(a: int64x2_t) -> float64x2_t;
        fn vreinterpret_f64_p8(a: poly8x8_t) -> float64x1_t;
        fn vreinterpret_f64_u16(a: uint16x4_t) -> float64x1_t;
        fn vreinterpret_f64_u32(a: uint32x2_t) -> float64x1_t;
        fn vreinterpret_f64_u64(a: uint64x1_t) -> float64x1_t;
        fn vreinterpretq_f64_p8(a: poly8x16_t) -> float64x2_t;
        fn vreinterpretq_f64_u16(a: uint16x8_t) -> float64x2_t;
        fn vreinterpretq_f64_u32(a: uint32x4_t) -> float64x2_t;
        fn vreinterpretq_f64_u64(a: uint64x2_t) -> float64x2_t;
        fn vreinterpret_f64_u8(a: uint8x8_t) -> float64x1_t;
        fn vreinterpret_f64_p16(a: poly16x4_t) -> float64x1_t;
        fn vreinterpret_f64_p64(a: poly64x1_t) -> float64x1_t;
        fn vreinterpret_f32_p64(a: poly64x1_t) -> float32x2_t;
        fn vreinterpretq_f64_u8(a: uint8x16_t) -> float64x2_t;
        fn vreinterpretq_f64_p16(a: poly16x8_t) -> float64x2_t;
        fn vreinterpretq_f64_p64(a: poly64x2_t) -> float64x2_t;
        fn vreinterpretq_f32_p64(a: poly64x2_t) -> float32x4_t;
        fn vreinterpretq_f64_p128(a: p128) -> float64x2_t;
        fn vreinterpret_f64_f32(a: float32x2_t) -> float64x1_t;
        fn vreinterpret_f32_f64(a: float64x1_t) -> float32x2_t;
        fn vreinterpretq_f64_f32(a: float32x4_t) -> float64x2_t;
        fn vreinterpretq_f32_f64(a: float64x2_t) -> float32x4_t;
        fn vrshld_s64(a: i64, b: i64) -> i64;
        fn vrshld_u64(a: u64, b: i64) -> u64;
        fn vrshrd_n_s64<const N: i32>(a: i64) -> i64;
        fn vrshrd_n_u64<const N: i32>(a: u64) -> u64;
        fn vrshrn_high_n_s16<const N: i32>(a: int8x8_t, b: int16x8_t) -> int8x16_t;
        fn vrshrn_high_n_s32<const N: i32>(a: int16x4_t, b: int32x4_t) -> int16x8_t;
        fn vrshrn_high_n_s64<const N: i32>(a: int32x2_t, b: int64x2_t) -> int32x4_t;
        fn vrshrn_high_n_u16<const N: i32>(a: uint8x8_t, b: uint16x8_t) -> uint8x16_t;
        fn vrshrn_high_n_u32<const N: i32>(a: uint16x4_t, b: uint32x4_t) -> uint16x8_t;
        fn vrshrn_high_n_u64<const N: i32>(a: uint32x2_t, b: uint64x2_t) -> uint32x4_t;
        fn vrsrad_n_s64<const N: i32>(a: i64, b: i64) -> i64;
        fn vrsrad_n_u64<const N: i32>(a: u64, b: u64) -> u64;
        fn vrsubhn_high_s16(a: int8x8_t, b: int16x8_t, c: int16x8_t) -> int8x16_t;
        fn vrsubhn_high_s32(a: int16x4_t, b: int32x4_t, c: int32x4_t) -> int16x8_t;
        fn vrsubhn_high_s64(a: int32x2_t, b: int64x2_t, c: int64x2_t) -> int32x4_t;
        fn vrsubhn_high_u16(a: uint8x8_t, b: uint16x8_t, c: uint16x8_t) -> uint8x16_t;
        fn vrsubhn_high_u32(a: uint16x4_t, b: uint32x4_t, c: uint32x4_t) -> uint16x8_t;
        fn vrsubhn_high_u64(a: uint32x2_t, b: uint64x2_t, c: uint64x2_t) -> uint32x4_t;
        fn vset_lane_f64<const LANE: i32>(a: f64, b: float64x1_t) -> float64x1_t;
        fn vsetq_lane_f64<const LANE: i32>(a: f64, b: float64x2_t) -> float64x2_t;
        fn vshld_s64(a: i64, b: i64) -> i64;
        fn vshld_u64(a: u64, b: i64) -> u64;
        fn vshll_high_n_s8<const N: i32>(a: int8x16_t) -> int16x8_t;
        fn vshll_high_n_s16<const N: i32>(a: int16x8_t) -> int32x4_t;
        fn vshll_high_n_s32<const N: i32>(a: int32x4_t) -> int64x2_t;
        fn vshll_high_n_u8<const N: i32>(a: uint8x16_t) -> uint16x8_t;
        fn vshll_high_n_u16<const N: i32>(a: uint16x8_t) -> uint32x4_t;
        fn vshll_high_n_u32<const N: i32>(a: uint32x4_t) -> uint64x2_t;
        fn vshrn_high_n_s16<const N: i32>(a: int8x8_t, b: int16x8_t) -> int8x16_t;
        fn vshrn_high_n_s32<const N: i32>(a: int16x4_t, b: int32x4_t) -> int16x8_t;
        fn vshrn_high_n_s64<const N: i32>(a: int32x2_t, b: int64x2_t) -> int32x4_t;
        fn vshrn_high_n_u16<const N: i32>(a: uint8x8_t, b: uint16x8_t) -> uint8x16_t;
        fn vshrn_high_n_u32<const N: i32>(a: uint16x4_t, b: uint32x4_t) -> uint16x8_t;
        fn vshrn_high_n_u64<const N: i32>(a: uint32x2_t, b: uint64x2_t) -> uint32x4_t;
        fn vtrn1_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vtrn1q_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vtrn1_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vtrn1q_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vtrn1q_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vtrn1_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vtrn1q_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vtrn1_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vtrn1q_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vtrn1q_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vtrn1_p8(a: poly8x8_t, b: poly8x8_t) -> poly8x8_t;
        fn vtrn1q_p8(a: poly8x16_t, b: poly8x16_t) -> poly8x16_t;
        fn vtrn1_p16(a: poly16x4_t, b: poly16x4_t) -> poly16x4_t;
        fn vtrn1q_p16(a: poly16x8_t, b: poly16x8_t) -> poly16x8_t;
        fn vtrn1_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vtrn1q_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vtrn1_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vtrn1q_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vtrn1q_p64(a: poly64x2_t, b: poly64x2_t) -> poly64x2_t;
        fn vtrn1q_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vtrn1_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vtrn1q_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vtrn2_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vtrn2q_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vtrn2_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vtrn2q_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vtrn2q_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vtrn2_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vtrn2q_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vtrn2_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vtrn2q_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vtrn2q_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vtrn2_p8(a: poly8x8_t, b: poly8x8_t) -> poly8x8_t;
        fn vtrn2q_p8(a: poly8x16_t, b: poly8x16_t) -> poly8x16_t;
        fn vtrn2_p16(a: poly16x4_t, b: poly16x4_t) -> poly16x4_t;
        fn vtrn2q_p16(a: poly16x8_t, b: poly16x8_t) -> poly16x8_t;
        fn vtrn2_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vtrn2q_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vtrn2_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vtrn2q_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vtrn2q_p64(a: poly64x2_t, b: poly64x2_t) -> poly64x2_t;
        fn vtrn2q_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vtrn2_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vtrn2q_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vzip1_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vzip1q_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vzip1_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vzip1q_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vzip1_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vzip1q_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vzip1q_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vzip1_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vzip1q_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vzip1_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vzip1q_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vzip1_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vzip1q_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vzip1q_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vzip1_p8(a: poly8x8_t, b: poly8x8_t) -> poly8x8_t;
        fn vzip1q_p8(a: poly8x16_t, b: poly8x16_t) -> poly8x16_t;
        fn vzip1_p16(a: poly16x4_t, b: poly16x4_t) -> poly16x4_t;
        fn vzip1q_p16(a: poly16x8_t, b: poly16x8_t) -> poly16x8_t;
        fn vzip1q_p64(a: poly64x2_t, b: poly64x2_t) -> poly64x2_t;
        fn vzip1_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vzip1q_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vzip1q_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vzip2_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vzip2q_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vzip2_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vzip2q_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vzip2_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vzip2q_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vzip2q_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vzip2_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vzip2q_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vzip2_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vzip2q_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vzip2_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vzip2q_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vzip2q_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vzip2_p8(a: poly8x8_t, b: poly8x8_t) -> poly8x8_t;
        fn vzip2q_p8(a: poly8x16_t, b: poly8x16_t) -> poly8x16_t;
        fn vzip2_p16(a: poly16x4_t, b: poly16x4_t) -> poly16x4_t;
        fn vzip2q_p16(a: poly16x8_t, b: poly16x8_t) -> poly16x8_t;
        fn vzip2q_p64(a: poly64x2_t, b: poly64x2_t) -> poly64x2_t;
        fn vzip2_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vzip2q_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vzip2q_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vuzp1_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vuzp1q_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vuzp1_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vuzp1q_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vuzp1q_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vuzp1_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vuzp1q_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vuzp1_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vuzp1q_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vuzp1q_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vuzp1_p8(a: poly8x8_t, b: poly8x8_t) -> poly8x8_t;
        fn vuzp1q_p8(a: poly8x16_t, b: poly8x16_t) -> poly8x16_t;
        fn vuzp1_p16(a: poly16x4_t, b: poly16x4_t) -> poly16x4_t;
        fn vuzp1q_p16(a: poly16x8_t, b: poly16x8_t) -> poly16x8_t;
        fn vuzp1_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vuzp1q_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vuzp1_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vuzp1q_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vuzp1q_p64(a: poly64x2_t, b: poly64x2_t) -> poly64x2_t;
        fn vuzp1q_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vuzp1_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vuzp1q_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vuzp2_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vuzp2q_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vuzp2_s16(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vuzp2q_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vuzp2q_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vuzp2_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vuzp2q_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vuzp2_u16(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vuzp2q_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vuzp2q_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vuzp2_p8(a: poly8x8_t, b: poly8x8_t) -> poly8x8_t;
        fn vuzp2q_p8(a: poly8x16_t, b: poly8x16_t) -> poly8x16_t;
        fn vuzp2_p16(a: poly16x4_t, b: poly16x4_t) -> poly16x4_t;
        fn vuzp2q_p16(a: poly16x8_t, b: poly16x8_t) -> poly16x8_t;
        fn vuzp2_s32(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vuzp2q_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vuzp2_u32(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vuzp2q_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vuzp2q_p64(a: poly64x2_t, b: poly64x2_t) -> poly64x2_t;
        fn vuzp2q_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vuzp2_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vuzp2q_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vabal_high_u8(a: uint16x8_t, b: uint8x16_t, c: uint8x16_t) -> uint16x8_t;
        fn vabal_high_u16(a: uint32x4_t, b: uint16x8_t, c: uint16x8_t) -> uint32x4_t;
        fn vabal_high_u32(a: uint64x2_t, b: uint32x4_t, c: uint32x4_t) -> uint64x2_t;
        fn vabal_high_s8(a: int16x8_t, b: int8x16_t, c: int8x16_t) -> int16x8_t;
        fn vabal_high_s16(a: int32x4_t, b: int16x8_t, c: int16x8_t) -> int32x4_t;
        fn vabal_high_s32(a: int64x2_t, b: int32x4_t, c: int32x4_t) -> int64x2_t;
        fn vqabs_s64(a: int64x1_t) -> int64x1_t;
        fn vqabsq_s64(a: int64x2_t) -> int64x2_t;
        fn vqabsb_s8(a: i8) -> i8;
        fn vqabsh_s16(a: i16) -> i16;
        fn vqabss_s32(a: i32) -> i32;
        fn vqabsd_s64(a: i64) -> i64;
        fn vslid_n_s64<const N: i32>(a: i64, b: i64) -> i64;
        fn vslid_n_u64<const N: i32>(a: u64, b: u64) -> u64;
        fn vsrid_n_s64<const N: i32>(a: i64, b: i64) -> i64;
        fn vsrid_n_u64<const N: i32>(a: u64, b: u64) -> u64;
    }

    delegate! {
        fn vcopy_lane_s64<const N1: i32, const N2: i32>(
            _a: int64x1_t,
            b: int64x1_t,
        ) -> int64x1_t;
        fn vcopy_lane_u64<const N1: i32, const N2: i32>(
            _a: uint64x1_t,
            b: uint64x1_t,
        ) -> uint64x1_t;
        fn vcopy_lane_p64<const N1: i32, const N2: i32>(
            _a: poly64x1_t,
            b: poly64x1_t,
        ) -> poly64x1_t;
        fn vcopy_lane_f64<const N1: i32, const N2: i32>(
            _a: float64x1_t,
            b: float64x1_t,
        ) -> float64x1_t;
        fn vcopy_laneq_s64<const LANE1: i32, const LANE2: i32>(
            _a: int64x1_t,
            b: int64x2_t,
        ) -> int64x1_t;
        fn vcopy_laneq_u64<const LANE1: i32, const LANE2: i32>(
            _a: uint64x1_t,
            b: uint64x2_t,
        ) -> uint64x1_t;
        fn vcopy_laneq_p64<const LANE1: i32, const LANE2: i32>(
            _a: poly64x1_t,
            b: poly64x2_t,
        ) -> poly64x1_t;
        fn vcopy_laneq_f64<const LANE1: i32, const LANE2: i32>(
            _a: float64x1_t,
            b: float64x2_t,
        ) -> float64x1_t;
        unsafe fn vld1_s8(ptr: *const i8) -> int8x8_t;
        unsafe fn vld1q_s8(ptr: *const i8) -> int8x16_t;
        unsafe fn vld1_s16(ptr: *const i16) -> int16x4_t;
        unsafe fn vld1q_s16(ptr: *const i16) -> int16x8_t;
        unsafe fn vld1_s32(ptr: *const i32) -> int32x2_t;
        unsafe fn vld1q_s32(ptr: *const i32) -> int32x4_t;
        unsafe fn vld1_s64(ptr: *const i64) -> int64x1_t;
        unsafe fn vld1q_s64(ptr: *const i64) -> int64x2_t;
        unsafe fn vld1_u8(ptr: *const u8) -> uint8x8_t;
        unsafe fn vld1q_u8(ptr: *const u8) -> uint8x16_t;
        unsafe fn vld1_u16(ptr: *const u16) -> uint16x4_t;
        unsafe fn vld1q_u16(ptr: *const u16) -> uint16x8_t;
        unsafe fn vld1_u32(ptr: *const u32) -> uint32x2_t;
        unsafe fn vld1q_u32(ptr: *const u32) -> uint32x4_t;
        unsafe fn vld1_u64(ptr: *const u64) -> uint64x1_t;
        unsafe fn vld1q_u64(ptr: *const u64) -> uint64x2_t;
        unsafe fn vld1_p8(ptr: *const p8) -> poly8x8_t;
        unsafe fn vld1q_p8(ptr: *const p8) -> poly8x16_t;
        unsafe fn vld1_p16(ptr: *const p16) -> poly16x4_t;
        unsafe fn vld1q_p16(ptr: *const p16) -> poly16x8_t;
        unsafe fn vld1_f32(ptr: *const f32) -> float32x2_t;
        unsafe fn vld1q_f32(ptr: *const f32) -> float32x4_t;
        unsafe fn vld1_f64(ptr: *const f64) -> float64x1_t;
        unsafe fn vld1q_f64(ptr: *const f64) -> float64x2_t;
        unsafe fn vld1_dup_f64(ptr: *const f64) -> float64x1_t;
        unsafe fn vld1q_dup_f64(ptr: *const f64) -> float64x2_t;
        unsafe fn vld1_lane_f64<const LANE: i32>(ptr: *const f64, src: float64x1_t) -> float64x1_t;
        unsafe fn vld1q_lane_f64<const LANE: i32>(ptr: *const f64, src: float64x2_t) -> float64x2_t;
        unsafe fn vst1_s8(ptr: *mut i8, a: int8x8_t);
        unsafe fn vst1q_s8(ptr: *mut i8, a: int8x16_t);
        unsafe fn vst1_s16(ptr: *mut i16, a: int16x4_t);
        unsafe fn vst1q_s16(ptr: *mut i16, a: int16x8_t);
        unsafe fn vst1_s32(ptr: *mut i32, a: int32x2_t);
        unsafe fn vst1q_s32(ptr: *mut i32, a: int32x4_t);
        unsafe fn vst1_s64(ptr: *mut i64, a: int64x1_t);
        unsafe fn vst1q_s64(ptr: *mut i64, a: int64x2_t);
        unsafe fn vst1_u8(ptr: *mut u8, a: uint8x8_t);
        unsafe fn vst1q_u8(ptr: *mut u8, a: uint8x16_t);
        unsafe fn vst1_u16(ptr: *mut u16, a: uint16x4_t);
        unsafe fn vst1q_u16(ptr: *mut u16, a: uint16x8_t);
        unsafe fn vst1_u32(ptr: *mut u32, a: uint32x2_t);
        unsafe fn vst1q_u32(ptr: *mut u32, a: uint32x4_t);
        unsafe fn vst1_u64(ptr: *mut u64, a: uint64x1_t);
        unsafe fn vst1q_u64(ptr: *mut u64, a: uint64x2_t);
        unsafe fn vst1_p8(ptr: *mut p8, a: poly8x8_t);
        unsafe fn vst1q_p8(ptr: *mut p8, a: poly8x16_t);
        unsafe fn vst1_p16(ptr: *mut p16, a: poly16x4_t);
        unsafe fn vst1q_p16(ptr: *mut p16, a: poly16x8_t);
        unsafe fn vst1_f32(ptr: *mut f32, a: float32x2_t);
        unsafe fn vst1q_f32(ptr: *mut f32, a: float32x4_t);
        unsafe fn vst1_f64(ptr: *mut f64, a: float64x1_t);
        unsafe fn vst1q_f64(ptr: *mut f64, a: float64x2_t);
        fn vabsd_s64(a: i64) -> i64;
        fn vabs_s64(a: int64x1_t) -> int64x1_t;
        fn vabsq_s64(a: int64x2_t) -> int64x2_t;
        fn vbsl_f64(a: uint64x1_t, b: float64x1_t, c: float64x1_t) -> float64x1_t;
        fn vbsl_p64(a: poly64x1_t, b: poly64x1_t, c: poly64x1_t) -> poly64x1_t;
        fn vbslq_f64(a: uint64x2_t, b: float64x2_t, c: float64x2_t) -> float64x2_t;
        fn vbslq_p64(a: poly64x2_t, b: poly64x2_t, c: poly64x2_t) -> poly64x2_t;
        fn vuqadd_s8(a: int8x8_t, b: uint8x8_t) -> int8x8_t;
        fn vuqaddq_s8(a: int8x16_t, b: uint8x16_t) -> int8x16_t;
        fn vuqadd_s16(a: int16x4_t, b: uint16x4_t) -> int16x4_t;
        fn vuqaddq_s16(a: int16x8_t, b: uint16x8_t) -> int16x8_t;
        fn vuqadd_s32(a: int32x2_t, b: uint32x2_t) -> int32x2_t;
        fn vuqaddq_s32(a: int32x4_t, b: uint32x4_t) -> int32x4_t;
        fn vuqadd_s64(a: int64x1_t, b: uint64x1_t) -> int64x1_t;
        fn vuqaddq_s64(a: int64x2_t, b: uint64x2_t) -> int64x2_t;
        fn vsqadd_u8(a: uint8x8_t, b: int8x8_t) -> uint8x8_t;
        fn vsqaddq_u8(a: uint8x16_t, b: int8x16_t) -> uint8x16_t;
        fn vsqadd_u16(a: uint16x4_t, b: int16x4_t) -> uint16x4_t;
        fn vsqaddq_u16(a: uint16x8_t, b: int16x8_t) -> uint16x8_t;
        fn vsqadd_u32(a: uint32x2_t, b: int32x2_t) -> uint32x2_t;
        fn vsqaddq_u32(a: uint32x4_t, b: int32x4_t) -> uint32x4_t;
        fn vsqadd_u64(a: uint64x1_t, b: int64x1_t) -> uint64x1_t;
        fn vsqaddq_u64(a: uint64x2_t, b: int64x2_t) -> uint64x2_t;
        fn vpaddq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vpaddq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vpaddq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vpaddq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vpaddq_s64(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vpaddq_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vpaddq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vpaddq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vpaddd_s64(a: int64x2_t) -> i64;
        fn vpaddd_u64(a: uint64x2_t) -> u64;
        fn vaddv_s16(a: int16x4_t) -> i16;
        fn vaddv_s32(a: int32x2_t) -> i32;
        fn vaddv_s8(a: int8x8_t) -> i8;
        fn vaddv_u16(a: uint16x4_t) -> u16;
        fn vaddv_u32(a: uint32x2_t) -> u32;
        fn vaddv_u8(a: uint8x8_t) -> u8;
        fn vaddvq_s16(a: int16x8_t) -> i16;
        fn vaddvq_s32(a: int32x4_t) -> i32;
        fn vaddvq_s8(a: int8x16_t) -> i8;
        fn vaddvq_u16(a: uint16x8_t) -> u16;
        fn vaddvq_u32(a: uint32x4_t) -> u32;
        fn vaddvq_u8(a: uint8x16_t) -> u8;
        fn vaddvq_s64(a: int64x2_t) -> i64;
        fn vaddvq_u64(a: uint64x2_t) -> u64;
        fn vaddlv_s8(a: int8x8_t) -> i16;
        fn vaddlvq_s8(a: int8x16_t) -> i16;
        fn vaddlv_u8(a: uint8x8_t) -> u16;
        fn vaddlvq_u8(a: uint8x16_t) -> u16;
        fn vadd_f64(a: float64x1_t, b: float64x1_t) -> float64x1_t;
        fn vaddq_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vadd_s64(a: int64x1_t, b: int64x1_t) -> int64x1_t;
        fn vadd_u64(a: uint64x1_t, b: uint64x1_t) -> uint64x1_t;
        fn vaddd_s64(a: i64, b: i64) -> i64;
        fn vaddd_u64(a: u64, b: u64) -> u64;
        fn vmaxv_s8(a: int8x8_t) -> i8;
        fn vmaxvq_s8(a: int8x16_t) -> i8;
        fn vmaxv_s16(a: int16x4_t) -> i16;
        fn vmaxvq_s16(a: int16x8_t) -> i16;
        fn vmaxv_s32(a: int32x2_t) -> i32;
        fn vmaxvq_s32(a: int32x4_t) -> i32;
        fn vmaxv_u8(a: uint8x8_t) -> u8;
        fn vmaxvq_u8(a: uint8x16_t) -> u8;
        fn vmaxv_u16(a: uint16x4_t) -> u16;
        fn vmaxvq_u16(a: uint16x8_t) -> u16;
        fn vmaxv_u32(a: uint32x2_t) -> u32;
        fn vmaxvq_u32(a: uint32x4_t) -> u32;
        fn vmaxv_f32(a: float32x2_t) -> f32;
        fn vmaxvq_f32(a: float32x4_t) -> f32;
        fn vmaxvq_f64(a: float64x2_t) -> f64;
        fn vminv_s8(a: int8x8_t) -> i8;
        fn vminvq_s8(a: int8x16_t) -> i8;
        fn vminv_s16(a: int16x4_t) -> i16;
        fn vminvq_s16(a: int16x8_t) -> i16;
        fn vminv_s32(a: int32x2_t) -> i32;
        fn vminvq_s32(a: int32x4_t) -> i32;
        fn vminv_u8(a: uint8x8_t) -> u8;
        fn vminvq_u8(a: uint8x16_t) -> u8;
        fn vminv_u16(a: uint16x4_t) -> u16;
        fn vminvq_u16(a: uint16x8_t) -> u16;
        fn vminv_u32(a: uint32x2_t) -> u32;
        fn vminvq_u32(a: uint32x4_t) -> u32;
        fn vminv_f32(a: float32x2_t) -> f32;
        fn vminvq_f32(a: float32x4_t) -> f32;
        fn vminvq_f64(a: float64x2_t) -> f64;
        fn vpminq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vpminq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vpminq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vpminq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vpminq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vpminq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vpminq_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vpminq_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vpmaxq_s8(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vpmaxq_s16(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vpmaxq_s32(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vpmaxq_u8(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vpmaxq_u16(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vpmaxq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vpmaxq_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vpmaxq_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vext_p64<const N: i32>(a: poly64x1_t, _b: poly64x1_t) -> poly64x1_t;
        fn vext_f64<const N: i32>(a: float64x1_t, _b: float64x1_t) -> float64x1_t;
        fn vdup_n_p64(value: p64) -> poly64x1_t;
        fn vdup_n_f64(value: f64) -> float64x1_t;
        fn vdupq_n_p64(value: p64) -> poly64x2_t;
        fn vdupq_n_f64(value: f64) -> float64x2_t;
        fn vmov_n_p64(value: p64) -> poly64x1_t;
        fn vmov_n_f64(value: f64) -> float64x1_t;
        fn vmovq_n_p64(value: p64) -> poly64x2_t;
        fn vmovq_n_f64(value: f64) -> float64x2_t;
        fn vget_high_f64(a: float64x2_t) -> float64x1_t;
        fn vget_high_p64(a: poly64x2_t) -> poly64x1_t;
        fn vget_low_f64(a: float64x2_t) -> float64x1_t;
        fn vget_low_p64(a: poly64x2_t) -> poly64x1_t;
        fn vget_lane_f64<const IMM5: i32>(v: float64x1_t) -> f64;
        fn vgetq_lane_f64<const IMM5: i32>(v: float64x2_t) -> f64;
        fn vcombine_f64(low: float64x1_t, high: float64x1_t) -> float64x2_t;
        fn vtbl1_s8(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vtbl1_u8(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vtbl1_p8(a: poly8x8_t, b: uint8x8_t) -> poly8x8_t;
        fn vtbl2_s8(a: int8x8x2_t, b: int8x8_t) -> int8x8_t;
        fn vtbl2_u8(a: uint8x8x2_t, b: uint8x8_t) -> uint8x8_t;
        fn vtbl2_p8(a: poly8x8x2_t, b: uint8x8_t) -> poly8x8_t;
        fn vtbl3_s8(a: int8x8x3_t, b: int8x8_t) -> int8x8_t;
        fn vtbl3_u8(a: uint8x8x3_t, b: uint8x8_t) -> uint8x8_t;
        fn vtbl3_p8(a: poly8x8x3_t, b: uint8x8_t) -> poly8x8_t;
        fn vtbl4_s8(a: int8x8x4_t, b: int8x8_t) -> int8x8_t;
        fn vtbl4_u8(a: uint8x8x4_t, b: uint8x8_t) -> uint8x8_t;
        fn vtbl4_p8(a: poly8x8x4_t, b: uint8x8_t) -> poly8x8_t;
        fn vtbx1_s8(a: int8x8_t, b: int8x8_t, c: int8x8_t) -> int8x8_t;
        fn vtbx1_u8(a: uint8x8_t, b: uint8x8_t, c: uint8x8_t) -> uint8x8_t;
        fn vtbx1_p8(a: poly8x8_t, b: poly8x8_t, c: uint8x8_t) -> poly8x8_t;
        fn vtbx2_s8(a: int8x8_t, b: int8x8x2_t, c: int8x8_t) -> int8x8_t;
        fn vtbx2_u8(a: uint8x8_t, b: uint8x8x2_t, c: uint8x8_t) -> uint8x8_t;
        fn vtbx2_p8(a: poly8x8_t, b: poly8x8x2_t, c: uint8x8_t) -> poly8x8_t;
        fn vtbx3_s8(a: int8x8_t, b: int8x8x3_t, c: int8x8_t) -> int8x8_t;
        fn vtbx3_u8(a: uint8x8_t, b: uint8x8x3_t, c: uint8x8_t) -> uint8x8_t;
        fn vtbx3_p8(a: poly8x8_t, b: poly8x8x3_t, c: uint8x8_t) -> poly8x8_t;
        fn vtbx4_s8(a: int8x8_t, b: int8x8x4_t, c: int8x8_t) -> int8x8_t;
        fn vtbx4_u8(a: uint8x8_t, b: uint8x8x4_t, c: uint8x8_t) -> uint8x8_t;
        fn vtbx4_p8(a: poly8x8_t, b: poly8x8x4_t, c: uint8x8_t) -> poly8x8_t;
        fn vqtbl1_s8(t: int8x16_t, idx: uint8x8_t) -> int8x8_t;
        fn vqtbl1q_s8(t: int8x16_t, idx: uint8x16_t) -> int8x16_t;
        fn vqtbl1_u8(t: uint8x16_t, idx: uint8x8_t) -> uint8x8_t;
        fn vqtbl1q_u8(t: uint8x16_t, idx: uint8x16_t) -> uint8x16_t;
        fn vqtbl1_p8(t: poly8x16_t, idx: uint8x8_t) -> poly8x8_t;
        fn vqtbl1q_p8(t: poly8x16_t, idx: uint8x16_t) -> poly8x16_t;
        fn vqtbx1_s8(a: int8x8_t, t: int8x16_t, idx: uint8x8_t) -> int8x8_t;
        fn vqtbx1q_s8(a: int8x16_t, t: int8x16_t, idx: uint8x16_t) -> int8x16_t;
        fn vqtbx1_u8(a: uint8x8_t, t: uint8x16_t, idx: uint8x8_t) -> uint8x8_t;
        fn vqtbx1q_u8(a: uint8x16_t, t: uint8x16_t, idx: uint8x16_t) -> uint8x16_t;
        fn vqtbx1_p8(a: poly8x8_t, t: poly8x16_t, idx: uint8x8_t) -> poly8x8_t;
        fn vqtbx1q_p8(a: poly8x16_t, t: poly8x16_t, idx: uint8x16_t) -> poly8x16_t;
        fn vqtbl2_s8(t: int8x16x2_t, idx: uint8x8_t) -> int8x8_t;
        fn vqtbl2q_s8(t: int8x16x2_t, idx: uint8x16_t) -> int8x16_t;
        fn vqtbl2_u8(t: uint8x16x2_t, idx: uint8x8_t) -> uint8x8_t;
        fn vqtbl2q_u8(t: uint8x16x2_t, idx: uint8x16_t) -> uint8x16_t;
        fn vqtbl2_p8(t: poly8x16x2_t, idx: uint8x8_t) -> poly8x8_t;
        fn vqtbl2q_p8(t: poly8x16x2_t, idx: uint8x16_t) -> poly8x16_t;
        fn vqtbx2_s8(a: int8x8_t, t: int8x16x2_t, idx: uint8x8_t) -> int8x8_t;
        fn vqtbx2q_s8(a: int8x16_t, t: int8x16x2_t, idx: uint8x16_t) -> int8x16_t;
        fn vqtbx2_u8(a: uint8x8_t, t: uint8x16x2_t, idx: uint8x8_t) -> uint8x8_t;
        fn vqtbx2q_u8(a: uint8x16_t, t: uint8x16x2_t, idx: uint8x16_t) -> uint8x16_t;
        fn vqtbx2_p8(a: poly8x8_t, t: poly8x16x2_t, idx: uint8x8_t) -> poly8x8_t;
        fn vqtbx2q_p8(a: poly8x16_t, t: poly8x16x2_t, idx: uint8x16_t) -> poly8x16_t;
        fn vqtbl3_s8(t: int8x16x3_t, idx: uint8x8_t) -> int8x8_t;
        fn vqtbl3q_s8(t: int8x16x3_t, idx: uint8x16_t) -> int8x16_t;
        fn vqtbl3_u8(t: uint8x16x3_t, idx: uint8x8_t) -> uint8x8_t;
        fn vqtbl3q_u8(t: uint8x16x3_t, idx: uint8x16_t) -> uint8x16_t;
        fn vqtbl3_p8(t: poly8x16x3_t, idx: uint8x8_t) -> poly8x8_t;
        fn vqtbl3q_p8(t: poly8x16x3_t, idx: uint8x16_t) -> poly8x16_t;
        fn vqtbx3_s8(a: int8x8_t, t: int8x16x3_t, idx: uint8x8_t) -> int8x8_t;
        fn vqtbx3q_s8(a: int8x16_t, t: int8x16x3_t, idx: uint8x16_t) -> int8x16_t;
        fn vqtbx3_u8(a: uint8x8_t, t: uint8x16x3_t, idx: uint8x8_t) -> uint8x8_t;
        fn vqtbx3q_u8(a: uint8x16_t, t: uint8x16x3_t, idx: uint8x16_t) -> uint8x16_t;
        fn vqtbx3_p8(a: poly8x8_t, t: poly8x16x3_t, idx: uint8x8_t) -> poly8x8_t;
        fn vqtbx3q_p8(a: poly8x16_t, t: poly8x16x3_t, idx: uint8x16_t) -> poly8x16_t;
        fn vqtbl4_s8(t: int8x16x4_t, idx: uint8x8_t) -> int8x8_t;
        fn vqtbl4q_s8(t: int8x16x4_t, idx: uint8x16_t) -> int8x16_t;
        fn vqtbl4_u8(t: uint8x16x4_t, idx: uint8x8_t) -> uint8x8_t;
        fn vqtbl4q_u8(t: uint8x16x4_t, idx: uint8x16_t) -> uint8x16_t;
        fn vqtbl4_p8(t: poly8x16x4_t, idx: uint8x8_t) -> poly8x8_t;
        fn vqtbl4q_p8(t: poly8x16x4_t, idx: uint8x16_t) -> poly8x16_t;
        fn vqtbx4_s8(a: int8x8_t, t: int8x16x4_t, idx: uint8x8_t) -> int8x8_t;
        fn vqtbx4q_s8(a: int8x16_t, t: int8x16x4_t, idx: uint8x16_t) -> int8x16_t;
        fn vqtbx4_u8(a: uint8x8_t, t: uint8x16x4_t, idx: uint8x8_t) -> uint8x8_t;
        fn vqtbx4q_u8(a: uint8x16_t, t: uint8x16x4_t, idx: uint8x16_t) -> uint8x16_t;
        fn vqtbx4_p8(a: poly8x8_t, t: poly8x16x4_t, idx: uint8x8_t) -> poly8x8_t;
        fn vqtbx4q_p8(a: poly8x16_t, t: poly8x16x4_t, idx: uint8x16_t) -> poly8x16_t;
        fn vshld_n_s64<const N: i32>(a: i64) -> i64;
        fn vshld_n_u64<const N: i32>(a: u64) -> u64;
        fn vshrd_n_s64<const N: i32>(a: i64) -> i64;
        fn vshrd_n_u64<const N: i32>(a: u64) -> u64;
        fn vsrad_n_s64<const N: i32>(a: i64, b: i64) -> i64;
        fn vsrad_n_u64<const N: i32>(a: u64, b: u64) -> u64;
        fn vsli_n_s8<const N: i32>(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vsliq_n_s8<const N: i32>(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vsli_n_s16<const N: i32>(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vsliq_n_s16<const N: i32>(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vsli_n_s32<const N: i32>(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vsliq_n_s32<const N: i32>(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vsli_n_s64<const N: i32>(a: int64x1_t, b: int64x1_t) -> int64x1_t;
        fn vsliq_n_s64<const N: i32>(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vsli_n_u8<const N: i32>(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vsliq_n_u8<const N: i32>(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vsli_n_u16<const N: i32>(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vsliq_n_u16<const N: i32>(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vsli_n_u32<const N: i32>(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vsliq_n_u32<const N: i32>(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vsli_n_u64<const N: i32>(a: uint64x1_t, b: uint64x1_t) -> uint64x1_t;
        fn vsliq_n_u64<const N: i32>(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vsli_n_p8<const N: i32>(a: poly8x8_t, b: poly8x8_t) -> poly8x8_t;
        fn vsliq_n_p8<const N: i32>(a: poly8x16_t, b: poly8x16_t) -> poly8x16_t;
        fn vsli_n_p16<const N: i32>(a: poly16x4_t, b: poly16x4_t) -> poly16x4_t;
        fn vsliq_n_p16<const N: i32>(a: poly16x8_t, b: poly16x8_t) -> poly16x8_t;
        fn vsri_n_s8<const N: i32>(a: int8x8_t, b: int8x8_t) -> int8x8_t;
        fn vsriq_n_s8<const N: i32>(a: int8x16_t, b: int8x16_t) -> int8x16_t;
        fn vsri_n_s16<const N: i32>(a: int16x4_t, b: int16x4_t) -> int16x4_t;
        fn vsriq_n_s16<const N: i32>(a: int16x8_t, b: int16x8_t) -> int16x8_t;
        fn vsri_n_s32<const N: i32>(a: int32x2_t, b: int32x2_t) -> int32x2_t;
        fn vsriq_n_s32<const N: i32>(a: int32x4_t, b: int32x4_t) -> int32x4_t;
        fn vsri_n_s64<const N: i32>(a: int64x1_t, b: int64x1_t) -> int64x1_t;
        fn vsriq_n_s64<const N: i32>(a: int64x2_t, b: int64x2_t) -> int64x2_t;
        fn vsri_n_u8<const N: i32>(a: uint8x8_t, b: uint8x8_t) -> uint8x8_t;
        fn vsriq_n_u8<const N: i32>(a: uint8x16_t, b: uint8x16_t) -> uint8x16_t;
        fn vsri_n_u16<const N: i32>(a: uint16x4_t, b: uint16x4_t) -> uint16x4_t;
        fn vsriq_n_u16<const N: i32>(a: uint16x8_t, b: uint16x8_t) -> uint16x8_t;
        fn vsri_n_u32<const N: i32>(a: uint32x2_t, b: uint32x2_t) -> uint32x2_t;
        fn vsriq_n_u32<const N: i32>(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vsri_n_u64<const N: i32>(a: uint64x1_t, b: uint64x1_t) -> uint64x1_t;
        fn vsriq_n_u64<const N: i32>(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vsri_n_p8<const N: i32>(a: poly8x8_t, b: poly8x8_t) -> poly8x8_t;
        fn vsriq_n_p8<const N: i32>(a: poly8x16_t, b: poly8x16_t) -> poly8x16_t;
        fn vsri_n_p16<const N: i32>(a: poly16x4_t, b: poly16x4_t) -> poly16x4_t;
        fn vsriq_n_p16<const N: i32>(a: poly16x8_t, b: poly16x8_t) -> poly16x8_t;
    }
}

impl Neon_Aes {
    delegate! {
        fn vcreate_p64(a: u64) -> poly64x1_t;
        unsafe fn vld1_dup_p64(ptr: *const p64) -> poly64x1_t;
        unsafe fn vld1_lane_p64<const LANE: i32>(ptr: *const p64, src: poly64x1_t) -> poly64x1_t;
        unsafe fn vld1_p64(ptr: *const p64) -> poly64x1_t;
        unsafe fn vld1_p64_x2(a: *const p64) -> poly64x1x2_t;
        unsafe fn vld1_p64_x3(a: *const p64) -> poly64x1x3_t;
        unsafe fn vld1_p64_x4(a: *const p64) -> poly64x1x4_t;
        unsafe fn vld1q_dup_p64(ptr: *const p64) -> poly64x2_t;
        unsafe fn vld1q_lane_p64<const LANE: i32>(ptr: *const p64, src: poly64x2_t) -> poly64x2_t;
        unsafe fn vld1q_p64(ptr: *const p64) -> poly64x2_t;
        unsafe fn vld1q_p64_x2(a: *const p64) -> poly64x2x2_t;
        unsafe fn vld1q_p64_x3(a: *const p64) -> poly64x2x3_t;
        unsafe fn vld1q_p64_x4(a: *const p64) -> poly64x2x4_t;
        unsafe fn vld2_dup_p64(a: *const p64) -> poly64x1x2_t;
        unsafe fn vld2_lane_p64<const LANE: i32>(a: *const p64, b: poly64x1x2_t) -> poly64x1x2_t;
        unsafe fn vld2_p64(a: *const p64) -> poly64x1x2_t;
        unsafe fn vld2q_dup_p64(a: *const p64) -> poly64x2x2_t;
        unsafe fn vld2q_lane_p64<const LANE: i32>(a: *const p64, b: poly64x2x2_t) -> poly64x2x2_t;
        unsafe fn vld2q_p64(a: *const p64) -> poly64x2x2_t;
        unsafe fn vld3_dup_p64(a: *const p64) -> poly64x1x3_t;
        unsafe fn vld3_lane_p64<const LANE: i32>(a: *const p64, b: poly64x1x3_t) -> poly64x1x3_t;
        unsafe fn vld3_p64(a: *const p64) -> poly64x1x3_t;
        unsafe fn vld3q_dup_p64(a: *const p64) -> poly64x2x3_t;
        unsafe fn vld3q_lane_p64<const LANE: i32>(a: *const p64, b: poly64x2x3_t) -> poly64x2x3_t;
        unsafe fn vld3q_p64(a: *const p64) -> poly64x2x3_t;
        unsafe fn vld4_dup_p64(a: *const p64) -> poly64x1x4_t;
        unsafe fn vld4_lane_p64<const LANE: i32>(a: *const p64, b: poly64x1x4_t) -> poly64x1x4_t;
        unsafe fn vld4_p64(a: *const p64) -> poly64x1x4_t;
        unsafe fn vld4q_dup_p64(a: *const p64) -> poly64x2x4_t;
        unsafe fn vld4q_lane_p64<const LANE: i32>(a: *const p64, b: poly64x2x4_t) -> poly64x2x4_t;
        unsafe fn vld4q_p64(a: *const p64) -> poly64x2x4_t;
        fn vmull_high_p64(a: poly64x2_t, b: poly64x2_t) -> p128;
        fn vmull_p64(a: p64, b: p64) -> p128;
        fn vreinterpret_p8_p64(a: poly64x1_t) -> poly8x8_t;
        fn vreinterpret_p16_p64(a: poly64x1_t) -> poly16x4_t;
        fn vreinterpret_p64_p8(a: poly8x8_t) -> poly64x1_t;
        fn vreinterpret_p64_p16(a: poly16x4_t) -> poly64x1_t;
        fn vreinterpret_p64_s8(a: int8x8_t) -> poly64x1_t;
        fn vreinterpret_p64_s16(a: int16x4_t) -> poly64x1_t;
        fn vreinterpret_p64_s32(a: int32x2_t) -> poly64x1_t;
        fn vreinterpret_p64_u8(a: uint8x8_t) -> poly64x1_t;
        fn vreinterpret_p64_u16(a: uint16x4_t) -> poly64x1_t;
        fn vreinterpret_p64_u32(a: uint32x2_t) -> poly64x1_t;
        fn vreinterpret_s8_p64(a: poly64x1_t) -> int8x8_t;
        fn vreinterpret_s16_p64(a: poly64x1_t) -> int16x4_t;
        fn vreinterpret_s32_p64(a: poly64x1_t) -> int32x2_t;
        fn vreinterpret_u8_p64(a: poly64x1_t) -> uint8x8_t;
        fn vreinterpret_u16_p64(a: poly64x1_t) -> uint16x4_t;
        fn vreinterpret_u32_p64(a: poly64x1_t) -> uint32x2_t;
        fn vreinterpretq_p8_p64(a: poly64x2_t) -> poly8x16_t;
        fn vreinterpretq_p8_p128(a: p128) -> poly8x16_t;
        fn vreinterpretq_p16_p64(a: poly64x2_t) -> poly16x8_t;
        fn vreinterpretq_p16_p128(a: p128) -> poly16x8_t;
        fn vreinterpretq_p64_p8(a: poly8x16_t) -> poly64x2_t;
        fn vreinterpretq_p64_p16(a: poly16x8_t) -> poly64x2_t;
        fn vreinterpretq_p64_p128(a: p128) -> poly64x2_t;
        fn vreinterpretq_p64_s8(a: int8x16_t) -> poly64x2_t;
        fn vreinterpretq_p64_s16(a: int16x8_t) -> poly64x2_t;
        fn vreinterpretq_p64_s32(a: int32x4_t) -> poly64x2_t;
        fn vreinterpretq_p64_u8(a: uint8x16_t) -> poly64x2_t;
        fn vreinterpretq_p64_u16(a: uint16x8_t) -> poly64x2_t;
        fn vreinterpretq_p64_u32(a: uint32x4_t) -> poly64x2_t;
        fn vreinterpretq_p128_p8(a: poly8x16_t) -> p128;
        fn vreinterpretq_p128_p16(a: poly16x8_t) -> p128;
        fn vreinterpretq_p128_p64(a: poly64x2_t) -> p128;
        fn vreinterpretq_p128_s8(a: int8x16_t) -> p128;
        fn vreinterpretq_p128_u8(a: uint8x16_t) -> p128;
        fn vreinterpretq_p128_s16(a: int16x8_t) -> p128;
        fn vreinterpretq_p128_u16(a: uint16x8_t) -> p128;
        fn vreinterpretq_p128_s32(a: int32x4_t) -> p128;
        fn vreinterpretq_p128_u32(a: uint32x4_t) -> p128;
        fn vreinterpretq_p128_s64(a: int64x2_t) -> p128;
        fn vreinterpretq_p128_u64(a: uint64x2_t) -> p128;
        fn vreinterpretq_s8_p64(a: poly64x2_t) -> int8x16_t;
        fn vreinterpretq_s8_p128(a: p128) -> int8x16_t;
        fn vreinterpretq_s16_p64(a: poly64x2_t) -> int16x8_t;
        fn vreinterpretq_s16_p128(a: p128) -> int16x8_t;
        fn vreinterpretq_s32_p64(a: poly64x2_t) -> int32x4_t;
        fn vreinterpretq_s32_p128(a: p128) -> int32x4_t;
        fn vreinterpretq_s64_p128(a: p128) -> int64x2_t;
        fn vreinterpretq_u8_p64(a: poly64x2_t) -> uint8x16_t;
        fn vreinterpretq_u8_p128(a: p128) -> uint8x16_t;
        fn vreinterpretq_u16_p64(a: poly64x2_t) -> uint16x8_t;
        fn vreinterpretq_u16_p128(a: p128) -> uint16x8_t;
        fn vreinterpretq_u32_p64(a: poly64x2_t) -> uint32x4_t;
        fn vreinterpretq_u32_p128(a: p128) -> uint32x4_t;
        fn vreinterpretq_u64_p128(a: p128) -> uint64x2_t;
        fn vset_lane_p64<const LANE: i32>(a: p64, b: poly64x1_t) -> poly64x1_t;
        fn vsetq_lane_p64<const LANE: i32>(a: p64, b: poly64x2_t) -> poly64x2_t;
        fn vsli_n_p64<const N: i32>(a: poly64x1_t, b: poly64x1_t) -> poly64x1_t;
        fn vsliq_n_p64<const N: i32>(a: poly64x2_t, b: poly64x2_t) -> poly64x2_t;
        fn vsri_n_p64<const N: i32>(a: poly64x1_t, b: poly64x1_t) -> poly64x1_t;
        fn vsriq_n_p64<const N: i32>(a: poly64x2_t, b: poly64x2_t) -> poly64x2_t;
        unsafe fn vst1_lane_p64<const LANE: i32>(a: *mut p64, b: poly64x1_t);
        unsafe fn vst1_p64(ptr: *mut p64, a: poly64x1_t);
        unsafe fn vst1_p64_x2(a: *mut p64, b: poly64x1x2_t);
        unsafe fn vst1_p64_x3(a: *mut p64, b: poly64x1x3_t);
        unsafe fn vst1_p64_x4(a: *mut p64, b: poly64x1x4_t);
        unsafe fn vst1q_p64(ptr: *mut p64, a: poly64x2_t);
        unsafe fn vst1q_p64_x2(a: *mut p64, b: poly64x2x2_t);
        unsafe fn vst1q_p64_x3(a: *mut p64, b: poly64x2x3_t);
        unsafe fn vst1q_p64_x4(a: *mut p64, b: poly64x2x4_t);
        unsafe fn vst2_p64(a: *mut p64, b: poly64x1x2_t);
        unsafe fn vst2_lane_p64<const LANE: i32>(a: *mut p64, b: poly64x1x2_t);
        unsafe fn vst2q_lane_p64<const LANE: i32>(a: *mut p64, b: poly64x2x2_t);
        unsafe fn vst2q_p64(a: *mut p64, b: poly64x2x2_t);
        unsafe fn vst3_lane_p64<const LANE: i32>(a: *mut p64, b: poly64x1x3_t);
        unsafe fn vst3_p64(a: *mut p64, b: poly64x1x3_t);
        unsafe fn vst3q_lane_p64<const LANE: i32>(a: *mut p64, b: poly64x2x3_t);
        unsafe fn vst3q_p64(a: *mut p64, b: poly64x2x3_t);
        unsafe fn vst4_lane_p64<const LANE: i32>(a: *mut p64, b: poly64x1x4_t);
        unsafe fn vst4q_lane_p64<const LANE: i32>(a: *mut p64, b: poly64x2x4_t);
        unsafe fn vst4_p64(a: *mut p64, b: poly64x1x4_t);
        unsafe fn vst4q_p64(a: *mut p64, b: poly64x2x4_t);
    }
}

#[cfg(feature = "nightly")]
impl Neon_Sha3 {
    delegate! {
        fn vbcaxq_s8(a: int8x16_t, b: int8x16_t, c: int8x16_t) -> int8x16_t;
        fn vbcaxq_s16(a: int16x8_t, b: int16x8_t, c: int16x8_t) -> int16x8_t;
        fn vbcaxq_s32(a: int32x4_t, b: int32x4_t, c: int32x4_t) -> int32x4_t;
        fn vbcaxq_s64(a: int64x2_t, b: int64x2_t, c: int64x2_t) -> int64x2_t;
        fn vbcaxq_u8(a: uint8x16_t, b: uint8x16_t, c: uint8x16_t) -> uint8x16_t;
        fn vbcaxq_u16(a: uint16x8_t, b: uint16x8_t, c: uint16x8_t) -> uint16x8_t;
        fn vbcaxq_u32(a: uint32x4_t, b: uint32x4_t, c: uint32x4_t) -> uint32x4_t;
        fn vbcaxq_u64(a: uint64x2_t, b: uint64x2_t, c: uint64x2_t) -> uint64x2_t;
        fn veor3q_s8(a: int8x16_t, b: int8x16_t, c: int8x16_t) -> int8x16_t;
        fn veor3q_s16(a: int16x8_t, b: int16x8_t, c: int16x8_t) -> int16x8_t;
        fn veor3q_s32(a: int32x4_t, b: int32x4_t, c: int32x4_t) -> int32x4_t;
        fn veor3q_s64(a: int64x2_t, b: int64x2_t, c: int64x2_t) -> int64x2_t;
        fn veor3q_u8(a: uint8x16_t, b: uint8x16_t, c: uint8x16_t) -> uint8x16_t;
        fn veor3q_u16(a: uint16x8_t, b: uint16x8_t, c: uint16x8_t) -> uint16x8_t;
        fn veor3q_u32(a: uint32x4_t, b: uint32x4_t, c: uint32x4_t) -> uint32x4_t;
        fn veor3q_u64(a: uint64x2_t, b: uint64x2_t, c: uint64x2_t) -> uint64x2_t;
        fn vrax1q_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vsha512hq_u64(a: uint64x2_t, b: uint64x2_t, c: uint64x2_t) -> uint64x2_t;
        fn vsha512h2q_u64(a: uint64x2_t, b: uint64x2_t, c: uint64x2_t) -> uint64x2_t;
        fn vsha512su0q_u64(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
        fn vsha512su1q_u64(a: uint64x2_t, b: uint64x2_t, c: uint64x2_t) -> uint64x2_t;
        fn vxarq_u64<const IMM6: i32>(a: uint64x2_t, b: uint64x2_t) -> uint64x2_t;
    }
}

#[cfg(feature = "nightly")]
impl Neon_Fcma {
    delegate! {
        fn vcadd_rot270_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vcaddq_rot270_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vcaddq_rot270_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vcadd_rot90_f32(a: float32x2_t, b: float32x2_t) -> float32x2_t;
        fn vcaddq_rot90_f32(a: float32x4_t, b: float32x4_t) -> float32x4_t;
        fn vcaddq_rot90_f64(a: float64x2_t, b: float64x2_t) -> float64x2_t;
        fn vcmla_f32(a: float32x2_t, b: float32x2_t, c: float32x2_t) -> float32x2_t;
        fn vcmlaq_f32(a: float32x4_t, b: float32x4_t, c: float32x4_t) -> float32x4_t;
        fn vcmlaq_f64(a: float64x2_t, b: float64x2_t, c: float64x2_t) -> float64x2_t;
        fn vcmla_rot90_f32(a: float32x2_t, b: float32x2_t, c: float32x2_t) -> float32x2_t;
        fn vcmlaq_rot90_f32(a: float32x4_t, b: float32x4_t, c: float32x4_t) -> float32x4_t;
        fn vcmlaq_rot90_f64(a: float64x2_t, b: float64x2_t, c: float64x2_t) -> float64x2_t;
        fn vcmla_rot180_f32(a: float32x2_t, b: float32x2_t, c: float32x2_t) -> float32x2_t;
        fn vcmlaq_rot180_f32(a: float32x4_t, b: float32x4_t, c: float32x4_t) -> float32x4_t;
        fn vcmlaq_rot180_f64(a: float64x2_t, b: float64x2_t, c: float64x2_t) -> float64x2_t;
        fn vcmla_rot270_f32(a: float32x2_t, b: float32x2_t, c: float32x2_t) -> float32x2_t;
        fn vcmlaq_rot270_f32(a: float32x4_t, b: float32x4_t, c: float32x4_t) -> float32x4_t;
        fn vcmlaq_rot270_f64(a: float64x2_t, b: float64x2_t, c: float64x2_t) -> float64x2_t;
        fn vcmla_lane_f32<const LANE: i32>(
            a: float32x2_t,
            b: float32x2_t,
            c: float32x2_t,
        ) -> float32x2_t;
        fn vcmla_laneq_f32<const LANE: i32>(
            a: float32x2_t,
            b: float32x2_t,
            c: float32x4_t,
        ) -> float32x2_t;
        fn vcmlaq_lane_f32<const LANE: i32>(
            a: float32x4_t,
            b: float32x4_t,
            c: float32x2_t,
        ) -> float32x4_t;
        fn vcmlaq_laneq_f32<const LANE: i32>(
            a: float32x4_t,
            b: float32x4_t,
            c: float32x4_t,
        ) -> float32x4_t;
        fn vcmla_rot90_lane_f32<const LANE: i32>(
            a: float32x2_t,
            b: float32x2_t,
            c: float32x2_t,
        ) -> float32x2_t;
        fn vcmla_rot90_laneq_f32<const LANE: i32>(
            a: float32x2_t,
            b: float32x2_t,
            c: float32x4_t,
        ) -> float32x2_t;
        fn vcmlaq_rot90_lane_f32<const LANE: i32>(
            a: float32x4_t,
            b: float32x4_t,
            c: float32x2_t,
        ) -> float32x4_t;
        fn vcmlaq_rot90_laneq_f32<const LANE: i32>(
            a: float32x4_t,
            b: float32x4_t,
            c: float32x4_t,
        ) -> float32x4_t;
        fn vcmla_rot180_lane_f32<const LANE: i32>(
            a: float32x2_t,
            b: float32x2_t,
            c: float32x2_t,
        ) -> float32x2_t;
        fn vcmla_rot180_laneq_f32<const LANE: i32>(
            a: float32x2_t,
            b: float32x2_t,
            c: float32x4_t,
        ) -> float32x2_t;
        fn vcmlaq_rot180_lane_f32<const LANE: i32>(
            a: float32x4_t,
            b: float32x4_t,
            c: float32x2_t,
        ) -> float32x4_t;
        fn vcmlaq_rot180_laneq_f32<const LANE: i32>(
            a: float32x4_t,
            b: float32x4_t,
            c: float32x4_t,
        ) -> float32x4_t;
        fn vcmla_rot270_lane_f32<const LANE: i32>(
            a: float32x2_t,
            b: float32x2_t,
            c: float32x2_t,
        ) -> float32x2_t;
        fn vcmla_rot270_laneq_f32<const LANE: i32>(
            a: float32x2_t,
            b: float32x2_t,
            c: float32x4_t,
        ) -> float32x2_t;
        fn vcmlaq_rot270_lane_f32<const LANE: i32>(
            a: float32x4_t,
            b: float32x4_t,
            c: float32x2_t,
        ) -> float32x4_t;
        fn vcmlaq_rot270_laneq_f32<const LANE: i32>(
            a: float32x4_t,
            b: float32x4_t,
            c: float32x4_t,
        ) -> float32x4_t;
    }
}

#[cfg(feature = "nightly")]
impl Neon_Dotprod {
    delegate! {
        fn vdot_laneq_s32<const LANE: i32>(a: int32x2_t, b: int8x8_t, c: int8x16_t)
            -> int32x2_t;
        fn vdotq_laneq_s32<const LANE: i32>(
            a: int32x4_t,
            b: int8x16_t,
            c: int8x16_t,
        ) -> int32x4_t;
        fn vdot_laneq_u32<const LANE: i32>(
            a: uint32x2_t,
            b: uint8x8_t,
            c: uint8x16_t,
        ) -> uint32x2_t;
        fn vdotq_laneq_u32<const LANE: i32>(
            a: uint32x4_t,
            b: uint8x16_t,
            c: uint8x16_t,
        ) -> uint32x4_t;
        fn vdot_s32(a: int32x2_t, b: int8x8_t, c: int8x8_t) -> int32x2_t;
        fn vdotq_s32(a: int32x4_t, b: int8x16_t, c: int8x16_t) -> int32x4_t;
        fn vdot_u32(a: uint32x2_t, b: uint8x8_t, c: uint8x8_t) -> uint32x2_t;
        fn vdotq_u32(a: uint32x4_t, b: uint8x16_t, c: uint8x16_t) -> uint32x4_t;
        fn vdot_lane_s32<const LANE: i32>(
            a: int32x2_t,
            b: int8x8_t,
            c: int8x8_t,
        ) -> int32x2_t;
        fn vdotq_lane_s32<const LANE: i32>(
            a: int32x4_t,
            b: int8x16_t,
            c: int8x8_t,
        ) -> int32x4_t;
        fn vdot_lane_u32<const LANE: i32>(
            a: uint32x2_t,
            b: uint8x8_t,
            c: uint8x8_t,
        ) -> uint32x2_t;
        fn vdotq_lane_u32<const LANE: i32>(
            a: uint32x4_t,
            b: uint8x16_t,
            c: uint8x8_t,
        ) -> uint32x4_t;
    }
}

#[cfg(feature = "nightly")]
impl Neon_Sm4 {
    delegate! {
        fn vsm3partw1q_u32(a: uint32x4_t, b: uint32x4_t, c: uint32x4_t) -> uint32x4_t;
        fn vsm3partw2q_u32(a: uint32x4_t, b: uint32x4_t, c: uint32x4_t) -> uint32x4_t;
        fn vsm3ss1q_u32(a: uint32x4_t, b: uint32x4_t, c: uint32x4_t) -> uint32x4_t;
        fn vsm4ekeyq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vsm4eq_u32(a: uint32x4_t, b: uint32x4_t) -> uint32x4_t;
        fn vrnd32x_f32(a: float32x2_t) -> float32x2_t;
        fn vrnd32xq_f32(a: float32x4_t) -> float32x4_t;
        fn vrnd32xq_f64(a: float64x2_t) -> float64x2_t;
        fn vrnd32x_f64(a: float64x1_t) -> float64x1_t;
        fn vrnd32z_f32(a: float32x2_t) -> float32x2_t;
        fn vrnd32zq_f32(a: float32x4_t) -> float32x4_t;
        fn vrnd32zq_f64(a: float64x2_t) -> float64x2_t;
        fn vrnd32z_f64(a: float64x1_t) -> float64x1_t;
        fn vrnd64x_f32(a: float32x2_t) -> float32x2_t;
        fn vrnd64xq_f32(a: float32x4_t) -> float32x4_t;
        fn vrnd64xq_f64(a: float64x2_t) -> float64x2_t;
        fn vrnd64x_f64(a: float64x1_t) -> float64x1_t;
        fn vrnd64z_f32(a: float32x2_t) -> float32x2_t;
        fn vrnd64zq_f32(a: float32x4_t) -> float32x4_t;
        fn vrnd64zq_f64(a: float64x2_t) -> float64x2_t;
        fn vrnd64z_f64(a: float64x1_t) -> float64x1_t;
        fn vsm3tt1aq_u32<const IMM2: i32>(
            a: uint32x4_t,
            b: uint32x4_t,
            c: uint32x4_t,
        ) -> uint32x4_t;
        fn vsm3tt1bq_u32<const IMM2: i32>(
            a: uint32x4_t,
            b: uint32x4_t,
            c: uint32x4_t,
        ) -> uint32x4_t;
        fn vsm3tt2aq_u32<const IMM2: i32>(
            a: uint32x4_t,
            b: uint32x4_t,
            c: uint32x4_t,
        ) -> uint32x4_t;
        fn vsm3tt2bq_u32<const IMM2: i32>(
            a: uint32x4_t,
            b: uint32x4_t,
            c: uint32x4_t,
        ) -> uint32x4_t;
    }
}

#[cfg(feature = "nightly")]
impl Neon_I8mm {
    delegate! {
        fn vusdot_s32(a: int32x2_t, b: uint8x8_t, c: int8x8_t) -> int32x2_t;
        fn vusdotq_s32(a: int32x4_t, b: uint8x16_t, c: int8x16_t) -> int32x4_t;
        fn vusdot_lane_s32<const LANE: i32>(
            a: int32x2_t,
            b: uint8x8_t,
            c: int8x8_t,
        ) -> int32x2_t;
        fn vusdotq_lane_s32<const LANE: i32>(
            a: int32x4_t,
            b: uint8x16_t,
            c: int8x8_t,
        ) -> int32x4_t;
        fn vsudot_lane_s32<const LANE: i32>(
            a: int32x2_t,
            b: int8x8_t,
            c: uint8x8_t,
        ) -> int32x2_t;
        fn vsudotq_lane_s32<const LANE: i32>(
            a: int32x4_t,
            b: int8x16_t,
            c: uint8x8_t,
        ) -> int32x4_t;
        fn vmmlaq_s32(a: int32x4_t, b: int8x16_t, c: int8x16_t) -> int32x4_t;
        fn vmmlaq_u32(a: uint32x4_t, b: uint8x16_t, c: uint8x16_t) -> uint32x4_t;
        fn vusmmlaq_s32(a: int32x4_t, b: uint8x16_t, c: int8x16_t) -> int32x4_t;
        fn vusdot_laneq_s32<const LANE: i32>(
            a: int32x2_t,
            b: uint8x8_t,
            c: int8x16_t,
        ) -> int32x2_t;
        fn vusdotq_laneq_s32<const LANE: i32>(
            a: int32x4_t,
            b: uint8x16_t,
            c: int8x16_t,
        ) -> int32x4_t;
        fn vsudot_laneq_s32<const LANE: i32>(
            a: int32x2_t,
            b: int8x8_t,
            c: uint8x16_t,
        ) -> int32x2_t;
        fn vsudotq_laneq_s32<const LANE: i32>(
            a: int32x4_t,
            b: int8x16_t,
            c: uint8x16_t,
        ) -> int32x4_t;
    }
}
