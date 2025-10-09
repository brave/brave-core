use super::arch::{_MM_CMPINT_ENUM, _MM_MANTISSA_NORM_ENUM, _MM_MANTISSA_SIGN_ENUM, _MM_PERM_ENUM};
use super::*;

impl Avx512f {
    delegate! {
        fn _mm512_abs_epi32(a: __m512i) -> __m512i;
        fn _mm512_mask_abs_epi32(src: __m512i, k: __mmask16, a: __m512i) -> __m512i;
        fn _mm512_maskz_abs_epi32(k: __mmask16, a: __m512i) -> __m512i;
        fn _mm512_abs_epi64(a: __m512i) -> __m512i;
        fn _mm512_mask_abs_epi64(src: __m512i, k: __mmask8, a: __m512i) -> __m512i;
        fn _mm512_maskz_abs_epi64(k: __mmask8, a: __m512i) -> __m512i;
        fn _mm512_abs_ps(v2: __m512) -> __m512;
        fn _mm512_mask_abs_ps(src: __m512, k: __mmask16, v2: __m512) -> __m512;
        fn _mm512_abs_pd(v2: __m512d) -> __m512d;
        fn _mm512_mask_abs_pd(src: __m512d, k: __mmask8, v2: __m512d) -> __m512d;
        fn _mm512_mask_mov_epi32(src: __m512i, k: __mmask16, a: __m512i) -> __m512i;
        fn _mm512_maskz_mov_epi32(k: __mmask16, a: __m512i) -> __m512i;
        fn _mm512_mask_mov_epi64(src: __m512i, k: __mmask8, a: __m512i) -> __m512i;
        fn _mm512_maskz_mov_epi64(k: __mmask8, a: __m512i) -> __m512i;
        fn _mm512_mask_mov_ps(src: __m512, k: __mmask16, a: __m512) -> __m512;
        fn _mm512_maskz_mov_ps(k: __mmask16, a: __m512) -> __m512;
        fn _mm512_mask_mov_pd(src: __m512d, k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_maskz_mov_pd(k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_add_epi32(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_add_epi32(src: __m512i, k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_add_epi32(k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_add_epi64(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_add_epi64(src: __m512i, k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_add_epi64(k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_add_ps(a: __m512, b: __m512) -> __m512;
        fn _mm512_mask_add_ps(src: __m512, k: __mmask16, a: __m512, b: __m512) -> __m512;
        fn _mm512_maskz_add_ps(k: __mmask16, a: __m512, b: __m512) -> __m512;
        fn _mm512_add_pd(a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_mask_add_pd(src: __m512d, k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_maskz_add_pd(k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_sub_epi32(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_sub_epi32(src: __m512i, k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_sub_epi32(k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_sub_epi64(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_sub_epi64(src: __m512i, k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_sub_epi64(k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_sub_ps(a: __m512, b: __m512) -> __m512;
        fn _mm512_mask_sub_ps(src: __m512, k: __mmask16, a: __m512, b: __m512) -> __m512;
        fn _mm512_maskz_sub_ps(k: __mmask16, a: __m512, b: __m512) -> __m512;
        fn _mm512_sub_pd(a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_mask_sub_pd(src: __m512d, k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_maskz_sub_pd(k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_mul_epi32(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_mul_epi32(src: __m512i, k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_mul_epi32(k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mullo_epi32(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_mullo_epi32(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_mullo_epi32(k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mullox_epi64(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_mullox_epi64(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_mul_epu32(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_mul_epu32(src: __m512i, k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_mul_epu32(k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mul_ps(a: __m512, b: __m512) -> __m512;
        fn _mm512_mask_mul_ps(src: __m512, k: __mmask16, a: __m512, b: __m512) -> __m512;
        fn _mm512_maskz_mul_ps(k: __mmask16, a: __m512, b: __m512) -> __m512;
        fn _mm512_mul_pd(a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_mask_mul_pd(src: __m512d, k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_maskz_mul_pd(k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_div_ps(a: __m512, b: __m512) -> __m512;
        fn _mm512_mask_div_ps(src: __m512, k: __mmask16, a: __m512, b: __m512) -> __m512;
        fn _mm512_maskz_div_ps(k: __mmask16, a: __m512, b: __m512) -> __m512;
        fn _mm512_div_pd(a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_mask_div_pd(src: __m512d, k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_maskz_div_pd(k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_max_epi32(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_max_epi32(src: __m512i, k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_max_epi32(k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_max_epi64(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_max_epi64(src: __m512i, k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_max_epi64(k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_max_ps(a: __m512, b: __m512) -> __m512;
        fn _mm512_mask_max_ps(src: __m512, k: __mmask16, a: __m512, b: __m512) -> __m512;
        fn _mm512_maskz_max_ps(k: __mmask16, a: __m512, b: __m512) -> __m512;
        fn _mm512_max_pd(a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_mask_max_pd(src: __m512d, k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_maskz_max_pd(k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_max_epu32(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_max_epu32(src: __m512i, k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_max_epu32(k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_max_epu64(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_max_epu64(src: __m512i, k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_max_epu64(k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_min_epi32(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_min_epi32(src: __m512i, k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_min_epi32(k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_min_epi64(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_min_epi64(src: __m512i, k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_min_epi64(k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_min_ps(a: __m512, b: __m512) -> __m512;
        fn _mm512_mask_min_ps(src: __m512, k: __mmask16, a: __m512, b: __m512) -> __m512;
        fn _mm512_maskz_min_ps(k: __mmask16, a: __m512, b: __m512) -> __m512;
        fn _mm512_min_pd(a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_mask_min_pd(src: __m512d, k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_maskz_min_pd(k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_min_epu32(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_min_epu32(src: __m512i, k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_min_epu32(k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_min_epu64(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_min_epu64(src: __m512i, k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_min_epu64(k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_sqrt_ps(a: __m512) -> __m512;
        fn _mm512_mask_sqrt_ps(src: __m512, k: __mmask16, a: __m512) -> __m512;
        fn _mm512_maskz_sqrt_ps(k: __mmask16, a: __m512) -> __m512;
        fn _mm512_sqrt_pd(a: __m512d) -> __m512d;
        fn _mm512_mask_sqrt_pd(src: __m512d, k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_maskz_sqrt_pd(k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_fmadd_ps(a: __m512, b: __m512, c: __m512) -> __m512;
        fn _mm512_mask_fmadd_ps(a: __m512, k: __mmask16, b: __m512, c: __m512) -> __m512;
        fn _mm512_maskz_fmadd_ps(k: __mmask16, a: __m512, b: __m512, c: __m512) -> __m512;
        fn _mm512_mask3_fmadd_ps(a: __m512, b: __m512, c: __m512, k: __mmask16) -> __m512;
        fn _mm512_fmadd_pd(a: __m512d, b: __m512d, c: __m512d) -> __m512d;
        fn _mm512_mask_fmadd_pd(a: __m512d, k: __mmask8, b: __m512d, c: __m512d) -> __m512d;
        fn _mm512_maskz_fmadd_pd(k: __mmask8, a: __m512d, b: __m512d, c: __m512d) -> __m512d;
        fn _mm512_mask3_fmadd_pd(a: __m512d, b: __m512d, c: __m512d, k: __mmask8) -> __m512d;
        fn _mm512_fmsub_ps(a: __m512, b: __m512, c: __m512) -> __m512;
        fn _mm512_mask_fmsub_ps(a: __m512, k: __mmask16, b: __m512, c: __m512) -> __m512;
        fn _mm512_maskz_fmsub_ps(k: __mmask16, a: __m512, b: __m512, c: __m512) -> __m512;
        fn _mm512_mask3_fmsub_ps(a: __m512, b: __m512, c: __m512, k: __mmask16) -> __m512;
        fn _mm512_fmsub_pd(a: __m512d, b: __m512d, c: __m512d) -> __m512d;
        fn _mm512_mask_fmsub_pd(a: __m512d, k: __mmask8, b: __m512d, c: __m512d) -> __m512d;
        fn _mm512_maskz_fmsub_pd(k: __mmask8, a: __m512d, b: __m512d, c: __m512d) -> __m512d;
        fn _mm512_mask3_fmsub_pd(a: __m512d, b: __m512d, c: __m512d, k: __mmask8) -> __m512d;
        fn _mm512_fmaddsub_ps(a: __m512, b: __m512, c: __m512) -> __m512;
        fn _mm512_mask_fmaddsub_ps(a: __m512, k: __mmask16, b: __m512, c: __m512) -> __m512;
        fn _mm512_maskz_fmaddsub_ps(k: __mmask16, a: __m512, b: __m512, c: __m512) -> __m512;
        fn _mm512_mask3_fmaddsub_ps(a: __m512, b: __m512, c: __m512, k: __mmask16) -> __m512;
        fn _mm512_fmaddsub_pd(a: __m512d, b: __m512d, c: __m512d) -> __m512d;
        fn _mm512_mask_fmaddsub_pd(a: __m512d, k: __mmask8, b: __m512d, c: __m512d) -> __m512d;
        fn _mm512_maskz_fmaddsub_pd(k: __mmask8, a: __m512d, b: __m512d, c: __m512d) -> __m512d;
        fn _mm512_mask3_fmaddsub_pd(a: __m512d, b: __m512d, c: __m512d, k: __mmask8) -> __m512d;
        fn _mm512_fmsubadd_ps(a: __m512, b: __m512, c: __m512) -> __m512;
        fn _mm512_mask_fmsubadd_ps(a: __m512, k: __mmask16, b: __m512, c: __m512) -> __m512;
        fn _mm512_maskz_fmsubadd_ps(k: __mmask16, a: __m512, b: __m512, c: __m512) -> __m512;
        fn _mm512_mask3_fmsubadd_ps(a: __m512, b: __m512, c: __m512, k: __mmask16) -> __m512;
        fn _mm512_fmsubadd_pd(a: __m512d, b: __m512d, c: __m512d) -> __m512d;
        fn _mm512_mask_fmsubadd_pd(a: __m512d, k: __mmask8, b: __m512d, c: __m512d) -> __m512d;
        fn _mm512_maskz_fmsubadd_pd(k: __mmask8, a: __m512d, b: __m512d, c: __m512d) -> __m512d;
        fn _mm512_mask3_fmsubadd_pd(a: __m512d, b: __m512d, c: __m512d, k: __mmask8) -> __m512d;
        fn _mm512_fnmadd_ps(a: __m512, b: __m512, c: __m512) -> __m512;
        fn _mm512_mask_fnmadd_ps(a: __m512, k: __mmask16, b: __m512, c: __m512) -> __m512;
        fn _mm512_maskz_fnmadd_ps(k: __mmask16, a: __m512, b: __m512, c: __m512) -> __m512;
        fn _mm512_mask3_fnmadd_ps(a: __m512, b: __m512, c: __m512, k: __mmask16) -> __m512;
        fn _mm512_fnmadd_pd(a: __m512d, b: __m512d, c: __m512d) -> __m512d;
        fn _mm512_mask_fnmadd_pd(a: __m512d, k: __mmask8, b: __m512d, c: __m512d) -> __m512d;
        fn _mm512_maskz_fnmadd_pd(k: __mmask8, a: __m512d, b: __m512d, c: __m512d) -> __m512d;
        fn _mm512_mask3_fnmadd_pd(a: __m512d, b: __m512d, c: __m512d, k: __mmask8) -> __m512d;
        fn _mm512_fnmsub_ps(a: __m512, b: __m512, c: __m512) -> __m512;
        fn _mm512_mask_fnmsub_ps(a: __m512, k: __mmask16, b: __m512, c: __m512) -> __m512;
        fn _mm512_maskz_fnmsub_ps(k: __mmask16, a: __m512, b: __m512, c: __m512) -> __m512;
        fn _mm512_mask3_fnmsub_ps(a: __m512, b: __m512, c: __m512, k: __mmask16) -> __m512;
        fn _mm512_fnmsub_pd(a: __m512d, b: __m512d, c: __m512d) -> __m512d;
        fn _mm512_mask_fnmsub_pd(a: __m512d, k: __mmask8, b: __m512d, c: __m512d) -> __m512d;
        fn _mm512_maskz_fnmsub_pd(k: __mmask8, a: __m512d, b: __m512d, c: __m512d) -> __m512d;
        fn _mm512_mask3_fnmsub_pd(a: __m512d, b: __m512d, c: __m512d, k: __mmask8) -> __m512d;
        fn _mm512_rcp14_ps(a: __m512) -> __m512;
        fn _mm512_mask_rcp14_ps(src: __m512, k: __mmask16, a: __m512) -> __m512;
        fn _mm512_maskz_rcp14_ps(k: __mmask16, a: __m512) -> __m512;
        fn _mm512_rcp14_pd(a: __m512d) -> __m512d;
        fn _mm512_mask_rcp14_pd(src: __m512d, k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_maskz_rcp14_pd(k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_rsqrt14_ps(a: __m512) -> __m512;
        fn _mm512_mask_rsqrt14_ps(src: __m512, k: __mmask16, a: __m512) -> __m512;
        fn _mm512_maskz_rsqrt14_ps(k: __mmask16, a: __m512) -> __m512;
        fn _mm512_rsqrt14_pd(a: __m512d) -> __m512d;
        fn _mm512_mask_rsqrt14_pd(src: __m512d, k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_maskz_rsqrt14_pd(k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_getexp_ps(a: __m512) -> __m512;
        fn _mm512_mask_getexp_ps(src: __m512, k: __mmask16, a: __m512) -> __m512;
        fn _mm512_maskz_getexp_ps(k: __mmask16, a: __m512) -> __m512;
        fn _mm512_getexp_pd(a: __m512d) -> __m512d;
        fn _mm512_mask_getexp_pd(src: __m512d, k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_maskz_getexp_pd(k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_roundscale_ps<const IMM8: i32>(a: __m512) -> __m512;
        fn _mm512_mask_roundscale_ps<const IMM8: i32>(
            src: __m512,
            k: __mmask16,
            a: __m512,
        ) -> __m512;
        fn _mm512_maskz_roundscale_ps<const IMM8: i32>(k: __mmask16, a: __m512) -> __m512;
        fn _mm512_roundscale_pd<const IMM8: i32>(a: __m512d) -> __m512d;
        fn _mm512_mask_roundscale_pd<const IMM8: i32>(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_roundscale_pd<const IMM8: i32>(k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_scalef_ps(a: __m512, b: __m512) -> __m512;
        fn _mm512_mask_scalef_ps(src: __m512, k: __mmask16, a: __m512, b: __m512) -> __m512;
        fn _mm512_maskz_scalef_ps(k: __mmask16, a: __m512, b: __m512) -> __m512;
        fn _mm512_scalef_pd(a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_mask_scalef_pd(src: __m512d, k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_maskz_scalef_pd(k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_fixupimm_ps<const IMM8: i32>(a: __m512, b: __m512, c: __m512i) -> __m512;
        fn _mm512_mask_fixupimm_ps<const IMM8: i32>(
            a: __m512,
            k: __mmask16,
            b: __m512,
            c: __m512i,
        ) -> __m512;
        fn _mm512_maskz_fixupimm_ps<const IMM8: i32>(
            k: __mmask16,
            a: __m512,
            b: __m512,
            c: __m512i,
        ) -> __m512;
        fn _mm512_fixupimm_pd<const IMM8: i32>(a: __m512d, b: __m512d, c: __m512i) -> __m512d;
        fn _mm512_mask_fixupimm_pd<const IMM8: i32>(
            a: __m512d,
            k: __mmask8,
            b: __m512d,
            c: __m512i,
        ) -> __m512d;
        fn _mm512_maskz_fixupimm_pd<const IMM8: i32>(
            k: __mmask8,
            a: __m512d,
            b: __m512d,
            c: __m512i,
        ) -> __m512d;
        fn _mm512_ternarylogic_epi32<const IMM8: i32>(
            a: __m512i,
            b: __m512i,
            c: __m512i,
        ) -> __m512i;
        fn _mm512_mask_ternarylogic_epi32<const IMM8: i32>(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_ternarylogic_epi32<const IMM8: i32>(
            k: __mmask16,
            a: __m512i,
            b: __m512i,
            c: __m512i,
        ) -> __m512i;
        fn _mm512_ternarylogic_epi64<const IMM8: i32>(
            a: __m512i,
            b: __m512i,
            c: __m512i,
        ) -> __m512i;
        fn _mm512_mask_ternarylogic_epi64<const IMM8: i32>(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_ternarylogic_epi64<const IMM8: i32>(
            k: __mmask8,
            a: __m512i,
            b: __m512i,
            c: __m512i,
        ) -> __m512i;
        fn _mm512_getmant_ps<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            a: __m512,
        ) -> __m512;
        fn _mm512_mask_getmant_ps<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            src: __m512,
            k: __mmask16,
            a: __m512,
        ) -> __m512;
        fn _mm512_maskz_getmant_ps<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            k: __mmask16,
            a: __m512,
        ) -> __m512;
        fn _mm512_getmant_pd<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            a: __m512d,
        ) -> __m512d;
        fn _mm512_mask_getmant_pd<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_getmant_pd<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            k: __mmask8,
            a: __m512d,
        ) -> __m512d;
        fn _mm512_add_round_ps<const ROUNDING: i32>(a: __m512, b: __m512) -> __m512;
        fn _mm512_mask_add_round_ps<const ROUNDING: i32>(
            src: __m512,
            k: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __m512;
        fn _mm512_maskz_add_round_ps<const ROUNDING: i32>(
            k: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __m512;
        fn _mm512_add_round_pd<const ROUNDING: i32>(a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_mask_add_round_pd<const ROUNDING: i32>(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_add_round_pd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_sub_round_ps<const ROUNDING: i32>(a: __m512, b: __m512) -> __m512;
        fn _mm512_mask_sub_round_ps<const ROUNDING: i32>(
            src: __m512,
            k: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __m512;
        fn _mm512_maskz_sub_round_ps<const ROUNDING: i32>(
            k: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __m512;
        fn _mm512_sub_round_pd<const ROUNDING: i32>(a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_mask_sub_round_pd<const ROUNDING: i32>(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_sub_round_pd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_mul_round_ps<const ROUNDING: i32>(a: __m512, b: __m512) -> __m512;
        fn _mm512_mask_mul_round_ps<const ROUNDING: i32>(
            src: __m512,
            k: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __m512;
        fn _mm512_maskz_mul_round_ps<const ROUNDING: i32>(
            k: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __m512;
        fn _mm512_mul_round_pd<const ROUNDING: i32>(a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_mask_mul_round_pd<const ROUNDING: i32>(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_mul_round_pd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_div_round_ps<const ROUNDING: i32>(a: __m512, b: __m512) -> __m512;
        fn _mm512_mask_div_round_ps<const ROUNDING: i32>(
            src: __m512,
            k: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __m512;
        fn _mm512_maskz_div_round_ps<const ROUNDING: i32>(
            k: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __m512;
        fn _mm512_div_round_pd<const ROUNDING: i32>(a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_mask_div_round_pd<const ROUNDING: i32>(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_div_round_pd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_sqrt_round_ps<const ROUNDING: i32>(a: __m512) -> __m512;
        fn _mm512_mask_sqrt_round_ps<const ROUNDING: i32>(
            src: __m512,
            k: __mmask16,
            a: __m512,
        ) -> __m512;
        fn _mm512_maskz_sqrt_round_ps<const ROUNDING: i32>(k: __mmask16, a: __m512) -> __m512;
        fn _mm512_sqrt_round_pd<const ROUNDING: i32>(a: __m512d) -> __m512d;
        fn _mm512_mask_sqrt_round_pd<const ROUNDING: i32>(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_sqrt_round_pd<const ROUNDING: i32>(k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_fmadd_round_ps<const ROUNDING: i32>(
            a: __m512,
            b: __m512,
            c: __m512,
        ) -> __m512;
        fn _mm512_mask_fmadd_round_ps<const ROUNDING: i32>(
            a: __m512,
            k: __mmask16,
            b: __m512,
            c: __m512,
        ) -> __m512;
        fn _mm512_maskz_fmadd_round_ps<const ROUNDING: i32>(
            k: __mmask16,
            a: __m512,
            b: __m512,
            c: __m512,
        ) -> __m512;
        fn _mm512_mask3_fmadd_round_ps<const ROUNDING: i32>(
            a: __m512,
            b: __m512,
            c: __m512,
            k: __mmask16,
        ) -> __m512;
        fn _mm512_fmadd_round_pd<const ROUNDING: i32>(
            a: __m512d,
            b: __m512d,
            c: __m512d,
        ) -> __m512d;
        fn _mm512_mask_fmadd_round_pd<const ROUNDING: i32>(
            a: __m512d,
            k: __mmask8,
            b: __m512d,
            c: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_fmadd_round_pd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m512d,
            b: __m512d,
            c: __m512d,
        ) -> __m512d;
        fn _mm512_mask3_fmadd_round_pd<const ROUNDING: i32>(
            a: __m512d,
            b: __m512d,
            c: __m512d,
            k: __mmask8,
        ) -> __m512d;
        fn _mm512_fmsub_round_ps<const ROUNDING: i32>(
            a: __m512,
            b: __m512,
            c: __m512,
        ) -> __m512;
        fn _mm512_mask_fmsub_round_ps<const ROUNDING: i32>(
            a: __m512,
            k: __mmask16,
            b: __m512,
            c: __m512,
        ) -> __m512;
        fn _mm512_maskz_fmsub_round_ps<const ROUNDING: i32>(
            k: __mmask16,
            a: __m512,
            b: __m512,
            c: __m512,
        ) -> __m512;
        fn _mm512_mask3_fmsub_round_ps<const ROUNDING: i32>(
            a: __m512,
            b: __m512,
            c: __m512,
            k: __mmask16,
        ) -> __m512;
        fn _mm512_fmsub_round_pd<const ROUNDING: i32>(
            a: __m512d,
            b: __m512d,
            c: __m512d,
        ) -> __m512d;
        fn _mm512_mask_fmsub_round_pd<const ROUNDING: i32>(
            a: __m512d,
            k: __mmask8,
            b: __m512d,
            c: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_fmsub_round_pd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m512d,
            b: __m512d,
            c: __m512d,
        ) -> __m512d;
        fn _mm512_mask3_fmsub_round_pd<const ROUNDING: i32>(
            a: __m512d,
            b: __m512d,
            c: __m512d,
            k: __mmask8,
        ) -> __m512d;
        fn _mm512_fmaddsub_round_ps<const ROUNDING: i32>(
            a: __m512,
            b: __m512,
            c: __m512,
        ) -> __m512;
        fn _mm512_mask_fmaddsub_round_ps<const ROUNDING: i32>(
            a: __m512,
            k: __mmask16,
            b: __m512,
            c: __m512,
        ) -> __m512;
        fn _mm512_maskz_fmaddsub_round_ps<const ROUNDING: i32>(
            k: __mmask16,
            a: __m512,
            b: __m512,
            c: __m512,
        ) -> __m512;
        fn _mm512_mask3_fmaddsub_round_ps<const ROUNDING: i32>(
            a: __m512,
            b: __m512,
            c: __m512,
            k: __mmask16,
        ) -> __m512;
        fn _mm512_fmaddsub_round_pd<const ROUNDING: i32>(
            a: __m512d,
            b: __m512d,
            c: __m512d,
        ) -> __m512d;
        fn _mm512_mask_fmaddsub_round_pd<const ROUNDING: i32>(
            a: __m512d,
            k: __mmask8,
            b: __m512d,
            c: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_fmaddsub_round_pd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m512d,
            b: __m512d,
            c: __m512d,
        ) -> __m512d;
        fn _mm512_mask3_fmaddsub_round_pd<const ROUNDING: i32>(
            a: __m512d,
            b: __m512d,
            c: __m512d,
            k: __mmask8,
        ) -> __m512d;
        fn _mm512_fmsubadd_round_ps<const ROUNDING: i32>(
            a: __m512,
            b: __m512,
            c: __m512,
        ) -> __m512;
        fn _mm512_mask_fmsubadd_round_ps<const ROUNDING: i32>(
            a: __m512,
            k: __mmask16,
            b: __m512,
            c: __m512,
        ) -> __m512;
        fn _mm512_maskz_fmsubadd_round_ps<const ROUNDING: i32>(
            k: __mmask16,
            a: __m512,
            b: __m512,
            c: __m512,
        ) -> __m512;
        fn _mm512_mask3_fmsubadd_round_ps<const ROUNDING: i32>(
            a: __m512,
            b: __m512,
            c: __m512,
            k: __mmask16,
        ) -> __m512;
        fn _mm512_fmsubadd_round_pd<const ROUNDING: i32>(
            a: __m512d,
            b: __m512d,
            c: __m512d,
        ) -> __m512d;
        fn _mm512_mask_fmsubadd_round_pd<const ROUNDING: i32>(
            a: __m512d,
            k: __mmask8,
            b: __m512d,
            c: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_fmsubadd_round_pd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m512d,
            b: __m512d,
            c: __m512d,
        ) -> __m512d;
        fn _mm512_mask3_fmsubadd_round_pd<const ROUNDING: i32>(
            a: __m512d,
            b: __m512d,
            c: __m512d,
            k: __mmask8,
        ) -> __m512d;
        fn _mm512_fnmadd_round_ps<const ROUNDING: i32>(
            a: __m512,
            b: __m512,
            c: __m512,
        ) -> __m512;
        fn _mm512_mask_fnmadd_round_ps<const ROUNDING: i32>(
            a: __m512,
            k: __mmask16,
            b: __m512,
            c: __m512,
        ) -> __m512;
        fn _mm512_maskz_fnmadd_round_ps<const ROUNDING: i32>(
            k: __mmask16,
            a: __m512,
            b: __m512,
            c: __m512,
        ) -> __m512;
        fn _mm512_mask3_fnmadd_round_ps<const ROUNDING: i32>(
            a: __m512,
            b: __m512,
            c: __m512,
            k: __mmask16,
        ) -> __m512;
        fn _mm512_fnmadd_round_pd<const ROUNDING: i32>(
            a: __m512d,
            b: __m512d,
            c: __m512d,
        ) -> __m512d;
        fn _mm512_mask_fnmadd_round_pd<const ROUNDING: i32>(
            a: __m512d,
            k: __mmask8,
            b: __m512d,
            c: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_fnmadd_round_pd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m512d,
            b: __m512d,
            c: __m512d,
        ) -> __m512d;
        fn _mm512_mask3_fnmadd_round_pd<const ROUNDING: i32>(
            a: __m512d,
            b: __m512d,
            c: __m512d,
            k: __mmask8,
        ) -> __m512d;
        fn _mm512_fnmsub_round_ps<const ROUNDING: i32>(
            a: __m512,
            b: __m512,
            c: __m512,
        ) -> __m512;
        fn _mm512_mask_fnmsub_round_ps<const ROUNDING: i32>(
            a: __m512,
            k: __mmask16,
            b: __m512,
            c: __m512,
        ) -> __m512;
        fn _mm512_maskz_fnmsub_round_ps<const ROUNDING: i32>(
            k: __mmask16,
            a: __m512,
            b: __m512,
            c: __m512,
        ) -> __m512;
        fn _mm512_mask3_fnmsub_round_ps<const ROUNDING: i32>(
            a: __m512,
            b: __m512,
            c: __m512,
            k: __mmask16,
        ) -> __m512;
        fn _mm512_fnmsub_round_pd<const ROUNDING: i32>(
            a: __m512d,
            b: __m512d,
            c: __m512d,
        ) -> __m512d;
        fn _mm512_mask_fnmsub_round_pd<const ROUNDING: i32>(
            a: __m512d,
            k: __mmask8,
            b: __m512d,
            c: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_fnmsub_round_pd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m512d,
            b: __m512d,
            c: __m512d,
        ) -> __m512d;
        fn _mm512_mask3_fnmsub_round_pd<const ROUNDING: i32>(
            a: __m512d,
            b: __m512d,
            c: __m512d,
            k: __mmask8,
        ) -> __m512d;
        fn _mm512_max_round_ps<const SAE: i32>(a: __m512, b: __m512) -> __m512;
        fn _mm512_mask_max_round_ps<const SAE: i32>(
            src: __m512,
            k: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __m512;
        fn _mm512_maskz_max_round_ps<const SAE: i32>(
            k: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __m512;
        fn _mm512_max_round_pd<const SAE: i32>(a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_mask_max_round_pd<const SAE: i32>(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_max_round_pd<const SAE: i32>(
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_min_round_ps<const SAE: i32>(a: __m512, b: __m512) -> __m512;
        fn _mm512_mask_min_round_ps<const SAE: i32>(
            src: __m512,
            k: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __m512;
        fn _mm512_maskz_min_round_ps<const SAE: i32>(
            k: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __m512;
        fn _mm512_min_round_pd<const SAE: i32>(a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_mask_min_round_pd<const SAE: i32>(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_min_round_pd<const SAE: i32>(
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_getexp_round_ps<const SAE: i32>(a: __m512) -> __m512;
        fn _mm512_mask_getexp_round_ps<const SAE: i32>(
            src: __m512,
            k: __mmask16,
            a: __m512,
        ) -> __m512;
        fn _mm512_maskz_getexp_round_ps<const SAE: i32>(k: __mmask16, a: __m512) -> __m512;
        fn _mm512_getexp_round_pd<const SAE: i32>(a: __m512d) -> __m512d;
        fn _mm512_mask_getexp_round_pd<const SAE: i32>(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_getexp_round_pd<const SAE: i32>(k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_roundscale_round_ps<const IMM8: i32, const SAE: i32>(a: __m512) -> __m512;
        fn _mm512_mask_roundscale_round_ps<const IMM8: i32, const SAE: i32>(
            src: __m512,
            k: __mmask16,
            a: __m512,
        ) -> __m512;
        fn _mm512_maskz_roundscale_round_ps<const IMM8: i32, const SAE: i32>(
            k: __mmask16,
            a: __m512,
        ) -> __m512;
        fn _mm512_roundscale_round_pd<const IMM8: i32, const SAE: i32>(a: __m512d) -> __m512d;
        fn _mm512_mask_roundscale_round_pd<const IMM8: i32, const SAE: i32>(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_roundscale_round_pd<const IMM8: i32, const SAE: i32>(
            k: __mmask8,
            a: __m512d,
        ) -> __m512d;
        fn _mm512_scalef_round_ps<const ROUNDING: i32>(a: __m512, b: __m512) -> __m512;
        fn _mm512_mask_scalef_round_ps<const ROUNDING: i32>(
            src: __m512,
            k: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __m512;
        fn _mm512_maskz_scalef_round_ps<const ROUNDING: i32>(
            k: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __m512;
        fn _mm512_scalef_round_pd<const ROUNDING: i32>(a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_mask_scalef_round_pd<const ROUNDING: i32>(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_scalef_round_pd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_fixupimm_round_ps<const IMM8: i32, const SAE: i32>(
            a: __m512,
            b: __m512,
            c: __m512i,
        ) -> __m512;
        fn _mm512_mask_fixupimm_round_ps<const IMM8: i32, const SAE: i32>(
            a: __m512,
            k: __mmask16,
            b: __m512,
            c: __m512i,
        ) -> __m512;
        fn _mm512_maskz_fixupimm_round_ps<const IMM8: i32, const SAE: i32>(
            k: __mmask16,
            a: __m512,
            b: __m512,
            c: __m512i,
        ) -> __m512;
        fn _mm512_fixupimm_round_pd<const IMM8: i32, const SAE: i32>(
            a: __m512d,
            b: __m512d,
            c: __m512i,
        ) -> __m512d;
        fn _mm512_mask_fixupimm_round_pd<const IMM8: i32, const SAE: i32>(
            a: __m512d,
            k: __mmask8,
            b: __m512d,
            c: __m512i,
        ) -> __m512d;
        fn _mm512_maskz_fixupimm_round_pd<const IMM8: i32, const SAE: i32>(
            k: __mmask8,
            a: __m512d,
            b: __m512d,
            c: __m512i,
        ) -> __m512d;
        fn _mm512_getmant_round_ps<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
            const SAE: i32,
        >(
            a: __m512,
        ) -> __m512;
        fn _mm512_mask_getmant_round_ps<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
            const SAE: i32,
        >(
            src: __m512,
            k: __mmask16,
            a: __m512,
        ) -> __m512;
        fn _mm512_maskz_getmant_round_ps<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
            const SAE: i32,
        >(
            k: __mmask16,
            a: __m512,
        ) -> __m512;
        fn _mm512_getmant_round_pd<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
            const SAE: i32,
        >(
            a: __m512d,
        ) -> __m512d;
        fn _mm512_mask_getmant_round_pd<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
            const SAE: i32,
        >(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_getmant_round_pd<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
            const SAE: i32,
        >(
            k: __mmask8,
            a: __m512d,
        ) -> __m512d;
        fn _mm512_cvtps_epi32(a: __m512) -> __m512i;
        fn _mm512_mask_cvtps_epi32(src: __m512i, k: __mmask16, a: __m512) -> __m512i;
        fn _mm512_maskz_cvtps_epi32(k: __mmask16, a: __m512) -> __m512i;
        fn _mm512_cvtps_epu32(a: __m512) -> __m512i;
        fn _mm512_mask_cvtps_epu32(src: __m512i, k: __mmask16, a: __m512) -> __m512i;
        fn _mm512_maskz_cvtps_epu32(k: __mmask16, a: __m512) -> __m512i;
        fn _mm512_cvtps_pd(a: __m256) -> __m512d;
        fn _mm512_mask_cvtps_pd(src: __m512d, k: __mmask8, a: __m256) -> __m512d;
        fn _mm512_maskz_cvtps_pd(k: __mmask8, a: __m256) -> __m512d;
        fn _mm512_cvtpslo_pd(v2: __m512) -> __m512d;
        fn _mm512_mask_cvtpslo_pd(src: __m512d, k: __mmask8, v2: __m512) -> __m512d;
        fn _mm512_cvtpd_ps(a: __m512d) -> __m256;
        fn _mm512_mask_cvtpd_ps(src: __m256, k: __mmask8, a: __m512d) -> __m256;
        fn _mm512_maskz_cvtpd_ps(k: __mmask8, a: __m512d) -> __m256;
        fn _mm512_cvtpd_epi32(a: __m512d) -> __m256i;
        fn _mm512_mask_cvtpd_epi32(src: __m256i, k: __mmask8, a: __m512d) -> __m256i;
        fn _mm512_maskz_cvtpd_epi32(k: __mmask8, a: __m512d) -> __m256i;
        fn _mm512_cvtpd_epu32(a: __m512d) -> __m256i;
        fn _mm512_mask_cvtpd_epu32(src: __m256i, k: __mmask8, a: __m512d) -> __m256i;
        fn _mm512_maskz_cvtpd_epu32(k: __mmask8, a: __m512d) -> __m256i;
        fn _mm512_cvtpd_pslo(v2: __m512d) -> __m512;
        fn _mm512_mask_cvtpd_pslo(src: __m512, k: __mmask8, v2: __m512d) -> __m512;
        fn _mm512_cvtepi8_epi32(a: __m128i) -> __m512i;
        fn _mm512_mask_cvtepi8_epi32(src: __m512i, k: __mmask16, a: __m128i) -> __m512i;
        fn _mm512_maskz_cvtepi8_epi32(k: __mmask16, a: __m128i) -> __m512i;
        fn _mm512_cvtepi8_epi64(a: __m128i) -> __m512i;
        fn _mm512_mask_cvtepi8_epi64(src: __m512i, k: __mmask8, a: __m128i) -> __m512i;
        fn _mm512_maskz_cvtepi8_epi64(k: __mmask8, a: __m128i) -> __m512i;
        fn _mm512_cvtepu8_epi32(a: __m128i) -> __m512i;
        fn _mm512_mask_cvtepu8_epi32(src: __m512i, k: __mmask16, a: __m128i) -> __m512i;
        fn _mm512_maskz_cvtepu8_epi32(k: __mmask16, a: __m128i) -> __m512i;
        fn _mm512_cvtepu8_epi64(a: __m128i) -> __m512i;
        fn _mm512_mask_cvtepu8_epi64(src: __m512i, k: __mmask8, a: __m128i) -> __m512i;
        fn _mm512_maskz_cvtepu8_epi64(k: __mmask8, a: __m128i) -> __m512i;
        fn _mm512_cvtepi16_epi32(a: __m256i) -> __m512i;
        fn _mm512_mask_cvtepi16_epi32(src: __m512i, k: __mmask16, a: __m256i) -> __m512i;
        fn _mm512_maskz_cvtepi16_epi32(k: __mmask16, a: __m256i) -> __m512i;
        fn _mm512_cvtepi16_epi64(a: __m128i) -> __m512i;
        fn _mm512_mask_cvtepi16_epi64(src: __m512i, k: __mmask8, a: __m128i) -> __m512i;
        fn _mm512_maskz_cvtepi16_epi64(k: __mmask8, a: __m128i) -> __m512i;
        fn _mm512_cvtepu16_epi32(a: __m256i) -> __m512i;
        fn _mm512_mask_cvtepu16_epi32(src: __m512i, k: __mmask16, a: __m256i) -> __m512i;
        fn _mm512_maskz_cvtepu16_epi32(k: __mmask16, a: __m256i) -> __m512i;
        fn _mm512_cvtepu16_epi64(a: __m128i) -> __m512i;
        fn _mm512_mask_cvtepu16_epi64(src: __m512i, k: __mmask8, a: __m128i) -> __m512i;
        fn _mm512_maskz_cvtepu16_epi64(k: __mmask8, a: __m128i) -> __m512i;
        fn _mm512_cvtepi32_epi64(a: __m256i) -> __m512i;
        fn _mm512_mask_cvtepi32_epi64(src: __m512i, k: __mmask8, a: __m256i) -> __m512i;
        fn _mm512_maskz_cvtepi32_epi64(k: __mmask8, a: __m256i) -> __m512i;
        fn _mm512_cvtepu32_epi64(a: __m256i) -> __m512i;
        fn _mm512_mask_cvtepu32_epi64(src: __m512i, k: __mmask8, a: __m256i) -> __m512i;
        fn _mm512_maskz_cvtepu32_epi64(k: __mmask8, a: __m256i) -> __m512i;
        fn _mm512_cvtepi32_ps(a: __m512i) -> __m512;
        fn _mm512_mask_cvtepi32_ps(src: __m512, k: __mmask16, a: __m512i) -> __m512;
        fn _mm512_maskz_cvtepi32_ps(k: __mmask16, a: __m512i) -> __m512;
        fn _mm512_cvtepi32_pd(a: __m256i) -> __m512d;
        fn _mm512_mask_cvtepi32_pd(src: __m512d, k: __mmask8, a: __m256i) -> __m512d;
        fn _mm512_maskz_cvtepi32_pd(k: __mmask8, a: __m256i) -> __m512d;
        fn _mm512_cvtepu32_ps(a: __m512i) -> __m512;
        fn _mm512_mask_cvtepu32_ps(src: __m512, k: __mmask16, a: __m512i) -> __m512;
        fn _mm512_maskz_cvtepu32_ps(k: __mmask16, a: __m512i) -> __m512;
        fn _mm512_cvtepu32_pd(a: __m256i) -> __m512d;
        fn _mm512_mask_cvtepu32_pd(src: __m512d, k: __mmask8, a: __m256i) -> __m512d;
        fn _mm512_maskz_cvtepu32_pd(k: __mmask8, a: __m256i) -> __m512d;
        fn _mm512_cvtepi32lo_pd(v2: __m512i) -> __m512d;
        fn _mm512_mask_cvtepi32lo_pd(src: __m512d, k: __mmask8, v2: __m512i) -> __m512d;
        fn _mm512_cvtepu32lo_pd(v2: __m512i) -> __m512d;
        fn _mm512_mask_cvtepu32lo_pd(src: __m512d, k: __mmask8, v2: __m512i) -> __m512d;
        fn _mm512_cvtepi32_epi16(a: __m512i) -> __m256i;
        fn _mm512_mask_cvtepi32_epi16(src: __m256i, k: __mmask16, a: __m512i) -> __m256i;
        fn _mm512_maskz_cvtepi32_epi16(k: __mmask16, a: __m512i) -> __m256i;
        fn _mm512_cvtepi32_epi8(a: __m512i) -> __m128i;
        fn _mm512_mask_cvtepi32_epi8(src: __m128i, k: __mmask16, a: __m512i) -> __m128i;
        fn _mm512_maskz_cvtepi32_epi8(k: __mmask16, a: __m512i) -> __m128i;
        fn _mm512_cvtepi64_epi32(a: __m512i) -> __m256i;
        fn _mm512_mask_cvtepi64_epi32(src: __m256i, k: __mmask8, a: __m512i) -> __m256i;
        fn _mm512_maskz_cvtepi64_epi32(k: __mmask8, a: __m512i) -> __m256i;
        fn _mm512_cvtepi64_epi16(a: __m512i) -> __m128i;
        fn _mm512_mask_cvtepi64_epi16(src: __m128i, k: __mmask8, a: __m512i) -> __m128i;
        fn _mm512_maskz_cvtepi64_epi16(k: __mmask8, a: __m512i) -> __m128i;
        fn _mm512_cvtepi64_epi8(a: __m512i) -> __m128i;
        fn _mm512_mask_cvtepi64_epi8(src: __m128i, k: __mmask8, a: __m512i) -> __m128i;
        fn _mm512_maskz_cvtepi64_epi8(k: __mmask8, a: __m512i) -> __m128i;
        fn _mm512_cvtsepi32_epi16(a: __m512i) -> __m256i;
        fn _mm512_mask_cvtsepi32_epi16(src: __m256i, k: __mmask16, a: __m512i) -> __m256i;
        fn _mm512_maskz_cvtsepi32_epi16(k: __mmask16, a: __m512i) -> __m256i;
        fn _mm512_cvtsepi32_epi8(a: __m512i) -> __m128i;
        fn _mm512_mask_cvtsepi32_epi8(src: __m128i, k: __mmask16, a: __m512i) -> __m128i;
        fn _mm512_maskz_cvtsepi32_epi8(k: __mmask16, a: __m512i) -> __m128i;
        fn _mm512_cvtsepi64_epi32(a: __m512i) -> __m256i;
        fn _mm512_mask_cvtsepi64_epi32(src: __m256i, k: __mmask8, a: __m512i) -> __m256i;
        fn _mm512_maskz_cvtsepi64_epi32(k: __mmask8, a: __m512i) -> __m256i;
        fn _mm512_cvtsepi64_epi16(a: __m512i) -> __m128i;
        fn _mm512_mask_cvtsepi64_epi16(src: __m128i, k: __mmask8, a: __m512i) -> __m128i;
        fn _mm512_maskz_cvtsepi64_epi16(k: __mmask8, a: __m512i) -> __m128i;
        fn _mm512_cvtsepi64_epi8(a: __m512i) -> __m128i;
        fn _mm512_mask_cvtsepi64_epi8(src: __m128i, k: __mmask8, a: __m512i) -> __m128i;
        fn _mm512_maskz_cvtsepi64_epi8(k: __mmask8, a: __m512i) -> __m128i;
        fn _mm512_cvtusepi32_epi16(a: __m512i) -> __m256i;
        fn _mm512_mask_cvtusepi32_epi16(src: __m256i, k: __mmask16, a: __m512i) -> __m256i;
        fn _mm512_maskz_cvtusepi32_epi16(k: __mmask16, a: __m512i) -> __m256i;
        fn _mm512_cvtusepi32_epi8(a: __m512i) -> __m128i;
        fn _mm512_mask_cvtusepi32_epi8(src: __m128i, k: __mmask16, a: __m512i) -> __m128i;
        fn _mm512_maskz_cvtusepi32_epi8(k: __mmask16, a: __m512i) -> __m128i;
        fn _mm512_cvtusepi64_epi32(a: __m512i) -> __m256i;
        fn _mm512_mask_cvtusepi64_epi32(src: __m256i, k: __mmask8, a: __m512i) -> __m256i;
        fn _mm512_maskz_cvtusepi64_epi32(k: __mmask8, a: __m512i) -> __m256i;
        fn _mm512_cvtusepi64_epi16(a: __m512i) -> __m128i;
        fn _mm512_mask_cvtusepi64_epi16(src: __m128i, k: __mmask8, a: __m512i) -> __m128i;
        fn _mm512_maskz_cvtusepi64_epi16(k: __mmask8, a: __m512i) -> __m128i;
        fn _mm512_cvtusepi64_epi8(a: __m512i) -> __m128i;
        fn _mm512_mask_cvtusepi64_epi8(src: __m128i, k: __mmask8, a: __m512i) -> __m128i;
        fn _mm512_maskz_cvtusepi64_epi8(k: __mmask8, a: __m512i) -> __m128i;
        fn _mm512_cvt_roundps_epi32<const ROUNDING: i32>(a: __m512) -> __m512i;
        fn _mm512_mask_cvt_roundps_epi32<const ROUNDING: i32>(
            src: __m512i,
            k: __mmask16,
            a: __m512,
        ) -> __m512i;
        fn _mm512_maskz_cvt_roundps_epi32<const ROUNDING: i32>(
            k: __mmask16,
            a: __m512,
        ) -> __m512i;
        fn _mm512_cvt_roundps_epu32<const ROUNDING: i32>(a: __m512) -> __m512i;
        fn _mm512_mask_cvt_roundps_epu32<const ROUNDING: i32>(
            src: __m512i,
            k: __mmask16,
            a: __m512,
        ) -> __m512i;
        fn _mm512_maskz_cvt_roundps_epu32<const ROUNDING: i32>(
            k: __mmask16,
            a: __m512,
        ) -> __m512i;
        fn _mm512_cvt_roundps_pd<const SAE: i32>(a: __m256) -> __m512d;
        fn _mm512_mask_cvt_roundps_pd<const SAE: i32>(
            src: __m512d,
            k: __mmask8,
            a: __m256,
        ) -> __m512d;
        fn _mm512_maskz_cvt_roundps_pd<const SAE: i32>(k: __mmask8, a: __m256) -> __m512d;
        fn _mm512_cvt_roundpd_epi32<const ROUNDING: i32>(a: __m512d) -> __m256i;
        fn _mm512_mask_cvt_roundpd_epi32<const ROUNDING: i32>(
            src: __m256i,
            k: __mmask8,
            a: __m512d,
        ) -> __m256i;
        fn _mm512_maskz_cvt_roundpd_epi32<const ROUNDING: i32>(
            k: __mmask8,
            a: __m512d,
        ) -> __m256i;
        fn _mm512_cvt_roundpd_epu32<const ROUNDING: i32>(a: __m512d) -> __m256i;
        fn _mm512_mask_cvt_roundpd_epu32<const ROUNDING: i32>(
            src: __m256i,
            k: __mmask8,
            a: __m512d,
        ) -> __m256i;
        fn _mm512_maskz_cvt_roundpd_epu32<const ROUNDING: i32>(
            k: __mmask8,
            a: __m512d,
        ) -> __m256i;
        fn _mm512_cvt_roundpd_ps<const ROUNDING: i32>(a: __m512d) -> __m256;
        fn _mm512_mask_cvt_roundpd_ps<const ROUNDING: i32>(
            src: __m256,
            k: __mmask8,
            a: __m512d,
        ) -> __m256;
        fn _mm512_maskz_cvt_roundpd_ps<const ROUNDING: i32>(k: __mmask8, a: __m512d) -> __m256;
        fn _mm512_cvt_roundepi32_ps<const ROUNDING: i32>(a: __m512i) -> __m512;
        fn _mm512_mask_cvt_roundepi32_ps<const ROUNDING: i32>(
            src: __m512,
            k: __mmask16,
            a: __m512i,
        ) -> __m512;
        fn _mm512_maskz_cvt_roundepi32_ps<const ROUNDING: i32>(
            k: __mmask16,
            a: __m512i,
        ) -> __m512;
        fn _mm512_cvt_roundepu32_ps<const ROUNDING: i32>(a: __m512i) -> __m512;
        fn _mm512_mask_cvt_roundepu32_ps<const ROUNDING: i32>(
            src: __m512,
            k: __mmask16,
            a: __m512i,
        ) -> __m512;
        fn _mm512_maskz_cvt_roundepu32_ps<const ROUNDING: i32>(
            k: __mmask16,
            a: __m512i,
        ) -> __m512;
        fn _mm512_cvt_roundps_ph<const SAE: i32>(a: __m512) -> __m256i;
        fn _mm512_mask_cvt_roundps_ph<const SAE: i32>(
            src: __m256i,
            k: __mmask16,
            a: __m512,
        ) -> __m256i;
        fn _mm512_maskz_cvt_roundps_ph<const SAE: i32>(k: __mmask16, a: __m512) -> __m256i;
        fn _mm512_cvtps_ph<const SAE: i32>(a: __m512) -> __m256i;
        fn _mm512_mask_cvtps_ph<const SAE: i32>(
            src: __m256i,
            k: __mmask16,
            a: __m512,
        ) -> __m256i;
        fn _mm512_maskz_cvtps_ph<const SAE: i32>(k: __mmask16, a: __m512) -> __m256i;
        fn _mm512_cvt_roundph_ps<const SAE: i32>(a: __m256i) -> __m512;
        fn _mm512_mask_cvt_roundph_ps<const SAE: i32>(
            src: __m512,
            k: __mmask16,
            a: __m256i,
        ) -> __m512;
        fn _mm512_maskz_cvt_roundph_ps<const SAE: i32>(k: __mmask16, a: __m256i) -> __m512;
        fn _mm512_cvtph_ps(a: __m256i) -> __m512;
        fn _mm512_mask_cvtph_ps(src: __m512, k: __mmask16, a: __m256i) -> __m512;
        fn _mm512_maskz_cvtph_ps(k: __mmask16, a: __m256i) -> __m512;
        fn _mm512_cvtt_roundps_epi32<const SAE: i32>(a: __m512) -> __m512i;
        fn _mm512_mask_cvtt_roundps_epi32<const SAE: i32>(
            src: __m512i,
            k: __mmask16,
            a: __m512,
        ) -> __m512i;
        fn _mm512_maskz_cvtt_roundps_epi32<const SAE: i32>(k: __mmask16, a: __m512) -> __m512i;
        fn _mm512_cvtt_roundps_epu32<const SAE: i32>(a: __m512) -> __m512i;
        fn _mm512_mask_cvtt_roundps_epu32<const SAE: i32>(
            src: __m512i,
            k: __mmask16,
            a: __m512,
        ) -> __m512i;
        fn _mm512_maskz_cvtt_roundps_epu32<const SAE: i32>(k: __mmask16, a: __m512) -> __m512i;
        fn _mm512_cvtt_roundpd_epi32<const SAE: i32>(a: __m512d) -> __m256i;
        fn _mm512_mask_cvtt_roundpd_epi32<const SAE: i32>(
            src: __m256i,
            k: __mmask8,
            a: __m512d,
        ) -> __m256i;
        fn _mm512_maskz_cvtt_roundpd_epi32<const SAE: i32>(k: __mmask8, a: __m512d) -> __m256i;
        fn _mm512_cvtt_roundpd_epu32<const SAE: i32>(a: __m512d) -> __m256i;
        fn _mm512_mask_cvtt_roundpd_epu32<const SAE: i32>(
            src: __m256i,
            k: __mmask8,
            a: __m512d,
        ) -> __m256i;
        fn _mm512_cvttps_epi32(a: __m512) -> __m512i;
        fn _mm512_mask_cvttps_epi32(src: __m512i, k: __mmask16, a: __m512) -> __m512i;
        fn _mm512_maskz_cvttps_epi32(k: __mmask16, a: __m512) -> __m512i;
        fn _mm512_cvttps_epu32(a: __m512) -> __m512i;
        fn _mm512_mask_cvttps_epu32(src: __m512i, k: __mmask16, a: __m512) -> __m512i;
        fn _mm512_maskz_cvttps_epu32(k: __mmask16, a: __m512) -> __m512i;
        fn _mm512_maskz_cvtt_roundpd_epu32<const SAE: i32>(k: __mmask8, a: __m512d) -> __m256i;
        fn _mm512_cvttpd_epi32(a: __m512d) -> __m256i;
        fn _mm512_mask_cvttpd_epi32(src: __m256i, k: __mmask8, a: __m512d) -> __m256i;
        fn _mm512_maskz_cvttpd_epi32(k: __mmask8, a: __m512d) -> __m256i;
        fn _mm512_cvttpd_epu32(a: __m512d) -> __m256i;
        fn _mm512_mask_cvttpd_epu32(src: __m256i, k: __mmask8, a: __m512d) -> __m256i;
        fn _mm512_maskz_cvttpd_epu32(k: __mmask8, a: __m512d) -> __m256i;
        fn _mm512_setzero_pd() -> __m512d;
        fn _mm512_setzero_ps() -> __m512;
        fn _mm512_setzero() -> __m512;
        fn _mm512_setzero_si512() -> __m512i;
        fn _mm512_setzero_epi32() -> __m512i;
        fn _mm512_setr_epi32(
            e15: i32,
            e14: i32,
            e13: i32,
            e12: i32,
            e11: i32,
            e10: i32,
            e9: i32,
            e8: i32,
            e7: i32,
            e6: i32,
            e5: i32,
            e4: i32,
            e3: i32,
            e2: i32,
            e1: i32,
            e0: i32,
        ) -> __m512i;
        fn _mm512_set_epi8(
            e63: i8,
            e62: i8,
            e61: i8,
            e60: i8,
            e59: i8,
            e58: i8,
            e57: i8,
            e56: i8,
            e55: i8,
            e54: i8,
            e53: i8,
            e52: i8,
            e51: i8,
            e50: i8,
            e49: i8,
            e48: i8,
            e47: i8,
            e46: i8,
            e45: i8,
            e44: i8,
            e43: i8,
            e42: i8,
            e41: i8,
            e40: i8,
            e39: i8,
            e38: i8,
            e37: i8,
            e36: i8,
            e35: i8,
            e34: i8,
            e33: i8,
            e32: i8,
            e31: i8,
            e30: i8,
            e29: i8,
            e28: i8,
            e27: i8,
            e26: i8,
            e25: i8,
            e24: i8,
            e23: i8,
            e22: i8,
            e21: i8,
            e20: i8,
            e19: i8,
            e18: i8,
            e17: i8,
            e16: i8,
            e15: i8,
            e14: i8,
            e13: i8,
            e12: i8,
            e11: i8,
            e10: i8,
            e9: i8,
            e8: i8,
            e7: i8,
            e6: i8,
            e5: i8,
            e4: i8,
            e3: i8,
            e2: i8,
            e1: i8,
            e0: i8,
        ) -> __m512i;
        fn _mm512_set_epi16(
            e31: i16,
            e30: i16,
            e29: i16,
            e28: i16,
            e27: i16,
            e26: i16,
            e25: i16,
            e24: i16,
            e23: i16,
            e22: i16,
            e21: i16,
            e20: i16,
            e19: i16,
            e18: i16,
            e17: i16,
            e16: i16,
            e15: i16,
            e14: i16,
            e13: i16,
            e12: i16,
            e11: i16,
            e10: i16,
            e9: i16,
            e8: i16,
            e7: i16,
            e6: i16,
            e5: i16,
            e4: i16,
            e3: i16,
            e2: i16,
            e1: i16,
            e0: i16,
        ) -> __m512i;
        fn _mm512_set4_epi32(d: i32, c: i32, b: i32, a: i32) -> __m512i;
        fn _mm512_set4_ps(d: f32, c: f32, b: f32, a: f32) -> __m512;
        fn _mm512_set4_pd(d: f64, c: f64, b: f64, a: f64) -> __m512d;
        fn _mm512_setr4_epi32(d: i32, c: i32, b: i32, a: i32) -> __m512i;
        fn _mm512_setr4_ps(d: f32, c: f32, b: f32, a: f32) -> __m512;
        fn _mm512_setr4_pd(d: f64, c: f64, b: f64, a: f64) -> __m512d;
        fn _mm512_set_epi64(
            e0: i64,
            e1: i64,
            e2: i64,
            e3: i64,
            e4: i64,
            e5: i64,
            e6: i64,
            e7: i64,
        ) -> __m512i;
        fn _mm512_setr_epi64(
            e0: i64,
            e1: i64,
            e2: i64,
            e3: i64,
            e4: i64,
            e5: i64,
            e6: i64,
            e7: i64,
        ) -> __m512i;
        unsafe fn _mm512_i32gather_pd<const SCALE: i32>(offsets: __m256i, slice: *const u8) -> __m512d;
        unsafe fn _mm512_mask_i32gather_pd<const SCALE: i32>(
            src: __m512d,
            mask: __mmask8,
            offsets: __m256i,
            slice: *const u8,
        ) -> __m512d;
        unsafe fn _mm512_i64gather_pd<const SCALE: i32>(offsets: __m512i, slice: *const u8) -> __m512d;
        unsafe fn _mm512_mask_i64gather_pd<const SCALE: i32>(
            src: __m512d,
            mask: __mmask8,
            offsets: __m512i,
            slice: *const u8,
        ) -> __m512d;
        unsafe fn _mm512_i64gather_ps<const SCALE: i32>(offsets: __m512i, slice: *const u8) -> __m256;
        unsafe fn _mm512_mask_i64gather_ps<const SCALE: i32>(
            src: __m256,
            mask: __mmask8,
            offsets: __m512i,
            slice: *const u8,
        ) -> __m256;
        unsafe fn _mm512_i32gather_ps<const SCALE: i32>(offsets: __m512i, slice: *const u8) -> __m512;
        unsafe fn _mm512_mask_i32gather_ps<const SCALE: i32>(
            src: __m512,
            mask: __mmask16,
            offsets: __m512i,
            slice: *const u8,
        ) -> __m512;
        unsafe fn _mm512_i32gather_epi32<const SCALE: i32>(
            offsets: __m512i,
            slice: *const u8,
        ) -> __m512i;
        unsafe fn _mm512_mask_i32gather_epi32<const SCALE: i32>(
            src: __m512i,
            mask: __mmask16,
            offsets: __m512i,
            slice: *const u8,
        ) -> __m512i;
        unsafe fn _mm512_i32gather_epi64<const SCALE: i32>(
            offsets: __m256i,
            slice: *const u8,
        ) -> __m512i;
        unsafe fn _mm512_mask_i32gather_epi64<const SCALE: i32>(
            src: __m512i,
            mask: __mmask8,
            offsets: __m256i,
            slice: *const u8,
        ) -> __m512i;
        unsafe fn _mm512_i64gather_epi64<const SCALE: i32>(
            offsets: __m512i,
            slice: *const u8,
        ) -> __m512i;
        unsafe fn _mm512_mask_i64gather_epi64<const SCALE: i32>(
            src: __m512i,
            mask: __mmask8,
            offsets: __m512i,
            slice: *const u8,
        ) -> __m512i;
        unsafe fn _mm512_i64gather_epi32<const SCALE: i32>(
            offsets: __m512i,
            slice: *const u8,
        ) -> __m256i;
        unsafe fn _mm512_mask_i64gather_epi32<const SCALE: i32>(
            src: __m256i,
            mask: __mmask8,
            offsets: __m512i,
            slice: *const u8,
        ) -> __m256i;
        unsafe fn _mm512_i32scatter_pd<const SCALE: i32>(
            slice: *mut u8,
            offsets: __m256i,
            src: __m512d,
        );
        unsafe fn _mm512_mask_i32scatter_pd<const SCALE: i32>(
            slice: *mut u8,
            mask: __mmask8,
            offsets: __m256i,
            src: __m512d,
        );
        unsafe fn _mm512_i64scatter_pd<const SCALE: i32>(
            slice: *mut u8,
            offsets: __m512i,
            src: __m512d,
        );
        unsafe fn _mm512_mask_i64scatter_pd<const SCALE: i32>(
            slice: *mut u8,
            mask: __mmask8,
            offsets: __m512i,
            src: __m512d,
        );
        unsafe fn _mm512_i32scatter_ps<const SCALE: i32>(
            slice: *mut u8,
            offsets: __m512i,
            src: __m512,
        );
        unsafe fn _mm512_mask_i32scatter_ps<const SCALE: i32>(
            slice: *mut u8,
            mask: __mmask16,
            offsets: __m512i,
            src: __m512,
        );
        unsafe fn _mm512_i64scatter_ps<const SCALE: i32>(
            slice: *mut u8,
            offsets: __m512i,
            src: __m256,
        );
        unsafe fn _mm512_mask_i64scatter_ps<const SCALE: i32>(
            slice: *mut u8,
            mask: __mmask8,
            offsets: __m512i,
            src: __m256,
        );
        unsafe fn _mm512_i32scatter_epi64<const SCALE: i32>(
            slice: *mut u8,
            offsets: __m256i,
            src: __m512i,
        );
        unsafe fn _mm512_mask_i32scatter_epi64<const SCALE: i32>(
            slice: *mut u8,
            mask: __mmask8,
            offsets: __m256i,
            src: __m512i,
        );
        unsafe fn _mm512_i64scatter_epi64<const SCALE: i32>(
            slice: *mut u8,
            offsets: __m512i,
            src: __m512i,
        );
        unsafe fn _mm512_mask_i64scatter_epi64<const SCALE: i32>(
            slice: *mut u8,
            mask: __mmask8,
            offsets: __m512i,
            src: __m512i,
        );
        unsafe fn _mm512_i32scatter_epi32<const SCALE: i32>(
            slice: *mut u8,
            offsets: __m512i,
            src: __m512i,
        );
        unsafe fn _mm512_mask_i32scatter_epi32<const SCALE: i32>(
            slice: *mut u8,
            mask: __mmask16,
            offsets: __m512i,
            src: __m512i,
        );
        unsafe fn _mm512_i64scatter_epi32<const SCALE: i32>(
            slice: *mut u8,
            offsets: __m512i,
            src: __m256i,
        );
        unsafe fn _mm512_mask_i64scatter_epi32<const SCALE: i32>(
            slice: *mut u8,
            mask: __mmask8,
            offsets: __m512i,
            src: __m256i,
        );
        fn _mm512_mask_compress_epi32(src: __m512i, k: __mmask16, a: __m512i) -> __m512i;
        fn _mm512_maskz_compress_epi32(k: __mmask16, a: __m512i) -> __m512i;
        fn _mm512_mask_compress_epi64(src: __m512i, k: __mmask8, a: __m512i) -> __m512i;
        fn _mm512_maskz_compress_epi64(k: __mmask8, a: __m512i) -> __m512i;
        fn _mm512_mask_compress_ps(src: __m512, k: __mmask16, a: __m512) -> __m512;
        fn _mm512_maskz_compress_ps(k: __mmask16, a: __m512) -> __m512;
        fn _mm512_mask_compress_pd(src: __m512d, k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_maskz_compress_pd(k: __mmask8, a: __m512d) -> __m512d;
        unsafe fn _mm512_mask_compressstoreu_epi32(base_addr: *mut u8, k: __mmask16, a: __m512i);
        unsafe fn _mm512_mask_compressstoreu_epi64(base_addr: *mut u8, k: __mmask8, a: __m512i);
        unsafe fn _mm512_mask_compressstoreu_ps(base_addr: *mut u8, k: __mmask16, a: __m512);
        unsafe fn _mm512_mask_compressstoreu_pd(base_addr: *mut u8, k: __mmask8, a: __m512d);
        fn _mm512_mask_expand_epi32(src: __m512i, k: __mmask16, a: __m512i) -> __m512i;
        fn _mm512_maskz_expand_epi32(k: __mmask16, a: __m512i) -> __m512i;
        fn _mm512_mask_expand_epi64(src: __m512i, k: __mmask8, a: __m512i) -> __m512i;
        fn _mm512_maskz_expand_epi64(k: __mmask8, a: __m512i) -> __m512i;
        fn _mm512_mask_expand_ps(src: __m512, k: __mmask16, a: __m512) -> __m512;
        fn _mm512_maskz_expand_ps(k: __mmask16, a: __m512) -> __m512;
        fn _mm512_mask_expand_pd(src: __m512d, k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_maskz_expand_pd(k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_rol_epi32<const IMM8: i32>(a: __m512i) -> __m512i;
        fn _mm512_mask_rol_epi32<const IMM8: i32>(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_rol_epi32<const IMM8: i32>(k: __mmask16, a: __m512i) -> __m512i;
        fn _mm512_ror_epi32<const IMM8: i32>(a: __m512i) -> __m512i;
        fn _mm512_mask_ror_epi32<const IMM8: i32>(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_ror_epi32<const IMM8: i32>(k: __mmask16, a: __m512i) -> __m512i;
        fn _mm512_rol_epi64<const IMM8: i32>(a: __m512i) -> __m512i;
        fn _mm512_mask_rol_epi64<const IMM8: i32>(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_rol_epi64<const IMM8: i32>(k: __mmask8, a: __m512i) -> __m512i;
        fn _mm512_ror_epi64<const IMM8: i32>(a: __m512i) -> __m512i;
        fn _mm512_mask_ror_epi64<const IMM8: i32>(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_ror_epi64<const IMM8: i32>(k: __mmask8, a: __m512i) -> __m512i;
        fn _mm512_slli_epi32<const IMM8: u32>(a: __m512i) -> __m512i;
        fn _mm512_mask_slli_epi32<const IMM8: u32>(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_slli_epi32<const IMM8: u32>(k: __mmask16, a: __m512i) -> __m512i;
        fn _mm512_srli_epi32<const IMM8: u32>(a: __m512i) -> __m512i;
        fn _mm512_mask_srli_epi32<const IMM8: u32>(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_srli_epi32<const IMM8: u32>(k: __mmask16, a: __m512i) -> __m512i;
        fn _mm512_slli_epi64<const IMM8: u32>(a: __m512i) -> __m512i;
        fn _mm512_mask_slli_epi64<const IMM8: u32>(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_slli_epi64<const IMM8: u32>(k: __mmask8, a: __m512i) -> __m512i;
        fn _mm512_srli_epi64<const IMM8: u32>(a: __m512i) -> __m512i;
        fn _mm512_mask_srli_epi64<const IMM8: u32>(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_srli_epi64<const IMM8: u32>(k: __mmask8, a: __m512i) -> __m512i;
        fn _mm512_sll_epi32(a: __m512i, count: __m128i) -> __m512i;
        fn _mm512_mask_sll_epi32(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
            count: __m128i,
        ) -> __m512i;
        fn _mm512_maskz_sll_epi32(k: __mmask16, a: __m512i, count: __m128i) -> __m512i;
        fn _mm512_srl_epi32(a: __m512i, count: __m128i) -> __m512i;
        fn _mm512_mask_srl_epi32(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
            count: __m128i,
        ) -> __m512i;
        fn _mm512_maskz_srl_epi32(k: __mmask16, a: __m512i, count: __m128i) -> __m512i;
        fn _mm512_sll_epi64(a: __m512i, count: __m128i) -> __m512i;
        fn _mm512_mask_sll_epi64(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
            count: __m128i,
        ) -> __m512i;
        fn _mm512_maskz_sll_epi64(k: __mmask8, a: __m512i, count: __m128i) -> __m512i;
        fn _mm512_srl_epi64(a: __m512i, count: __m128i) -> __m512i;
        fn _mm512_mask_srl_epi64(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
            count: __m128i,
        ) -> __m512i;
        fn _mm512_maskz_srl_epi64(k: __mmask8, a: __m512i, count: __m128i) -> __m512i;
        fn _mm512_sra_epi32(a: __m512i, count: __m128i) -> __m512i;
        fn _mm512_mask_sra_epi32(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
            count: __m128i,
        ) -> __m512i;
        fn _mm512_maskz_sra_epi32(k: __mmask16, a: __m512i, count: __m128i) -> __m512i;
        fn _mm512_sra_epi64(a: __m512i, count: __m128i) -> __m512i;
        fn _mm512_mask_sra_epi64(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
            count: __m128i,
        ) -> __m512i;
        fn _mm512_maskz_sra_epi64(k: __mmask8, a: __m512i, count: __m128i) -> __m512i;
        fn _mm512_srai_epi32<const IMM8: u32>(a: __m512i) -> __m512i;
        fn _mm512_mask_srai_epi32<const IMM8: u32>(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_srai_epi32<const IMM8: u32>(k: __mmask16, a: __m512i) -> __m512i;
        fn _mm512_srai_epi64<const IMM8: u32>(a: __m512i) -> __m512i;
        fn _mm512_mask_srai_epi64<const IMM8: u32>(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_srai_epi64<const IMM8: u32>(k: __mmask8, a: __m512i) -> __m512i;
        fn _mm512_srav_epi32(a: __m512i, count: __m512i) -> __m512i;
        fn _mm512_mask_srav_epi32(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
            count: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_srav_epi32(k: __mmask16, a: __m512i, count: __m512i) -> __m512i;
        fn _mm512_srav_epi64(a: __m512i, count: __m512i) -> __m512i;
        fn _mm512_mask_srav_epi64(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
            count: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_srav_epi64(k: __mmask8, a: __m512i, count: __m512i) -> __m512i;
        fn _mm512_rolv_epi32(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_rolv_epi32(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_rolv_epi32(k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_rorv_epi32(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_rorv_epi32(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_rorv_epi32(k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_rolv_epi64(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_rolv_epi64(src: __m512i, k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_rolv_epi64(k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_rorv_epi64(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_rorv_epi64(src: __m512i, k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_rorv_epi64(k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_sllv_epi32(a: __m512i, count: __m512i) -> __m512i;
        fn _mm512_mask_sllv_epi32(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
            count: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_sllv_epi32(k: __mmask16, a: __m512i, count: __m512i) -> __m512i;
        fn _mm512_srlv_epi32(a: __m512i, count: __m512i) -> __m512i;
        fn _mm512_mask_srlv_epi32(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
            count: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_srlv_epi32(k: __mmask16, a: __m512i, count: __m512i) -> __m512i;
        fn _mm512_sllv_epi64(a: __m512i, count: __m512i) -> __m512i;
        fn _mm512_mask_sllv_epi64(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
            count: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_sllv_epi64(k: __mmask8, a: __m512i, count: __m512i) -> __m512i;
        fn _mm512_srlv_epi64(a: __m512i, count: __m512i) -> __m512i;
        fn _mm512_mask_srlv_epi64(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
            count: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_srlv_epi64(k: __mmask8, a: __m512i, count: __m512i) -> __m512i;
        fn _mm512_permute_ps<const MASK: i32>(a: __m512) -> __m512;
        fn _mm512_mask_permute_ps<const MASK: i32>(
            src: __m512,
            k: __mmask16,
            a: __m512,
        ) -> __m512;
        fn _mm512_maskz_permute_ps<const MASK: i32>(k: __mmask16, a: __m512) -> __m512;
        fn _mm512_permute_pd<const MASK: i32>(a: __m512d) -> __m512d;
        fn _mm512_mask_permute_pd<const MASK: i32>(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_permute_pd<const MASK: i32>(k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_permutex_epi64<const MASK: i32>(a: __m512i) -> __m512i;
        fn _mm512_mask_permutex_epi64<const MASK: i32>(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_permutex_epi64<const MASK: i32>(k: __mmask8, a: __m512i) -> __m512i;
        fn _mm512_permutex_pd<const MASK: i32>(a: __m512d) -> __m512d;
        fn _mm512_mask_permutex_pd<const MASK: i32>(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_permutex_pd<const MASK: i32>(k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_permutevar_epi32(idx: __m512i, a: __m512i) -> __m512i;
        fn _mm512_mask_permutevar_epi32(
            src: __m512i,
            k: __mmask16,
            idx: __m512i,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_permutevar_ps(a: __m512, b: __m512i) -> __m512;
        fn _mm512_mask_permutevar_ps(
            src: __m512,
            k: __mmask16,
            a: __m512,
            b: __m512i,
        ) -> __m512;
        fn _mm512_maskz_permutevar_ps(k: __mmask16, a: __m512, b: __m512i) -> __m512;
        fn _mm512_permutevar_pd(a: __m512d, b: __m512i) -> __m512d;
        fn _mm512_mask_permutevar_pd(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
            b: __m512i,
        ) -> __m512d;
        fn _mm512_maskz_permutevar_pd(k: __mmask8, a: __m512d, b: __m512i) -> __m512d;
        fn _mm512_permutexvar_epi32(idx: __m512i, a: __m512i) -> __m512i;
        fn _mm512_mask_permutexvar_epi32(
            src: __m512i,
            k: __mmask16,
            idx: __m512i,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_permutexvar_epi32(k: __mmask16, idx: __m512i, a: __m512i) -> __m512i;
        fn _mm512_permutexvar_epi64(idx: __m512i, a: __m512i) -> __m512i;
        fn _mm512_mask_permutexvar_epi64(
            src: __m512i,
            k: __mmask8,
            idx: __m512i,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_permutexvar_epi64(k: __mmask8, idx: __m512i, a: __m512i) -> __m512i;
        fn _mm512_permutexvar_ps(idx: __m512i, a: __m512) -> __m512;
        fn _mm512_mask_permutexvar_ps(
            src: __m512,
            k: __mmask16,
            idx: __m512i,
            a: __m512,
        ) -> __m512;
        fn _mm512_maskz_permutexvar_ps(k: __mmask16, idx: __m512i, a: __m512) -> __m512;
        fn _mm512_permutexvar_pd(idx: __m512i, a: __m512d) -> __m512d;
        fn _mm512_mask_permutexvar_pd(
            src: __m512d,
            k: __mmask8,
            idx: __m512i,
            a: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_permutexvar_pd(k: __mmask8, idx: __m512i, a: __m512d) -> __m512d;
        fn _mm512_permutex2var_epi32(a: __m512i, idx: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_permutex2var_epi32(
            a: __m512i,
            k: __mmask16,
            idx: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_permutex2var_epi32(
            k: __mmask16,
            a: __m512i,
            idx: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_mask2_permutex2var_epi32(
            a: __m512i,
            idx: __m512i,
            k: __mmask16,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_permutex2var_epi64(a: __m512i, idx: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_permutex2var_epi64(
            a: __m512i,
            k: __mmask8,
            idx: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_permutex2var_epi64(
            k: __mmask8,
            a: __m512i,
            idx: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_mask2_permutex2var_epi64(
            a: __m512i,
            idx: __m512i,
            k: __mmask8,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_permutex2var_ps(a: __m512, idx: __m512i, b: __m512) -> __m512;
        fn _mm512_mask_permutex2var_ps(
            a: __m512,
            k: __mmask16,
            idx: __m512i,
            b: __m512,
        ) -> __m512;
        fn _mm512_maskz_permutex2var_ps(
            k: __mmask16,
            a: __m512,
            idx: __m512i,
            b: __m512,
        ) -> __m512;
        fn _mm512_mask2_permutex2var_ps(
            a: __m512,
            idx: __m512i,
            k: __mmask16,
            b: __m512,
        ) -> __m512;
        fn _mm512_permutex2var_pd(a: __m512d, idx: __m512i, b: __m512d) -> __m512d;
        fn _mm512_mask_permutex2var_pd(
            a: __m512d,
            k: __mmask8,
            idx: __m512i,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_permutex2var_pd(
            k: __mmask8,
            a: __m512d,
            idx: __m512i,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_mask2_permutex2var_pd(
            a: __m512d,
            idx: __m512i,
            k: __mmask8,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_shuffle_epi32<const MASK: _MM_PERM_ENUM>(a: __m512i) -> __m512i;
        fn _mm512_mask_shuffle_epi32<const MASK: _MM_PERM_ENUM>(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_shuffle_epi32<const MASK: _MM_PERM_ENUM>(
            k: __mmask16,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_shuffle_ps<const MASK: i32>(a: __m512, b: __m512) -> __m512;
        fn _mm512_mask_shuffle_ps<const MASK: i32>(
            src: __m512,
            k: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __m512;
        fn _mm512_maskz_shuffle_ps<const MASK: i32>(
            k: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __m512;
        fn _mm512_shuffle_pd<const MASK: i32>(a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_mask_shuffle_pd<const MASK: i32>(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_shuffle_pd<const MASK: i32>(
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_shuffle_i32x4<const MASK: i32>(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_shuffle_i32x4<const MASK: i32>(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_shuffle_i32x4<const MASK: i32>(
            k: __mmask16,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_shuffle_i64x2<const MASK: i32>(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_shuffle_i64x2<const MASK: i32>(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_shuffle_i64x2<const MASK: i32>(
            k: __mmask8,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_shuffle_f32x4<const MASK: i32>(a: __m512, b: __m512) -> __m512;
        fn _mm512_mask_shuffle_f32x4<const MASK: i32>(
            src: __m512,
            k: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __m512;
        fn _mm512_maskz_shuffle_f32x4<const MASK: i32>(
            k: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __m512;
        fn _mm512_shuffle_f64x2<const MASK: i32>(a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_mask_shuffle_f64x2<const MASK: i32>(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_shuffle_f64x2<const MASK: i32>(
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_extractf32x4_ps<const IMM8: i32>(a: __m512) -> __m128;
        fn _mm512_mask_extractf32x4_ps<const IMM8: i32>(
            src: __m128,
            k: __mmask8,
            a: __m512,
        ) -> __m128;
        fn _mm512_maskz_extractf32x4_ps<const IMM8: i32>(k: __mmask8, a: __m512) -> __m128;
        fn _mm256_extractf32x4_ps<const IMM8: i32>(a: __m256) -> __m128;
        fn _mm256_mask_extractf32x4_ps<const IMM8: i32>(
            src: __m128,
            k: __mmask8,
            a: __m256,
        ) -> __m128;
        fn _mm256_maskz_extractf32x4_ps<const IMM8: i32>(k: __mmask8, a: __m256) -> __m128;
        fn _mm512_extracti64x4_epi64<const IMM1: i32>(a: __m512i) -> __m256i;
        fn _mm512_mask_extracti64x4_epi64<const IMM1: i32>(
            src: __m256i,
            k: __mmask8,
            a: __m512i,
        ) -> __m256i;
        fn _mm512_maskz_extracti64x4_epi64<const IMM1: i32>(k: __mmask8, a: __m512i) -> __m256i;
        fn _mm512_extractf64x4_pd<const IMM8: i32>(a: __m512d) -> __m256d;
        fn _mm512_mask_extractf64x4_pd<const IMM8: i32>(
            src: __m256d,
            k: __mmask8,
            a: __m512d,
        ) -> __m256d;
        fn _mm512_maskz_extractf64x4_pd<const IMM8: i32>(k: __mmask8, a: __m512d) -> __m256d;
        fn _mm512_extracti32x4_epi32<const IMM2: i32>(a: __m512i) -> __m128i;
        fn _mm512_mask_extracti32x4_epi32<const IMM2: i32>(
            src: __m128i,
            k: __mmask8,
            a: __m512i,
        ) -> __m128i;
        fn _mm512_maskz_extracti32x4_epi32<const IMM2: i32>(k: __mmask8, a: __m512i) -> __m128i;
        fn _mm256_extracti32x4_epi32<const IMM1: i32>(a: __m256i) -> __m128i;
        fn _mm256_mask_extracti32x4_epi32<const IMM1: i32>(
            src: __m128i,
            k: __mmask8,
            a: __m256i,
        ) -> __m128i;
        fn _mm256_maskz_extracti32x4_epi32<const IMM1: i32>(k: __mmask8, a: __m256i) -> __m128i;
        fn _mm512_moveldup_ps(a: __m512) -> __m512;
        fn _mm512_mask_moveldup_ps(src: __m512, k: __mmask16, a: __m512) -> __m512;
        fn _mm512_maskz_moveldup_ps(k: __mmask16, a: __m512) -> __m512;
        fn _mm512_movehdup_ps(a: __m512) -> __m512;
        fn _mm512_mask_movehdup_ps(src: __m512, k: __mmask16, a: __m512) -> __m512;
        fn _mm512_maskz_movehdup_ps(k: __mmask16, a: __m512) -> __m512;
        fn _mm512_movedup_pd(a: __m512d) -> __m512d;
        fn _mm512_mask_movedup_pd(src: __m512d, k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_maskz_movedup_pd(k: __mmask8, a: __m512d) -> __m512d;
        fn _mm512_inserti32x4<const IMM8: i32>(a: __m512i, b: __m128i) -> __m512i;
        fn _mm512_mask_inserti32x4<const IMM8: i32>(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
            b: __m128i,
        ) -> __m512i;
        fn _mm512_maskz_inserti32x4<const IMM8: i32>(
            k: __mmask16,
            a: __m512i,
            b: __m128i,
        ) -> __m512i;
        fn _mm256_inserti32x4<const IMM8: i32>(a: __m256i, b: __m128i) -> __m256i;
        fn _mm256_mask_inserti32x4<const IMM8: i32>(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            b: __m128i,
        ) -> __m256i;
        fn _mm256_maskz_inserti32x4<const IMM8: i32>(
            k: __mmask8,
            a: __m256i,
            b: __m128i,
        ) -> __m256i;
        fn _mm512_inserti64x4<const IMM8: i32>(a: __m512i, b: __m256i) -> __m512i;
        fn _mm512_mask_inserti64x4<const IMM8: i32>(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
            b: __m256i,
        ) -> __m512i;
        fn _mm512_maskz_inserti64x4<const IMM8: i32>(
            k: __mmask8,
            a: __m512i,
            b: __m256i,
        ) -> __m512i;
        fn _mm512_insertf32x4<const IMM8: i32>(a: __m512, b: __m128) -> __m512;
        fn _mm512_mask_insertf32x4<const IMM8: i32>(
            src: __m512,
            k: __mmask16,
            a: __m512,
            b: __m128,
        ) -> __m512;
        fn _mm512_maskz_insertf32x4<const IMM8: i32>(
            k: __mmask16,
            a: __m512,
            b: __m128,
        ) -> __m512;
        fn _mm256_insertf32x4<const IMM8: i32>(a: __m256, b: __m128) -> __m256;
        fn _mm256_mask_insertf32x4<const IMM8: i32>(
            src: __m256,
            k: __mmask8,
            a: __m256,
            b: __m128,
        ) -> __m256;
        fn _mm256_maskz_insertf32x4<const IMM8: i32>(
            k: __mmask8,
            a: __m256,
            b: __m128,
        ) -> __m256;
        fn _mm512_insertf64x4<const IMM8: i32>(a: __m512d, b: __m256d) -> __m512d;
        fn _mm512_mask_insertf64x4<const IMM8: i32>(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
            b: __m256d,
        ) -> __m512d;
        fn _mm512_maskz_insertf64x4<const IMM8: i32>(
            k: __mmask8,
            a: __m512d,
            b: __m256d,
        ) -> __m512d;
        fn _mm512_unpackhi_epi32(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_unpackhi_epi32(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_unpackhi_epi32(k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_unpackhi_epi64(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_unpackhi_epi64(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_unpackhi_epi64(k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_unpackhi_ps(a: __m512, b: __m512) -> __m512;
        fn _mm512_mask_unpackhi_ps(src: __m512, k: __mmask16, a: __m512, b: __m512) -> __m512;
        fn _mm512_maskz_unpackhi_ps(k: __mmask16, a: __m512, b: __m512) -> __m512;
        fn _mm512_unpackhi_pd(a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_mask_unpackhi_pd(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_unpackhi_pd(k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_unpacklo_epi32(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_unpacklo_epi32(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_unpacklo_epi32(k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_unpacklo_epi64(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_unpacklo_epi64(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_unpacklo_epi64(k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_unpacklo_ps(a: __m512, b: __m512) -> __m512;
        fn _mm512_mask_unpacklo_ps(src: __m512, k: __mmask16, a: __m512, b: __m512) -> __m512;
        fn _mm512_maskz_unpacklo_ps(k: __mmask16, a: __m512, b: __m512) -> __m512;
        fn _mm512_unpacklo_pd(a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_mask_unpacklo_pd(
            src: __m512d,
            k: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __m512d;
        fn _mm512_maskz_unpacklo_pd(k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_castps128_ps512(a: __m128) -> __m512;
        fn _mm512_castps256_ps512(a: __m256) -> __m512;
        fn _mm512_zextps128_ps512(a: __m128) -> __m512;
        fn _mm512_zextps256_ps512(a: __m256) -> __m512;
        fn _mm512_castps512_ps128(a: __m512) -> __m128;
        fn _mm512_castps512_ps256(a: __m512) -> __m256;
        fn _mm512_castps_pd(a: __m512) -> __m512d;
        fn _mm512_castps_si512(a: __m512) -> __m512i;
        fn _mm512_castpd128_pd512(a: __m128d) -> __m512d;
        fn _mm512_castpd256_pd512(a: __m256d) -> __m512d;
        fn _mm512_zextpd128_pd512(a: __m128d) -> __m512d;
        fn _mm512_zextpd256_pd512(a: __m256d) -> __m512d;
        fn _mm512_castpd512_pd128(a: __m512d) -> __m128d;
        fn _mm512_castpd512_pd256(a: __m512d) -> __m256d;
        fn _mm512_castpd_ps(a: __m512d) -> __m512;
        fn _mm512_castpd_si512(a: __m512d) -> __m512i;
        fn _mm512_castsi128_si512(a: __m128i) -> __m512i;
        fn _mm512_castsi256_si512(a: __m256i) -> __m512i;
        fn _mm512_zextsi128_si512(a: __m128i) -> __m512i;
        fn _mm512_zextsi256_si512(a: __m256i) -> __m512i;
        fn _mm512_castsi512_si128(a: __m512i) -> __m128i;
        fn _mm512_castsi512_si256(a: __m512i) -> __m256i;
        fn _mm512_castsi512_ps(a: __m512i) -> __m512;
        fn _mm512_castsi512_pd(a: __m512i) -> __m512d;
        fn _mm512_cvtsi512_si32(a: __m512i) -> i32;
        fn _mm512_broadcastd_epi32(a: __m128i) -> __m512i;
        fn _mm512_mask_broadcastd_epi32(src: __m512i, k: __mmask16, a: __m128i) -> __m512i;
        fn _mm512_maskz_broadcastd_epi32(k: __mmask16, a: __m128i) -> __m512i;
        fn _mm512_broadcastq_epi64(a: __m128i) -> __m512i;
        fn _mm512_mask_broadcastq_epi64(src: __m512i, k: __mmask8, a: __m128i) -> __m512i;
        fn _mm512_maskz_broadcastq_epi64(k: __mmask8, a: __m128i) -> __m512i;
        fn _mm512_broadcastss_ps(a: __m128) -> __m512;
        fn _mm512_mask_broadcastss_ps(src: __m512, k: __mmask16, a: __m128) -> __m512;
        fn _mm512_maskz_broadcastss_ps(k: __mmask16, a: __m128) -> __m512;
        fn _mm512_broadcastsd_pd(a: __m128d) -> __m512d;
        fn _mm512_mask_broadcastsd_pd(src: __m512d, k: __mmask8, a: __m128d) -> __m512d;
        fn _mm512_maskz_broadcastsd_pd(k: __mmask8, a: __m128d) -> __m512d;
        fn _mm512_broadcast_i32x4(a: __m128i) -> __m512i;
        fn _mm512_mask_broadcast_i32x4(src: __m512i, k: __mmask16, a: __m128i) -> __m512i;
        fn _mm512_maskz_broadcast_i32x4(k: __mmask16, a: __m128i) -> __m512i;
        fn _mm512_broadcast_i64x4(a: __m256i) -> __m512i;
        fn _mm512_mask_broadcast_i64x4(src: __m512i, k: __mmask8, a: __m256i) -> __m512i;
        fn _mm512_maskz_broadcast_i64x4(k: __mmask8, a: __m256i) -> __m512i;
        fn _mm512_broadcast_f32x4(a: __m128) -> __m512;
        fn _mm512_mask_broadcast_f32x4(src: __m512, k: __mmask16, a: __m128) -> __m512;
        fn _mm512_maskz_broadcast_f32x4(k: __mmask16, a: __m128) -> __m512;
        fn _mm512_broadcast_f64x4(a: __m256d) -> __m512d;
        fn _mm512_mask_broadcast_f64x4(src: __m512d, k: __mmask8, a: __m256d) -> __m512d;
        fn _mm512_maskz_broadcast_f64x4(k: __mmask8, a: __m256d) -> __m512d;
        fn _mm512_mask_blend_epi32(k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_blend_epi64(k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_blend_ps(k: __mmask16, a: __m512, b: __m512) -> __m512;
        fn _mm512_mask_blend_pd(k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
        fn _mm512_alignr_epi32<const IMM8: i32>(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_alignr_epi32<const IMM8: i32>(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_alignr_epi32<const IMM8: i32>(
            k: __mmask16,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_alignr_epi64<const IMM8: i32>(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_alignr_epi64<const IMM8: i32>(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_alignr_epi64<const IMM8: i32>(
            k: __mmask8,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_and_epi32(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_and_epi32(src: __m512i, k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_and_epi32(k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_and_epi64(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_and_epi64(src: __m512i, k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_and_epi64(k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_and_si512(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_or_epi32(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_or_epi32(src: __m512i, k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_or_epi32(k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_or_epi64(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_or_epi64(src: __m512i, k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_or_epi64(k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_or_si512(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_xor_epi32(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_xor_epi32(src: __m512i, k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_xor_epi32(k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_xor_epi64(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_xor_epi64(src: __m512i, k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_xor_epi64(k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_xor_si512(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_andnot_epi32(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_andnot_epi32(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_andnot_epi32(k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_andnot_epi64(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_andnot_epi64(
            src: __m512i,
            k: __mmask8,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_andnot_epi64(k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_andnot_si512(a: __m512i, b: __m512i) -> __m512i;
        fn _kand_mask16(a: __mmask16, b: __mmask16) -> __mmask16;
        fn _mm512_kand(a: __mmask16, b: __mmask16) -> __mmask16;
        fn _kor_mask16(a: __mmask16, b: __mmask16) -> __mmask16;
        fn _mm512_kor(a: __mmask16, b: __mmask16) -> __mmask16;
        fn _kxor_mask16(a: __mmask16, b: __mmask16) -> __mmask16;
        fn _mm512_kxor(a: __mmask16, b: __mmask16) -> __mmask16;
        fn _knot_mask16(a: __mmask16) -> __mmask16;
        fn _mm512_knot(a: __mmask16) -> __mmask16;
        fn _kandn_mask16(a: __mmask16, b: __mmask16) -> __mmask16;
        fn _mm512_kandn(a: __mmask16, b: __mmask16) -> __mmask16;
        fn _kxnor_mask16(a: __mmask16, b: __mmask16) -> __mmask16;
        fn _mm512_kxnor(a: __mmask16, b: __mmask16) -> __mmask16;
        fn _mm512_kmov(a: __mmask16) -> __mmask16;
        fn _mm512_int2mask(mask: i32) -> __mmask16;
        fn _mm512_mask2int(k1: __mmask16) -> i32;
        fn _mm512_kunpackb(a: __mmask16, b: __mmask16) -> __mmask16;
        fn _mm512_kortestc(a: __mmask16, b: __mmask16) -> i32;
        fn _mm512_test_epi32_mask(a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_mask_test_epi32_mask(k: __mmask16, a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_test_epi64_mask(a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_mask_test_epi64_mask(k: __mmask8, a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_testn_epi32_mask(a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_mask_testn_epi32_mask(k: __mmask16, a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_testn_epi64_mask(a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_mask_testn_epi64_mask(k: __mmask8, a: __m512i, b: __m512i) -> __mmask8;
        unsafe fn _mm512_stream_ps(mem_addr: *mut f32, a: __m512);
        unsafe fn _mm512_stream_pd(mem_addr: *mut f64, a: __m512d);
        unsafe fn _mm512_stream_si512(mem_addr: *mut i32, a: __m512i);
        fn _mm512_set_ps(
            e0: f32,
            e1: f32,
            e2: f32,
            e3: f32,
            e4: f32,
            e5: f32,
            e6: f32,
            e7: f32,
            e8: f32,
            e9: f32,
            e10: f32,
            e11: f32,
            e12: f32,
            e13: f32,
            e14: f32,
            e15: f32,
        ) -> __m512;
        fn _mm512_setr_ps(
            e0: f32,
            e1: f32,
            e2: f32,
            e3: f32,
            e4: f32,
            e5: f32,
            e6: f32,
            e7: f32,
            e8: f32,
            e9: f32,
            e10: f32,
            e11: f32,
            e12: f32,
            e13: f32,
            e14: f32,
            e15: f32,
        ) -> __m512;
        fn _mm512_set1_pd(a: f64) -> __m512d;
        fn _mm512_set1_ps(a: f32) -> __m512;
        fn _mm512_set_epi32(
            e15: i32,
            e14: i32,
            e13: i32,
            e12: i32,
            e11: i32,
            e10: i32,
            e9: i32,
            e8: i32,
            e7: i32,
            e6: i32,
            e5: i32,
            e4: i32,
            e3: i32,
            e2: i32,
            e1: i32,
            e0: i32,
        ) -> __m512i;
        fn _mm512_set1_epi8(a: i8) -> __m512i;
        fn _mm512_set1_epi16(a: i16) -> __m512i;
        fn _mm512_set1_epi32(a: i32) -> __m512i;
        fn _mm512_mask_set1_epi32(src: __m512i, k: __mmask16, a: i32) -> __m512i;
        fn _mm512_maskz_set1_epi32(k: __mmask16, a: i32) -> __m512i;
        fn _mm512_set1_epi64(a: i64) -> __m512i;
        fn _mm512_mask_set1_epi64(src: __m512i, k: __mmask8, a: i64) -> __m512i;
        fn _mm512_maskz_set1_epi64(k: __mmask8, a: i64) -> __m512i;
        fn _mm512_set4_epi64(d: i64, c: i64, b: i64, a: i64) -> __m512i;
        fn _mm512_setr4_epi64(d: i64, c: i64, b: i64, a: i64) -> __m512i;
        fn _mm512_cmplt_ps_mask(a: __m512, b: __m512) -> __mmask16;
        fn _mm512_mask_cmplt_ps_mask(k1: __mmask16, a: __m512, b: __m512) -> __mmask16;
        fn _mm512_cmpnlt_ps_mask(a: __m512, b: __m512) -> __mmask16;
        fn _mm512_mask_cmpnlt_ps_mask(k1: __mmask16, a: __m512, b: __m512) -> __mmask16;
        fn _mm512_cmple_ps_mask(a: __m512, b: __m512) -> __mmask16;
        fn _mm512_mask_cmple_ps_mask(k1: __mmask16, a: __m512, b: __m512) -> __mmask16;
        fn _mm512_cmpnle_ps_mask(a: __m512, b: __m512) -> __mmask16;
        fn _mm512_mask_cmpnle_ps_mask(k1: __mmask16, a: __m512, b: __m512) -> __mmask16;
        fn _mm512_cmpeq_ps_mask(a: __m512, b: __m512) -> __mmask16;
        fn _mm512_mask_cmpeq_ps_mask(k1: __mmask16, a: __m512, b: __m512) -> __mmask16;
        fn _mm512_cmpneq_ps_mask(a: __m512, b: __m512) -> __mmask16;
        fn _mm512_mask_cmpneq_ps_mask(k1: __mmask16, a: __m512, b: __m512) -> __mmask16;
        fn _mm512_cmp_ps_mask<const IMM8: i32>(a: __m512, b: __m512) -> __mmask16;
        fn _mm512_mask_cmp_ps_mask<const IMM8: i32>(
            k1: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __mmask16;
        fn _mm512_cmp_round_ps_mask<const IMM5: i32, const SAE: i32>(
            a: __m512,
            b: __m512,
        ) -> __mmask16;
        fn _mm512_mask_cmp_round_ps_mask<const IMM5: i32, const SAE: i32>(
            m: __mmask16,
            a: __m512,
            b: __m512,
        ) -> __mmask16;
        fn _mm512_cmpord_ps_mask(a: __m512, b: __m512) -> __mmask16;
        fn _mm512_mask_cmpord_ps_mask(k1: __mmask16, a: __m512, b: __m512) -> __mmask16;
        fn _mm512_cmpunord_ps_mask(a: __m512, b: __m512) -> __mmask16;
        fn _mm512_mask_cmpunord_ps_mask(k1: __mmask16, a: __m512, b: __m512) -> __mmask16;
        fn _mm512_cmplt_pd_mask(a: __m512d, b: __m512d) -> __mmask8;
        fn _mm512_mask_cmplt_pd_mask(k1: __mmask8, a: __m512d, b: __m512d) -> __mmask8;
        fn _mm512_cmpnlt_pd_mask(a: __m512d, b: __m512d) -> __mmask8;
        fn _mm512_mask_cmpnlt_pd_mask(m: __mmask8, a: __m512d, b: __m512d) -> __mmask8;
        fn _mm512_cmple_pd_mask(a: __m512d, b: __m512d) -> __mmask8;
        fn _mm512_mask_cmple_pd_mask(k1: __mmask8, a: __m512d, b: __m512d) -> __mmask8;
        fn _mm512_cmpnle_pd_mask(a: __m512d, b: __m512d) -> __mmask8;
        fn _mm512_mask_cmpnle_pd_mask(k1: __mmask8, a: __m512d, b: __m512d) -> __mmask8;
        fn _mm512_cmpeq_pd_mask(a: __m512d, b: __m512d) -> __mmask8;
        fn _mm512_mask_cmpeq_pd_mask(k1: __mmask8, a: __m512d, b: __m512d) -> __mmask8;
        fn _mm512_cmpneq_pd_mask(a: __m512d, b: __m512d) -> __mmask8;
        fn _mm512_mask_cmpneq_pd_mask(k1: __mmask8, a: __m512d, b: __m512d) -> __mmask8;
        fn _mm512_cmp_pd_mask<const IMM8: i32>(a: __m512d, b: __m512d) -> __mmask8;
        fn _mm512_mask_cmp_pd_mask<const IMM8: i32>(
            k1: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __mmask8;
        fn _mm512_cmp_round_pd_mask<const IMM5: i32, const SAE: i32>(
            a: __m512d,
            b: __m512d,
        ) -> __mmask8;
        fn _mm512_mask_cmp_round_pd_mask<const IMM5: i32, const SAE: i32>(
            k1: __mmask8,
            a: __m512d,
            b: __m512d,
        ) -> __mmask8;
        fn _mm512_cmpord_pd_mask(a: __m512d, b: __m512d) -> __mmask8;
        fn _mm512_mask_cmpord_pd_mask(k1: __mmask8, a: __m512d, b: __m512d) -> __mmask8;
        fn _mm512_cmpunord_pd_mask(a: __m512d, b: __m512d) -> __mmask8;
        fn _mm512_mask_cmpunord_pd_mask(k1: __mmask8, a: __m512d, b: __m512d) -> __mmask8;
        fn _mm_cmp_ss_mask<const IMM8: i32>(a: __m128, b: __m128) -> __mmask8;
        fn _mm_mask_cmp_ss_mask<const IMM8: i32>(
            k1: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __mmask8;
        fn _mm_cmp_round_ss_mask<const IMM5: i32, const SAE: i32>(
            a: __m128,
            b: __m128,
        ) -> __mmask8;
        fn _mm_mask_cmp_round_ss_mask<const IMM5: i32, const SAE: i32>(
            k1: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __mmask8;
        fn _mm_cmp_sd_mask<const IMM8: i32>(a: __m128d, b: __m128d) -> __mmask8;
        fn _mm_mask_cmp_sd_mask<const IMM8: i32>(
            k1: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __mmask8;
        fn _mm_cmp_round_sd_mask<const IMM5: i32, const SAE: i32>(
            a: __m128d,
            b: __m128d,
        ) -> __mmask8;
        fn _mm_mask_cmp_round_sd_mask<const IMM5: i32, const SAE: i32>(
            k1: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __mmask8;
        fn _mm512_cmplt_epu32_mask(a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_mask_cmplt_epu32_mask(k1: __mmask16, a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_cmpgt_epu32_mask(a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_mask_cmpgt_epu32_mask(k1: __mmask16, a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_cmple_epu32_mask(a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_mask_cmple_epu32_mask(k1: __mmask16, a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_cmpge_epu32_mask(a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_mask_cmpge_epu32_mask(k1: __mmask16, a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_cmpeq_epu32_mask(a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_mask_cmpeq_epu32_mask(k1: __mmask16, a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_cmpneq_epu32_mask(a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_mask_cmpneq_epu32_mask(k1: __mmask16, a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_cmp_epu32_mask<const IMM3: _MM_CMPINT_ENUM>(
            a: __m512i,
            b: __m512i,
        ) -> __mmask16;
        fn _mm512_mask_cmp_epu32_mask<const IMM3: _MM_CMPINT_ENUM>(
            k1: __mmask16,
            a: __m512i,
            b: __m512i,
        ) -> __mmask16;
        fn _mm512_cmplt_epi32_mask(a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_mask_cmplt_epi32_mask(k1: __mmask16, a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_cmpgt_epi32_mask(a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_mask_cmpgt_epi32_mask(k1: __mmask16, a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_cmple_epi32_mask(a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_mask_cmple_epi32_mask(k1: __mmask16, a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_cmpge_epi32_mask(a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_mask_cmpge_epi32_mask(k1: __mmask16, a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_cmpeq_epi32_mask(a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_mask_cmpeq_epi32_mask(k1: __mmask16, a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_cmpneq_epi32_mask(a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_mask_cmpneq_epi32_mask(k1: __mmask16, a: __m512i, b: __m512i) -> __mmask16;
        fn _mm512_cmp_epi32_mask<const IMM3: _MM_CMPINT_ENUM>(
            a: __m512i,
            b: __m512i,
        ) -> __mmask16;
        fn _mm512_mask_cmp_epi32_mask<const IMM3: _MM_CMPINT_ENUM>(
            k1: __mmask16,
            a: __m512i,
            b: __m512i,
        ) -> __mmask16;
        fn _mm512_cmplt_epu64_mask(a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_mask_cmplt_epu64_mask(k1: __mmask8, a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_cmpgt_epu64_mask(a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_mask_cmpgt_epu64_mask(k1: __mmask8, a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_cmple_epu64_mask(a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_mask_cmple_epu64_mask(k1: __mmask8, a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_cmpge_epu64_mask(a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_mask_cmpge_epu64_mask(k1: __mmask8, a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_cmpeq_epu64_mask(a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_mask_cmpeq_epu64_mask(k1: __mmask8, a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_cmpneq_epu64_mask(a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_mask_cmpneq_epu64_mask(k1: __mmask8, a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_cmp_epu64_mask<const IMM3: _MM_CMPINT_ENUM>(
            a: __m512i,
            b: __m512i,
        ) -> __mmask8;
        fn _mm512_mask_cmp_epu64_mask<const IMM3: _MM_CMPINT_ENUM>(
            k1: __mmask8,
            a: __m512i,
            b: __m512i,
        ) -> __mmask8;
        fn _mm512_cmplt_epi64_mask(a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_mask_cmplt_epi64_mask(k1: __mmask8, a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_cmpgt_epi64_mask(a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_mask_cmpgt_epi64_mask(k1: __mmask8, a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_cmple_epi64_mask(a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_mask_cmple_epi64_mask(k1: __mmask8, a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_cmpge_epi64_mask(a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_mask_cmpge_epi64_mask(k1: __mmask8, a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_cmpeq_epi64_mask(a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_mask_cmpeq_epi64_mask(k1: __mmask8, a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_cmpneq_epi64_mask(a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_mask_cmpneq_epi64_mask(k1: __mmask8, a: __m512i, b: __m512i) -> __mmask8;
        fn _mm512_cmp_epi64_mask<const IMM3: _MM_CMPINT_ENUM>(
            a: __m512i,
            b: __m512i,
        ) -> __mmask8;
        fn _mm512_mask_cmp_epi64_mask<const IMM3: _MM_CMPINT_ENUM>(
            k1: __mmask8,
            a: __m512i,
            b: __m512i,
        ) -> __mmask8;
        fn _mm512_reduce_add_epi32(a: __m512i) -> i32;
        fn _mm512_mask_reduce_add_epi32(k: __mmask16, a: __m512i) -> i32;
        fn _mm512_reduce_add_epi64(a: __m512i) -> i64;
        fn _mm512_mask_reduce_add_epi64(k: __mmask8, a: __m512i) -> i64;
        fn _mm512_reduce_add_ps(a: __m512) -> f32;
        fn _mm512_mask_reduce_add_ps(k: __mmask16, a: __m512) -> f32;
        fn _mm512_reduce_add_pd(a: __m512d) -> f64;
        fn _mm512_mask_reduce_add_pd(k: __mmask8, a: __m512d) -> f64;
        fn _mm512_reduce_mul_epi32(a: __m512i) -> i32;
        fn _mm512_mask_reduce_mul_epi32(k: __mmask16, a: __m512i) -> i32;
        fn _mm512_reduce_mul_epi64(a: __m512i) -> i64;
        fn _mm512_mask_reduce_mul_epi64(k: __mmask8, a: __m512i) -> i64;
        fn _mm512_reduce_mul_ps(a: __m512) -> f32;
        fn _mm512_mask_reduce_mul_ps(k: __mmask16, a: __m512) -> f32;
        fn _mm512_reduce_mul_pd(a: __m512d) -> f64;
        fn _mm512_mask_reduce_mul_pd(k: __mmask8, a: __m512d) -> f64;
        fn _mm512_reduce_max_epi32(a: __m512i) -> i32;
        fn _mm512_mask_reduce_max_epi32(k: __mmask16, a: __m512i) -> i32;
        fn _mm512_reduce_max_epi64(a: __m512i) -> i64;
        fn _mm512_mask_reduce_max_epi64(k: __mmask8, a: __m512i) -> i64;
        fn _mm512_reduce_max_epu32(a: __m512i) -> u32;
        fn _mm512_mask_reduce_max_epu32(k: __mmask16, a: __m512i) -> u32;
        fn _mm512_reduce_max_epu64(a: __m512i) -> u64;
        fn _mm512_mask_reduce_max_epu64(k: __mmask8, a: __m512i) -> u64;
        fn _mm512_reduce_max_ps(a: __m512) -> f32;
        fn _mm512_mask_reduce_max_ps(k: __mmask16, a: __m512) -> f32;
        fn _mm512_reduce_max_pd(a: __m512d) -> f64;
        fn _mm512_mask_reduce_max_pd(k: __mmask8, a: __m512d) -> f64;
        fn _mm512_reduce_min_epi32(a: __m512i) -> i32;
        fn _mm512_mask_reduce_min_epi32(k: __mmask16, a: __m512i) -> i32;
        fn _mm512_reduce_min_epi64(a: __m512i) -> i64;
        fn _mm512_mask_reduce_min_epi64(k: __mmask8, a: __m512i) -> i64;
        fn _mm512_reduce_min_epu32(a: __m512i) -> u32;
        fn _mm512_mask_reduce_min_epu32(k: __mmask16, a: __m512i) -> u32;
        fn _mm512_reduce_min_epu64(a: __m512i) -> u64;
        fn _mm512_mask_reduce_min_epu64(k: __mmask8, a: __m512i) -> u64;
        fn _mm512_reduce_min_ps(a: __m512) -> f32;
        fn _mm512_mask_reduce_min_ps(k: __mmask16, a: __m512) -> f32;
        fn _mm512_reduce_min_pd(a: __m512d) -> f64;
        fn _mm512_mask_reduce_min_pd(k: __mmask8, a: __m512d) -> f64;
        fn _mm512_reduce_and_epi32(a: __m512i) -> i32;
        fn _mm512_mask_reduce_and_epi32(k: __mmask16, a: __m512i) -> i32;
        fn _mm512_reduce_and_epi64(a: __m512i) -> i64;
        fn _mm512_mask_reduce_and_epi64(k: __mmask8, a: __m512i) -> i64;
        fn _mm512_reduce_or_epi32(a: __m512i) -> i32;
        fn _mm512_mask_reduce_or_epi32(k: __mmask16, a: __m512i) -> i32;
        fn _mm512_reduce_or_epi64(a: __m512i) -> i64;
        fn _mm512_mask_reduce_or_epi64(k: __mmask8, a: __m512i) -> i64;
        // This intrinsic has no corresponding instruction.
        fn _mm512_undefined_pd() -> __m512d;
        // This intrinsic has no corresponding instruction.
        fn _mm512_undefined_ps() -> __m512;
        // This intrinsic has no corresponding instruction.
        fn _mm512_undefined_epi32() -> __m512i;
        // This intrinsic has no corresponding instruction.
        fn _mm512_undefined() -> __m512;
        unsafe fn _mm512_loadu_epi32(mem_addr: *const i32) -> __m512i;
        unsafe fn _mm512_mask_cvtepi32_storeu_epi16(mem_addr: *mut i8, k: __mmask16, a: __m512i);
        unsafe fn _mm512_mask_cvtsepi32_storeu_epi16(mem_addr: *mut i8, k: __mmask16, a: __m512i);
        unsafe fn _mm512_mask_cvtusepi32_storeu_epi16(mem_addr: *mut i8, k: __mmask16, a: __m512i);
        unsafe fn _mm512_mask_cvtepi32_storeu_epi8(mem_addr: *mut i8, k: __mmask16, a: __m512i);
        unsafe fn _mm512_mask_cvtsepi32_storeu_epi8(mem_addr: *mut i8, k: __mmask16, a: __m512i);
        unsafe fn _mm512_mask_cvtusepi32_storeu_epi8(mem_addr: *mut i8, k: __mmask16, a: __m512i);
        unsafe fn _mm512_mask_cvtepi64_storeu_epi16(mem_addr: *mut i8, k: __mmask8, a: __m512i);
        unsafe fn _mm512_mask_cvtsepi64_storeu_epi16(mem_addr: *mut i8, k: __mmask8, a: __m512i);
        unsafe fn _mm512_mask_cvtusepi64_storeu_epi16(mem_addr: *mut i8, k: __mmask8, a: __m512i);
        unsafe fn _mm512_mask_cvtepi64_storeu_epi8(mem_addr: *mut i8, k: __mmask8, a: __m512i);
        unsafe fn _mm512_mask_cvtsepi64_storeu_epi8(mem_addr: *mut i8, k: __mmask8, a: __m512i);
        unsafe fn _mm512_mask_cvtusepi64_storeu_epi8(mem_addr: *mut i8, k: __mmask8, a: __m512i);
        unsafe fn _mm512_mask_cvtepi64_storeu_epi32(mem_addr: *mut i8, k: __mmask8, a: __m512i);
        unsafe fn _mm512_mask_cvtsepi64_storeu_epi32(mem_addr: *mut i8, k: __mmask8, a: __m512i);
        unsafe fn _mm512_mask_cvtusepi64_storeu_epi32(mem_addr: *mut i8, k: __mmask8, a: __m512i);
        unsafe fn _mm512_storeu_epi32(mem_addr: *mut i32, a: __m512i);
        unsafe fn _mm512_loadu_epi64(mem_addr: *const i64) -> __m512i;
        unsafe fn _mm512_storeu_epi64(mem_addr: *mut i64, a: __m512i);
        unsafe fn _mm512_loadu_si512(mem_addr: *const i32) -> __m512i;
        unsafe fn _mm512_storeu_si512(mem_addr: *mut i32, a: __m512i);
        unsafe fn _mm512_loadu_pd(mem_addr: *const f64) -> __m512d;
        unsafe fn _mm512_storeu_pd(mem_addr: *mut f64, a: __m512d);
        unsafe fn _mm512_loadu_ps(mem_addr: *const f32) -> __m512;
        unsafe fn _mm512_storeu_ps(mem_addr: *mut f32, a: __m512);
        unsafe fn _mm512_load_si512(mem_addr: *const i32) -> __m512i;
        unsafe fn _mm512_store_si512(mem_addr: *mut i32, a: __m512i);
        unsafe fn _mm512_load_epi32(mem_addr: *const i32) -> __m512i;
        unsafe fn _mm512_store_epi32(mem_addr: *mut i32, a: __m512i);
        unsafe fn _mm512_load_epi64(mem_addr: *const i64) -> __m512i;
        unsafe fn _mm512_store_epi64(mem_addr: *mut i64, a: __m512i);
        unsafe fn _mm512_load_ps(mem_addr: *const f32) -> __m512;
        unsafe fn _mm512_store_ps(mem_addr: *mut f32, a: __m512);
        unsafe fn _mm512_load_pd(mem_addr: *const f64) -> __m512d;
        unsafe fn _mm512_store_pd(mem_addr: *mut f64, a: __m512d);
        unsafe fn _mm512_mask_loadu_epi32(src: __m512i, k: __mmask16, mem_addr: *const i32) -> __m512i;
        unsafe fn _mm512_maskz_loadu_epi32(k: __mmask16, mem_addr: *const i32) -> __m512i;
        unsafe fn _mm512_mask_loadu_epi64(src: __m512i, k: __mmask8, mem_addr: *const i64) -> __m512i;
        unsafe fn _mm512_maskz_loadu_epi64(k: __mmask8, mem_addr: *const i64) -> __m512i;
        unsafe fn _mm512_mask_loadu_ps(src: __m512, k: __mmask16, mem_addr: *const f32) -> __m512;
        unsafe fn _mm512_maskz_loadu_ps(k: __mmask16, mem_addr: *const f32) -> __m512;
        unsafe fn _mm512_mask_loadu_pd(src: __m512d, k: __mmask8, mem_addr: *const f64) -> __m512d;
        unsafe fn _mm512_maskz_loadu_pd(k: __mmask8, mem_addr: *const f64) -> __m512d;
        unsafe fn _mm256_mask_loadu_epi32(src: __m256i, k: __mmask8, mem_addr: *const i32) -> __m256i;
        unsafe fn _mm256_maskz_loadu_epi32(k: __mmask8, mem_addr: *const i32) -> __m256i;
        unsafe fn _mm256_mask_loadu_epi64(src: __m256i, k: __mmask8, mem_addr: *const i64) -> __m256i;
        unsafe fn _mm256_maskz_loadu_epi64(k: __mmask8, mem_addr: *const i64) -> __m256i;
        unsafe fn _mm256_mask_loadu_ps(src: __m256, k: __mmask8, mem_addr: *const f32) -> __m256;
        unsafe fn _mm256_maskz_loadu_ps(k: __mmask8, mem_addr: *const f32) -> __m256;
        unsafe fn _mm256_mask_loadu_pd(src: __m256d, k: __mmask8, mem_addr: *const f64) -> __m256d;
        unsafe fn _mm256_maskz_loadu_pd(k: __mmask8, mem_addr: *const f64) -> __m256d;
        unsafe fn _mm_mask_loadu_epi32(src: __m128i, k: __mmask8, mem_addr: *const i32) -> __m128i;
        unsafe fn _mm_maskz_loadu_epi32(k: __mmask8, mem_addr: *const i32) -> __m128i;
        unsafe fn _mm_mask_loadu_epi64(src: __m128i, k: __mmask8, mem_addr: *const i64) -> __m128i;
        unsafe fn _mm_maskz_loadu_epi64(k: __mmask8, mem_addr: *const i64) -> __m128i;
        unsafe fn _mm_mask_loadu_ps(src: __m128, k: __mmask8, mem_addr: *const f32) -> __m128;
        unsafe fn _mm_maskz_loadu_ps(k: __mmask8, mem_addr: *const f32) -> __m128;
        unsafe fn _mm_mask_loadu_pd(src: __m128d, k: __mmask8, mem_addr: *const f64) -> __m128d;
        unsafe fn _mm_maskz_loadu_pd(k: __mmask8, mem_addr: *const f64) -> __m128d;
        unsafe fn _mm512_mask_load_epi32(src: __m512i, k: __mmask16, mem_addr: *const i32) -> __m512i;
        unsafe fn _mm512_maskz_load_epi32(k: __mmask16, mem_addr: *const i32) -> __m512i;
        unsafe fn _mm512_mask_load_epi64(src: __m512i, k: __mmask8, mem_addr: *const i64) -> __m512i;
        unsafe fn _mm512_maskz_load_epi64(k: __mmask8, mem_addr: *const i64) -> __m512i;
        unsafe fn _mm512_mask_load_ps(src: __m512, k: __mmask16, mem_addr: *const f32) -> __m512;
        unsafe fn _mm512_maskz_load_ps(k: __mmask16, mem_addr: *const f32) -> __m512;
        unsafe fn _mm512_mask_load_pd(src: __m512d, k: __mmask8, mem_addr: *const f64) -> __m512d;
        unsafe fn _mm512_maskz_load_pd(k: __mmask8, mem_addr: *const f64) -> __m512d;
        unsafe fn _mm256_mask_load_epi32(src: __m256i, k: __mmask8, mem_addr: *const i32) -> __m256i;
        unsafe fn _mm256_maskz_load_epi32(k: __mmask8, mem_addr: *const i32) -> __m256i;
        unsafe fn _mm256_mask_load_epi64(src: __m256i, k: __mmask8, mem_addr: *const i64) -> __m256i;
        unsafe fn _mm256_maskz_load_epi64(k: __mmask8, mem_addr: *const i64) -> __m256i;
        unsafe fn _mm256_mask_load_ps(src: __m256, k: __mmask8, mem_addr: *const f32) -> __m256;
        unsafe fn _mm256_maskz_load_ps(k: __mmask8, mem_addr: *const f32) -> __m256;
        unsafe fn _mm256_mask_load_pd(src: __m256d, k: __mmask8, mem_addr: *const f64) -> __m256d;
        unsafe fn _mm256_maskz_load_pd(k: __mmask8, mem_addr: *const f64) -> __m256d;
        unsafe fn _mm_mask_load_epi32(src: __m128i, k: __mmask8, mem_addr: *const i32) -> __m128i;
        unsafe fn _mm_maskz_load_epi32(k: __mmask8, mem_addr: *const i32) -> __m128i;
        unsafe fn _mm_mask_load_epi64(src: __m128i, k: __mmask8, mem_addr: *const i64) -> __m128i;
        unsafe fn _mm_maskz_load_epi64(k: __mmask8, mem_addr: *const i64) -> __m128i;
        unsafe fn _mm_mask_load_ps(src: __m128, k: __mmask8, mem_addr: *const f32) -> __m128;
        unsafe fn _mm_maskz_load_ps(k: __mmask8, mem_addr: *const f32) -> __m128;
        unsafe fn _mm_mask_load_pd(src: __m128d, k: __mmask8, mem_addr: *const f64) -> __m128d;
        unsafe fn _mm_maskz_load_pd(k: __mmask8, mem_addr: *const f64) -> __m128d;
        unsafe fn _mm512_mask_storeu_epi32(mem_addr: *mut i32, mask: __mmask16, a: __m512i);
        unsafe fn _mm512_mask_storeu_epi64(mem_addr: *mut i64, mask: __mmask8, a: __m512i);
        unsafe fn _mm512_mask_storeu_ps(mem_addr: *mut f32, mask: __mmask16, a: __m512);
        unsafe fn _mm512_mask_storeu_pd(mem_addr: *mut f64, mask: __mmask8, a: __m512d);
        unsafe fn _mm256_mask_storeu_epi32(mem_addr: *mut i32, mask: __mmask8, a: __m256i);
        unsafe fn _mm256_mask_storeu_epi64(mem_addr: *mut i64, mask: __mmask8, a: __m256i);
        unsafe fn _mm256_mask_storeu_ps(mem_addr: *mut f32, mask: __mmask8, a: __m256);
        unsafe fn _mm256_mask_storeu_pd(mem_addr: *mut f64, mask: __mmask8, a: __m256d);
        unsafe fn _mm_mask_storeu_epi32(mem_addr: *mut i32, mask: __mmask8, a: __m128i);
        unsafe fn _mm_mask_storeu_epi64(mem_addr: *mut i64, mask: __mmask8, a: __m128i);
        unsafe fn _mm_mask_storeu_ps(mem_addr: *mut f32, mask: __mmask8, a: __m128);
        unsafe fn _mm_mask_storeu_pd(mem_addr: *mut f64, mask: __mmask8, a: __m128d);
        unsafe fn _mm512_mask_store_epi32(mem_addr: *mut i32, mask: __mmask16, a: __m512i);
        unsafe fn _mm512_mask_store_epi64(mem_addr: *mut i64, mask: __mmask8, a: __m512i);
        unsafe fn _mm512_mask_store_ps(mem_addr: *mut f32, mask: __mmask16, a: __m512);
        unsafe fn _mm512_mask_store_pd(mem_addr: *mut f64, mask: __mmask8, a: __m512d);
        unsafe fn _mm256_mask_store_epi32(mem_addr: *mut i32, mask: __mmask8, a: __m256i);
        unsafe fn _mm256_mask_store_epi64(mem_addr: *mut i64, mask: __mmask8, a: __m256i);
        unsafe fn _mm256_mask_store_ps(mem_addr: *mut f32, mask: __mmask8, a: __m256);
        unsafe fn _mm256_mask_store_pd(mem_addr: *mut f64, mask: __mmask8, a: __m256d);
        unsafe fn _mm_mask_store_epi32(mem_addr: *mut i32, mask: __mmask8, a: __m128i);
        unsafe fn _mm_mask_store_epi64(mem_addr: *mut i64, mask: __mmask8, a: __m128i);
        unsafe fn _mm_mask_store_ps(mem_addr: *mut f32, mask: __mmask8, a: __m128);
        unsafe fn _mm_mask_store_pd(mem_addr: *mut f64, mask: __mmask8, a: __m128d);
        unsafe fn _mm512_mask_expandloadu_epi32(
            src: __m512i,
            k: __mmask16,
            mem_addr: *const i32,
        ) -> __m512i;
        unsafe fn _mm512_maskz_expandloadu_epi32(k: __mmask16, mem_addr: *const i32) -> __m512i;
        unsafe fn _mm256_mask_expandloadu_epi32(
            src: __m256i,
            k: __mmask8,
            mem_addr: *const i32,
        ) -> __m256i;
        unsafe fn _mm256_maskz_expandloadu_epi32(k: __mmask8, mem_addr: *const i32) -> __m256i;
        unsafe fn _mm_mask_expandloadu_epi32(
            src: __m128i,
            k: __mmask8,
            mem_addr: *const i32,
        ) -> __m128i;
        unsafe fn _mm_maskz_expandloadu_epi32(k: __mmask8, mem_addr: *const i32) -> __m128i;
        unsafe fn _mm512_mask_expandloadu_epi64(
            src: __m512i,
            k: __mmask8,
            mem_addr: *const i64,
        ) -> __m512i;
        unsafe fn _mm512_maskz_expandloadu_epi64(k: __mmask8, mem_addr: *const i64) -> __m512i;
        unsafe fn _mm256_mask_expandloadu_epi64(
            src: __m256i,
            k: __mmask8,
            mem_addr: *const i64,
        ) -> __m256i;
        unsafe fn _mm256_maskz_expandloadu_epi64(k: __mmask8, mem_addr: *const i64) -> __m256i;
        unsafe fn _mm_mask_expandloadu_epi64(
            src: __m128i,
            k: __mmask8,
            mem_addr: *const i64,
        ) -> __m128i;
        unsafe fn _mm_maskz_expandloadu_epi64(k: __mmask8, mem_addr: *const i64) -> __m128i;
        unsafe fn _mm512_mask_expandloadu_ps(
            src: __m512,
            k: __mmask16,
            mem_addr: *const f32,
        ) -> __m512;
        unsafe fn _mm512_maskz_expandloadu_ps(k: __mmask16, mem_addr: *const f32) -> __m512;
        unsafe fn _mm256_mask_expandloadu_ps(src: __m256, k: __mmask8, mem_addr: *const f32) -> __m256;
        unsafe fn _mm256_maskz_expandloadu_ps(k: __mmask8, mem_addr: *const f32) -> __m256;
        unsafe fn _mm_mask_expandloadu_ps(src: __m128, k: __mmask8, mem_addr: *const f32) -> __m128;
        unsafe fn _mm_maskz_expandloadu_ps(k: __mmask8, mem_addr: *const f32) -> __m128;
        unsafe fn _mm512_mask_expandloadu_pd(
            src: __m512d,
            k: __mmask8,
            mem_addr: *const f64,
        ) -> __m512d;
        unsafe fn _mm512_maskz_expandloadu_pd(k: __mmask8, mem_addr: *const f64) -> __m512d;
        unsafe fn _mm256_mask_expandloadu_pd(
            src: __m256d,
            k: __mmask8,
            mem_addr: *const f64,
        ) -> __m256d;
        unsafe fn _mm256_maskz_expandloadu_pd(k: __mmask8, mem_addr: *const f64) -> __m256d;
        unsafe fn _mm_mask_expandloadu_pd(src: __m128d, k: __mmask8, mem_addr: *const f64) -> __m128d;
        unsafe fn _mm_maskz_expandloadu_pd(k: __mmask8, mem_addr: *const f64) -> __m128d;
        fn _mm512_setr_pd(
            e0: f64,
            e1: f64,
            e2: f64,
            e3: f64,
            e4: f64,
            e5: f64,
            e6: f64,
            e7: f64,
        ) -> __m512d;
        fn _mm512_set_pd(
            e0: f64,
            e1: f64,
            e2: f64,
            e3: f64,
            e4: f64,
            e5: f64,
            e6: f64,
            e7: f64,
        ) -> __m512d;
        fn _mm_mask_move_ss(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_move_ss(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_mask_move_sd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_move_sd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_add_ss(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_add_ss(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_mask_add_sd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_add_sd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_sub_ss(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_sub_ss(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_mask_sub_sd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_sub_sd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_mul_ss(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_mul_ss(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_mask_mul_sd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_mul_sd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_div_ss(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_div_ss(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_mask_div_sd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_div_sd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_max_ss(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_max_ss(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_mask_max_sd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_max_sd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_min_ss(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_min_ss(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_mask_min_sd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_min_sd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_sqrt_ss(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_sqrt_ss(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_mask_sqrt_sd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_sqrt_sd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_rsqrt14_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_mask_rsqrt14_ss(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_rsqrt14_ss(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_rsqrt14_sd(a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_rsqrt14_sd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_rsqrt14_sd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_rcp14_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_mask_rcp14_ss(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_rcp14_ss(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_rcp14_sd(a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_rcp14_sd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_rcp14_sd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_getexp_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_mask_getexp_ss(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_getexp_ss(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_getexp_sd(a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_getexp_sd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_getexp_sd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_getmant_ss<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_mask_getmant_ss<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            src: __m128,
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_maskz_getmant_ss<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_getmant_sd<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_mask_getmant_sd<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            src: __m128d,
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_maskz_getmant_sd<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_roundscale_ss<const IMM8: i32>(a: __m128, b: __m128) -> __m128;
        fn _mm_mask_roundscale_ss<const IMM8: i32>(
            src: __m128,
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_maskz_roundscale_ss<const IMM8: i32>(
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_roundscale_sd<const IMM8: i32>(a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_roundscale_sd<const IMM8: i32>(
            src: __m128d,
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_maskz_roundscale_sd<const IMM8: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_scalef_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_mask_scalef_ss(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_scalef_ss(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_scalef_sd(a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_scalef_sd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_scalef_sd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_fmadd_ss(a: __m128, k: __mmask8, b: __m128, c: __m128) -> __m128;
        fn _mm_maskz_fmadd_ss(k: __mmask8, a: __m128, b: __m128, c: __m128) -> __m128;
        fn _mm_mask3_fmadd_ss(a: __m128, b: __m128, c: __m128, k: __mmask8) -> __m128;
        fn _mm_mask_fmadd_sd(a: __m128d, k: __mmask8, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_maskz_fmadd_sd(k: __mmask8, a: __m128d, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_mask3_fmadd_sd(a: __m128d, b: __m128d, c: __m128d, k: __mmask8) -> __m128d;
        fn _mm_mask_fmsub_ss(a: __m128, k: __mmask8, b: __m128, c: __m128) -> __m128;
        fn _mm_maskz_fmsub_ss(k: __mmask8, a: __m128, b: __m128, c: __m128) -> __m128;
        fn _mm_mask3_fmsub_ss(a: __m128, b: __m128, c: __m128, k: __mmask8) -> __m128;
        fn _mm_mask_fmsub_sd(a: __m128d, k: __mmask8, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_maskz_fmsub_sd(k: __mmask8, a: __m128d, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_mask3_fmsub_sd(a: __m128d, b: __m128d, c: __m128d, k: __mmask8) -> __m128d;
        fn _mm_mask_fnmadd_ss(a: __m128, k: __mmask8, b: __m128, c: __m128) -> __m128;
        fn _mm_maskz_fnmadd_ss(k: __mmask8, a: __m128, b: __m128, c: __m128) -> __m128;
        fn _mm_mask3_fnmadd_ss(a: __m128, b: __m128, c: __m128, k: __mmask8) -> __m128;
        fn _mm_mask_fnmadd_sd(a: __m128d, k: __mmask8, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_maskz_fnmadd_sd(k: __mmask8, a: __m128d, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_mask3_fnmadd_sd(a: __m128d, b: __m128d, c: __m128d, k: __mmask8) -> __m128d;
        fn _mm_mask_fnmsub_ss(a: __m128, k: __mmask8, b: __m128, c: __m128) -> __m128;
        fn _mm_maskz_fnmsub_ss(k: __mmask8, a: __m128, b: __m128, c: __m128) -> __m128;
        fn _mm_mask3_fnmsub_ss(a: __m128, b: __m128, c: __m128, k: __mmask8) -> __m128;
        fn _mm_mask_fnmsub_sd(a: __m128d, k: __mmask8, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_maskz_fnmsub_sd(k: __mmask8, a: __m128d, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_mask3_fnmsub_sd(a: __m128d, b: __m128d, c: __m128d, k: __mmask8) -> __m128d;
        fn _mm_add_round_ss<const ROUNDING: i32>(a: __m128, b: __m128) -> __m128;
        fn _mm_mask_add_round_ss<const ROUNDING: i32>(
            src: __m128,
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_maskz_add_round_ss<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_add_round_sd<const ROUNDING: i32>(a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_add_round_sd<const ROUNDING: i32>(
            src: __m128d,
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_maskz_add_round_sd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_sub_round_ss<const ROUNDING: i32>(a: __m128, b: __m128) -> __m128;
        fn _mm_mask_sub_round_ss<const ROUNDING: i32>(
            src: __m128,
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_maskz_sub_round_ss<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_sub_round_sd<const ROUNDING: i32>(a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_sub_round_sd<const ROUNDING: i32>(
            src: __m128d,
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_maskz_sub_round_sd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_mul_round_ss<const ROUNDING: i32>(a: __m128, b: __m128) -> __m128;
        fn _mm_mask_mul_round_ss<const ROUNDING: i32>(
            src: __m128,
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_maskz_mul_round_ss<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_mul_round_sd<const ROUNDING: i32>(a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_mul_round_sd<const ROUNDING: i32>(
            src: __m128d,
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_maskz_mul_round_sd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_div_round_ss<const ROUNDING: i32>(a: __m128, b: __m128) -> __m128;
        fn _mm_mask_div_round_ss<const ROUNDING: i32>(
            src: __m128,
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_maskz_div_round_ss<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_div_round_sd<const ROUNDING: i32>(a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_div_round_sd<const ROUNDING: i32>(
            src: __m128d,
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_maskz_div_round_sd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_max_round_ss<const SAE: i32>(a: __m128, b: __m128) -> __m128;
        fn _mm_mask_max_round_ss<const SAE: i32>(
            src: __m128,
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_maskz_max_round_ss<const SAE: i32>(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_max_round_sd<const SAE: i32>(a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_max_round_sd<const SAE: i32>(
            src: __m128d,
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_maskz_max_round_sd<const SAE: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_min_round_ss<const SAE: i32>(a: __m128, b: __m128) -> __m128;
        fn _mm_mask_min_round_ss<const SAE: i32>(
            src: __m128,
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_maskz_min_round_ss<const SAE: i32>(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_min_round_sd<const SAE: i32>(a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_min_round_sd<const SAE: i32>(
            src: __m128d,
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_maskz_min_round_sd<const SAE: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_sqrt_round_ss<const ROUNDING: i32>(a: __m128, b: __m128) -> __m128;
        fn _mm_mask_sqrt_round_ss<const ROUNDING: i32>(
            src: __m128,
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_maskz_sqrt_round_ss<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_sqrt_round_sd<const ROUNDING: i32>(a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_sqrt_round_sd<const ROUNDING: i32>(
            src: __m128d,
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_maskz_sqrt_round_sd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_getexp_round_ss<const SAE: i32>(a: __m128, b: __m128) -> __m128;
        fn _mm_mask_getexp_round_ss<const SAE: i32>(
            src: __m128,
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_maskz_getexp_round_ss<const SAE: i32>(
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_getexp_round_sd<const SAE: i32>(a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_getexp_round_sd<const SAE: i32>(
            src: __m128d,
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_maskz_getexp_round_sd<const SAE: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_getmant_round_ss<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
            const SAE: i32,
        >(
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_mask_getmant_round_ss<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
            const SAE: i32,
        >(
            src: __m128,
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_maskz_getmant_round_ss<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
            const SAE: i32,
        >(
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_getmant_round_sd<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
            const SAE: i32,
        >(
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_mask_getmant_round_sd<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
            const SAE: i32,
        >(
            src: __m128d,
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_maskz_getmant_round_sd<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
            const SAE: i32,
        >(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_roundscale_round_ss<const IMM8: i32, const SAE: i32>(
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_mask_roundscale_round_ss<const IMM8: i32, const SAE: i32>(
            src: __m128,
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_maskz_roundscale_round_ss<const IMM8: i32, const SAE: i32>(
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_roundscale_round_sd<const IMM8: i32, const SAE: i32>(
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_mask_roundscale_round_sd<const IMM8: i32, const SAE: i32>(
            src: __m128d,
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_maskz_roundscale_round_sd<const IMM8: i32, const SAE: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_scalef_round_ss<const ROUNDING: i32>(a: __m128, b: __m128) -> __m128;
        fn _mm_mask_scalef_round_ss<const ROUNDING: i32>(
            src: __m128,
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_maskz_scalef_round_ss<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_scalef_round_sd<const ROUNDING: i32>(a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_scalef_round_sd<const ROUNDING: i32>(
            src: __m128d,
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_maskz_scalef_round_sd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_fmadd_round_ss<const ROUNDING: i32>(a: __m128, b: __m128, c: __m128) -> __m128;
        fn _mm_mask_fmadd_round_ss<const ROUNDING: i32>(
            a: __m128,
            k: __mmask8,
            b: __m128,
            c: __m128,
        ) -> __m128;
        fn _mm_maskz_fmadd_round_ss<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128,
            b: __m128,
            c: __m128,
        ) -> __m128;
        fn _mm_mask3_fmadd_round_ss<const ROUNDING: i32>(
            a: __m128,
            b: __m128,
            c: __m128,
            k: __mmask8,
        ) -> __m128;
        fn _mm_fmadd_round_sd<const ROUNDING: i32>(
            a: __m128d,
            b: __m128d,
            c: __m128d,
        ) -> __m128d;
        fn _mm_mask_fmadd_round_sd<const ROUNDING: i32>(
            a: __m128d,
            k: __mmask8,
            b: __m128d,
            c: __m128d,
        ) -> __m128d;
        fn _mm_maskz_fmadd_round_sd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
            c: __m128d,
        ) -> __m128d;
        fn _mm_mask3_fmadd_round_sd<const ROUNDING: i32>(
            a: __m128d,
            b: __m128d,
            c: __m128d,
            k: __mmask8,
        ) -> __m128d;
        fn _mm_fmsub_round_ss<const ROUNDING: i32>(a: __m128, b: __m128, c: __m128) -> __m128;
        fn _mm_mask_fmsub_round_ss<const ROUNDING: i32>(
            a: __m128,
            k: __mmask8,
            b: __m128,
            c: __m128,
        ) -> __m128;
        fn _mm_maskz_fmsub_round_ss<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128,
            b: __m128,
            c: __m128,
        ) -> __m128;
        fn _mm_mask3_fmsub_round_ss<const ROUNDING: i32>(
            a: __m128,
            b: __m128,
            c: __m128,
            k: __mmask8,
        ) -> __m128;
        fn _mm_fmsub_round_sd<const ROUNDING: i32>(
            a: __m128d,
            b: __m128d,
            c: __m128d,
        ) -> __m128d;
        fn _mm_mask_fmsub_round_sd<const ROUNDING: i32>(
            a: __m128d,
            k: __mmask8,
            b: __m128d,
            c: __m128d,
        ) -> __m128d;
        fn _mm_maskz_fmsub_round_sd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
            c: __m128d,
        ) -> __m128d;
        fn _mm_mask3_fmsub_round_sd<const ROUNDING: i32>(
            a: __m128d,
            b: __m128d,
            c: __m128d,
            k: __mmask8,
        ) -> __m128d;
        fn _mm_fnmadd_round_ss<const ROUNDING: i32>(a: __m128, b: __m128, c: __m128) -> __m128;
        fn _mm_mask_fnmadd_round_ss<const ROUNDING: i32>(
            a: __m128,
            k: __mmask8,
            b: __m128,
            c: __m128,
        ) -> __m128;
        fn _mm_maskz_fnmadd_round_ss<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128,
            b: __m128,
            c: __m128,
        ) -> __m128;
        fn _mm_mask3_fnmadd_round_ss<const ROUNDING: i32>(
            a: __m128,
            b: __m128,
            c: __m128,
            k: __mmask8,
        ) -> __m128;
        fn _mm_fnmadd_round_sd<const ROUNDING: i32>(
            a: __m128d,
            b: __m128d,
            c: __m128d,
        ) -> __m128d;
        fn _mm_mask_fnmadd_round_sd<const ROUNDING: i32>(
            a: __m128d,
            k: __mmask8,
            b: __m128d,
            c: __m128d,
        ) -> __m128d;
        fn _mm_maskz_fnmadd_round_sd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
            c: __m128d,
        ) -> __m128d;
        fn _mm_mask3_fnmadd_round_sd<const ROUNDING: i32>(
            a: __m128d,
            b: __m128d,
            c: __m128d,
            k: __mmask8,
        ) -> __m128d;
        fn _mm_fnmsub_round_ss<const ROUNDING: i32>(a: __m128, b: __m128, c: __m128) -> __m128;
        fn _mm_mask_fnmsub_round_ss<const ROUNDING: i32>(
            a: __m128,
            k: __mmask8,
            b: __m128,
            c: __m128,
        ) -> __m128;
        fn _mm_maskz_fnmsub_round_ss<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128,
            b: __m128,
            c: __m128,
        ) -> __m128;
        fn _mm_mask3_fnmsub_round_ss<const ROUNDING: i32>(
            a: __m128,
            b: __m128,
            c: __m128,
            k: __mmask8,
        ) -> __m128;
        fn _mm_fnmsub_round_sd<const ROUNDING: i32>(
            a: __m128d,
            b: __m128d,
            c: __m128d,
        ) -> __m128d;
        fn _mm_mask_fnmsub_round_sd<const ROUNDING: i32>(
            a: __m128d,
            k: __mmask8,
            b: __m128d,
            c: __m128d,
        ) -> __m128d;
        fn _mm_maskz_fnmsub_round_sd<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
            c: __m128d,
        ) -> __m128d;
        fn _mm_mask3_fnmsub_round_sd<const ROUNDING: i32>(
            a: __m128d,
            b: __m128d,
            c: __m128d,
            k: __mmask8,
        ) -> __m128d;
        fn _mm_fixupimm_ss<const IMM8: i32>(a: __m128, b: __m128, c: __m128i) -> __m128;
        fn _mm_mask_fixupimm_ss<const IMM8: i32>(
            a: __m128,
            k: __mmask8,
            b: __m128,
            c: __m128i,
        ) -> __m128;
        fn _mm_maskz_fixupimm_ss<const IMM8: i32>(
            k: __mmask8,
            a: __m128,
            b: __m128,
            c: __m128i,
        ) -> __m128;
        fn _mm_fixupimm_sd<const IMM8: i32>(a: __m128d, b: __m128d, c: __m128i) -> __m128d;
        fn _mm_mask_fixupimm_sd<const IMM8: i32>(
            a: __m128d,
            k: __mmask8,
            b: __m128d,
            c: __m128i,
        ) -> __m128d;
        fn _mm_maskz_fixupimm_sd<const IMM8: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
            c: __m128i,
        ) -> __m128d;
        fn _mm_fixupimm_round_ss<const IMM8: i32, const SAE: i32>(
            a: __m128,
            b: __m128,
            c: __m128i,
        ) -> __m128;
        fn _mm_mask_fixupimm_round_ss<const IMM8: i32, const SAE: i32>(
            a: __m128,
            k: __mmask8,
            b: __m128,
            c: __m128i,
        ) -> __m128;
        fn _mm_maskz_fixupimm_round_ss<const IMM8: i32, const SAE: i32>(
            k: __mmask8,
            a: __m128,
            b: __m128,
            c: __m128i,
        ) -> __m128;
        fn _mm_fixupimm_round_sd<const IMM8: i32, const SAE: i32>(
            a: __m128d,
            b: __m128d,
            c: __m128i,
        ) -> __m128d;
        fn _mm_mask_fixupimm_round_sd<const IMM8: i32, const SAE: i32>(
            a: __m128d,
            k: __mmask8,
            b: __m128d,
            c: __m128i,
        ) -> __m128d;
        fn _mm_maskz_fixupimm_round_sd<const IMM8: i32, const SAE: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
            c: __m128i,
        ) -> __m128d;
        fn _mm_mask_cvtss_sd(src: __m128d, k: __mmask8, a: __m128d, b: __m128) -> __m128d;
        fn _mm_maskz_cvtss_sd(k: __mmask8, a: __m128d, b: __m128) -> __m128d;
        fn _mm_mask_cvtsd_ss(src: __m128, k: __mmask8, a: __m128, b: __m128d) -> __m128;
        fn _mm_maskz_cvtsd_ss(k: __mmask8, a: __m128, b: __m128d) -> __m128;
        fn _mm_cvt_roundss_sd<const SAE: i32>(a: __m128d, b: __m128) -> __m128d;
        fn _mm_mask_cvt_roundss_sd<const SAE: i32>(
            src: __m128d,
            k: __mmask8,
            a: __m128d,
            b: __m128,
        ) -> __m128d;
        fn _mm_maskz_cvt_roundss_sd<const SAE: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128,
        ) -> __m128d;
        fn _mm_cvt_roundsd_ss<const ROUNDING: i32>(a: __m128, b: __m128d) -> __m128;
        fn _mm_mask_cvt_roundsd_ss<const ROUNDING: i32>(
            src: __m128,
            k: __mmask8,
            a: __m128,
            b: __m128d,
        ) -> __m128;
        fn _mm_maskz_cvt_roundsd_ss<const ROUNDING: i32>(
            k: __mmask8,
            a: __m128,
            b: __m128d,
        ) -> __m128;
        fn _mm_cvt_roundss_si32<const ROUNDING: i32>(a: __m128) -> i32;
        fn _mm_cvt_roundss_i32<const ROUNDING: i32>(a: __m128) -> i32;
        fn _mm_cvt_roundss_u32<const ROUNDING: i32>(a: __m128) -> u32;
        fn _mm_cvtss_i32(a: __m128) -> i32;
        fn _mm_cvtss_u32(a: __m128) -> u32;
        fn _mm_cvt_roundsd_si32<const ROUNDING: i32>(a: __m128d) -> i32;
        fn _mm_cvt_roundsd_i32<const ROUNDING: i32>(a: __m128d) -> i32;
        fn _mm_cvt_roundsd_u32<const ROUNDING: i32>(a: __m128d) -> u32;
        fn _mm_cvtsd_i32(a: __m128d) -> i32;
        fn _mm_cvtsd_u32(a: __m128d) -> u32;
        fn _mm_cvt_roundi32_ss<const ROUNDING: i32>(a: __m128, b: i32) -> __m128;
        fn _mm_cvt_roundsi32_ss<const ROUNDING: i32>(a: __m128, b: i32) -> __m128;
        fn _mm_cvt_roundu32_ss<const ROUNDING: i32>(a: __m128, b: u32) -> __m128;
        fn _mm_cvti32_ss(a: __m128, b: i32) -> __m128;
        fn _mm_cvti32_sd(a: __m128d, b: i32) -> __m128d;
        fn _mm_cvtt_roundss_si32<const SAE: i32>(a: __m128) -> i32;
        fn _mm_cvtt_roundss_i32<const SAE: i32>(a: __m128) -> i32;
        fn _mm_cvtt_roundss_u32<const SAE: i32>(a: __m128) -> u32;
        fn _mm_cvttss_i32(a: __m128) -> i32;
        fn _mm_cvttss_u32(a: __m128) -> u32;
        fn _mm_cvtt_roundsd_si32<const SAE: i32>(a: __m128d) -> i32;
        fn _mm_cvtt_roundsd_i32<const SAE: i32>(a: __m128d) -> i32;
        fn _mm_cvtt_roundsd_u32<const SAE: i32>(a: __m128d) -> u32;
        fn _mm_cvttsd_i32(a: __m128d) -> i32;
        fn _mm_cvttsd_u32(a: __m128d) -> u32;
        fn _mm_cvtu32_ss(a: __m128, b: u32) -> __m128;
        fn _mm_cvtu32_sd(a: __m128d, b: u32) -> __m128d;
        fn _mm_comi_round_ss<const IMM5: i32, const SAE: i32>(a: __m128, b: __m128) -> i32;
        fn _mm_comi_round_sd<const IMM5: i32, const SAE: i32>(a: __m128d, b: __m128d) -> i32;
    }
}

impl Avx512f_Avx512vl {
    delegate! {
        fn _mm256_mask_abs_epi32(src: __m256i, k: __mmask8, a: __m256i) -> __m256i;
        fn _mm256_maskz_abs_epi32(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_mask_abs_epi32(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_abs_epi32(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_abs_epi64(a: __m256i) -> __m256i;
        fn _mm256_mask_abs_epi64(src: __m256i, k: __mmask8, a: __m256i) -> __m256i;
        fn _mm256_maskz_abs_epi64(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm256_mask_mov_epi32(src: __m256i, k: __mmask8, a: __m256i) -> __m256i;
        fn _mm256_maskz_mov_epi32(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_mask_mov_epi32(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_mov_epi32(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_mov_epi64(src: __m256i, k: __mmask8, a: __m256i) -> __m256i;
        fn _mm256_maskz_mov_epi64(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_mask_mov_epi64(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_mov_epi64(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_mov_ps(src: __m256, k: __mmask8, a: __m256) -> __m256;
        fn _mm256_maskz_mov_ps(k: __mmask8, a: __m256) -> __m256;
        fn _mm_mask_mov_ps(src: __m128, k: __mmask8, a: __m128) -> __m128;
        fn _mm_maskz_mov_ps(k: __mmask8, a: __m128) -> __m128;
        fn _mm256_mask_mov_pd(src: __m256d, k: __mmask8, a: __m256d) -> __m256d;
        fn _mm256_maskz_mov_pd(k: __mmask8, a: __m256d) -> __m256d;
        fn _mm_mask_mov_pd(src: __m128d, k: __mmask8, a: __m128d) -> __m128d;
        fn _mm_maskz_mov_pd(k: __mmask8, a: __m128d) -> __m128d;
        fn _mm256_mask_add_epi32(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_add_epi32(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_add_epi32(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_add_epi32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_add_epi64(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_add_epi64(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_add_epi64(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_add_epi64(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_add_ps(src: __m256, k: __mmask8, a: __m256, b: __m256) -> __m256;
        fn _mm256_maskz_add_ps(k: __mmask8, a: __m256, b: __m256) -> __m256;
        fn _mm_mask_add_ps(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_add_ps(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm256_mask_add_pd(src: __m256d, k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_maskz_add_pd(k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
        fn _mm_mask_add_pd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_add_pd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm256_mask_sub_epi32(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_sub_epi32(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_sub_epi32(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_sub_epi32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_sub_epi64(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_sub_epi64(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_sub_epi64(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_sub_epi64(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_sub_ps(src: __m256, k: __mmask8, a: __m256, b: __m256) -> __m256;
        fn _mm256_maskz_sub_ps(k: __mmask8, a: __m256, b: __m256) -> __m256;
        fn _mm_mask_sub_ps(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_sub_ps(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm256_mask_sub_pd(src: __m256d, k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_maskz_sub_pd(k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
        fn _mm_mask_sub_pd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_sub_pd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm256_mask_mul_epi32(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_mul_epi32(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_mul_epi32(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_mul_epi32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_mullo_epi32(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_mullo_epi32(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_mullo_epi32(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_mullo_epi32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_mul_epu32(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_mul_epu32(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_mul_epu32(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_mul_epu32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_mul_ps(src: __m256, k: __mmask8, a: __m256, b: __m256) -> __m256;
        fn _mm256_maskz_mul_ps(k: __mmask8, a: __m256, b: __m256) -> __m256;
        fn _mm_mask_mul_ps(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_mul_ps(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm256_mask_mul_pd(src: __m256d, k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_maskz_mul_pd(k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
        fn _mm_mask_mul_pd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_mul_pd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm256_mask_div_ps(src: __m256, k: __mmask8, a: __m256, b: __m256) -> __m256;
        fn _mm256_maskz_div_ps(k: __mmask8, a: __m256, b: __m256) -> __m256;
        fn _mm_mask_div_ps(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_div_ps(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm256_mask_div_pd(src: __m256d, k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_maskz_div_pd(k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
        fn _mm_mask_div_pd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_div_pd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm256_mask_max_epi32(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_max_epi32(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_max_epi32(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_max_epi32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_max_epi64(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_max_epi64(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_max_epi64(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_max_epi64(a: __m128i, b: __m128i) -> __m128i;
        fn _mm_mask_max_epi64(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_max_epi64(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_max_ps(src: __m256, k: __mmask8, a: __m256, b: __m256) -> __m256;
        fn _mm256_maskz_max_ps(k: __mmask8, a: __m256, b: __m256) -> __m256;
        fn _mm_mask_max_ps(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_max_ps(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm256_mask_max_pd(src: __m256d, k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_maskz_max_pd(k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
        fn _mm_mask_max_pd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_max_pd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm256_mask_max_epu32(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_max_epu32(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_max_epu32(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_max_epu32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_max_epu64(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_max_epu64(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_max_epu64(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_max_epu64(a: __m128i, b: __m128i) -> __m128i;
        fn _mm_mask_max_epu64(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_max_epu64(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_min_epi32(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_min_epi32(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_min_epi32(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_min_epi32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_min_epi64(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_min_epi64(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_min_epi64(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_min_ps(src: __m256, k: __mmask8, a: __m256, b: __m256) -> __m256;
        fn _mm256_maskz_min_ps(k: __mmask8, a: __m256, b: __m256) -> __m256;
        fn _mm_mask_min_ps(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_min_ps(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm256_mask_min_pd(src: __m256d, k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_maskz_min_pd(k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
        fn _mm_mask_min_pd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_min_pd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm256_mask_min_epu32(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_min_epu32(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_min_epu32(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_min_epu32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_min_epu64(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_min_epu64(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_min_epu64(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_min_epu64(a: __m128i, b: __m128i) -> __m128i;
        fn _mm_mask_min_epu64(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_min_epu64(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_sqrt_ps(src: __m256, k: __mmask8, a: __m256) -> __m256;
        fn _mm256_maskz_sqrt_ps(k: __mmask8, a: __m256) -> __m256;
        fn _mm_mask_sqrt_ps(src: __m128, k: __mmask8, a: __m128) -> __m128;
        fn _mm_maskz_sqrt_ps(k: __mmask8, a: __m128) -> __m128;
        fn _mm256_mask_sqrt_pd(src: __m256d, k: __mmask8, a: __m256d) -> __m256d;
        fn _mm256_maskz_sqrt_pd(k: __mmask8, a: __m256d) -> __m256d;
        fn _mm_mask_sqrt_pd(src: __m128d, k: __mmask8, a: __m128d) -> __m128d;
        fn _mm_maskz_sqrt_pd(k: __mmask8, a: __m128d) -> __m128d;
        fn _mm256_mask_fmadd_ps(a: __m256, k: __mmask8, b: __m256, c: __m256) -> __m256;
        fn _mm256_maskz_fmadd_ps(k: __mmask8, a: __m256, b: __m256, c: __m256) -> __m256;
        fn _mm256_mask3_fmadd_ps(a: __m256, b: __m256, c: __m256, k: __mmask8) -> __m256;
        fn _mm_mask_fmadd_ps(a: __m128, k: __mmask8, b: __m128, c: __m128) -> __m128;
        fn _mm_maskz_fmadd_ps(k: __mmask8, a: __m128, b: __m128, c: __m128) -> __m128;
        fn _mm_mask3_fmadd_ps(a: __m128, b: __m128, c: __m128, k: __mmask8) -> __m128;
        fn _mm256_mask_fmadd_pd(a: __m256d, k: __mmask8, b: __m256d, c: __m256d) -> __m256d;
        fn _mm256_maskz_fmadd_pd(k: __mmask8, a: __m256d, b: __m256d, c: __m256d) -> __m256d;
        fn _mm256_mask3_fmadd_pd(a: __m256d, b: __m256d, c: __m256d, k: __mmask8) -> __m256d;
        fn _mm_mask_fmadd_pd(a: __m128d, k: __mmask8, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_maskz_fmadd_pd(k: __mmask8, a: __m128d, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_mask3_fmadd_pd(a: __m128d, b: __m128d, c: __m128d, k: __mmask8) -> __m128d;
        fn _mm256_mask_fmsub_ps(a: __m256, k: __mmask8, b: __m256, c: __m256) -> __m256;
        fn _mm256_maskz_fmsub_ps(k: __mmask8, a: __m256, b: __m256, c: __m256) -> __m256;
        fn _mm256_mask3_fmsub_ps(a: __m256, b: __m256, c: __m256, k: __mmask8) -> __m256;
        fn _mm_mask_fmsub_ps(a: __m128, k: __mmask8, b: __m128, c: __m128) -> __m128;
        fn _mm_maskz_fmsub_ps(k: __mmask8, a: __m128, b: __m128, c: __m128) -> __m128;
        fn _mm_mask3_fmsub_ps(a: __m128, b: __m128, c: __m128, k: __mmask8) -> __m128;
        fn _mm256_mask_fmsub_pd(a: __m256d, k: __mmask8, b: __m256d, c: __m256d) -> __m256d;
        fn _mm256_maskz_fmsub_pd(k: __mmask8, a: __m256d, b: __m256d, c: __m256d) -> __m256d;
        fn _mm256_mask3_fmsub_pd(a: __m256d, b: __m256d, c: __m256d, k: __mmask8) -> __m256d;
        fn _mm_mask_fmsub_pd(a: __m128d, k: __mmask8, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_maskz_fmsub_pd(k: __mmask8, a: __m128d, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_mask3_fmsub_pd(a: __m128d, b: __m128d, c: __m128d, k: __mmask8) -> __m128d;
        fn _mm256_mask_fmaddsub_ps(a: __m256, k: __mmask8, b: __m256, c: __m256) -> __m256;
        fn _mm256_maskz_fmaddsub_ps(k: __mmask8, a: __m256, b: __m256, c: __m256) -> __m256;
        fn _mm256_mask3_fmaddsub_ps(a: __m256, b: __m256, c: __m256, k: __mmask8) -> __m256;
        fn _mm_mask_fmaddsub_ps(a: __m128, k: __mmask8, b: __m128, c: __m128) -> __m128;
        fn _mm_maskz_fmaddsub_ps(k: __mmask8, a: __m128, b: __m128, c: __m128) -> __m128;
        fn _mm_mask3_fmaddsub_ps(a: __m128, b: __m128, c: __m128, k: __mmask8) -> __m128;
        fn _mm256_mask_fmaddsub_pd(a: __m256d, k: __mmask8, b: __m256d, c: __m256d) -> __m256d;
        fn _mm256_maskz_fmaddsub_pd(k: __mmask8, a: __m256d, b: __m256d, c: __m256d) -> __m256d;
        fn _mm256_mask3_fmaddsub_pd(a: __m256d, b: __m256d, c: __m256d, k: __mmask8) -> __m256d;
        fn _mm_mask_fmaddsub_pd(a: __m128d, k: __mmask8, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_maskz_fmaddsub_pd(k: __mmask8, a: __m128d, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_mask3_fmaddsub_pd(a: __m128d, b: __m128d, c: __m128d, k: __mmask8) -> __m128d;
        fn _mm256_mask_fmsubadd_ps(a: __m256, k: __mmask8, b: __m256, c: __m256) -> __m256;
        fn _mm256_maskz_fmsubadd_ps(k: __mmask8, a: __m256, b: __m256, c: __m256) -> __m256;
        fn _mm256_mask3_fmsubadd_ps(a: __m256, b: __m256, c: __m256, k: __mmask8) -> __m256;
        fn _mm_mask_fmsubadd_ps(a: __m128, k: __mmask8, b: __m128, c: __m128) -> __m128;
        fn _mm_maskz_fmsubadd_ps(k: __mmask8, a: __m128, b: __m128, c: __m128) -> __m128;
        fn _mm_mask3_fmsubadd_ps(a: __m128, b: __m128, c: __m128, k: __mmask8) -> __m128;
        fn _mm256_mask_fmsubadd_pd(a: __m256d, k: __mmask8, b: __m256d, c: __m256d) -> __m256d;
        fn _mm256_maskz_fmsubadd_pd(k: __mmask8, a: __m256d, b: __m256d, c: __m256d) -> __m256d;
        fn _mm256_mask3_fmsubadd_pd(a: __m256d, b: __m256d, c: __m256d, k: __mmask8) -> __m256d;
        fn _mm_mask_fmsubadd_pd(a: __m128d, k: __mmask8, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_maskz_fmsubadd_pd(k: __mmask8, a: __m128d, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_mask3_fmsubadd_pd(a: __m128d, b: __m128d, c: __m128d, k: __mmask8) -> __m128d;
        fn _mm256_mask_fnmadd_ps(a: __m256, k: __mmask8, b: __m256, c: __m256) -> __m256;
        fn _mm256_maskz_fnmadd_ps(k: __mmask8, a: __m256, b: __m256, c: __m256) -> __m256;
        fn _mm256_mask3_fnmadd_ps(a: __m256, b: __m256, c: __m256, k: __mmask8) -> __m256;
        fn _mm_mask_fnmadd_ps(a: __m128, k: __mmask8, b: __m128, c: __m128) -> __m128;
        fn _mm_maskz_fnmadd_ps(k: __mmask8, a: __m128, b: __m128, c: __m128) -> __m128;
        fn _mm_mask3_fnmadd_ps(a: __m128, b: __m128, c: __m128, k: __mmask8) -> __m128;
        fn _mm256_mask_fnmadd_pd(a: __m256d, k: __mmask8, b: __m256d, c: __m256d) -> __m256d;
        fn _mm256_maskz_fnmadd_pd(k: __mmask8, a: __m256d, b: __m256d, c: __m256d) -> __m256d;
        fn _mm256_mask3_fnmadd_pd(a: __m256d, b: __m256d, c: __m256d, k: __mmask8) -> __m256d;
        fn _mm_mask_fnmadd_pd(a: __m128d, k: __mmask8, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_maskz_fnmadd_pd(k: __mmask8, a: __m128d, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_mask3_fnmadd_pd(a: __m128d, b: __m128d, c: __m128d, k: __mmask8) -> __m128d;
        fn _mm256_mask_fnmsub_ps(a: __m256, k: __mmask8, b: __m256, c: __m256) -> __m256;
        fn _mm256_maskz_fnmsub_ps(k: __mmask8, a: __m256, b: __m256, c: __m256) -> __m256;
        fn _mm256_mask3_fnmsub_ps(a: __m256, b: __m256, c: __m256, k: __mmask8) -> __m256;
        fn _mm_mask_fnmsub_ps(a: __m128, k: __mmask8, b: __m128, c: __m128) -> __m128;
        fn _mm_maskz_fnmsub_ps(k: __mmask8, a: __m128, b: __m128, c: __m128) -> __m128;
        fn _mm_mask3_fnmsub_ps(a: __m128, b: __m128, c: __m128, k: __mmask8) -> __m128;
        fn _mm256_mask_fnmsub_pd(a: __m256d, k: __mmask8, b: __m256d, c: __m256d) -> __m256d;
        fn _mm256_maskz_fnmsub_pd(k: __mmask8, a: __m256d, b: __m256d, c: __m256d) -> __m256d;
        fn _mm256_mask3_fnmsub_pd(a: __m256d, b: __m256d, c: __m256d, k: __mmask8) -> __m256d;
        fn _mm_mask_fnmsub_pd(a: __m128d, k: __mmask8, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_maskz_fnmsub_pd(k: __mmask8, a: __m128d, b: __m128d, c: __m128d) -> __m128d;
        fn _mm_mask3_fnmsub_pd(a: __m128d, b: __m128d, c: __m128d, k: __mmask8) -> __m128d;
        fn _mm256_rcp14_ps(a: __m256) -> __m256;
        fn _mm256_mask_rcp14_ps(src: __m256, k: __mmask8, a: __m256) -> __m256;
        fn _mm256_maskz_rcp14_ps(k: __mmask8, a: __m256) -> __m256;
        fn _mm_rcp14_ps(a: __m128) -> __m128;
        fn _mm_mask_rcp14_ps(src: __m128, k: __mmask8, a: __m128) -> __m128;
        fn _mm_maskz_rcp14_ps(k: __mmask8, a: __m128) -> __m128;
        fn _mm256_rcp14_pd(a: __m256d) -> __m256d;
        fn _mm256_mask_rcp14_pd(src: __m256d, k: __mmask8, a: __m256d) -> __m256d;
        fn _mm256_maskz_rcp14_pd(k: __mmask8, a: __m256d) -> __m256d;
        fn _mm_rcp14_pd(a: __m128d) -> __m128d;
        fn _mm_mask_rcp14_pd(src: __m128d, k: __mmask8, a: __m128d) -> __m128d;
        fn _mm_maskz_rcp14_pd(k: __mmask8, a: __m128d) -> __m128d;
        fn _mm256_mask_rsqrt14_ps(src: __m256, k: __mmask8, a: __m256) -> __m256;
        fn _mm256_maskz_rsqrt14_ps(k: __mmask8, a: __m256) -> __m256;
        fn _mm_mask_rsqrt14_ps(src: __m128, k: __mmask8, a: __m128) -> __m128;
        fn _mm_maskz_rsqrt14_ps(k: __mmask8, a: __m128) -> __m128;
        fn _mm256_mask_rsqrt14_pd(src: __m256d, k: __mmask8, a: __m256d) -> __m256d;
        fn _mm256_maskz_rsqrt14_pd(k: __mmask8, a: __m256d) -> __m256d;
        fn _mm_mask_rsqrt14_pd(src: __m128d, k: __mmask8, a: __m128d) -> __m128d;
        fn _mm_maskz_rsqrt14_pd(k: __mmask8, a: __m128d) -> __m128d;
        fn _mm256_getexp_ps(a: __m256) -> __m256;
        fn _mm256_mask_getexp_ps(src: __m256, k: __mmask8, a: __m256) -> __m256;
        fn _mm256_maskz_getexp_ps(k: __mmask8, a: __m256) -> __m256;
        fn _mm_getexp_ps(a: __m128) -> __m128;
        fn _mm_mask_getexp_ps(src: __m128, k: __mmask8, a: __m128) -> __m128;
        fn _mm_maskz_getexp_ps(k: __mmask8, a: __m128) -> __m128;
        fn _mm256_getexp_pd(a: __m256d) -> __m256d;
        fn _mm256_mask_getexp_pd(src: __m256d, k: __mmask8, a: __m256d) -> __m256d;
        fn _mm256_maskz_getexp_pd(k: __mmask8, a: __m256d) -> __m256d;
        fn _mm_getexp_pd(a: __m128d) -> __m128d;
        fn _mm_mask_getexp_pd(src: __m128d, k: __mmask8, a: __m128d) -> __m128d;
        fn _mm_maskz_getexp_pd(k: __mmask8, a: __m128d) -> __m128d;
        fn _mm256_roundscale_ps<const IMM8: i32>(a: __m256) -> __m256;
        fn _mm256_mask_roundscale_ps<const IMM8: i32>(
            src: __m256,
            k: __mmask8,
            a: __m256,
        ) -> __m256;
        fn _mm256_maskz_roundscale_ps<const IMM8: i32>(k: __mmask8, a: __m256) -> __m256;
        fn _mm_roundscale_ps<const IMM8: i32>(a: __m128) -> __m128;
        fn _mm_mask_roundscale_ps<const IMM8: i32>(
            src: __m128,
            k: __mmask8,
            a: __m128,
        ) -> __m128;
        fn _mm_maskz_roundscale_ps<const IMM8: i32>(k: __mmask8, a: __m128) -> __m128;
        fn _mm256_roundscale_pd<const IMM8: i32>(a: __m256d) -> __m256d;
        fn _mm256_mask_roundscale_pd<const IMM8: i32>(
            src: __m256d,
            k: __mmask8,
            a: __m256d,
        ) -> __m256d;
        fn _mm256_maskz_roundscale_pd<const IMM8: i32>(k: __mmask8, a: __m256d) -> __m256d;
        fn _mm_roundscale_pd<const IMM8: i32>(a: __m128d) -> __m128d;
        fn _mm_mask_roundscale_pd<const IMM8: i32>(
            src: __m128d,
            k: __mmask8,
            a: __m128d,
        ) -> __m128d;
        fn _mm_maskz_roundscale_pd<const IMM8: i32>(k: __mmask8, a: __m128d) -> __m128d;
        fn _mm256_scalef_ps(a: __m256, b: __m256) -> __m256;
        fn _mm256_mask_scalef_ps(src: __m256, k: __mmask8, a: __m256, b: __m256) -> __m256;
        fn _mm256_maskz_scalef_ps(k: __mmask8, a: __m256, b: __m256) -> __m256;
        fn _mm_scalef_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_mask_scalef_ps(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_scalef_ps(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm256_scalef_pd(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_mask_scalef_pd(src: __m256d, k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_maskz_scalef_pd(k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
        fn _mm_scalef_pd(a: __m128d, b: __m128d) -> __m128d;
        fn _mm_mask_scalef_pd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_scalef_pd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm256_fixupimm_ps<const IMM8: i32>(a: __m256, b: __m256, c: __m256i) -> __m256;
        fn _mm256_mask_fixupimm_ps<const IMM8: i32>(
            a: __m256,
            k: __mmask8,
            b: __m256,
            c: __m256i,
        ) -> __m256;
        fn _mm256_maskz_fixupimm_ps<const IMM8: i32>(
            k: __mmask8,
            a: __m256,
            b: __m256,
            c: __m256i,
        ) -> __m256;
        fn _mm_fixupimm_ps<const IMM8: i32>(a: __m128, b: __m128, c: __m128i) -> __m128;
        fn _mm_mask_fixupimm_ps<const IMM8: i32>(
            a: __m128,
            k: __mmask8,
            b: __m128,
            c: __m128i,
        ) -> __m128;
        fn _mm_maskz_fixupimm_ps<const IMM8: i32>(
            k: __mmask8,
            a: __m128,
            b: __m128,
            c: __m128i,
        ) -> __m128;
        fn _mm256_fixupimm_pd<const IMM8: i32>(a: __m256d, b: __m256d, c: __m256i) -> __m256d;
        fn _mm256_mask_fixupimm_pd<const IMM8: i32>(
            a: __m256d,
            k: __mmask8,
            b: __m256d,
            c: __m256i,
        ) -> __m256d;
        fn _mm256_maskz_fixupimm_pd<const IMM8: i32>(
            k: __mmask8,
            a: __m256d,
            b: __m256d,
            c: __m256i,
        ) -> __m256d;
        fn _mm_fixupimm_pd<const IMM8: i32>(a: __m128d, b: __m128d, c: __m128i) -> __m128d;
        fn _mm_mask_fixupimm_pd<const IMM8: i32>(
            a: __m128d,
            k: __mmask8,
            b: __m128d,
            c: __m128i,
        ) -> __m128d;
        fn _mm_maskz_fixupimm_pd<const IMM8: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
            c: __m128i,
        ) -> __m128d;
        fn _mm256_ternarylogic_epi32<const IMM8: i32>(
            a: __m256i,
            b: __m256i,
            c: __m256i,
        ) -> __m256i;
        fn _mm256_mask_ternarylogic_epi32<const IMM8: i32>(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_ternarylogic_epi32<const IMM8: i32>(
            k: __mmask8,
            a: __m256i,
            b: __m256i,
            c: __m256i,
        ) -> __m256i;
        fn _mm_ternarylogic_epi32<const IMM8: i32>(
            a: __m128i,
            b: __m128i,
            c: __m128i,
        ) -> __m128i;
        fn _mm_mask_ternarylogic_epi32<const IMM8: i32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_maskz_ternarylogic_epi32<const IMM8: i32>(
            k: __mmask8,
            a: __m128i,
            b: __m128i,
            c: __m128i,
        ) -> __m128i;
        fn _mm256_ternarylogic_epi64<const IMM8: i32>(
            a: __m256i,
            b: __m256i,
            c: __m256i,
        ) -> __m256i;
        fn _mm256_mask_ternarylogic_epi64<const IMM8: i32>(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_ternarylogic_epi64<const IMM8: i32>(
            k: __mmask8,
            a: __m256i,
            b: __m256i,
            c: __m256i,
        ) -> __m256i;
        fn _mm_ternarylogic_epi64<const IMM8: i32>(
            a: __m128i,
            b: __m128i,
            c: __m128i,
        ) -> __m128i;
        fn _mm_mask_ternarylogic_epi64<const IMM8: i32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_maskz_ternarylogic_epi64<const IMM8: i32>(
            k: __mmask8,
            a: __m128i,
            b: __m128i,
            c: __m128i,
        ) -> __m128i;
        fn _mm256_getmant_ps<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            a: __m256,
        ) -> __m256;
        fn _mm256_mask_getmant_ps<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            src: __m256,
            k: __mmask8,
            a: __m256,
        ) -> __m256;
        fn _mm256_maskz_getmant_ps<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            k: __mmask8,
            a: __m256,
        ) -> __m256;
        fn _mm_getmant_ps<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            a: __m128,
        ) -> __m128;
        fn _mm_mask_getmant_ps<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            src: __m128,
            k: __mmask8,
            a: __m128,
        ) -> __m128;
        fn _mm_maskz_getmant_ps<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            k: __mmask8,
            a: __m128,
        ) -> __m128;
        fn _mm256_getmant_pd<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            a: __m256d,
        ) -> __m256d;
        fn _mm256_mask_getmant_pd<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            src: __m256d,
            k: __mmask8,
            a: __m256d,
        ) -> __m256d;
        fn _mm256_maskz_getmant_pd<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            k: __mmask8,
            a: __m256d,
        ) -> __m256d;
        fn _mm_getmant_pd<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            a: __m128d,
        ) -> __m128d;
        fn _mm_mask_getmant_pd<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            src: __m128d,
            k: __mmask8,
            a: __m128d,
        ) -> __m128d;
        fn _mm_maskz_getmant_pd<
            const NORM: _MM_MANTISSA_NORM_ENUM,
            const SIGN: _MM_MANTISSA_SIGN_ENUM,
        >(
            k: __mmask8,
            a: __m128d,
        ) -> __m128d;
        fn _mm256_mask_cvtps_epi32(src: __m256i, k: __mmask8, a: __m256) -> __m256i;
        fn _mm256_maskz_cvtps_epi32(k: __mmask8, a: __m256) -> __m256i;
        fn _mm_mask_cvtps_epi32(src: __m128i, k: __mmask8, a: __m128) -> __m128i;
        fn _mm_maskz_cvtps_epi32(k: __mmask8, a: __m128) -> __m128i;
        fn _mm256_cvtps_epu32(a: __m256) -> __m256i;
        fn _mm256_mask_cvtps_epu32(src: __m256i, k: __mmask8, a: __m256) -> __m256i;
        fn _mm256_maskz_cvtps_epu32(k: __mmask8, a: __m256) -> __m256i;
        fn _mm_cvtps_epu32(a: __m128) -> __m128i;
        fn _mm_mask_cvtps_epu32(src: __m128i, k: __mmask8, a: __m128) -> __m128i;
        fn _mm_maskz_cvtps_epu32(k: __mmask8, a: __m128) -> __m128i;
        fn _mm256_mask_cvtpd_ps(src: __m128, k: __mmask8, a: __m256d) -> __m128;
        fn _mm256_maskz_cvtpd_ps(k: __mmask8, a: __m256d) -> __m128;
        fn _mm_mask_cvtpd_ps(src: __m128, k: __mmask8, a: __m128d) -> __m128;
        fn _mm_maskz_cvtpd_ps(k: __mmask8, a: __m128d) -> __m128;
        fn _mm256_mask_cvtpd_epi32(src: __m128i, k: __mmask8, a: __m256d) -> __m128i;
        fn _mm256_maskz_cvtpd_epi32(k: __mmask8, a: __m256d) -> __m128i;
        fn _mm_mask_cvtpd_epi32(src: __m128i, k: __mmask8, a: __m128d) -> __m128i;
        fn _mm_maskz_cvtpd_epi32(k: __mmask8, a: __m128d) -> __m128i;
        fn _mm256_cvtpd_epu32(a: __m256d) -> __m128i;
        fn _mm256_mask_cvtpd_epu32(src: __m128i, k: __mmask8, a: __m256d) -> __m128i;
        fn _mm256_maskz_cvtpd_epu32(k: __mmask8, a: __m256d) -> __m128i;
        fn _mm_cvtpd_epu32(a: __m128d) -> __m128i;
        fn _mm_mask_cvtpd_epu32(src: __m128i, k: __mmask8, a: __m128d) -> __m128i;
        fn _mm_maskz_cvtpd_epu32(k: __mmask8, a: __m128d) -> __m128i;
        fn _mm256_mask_cvtepi8_epi32(src: __m256i, k: __mmask8, a: __m128i) -> __m256i;
        fn _mm256_maskz_cvtepi8_epi32(k: __mmask8, a: __m128i) -> __m256i;
        fn _mm_mask_cvtepi8_epi32(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtepi8_epi32(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_cvtepi8_epi64(src: __m256i, k: __mmask8, a: __m128i) -> __m256i;
        fn _mm256_maskz_cvtepi8_epi64(k: __mmask8, a: __m128i) -> __m256i;
        fn _mm_mask_cvtepi8_epi64(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtepi8_epi64(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_cvtepu8_epi32(src: __m256i, k: __mmask8, a: __m128i) -> __m256i;
        fn _mm256_maskz_cvtepu8_epi32(k: __mmask8, a: __m128i) -> __m256i;
        fn _mm_mask_cvtepu8_epi32(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtepu8_epi32(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_cvtepu8_epi64(src: __m256i, k: __mmask8, a: __m128i) -> __m256i;
        fn _mm256_maskz_cvtepu8_epi64(k: __mmask8, a: __m128i) -> __m256i;
        fn _mm_mask_cvtepu8_epi64(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtepu8_epi64(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_cvtepi16_epi32(src: __m256i, k: __mmask8, a: __m128i) -> __m256i;
        fn _mm256_maskz_cvtepi16_epi32(k: __mmask8, a: __m128i) -> __m256i;
        fn _mm_mask_cvtepi16_epi32(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtepi16_epi32(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_cvtepi16_epi64(src: __m256i, k: __mmask8, a: __m128i) -> __m256i;
        fn _mm256_maskz_cvtepi16_epi64(k: __mmask8, a: __m128i) -> __m256i;
        fn _mm_mask_cvtepi16_epi64(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtepi16_epi64(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_cvtepu16_epi32(src: __m256i, k: __mmask8, a: __m128i) -> __m256i;
        fn _mm256_maskz_cvtepu16_epi32(k: __mmask8, a: __m128i) -> __m256i;
        fn _mm_mask_cvtepu16_epi32(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtepu16_epi32(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_cvtepu16_epi64(src: __m256i, k: __mmask8, a: __m128i) -> __m256i;
        fn _mm256_maskz_cvtepu16_epi64(k: __mmask8, a: __m128i) -> __m256i;
        fn _mm_mask_cvtepu16_epi64(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtepu16_epi64(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_cvtepi32_epi64(src: __m256i, k: __mmask8, a: __m128i) -> __m256i;
        fn _mm256_maskz_cvtepi32_epi64(k: __mmask8, a: __m128i) -> __m256i;
        fn _mm_mask_cvtepi32_epi64(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtepi32_epi64(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_cvtepu32_epi64(src: __m256i, k: __mmask8, a: __m128i) -> __m256i;
        fn _mm256_maskz_cvtepu32_epi64(k: __mmask8, a: __m128i) -> __m256i;
        fn _mm_mask_cvtepu32_epi64(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtepu32_epi64(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_cvtepi32_ps(src: __m256, k: __mmask8, a: __m256i) -> __m256;
        fn _mm256_maskz_cvtepi32_ps(k: __mmask8, a: __m256i) -> __m256;
        fn _mm_mask_cvtepi32_ps(src: __m128, k: __mmask8, a: __m128i) -> __m128;
        fn _mm_maskz_cvtepi32_ps(k: __mmask8, a: __m128i) -> __m128;
        fn _mm256_mask_cvtepi32_pd(src: __m256d, k: __mmask8, a: __m128i) -> __m256d;
        fn _mm256_maskz_cvtepi32_pd(k: __mmask8, a: __m128i) -> __m256d;
        fn _mm_mask_cvtepi32_pd(src: __m128d, k: __mmask8, a: __m128i) -> __m128d;
        fn _mm_maskz_cvtepi32_pd(k: __mmask8, a: __m128i) -> __m128d;
        fn _mm256_cvtepu32_pd(a: __m128i) -> __m256d;
        fn _mm256_mask_cvtepu32_pd(src: __m256d, k: __mmask8, a: __m128i) -> __m256d;
        fn _mm256_maskz_cvtepu32_pd(k: __mmask8, a: __m128i) -> __m256d;
        fn _mm_cvtepu32_pd(a: __m128i) -> __m128d;
        fn _mm_mask_cvtepu32_pd(src: __m128d, k: __mmask8, a: __m128i) -> __m128d;
        fn _mm_maskz_cvtepu32_pd(k: __mmask8, a: __m128i) -> __m128d;
        fn _mm256_cvtepi32_epi16(a: __m256i) -> __m128i;
        fn _mm256_mask_cvtepi32_epi16(src: __m128i, k: __mmask8, a: __m256i) -> __m128i;
        fn _mm256_maskz_cvtepi32_epi16(k: __mmask8, a: __m256i) -> __m128i;
        fn _mm_cvtepi32_epi16(a: __m128i) -> __m128i;
        fn _mm_mask_cvtepi32_epi16(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtepi32_epi16(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_cvtepi32_epi8(a: __m256i) -> __m128i;
        fn _mm256_mask_cvtepi32_epi8(src: __m128i, k: __mmask8, a: __m256i) -> __m128i;
        fn _mm256_maskz_cvtepi32_epi8(k: __mmask8, a: __m256i) -> __m128i;
        fn _mm_cvtepi32_epi8(a: __m128i) -> __m128i;
        fn _mm_mask_cvtepi32_epi8(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtepi32_epi8(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_cvtepi64_epi32(a: __m256i) -> __m128i;
        fn _mm256_mask_cvtepi64_epi32(src: __m128i, k: __mmask8, a: __m256i) -> __m128i;
        fn _mm256_maskz_cvtepi64_epi32(k: __mmask8, a: __m256i) -> __m128i;
        fn _mm_cvtepi64_epi32(a: __m128i) -> __m128i;
        fn _mm_mask_cvtepi64_epi32(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtepi64_epi32(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_cvtepi64_epi16(a: __m256i) -> __m128i;
        fn _mm256_mask_cvtepi64_epi16(src: __m128i, k: __mmask8, a: __m256i) -> __m128i;
        fn _mm256_maskz_cvtepi64_epi16(k: __mmask8, a: __m256i) -> __m128i;
        fn _mm_cvtepi64_epi16(a: __m128i) -> __m128i;
        fn _mm_mask_cvtepi64_epi16(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtepi64_epi16(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_cvtepi64_epi8(a: __m256i) -> __m128i;
        fn _mm256_mask_cvtepi64_epi8(src: __m128i, k: __mmask8, a: __m256i) -> __m128i;
        fn _mm256_maskz_cvtepi64_epi8(k: __mmask8, a: __m256i) -> __m128i;
        fn _mm_cvtepi64_epi8(a: __m128i) -> __m128i;
        fn _mm_mask_cvtepi64_epi8(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtepi64_epi8(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_cvtsepi32_epi16(a: __m256i) -> __m128i;
        fn _mm256_mask_cvtsepi32_epi16(src: __m128i, k: __mmask8, a: __m256i) -> __m128i;
        fn _mm256_maskz_cvtsepi32_epi16(k: __mmask8, a: __m256i) -> __m128i;
        fn _mm_cvtsepi32_epi16(a: __m128i) -> __m128i;
        fn _mm_mask_cvtsepi32_epi16(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtsepi32_epi16(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_cvtsepi32_epi8(a: __m256i) -> __m128i;
        fn _mm256_mask_cvtsepi32_epi8(src: __m128i, k: __mmask8, a: __m256i) -> __m128i;
        fn _mm256_maskz_cvtsepi32_epi8(k: __mmask8, a: __m256i) -> __m128i;
        fn _mm_cvtsepi32_epi8(a: __m128i) -> __m128i;
        fn _mm_mask_cvtsepi32_epi8(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtsepi32_epi8(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_cvtsepi64_epi32(a: __m256i) -> __m128i;
        fn _mm256_mask_cvtsepi64_epi32(src: __m128i, k: __mmask8, a: __m256i) -> __m128i;
        fn _mm256_maskz_cvtsepi64_epi32(k: __mmask8, a: __m256i) -> __m128i;
        fn _mm_cvtsepi64_epi32(a: __m128i) -> __m128i;
        fn _mm_mask_cvtsepi64_epi32(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtsepi64_epi32(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_cvtsepi64_epi16(a: __m256i) -> __m128i;
        fn _mm256_mask_cvtsepi64_epi16(src: __m128i, k: __mmask8, a: __m256i) -> __m128i;
        fn _mm256_maskz_cvtsepi64_epi16(k: __mmask8, a: __m256i) -> __m128i;
        fn _mm_cvtsepi64_epi16(a: __m128i) -> __m128i;
        fn _mm_mask_cvtsepi64_epi16(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtsepi64_epi16(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_cvtsepi64_epi8(a: __m256i) -> __m128i;
        fn _mm256_mask_cvtsepi64_epi8(src: __m128i, k: __mmask8, a: __m256i) -> __m128i;
        fn _mm256_maskz_cvtsepi64_epi8(k: __mmask8, a: __m256i) -> __m128i;
        fn _mm_cvtsepi64_epi8(a: __m128i) -> __m128i;
        fn _mm_mask_cvtsepi64_epi8(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtsepi64_epi8(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_cvtusepi32_epi16(a: __m256i) -> __m128i;
        fn _mm256_mask_cvtusepi32_epi16(src: __m128i, k: __mmask8, a: __m256i) -> __m128i;
        fn _mm256_maskz_cvtusepi32_epi16(k: __mmask8, a: __m256i) -> __m128i;
        fn _mm_cvtusepi32_epi16(a: __m128i) -> __m128i;
        fn _mm_mask_cvtusepi32_epi16(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtusepi32_epi16(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_cvtusepi32_epi8(a: __m256i) -> __m128i;
        fn _mm256_mask_cvtusepi32_epi8(src: __m128i, k: __mmask8, a: __m256i) -> __m128i;
        fn _mm256_maskz_cvtusepi32_epi8(k: __mmask8, a: __m256i) -> __m128i;
        fn _mm_cvtusepi32_epi8(a: __m128i) -> __m128i;
        fn _mm_mask_cvtusepi32_epi8(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtusepi32_epi8(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_cvtusepi64_epi32(a: __m256i) -> __m128i;
        fn _mm256_mask_cvtusepi64_epi32(src: __m128i, k: __mmask8, a: __m256i) -> __m128i;
        fn _mm256_maskz_cvtusepi64_epi32(k: __mmask8, a: __m256i) -> __m128i;
        fn _mm_cvtusepi64_epi32(a: __m128i) -> __m128i;
        fn _mm_mask_cvtusepi64_epi32(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtusepi64_epi32(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_cvtusepi64_epi16(a: __m256i) -> __m128i;
        fn _mm256_mask_cvtusepi64_epi16(src: __m128i, k: __mmask8, a: __m256i) -> __m128i;
        fn _mm256_maskz_cvtusepi64_epi16(k: __mmask8, a: __m256i) -> __m128i;
        fn _mm_cvtusepi64_epi16(a: __m128i) -> __m128i;
        fn _mm_mask_cvtusepi64_epi16(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtusepi64_epi16(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_cvtusepi64_epi8(a: __m256i) -> __m128i;
        fn _mm256_mask_cvtusepi64_epi8(src: __m128i, k: __mmask8, a: __m256i) -> __m128i;
        fn _mm256_maskz_cvtusepi64_epi8(k: __mmask8, a: __m256i) -> __m128i;
        fn _mm_cvtusepi64_epi8(a: __m128i) -> __m128i;
        fn _mm_mask_cvtusepi64_epi8(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtusepi64_epi8(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_cvt_roundps_ph<const IMM8: i32>(
            src: __m128i,
            k: __mmask8,
            a: __m256,
        ) -> __m128i;
        fn _mm256_maskz_cvt_roundps_ph<const IMM8: i32>(k: __mmask8, a: __m256) -> __m128i;
        fn _mm_mask_cvt_roundps_ph<const IMM8: i32>(
            src: __m128i,
            k: __mmask8,
            a: __m128,
        ) -> __m128i;
        fn _mm_maskz_cvt_roundps_ph<const IMM8: i32>(k: __mmask8, a: __m128) -> __m128i;
        fn _mm256_mask_cvtps_ph<const IMM8: i32>(
            src: __m128i,
            k: __mmask8,
            a: __m256,
        ) -> __m128i;
        fn _mm256_maskz_cvtps_ph<const IMM8: i32>(k: __mmask8, a: __m256) -> __m128i;
        fn _mm_mask_cvtps_ph<const IMM8: i32>(src: __m128i, k: __mmask8, a: __m128) -> __m128i;
        fn _mm_maskz_cvtps_ph<const IMM8: i32>(k: __mmask8, a: __m128) -> __m128i;
        fn _mm256_mask_cvtph_ps(src: __m256, k: __mmask8, a: __m128i) -> __m256;
        fn _mm256_maskz_cvtph_ps(k: __mmask8, a: __m128i) -> __m256;
        fn _mm_mask_cvtph_ps(src: __m128, k: __mmask8, a: __m128i) -> __m128;
        fn _mm_maskz_cvtph_ps(k: __mmask8, a: __m128i) -> __m128;
        fn _mm256_mask_cvttps_epi32(src: __m256i, k: __mmask8, a: __m256) -> __m256i;
        fn _mm256_maskz_cvttps_epi32(k: __mmask8, a: __m256) -> __m256i;
        fn _mm_mask_cvttps_epi32(src: __m128i, k: __mmask8, a: __m128) -> __m128i;
        fn _mm_maskz_cvttps_epi32(k: __mmask8, a: __m128) -> __m128i;
        fn _mm256_cvttps_epu32(a: __m256) -> __m256i;
        fn _mm256_mask_cvttps_epu32(src: __m256i, k: __mmask8, a: __m256) -> __m256i;
        fn _mm256_maskz_cvttps_epu32(k: __mmask8, a: __m256) -> __m256i;
        fn _mm_cvttps_epu32(a: __m128) -> __m128i;
        fn _mm_mask_cvttps_epu32(src: __m128i, k: __mmask8, a: __m128) -> __m128i;
        fn _mm_maskz_cvttps_epu32(k: __mmask8, a: __m128) -> __m128i;
        fn _mm256_mask_cvttpd_epi32(src: __m128i, k: __mmask8, a: __m256d) -> __m128i;
        fn _mm256_maskz_cvttpd_epi32(k: __mmask8, a: __m256d) -> __m128i;
        fn _mm_mask_cvttpd_epi32(src: __m128i, k: __mmask8, a: __m128d) -> __m128i;
        fn _mm_maskz_cvttpd_epi32(k: __mmask8, a: __m128d) -> __m128i;
        fn _mm256_cvttpd_epu32(a: __m256d) -> __m128i;
        fn _mm256_mask_cvttpd_epu32(src: __m128i, k: __mmask8, a: __m256d) -> __m128i;
        fn _mm256_maskz_cvttpd_epu32(k: __mmask8, a: __m256d) -> __m128i;
        fn _mm_cvttpd_epu32(a: __m128d) -> __m128i;
        fn _mm_mask_cvttpd_epu32(src: __m128i, k: __mmask8, a: __m128d) -> __m128i;
        fn _mm_maskz_cvttpd_epu32(k: __mmask8, a: __m128d) -> __m128i;
        fn _mm256_mask_compress_epi32(src: __m256i, k: __mmask8, a: __m256i) -> __m256i;
        fn _mm256_maskz_compress_epi32(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_mask_compress_epi32(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_compress_epi32(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_compress_epi64(src: __m256i, k: __mmask8, a: __m256i) -> __m256i;
        fn _mm256_maskz_compress_epi64(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_mask_compress_epi64(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_compress_epi64(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_compress_ps(src: __m256, k: __mmask8, a: __m256) -> __m256;
        fn _mm256_maskz_compress_ps(k: __mmask8, a: __m256) -> __m256;
        fn _mm_mask_compress_ps(src: __m128, k: __mmask8, a: __m128) -> __m128;
        fn _mm_maskz_compress_ps(k: __mmask8, a: __m128) -> __m128;
        fn _mm256_mask_compress_pd(src: __m256d, k: __mmask8, a: __m256d) -> __m256d;
        fn _mm256_maskz_compress_pd(k: __mmask8, a: __m256d) -> __m256d;
        fn _mm_mask_compress_pd(src: __m128d, k: __mmask8, a: __m128d) -> __m128d;
        fn _mm_maskz_compress_pd(k: __mmask8, a: __m128d) -> __m128d;
        unsafe fn _mm256_mask_compressstoreu_epi32(base_addr: *mut u8, k: __mmask8, a: __m256i);
        unsafe fn _mm_mask_compressstoreu_epi32(base_addr: *mut u8, k: __mmask8, a: __m128i);
        unsafe fn _mm256_mask_compressstoreu_epi64(base_addr: *mut u8, k: __mmask8, a: __m256i);
        unsafe fn _mm_mask_compressstoreu_epi64(base_addr: *mut u8, k: __mmask8, a: __m128i);
        unsafe fn _mm256_mask_compressstoreu_ps(base_addr: *mut u8, k: __mmask8, a: __m256);
        unsafe fn _mm_mask_compressstoreu_ps(base_addr: *mut u8, k: __mmask8, a: __m128);
        unsafe fn _mm256_mask_compressstoreu_pd(base_addr: *mut u8, k: __mmask8, a: __m256d);
        unsafe fn _mm_mask_compressstoreu_pd(base_addr: *mut u8, k: __mmask8, a: __m128d);
        fn _mm256_mask_expand_epi32(src: __m256i, k: __mmask8, a: __m256i) -> __m256i;
        fn _mm256_maskz_expand_epi32(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_mask_expand_epi32(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_expand_epi32(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_expand_epi64(src: __m256i, k: __mmask8, a: __m256i) -> __m256i;
        fn _mm256_maskz_expand_epi64(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_mask_expand_epi64(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_expand_epi64(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_expand_ps(src: __m256, k: __mmask8, a: __m256) -> __m256;
        fn _mm256_maskz_expand_ps(k: __mmask8, a: __m256) -> __m256;
        fn _mm_mask_expand_ps(src: __m128, k: __mmask8, a: __m128) -> __m128;
        fn _mm_maskz_expand_ps(k: __mmask8, a: __m128) -> __m128;
        fn _mm256_mask_expand_pd(src: __m256d, k: __mmask8, a: __m256d) -> __m256d;
        fn _mm256_maskz_expand_pd(k: __mmask8, a: __m256d) -> __m256d;
        fn _mm_mask_expand_pd(src: __m128d, k: __mmask8, a: __m128d) -> __m128d;
        fn _mm_maskz_expand_pd(k: __mmask8, a: __m128d) -> __m128d;
        fn _mm256_rol_epi32<const IMM8: i32>(a: __m256i) -> __m256i;
        fn _mm256_mask_rol_epi32<const IMM8: i32>(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_rol_epi32<const IMM8: i32>(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_rol_epi32<const IMM8: i32>(a: __m128i) -> __m128i;
        fn _mm_mask_rol_epi32<const IMM8: i32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
        ) -> __m128i;
        fn _mm_maskz_rol_epi32<const IMM8: i32>(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_ror_epi32<const IMM8: i32>(a: __m256i) -> __m256i;
        fn _mm256_mask_ror_epi32<const IMM8: i32>(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_ror_epi32<const IMM8: i32>(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_ror_epi32<const IMM8: i32>(a: __m128i) -> __m128i;
        fn _mm_mask_ror_epi32<const IMM8: i32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
        ) -> __m128i;
        fn _mm_maskz_ror_epi32<const IMM8: i32>(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_rol_epi64<const IMM8: i32>(a: __m256i) -> __m256i;
        fn _mm256_mask_rol_epi64<const IMM8: i32>(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_rol_epi64<const IMM8: i32>(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_rol_epi64<const IMM8: i32>(a: __m128i) -> __m128i;
        fn _mm_mask_rol_epi64<const IMM8: i32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
        ) -> __m128i;
        fn _mm_maskz_rol_epi64<const IMM8: i32>(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_ror_epi64<const IMM8: i32>(a: __m256i) -> __m256i;
        fn _mm256_mask_ror_epi64<const IMM8: i32>(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_ror_epi64<const IMM8: i32>(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_ror_epi64<const IMM8: i32>(a: __m128i) -> __m128i;
        fn _mm_mask_ror_epi64<const IMM8: i32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
        ) -> __m128i;
        fn _mm_maskz_ror_epi64<const IMM8: i32>(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_slli_epi32<const IMM8: u32>(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_slli_epi32<const IMM8: u32>(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_mask_slli_epi32<const IMM8: u32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
        ) -> __m128i;
        fn _mm_maskz_slli_epi32<const IMM8: u32>(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_srli_epi32<const IMM8: u32>(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_srli_epi32<const IMM8: u32>(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_mask_srli_epi32<const IMM8: u32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
        ) -> __m128i;
        fn _mm_maskz_srli_epi32<const IMM8: u32>(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_slli_epi64<const IMM8: u32>(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_slli_epi64<const IMM8: u32>(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_mask_slli_epi64<const IMM8: u32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
        ) -> __m128i;
        fn _mm_maskz_slli_epi64<const IMM8: u32>(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_srli_epi64<const IMM8: u32>(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_srli_epi64<const IMM8: u32>(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_mask_srli_epi64<const IMM8: u32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
        ) -> __m128i;
        fn _mm_maskz_srli_epi64<const IMM8: u32>(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_sll_epi32(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            count: __m128i,
        ) -> __m256i;
        fn _mm256_maskz_sll_epi32(k: __mmask8, a: __m256i, count: __m128i) -> __m256i;
        fn _mm_mask_sll_epi32(src: __m128i, k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm_maskz_sll_epi32(k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_mask_srl_epi32(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            count: __m128i,
        ) -> __m256i;
        fn _mm256_maskz_srl_epi32(k: __mmask8, a: __m256i, count: __m128i) -> __m256i;
        fn _mm_mask_srl_epi32(src: __m128i, k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm_maskz_srl_epi32(k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_mask_sll_epi64(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            count: __m128i,
        ) -> __m256i;
        fn _mm256_maskz_sll_epi64(k: __mmask8, a: __m256i, count: __m128i) -> __m256i;
        fn _mm_mask_sll_epi64(src: __m128i, k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm_maskz_sll_epi64(k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_mask_srl_epi64(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            count: __m128i,
        ) -> __m256i;
        fn _mm256_maskz_srl_epi64(k: __mmask8, a: __m256i, count: __m128i) -> __m256i;
        fn _mm_mask_srl_epi64(src: __m128i, k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm_maskz_srl_epi64(k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_mask_sra_epi32(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            count: __m128i,
        ) -> __m256i;
        fn _mm256_maskz_sra_epi32(k: __mmask8, a: __m256i, count: __m128i) -> __m256i;
        fn _mm_mask_sra_epi32(src: __m128i, k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm_maskz_sra_epi32(k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_sra_epi64(a: __m256i, count: __m128i) -> __m256i;
        fn _mm256_mask_sra_epi64(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            count: __m128i,
        ) -> __m256i;
        fn _mm256_maskz_sra_epi64(k: __mmask8, a: __m256i, count: __m128i) -> __m256i;
        fn _mm_sra_epi64(a: __m128i, count: __m128i) -> __m128i;
        fn _mm_mask_sra_epi64(src: __m128i, k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm_maskz_sra_epi64(k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_mask_srai_epi32<const IMM8: u32>(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_srai_epi32<const IMM8: u32>(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_mask_srai_epi32<const IMM8: u32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
        ) -> __m128i;
        fn _mm_maskz_srai_epi32<const IMM8: u32>(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_srai_epi64<const IMM8: u32>(a: __m256i) -> __m256i;
        fn _mm256_mask_srai_epi64<const IMM8: u32>(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_srai_epi64<const IMM8: u32>(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_srai_epi64<const IMM8: u32>(a: __m128i) -> __m128i;
        fn _mm_mask_srai_epi64<const IMM8: u32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
        ) -> __m128i;
        fn _mm_maskz_srai_epi64<const IMM8: u32>(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_srav_epi32(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            count: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_srav_epi32(k: __mmask8, a: __m256i, count: __m256i) -> __m256i;
        fn _mm_mask_srav_epi32(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            count: __m128i,
        ) -> __m128i;
        fn _mm_maskz_srav_epi32(k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_srav_epi64(a: __m256i, count: __m256i) -> __m256i;
        fn _mm256_mask_srav_epi64(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            count: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_srav_epi64(k: __mmask8, a: __m256i, count: __m256i) -> __m256i;
        fn _mm_srav_epi64(a: __m128i, count: __m128i) -> __m128i;
        fn _mm_mask_srav_epi64(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            count: __m128i,
        ) -> __m128i;
        fn _mm_maskz_srav_epi64(k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_rolv_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_rolv_epi32(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_rolv_epi32(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_rolv_epi32(a: __m128i, b: __m128i) -> __m128i;
        fn _mm_mask_rolv_epi32(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_rolv_epi32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_rorv_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_rorv_epi32(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_rorv_epi32(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_rorv_epi32(a: __m128i, b: __m128i) -> __m128i;
        fn _mm_mask_rorv_epi32(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_rorv_epi32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_rolv_epi64(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_rolv_epi64(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_rolv_epi64(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_rolv_epi64(a: __m128i, b: __m128i) -> __m128i;
        fn _mm_mask_rolv_epi64(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_rolv_epi64(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_rorv_epi64(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_rorv_epi64(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_rorv_epi64(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_rorv_epi64(a: __m128i, b: __m128i) -> __m128i;
        fn _mm_mask_rorv_epi64(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_rorv_epi64(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_sllv_epi32(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            count: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_sllv_epi32(k: __mmask8, a: __m256i, count: __m256i) -> __m256i;
        fn _mm_mask_sllv_epi32(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            count: __m128i,
        ) -> __m128i;
        fn _mm_maskz_sllv_epi32(k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_mask_srlv_epi32(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            count: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_srlv_epi32(k: __mmask8, a: __m256i, count: __m256i) -> __m256i;
        fn _mm_mask_srlv_epi32(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            count: __m128i,
        ) -> __m128i;
        fn _mm_maskz_srlv_epi32(k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_mask_sllv_epi64(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            count: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_sllv_epi64(k: __mmask8, a: __m256i, count: __m256i) -> __m256i;
        fn _mm_mask_sllv_epi64(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            count: __m128i,
        ) -> __m128i;
        fn _mm_maskz_sllv_epi64(k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_mask_srlv_epi64(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            count: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_srlv_epi64(k: __mmask8, a: __m256i, count: __m256i) -> __m256i;
        fn _mm_mask_srlv_epi64(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            count: __m128i,
        ) -> __m128i;
        fn _mm_maskz_srlv_epi64(k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_mask_permute_ps<const MASK: i32>(
            src: __m256,
            k: __mmask8,
            a: __m256,
        ) -> __m256;
        fn _mm256_maskz_permute_ps<const MASK: i32>(k: __mmask8, a: __m256) -> __m256;
        fn _mm_mask_permute_ps<const MASK: i32>(src: __m128, k: __mmask8, a: __m128) -> __m128;
        fn _mm_maskz_permute_ps<const MASK: i32>(k: __mmask8, a: __m128) -> __m128;
        fn _mm256_mask_permute_pd<const MASK: i32>(
            src: __m256d,
            k: __mmask8,
            a: __m256d,
        ) -> __m256d;
        fn _mm256_maskz_permute_pd<const MASK: i32>(k: __mmask8, a: __m256d) -> __m256d;
        fn _mm_mask_permute_pd<const IMM2: i32>(
            src: __m128d,
            k: __mmask8,
            a: __m128d,
        ) -> __m128d;
        fn _mm_maskz_permute_pd<const IMM2: i32>(k: __mmask8, a: __m128d) -> __m128d;
        fn _mm256_permutex_epi64<const MASK: i32>(a: __m256i) -> __m256i;
        fn _mm256_mask_permutex_epi64<const MASK: i32>(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_permutex_epi64<const MASK: i32>(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm256_permutex_pd<const MASK: i32>(a: __m256d) -> __m256d;
        fn _mm256_mask_permutex_pd<const MASK: i32>(
            src: __m256d,
            k: __mmask8,
            a: __m256d,
        ) -> __m256d;
        fn _mm256_maskz_permutex_pd<const MASK: i32>(k: __mmask8, a: __m256d) -> __m256d;
        fn _mm256_mask_permutevar_ps(src: __m256, k: __mmask8, a: __m256, b: __m256i) -> __m256;
        fn _mm256_maskz_permutevar_ps(k: __mmask8, a: __m256, b: __m256i) -> __m256;
        fn _mm_mask_permutevar_ps(src: __m128, k: __mmask8, a: __m128, b: __m128i) -> __m128;
        fn _mm_maskz_permutevar_ps(k: __mmask8, a: __m128, b: __m128i) -> __m128;
        fn _mm256_mask_permutevar_pd(
            src: __m256d,
            k: __mmask8,
            a: __m256d,
            b: __m256i,
        ) -> __m256d;
        fn _mm256_maskz_permutevar_pd(k: __mmask8, a: __m256d, b: __m256i) -> __m256d;
        fn _mm_mask_permutevar_pd(src: __m128d, k: __mmask8, a: __m128d, b: __m128i) -> __m128d;
        fn _mm_maskz_permutevar_pd(k: __mmask8, a: __m128d, b: __m128i) -> __m128d;
        fn _mm256_permutexvar_epi32(idx: __m256i, a: __m256i) -> __m256i;
        fn _mm256_mask_permutexvar_epi32(
            src: __m256i,
            k: __mmask8,
            idx: __m256i,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_permutexvar_epi32(k: __mmask8, idx: __m256i, a: __m256i) -> __m256i;
        fn _mm256_permutexvar_epi64(idx: __m256i, a: __m256i) -> __m256i;
        fn _mm256_mask_permutexvar_epi64(
            src: __m256i,
            k: __mmask8,
            idx: __m256i,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_permutexvar_epi64(k: __mmask8, idx: __m256i, a: __m256i) -> __m256i;
        fn _mm256_permutexvar_ps(idx: __m256i, a: __m256) -> __m256;
        fn _mm256_mask_permutexvar_ps(
            src: __m256,
            k: __mmask8,
            idx: __m256i,
            a: __m256,
        ) -> __m256;
        fn _mm256_maskz_permutexvar_ps(k: __mmask8, idx: __m256i, a: __m256) -> __m256;
        fn _mm256_permutexvar_pd(idx: __m256i, a: __m256d) -> __m256d;
        fn _mm256_mask_permutexvar_pd(
            src: __m256d,
            k: __mmask8,
            idx: __m256i,
            a: __m256d,
        ) -> __m256d;
        fn _mm256_maskz_permutexvar_pd(k: __mmask8, idx: __m256i, a: __m256d) -> __m256d;
        fn _mm256_permutex2var_epi32(a: __m256i, idx: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_permutex2var_epi32(
            a: __m256i,
            k: __mmask8,
            idx: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_permutex2var_epi32(
            k: __mmask8,
            a: __m256i,
            idx: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_mask2_permutex2var_epi32(
            a: __m256i,
            idx: __m256i,
            k: __mmask8,
            b: __m256i,
        ) -> __m256i;
        fn _mm_permutex2var_epi32(a: __m128i, idx: __m128i, b: __m128i) -> __m128i;
        fn _mm_mask_permutex2var_epi32(
            a: __m128i,
            k: __mmask8,
            idx: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_maskz_permutex2var_epi32(
            k: __mmask8,
            a: __m128i,
            idx: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_mask2_permutex2var_epi32(
            a: __m128i,
            idx: __m128i,
            k: __mmask8,
            b: __m128i,
        ) -> __m128i;
        fn _mm256_permutex2var_epi64(a: __m256i, idx: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_permutex2var_epi64(
            a: __m256i,
            k: __mmask8,
            idx: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_permutex2var_epi64(
            k: __mmask8,
            a: __m256i,
            idx: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_mask2_permutex2var_epi64(
            a: __m256i,
            idx: __m256i,
            k: __mmask8,
            b: __m256i,
        ) -> __m256i;
        fn _mm_permutex2var_epi64(a: __m128i, idx: __m128i, b: __m128i) -> __m128i;
        fn _mm_mask_permutex2var_epi64(
            a: __m128i,
            k: __mmask8,
            idx: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_maskz_permutex2var_epi64(
            k: __mmask8,
            a: __m128i,
            idx: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_mask2_permutex2var_epi64(
            a: __m128i,
            idx: __m128i,
            k: __mmask8,
            b: __m128i,
        ) -> __m128i;
        fn _mm256_permutex2var_ps(a: __m256, idx: __m256i, b: __m256) -> __m256;
        fn _mm256_mask_permutex2var_ps(
            a: __m256,
            k: __mmask8,
            idx: __m256i,
            b: __m256,
        ) -> __m256;
        fn _mm256_maskz_permutex2var_ps(
            k: __mmask8,
            a: __m256,
            idx: __m256i,
            b: __m256,
        ) -> __m256;
        fn _mm256_mask2_permutex2var_ps(
            a: __m256,
            idx: __m256i,
            k: __mmask8,
            b: __m256,
        ) -> __m256;
        fn _mm_permutex2var_ps(a: __m128, idx: __m128i, b: __m128) -> __m128;
        fn _mm_mask_permutex2var_ps(a: __m128, k: __mmask8, idx: __m128i, b: __m128) -> __m128;
        fn _mm_maskz_permutex2var_ps(k: __mmask8, a: __m128, idx: __m128i, b: __m128) -> __m128;
        fn _mm_mask2_permutex2var_ps(a: __m128, idx: __m128i, k: __mmask8, b: __m128) -> __m128;
        fn _mm256_permutex2var_pd(a: __m256d, idx: __m256i, b: __m256d) -> __m256d;
        fn _mm256_mask_permutex2var_pd(
            a: __m256d,
            k: __mmask8,
            idx: __m256i,
            b: __m256d,
        ) -> __m256d;
        fn _mm256_maskz_permutex2var_pd(
            k: __mmask8,
            a: __m256d,
            idx: __m256i,
            b: __m256d,
        ) -> __m256d;
        fn _mm256_mask2_permutex2var_pd(
            a: __m256d,
            idx: __m256i,
            k: __mmask8,
            b: __m256d,
        ) -> __m256d;
        fn _mm_permutex2var_pd(a: __m128d, idx: __m128i, b: __m128d) -> __m128d;
        fn _mm_mask_permutex2var_pd(
            a: __m128d,
            k: __mmask8,
            idx: __m128i,
            b: __m128d,
        ) -> __m128d;
        fn _mm_maskz_permutex2var_pd(
            k: __mmask8,
            a: __m128d,
            idx: __m128i,
            b: __m128d,
        ) -> __m128d;
        fn _mm_mask2_permutex2var_pd(
            a: __m128d,
            idx: __m128i,
            k: __mmask8,
            b: __m128d,
        ) -> __m128d;
        fn _mm256_mask_shuffle_epi32<const MASK: _MM_PERM_ENUM>(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_shuffle_epi32<const MASK: _MM_PERM_ENUM>(
            k: __mmask8,
            a: __m256i,
        ) -> __m256i;
        fn _mm_mask_shuffle_epi32<const MASK: _MM_PERM_ENUM>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
        ) -> __m128i;
        fn _mm_maskz_shuffle_epi32<const MASK: _MM_PERM_ENUM>(
            k: __mmask8,
            a: __m128i,
        ) -> __m128i;
        fn _mm256_mask_shuffle_ps<const MASK: i32>(
            src: __m256,
            k: __mmask8,
            a: __m256,
            b: __m256,
        ) -> __m256;
        fn _mm256_maskz_shuffle_ps<const MASK: i32>(
            k: __mmask8,
            a: __m256,
            b: __m256,
        ) -> __m256;
        fn _mm_mask_shuffle_ps<const MASK: i32>(
            src: __m128,
            k: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __m128;
        fn _mm_maskz_shuffle_ps<const MASK: i32>(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm256_mask_shuffle_pd<const MASK: i32>(
            src: __m256d,
            k: __mmask8,
            a: __m256d,
            b: __m256d,
        ) -> __m256d;
        fn _mm256_maskz_shuffle_pd<const MASK: i32>(
            k: __mmask8,
            a: __m256d,
            b: __m256d,
        ) -> __m256d;
        fn _mm_mask_shuffle_pd<const MASK: i32>(
            src: __m128d,
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm_maskz_shuffle_pd<const MASK: i32>(
            k: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __m128d;
        fn _mm256_shuffle_i32x4<const MASK: i32>(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_shuffle_i32x4<const MASK: i32>(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_shuffle_i32x4<const MASK: i32>(
            k: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_shuffle_i64x2<const MASK: i32>(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_shuffle_i64x2<const MASK: i32>(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_shuffle_i64x2<const MASK: i32>(
            k: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_shuffle_f32x4<const MASK: i32>(a: __m256, b: __m256) -> __m256;
        fn _mm256_mask_shuffle_f32x4<const MASK: i32>(
            src: __m256,
            k: __mmask8,
            a: __m256,
            b: __m256,
        ) -> __m256;
        fn _mm256_maskz_shuffle_f32x4<const MASK: i32>(
            k: __mmask8,
            a: __m256,
            b: __m256,
        ) -> __m256;
        fn _mm256_shuffle_f64x2<const MASK: i32>(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_mask_shuffle_f64x2<const MASK: i32>(
            src: __m256d,
            k: __mmask8,
            a: __m256d,
            b: __m256d,
        ) -> __m256d;
        fn _mm256_maskz_shuffle_f64x2<const MASK: i32>(
            k: __mmask8,
            a: __m256d,
            b: __m256d,
        ) -> __m256d;
        fn _mm256_mask_moveldup_ps(src: __m256, k: __mmask8, a: __m256) -> __m256;
        fn _mm256_maskz_moveldup_ps(k: __mmask8, a: __m256) -> __m256;
        fn _mm_mask_moveldup_ps(src: __m128, k: __mmask8, a: __m128) -> __m128;
        fn _mm_maskz_moveldup_ps(k: __mmask8, a: __m128) -> __m128;
        fn _mm256_mask_movehdup_ps(src: __m256, k: __mmask8, a: __m256) -> __m256;
        fn _mm256_maskz_movehdup_ps(k: __mmask8, a: __m256) -> __m256;
        fn _mm_mask_movehdup_ps(src: __m128, k: __mmask8, a: __m128) -> __m128;
        fn _mm_maskz_movehdup_ps(k: __mmask8, a: __m128) -> __m128;
        fn _mm256_mask_movedup_pd(src: __m256d, k: __mmask8, a: __m256d) -> __m256d;
        fn _mm256_maskz_movedup_pd(k: __mmask8, a: __m256d) -> __m256d;
        fn _mm_mask_movedup_pd(src: __m128d, k: __mmask8, a: __m128d) -> __m128d;
        fn _mm_maskz_movedup_pd(k: __mmask8, a: __m128d) -> __m128d;
        fn _mm256_mask_unpackhi_epi32(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_unpackhi_epi32(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_unpackhi_epi32(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_maskz_unpackhi_epi32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_unpackhi_epi64(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_unpackhi_epi64(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_unpackhi_epi64(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_maskz_unpackhi_epi64(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_unpackhi_ps(src: __m256, k: __mmask8, a: __m256, b: __m256) -> __m256;
        fn _mm256_maskz_unpackhi_ps(k: __mmask8, a: __m256, b: __m256) -> __m256;
        fn _mm_mask_unpackhi_ps(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_unpackhi_ps(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm256_mask_unpackhi_pd(
            src: __m256d,
            k: __mmask8,
            a: __m256d,
            b: __m256d,
        ) -> __m256d;
        fn _mm256_maskz_unpackhi_pd(k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
        fn _mm_mask_unpackhi_pd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_unpackhi_pd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm256_mask_unpacklo_epi32(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_unpacklo_epi32(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_unpacklo_epi32(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_maskz_unpacklo_epi32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_unpacklo_epi64(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_unpacklo_epi64(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_unpacklo_epi64(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_maskz_unpacklo_epi64(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_unpacklo_ps(src: __m256, k: __mmask8, a: __m256, b: __m256) -> __m256;
        fn _mm256_maskz_unpacklo_ps(k: __mmask8, a: __m256, b: __m256) -> __m256;
        fn _mm_mask_unpacklo_ps(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm_maskz_unpacklo_ps(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm256_mask_unpacklo_pd(
            src: __m256d,
            k: __mmask8,
            a: __m256d,
            b: __m256d,
        ) -> __m256d;
        fn _mm256_maskz_unpacklo_pd(k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
        fn _mm_mask_unpacklo_pd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm_maskz_unpacklo_pd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm256_mask_broadcastd_epi32(src: __m256i, k: __mmask8, a: __m128i) -> __m256i;
        fn _mm256_maskz_broadcastd_epi32(k: __mmask8, a: __m128i) -> __m256i;
        fn _mm_mask_broadcastd_epi32(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_broadcastd_epi32(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_broadcastq_epi64(src: __m256i, k: __mmask8, a: __m128i) -> __m256i;
        fn _mm256_maskz_broadcastq_epi64(k: __mmask8, a: __m128i) -> __m256i;
        fn _mm_mask_broadcastq_epi64(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_broadcastq_epi64(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_broadcastss_ps(src: __m256, k: __mmask8, a: __m128) -> __m256;
        fn _mm256_maskz_broadcastss_ps(k: __mmask8, a: __m128) -> __m256;
        fn _mm_mask_broadcastss_ps(src: __m128, k: __mmask8, a: __m128) -> __m128;
        fn _mm_maskz_broadcastss_ps(k: __mmask8, a: __m128) -> __m128;
        fn _mm256_mask_broadcastsd_pd(src: __m256d, k: __mmask8, a: __m128d) -> __m256d;
        fn _mm256_maskz_broadcastsd_pd(k: __mmask8, a: __m128d) -> __m256d;
        fn _mm256_broadcast_i32x4(a: __m128i) -> __m256i;
        fn _mm256_mask_broadcast_i32x4(src: __m256i, k: __mmask8, a: __m128i) -> __m256i;
        fn _mm256_maskz_broadcast_i32x4(k: __mmask8, a: __m128i) -> __m256i;
        fn _mm256_broadcast_f32x4(a: __m128) -> __m256;
        fn _mm256_mask_broadcast_f32x4(src: __m256, k: __mmask8, a: __m128) -> __m256;
        fn _mm256_maskz_broadcast_f32x4(k: __mmask8, a: __m128) -> __m256;
        fn _mm256_mask_blend_epi32(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_blend_epi32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_blend_epi64(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_blend_epi64(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_blend_ps(k: __mmask8, a: __m256, b: __m256) -> __m256;
        fn _mm_mask_blend_ps(k: __mmask8, a: __m128, b: __m128) -> __m128;
        fn _mm256_mask_blend_pd(k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
        fn _mm_mask_blend_pd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
        fn _mm256_alignr_epi32<const IMM8: i32>(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_alignr_epi32<const IMM8: i32>(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_alignr_epi32<const IMM8: i32>(
            k: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm_alignr_epi32<const IMM8: i32>(a: __m128i, b: __m128i) -> __m128i;
        fn _mm_mask_alignr_epi32<const IMM8: i32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_maskz_alignr_epi32<const IMM8: i32>(
            k: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm256_alignr_epi64<const IMM8: i32>(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_alignr_epi64<const IMM8: i32>(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_alignr_epi64<const IMM8: i32>(
            k: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm_alignr_epi64<const IMM8: i32>(a: __m128i, b: __m128i) -> __m128i;
        fn _mm_mask_alignr_epi64<const IMM8: i32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_maskz_alignr_epi64<const IMM8: i32>(
            k: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm256_mask_and_epi32(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_and_epi32(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_and_epi32(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_and_epi32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_and_epi64(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_and_epi64(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_and_epi64(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_and_epi64(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_or_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_or_epi32(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_or_epi32(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_or_epi32(a: __m128i, b: __m128i) -> __m128i;
        fn _mm_mask_or_epi32(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_or_epi32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_or_epi64(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_or_epi64(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_or_epi64(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_or_epi64(a: __m128i, b: __m128i) -> __m128i;
        fn _mm_mask_or_epi64(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_or_epi64(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_xor_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_xor_epi32(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_xor_epi32(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_xor_epi32(a: __m128i, b: __m128i) -> __m128i;
        fn _mm_mask_xor_epi32(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_xor_epi32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_xor_epi64(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_xor_epi64(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_xor_epi64(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_xor_epi64(a: __m128i, b: __m128i) -> __m128i;
        fn _mm_mask_xor_epi64(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_xor_epi64(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_andnot_epi32(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_andnot_epi32(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_andnot_epi32(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_andnot_epi32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_andnot_epi64(
            src: __m256i,
            k: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_andnot_epi64(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_andnot_epi64(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_andnot_epi64(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_test_epi32_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_test_epi32_mask(k: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_test_epi32_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_test_epi32_mask(k: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_test_epi64_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_test_epi64_mask(k: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_test_epi64_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_test_epi64_mask(k: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_testn_epi32_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_testn_epi32_mask(k: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_testn_epi32_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_testn_epi32_mask(k: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_testn_epi64_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_testn_epi64_mask(k: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_testn_epi64_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_testn_epi64_mask(k: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_mask_set1_epi32(src: __m256i, k: __mmask8, a: i32) -> __m256i;
        fn _mm256_maskz_set1_epi32(k: __mmask8, a: i32) -> __m256i;
        fn _mm_mask_set1_epi32(src: __m128i, k: __mmask8, a: i32) -> __m128i;
        fn _mm_maskz_set1_epi32(k: __mmask8, a: i32) -> __m128i;
        fn _mm256_mask_set1_epi64(src: __m256i, k: __mmask8, a: i64) -> __m256i;
        fn _mm256_maskz_set1_epi64(k: __mmask8, a: i64) -> __m256i;
        fn _mm_mask_set1_epi64(src: __m128i, k: __mmask8, a: i64) -> __m128i;
        fn _mm_maskz_set1_epi64(k: __mmask8, a: i64) -> __m128i;
        fn _mm256_cmp_ps_mask<const IMM8: i32>(a: __m256, b: __m256) -> __mmask8;
        fn _mm256_mask_cmp_ps_mask<const IMM8: i32>(
            k1: __mmask8,
            a: __m256,
            b: __m256,
        ) -> __mmask8;
        fn _mm_cmp_ps_mask<const IMM8: i32>(a: __m128, b: __m128) -> __mmask8;
        fn _mm_mask_cmp_ps_mask<const IMM8: i32>(
            k1: __mmask8,
            a: __m128,
            b: __m128,
        ) -> __mmask8;
        fn _mm256_cmp_pd_mask<const IMM8: i32>(a: __m256d, b: __m256d) -> __mmask8;
        fn _mm256_mask_cmp_pd_mask<const IMM8: i32>(
            k1: __mmask8,
            a: __m256d,
            b: __m256d,
        ) -> __mmask8;
        fn _mm_cmp_pd_mask<const IMM8: i32>(a: __m128d, b: __m128d) -> __mmask8;
        fn _mm_mask_cmp_pd_mask<const IMM8: i32>(
            k1: __mmask8,
            a: __m128d,
            b: __m128d,
        ) -> __mmask8;
        fn _mm256_cmplt_epu32_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmplt_epu32_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmplt_epu32_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmplt_epu32_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpgt_epu32_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmpgt_epu32_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmpgt_epu32_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpgt_epu32_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmple_epu32_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmple_epu32_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmple_epu32_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmple_epu32_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpge_epu32_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmpge_epu32_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmpge_epu32_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpge_epu32_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpeq_epu32_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmpeq_epu32_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmpeq_epu32_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpeq_epu32_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpneq_epu32_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmpneq_epu32_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmpneq_epu32_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpneq_epu32_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmp_epu32_mask<const IMM3: _MM_CMPINT_ENUM>(
            a: __m256i,
            b: __m256i,
        ) -> __mmask8;
        fn _mm256_mask_cmp_epu32_mask<const IMM3: _MM_CMPINT_ENUM>(
            k1: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __mmask8;
        fn _mm_cmp_epu32_mask<const IMM3: _MM_CMPINT_ENUM>(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmp_epu32_mask<const IMM3: _MM_CMPINT_ENUM>(
            k1: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __mmask8;
        fn _mm256_cmplt_epi32_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmplt_epi32_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmplt_epi32_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmplt_epi32_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpgt_epi32_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmpgt_epi32_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmpgt_epi32_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpgt_epi32_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmple_epi32_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmple_epi32_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmple_epi32_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmple_epi32_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpge_epi32_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmpge_epi32_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmpge_epi32_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpge_epi32_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpeq_epi32_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmpeq_epi32_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmpeq_epi32_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpeq_epi32_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpneq_epi32_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmpneq_epi32_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmpneq_epi32_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpneq_epi32_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmp_epi32_mask<const IMM3: _MM_CMPINT_ENUM>(
            a: __m256i,
            b: __m256i,
        ) -> __mmask8;
        fn _mm256_mask_cmp_epi32_mask<const IMM3: _MM_CMPINT_ENUM>(
            k1: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __mmask8;
        fn _mm_cmp_epi32_mask<const IMM3: _MM_CMPINT_ENUM>(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmp_epi32_mask<const IMM3: _MM_CMPINT_ENUM>(
            k1: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __mmask8;
        fn _mm256_cmplt_epu64_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmplt_epu64_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmplt_epu64_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmplt_epu64_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpgt_epu64_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmpgt_epu64_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmpgt_epu64_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpgt_epu64_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmple_epu64_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmple_epu64_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmple_epu64_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmple_epu64_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpge_epu64_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmpge_epu64_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmpge_epu64_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpge_epu64_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpeq_epu64_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmpeq_epu64_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmpeq_epu64_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpeq_epu64_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpneq_epu64_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmpneq_epu64_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmpneq_epu64_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpneq_epu64_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmp_epu64_mask<const IMM3: _MM_CMPINT_ENUM>(
            a: __m256i,
            b: __m256i,
        ) -> __mmask8;
        fn _mm256_mask_cmp_epu64_mask<const IMM3: _MM_CMPINT_ENUM>(
            k1: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __mmask8;
        fn _mm_cmp_epu64_mask<const IMM3: _MM_CMPINT_ENUM>(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmp_epu64_mask<const IMM3: _MM_CMPINT_ENUM>(
            k1: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __mmask8;
        fn _mm256_cmplt_epi64_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmplt_epi64_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmplt_epi64_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmplt_epi64_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpgt_epi64_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmpgt_epi64_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmpgt_epi64_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpgt_epi64_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmple_epi64_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmple_epi64_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmple_epi64_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmple_epi64_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpge_epi64_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmpge_epi64_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmpge_epi64_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpge_epi64_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpeq_epi64_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmpeq_epi64_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmpeq_epi64_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpeq_epi64_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpneq_epi64_mask(a: __m256i, b: __m256i) -> __mmask8;
        fn _mm256_mask_cmpneq_epi64_mask(k1: __mmask8, a: __m256i, b: __m256i) -> __mmask8;
        fn _mm_cmpneq_epi64_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpneq_epi64_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmp_epi64_mask<const IMM3: _MM_CMPINT_ENUM>(
            a: __m256i,
            b: __m256i,
        ) -> __mmask8;
        fn _mm256_mask_cmp_epi64_mask<const IMM3: _MM_CMPINT_ENUM>(
            k1: __mmask8,
            a: __m256i,
            b: __m256i,
        ) -> __mmask8;
        fn _mm_cmp_epi64_mask<const IMM3: _MM_CMPINT_ENUM>(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmp_epi64_mask<const IMM3: _MM_CMPINT_ENUM>(
            k1: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __mmask8;
        unsafe fn _mm256_loadu_epi32(mem_addr: *const i32) -> __m256i;
        unsafe fn _mm_loadu_epi32(mem_addr: *const i32) -> __m128i;
        unsafe fn _mm256_mask_cvtepi32_storeu_epi16(mem_addr: *mut i8, k: __mmask8, a: __m256i);
        unsafe fn _mm_mask_cvtepi32_storeu_epi16(mem_addr: *mut i8, k: __mmask8, a: __m128i);
        unsafe fn _mm256_mask_cvtsepi32_storeu_epi16(mem_addr: *mut i8, k: __mmask8, a: __m256i);
        unsafe fn _mm_mask_cvtsepi32_storeu_epi16(mem_addr: *mut i8, k: __mmask8, a: __m128i);
        unsafe fn _mm256_mask_cvtusepi32_storeu_epi16(mem_addr: *mut i8, k: __mmask8, a: __m256i);
        unsafe fn _mm_mask_cvtusepi32_storeu_epi16(mem_addr: *mut i8, k: __mmask8, a: __m128i);
        unsafe fn _mm256_mask_cvtepi32_storeu_epi8(mem_addr: *mut i8, k: __mmask8, a: __m256i);
        unsafe fn _mm_mask_cvtepi32_storeu_epi8(mem_addr: *mut i8, k: __mmask8, a: __m128i);
        unsafe fn _mm256_mask_cvtsepi32_storeu_epi8(mem_addr: *mut i8, k: __mmask8, a: __m256i);
        unsafe fn _mm_mask_cvtsepi32_storeu_epi8(mem_addr: *mut i8, k: __mmask8, a: __m128i);
        unsafe fn _mm256_mask_cvtusepi32_storeu_epi8(mem_addr: *mut i8, k: __mmask8, a: __m256i);
        unsafe fn _mm_mask_cvtusepi32_storeu_epi8(mem_addr: *mut i8, k: __mmask8, a: __m128i);
        unsafe fn _mm256_mask_cvtepi64_storeu_epi16(mem_addr: *mut i8, k: __mmask8, a: __m256i);
        unsafe fn _mm_mask_cvtepi64_storeu_epi16(mem_addr: *mut i8, k: __mmask8, a: __m128i);
        unsafe fn _mm256_mask_cvtsepi64_storeu_epi16(mem_addr: *mut i8, k: __mmask8, a: __m256i);
        unsafe fn _mm_mask_cvtsepi64_storeu_epi16(mem_addr: *mut i8, k: __mmask8, a: __m128i);
        unsafe fn _mm256_mask_cvtusepi64_storeu_epi16(mem_addr: *mut i8, k: __mmask8, a: __m256i);
        unsafe fn _mm_mask_cvtusepi64_storeu_epi16(mem_addr: *mut i8, k: __mmask8, a: __m128i);
        unsafe fn _mm256_mask_cvtepi64_storeu_epi8(mem_addr: *mut i8, k: __mmask8, a: __m256i);
        unsafe fn _mm_mask_cvtepi64_storeu_epi8(mem_addr: *mut i8, k: __mmask8, a: __m128i);
        unsafe fn _mm256_mask_cvtsepi64_storeu_epi8(mem_addr: *mut i8, k: __mmask8, a: __m256i);
        unsafe fn _mm_mask_cvtsepi64_storeu_epi8(mem_addr: *mut i8, k: __mmask8, a: __m128i);
        unsafe fn _mm256_mask_cvtusepi64_storeu_epi8(mem_addr: *mut i8, k: __mmask8, a: __m256i);
        unsafe fn _mm_mask_cvtusepi64_storeu_epi8(mem_addr: *mut i8, k: __mmask8, a: __m128i);
        unsafe fn _mm256_mask_cvtepi64_storeu_epi32(mem_addr: *mut i8, k: __mmask8, a: __m256i);
        unsafe fn _mm_mask_cvtepi64_storeu_epi32(mem_addr: *mut i8, k: __mmask8, a: __m128i);
        unsafe fn _mm256_mask_cvtsepi64_storeu_epi32(mem_addr: *mut i8, k: __mmask8, a: __m256i);
        unsafe fn _mm_mask_cvtsepi64_storeu_epi32(mem_addr: *mut i8, k: __mmask8, a: __m128i);
        unsafe fn _mm256_mask_cvtusepi64_storeu_epi32(mem_addr: *mut i8, k: __mmask8, a: __m256i);
        unsafe fn _mm_mask_cvtusepi64_storeu_epi32(mem_addr: *mut i8, k: __mmask8, a: __m128i);
        unsafe fn _mm256_storeu_epi32(mem_addr: *mut i32, a: __m256i);
        unsafe fn _mm_storeu_epi32(mem_addr: *mut i32, a: __m128i);
        unsafe fn _mm256_loadu_epi64(mem_addr: *const i64) -> __m256i;
        unsafe fn _mm_loadu_epi64(mem_addr: *const i64) -> __m128i;
        unsafe fn _mm256_storeu_epi64(mem_addr: *mut i64, a: __m256i);
        unsafe fn _mm_storeu_epi64(mem_addr: *mut i64, a: __m128i);
        unsafe fn _mm256_load_epi32(mem_addr: *const i32) -> __m256i;
        unsafe fn _mm_load_epi32(mem_addr: *const i32) -> __m128i;
        unsafe fn _mm256_store_epi32(mem_addr: *mut i32, a: __m256i);
        unsafe fn _mm_store_epi32(mem_addr: *mut i32, a: __m128i);
        unsafe fn _mm256_load_epi64(mem_addr: *const i64) -> __m256i;
        unsafe fn _mm_load_epi64(mem_addr: *const i64) -> __m128i;
        unsafe fn _mm256_store_epi64(mem_addr: *mut i64, a: __m256i);
    }
}
