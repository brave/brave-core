use super::*;

impl Sse3 {
    delegate! {
        fn _mm_addsub_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_addsub_pd(a: __m128d, b: __m128d) -> __m128d;
        fn _mm_hadd_pd(a: __m128d, b: __m128d) -> __m128d;
        fn _mm_hadd_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_hsub_pd(a: __m128d, b: __m128d) -> __m128d;
        fn _mm_hsub_ps(a: __m128, b: __m128) -> __m128;
        unsafe fn _mm_lddqu_si128(mem_addr: *const __m128i) -> __m128i;
        fn _mm_movedup_pd(a: __m128d) -> __m128d;
        unsafe fn _mm_loaddup_pd(mem_addr: *const f64) -> __m128d;
        fn _mm_movehdup_ps(a: __m128) -> __m128;
        fn _mm_moveldup_ps(a: __m128) -> __m128;
    }
}
