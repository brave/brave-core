use super::*;

impl Avx512cd {
    delegate! {
        fn _mm512_broadcastmw_epi32(k: __mmask16) -> __m512i;
        fn _mm512_broadcastmb_epi64(k: __mmask8) -> __m512i;
        fn _mm512_conflict_epi32(a: __m512i) -> __m512i;
        fn _mm512_mask_conflict_epi32(src: __m512i, k: __mmask16, a: __m512i) -> __m512i;
        fn _mm512_maskz_conflict_epi32(k: __mmask16, a: __m512i) -> __m512i;
        fn _mm512_conflict_epi64(a: __m512i) -> __m512i;
        fn _mm512_mask_conflict_epi64(src: __m512i, k: __mmask8, a: __m512i) -> __m512i;
        fn _mm512_maskz_conflict_epi64(k: __mmask8, a: __m512i) -> __m512i;
        fn _mm512_lzcnt_epi32(a: __m512i) -> __m512i;
        fn _mm512_mask_lzcnt_epi32(src: __m512i, k: __mmask16, a: __m512i) -> __m512i;
        fn _mm512_maskz_lzcnt_epi32(k: __mmask16, a: __m512i) -> __m512i;
        fn _mm512_lzcnt_epi64(a: __m512i) -> __m512i;
        fn _mm512_mask_lzcnt_epi64(src: __m512i, k: __mmask8, a: __m512i) -> __m512i;
        fn _mm512_maskz_lzcnt_epi64(k: __mmask8, a: __m512i) -> __m512i;
    }
}
impl Avx512cd_Avx512vl {
    delegate! {
        fn _mm256_broadcastmw_epi32(k: __mmask16) -> __m256i;
        fn _mm_broadcastmw_epi32(k: __mmask16) -> __m128i;
        fn _mm256_broadcastmb_epi64(k: __mmask8) -> __m256i;
        fn _mm_broadcastmb_epi64(k: __mmask8) -> __m128i;
        fn _mm256_conflict_epi32(a: __m256i) -> __m256i;
        fn _mm256_mask_conflict_epi32(src: __m256i, k: __mmask8, a: __m256i) -> __m256i;
        fn _mm256_maskz_conflict_epi32(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_conflict_epi32(a: __m128i) -> __m128i;
        fn _mm_mask_conflict_epi32(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_conflict_epi32(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_conflict_epi64(a: __m256i) -> __m256i;
        fn _mm256_mask_conflict_epi64(src: __m256i, k: __mmask8, a: __m256i) -> __m256i;
        fn _mm256_maskz_conflict_epi64(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_conflict_epi64(a: __m128i) -> __m128i;
        fn _mm_mask_conflict_epi64(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_conflict_epi64(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_lzcnt_epi32(a: __m256i) -> __m256i;
        fn _mm256_mask_lzcnt_epi32(src: __m256i, k: __mmask8, a: __m256i) -> __m256i;
        fn _mm256_maskz_lzcnt_epi32(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_lzcnt_epi32(a: __m128i) -> __m128i;
        fn _mm_mask_lzcnt_epi32(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_lzcnt_epi32(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_lzcnt_epi64(a: __m256i) -> __m256i;
        fn _mm256_mask_lzcnt_epi64(src: __m256i, k: __mmask8, a: __m256i) -> __m256i;
        fn _mm256_maskz_lzcnt_epi64(k: __mmask8, a: __m256i) -> __m256i;
        fn _mm_lzcnt_epi64(a: __m128i) -> __m128i;
        fn _mm_mask_lzcnt_epi64(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_lzcnt_epi64(k: __mmask8, a: __m128i) -> __m128i;
    }
}
