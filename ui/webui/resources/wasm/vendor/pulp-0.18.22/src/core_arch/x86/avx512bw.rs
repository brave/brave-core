use super::*;

impl Avx512bw {
    delegate! {
        fn _mm512_abs_epi16(a: __m512i) -> __m512i;
        fn _mm512_mask_abs_epi16(src: __m512i, k: __mmask32, a: __m512i) -> __m512i;
        fn _mm512_maskz_abs_epi16(k: __mmask32, a: __m512i) -> __m512i;
        fn _mm512_abs_epi8(a: __m512i) -> __m512i;
        fn _mm512_mask_abs_epi8(src: __m512i, k: __mmask64, a: __m512i) -> __m512i;
        fn _mm512_maskz_abs_epi8(k: __mmask64, a: __m512i) -> __m512i;
        fn _mm512_add_epi16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_add_epi16(src: __m512i, k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_add_epi16(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_add_epi8(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_add_epi8(src: __m512i, k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_add_epi8(k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_adds_epu16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_adds_epu16(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_adds_epu16(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_adds_epu8(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_adds_epu8(src: __m512i, k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_adds_epu8(k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_adds_epi16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_adds_epi16(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_adds_epi16(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_adds_epi8(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_adds_epi8(src: __m512i, k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_adds_epi8(k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_sub_epi16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_sub_epi16(src: __m512i, k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_sub_epi16(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_sub_epi8(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_sub_epi8(src: __m512i, k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_sub_epi8(k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_subs_epu16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_subs_epu16(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_subs_epu16(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_subs_epu8(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_subs_epu8(src: __m512i, k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_subs_epu8(k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_subs_epi16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_subs_epi16(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_subs_epi16(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_subs_epi8(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_subs_epi8(src: __m512i, k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_subs_epi8(k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mulhi_epu16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_mulhi_epu16(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_mulhi_epu16(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mulhi_epi16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_mulhi_epi16(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_mulhi_epi16(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mulhrs_epi16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_mulhrs_epi16(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_mulhrs_epi16(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mullo_epi16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_mullo_epi16(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_mullo_epi16(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_max_epu16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_max_epu16(src: __m512i, k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_max_epu16(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_max_epu8(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_max_epu8(src: __m512i, k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_max_epu8(k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_max_epi16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_max_epi16(src: __m512i, k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_max_epi16(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_max_epi8(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_max_epi8(src: __m512i, k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_max_epi8(k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_min_epu16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_min_epu16(src: __m512i, k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_min_epu16(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_min_epu8(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_min_epu8(src: __m512i, k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_min_epu8(k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_min_epi16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_min_epi16(src: __m512i, k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_min_epi16(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_min_epi8(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_min_epi8(src: __m512i, k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_min_epi8(k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_cmplt_epu16_mask(a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_mask_cmplt_epu16_mask(k1: __mmask32, a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_cmplt_epu8_mask(a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_mask_cmplt_epu8_mask(k1: __mmask64, a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_cmplt_epi16_mask(a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_mask_cmplt_epi16_mask(k1: __mmask32, a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_cmplt_epi8_mask(a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_mask_cmplt_epi8_mask(k1: __mmask64, a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_cmpgt_epu16_mask(a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_mask_cmpgt_epu16_mask(k1: __mmask32, a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_cmpgt_epu8_mask(a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_mask_cmpgt_epu8_mask(k1: __mmask64, a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_cmpgt_epi16_mask(a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_mask_cmpgt_epi16_mask(k1: __mmask32, a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_cmpgt_epi8_mask(a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_mask_cmpgt_epi8_mask(k1: __mmask64, a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_cmple_epu16_mask(a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_mask_cmple_epu16_mask(k1: __mmask32, a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_cmple_epu8_mask(a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_mask_cmple_epu8_mask(k1: __mmask64, a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_cmple_epi16_mask(a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_mask_cmple_epi16_mask(k1: __mmask32, a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_cmple_epi8_mask(a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_mask_cmple_epi8_mask(k1: __mmask64, a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_cmpge_epu16_mask(a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_mask_cmpge_epu16_mask(k1: __mmask32, a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_cmpge_epu8_mask(a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_mask_cmpge_epu8_mask(k1: __mmask64, a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_cmpge_epi16_mask(a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_mask_cmpge_epi16_mask(k1: __mmask32, a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_cmpge_epi8_mask(a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_mask_cmpge_epi8_mask(k1: __mmask64, a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_cmpeq_epu16_mask(a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_mask_cmpeq_epu16_mask(k1: __mmask32, a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_cmpeq_epu8_mask(a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_mask_cmpeq_epu8_mask(k1: __mmask64, a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_cmpeq_epi16_mask(a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_mask_cmpeq_epi16_mask(k1: __mmask32, a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_cmpeq_epi8_mask(a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_mask_cmpeq_epi8_mask(k1: __mmask64, a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_cmpneq_epu16_mask(a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_mask_cmpneq_epu16_mask(k1: __mmask32, a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_cmpneq_epu8_mask(a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_mask_cmpneq_epu8_mask(k1: __mmask64, a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_cmpneq_epi16_mask(a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_mask_cmpneq_epi16_mask(k1: __mmask32, a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_cmpneq_epi8_mask(a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_mask_cmpneq_epi8_mask(k1: __mmask64, a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_cmp_epu16_mask<const IMM8: i32>(a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_mask_cmp_epu16_mask<const IMM8: i32>(
            k1: __mmask32,
            a: __m512i,
            b: __m512i,
        ) -> __mmask32;
        fn _mm512_cmp_epu8_mask<const IMM8: i32>(a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_mask_cmp_epu8_mask<const IMM8: i32>(
            k1: __mmask64,
            a: __m512i,
            b: __m512i,
        ) -> __mmask64;
        fn _mm512_cmp_epi16_mask<const IMM8: i32>(a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_mask_cmp_epi16_mask<const IMM8: i32>(
            k1: __mmask32,
            a: __m512i,
            b: __m512i,
        ) -> __mmask32;
        fn _mm512_cmp_epi8_mask<const IMM8: i32>(a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_mask_cmp_epi8_mask<const IMM8: i32>(
            k1: __mmask64,
            a: __m512i,
            b: __m512i,
        ) -> __mmask64;
        unsafe fn _mm512_loadu_epi16(mem_addr: *const i16) -> __m512i;
        unsafe fn _mm512_loadu_epi8(mem_addr: *const i8) -> __m512i;
        unsafe fn _mm512_storeu_epi16(mem_addr: *mut i16, a: __m512i);
        unsafe fn _mm512_storeu_epi8(mem_addr: *mut i8, a: __m512i);
        unsafe fn _mm512_mask_loadu_epi16(src: __m512i, k: __mmask32, mem_addr: *const i16) -> __m512i;
        unsafe fn _mm512_maskz_loadu_epi16(k: __mmask32, mem_addr: *const i16) -> __m512i;
        unsafe fn _mm512_mask_loadu_epi8(src: __m512i, k: __mmask64, mem_addr: *const i8) -> __m512i;
        unsafe fn _mm512_maskz_loadu_epi8(k: __mmask64, mem_addr: *const i8) -> __m512i;
        unsafe fn _mm256_mask_loadu_epi16(src: __m256i, k: __mmask16, mem_addr: *const i16) -> __m256i;
        unsafe fn _mm256_maskz_loadu_epi16(k: __mmask16, mem_addr: *const i16) -> __m256i;
        unsafe fn _mm256_mask_loadu_epi8(src: __m256i, k: __mmask32, mem_addr: *const i8) -> __m256i;
        unsafe fn _mm256_maskz_loadu_epi8(k: __mmask32, mem_addr: *const i8) -> __m256i;
        unsafe fn _mm_mask_loadu_epi16(src: __m128i, k: __mmask8, mem_addr: *const i16) -> __m128i;
        unsafe fn _mm_maskz_loadu_epi16(k: __mmask8, mem_addr: *const i16) -> __m128i;
        unsafe fn _mm_mask_loadu_epi8(src: __m128i, k: __mmask16, mem_addr: *const i8) -> __m128i;
        unsafe fn _mm_maskz_loadu_epi8(k: __mmask16, mem_addr: *const i8) -> __m128i;
        unsafe fn _mm512_mask_storeu_epi16(mem_addr: *mut i16, mask: __mmask32, a: __m512i);
        unsafe fn _mm512_mask_storeu_epi8(mem_addr: *mut i8, mask: __mmask64, a: __m512i);
        unsafe fn _mm256_mask_storeu_epi16(mem_addr: *mut i16, mask: __mmask16, a: __m256i);
        unsafe fn _mm256_mask_storeu_epi8(mem_addr: *mut i8, mask: __mmask32, a: __m256i);
        unsafe fn _mm_mask_storeu_epi16(mem_addr: *mut i16, mask: __mmask8, a: __m128i);
        unsafe fn _mm_mask_storeu_epi8(mem_addr: *mut i8, mask: __mmask16, a: __m128i);
        fn _mm512_madd_epi16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_madd_epi16(
            src: __m512i,
            k: __mmask16,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_madd_epi16(k: __mmask16, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maddubs_epi16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_maddubs_epi16(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_maddubs_epi16(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_packs_epi32(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_packs_epi32(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_packs_epi32(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_packs_epi16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_packs_epi16(
            src: __m512i,
            k: __mmask64,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_packs_epi16(k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_packus_epi32(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_packus_epi32(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_packus_epi32(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_packus_epi16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_packus_epi16(
            src: __m512i,
            k: __mmask64,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_packus_epi16(k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_avg_epu16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_avg_epu16(src: __m512i, k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_avg_epu16(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_avg_epu8(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_avg_epu8(src: __m512i, k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_maskz_avg_epu8(k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_sll_epi16(a: __m512i, count: __m128i) -> __m512i;
        fn _mm512_mask_sll_epi16(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            count: __m128i,
        ) -> __m512i;
        fn _mm512_maskz_sll_epi16(k: __mmask32, a: __m512i, count: __m128i) -> __m512i;
        fn _mm512_slli_epi16<const IMM8: u32>(a: __m512i) -> __m512i;
        fn _mm512_mask_slli_epi16<const IMM8: u32>(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_slli_epi16<const IMM8: u32>(k: __mmask32, a: __m512i) -> __m512i;
        fn _mm512_sllv_epi16(a: __m512i, count: __m512i) -> __m512i;
        fn _mm512_mask_sllv_epi16(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            count: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_sllv_epi16(k: __mmask32, a: __m512i, count: __m512i) -> __m512i;
        fn _mm512_srl_epi16(a: __m512i, count: __m128i) -> __m512i;
        fn _mm512_mask_srl_epi16(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            count: __m128i,
        ) -> __m512i;
        fn _mm512_maskz_srl_epi16(k: __mmask32, a: __m512i, count: __m128i) -> __m512i;
        fn _mm512_srli_epi16<const IMM8: u32>(a: __m512i) -> __m512i;
        fn _mm512_mask_srli_epi16<const IMM8: u32>(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_srli_epi16<const IMM8: i32>(k: __mmask32, a: __m512i) -> __m512i;
        fn _mm512_srlv_epi16(a: __m512i, count: __m512i) -> __m512i;
        fn _mm512_mask_srlv_epi16(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            count: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_srlv_epi16(k: __mmask32, a: __m512i, count: __m512i) -> __m512i;
        fn _mm512_sra_epi16(a: __m512i, count: __m128i) -> __m512i;
        fn _mm512_mask_sra_epi16(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            count: __m128i,
        ) -> __m512i;
        fn _mm512_maskz_sra_epi16(k: __mmask32, a: __m512i, count: __m128i) -> __m512i;
        fn _mm512_srai_epi16<const IMM8: u32>(a: __m512i) -> __m512i;
        fn _mm512_mask_srai_epi16<const IMM8: u32>(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_srai_epi16<const IMM8: u32>(k: __mmask32, a: __m512i) -> __m512i;
        fn _mm512_srav_epi16(a: __m512i, count: __m512i) -> __m512i;
        fn _mm512_mask_srav_epi16(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            count: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_srav_epi16(k: __mmask32, a: __m512i, count: __m512i) -> __m512i;
        fn _mm512_permutex2var_epi16(a: __m512i, idx: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_permutex2var_epi16(
            a: __m512i,
            k: __mmask32,
            idx: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_permutex2var_epi16(
            k: __mmask32,
            a: __m512i,
            idx: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_mask2_permutex2var_epi16(
            a: __m512i,
            idx: __m512i,
            k: __mmask32,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_permutexvar_epi16(idx: __m512i, a: __m512i) -> __m512i;
        fn _mm512_mask_permutexvar_epi16(
            src: __m512i,
            k: __mmask32,
            idx: __m512i,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_permutexvar_epi16(k: __mmask32, idx: __m512i, a: __m512i) -> __m512i;
        fn _mm512_mask_blend_epi16(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_blend_epi8(k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_broadcastw_epi16(a: __m128i) -> __m512i;
        fn _mm512_mask_broadcastw_epi16(src: __m512i, k: __mmask32, a: __m128i) -> __m512i;
        fn _mm512_maskz_broadcastw_epi16(k: __mmask32, a: __m128i) -> __m512i;
        fn _mm512_broadcastb_epi8(a: __m128i) -> __m512i;
        fn _mm512_mask_broadcastb_epi8(src: __m512i, k: __mmask64, a: __m128i) -> __m512i;
        fn _mm512_maskz_broadcastb_epi8(k: __mmask64, a: __m128i) -> __m512i;
        fn _mm512_unpackhi_epi16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_unpackhi_epi16(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_unpackhi_epi16(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_unpackhi_epi8(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_unpackhi_epi8(
            src: __m512i,
            k: __mmask64,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_unpackhi_epi8(k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_unpacklo_epi16(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_unpacklo_epi16(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_unpacklo_epi16(k: __mmask32, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_unpacklo_epi8(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_unpacklo_epi8(
            src: __m512i,
            k: __mmask64,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_unpacklo_epi8(k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_mov_epi16(src: __m512i, k: __mmask32, a: __m512i) -> __m512i;
        fn _mm512_maskz_mov_epi16(k: __mmask32, a: __m512i) -> __m512i;
        fn _mm512_mask_mov_epi8(src: __m512i, k: __mmask64, a: __m512i) -> __m512i;
        fn _mm512_maskz_mov_epi8(k: __mmask64, a: __m512i) -> __m512i;
        fn _mm512_mask_set1_epi16(src: __m512i, k: __mmask32, a: i16) -> __m512i;
        fn _mm512_maskz_set1_epi16(k: __mmask32, a: i16) -> __m512i;
        fn _mm512_mask_set1_epi8(src: __m512i, k: __mmask64, a: i8) -> __m512i;
        fn _mm512_maskz_set1_epi8(k: __mmask64, a: i8) -> __m512i;
        fn _mm512_shufflelo_epi16<const IMM8: i32>(a: __m512i) -> __m512i;
        fn _mm512_mask_shufflelo_epi16<const IMM8: i32>(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_shufflelo_epi16<const IMM8: i32>(k: __mmask32, a: __m512i) -> __m512i;
        fn _mm512_shufflehi_epi16<const IMM8: i32>(a: __m512i) -> __m512i;
        fn _mm512_mask_shufflehi_epi16<const IMM8: i32>(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_shufflehi_epi16<const IMM8: i32>(k: __mmask32, a: __m512i) -> __m512i;
        fn _mm512_shuffle_epi8(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_shuffle_epi8(
            src: __m512i,
            k: __mmask64,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_shuffle_epi8(k: __mmask64, a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_test_epi16_mask(a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_mask_test_epi16_mask(k: __mmask32, a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_test_epi8_mask(a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_mask_test_epi8_mask(k: __mmask64, a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_testn_epi16_mask(a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_mask_testn_epi16_mask(k: __mmask32, a: __m512i, b: __m512i) -> __mmask32;
        fn _mm512_testn_epi8_mask(a: __m512i, b: __m512i) -> __mmask64;
        fn _mm512_mask_testn_epi8_mask(k: __mmask64, a: __m512i, b: __m512i) -> __mmask64;
        unsafe fn _store_mask64(mem_addr: *mut u64, a: __mmask64);
        unsafe fn _store_mask32(mem_addr: *mut u32, a: __mmask32);
        unsafe fn _load_mask64(mem_addr: *const u64) -> __mmask64;
        unsafe fn _load_mask32(mem_addr: *const u32) -> __mmask32;
        fn _mm512_sad_epu8(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_dbsad_epu8<const IMM8: i32>(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_dbsad_epu8<const IMM8: i32>(
            src: __m512i,
            k: __mmask32,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_dbsad_epu8<const IMM8: i32>(
            k: __mmask32,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_movepi16_mask(a: __m512i) -> __mmask32;
        fn _mm512_movepi8_mask(a: __m512i) -> __mmask64;
        fn _mm512_movm_epi16(k: __mmask32) -> __m512i;
        fn _mm512_movm_epi8(k: __mmask64) -> __m512i;
        fn _kadd_mask32(a: __mmask32, b: __mmask32) -> __mmask32;
        fn _kadd_mask64(a: __mmask64, b: __mmask64) -> __mmask64;
        fn _kand_mask32(a: __mmask32, b: __mmask32) -> __mmask32;
        fn _kand_mask64(a: __mmask64, b: __mmask64) -> __mmask64;
        fn _knot_mask32(a: __mmask32) -> __mmask32;
        fn _knot_mask64(a: __mmask64) -> __mmask64;
        fn _kandn_mask32(a: __mmask32, b: __mmask32) -> __mmask32;
        fn _kandn_mask64(a: __mmask64, b: __mmask64) -> __mmask64;
        fn _kor_mask32(a: __mmask32, b: __mmask32) -> __mmask32;
        fn _kor_mask64(a: __mmask64, b: __mmask64) -> __mmask64;
        fn _kxor_mask32(a: __mmask32, b: __mmask32) -> __mmask32;
        fn _kxor_mask64(a: __mmask64, b: __mmask64) -> __mmask64;
        fn _kxnor_mask32(a: __mmask32, b: __mmask32) -> __mmask32;
        fn _kxnor_mask64(a: __mmask64, b: __mmask64) -> __mmask64;
        fn _mm512_cvtepi16_epi8(a: __m512i) -> __m256i;
        fn _mm512_mask_cvtepi16_epi8(src: __m256i, k: __mmask32, a: __m512i) -> __m256i;
        fn _mm512_maskz_cvtepi16_epi8(k: __mmask32, a: __m512i) -> __m256i;
        fn _mm512_cvtsepi16_epi8(a: __m512i) -> __m256i;
        fn _mm512_mask_cvtsepi16_epi8(src: __m256i, k: __mmask32, a: __m512i) -> __m256i;
        fn _mm512_maskz_cvtsepi16_epi8(k: __mmask32, a: __m512i) -> __m256i;
        fn _mm512_cvtusepi16_epi8(a: __m512i) -> __m256i;
        fn _mm512_mask_cvtusepi16_epi8(src: __m256i, k: __mmask32, a: __m512i) -> __m256i;
        fn _mm512_maskz_cvtusepi16_epi8(k: __mmask32, a: __m512i) -> __m256i;
        fn _mm512_cvtepi8_epi16(a: __m256i) -> __m512i;
        fn _mm512_mask_cvtepi8_epi16(src: __m512i, k: __mmask32, a: __m256i) -> __m512i;
        fn _mm512_maskz_cvtepi8_epi16(k: __mmask32, a: __m256i) -> __m512i;
        fn _mm512_cvtepu8_epi16(a: __m256i) -> __m512i;
        fn _mm512_mask_cvtepu8_epi16(src: __m512i, k: __mmask32, a: __m256i) -> __m512i;
        fn _mm512_maskz_cvtepu8_epi16(k: __mmask32, a: __m256i) -> __m512i;
        fn _mm512_bslli_epi128<const IMM8: i32>(a: __m512i) -> __m512i;
        fn _mm512_bsrli_epi128<const IMM8: i32>(a: __m512i) -> __m512i;
        fn _mm512_alignr_epi8<const IMM8: i32>(a: __m512i, b: __m512i) -> __m512i;
        fn _mm512_mask_alignr_epi8<const IMM8: i32>(
            src: __m512i,
            k: __mmask64,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        fn _mm512_maskz_alignr_epi8<const IMM8: i32>(
            k: __mmask64,
            a: __m512i,
            b: __m512i,
        ) -> __m512i;
        unsafe fn _mm512_mask_cvtsepi16_storeu_epi8(mem_addr: *mut i8, k: __mmask32, a: __m512i);
        unsafe fn _mm512_mask_cvtepi16_storeu_epi8(mem_addr: *mut i8, k: __mmask32, a: __m512i);
        unsafe fn _mm512_mask_cvtusepi16_storeu_epi8(mem_addr: *mut i8, k: __mmask32, a: __m512i);
    }
}

impl Avx512bw_Avx512vl {
    delegate! {
        fn _mm256_mask_abs_epi16(src: __m256i, k: __mmask16, a: __m256i) -> __m256i;
        fn _mm256_maskz_abs_epi16(k: __mmask16, a: __m256i) -> __m256i;
        fn _mm_mask_abs_epi16(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_abs_epi16(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_abs_epi8(src: __m256i, k: __mmask32, a: __m256i) -> __m256i;
        fn _mm256_maskz_abs_epi8(k: __mmask32, a: __m256i) -> __m256i;
        fn _mm_mask_abs_epi8(src: __m128i, k: __mmask16, a: __m128i) -> __m128i;
        fn _mm_maskz_abs_epi8(k: __mmask16, a: __m128i) -> __m128i;
        fn _mm256_mask_add_epi16(src: __m256i, k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_add_epi16(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_add_epi16(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_add_epi16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_add_epi8(src: __m256i, k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_add_epi8(k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_add_epi8(src: __m128i, k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_add_epi8(k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_adds_epu16(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_adds_epu16(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_adds_epu16(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_adds_epu16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_adds_epu8(src: __m256i, k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_adds_epu8(k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_adds_epu8(src: __m128i, k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_adds_epu8(k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_adds_epi16(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_adds_epi16(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_adds_epi16(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_adds_epi16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_adds_epi8(src: __m256i, k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_adds_epi8(k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_adds_epi8(src: __m128i, k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_adds_epi8(k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_sub_epi16(src: __m256i, k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_sub_epi16(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_sub_epi16(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_sub_epi16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_sub_epi8(src: __m256i, k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_sub_epi8(k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_sub_epi8(src: __m128i, k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_sub_epi8(k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_subs_epu16(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_subs_epu16(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_subs_epu16(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_subs_epu16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_subs_epu8(src: __m256i, k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_subs_epu8(k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_subs_epu8(src: __m128i, k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_subs_epu8(k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_subs_epi16(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_subs_epi16(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_subs_epi16(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_subs_epi16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_subs_epi8(src: __m256i, k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_subs_epi8(k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_subs_epi8(src: __m128i, k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_subs_epi8(k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_mulhi_epu16(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_mulhi_epu16(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_mulhi_epu16(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_mulhi_epu16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_mulhi_epi16(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_mulhi_epi16(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_mulhi_epi16(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_mulhi_epi16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_mulhrs_epi16(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_mulhrs_epi16(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_mulhrs_epi16(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_mulhrs_epi16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_mullo_epi16(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_mullo_epi16(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_mullo_epi16(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_mullo_epi16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_max_epu16(src: __m256i, k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_max_epu16(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_max_epu16(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_max_epu16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_max_epu8(src: __m256i, k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_max_epu8(k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_max_epu8(src: __m128i, k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_max_epu8(k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_max_epi16(src: __m256i, k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_max_epi16(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_max_epi16(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_max_epi16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_max_epi8(src: __m256i, k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_max_epi8(k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_max_epi8(src: __m128i, k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_max_epi8(k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_min_epu16(src: __m256i, k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_min_epu16(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_min_epu16(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_min_epu16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_min_epu8(src: __m256i, k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_min_epu8(k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_min_epu8(src: __m128i, k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_min_epu8(k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_min_epi16(src: __m256i, k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_min_epi16(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_min_epi16(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_min_epi16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_min_epi8(src: __m256i, k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_min_epi8(k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_min_epi8(src: __m128i, k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_min_epi8(k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_cmplt_epu16_mask(a: __m256i, b: __m256i) -> __mmask16;
        fn _mm256_mask_cmplt_epu16_mask(k1: __mmask16, a: __m256i, b: __m256i) -> __mmask16;
        fn _mm_cmplt_epu16_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmplt_epu16_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmplt_epu8_mask(a: __m256i, b: __m256i) -> __mmask32;
        fn _mm256_mask_cmplt_epu8_mask(k1: __mmask32, a: __m256i, b: __m256i) -> __mmask32;
        fn _mm_cmplt_epu8_mask(a: __m128i, b: __m128i) -> __mmask16;
        fn _mm_mask_cmplt_epu8_mask(k1: __mmask16, a: __m128i, b: __m128i) -> __mmask16;
        fn _mm256_cmplt_epi16_mask(a: __m256i, b: __m256i) -> __mmask16;
        fn _mm256_mask_cmplt_epi16_mask(k1: __mmask16, a: __m256i, b: __m256i) -> __mmask16;
        fn _mm_cmplt_epi16_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmplt_epi16_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmplt_epi8_mask(a: __m256i, b: __m256i) -> __mmask32;
        fn _mm256_mask_cmplt_epi8_mask(k1: __mmask32, a: __m256i, b: __m256i) -> __mmask32;
        fn _mm_cmplt_epi8_mask(a: __m128i, b: __m128i) -> __mmask16;
        fn _mm_mask_cmplt_epi8_mask(k1: __mmask16, a: __m128i, b: __m128i) -> __mmask16;
        fn _mm256_cmpgt_epu16_mask(a: __m256i, b: __m256i) -> __mmask16;
        fn _mm256_mask_cmpgt_epu16_mask(k1: __mmask16, a: __m256i, b: __m256i) -> __mmask16;
        fn _mm_cmpgt_epu16_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpgt_epu16_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpgt_epu8_mask(a: __m256i, b: __m256i) -> __mmask32;
        fn _mm256_mask_cmpgt_epu8_mask(k1: __mmask32, a: __m256i, b: __m256i) -> __mmask32;
        fn _mm_cmpgt_epu8_mask(a: __m128i, b: __m128i) -> __mmask16;
        fn _mm_mask_cmpgt_epu8_mask(k1: __mmask16, a: __m128i, b: __m128i) -> __mmask16;
        fn _mm256_cmpgt_epi16_mask(a: __m256i, b: __m256i) -> __mmask16;
        fn _mm256_mask_cmpgt_epi16_mask(k1: __mmask16, a: __m256i, b: __m256i) -> __mmask16;
        fn _mm_cmpgt_epi16_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpgt_epi16_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpgt_epi8_mask(a: __m256i, b: __m256i) -> __mmask32;
        fn _mm256_mask_cmpgt_epi8_mask(k1: __mmask32, a: __m256i, b: __m256i) -> __mmask32;
        fn _mm_cmpgt_epi8_mask(a: __m128i, b: __m128i) -> __mmask16;
        fn _mm_mask_cmpgt_epi8_mask(k1: __mmask16, a: __m128i, b: __m128i) -> __mmask16;
        fn _mm256_cmple_epu16_mask(a: __m256i, b: __m256i) -> __mmask16;
        fn _mm256_mask_cmple_epu16_mask(k1: __mmask16, a: __m256i, b: __m256i) -> __mmask16;
        fn _mm_cmple_epu16_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmple_epu16_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmple_epu8_mask(a: __m256i, b: __m256i) -> __mmask32;
        fn _mm256_mask_cmple_epu8_mask(k1: __mmask32, a: __m256i, b: __m256i) -> __mmask32;
        fn _mm_cmple_epu8_mask(a: __m128i, b: __m128i) -> __mmask16;
        fn _mm_mask_cmple_epu8_mask(k1: __mmask16, a: __m128i, b: __m128i) -> __mmask16;
        fn _mm256_cmple_epi16_mask(a: __m256i, b: __m256i) -> __mmask16;
        fn _mm256_mask_cmple_epi16_mask(k1: __mmask16, a: __m256i, b: __m256i) -> __mmask16;
        fn _mm_cmple_epi16_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmple_epi16_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmple_epi8_mask(a: __m256i, b: __m256i) -> __mmask32;
        fn _mm256_mask_cmple_epi8_mask(k1: __mmask32, a: __m256i, b: __m256i) -> __mmask32;
        fn _mm_cmple_epi8_mask(a: __m128i, b: __m128i) -> __mmask16;
        fn _mm_mask_cmple_epi8_mask(k1: __mmask16, a: __m128i, b: __m128i) -> __mmask16;
        fn _mm256_cmpge_epu16_mask(a: __m256i, b: __m256i) -> __mmask16;
        fn _mm256_mask_cmpge_epu16_mask(k1: __mmask16, a: __m256i, b: __m256i) -> __mmask16;
        fn _mm_cmpge_epu16_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpge_epu16_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpge_epu8_mask(a: __m256i, b: __m256i) -> __mmask32;
        fn _mm256_mask_cmpge_epu8_mask(k1: __mmask32, a: __m256i, b: __m256i) -> __mmask32;
        fn _mm_cmpge_epu8_mask(a: __m128i, b: __m128i) -> __mmask16;
        fn _mm_mask_cmpge_epu8_mask(k1: __mmask16, a: __m128i, b: __m128i) -> __mmask16;
        fn _mm256_cmpge_epi16_mask(a: __m256i, b: __m256i) -> __mmask16;
        fn _mm256_mask_cmpge_epi16_mask(k1: __mmask16, a: __m256i, b: __m256i) -> __mmask16;
        fn _mm_cmpge_epi16_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpge_epi16_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpge_epi8_mask(a: __m256i, b: __m256i) -> __mmask32;
        fn _mm256_mask_cmpge_epi8_mask(k1: __mmask32, a: __m256i, b: __m256i) -> __mmask32;
        fn _mm_cmpge_epi8_mask(a: __m128i, b: __m128i) -> __mmask16;
        fn _mm_mask_cmpge_epi8_mask(k1: __mmask16, a: __m128i, b: __m128i) -> __mmask16;
        fn _mm256_cmpeq_epu16_mask(a: __m256i, b: __m256i) -> __mmask16;
        fn _mm256_mask_cmpeq_epu16_mask(k1: __mmask16, a: __m256i, b: __m256i) -> __mmask16;
        fn _mm_cmpeq_epu16_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpeq_epu16_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpeq_epu8_mask(a: __m256i, b: __m256i) -> __mmask32;
        fn _mm256_mask_cmpeq_epu8_mask(k1: __mmask32, a: __m256i, b: __m256i) -> __mmask32;
        fn _mm_cmpeq_epu8_mask(a: __m128i, b: __m128i) -> __mmask16;
        fn _mm_mask_cmpeq_epu8_mask(k1: __mmask16, a: __m128i, b: __m128i) -> __mmask16;
        fn _mm256_cmpeq_epi16_mask(a: __m256i, b: __m256i) -> __mmask16;
        fn _mm256_mask_cmpeq_epi16_mask(k1: __mmask16, a: __m256i, b: __m256i) -> __mmask16;
        fn _mm_cmpeq_epi16_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpeq_epi16_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpeq_epi8_mask(a: __m256i, b: __m256i) -> __mmask32;
        fn _mm256_mask_cmpeq_epi8_mask(k1: __mmask32, a: __m256i, b: __m256i) -> __mmask32;
        fn _mm_cmpeq_epi8_mask(a: __m128i, b: __m128i) -> __mmask16;
        fn _mm_mask_cmpeq_epi8_mask(k1: __mmask16, a: __m128i, b: __m128i) -> __mmask16;
        fn _mm256_cmpneq_epu16_mask(a: __m256i, b: __m256i) -> __mmask16;
        fn _mm256_mask_cmpneq_epu16_mask(k1: __mmask16, a: __m256i, b: __m256i) -> __mmask16;
        fn _mm_cmpneq_epu16_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpneq_epu16_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpneq_epu8_mask(a: __m256i, b: __m256i) -> __mmask32;
        fn _mm256_mask_cmpneq_epu8_mask(k1: __mmask32, a: __m256i, b: __m256i) -> __mmask32;
        fn _mm_cmpneq_epu8_mask(a: __m128i, b: __m128i) -> __mmask16;
        fn _mm_mask_cmpneq_epu8_mask(k1: __mmask16, a: __m128i, b: __m128i) -> __mmask16;
        fn _mm256_cmpneq_epi16_mask(a: __m256i, b: __m256i) -> __mmask16;
        fn _mm256_mask_cmpneq_epi16_mask(k1: __mmask16, a: __m256i, b: __m256i) -> __mmask16;
        fn _mm_cmpneq_epi16_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmpneq_epi16_mask(k1: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_cmpneq_epi8_mask(a: __m256i, b: __m256i) -> __mmask32;
        fn _mm256_mask_cmpneq_epi8_mask(k1: __mmask32, a: __m256i, b: __m256i) -> __mmask32;
        fn _mm_cmpneq_epi8_mask(a: __m128i, b: __m128i) -> __mmask16;
        fn _mm_mask_cmpneq_epi8_mask(k1: __mmask16, a: __m128i, b: __m128i) -> __mmask16;
        fn _mm256_cmp_epu16_mask<const IMM8: i32>(a: __m256i, b: __m256i) -> __mmask16;
        fn _mm256_mask_cmp_epu16_mask<const IMM8: i32>(
            k1: __mmask16,
            a: __m256i,
            b: __m256i,
        ) -> __mmask16;
        fn _mm_cmp_epu16_mask<const IMM8: i32>(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmp_epu16_mask<const IMM8: i32>(
            k1: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __mmask8;
        fn _mm256_cmp_epu8_mask<const IMM8: i32>(a: __m256i, b: __m256i) -> __mmask32;
        fn _mm256_mask_cmp_epu8_mask<const IMM8: i32>(
            k1: __mmask32,
            a: __m256i,
            b: __m256i,
        ) -> __mmask32;
        fn _mm_cmp_epu8_mask<const IMM8: i32>(a: __m128i, b: __m128i) -> __mmask16;
        fn _mm_mask_cmp_epu8_mask<const IMM8: i32>(
            k1: __mmask16,
            a: __m128i,
            b: __m128i,
        ) -> __mmask16;
        fn _mm256_cmp_epi16_mask<const IMM8: i32>(a: __m256i, b: __m256i) -> __mmask16;
        fn _mm256_mask_cmp_epi16_mask<const IMM8: i32>(
            k1: __mmask16,
            a: __m256i,
            b: __m256i,
        ) -> __mmask16;
        fn _mm_cmp_epi16_mask<const IMM8: i32>(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_cmp_epi16_mask<const IMM8: i32>(
            k1: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __mmask8;
        fn _mm256_cmp_epi8_mask<const IMM8: i32>(a: __m256i, b: __m256i) -> __mmask32;
        fn _mm256_mask_cmp_epi8_mask<const IMM8: i32>(
            k1: __mmask32,
            a: __m256i,
            b: __m256i,
        ) -> __mmask32;
        fn _mm_cmp_epi8_mask<const IMM8: i32>(a: __m128i, b: __m128i) -> __mmask16;
        fn _mm_mask_cmp_epi8_mask<const IMM8: i32>(
            k1: __mmask16,
            a: __m128i,
            b: __m128i,
        ) -> __mmask16;
        unsafe fn _mm256_loadu_epi16(mem_addr: *const i16) -> __m256i;
        unsafe fn _mm_loadu_epi16(mem_addr: *const i16) -> __m128i;
        unsafe fn _mm256_loadu_epi8(mem_addr: *const i8) -> __m256i;
        unsafe fn _mm_loadu_epi8(mem_addr: *const i8) -> __m128i;
        unsafe fn _mm256_storeu_epi16(mem_addr: *mut i16, a: __m256i);
        unsafe fn _mm_storeu_epi16(mem_addr: *mut i16, a: __m128i);
        unsafe fn _mm256_storeu_epi8(mem_addr: *mut i8, a: __m256i);
        unsafe fn _mm_storeu_epi8(mem_addr: *mut i8, a: __m128i);
        fn _mm256_mask_madd_epi16(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_madd_epi16(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_madd_epi16(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_madd_epi16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_maddubs_epi16(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_maddubs_epi16(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_maddubs_epi16(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_maddubs_epi16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_packs_epi32(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_packs_epi32(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_packs_epi32(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_packs_epi32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_packs_epi16(
            src: __m256i,
            k: __mmask32,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_packs_epi16(k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_packs_epi16(src: __m128i, k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_packs_epi16(k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_packus_epi32(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_packus_epi32(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_packus_epi32(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_packus_epi32(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_packus_epi16(
            src: __m256i,
            k: __mmask32,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_packus_epi16(k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_packus_epi16(src: __m128i, k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_packus_epi16(k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_avg_epu16(src: __m256i, k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_avg_epu16(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_avg_epu16(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_avg_epu16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_avg_epu8(src: __m256i, k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_maskz_avg_epu8(k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_avg_epu8(src: __m128i, k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_avg_epu8(k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_sll_epi16(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            count: __m128i,
        ) -> __m256i;
        fn _mm256_maskz_sll_epi16(k: __mmask16, a: __m256i, count: __m128i) -> __m256i;
        fn _mm_mask_sll_epi16(src: __m128i, k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm_maskz_sll_epi16(k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_mask_slli_epi16<const IMM8: u32>(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_slli_epi16<const IMM8: u32>(k: __mmask16, a: __m256i) -> __m256i;
        fn _mm_mask_slli_epi16<const IMM8: u32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
        ) -> __m128i;
        fn _mm_maskz_slli_epi16<const IMM8: u32>(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_sllv_epi16(a: __m256i, count: __m256i) -> __m256i;
        fn _mm256_mask_sllv_epi16(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            count: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_sllv_epi16(k: __mmask16, a: __m256i, count: __m256i) -> __m256i;
        fn _mm_sllv_epi16(a: __m128i, count: __m128i) -> __m128i;
        fn _mm_mask_sllv_epi16(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            count: __m128i,
        ) -> __m128i;
        fn _mm_maskz_sllv_epi16(k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_mask_srl_epi16(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            count: __m128i,
        ) -> __m256i;
        fn _mm256_maskz_srl_epi16(k: __mmask16, a: __m256i, count: __m128i) -> __m256i;
        fn _mm_mask_srl_epi16(src: __m128i, k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm_maskz_srl_epi16(k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_mask_srli_epi16<const IMM8: i32>(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_srli_epi16<const IMM8: i32>(k: __mmask16, a: __m256i) -> __m256i;
        fn _mm_mask_srli_epi16<const IMM8: i32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
        ) -> __m128i;
        fn _mm_maskz_srli_epi16<const IMM8: i32>(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_srlv_epi16(a: __m256i, count: __m256i) -> __m256i;
        fn _mm256_mask_srlv_epi16(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            count: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_srlv_epi16(k: __mmask16, a: __m256i, count: __m256i) -> __m256i;
        fn _mm_srlv_epi16(a: __m128i, count: __m128i) -> __m128i;
        fn _mm_mask_srlv_epi16(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            count: __m128i,
        ) -> __m128i;
        fn _mm_maskz_srlv_epi16(k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_mask_sra_epi16(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            count: __m128i,
        ) -> __m256i;
        fn _mm256_maskz_sra_epi16(k: __mmask16, a: __m256i, count: __m128i) -> __m256i;
        fn _mm_mask_sra_epi16(src: __m128i, k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm_maskz_sra_epi16(k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_mask_srai_epi16<const IMM8: u32>(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_srai_epi16<const IMM8: u32>(k: __mmask16, a: __m256i) -> __m256i;
        fn _mm_mask_srai_epi16<const IMM8: u32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
        ) -> __m128i;
        fn _mm_maskz_srai_epi16<const IMM8: u32>(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_srav_epi16(a: __m256i, count: __m256i) -> __m256i;
        fn _mm256_mask_srav_epi16(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            count: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_srav_epi16(k: __mmask16, a: __m256i, count: __m256i) -> __m256i;
        fn _mm_srav_epi16(a: __m128i, count: __m128i) -> __m128i;
        fn _mm_mask_srav_epi16(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            count: __m128i,
        ) -> __m128i;
        fn _mm_maskz_srav_epi16(k: __mmask8, a: __m128i, count: __m128i) -> __m128i;
        fn _mm256_permutex2var_epi16(a: __m256i, idx: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_permutex2var_epi16(
            a: __m256i,
            k: __mmask16,
            idx: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_permutex2var_epi16(
            k: __mmask16,
            a: __m256i,
            idx: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_mask2_permutex2var_epi16(
            a: __m256i,
            idx: __m256i,
            k: __mmask16,
            b: __m256i,
        ) -> __m256i;
        fn _mm_permutex2var_epi16(a: __m128i, idx: __m128i, b: __m128i) -> __m128i;
        fn _mm_mask_permutex2var_epi16(
            a: __m128i,
            k: __mmask8,
            idx: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_maskz_permutex2var_epi16(
            k: __mmask8,
            a: __m128i,
            idx: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_mask2_permutex2var_epi16(
            a: __m128i,
            idx: __m128i,
            k: __mmask8,
            b: __m128i,
        ) -> __m128i;
        fn _mm256_permutexvar_epi16(idx: __m256i, a: __m256i) -> __m256i;
        fn _mm256_mask_permutexvar_epi16(
            src: __m256i,
            k: __mmask16,
            idx: __m256i,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_permutexvar_epi16(k: __mmask16, idx: __m256i, a: __m256i) -> __m256i;
        fn _mm_permutexvar_epi16(idx: __m128i, a: __m128i) -> __m128i;
        fn _mm_mask_permutexvar_epi16(
            src: __m128i,
            k: __mmask8,
            idx: __m128i,
            a: __m128i,
        ) -> __m128i;
        fn _mm_maskz_permutexvar_epi16(k: __mmask8, idx: __m128i, a: __m128i) -> __m128i;
        fn _mm256_mask_blend_epi16(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_blend_epi16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_blend_epi8(k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_blend_epi8(k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_broadcastw_epi16(src: __m256i, k: __mmask16, a: __m128i) -> __m256i;
        fn _mm256_maskz_broadcastw_epi16(k: __mmask16, a: __m128i) -> __m256i;
        fn _mm_mask_broadcastw_epi16(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_broadcastw_epi16(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_broadcastb_epi8(src: __m256i, k: __mmask32, a: __m128i) -> __m256i;
        fn _mm256_maskz_broadcastb_epi8(k: __mmask32, a: __m128i) -> __m256i;
        fn _mm_mask_broadcastb_epi8(src: __m128i, k: __mmask16, a: __m128i) -> __m128i;
        fn _mm_maskz_broadcastb_epi8(k: __mmask16, a: __m128i) -> __m128i;
        fn _mm256_mask_unpackhi_epi16(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_unpackhi_epi16(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_unpackhi_epi16(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_maskz_unpackhi_epi16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_unpackhi_epi8(
            src: __m256i,
            k: __mmask32,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_unpackhi_epi8(k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_unpackhi_epi8(
            src: __m128i,
            k: __mmask16,
            a: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_maskz_unpackhi_epi8(k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_unpacklo_epi16(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_unpacklo_epi16(k: __mmask16, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_unpacklo_epi16(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_maskz_unpacklo_epi16(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_unpacklo_epi8(
            src: __m256i,
            k: __mmask32,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_unpacklo_epi8(k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_unpacklo_epi8(
            src: __m128i,
            k: __mmask16,
            a: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_maskz_unpacklo_epi8(k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_mask_mov_epi16(src: __m256i, k: __mmask16, a: __m256i) -> __m256i;
        fn _mm256_maskz_mov_epi16(k: __mmask16, a: __m256i) -> __m256i;
        fn _mm_mask_mov_epi16(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_mov_epi16(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_mov_epi8(src: __m256i, k: __mmask32, a: __m256i) -> __m256i;
        fn _mm256_maskz_mov_epi8(k: __mmask32, a: __m256i) -> __m256i;
        fn _mm_mask_mov_epi8(src: __m128i, k: __mmask16, a: __m128i) -> __m128i;
        fn _mm_maskz_mov_epi8(k: __mmask16, a: __m128i) -> __m128i;
        fn _mm256_mask_set1_epi16(src: __m256i, k: __mmask16, a: i16) -> __m256i;
        fn _mm256_maskz_set1_epi16(k: __mmask16, a: i16) -> __m256i;
        fn _mm_mask_set1_epi16(src: __m128i, k: __mmask8, a: i16) -> __m128i;
        fn _mm_maskz_set1_epi16(k: __mmask8, a: i16) -> __m128i;
        fn _mm256_mask_set1_epi8(src: __m256i, k: __mmask32, a: i8) -> __m256i;
        fn _mm256_maskz_set1_epi8(k: __mmask32, a: i8) -> __m256i;
        fn _mm_mask_set1_epi8(src: __m128i, k: __mmask16, a: i8) -> __m128i;
        fn _mm_maskz_set1_epi8(k: __mmask16, a: i8) -> __m128i;
        fn _mm256_mask_shufflelo_epi16<const IMM8: i32>(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_shufflelo_epi16<const IMM8: i32>(k: __mmask16, a: __m256i) -> __m256i;
        fn _mm_mask_shufflelo_epi16<const IMM8: i32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
        ) -> __m128i;
        fn _mm_maskz_shufflelo_epi16<const IMM8: i32>(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_shufflehi_epi16<const IMM8: i32>(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_shufflehi_epi16<const IMM8: i32>(k: __mmask16, a: __m256i) -> __m256i;
        fn _mm_mask_shufflehi_epi16<const IMM8: i32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
        ) -> __m128i;
        fn _mm_maskz_shufflehi_epi16<const IMM8: i32>(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_shuffle_epi8(
            src: __m256i,
            k: __mmask32,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_shuffle_epi8(k: __mmask32, a: __m256i, b: __m256i) -> __m256i;
        fn _mm_mask_shuffle_epi8(src: __m128i, k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm_maskz_shuffle_epi8(k: __mmask16, a: __m128i, b: __m128i) -> __m128i;
        fn _mm256_test_epi16_mask(a: __m256i, b: __m256i) -> __mmask16;
        fn _mm256_mask_test_epi16_mask(k: __mmask16, a: __m256i, b: __m256i) -> __mmask16;
        fn _mm_test_epi16_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_test_epi16_mask(k: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_test_epi8_mask(a: __m256i, b: __m256i) -> __mmask32;
        fn _mm256_mask_test_epi8_mask(k: __mmask32, a: __m256i, b: __m256i) -> __mmask32;
        fn _mm_test_epi8_mask(a: __m128i, b: __m128i) -> __mmask16;
        fn _mm_mask_test_epi8_mask(k: __mmask16, a: __m128i, b: __m128i) -> __mmask16;
        fn _mm256_testn_epi16_mask(a: __m256i, b: __m256i) -> __mmask16;
        fn _mm256_mask_testn_epi16_mask(k: __mmask16, a: __m256i, b: __m256i) -> __mmask16;
        fn _mm_testn_epi16_mask(a: __m128i, b: __m128i) -> __mmask8;
        fn _mm_mask_testn_epi16_mask(k: __mmask8, a: __m128i, b: __m128i) -> __mmask8;
        fn _mm256_testn_epi8_mask(a: __m256i, b: __m256i) -> __mmask32;
        fn _mm256_mask_testn_epi8_mask(k: __mmask32, a: __m256i, b: __m256i) -> __mmask32;
        fn _mm_testn_epi8_mask(a: __m128i, b: __m128i) -> __mmask16;
        fn _mm_mask_testn_epi8_mask(k: __mmask16, a: __m128i, b: __m128i) -> __mmask16;
        fn _mm256_dbsad_epu8<const IMM8: i32>(a: __m256i, b: __m256i) -> __m256i;
        fn _mm256_mask_dbsad_epu8<const IMM8: i32>(
            src: __m256i,
            k: __mmask16,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_dbsad_epu8<const IMM8: i32>(
            k: __mmask16,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm_dbsad_epu8<const IMM8: i32>(a: __m128i, b: __m128i) -> __m128i;
        fn _mm_mask_dbsad_epu8<const IMM8: i32>(
            src: __m128i,
            k: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_maskz_dbsad_epu8<const IMM8: i32>(
            k: __mmask8,
            a: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm256_movepi16_mask(a: __m256i) -> __mmask16;
        fn _mm_movepi16_mask(a: __m128i) -> __mmask8;
        fn _mm256_movepi8_mask(a: __m256i) -> __mmask32;
        fn _mm_movepi8_mask(a: __m128i) -> __mmask16;
        fn _mm256_movm_epi16(k: __mmask16) -> __m256i;
        fn _mm_movm_epi16(k: __mmask8) -> __m128i;
        fn _mm256_movm_epi8(k: __mmask32) -> __m256i;
        fn _mm_movm_epi8(k: __mmask16) -> __m128i;
        fn _mm256_cvtepi16_epi8(a: __m256i) -> __m128i;
        fn _mm256_mask_cvtepi16_epi8(src: __m128i, k: __mmask16, a: __m256i) -> __m128i;
        fn _mm256_maskz_cvtepi16_epi8(k: __mmask16, a: __m256i) -> __m128i;
        fn _mm_cvtepi16_epi8(a: __m128i) -> __m128i;
        fn _mm_mask_cvtepi16_epi8(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtepi16_epi8(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_cvtsepi16_epi8(a: __m256i) -> __m128i;
        fn _mm256_mask_cvtsepi16_epi8(src: __m128i, k: __mmask16, a: __m256i) -> __m128i;
        fn _mm256_maskz_cvtsepi16_epi8(k: __mmask16, a: __m256i) -> __m128i;
        fn _mm_cvtsepi16_epi8(a: __m128i) -> __m128i;
        fn _mm_mask_cvtsepi16_epi8(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtsepi16_epi8(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_cvtusepi16_epi8(a: __m256i) -> __m128i;
        fn _mm256_mask_cvtusepi16_epi8(src: __m128i, k: __mmask16, a: __m256i) -> __m128i;
        fn _mm256_maskz_cvtusepi16_epi8(k: __mmask16, a: __m256i) -> __m128i;
        fn _mm_cvtusepi16_epi8(a: __m128i) -> __m128i;
        fn _mm_mask_cvtusepi16_epi8(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtusepi16_epi8(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_cvtepi8_epi16(src: __m256i, k: __mmask16, a: __m128i) -> __m256i;
        fn _mm256_maskz_cvtepi8_epi16(k: __mmask16, a: __m128i) -> __m256i;
        fn _mm_mask_cvtepi8_epi16(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtepi8_epi16(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_cvtepu8_epi16(src: __m256i, k: __mmask16, a: __m128i) -> __m256i;
        fn _mm256_maskz_cvtepu8_epi16(k: __mmask16, a: __m128i) -> __m256i;
        fn _mm_mask_cvtepu8_epi16(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
        fn _mm_maskz_cvtepu8_epi16(k: __mmask8, a: __m128i) -> __m128i;
        fn _mm256_mask_alignr_epi8<const IMM8: i32>(
            src: __m256i,
            k: __mmask32,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm256_maskz_alignr_epi8<const IMM8: i32>(
            k: __mmask32,
            a: __m256i,
            b: __m256i,
        ) -> __m256i;
        fn _mm_mask_alignr_epi8<const IMM8: i32>(
            src: __m128i,
            k: __mmask16,
            a: __m128i,
            b: __m128i,
        ) -> __m128i;
        fn _mm_maskz_alignr_epi8<const IMM8: i32>(
            k: __mmask16,
            a: __m128i,
            b: __m128i,
        ) -> __m128i;
        unsafe fn _mm256_mask_cvtsepi16_storeu_epi8(mem_addr: *mut i8, k: __mmask16, a: __m256i);
        unsafe fn _mm_mask_cvtsepi16_storeu_epi8(mem_addr: *mut i8, k: __mmask8, a: __m128i);
        unsafe fn _mm256_mask_cvtepi16_storeu_epi8(mem_addr: *mut i8, k: __mmask16, a: __m256i);
        unsafe fn _mm_mask_cvtepi16_storeu_epi8(mem_addr: *mut i8, k: __mmask8, a: __m128i);
        unsafe fn _mm256_mask_cvtusepi16_storeu_epi8(mem_addr: *mut i8, k: __mmask16, a: __m256i);
        unsafe fn _mm_mask_cvtusepi16_storeu_epi8(mem_addr: *mut i8, k: __mmask8, a: __m128i);
    }
}
