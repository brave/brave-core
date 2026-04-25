use super::*;

impl Sse4_2 {
    delegate! {
        fn _mm_cmpistrm<const IMM8: i32>(a: __m128i, b: __m128i) -> __m128i;
        fn _mm_cmpistri<const IMM8: i32>(a: __m128i, b: __m128i) -> i32;
        fn _mm_cmpistrz<const IMM8: i32>(a: __m128i, b: __m128i) -> i32;
        fn _mm_cmpistrc<const IMM8: i32>(a: __m128i, b: __m128i) -> i32;
        fn _mm_cmpistrs<const IMM8: i32>(a: __m128i, b: __m128i) -> i32;
        fn _mm_cmpistro<const IMM8: i32>(a: __m128i, b: __m128i) -> i32;
        fn _mm_cmpistra<const IMM8: i32>(a: __m128i, b: __m128i) -> i32;
        fn _mm_cmpestrm<const IMM8: i32>(a: __m128i, la: i32, b: __m128i, lb: i32) -> __m128i;
        fn _mm_cmpestri<const IMM8: i32>(a: __m128i, la: i32, b: __m128i, lb: i32) -> i32;
        fn _mm_cmpestrz<const IMM8: i32>(a: __m128i, la: i32, b: __m128i, lb: i32) -> i32;
        fn _mm_cmpestrc<const IMM8: i32>(a: __m128i, la: i32, b: __m128i, lb: i32) -> i32;
        fn _mm_cmpestrs<const IMM8: i32>(a: __m128i, la: i32, b: __m128i, lb: i32) -> i32;
        fn _mm_cmpestro<const IMM8: i32>(a: __m128i, la: i32, b: __m128i, lb: i32) -> i32;
        fn _mm_cmpestra<const IMM8: i32>(a: __m128i, la: i32, b: __m128i, lb: i32) -> i32;
        fn _mm_crc32_u8(crc: u32, v: u8) -> u32;
        fn _mm_crc32_u16(crc: u32, v: u16) -> u32;
        fn _mm_crc32_u32(crc: u32, v: u32) -> u32;
        fn _mm_cmpgt_epi64(a: __m128i, b: __m128i) -> __m128i;
    }
}
