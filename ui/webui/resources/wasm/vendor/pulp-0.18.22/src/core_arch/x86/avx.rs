use super::*;

impl Avx {
    delegate! {
        fn _mm256_add_pd(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_add_ps(a: __m256, b: __m256) -> __m256;
        fn _mm256_and_pd(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_and_ps(a: __m256, b: __m256) -> __m256;
        fn _mm256_or_pd(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_or_ps(a: __m256, b: __m256) -> __m256;
        fn _mm256_shuffle_pd<const MASK: i32>(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_shuffle_ps<const MASK: i32>(a: __m256, b: __m256) -> __m256;
        fn _mm256_andnot_pd(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_andnot_ps(a: __m256, b: __m256) -> __m256;
        fn _mm256_max_pd(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_max_ps(a: __m256, b: __m256) -> __m256;
        fn _mm256_min_pd(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_min_ps(a: __m256, b: __m256) -> __m256;
        fn _mm256_mul_pd(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_mul_ps(a: __m256, b: __m256) -> __m256;
        fn _mm256_addsub_pd(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_addsub_ps(a: __m256, b: __m256) -> __m256;
        fn _mm256_sub_pd(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_sub_ps(a: __m256, b: __m256) -> __m256;
        fn _mm256_div_ps(a: __m256, b: __m256) -> __m256;
        fn _mm256_div_pd(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_round_pd<const ROUNDING: i32>(a: __m256d) -> __m256d;
        fn _mm256_ceil_pd(a: __m256d) -> __m256d;
        fn _mm256_floor_pd(a: __m256d) -> __m256d;
        fn _mm256_round_ps<const ROUNDING: i32>(a: __m256) -> __m256;
        fn _mm256_ceil_ps(a: __m256) -> __m256;
        fn _mm256_floor_ps(a: __m256) -> __m256;
        fn _mm256_sqrt_ps(a: __m256) -> __m256;
        fn _mm256_sqrt_pd(a: __m256d) -> __m256d;
        fn _mm256_blend_pd<const IMM4: i32>(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_blend_ps<const IMM8: i32>(a: __m256, b: __m256) -> __m256;
        fn _mm256_blendv_pd(a: __m256d, b: __m256d, c: __m256d) -> __m256d;
        fn _mm256_blendv_ps(a: __m256, b: __m256, c: __m256) -> __m256;
        fn _mm256_dp_ps<const IMM8: i32>(a: __m256, b: __m256) -> __m256;
        fn _mm256_hadd_pd(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_hadd_ps(a: __m256, b: __m256) -> __m256;
        fn _mm256_hsub_pd(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_hsub_ps(a: __m256, b: __m256) -> __m256;
        fn _mm256_xor_pd(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_xor_ps(a: __m256, b: __m256) -> __m256;
        fn _mm_cmp_pd<const IMM5: i32>(a: __m128d, b: __m128d) -> __m128d;
        fn _mm256_cmp_pd<const IMM5: i32>(a: __m256d, b: __m256d) -> __m256d;
        fn _mm_cmp_ps<const IMM5: i32>(a: __m128, b: __m128) -> __m128;
        fn _mm256_cmp_ps<const IMM5: i32>(a: __m256, b: __m256) -> __m256;
        fn _mm_cmp_sd<const IMM5: i32>(a: __m128d, b: __m128d) -> __m128d;
        fn _mm_cmp_ss<const IMM5: i32>(a: __m128, b: __m128) -> __m128;
        fn _mm256_cvtepi32_pd(a: __m128i) -> __m256d;
        fn _mm256_cvtepi32_ps(a: __m256i) -> __m256;
        fn _mm256_cvtpd_ps(a: __m256d) -> __m128;
        fn _mm256_cvtps_epi32(a: __m256) -> __m256i;
        fn _mm256_cvtps_pd(a: __m128) -> __m256d;
        fn _mm256_cvttpd_epi32(a: __m256d) -> __m128i;
        fn _mm256_cvtpd_epi32(a: __m256d) -> __m128i;
        fn _mm256_cvttps_epi32(a: __m256) -> __m256i;
        fn _mm256_extractf128_ps<const IMM1: i32>(a: __m256) -> __m128;
        fn _mm256_extractf128_pd<const IMM1: i32>(a: __m256d) -> __m128d;
        fn _mm256_extractf128_si256<const IMM1: i32>(a: __m256i) -> __m128i;
        fn _mm256_zeroall();
        fn _mm256_zeroupper();
        fn _mm256_permutevar_ps(a: __m256, b: __m256i) -> __m256;
        fn _mm_permutevar_ps(a: __m128, b: __m128i) -> __m128;
        fn _mm256_permute_ps<const IMM8: i32>(a: __m256) -> __m256;
        fn _mm_permute_ps<const IMM8: i32>(a: __m128) -> __m128;
        fn _mm256_permutevar_pd(a: __m256d, b: __m256i) -> __m256d;
        fn _mm_permutevar_pd(a: __m128d, b: __m128i) -> __m128d;
        fn _mm256_permute_pd<const IMM4: i32>(a: __m256d) -> __m256d;
        fn _mm_permute_pd<const IMM2: i32>(a: __m128d) -> __m128d;
        fn _mm256_permute2f128_ps<const IMM8: i32>(a: __m256, b: __m256) -> __m256;
        fn _mm256_permute2f128_pd<const IMM8: i32>(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_permute2f128_si256<const IMM8: i32>(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_broadcast_ss(f: &f32) -> __m256;
        fn _mm_broadcast_ss(f: &f32) -> __m128;
        fn _mm256_broadcast_sd(f: &f64) -> __m256d;
        fn _mm256_broadcast_ps(a: &__m128) -> __m256;
        fn _mm256_broadcast_pd(a: &__m128d) -> __m256d;
        fn _mm256_insertf128_ps<const IMM1: i32>(a: __m256, b: __m128) -> __m256;
        fn _mm256_insertf128_pd<const IMM1: i32>(a: __m256d, b: __m128d) -> __m256d;
        fn _mm256_insertf128_si256<const IMM1: i32>(a: __m256i, b: __m128i) -> __m256i;
        fn _mm256_insert_epi8<const INDEX: i32>(a: __m256i, i: i8) -> __m256i;
        fn _mm256_insert_epi16<const INDEX: i32>(a: __m256i, i: i16) -> __m256i;
        fn _mm256_insert_epi32<const INDEX: i32>(a: __m256i, i: i32) -> __m256i;
        unsafe fn _mm256_load_pd(mem_addr: *const f64) -> __m256d;
        unsafe fn _mm256_store_pd(mem_addr: *mut f64, a: __m256d);
        unsafe fn _mm256_load_ps(mem_addr: *const f32) -> __m256;
        unsafe fn _mm256_store_ps(mem_addr: *mut f32, a: __m256);
        unsafe fn _mm256_loadu_pd(mem_addr: *const f64) -> __m256d;
        unsafe fn _mm256_storeu_pd(mem_addr: *mut f64, a: __m256d);
        unsafe fn _mm256_loadu_ps(mem_addr: *const f32) -> __m256;
        unsafe fn _mm256_storeu_ps(mem_addr: *mut f32, a: __m256);
        unsafe fn _mm256_load_si256(mem_addr: *const __m256i) -> __m256i;
        unsafe fn _mm256_store_si256(mem_addr: *mut __m256i, a: __m256i);
        unsafe fn _mm256_loadu_si256(mem_addr: *const __m256i) -> __m256i;
        unsafe fn _mm256_storeu_si256(mem_addr: *mut __m256i, a: __m256i);
        unsafe fn _mm256_maskload_pd(mem_addr: *const f64, mask: __m256i) -> __m256d;
        unsafe fn _mm256_maskstore_pd(mem_addr: *mut f64, mask: __m256i, a: __m256d);
        unsafe fn _mm_maskload_pd(mem_addr: *const f64, mask: __m128i) -> __m128d;
        unsafe fn _mm_maskstore_pd(mem_addr: *mut f64, mask: __m128i, a: __m128d);
        unsafe fn _mm256_maskload_ps(mem_addr: *const f32, mask: __m256i) -> __m256;
        unsafe fn _mm256_maskstore_ps(mem_addr: *mut f32, mask: __m256i, a: __m256);
        unsafe fn _mm_maskload_ps(mem_addr: *const f32, mask: __m128i) -> __m128;
        unsafe fn _mm_maskstore_ps(mem_addr: *mut f32, mask: __m128i, a: __m128);
        fn _mm256_movehdup_ps(a: __m256) -> __m256;
        fn _mm256_moveldup_ps(a: __m256) -> __m256;
        fn _mm256_movedup_pd(a: __m256d) -> __m256d;
        unsafe fn _mm256_lddqu_si256(mem_addr: *const __m256i) -> __m256i;
        unsafe fn _mm256_stream_si256(mem_addr: *mut __m256i, a: __m256i);
        unsafe fn _mm256_stream_pd(mem_addr: *mut f64, a: __m256d);
        unsafe fn _mm256_stream_ps(mem_addr: *mut f32, a: __m256);
        fn _mm256_rcp_ps(a: __m256) -> __m256;
        fn _mm256_rsqrt_ps(a: __m256) -> __m256;
        fn _mm256_unpackhi_pd(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_unpackhi_ps(a: __m256, b: __m256) -> __m256;
        fn _mm256_unpacklo_pd(a: __m256d, b: __m256d) -> __m256d;
        fn _mm256_unpacklo_ps(a: __m256, b: __m256) -> __m256;
        fn _mm256_testz_si256(a: __m256i, b: __m256i) -> i32;
        fn _mm256_testc_si256(a: __m256i, b: __m256i) -> i32;
        fn _mm256_testnzc_si256(a: __m256i, b: __m256i) -> i32;
        fn _mm256_testz_pd(a: __m256d, b: __m256d) -> i32;
        fn _mm256_testc_pd(a: __m256d, b: __m256d) -> i32;
        fn _mm256_testnzc_pd(a: __m256d, b: __m256d) -> i32;
        fn _mm_testz_pd(a: __m128d, b: __m128d) -> i32;
        fn _mm_testc_pd(a: __m128d, b: __m128d) -> i32;
        fn _mm_testnzc_pd(a: __m128d, b: __m128d) -> i32;
        fn _mm256_testz_ps(a: __m256, b: __m256) -> i32;
        fn _mm256_testc_ps(a: __m256, b: __m256) -> i32;
        fn _mm256_testnzc_ps(a: __m256, b: __m256) -> i32;
        fn _mm_testz_ps(a: __m128, b: __m128) -> i32;
        fn _mm_testc_ps(a: __m128, b: __m128) -> i32;
        fn _mm_testnzc_ps(a: __m128, b: __m128) -> i32;
        fn _mm256_movemask_pd(a: __m256d) -> i32;
        fn _mm256_movemask_ps(a: __m256) -> i32;
        fn _mm256_setzero_pd() -> __m256d;
        fn _mm256_setzero_ps() -> __m256;
        fn _mm256_setzero_si256() -> __m256i;
        fn _mm256_set_pd(a: f64, b: f64, c: f64, d: f64) -> __m256d;
        fn _mm256_set_ps(
            a: f32,
            b: f32,
            c: f32,
            d: f32,
            e: f32,
            f: f32,
            g: f32,
            h: f32,
        ) -> __m256;
        fn _mm256_set_epi8(
            e00: i8,
            e01: i8,
            e02: i8,
            e03: i8,
            e04: i8,
            e05: i8,
            e06: i8,
            e07: i8,
            e08: i8,
            e09: i8,
            e10: i8,
            e11: i8,
            e12: i8,
            e13: i8,
            e14: i8,
            e15: i8,
            e16: i8,
            e17: i8,
            e18: i8,
            e19: i8,
            e20: i8,
            e21: i8,
            e22: i8,
            e23: i8,
            e24: i8,
            e25: i8,
            e26: i8,
            e27: i8,
            e28: i8,
            e29: i8,
            e30: i8,
            e31: i8,
        ) -> __m256i;
        fn _mm256_set_epi16(
            e00: i16,
            e01: i16,
            e02: i16,
            e03: i16,
            e04: i16,
            e05: i16,
            e06: i16,
            e07: i16,
            e08: i16,
            e09: i16,
            e10: i16,
            e11: i16,
            e12: i16,
            e13: i16,
            e14: i16,
            e15: i16,
        ) -> __m256i;
        fn _mm256_set_epi32(
            e0: i32,
            e1: i32,
            e2: i32,
            e3: i32,
            e4: i32,
            e5: i32,
            e6: i32,
            e7: i32,
        ) -> __m256i;
        fn _mm256_set_epi64x(a: i64, b: i64, c: i64, d: i64) -> __m256i;
        fn _mm256_setr_pd(a: f64, b: f64, c: f64, d: f64) -> __m256d;
        fn _mm256_setr_ps(
            a: f32,
            b: f32,
            c: f32,
            d: f32,
            e: f32,
            f: f32,
            g: f32,
            h: f32,
        ) -> __m256;
        fn _mm256_setr_epi8(
            e00: i8,
            e01: i8,
            e02: i8,
            e03: i8,
            e04: i8,
            e05: i8,
            e06: i8,
            e07: i8,
            e08: i8,
            e09: i8,
            e10: i8,
            e11: i8,
            e12: i8,
            e13: i8,
            e14: i8,
            e15: i8,
            e16: i8,
            e17: i8,
            e18: i8,
            e19: i8,
            e20: i8,
            e21: i8,
            e22: i8,
            e23: i8,
            e24: i8,
            e25: i8,
            e26: i8,
            e27: i8,
            e28: i8,
            e29: i8,
            e30: i8,
            e31: i8,
        ) -> __m256i;
        fn _mm256_setr_epi16(
            e00: i16,
            e01: i16,
            e02: i16,
            e03: i16,
            e04: i16,
            e05: i16,
            e06: i16,
            e07: i16,
            e08: i16,
            e09: i16,
            e10: i16,
            e11: i16,
            e12: i16,
            e13: i16,
            e14: i16,
            e15: i16,
        ) -> __m256i;
        fn _mm256_setr_epi32(
            e0: i32,
            e1: i32,
            e2: i32,
            e3: i32,
            e4: i32,
            e5: i32,
            e6: i32,
            e7: i32,
        ) -> __m256i;
        fn _mm256_setr_epi64x(a: i64, b: i64, c: i64, d: i64) -> __m256i;
        fn _mm256_set1_pd(a: f64) -> __m256d;
        fn _mm256_set1_ps(a: f32) -> __m256;
        fn _mm256_set1_epi8(a: i8) -> __m256i;
        fn _mm256_set1_epi16(a: i16) -> __m256i;
        fn _mm256_set1_epi32(a: i32) -> __m256i;
        fn _mm256_set1_epi64x(a: i64) -> __m256i;
        fn _mm256_castpd_ps(a: __m256d) -> __m256;
        fn _mm256_castps_pd(a: __m256) -> __m256d;
        fn _mm256_castps_si256(a: __m256) -> __m256i;
        fn _mm256_castsi256_ps(a: __m256i) -> __m256;
        fn _mm256_castpd_si256(a: __m256d) -> __m256i;
        fn _mm256_castsi256_pd(a: __m256i) -> __m256d;
        fn _mm256_castps256_ps128(a: __m256) -> __m128;
        fn _mm256_castpd256_pd128(a: __m256d) -> __m128d;
        fn _mm256_castsi256_si128(a: __m256i) -> __m128i;
        fn _mm256_castps128_ps256(a: __m128) -> __m256;
        fn _mm256_castpd128_pd256(a: __m128d) -> __m256d;
        fn _mm256_castsi128_si256(a: __m128i) -> __m256i;
        fn _mm256_zextps128_ps256(a: __m128) -> __m256;
        fn _mm256_zextsi128_si256(a: __m128i) -> __m256i;
        fn _mm256_zextpd128_pd256(a: __m128d) -> __m256d;
        fn _mm256_undefined_ps() -> __m256;
        fn _mm256_undefined_pd() -> __m256d;
        fn _mm256_undefined_si256() -> __m256i;
        fn _mm256_set_m128(hi: __m128, lo: __m128) -> __m256;
        fn _mm256_set_m128d(hi: __m128d, lo: __m128d) -> __m256d;
        fn _mm256_set_m128i(hi: __m128i, lo: __m128i) -> __m256i;
        fn _mm256_setr_m128(lo: __m128, hi: __m128) -> __m256;
        fn _mm256_setr_m128d(lo: __m128d, hi: __m128d) -> __m256d;
        fn _mm256_setr_m128i(lo: __m128i, hi: __m128i) -> __m256i;
        unsafe fn _mm256_loadu2_m128(hiaddr: *const f32, loaddr: *const f32) -> __m256;
        unsafe fn _mm256_loadu2_m128d(hiaddr: *const f64, loaddr: *const f64) -> __m256d;
        unsafe fn _mm256_loadu2_m128i(hiaddr: *const __m128i, loaddr: *const __m128i) -> __m256i;
        unsafe fn _mm256_storeu2_m128(hiaddr: *mut f32, loaddr: *mut f32, a: __m256);
        unsafe fn _mm256_storeu2_m128d(hiaddr: *mut f64, loaddr: *mut f64, a: __m256d);
        unsafe fn _mm256_storeu2_m128i(hiaddr: *mut __m128i, loaddr: *mut __m128i, a: __m256i);
        fn _mm256_cvtss_f32(a: __m256) -> f32;
    }
}
