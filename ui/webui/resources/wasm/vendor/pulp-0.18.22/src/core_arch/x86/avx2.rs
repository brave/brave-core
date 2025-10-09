use super::*;

impl Avx2 {
    delegate! {
        fn _mm256_abs_epi32(a: __m256i) -> __m256i;
        fn _mm256_abs_epi16(a: __m256i) -> __m256i;
        fn _mm256_abs_epi8(a: __m256i) -> __m256i;
        fn _mm256_add_epi64(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_add_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_add_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_add_epi8(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_adds_epi8(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_adds_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_adds_epu8(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_adds_epu16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_alignr_epi8<const IMM8: i32>(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_and_si256(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_andnot_si256(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_avg_epu16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_avg_epu8(a: __m256i, b: __m256i) -> __m256i;
        fn _mm_blend_epi32<const IMM4: i32>(a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_blend_epi32<const IMM8: i32>(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_blend_epi16<const IMM8: i32>(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_blendv_epi8(a: __m256i, b: __m256i, mask: __m256i) -> __m256i;
        fn _mm_broadcastb_epi8(a: __m128i) -> __m128i;
        fn _mm256_broadcastb_epi8(a: __m128i) -> __m256i;
        fn _mm_broadcastd_epi32(a: __m128i) -> __m128i;
        fn _mm256_broadcastd_epi32(a: __m128i) -> __m256i;
        fn _mm_broadcastq_epi64(a: __m128i) -> __m128i;
        fn _mm256_broadcastq_epi64(a: __m128i) -> __m256i;
        fn _mm_broadcastsd_pd(a: __m128d) -> __m128d;
        fn _mm256_broadcastsd_pd(a: __m128d) -> __m256d;
        fn _mm256_broadcastsi128_si256(a: __m128i) -> __m256i;
        fn _mm_broadcastss_ps(a: __m128) -> __m128;
        fn _mm256_broadcastss_ps(a: __m128) -> __m256;
        fn _mm_broadcastw_epi16(a: __m128i) -> __m128i;
        fn _mm256_broadcastw_epi16(a: __m128i) -> __m256i;
        fn _mm256_cmpeq_epi64(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_cmpeq_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_cmpeq_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_cmpeq_epi8(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_cmpgt_epi64(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_cmpgt_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_cmpgt_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_cmpgt_epi8(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_cvtepi16_epi32(a: __m128i) -> __m256i;
        fn _mm256_cvtepi16_epi64(a: __m128i) -> __m256i;
        fn _mm256_cvtepi32_epi64(a: __m128i) -> __m256i;
        fn _mm256_cvtepi8_epi16(a: __m128i) -> __m256i;
        fn _mm256_cvtepi8_epi32(a: __m128i) -> __m256i;
        fn _mm256_cvtepi8_epi64(a: __m128i) -> __m256i;
        fn _mm256_cvtepu16_epi32(a: __m128i) -> __m256i;
        fn _mm256_cvtepu16_epi64(a: __m128i) -> __m256i;
        fn _mm256_cvtepu32_epi64(a: __m128i) -> __m256i;
        fn _mm256_cvtepu8_epi16(a: __m128i) -> __m256i;
        fn _mm256_cvtepu8_epi32(a: __m128i) -> __m256i;
        fn _mm256_cvtepu8_epi64(a: __m128i) -> __m256i;
        fn _mm256_extracti128_si256<const IMM1: i32>(a: __m256i) -> __m128i;
        fn _mm256_hadd_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_hadd_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_hadds_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_hsub_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_hsub_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_hsubs_epi16(a: __m256i, b: __m256i) -> __m256i;
        unsafe fn _mm_i32gather_epi32<const SCALE: i32>(
            slice: *const i32,
            offsets: __m128i,
        ) -> __m128i;
        unsafe fn _mm_mask_i32gather_epi32<const SCALE: i32>(
            src: __m128i,
            slice: *const i32,
            offsets: __m128i,
            mask: __m128i,
        ) -> __m128i;
        unsafe fn _mm256_i32gather_epi32<const SCALE: i32>(
            slice: *const i32,
            offsets: __m256i,
        ) -> __m256i;
        unsafe fn _mm256_mask_i32gather_epi32<const SCALE: i32>(
            src: __m256i,
            slice: *const i32,
            offsets: __m256i,
            mask: __m256i,
        ) -> __m256i;
        unsafe fn _mm_i32gather_ps<const SCALE: i32>(slice: *const f32, offsets: __m128i) -> __m128;
        unsafe fn _mm_mask_i32gather_ps<const SCALE: i32>(
            src: __m128,
            slice: *const f32,
            offsets: __m128i,
            mask: __m128,
        ) -> __m128;
        unsafe fn _mm256_i32gather_ps<const SCALE: i32>(slice: *const f32, offsets: __m256i) -> __m256;
        unsafe fn _mm256_mask_i32gather_ps<const SCALE: i32>(
            src: __m256,
            slice: *const f32,
            offsets: __m256i,
            mask: __m256,
        ) -> __m256;
        unsafe fn _mm_i32gather_epi64<const SCALE: i32>(
            slice: *const i64,
            offsets: __m128i,
        ) -> __m128i;
        unsafe fn _mm_mask_i32gather_epi64<const SCALE: i32>(
            src: __m128i,
            slice: *const i64,
            offsets: __m128i,
            mask: __m128i,
        ) -> __m128i;
        unsafe fn _mm256_i32gather_epi64<const SCALE: i32>(
            slice: *const i64,
            offsets: __m128i,
        ) -> __m256i;
        unsafe fn _mm256_mask_i32gather_epi64<const SCALE: i32>(
            src: __m256i,
            slice: *const i64,
            offsets: __m128i,
            mask: __m256i,
        ) -> __m256i;
        unsafe fn _mm_i32gather_pd<const SCALE: i32>(slice: *const f64, offsets: __m128i) -> __m128d;
        unsafe fn _mm_mask_i32gather_pd<const SCALE: i32>(
            src: __m128d,
            slice: *const f64,
            offsets: __m128i,
            mask: __m128d,
        ) -> __m128d;
        unsafe fn _mm256_i32gather_pd<const SCALE: i32>(
            slice: *const f64,
            offsets: __m128i,
        ) -> __m256d;
        unsafe fn _mm256_mask_i32gather_pd<const SCALE: i32>(
            src: __m256d,
            slice: *const f64,
            offsets: __m128i,
            mask: __m256d,
        ) -> __m256d;
        unsafe fn _mm_i64gather_epi32<const SCALE: i32>(
            slice: *const i32,
            offsets: __m128i,
        ) -> __m128i;
        unsafe fn _mm_mask_i64gather_epi32<const SCALE: i32>(
            src: __m128i,
            slice: *const i32,
            offsets: __m128i,
            mask: __m128i,
        ) -> __m128i;
        unsafe fn _mm256_i64gather_epi32<const SCALE: i32>(
            slice: *const i32,
            offsets: __m256i,
        ) -> __m128i;
        unsafe fn _mm256_mask_i64gather_epi32<const SCALE: i32>(
            src: __m128i,
            slice: *const i32,
            offsets: __m256i,
            mask: __m128i,
        ) -> __m128i;
        unsafe fn _mm_i64gather_ps<const SCALE: i32>(slice: *const f32, offsets: __m128i) -> __m128;
        unsafe fn _mm_mask_i64gather_ps<const SCALE: i32>(
            src: __m128,
            slice: *const f32,
            offsets: __m128i,
            mask: __m128,
        ) -> __m128;
        unsafe fn _mm256_i64gather_ps<const SCALE: i32>(slice: *const f32, offsets: __m256i) -> __m128;
        unsafe fn _mm256_mask_i64gather_ps<const SCALE: i32>(
            src: __m128,
            slice: *const f32,
            offsets: __m256i,
            mask: __m128,
        ) -> __m128;
        unsafe fn _mm_i64gather_epi64<const SCALE: i32>(
            slice: *const i64,
            offsets: __m128i,
        ) -> __m128i;
        unsafe fn _mm_mask_i64gather_epi64<const SCALE: i32>(
            src: __m128i,
            slice: *const i64,
            offsets: __m128i,
            mask: __m128i,
        ) -> __m128i;
        unsafe fn _mm256_i64gather_epi64<const SCALE: i32>(
            slice: *const i64,
            offsets: __m256i,
        ) -> __m256i;
        unsafe fn _mm256_mask_i64gather_epi64<const SCALE: i32>(
            src: __m256i,
            slice: *const i64,
            offsets: __m256i,
            mask: __m256i,
        ) -> __m256i;
        unsafe fn _mm_i64gather_pd<const SCALE: i32>(slice: *const f64, offsets: __m128i) -> __m128d;
        unsafe fn _mm_mask_i64gather_pd<const SCALE: i32>(
            src: __m128d,
            slice: *const f64,
            offsets: __m128i,
            mask: __m128d,
        ) -> __m128d;
        unsafe fn _mm256_i64gather_pd<const SCALE: i32>(
            slice: *const f64,
            offsets: __m256i,
        ) -> __m256d;
        unsafe fn _mm256_mask_i64gather_pd<const SCALE: i32>(
            src: __m256d,
            slice: *const f64,
            offsets: __m256i,
            mask: __m256d,
        ) -> __m256d;
        fn _mm256_inserti128_si256<const IMM1: i32>(a: __m256i, b: __m128i) -> __m256i;
        fn _mm256_madd_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maddubs_epi16(a: __m256i, b: __m256i) -> __m256i;
        unsafe fn _mm_maskload_epi32(mem_addr: *const i32, mask: __m128i) -> __m128i;
        unsafe fn _mm256_maskload_epi32(mem_addr: *const i32, mask: __m256i) -> __m256i;
        unsafe fn _mm_maskload_epi64(mem_addr: *const i64, mask: __m128i) -> __m128i;
        unsafe fn _mm256_maskload_epi64(mem_addr: *const i64, mask: __m256i) -> __m256i;
        unsafe fn _mm_maskstore_epi32(mem_addr: *mut i32, mask: __m128i, a: __m128i);
        unsafe fn _mm256_maskstore_epi32(mem_addr: *mut i32, mask: __m256i, a: __m256i);
        unsafe fn _mm_maskstore_epi64(mem_addr: *mut i64, mask: __m128i, a: __m128i);
        unsafe fn _mm256_maskstore_epi64(mem_addr: *mut i64, mask: __m256i, a: __m256i);
        fn _mm256_max_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_max_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_max_epi8(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_max_epu16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_max_epu32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_max_epu8(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_min_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_min_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_min_epi8(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_min_epu16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_min_epu32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_min_epu8(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_movemask_epi8(a: __m256i) -> i32;
        fn _mm256_mpsadbw_epu8<const IMM8: i32>(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mul_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mul_epu32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mulhi_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mulhi_epu16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mullo_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mullo_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mulhrs_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_or_si256(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_packs_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_packs_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_packus_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_packus_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_permutevar8x32_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_permute4x64_epi64<const IMM8: i32>(a: __m256i) -> __m256i;
        fn _mm256_permute2x128_si256<const IMM8: i32>(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_permute4x64_pd<const IMM8: i32>(a: __m256d) -> __m256d;
        fn _mm256_permutevar8x32_ps(a: __m256, idx: __m256i) -> __m256;
        fn _mm256_sad_epu8(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_shuffle_epi8(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_shuffle_epi32<const MASK: i32>(a: __m256i) -> __m256i;
        fn _mm256_shufflehi_epi16<const IMM8: i32>(a: __m256i) -> __m256i;
        fn _mm256_shufflelo_epi16<const IMM8: i32>(a: __m256i) -> __m256i;
        fn _mm256_sign_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_sign_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_sign_epi8(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_sll_epi16(a: __m256i, count: __m128i) -> __m256i;
        fn _mm256_sll_epi32(a: __m256i, count: __m128i) -> __m256i;
        fn _mm256_sll_epi64(a: __m256i, count: __m128i) -> __m256i;
        fn _mm256_slli_epi16<const IMM8: i32>(a: __m256i) -> __m256i;
        fn _mm256_slli_epi32<const IMM8: i32>(a: __m256i) -> __m256i;
        fn _mm256_slli_epi64<const IMM8: i32>(a: __m256i) -> __m256i;
        fn _mm256_slli_si256<const IMM8: i32>(a: __m256i) -> __m256i;
        fn _mm256_bslli_epi128<const IMM8: i32>(a: __m256i) -> __m256i;
        fn _mm_sllv_epi32(a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_sllv_epi32(a: __m256i, count: __m256i) -> __m256i;
        fn _mm_sllv_epi64(a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_sllv_epi64(a: __m256i, count: __m256i) -> __m256i;
        fn _mm256_sra_epi16(a: __m256i, count: __m128i) -> __m256i;
        fn _mm256_sra_epi32(a: __m256i, count: __m128i) -> __m256i;
        fn _mm256_srai_epi16<const IMM8: i32>(a: __m256i) -> __m256i;
        fn _mm256_srai_epi32<const IMM8: i32>(a: __m256i) -> __m256i;
        fn _mm_srav_epi32(a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_srav_epi32(a: __m256i, count: __m256i) -> __m256i;
        fn _mm256_srli_si256<const IMM8: i32>(a: __m256i) -> __m256i;
        fn _mm256_bsrli_epi128<const IMM8: i32>(a: __m256i) -> __m256i;
        fn _mm256_srl_epi16(a: __m256i, count: __m128i) -> __m256i;
        fn _mm256_srl_epi32(a: __m256i, count: __m128i) -> __m256i;
        fn _mm256_srl_epi64(a: __m256i, count: __m128i) -> __m256i;
        fn _mm256_srli_epi16<const IMM8: i32>(a: __m256i) -> __m256i;
        fn _mm256_srli_epi32<const IMM8: i32>(a: __m256i) -> __m256i;
        fn _mm256_srli_epi64<const IMM8: i32>(a: __m256i) -> __m256i;
        fn _mm_srlv_epi32(a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_srlv_epi32(a: __m256i, count: __m256i) -> __m256i;
        fn _mm_srlv_epi64(a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_srlv_epi64(a: __m256i, count: __m256i) -> __m256i;
        fn _mm256_sub_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_sub_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_sub_epi64(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_sub_epi8(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_subs_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_subs_epi8(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_subs_epu16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_subs_epu8(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_unpackhi_epi8(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_unpacklo_epi8(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_unpackhi_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_unpacklo_epi16(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_unpackhi_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_unpacklo_epi32(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_unpackhi_epi64(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_unpacklo_epi64(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_xor_si256(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_extract_epi8<const INDEX: i32>(a: __m256i) -> i32;
        fn _mm256_extract_epi16<const INDEX: i32>(a: __m256i) -> i32;
        fn _mm256_extract_epi32<const INDEX: i32>(a: __m256i) -> i32;
        fn _mm256_cvtsd_f64(a: __m256d) -> f64;
        fn _mm256_cvtsi256_si32(a: __m256i) -> i32;
    }
}
