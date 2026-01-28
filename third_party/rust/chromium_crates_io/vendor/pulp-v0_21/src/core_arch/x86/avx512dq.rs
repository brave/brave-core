use super::*;

impl Avx512dq {
	delegate!({
		fn _mm_mask_and_pd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
		fn _mm_maskz_and_pd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
		fn _mm256_mask_and_pd(src: __m256d, k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
		fn _mm256_maskz_and_pd(k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
		fn _mm512_and_pd(a: __m512d, b: __m512d) -> __m512d;
		fn _mm512_mask_and_pd(src: __m512d, k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
		fn _mm512_maskz_and_pd(k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
		fn _mm_mask_and_ps(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
		fn _mm_maskz_and_ps(k: __mmask8, a: __m128, b: __m128) -> __m128;
		fn _mm256_mask_and_ps(src: __m256, k: __mmask8, a: __m256, b: __m256) -> __m256;
		fn _mm256_maskz_and_ps(k: __mmask8, a: __m256, b: __m256) -> __m256;
		fn _mm512_and_ps(a: __m512, b: __m512) -> __m512;
		fn _mm512_mask_and_ps(src: __m512, k: __mmask16, a: __m512, b: __m512) -> __m512;
		fn _mm512_maskz_and_ps(k: __mmask16, a: __m512, b: __m512) -> __m512;
		fn _mm_mask_andnot_pd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
		fn _mm_maskz_andnot_pd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
		fn _mm256_mask_andnot_pd(src: __m256d, k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
		fn _mm256_maskz_andnot_pd(k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
		fn _mm512_andnot_pd(a: __m512d, b: __m512d) -> __m512d;
		fn _mm512_mask_andnot_pd(src: __m512d, k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
		fn _mm512_maskz_andnot_pd(k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
		fn _mm_mask_andnot_ps(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
		fn _mm_maskz_andnot_ps(k: __mmask8, a: __m128, b: __m128) -> __m128;
		fn _mm256_mask_andnot_ps(src: __m256, k: __mmask8, a: __m256, b: __m256) -> __m256;
		fn _mm256_maskz_andnot_ps(k: __mmask8, a: __m256, b: __m256) -> __m256;
		fn _mm512_andnot_ps(a: __m512, b: __m512) -> __m512;
		fn _mm512_mask_andnot_ps(src: __m512, k: __mmask16, a: __m512, b: __m512) -> __m512;
		fn _mm512_maskz_andnot_ps(k: __mmask16, a: __m512, b: __m512) -> __m512;
		fn _mm_mask_or_pd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
		fn _mm_maskz_or_pd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
		fn _mm256_mask_or_pd(src: __m256d, k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
		fn _mm256_maskz_or_pd(k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
		fn _mm512_or_pd(a: __m512d, b: __m512d) -> __m512d;
		fn _mm512_mask_or_pd(src: __m512d, k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
		fn _mm512_maskz_or_pd(k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
		fn _mm_mask_or_ps(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
		fn _mm_maskz_or_ps(k: __mmask8, a: __m128, b: __m128) -> __m128;
		fn _mm256_mask_or_ps(src: __m256, k: __mmask8, a: __m256, b: __m256) -> __m256;
		fn _mm256_maskz_or_ps(k: __mmask8, a: __m256, b: __m256) -> __m256;
		fn _mm512_or_ps(a: __m512, b: __m512) -> __m512;
		fn _mm512_mask_or_ps(src: __m512, k: __mmask16, a: __m512, b: __m512) -> __m512;
		fn _mm512_maskz_or_ps(k: __mmask16, a: __m512, b: __m512) -> __m512;
		fn _mm_mask_xor_pd(src: __m128d, k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
		fn _mm_maskz_xor_pd(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
		fn _mm256_mask_xor_pd(src: __m256d, k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
		fn _mm256_maskz_xor_pd(k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
		fn _mm512_xor_pd(a: __m512d, b: __m512d) -> __m512d;
		fn _mm512_mask_xor_pd(src: __m512d, k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
		fn _mm512_maskz_xor_pd(k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
		fn _mm_mask_xor_ps(src: __m128, k: __mmask8, a: __m128, b: __m128) -> __m128;
		fn _mm_maskz_xor_ps(k: __mmask8, a: __m128, b: __m128) -> __m128;
		fn _mm256_mask_xor_ps(src: __m256, k: __mmask8, a: __m256, b: __m256) -> __m256;
		fn _mm256_maskz_xor_ps(k: __mmask8, a: __m256, b: __m256) -> __m256;
		fn _mm512_xor_ps(a: __m512, b: __m512) -> __m512;
		fn _mm512_mask_xor_ps(src: __m512, k: __mmask16, a: __m512, b: __m512) -> __m512;
		fn _mm512_maskz_xor_ps(k: __mmask16, a: __m512, b: __m512) -> __m512;
		fn _mm256_broadcast_f32x2(a: __m128) -> __m256;
		fn _mm256_mask_broadcast_f32x2(src: __m256, k: __mmask8, a: __m128) -> __m256;
		fn _mm256_maskz_broadcast_f32x2(k: __mmask8, a: __m128) -> __m256;
		fn _mm512_broadcast_f32x2(a: __m128) -> __m512;
		fn _mm512_mask_broadcast_f32x2(src: __m512, k: __mmask16, a: __m128) -> __m512;
		fn _mm512_maskz_broadcast_f32x2(k: __mmask16, a: __m128) -> __m512;
		fn _mm512_broadcast_f32x8(a: __m256) -> __m512;
		fn _mm512_mask_broadcast_f32x8(src: __m512, k: __mmask16, a: __m256) -> __m512;
		fn _mm512_maskz_broadcast_f32x8(k: __mmask16, a: __m256) -> __m512;
		fn _mm256_broadcast_f64x2(a: __m128d) -> __m256d;
		fn _mm256_mask_broadcast_f64x2(src: __m256d, k: __mmask8, a: __m128d) -> __m256d;
		fn _mm256_maskz_broadcast_f64x2(k: __mmask8, a: __m128d) -> __m256d;
		fn _mm512_broadcast_f64x2(a: __m128d) -> __m512d;
		fn _mm512_mask_broadcast_f64x2(src: __m512d, k: __mmask8, a: __m128d) -> __m512d;
		fn _mm512_maskz_broadcast_f64x2(k: __mmask8, a: __m128d) -> __m512d;
		fn _mm_broadcast_i32x2(a: __m128i) -> __m128i;
		fn _mm_mask_broadcast_i32x2(src: __m128i, k: __mmask8, a: __m128i) -> __m128i;
		fn _mm_maskz_broadcast_i32x2(k: __mmask8, a: __m128i) -> __m128i;
		fn _mm256_broadcast_i32x2(a: __m128i) -> __m256i;
		fn _mm256_mask_broadcast_i32x2(src: __m256i, k: __mmask8, a: __m128i) -> __m256i;
		fn _mm256_maskz_broadcast_i32x2(k: __mmask8, a: __m128i) -> __m256i;
		fn _mm512_broadcast_i32x2(a: __m128i) -> __m512i;
		fn _mm512_mask_broadcast_i32x2(src: __m512i, k: __mmask16, a: __m128i) -> __m512i;
		fn _mm512_maskz_broadcast_i32x2(k: __mmask16, a: __m128i) -> __m512i;
		fn _mm512_broadcast_i32x8(a: __m256i) -> __m512i;
		fn _mm512_mask_broadcast_i32x8(src: __m512i, k: __mmask16, a: __m256i) -> __m512i;
		fn _mm512_maskz_broadcast_i32x8(k: __mmask16, a: __m256i) -> __m512i;
		fn _mm256_broadcast_i64x2(a: __m128i) -> __m256i;
		fn _mm256_mask_broadcast_i64x2(src: __m256i, k: __mmask8, a: __m128i) -> __m256i;
		fn _mm256_maskz_broadcast_i64x2(k: __mmask8, a: __m128i) -> __m256i;
		fn _mm512_broadcast_i64x2(a: __m128i) -> __m512i;
		fn _mm512_mask_broadcast_i64x2(src: __m512i, k: __mmask8, a: __m128i) -> __m512i;
		fn _mm512_maskz_broadcast_i64x2(k: __mmask8, a: __m128i) -> __m512i;
		fn _mm512_extractf32x8_ps<const IMM8: i32>(a: __m512) -> __m256;
		fn _mm512_mask_extractf32x8_ps<const IMM8: i32>(
			src: __m256,
			k: __mmask8,
			a: __m512,
		) -> __m256;
		fn _mm512_maskz_extractf32x8_ps<const IMM8: i32>(k: __mmask8, a: __m512) -> __m256;
		fn _mm256_extractf64x2_pd<const IMM8: i32>(a: __m256d) -> __m128d;
		fn _mm256_mask_extractf64x2_pd<const IMM8: i32>(
			src: __m128d,
			k: __mmask8,
			a: __m256d,
		) -> __m128d;
		fn _mm256_maskz_extractf64x2_pd<const IMM8: i32>(k: __mmask8, a: __m256d) -> __m128d;
		fn _mm512_extractf64x2_pd<const IMM8: i32>(a: __m512d) -> __m128d;
		fn _mm512_mask_extractf64x2_pd<const IMM8: i32>(
			src: __m128d,
			k: __mmask8,
			a: __m512d,
		) -> __m128d;
		fn _mm512_maskz_extractf64x2_pd<const IMM8: i32>(k: __mmask8, a: __m512d) -> __m128d;
		fn _mm512_extracti32x8_epi32<const IMM8: i32>(a: __m512i) -> __m256i;
		fn _mm512_mask_extracti32x8_epi32<const IMM8: i32>(
			src: __m256i,
			k: __mmask8,
			a: __m512i,
		) -> __m256i;
		fn _mm512_maskz_extracti32x8_epi32<const IMM8: i32>(k: __mmask8, a: __m512i) -> __m256i;
		fn _mm256_extracti64x2_epi64<const IMM8: i32>(a: __m256i) -> __m128i;
		fn _mm256_mask_extracti64x2_epi64<const IMM8: i32>(
			src: __m128i,
			k: __mmask8,
			a: __m256i,
		) -> __m128i;
		fn _mm256_maskz_extracti64x2_epi64<const IMM8: i32>(k: __mmask8, a: __m256i) -> __m128i;
		fn _mm512_extracti64x2_epi64<const IMM8: i32>(a: __m512i) -> __m128i;
		fn _mm512_mask_extracti64x2_epi64<const IMM8: i32>(
			src: __m128i,
			k: __mmask8,
			a: __m512i,
		) -> __m128i;
		fn _mm512_maskz_extracti64x2_epi64<const IMM8: i32>(k: __mmask8, a: __m512i) -> __m128i;
		fn _mm512_insertf32x8<const IMM8: i32>(a: __m512, b: __m256) -> __m512;
		fn _mm512_mask_insertf32x8<const IMM8: i32>(
			src: __m512,
			k: __mmask16,
			a: __m512,
			b: __m256,
		) -> __m512;
		fn _mm512_maskz_insertf32x8<const IMM8: i32>(k: __mmask16, a: __m512, b: __m256) -> __m512;
		fn _mm256_insertf64x2<const IMM8: i32>(a: __m256d, b: __m128d) -> __m256d;
		fn _mm256_mask_insertf64x2<const IMM8: i32>(
			src: __m256d,
			k: __mmask8,
			a: __m256d,
			b: __m128d,
		) -> __m256d;
		fn _mm256_maskz_insertf64x2<const IMM8: i32>(
			k: __mmask8,
			a: __m256d,
			b: __m128d,
		) -> __m256d;
		fn _mm512_insertf64x2<const IMM8: i32>(a: __m512d, b: __m128d) -> __m512d;
		fn _mm512_mask_insertf64x2<const IMM8: i32>(
			src: __m512d,
			k: __mmask8,
			a: __m512d,
			b: __m128d,
		) -> __m512d;
		fn _mm512_maskz_insertf64x2<const IMM8: i32>(
			k: __mmask8,
			a: __m512d,
			b: __m128d,
		) -> __m512d;
		fn _mm512_inserti32x8<const IMM8: i32>(a: __m512i, b: __m256i) -> __m512i;
		fn _mm512_mask_inserti32x8<const IMM8: i32>(
			src: __m512i,
			k: __mmask16,
			a: __m512i,
			b: __m256i,
		) -> __m512i;
		fn _mm512_maskz_inserti32x8<const IMM8: i32>(
			k: __mmask16,
			a: __m512i,
			b: __m256i,
		) -> __m512i;
		fn _mm256_inserti64x2<const IMM8: i32>(a: __m256i, b: __m128i) -> __m256i;
		fn _mm256_mask_inserti64x2<const IMM8: i32>(
			src: __m256i,
			k: __mmask8,
			a: __m256i,
			b: __m128i,
		) -> __m256i;
		fn _mm256_maskz_inserti64x2<const IMM8: i32>(
			k: __mmask8,
			a: __m256i,
			b: __m128i,
		) -> __m256i;
		fn _mm512_inserti64x2<const IMM8: i32>(a: __m512i, b: __m128i) -> __m512i;
		fn _mm512_mask_inserti64x2<const IMM8: i32>(
			src: __m512i,
			k: __mmask8,
			a: __m512i,
			b: __m128i,
		) -> __m512i;
		fn _mm512_maskz_inserti64x2<const IMM8: i32>(
			k: __mmask8,
			a: __m512i,
			b: __m128i,
		) -> __m512i;
		fn _mm512_cvt_roundepi64_pd<const ROUNDING: i32>(a: __m512i) -> __m512d;
		fn _mm512_mask_cvt_roundepi64_pd<const ROUNDING: i32>(
			src: __m512d,
			k: __mmask8,
			a: __m512i,
		) -> __m512d;
		fn _mm512_maskz_cvt_roundepi64_pd<const ROUNDING: i32>(k: __mmask8, a: __m512i) -> __m512d;
		fn _mm_cvtepi64_pd(a: __m128i) -> __m128d;
		fn _mm_mask_cvtepi64_pd(src: __m128d, k: __mmask8, a: __m128i) -> __m128d;
		fn _mm_maskz_cvtepi64_pd(k: __mmask8, a: __m128i) -> __m128d;
		fn _mm256_cvtepi64_pd(a: __m256i) -> __m256d;
		fn _mm256_mask_cvtepi64_pd(src: __m256d, k: __mmask8, a: __m256i) -> __m256d;
		fn _mm256_maskz_cvtepi64_pd(k: __mmask8, a: __m256i) -> __m256d;
		fn _mm512_cvtepi64_pd(a: __m512i) -> __m512d;
		fn _mm512_mask_cvtepi64_pd(src: __m512d, k: __mmask8, a: __m512i) -> __m512d;
		fn _mm512_maskz_cvtepi64_pd(k: __mmask8, a: __m512i) -> __m512d;
		fn _mm512_cvt_roundepi64_ps<const ROUNDING: i32>(a: __m512i) -> __m256;
		fn _mm512_mask_cvt_roundepi64_ps<const ROUNDING: i32>(
			src: __m256,
			k: __mmask8,
			a: __m512i,
		) -> __m256;
		fn _mm512_maskz_cvt_roundepi64_ps<const ROUNDING: i32>(k: __mmask8, a: __m512i) -> __m256;
		fn _mm_cvtepi64_ps(a: __m128i) -> __m128;
		fn _mm_mask_cvtepi64_ps(src: __m128, k: __mmask8, a: __m128i) -> __m128;
		fn _mm_maskz_cvtepi64_ps(k: __mmask8, a: __m128i) -> __m128;
		fn _mm256_cvtepi64_ps(a: __m256i) -> __m128;
		fn _mm256_mask_cvtepi64_ps(src: __m128, k: __mmask8, a: __m256i) -> __m128;
		fn _mm256_maskz_cvtepi64_ps(k: __mmask8, a: __m256i) -> __m128;
		fn _mm512_cvtepi64_ps(a: __m512i) -> __m256;
		fn _mm512_mask_cvtepi64_ps(src: __m256, k: __mmask8, a: __m512i) -> __m256;
		fn _mm512_maskz_cvtepi64_ps(k: __mmask8, a: __m512i) -> __m256;
		fn _mm512_cvt_roundepu64_pd<const ROUNDING: i32>(a: __m512i) -> __m512d;
		fn _mm512_mask_cvt_roundepu64_pd<const ROUNDING: i32>(
			src: __m512d,
			k: __mmask8,
			a: __m512i,
		) -> __m512d;
		fn _mm512_maskz_cvt_roundepu64_pd<const ROUNDING: i32>(k: __mmask8, a: __m512i) -> __m512d;
		fn _mm_cvtepu64_pd(a: __m128i) -> __m128d;
		fn _mm_mask_cvtepu64_pd(src: __m128d, k: __mmask8, a: __m128i) -> __m128d;
		fn _mm_maskz_cvtepu64_pd(k: __mmask8, a: __m128i) -> __m128d;
		fn _mm256_cvtepu64_pd(a: __m256i) -> __m256d;
		fn _mm256_mask_cvtepu64_pd(src: __m256d, k: __mmask8, a: __m256i) -> __m256d;
		fn _mm256_maskz_cvtepu64_pd(k: __mmask8, a: __m256i) -> __m256d;
		fn _mm512_cvtepu64_pd(a: __m512i) -> __m512d;
		fn _mm512_mask_cvtepu64_pd(src: __m512d, k: __mmask8, a: __m512i) -> __m512d;
		fn _mm512_maskz_cvtepu64_pd(k: __mmask8, a: __m512i) -> __m512d;
		fn _mm512_cvt_roundepu64_ps<const ROUNDING: i32>(a: __m512i) -> __m256;
		fn _mm512_mask_cvt_roundepu64_ps<const ROUNDING: i32>(
			src: __m256,
			k: __mmask8,
			a: __m512i,
		) -> __m256;
		fn _mm512_maskz_cvt_roundepu64_ps<const ROUNDING: i32>(k: __mmask8, a: __m512i) -> __m256;
		fn _mm_cvtepu64_ps(a: __m128i) -> __m128;
		fn _mm_mask_cvtepu64_ps(src: __m128, k: __mmask8, a: __m128i) -> __m128;
		fn _mm_maskz_cvtepu64_ps(k: __mmask8, a: __m128i) -> __m128;
		fn _mm256_cvtepu64_ps(a: __m256i) -> __m128;
		fn _mm256_mask_cvtepu64_ps(src: __m128, k: __mmask8, a: __m256i) -> __m128;
		fn _mm256_maskz_cvtepu64_ps(k: __mmask8, a: __m256i) -> __m128;
		fn _mm512_cvtepu64_ps(a: __m512i) -> __m256;
		fn _mm512_mask_cvtepu64_ps(src: __m256, k: __mmask8, a: __m512i) -> __m256;
		fn _mm512_maskz_cvtepu64_ps(k: __mmask8, a: __m512i) -> __m256;
		fn _mm512_cvt_roundpd_epi64<const ROUNDING: i32>(a: __m512d) -> __m512i;
		fn _mm512_mask_cvt_roundpd_epi64<const ROUNDING: i32>(
			src: __m512i,
			k: __mmask8,
			a: __m512d,
		) -> __m512i;
		fn _mm512_maskz_cvt_roundpd_epi64<const ROUNDING: i32>(k: __mmask8, a: __m512d) -> __m512i;
		fn _mm_cvtpd_epi64(a: __m128d) -> __m128i;
		fn _mm_mask_cvtpd_epi64(src: __m128i, k: __mmask8, a: __m128d) -> __m128i;
		fn _mm_maskz_cvtpd_epi64(k: __mmask8, a: __m128d) -> __m128i;
		fn _mm256_cvtpd_epi64(a: __m256d) -> __m256i;
		fn _mm256_mask_cvtpd_epi64(src: __m256i, k: __mmask8, a: __m256d) -> __m256i;
		fn _mm256_maskz_cvtpd_epi64(k: __mmask8, a: __m256d) -> __m256i;
		fn _mm512_cvtpd_epi64(a: __m512d) -> __m512i;
		fn _mm512_mask_cvtpd_epi64(src: __m512i, k: __mmask8, a: __m512d) -> __m512i;
		fn _mm512_maskz_cvtpd_epi64(k: __mmask8, a: __m512d) -> __m512i;
		fn _mm512_cvt_roundps_epi64<const ROUNDING: i32>(a: __m256) -> __m512i;
		fn _mm512_mask_cvt_roundps_epi64<const ROUNDING: i32>(
			src: __m512i,
			k: __mmask8,
			a: __m256,
		) -> __m512i;
		fn _mm512_maskz_cvt_roundps_epi64<const ROUNDING: i32>(k: __mmask8, a: __m256) -> __m512i;
		fn _mm_cvtps_epi64(a: __m128) -> __m128i;
		fn _mm_mask_cvtps_epi64(src: __m128i, k: __mmask8, a: __m128) -> __m128i;
		fn _mm_maskz_cvtps_epi64(k: __mmask8, a: __m128) -> __m128i;
		fn _mm256_cvtps_epi64(a: __m128) -> __m256i;
		fn _mm256_mask_cvtps_epi64(src: __m256i, k: __mmask8, a: __m128) -> __m256i;
		fn _mm256_maskz_cvtps_epi64(k: __mmask8, a: __m128) -> __m256i;
		fn _mm512_cvtps_epi64(a: __m256) -> __m512i;
		fn _mm512_mask_cvtps_epi64(src: __m512i, k: __mmask8, a: __m256) -> __m512i;
		fn _mm512_maskz_cvtps_epi64(k: __mmask8, a: __m256) -> __m512i;
		fn _mm512_cvt_roundpd_epu64<const ROUNDING: i32>(a: __m512d) -> __m512i;
		fn _mm512_mask_cvt_roundpd_epu64<const ROUNDING: i32>(
			src: __m512i,
			k: __mmask8,
			a: __m512d,
		) -> __m512i;
		fn _mm512_maskz_cvt_roundpd_epu64<const ROUNDING: i32>(k: __mmask8, a: __m512d) -> __m512i;
		fn _mm_cvtpd_epu64(a: __m128d) -> __m128i;
		fn _mm_mask_cvtpd_epu64(src: __m128i, k: __mmask8, a: __m128d) -> __m128i;
		fn _mm_maskz_cvtpd_epu64(k: __mmask8, a: __m128d) -> __m128i;
		fn _mm256_cvtpd_epu64(a: __m256d) -> __m256i;
		fn _mm256_mask_cvtpd_epu64(src: __m256i, k: __mmask8, a: __m256d) -> __m256i;
		fn _mm256_maskz_cvtpd_epu64(k: __mmask8, a: __m256d) -> __m256i;
		fn _mm512_cvtpd_epu64(a: __m512d) -> __m512i;
		fn _mm512_mask_cvtpd_epu64(src: __m512i, k: __mmask8, a: __m512d) -> __m512i;
		fn _mm512_maskz_cvtpd_epu64(k: __mmask8, a: __m512d) -> __m512i;
		fn _mm512_cvt_roundps_epu64<const ROUNDING: i32>(a: __m256) -> __m512i;
		fn _mm512_mask_cvt_roundps_epu64<const ROUNDING: i32>(
			src: __m512i,
			k: __mmask8,
			a: __m256,
		) -> __m512i;
		fn _mm512_maskz_cvt_roundps_epu64<const ROUNDING: i32>(k: __mmask8, a: __m256) -> __m512i;
		fn _mm_cvtps_epu64(a: __m128) -> __m128i;
		fn _mm_mask_cvtps_epu64(src: __m128i, k: __mmask8, a: __m128) -> __m128i;
		fn _mm_maskz_cvtps_epu64(k: __mmask8, a: __m128) -> __m128i;
		fn _mm256_cvtps_epu64(a: __m128) -> __m256i;
		fn _mm256_mask_cvtps_epu64(src: __m256i, k: __mmask8, a: __m128) -> __m256i;
		fn _mm256_maskz_cvtps_epu64(k: __mmask8, a: __m128) -> __m256i;
		fn _mm512_cvtps_epu64(a: __m256) -> __m512i;
		fn _mm512_mask_cvtps_epu64(src: __m512i, k: __mmask8, a: __m256) -> __m512i;
		fn _mm512_maskz_cvtps_epu64(k: __mmask8, a: __m256) -> __m512i;
		fn _mm512_cvtt_roundpd_epi64<const SAE: i32>(a: __m512d) -> __m512i;
		fn _mm512_mask_cvtt_roundpd_epi64<const SAE: i32>(
			src: __m512i,
			k: __mmask8,
			a: __m512d,
		) -> __m512i;
		fn _mm512_maskz_cvtt_roundpd_epi64<const SAE: i32>(k: __mmask8, a: __m512d) -> __m512i;
		fn _mm_cvttpd_epi64(a: __m128d) -> __m128i;
		fn _mm_mask_cvttpd_epi64(src: __m128i, k: __mmask8, a: __m128d) -> __m128i;
		fn _mm_maskz_cvttpd_epi64(k: __mmask8, a: __m128d) -> __m128i;
		fn _mm256_cvttpd_epi64(a: __m256d) -> __m256i;
		fn _mm256_mask_cvttpd_epi64(src: __m256i, k: __mmask8, a: __m256d) -> __m256i;
		fn _mm256_maskz_cvttpd_epi64(k: __mmask8, a: __m256d) -> __m256i;
		fn _mm512_cvttpd_epi64(a: __m512d) -> __m512i;
		fn _mm512_mask_cvttpd_epi64(src: __m512i, k: __mmask8, a: __m512d) -> __m512i;
		fn _mm512_maskz_cvttpd_epi64(k: __mmask8, a: __m512d) -> __m512i;
		fn _mm512_cvtt_roundps_epi64<const SAE: i32>(a: __m256) -> __m512i;
		fn _mm512_mask_cvtt_roundps_epi64<const SAE: i32>(
			src: __m512i,
			k: __mmask8,
			a: __m256,
		) -> __m512i;
		fn _mm512_maskz_cvtt_roundps_epi64<const SAE: i32>(k: __mmask8, a: __m256) -> __m512i;
		fn _mm_cvttps_epi64(a: __m128) -> __m128i;
		fn _mm_mask_cvttps_epi64(src: __m128i, k: __mmask8, a: __m128) -> __m128i;
		fn _mm_maskz_cvttps_epi64(k: __mmask8, a: __m128) -> __m128i;
		fn _mm256_cvttps_epi64(a: __m128) -> __m256i;
		fn _mm256_mask_cvttps_epi64(src: __m256i, k: __mmask8, a: __m128) -> __m256i;
		fn _mm256_maskz_cvttps_epi64(k: __mmask8, a: __m128) -> __m256i;
		fn _mm512_cvttps_epi64(a: __m256) -> __m512i;
		fn _mm512_mask_cvttps_epi64(src: __m512i, k: __mmask8, a: __m256) -> __m512i;
		fn _mm512_maskz_cvttps_epi64(k: __mmask8, a: __m256) -> __m512i;
		fn _mm512_cvtt_roundpd_epu64<const SAE: i32>(a: __m512d) -> __m512i;
		fn _mm512_mask_cvtt_roundpd_epu64<const SAE: i32>(
			src: __m512i,
			k: __mmask8,
			a: __m512d,
		) -> __m512i;
		fn _mm512_maskz_cvtt_roundpd_epu64<const SAE: i32>(k: __mmask8, a: __m512d) -> __m512i;
		fn _mm_cvttpd_epu64(a: __m128d) -> __m128i;
		fn _mm_mask_cvttpd_epu64(src: __m128i, k: __mmask8, a: __m128d) -> __m128i;
		fn _mm_maskz_cvttpd_epu64(k: __mmask8, a: __m128d) -> __m128i;
		fn _mm256_cvttpd_epu64(a: __m256d) -> __m256i;
		fn _mm256_mask_cvttpd_epu64(src: __m256i, k: __mmask8, a: __m256d) -> __m256i;
		fn _mm256_maskz_cvttpd_epu64(k: __mmask8, a: __m256d) -> __m256i;
		fn _mm512_cvttpd_epu64(a: __m512d) -> __m512i;
		fn _mm512_mask_cvttpd_epu64(src: __m512i, k: __mmask8, a: __m512d) -> __m512i;
		fn _mm512_maskz_cvttpd_epu64(k: __mmask8, a: __m512d) -> __m512i;
		fn _mm512_cvtt_roundps_epu64<const SAE: i32>(a: __m256) -> __m512i;
		fn _mm512_mask_cvtt_roundps_epu64<const SAE: i32>(
			src: __m512i,
			k: __mmask8,
			a: __m256,
		) -> __m512i;
		fn _mm512_maskz_cvtt_roundps_epu64<const SAE: i32>(k: __mmask8, a: __m256) -> __m512i;
		fn _mm_cvttps_epu64(a: __m128) -> __m128i;
		fn _mm_mask_cvttps_epu64(src: __m128i, k: __mmask8, a: __m128) -> __m128i;
		fn _mm_maskz_cvttps_epu64(k: __mmask8, a: __m128) -> __m128i;
		fn _mm256_cvttps_epu64(a: __m128) -> __m256i;
		fn _mm256_mask_cvttps_epu64(src: __m256i, k: __mmask8, a: __m128) -> __m256i;
		fn _mm256_maskz_cvttps_epu64(k: __mmask8, a: __m128) -> __m256i;
		fn _mm512_cvttps_epu64(a: __m256) -> __m512i;
		fn _mm512_mask_cvttps_epu64(src: __m512i, k: __mmask8, a: __m256) -> __m512i;
		fn _mm512_maskz_cvttps_epu64(k: __mmask8, a: __m256) -> __m512i;
		fn _mm_mullo_epi64(a: __m128i, b: __m128i) -> __m128i;
		fn _mm_mask_mullo_epi64(src: __m128i, k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
		fn _mm_maskz_mullo_epi64(k: __mmask8, a: __m128i, b: __m128i) -> __m128i;
		fn _mm256_mullo_epi64(a: __m256i, b: __m256i) -> __m256i;
		fn _mm256_mask_mullo_epi64(src: __m256i, k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
		fn _mm256_maskz_mullo_epi64(k: __mmask8, a: __m256i, b: __m256i) -> __m256i;
		fn _mm512_mullo_epi64(a: __m512i, b: __m512i) -> __m512i;
		fn _mm512_mask_mullo_epi64(src: __m512i, k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
		fn _mm512_maskz_mullo_epi64(k: __mmask8, a: __m512i, b: __m512i) -> __m512i;
		fn _cvtmask8_u32(a: __mmask8) -> u32;
		fn _cvtu32_mask8(a: u32) -> __mmask8;
		fn _kadd_mask16(a: __mmask16, b: __mmask16) -> __mmask16;
		fn _kadd_mask8(a: __mmask8, b: __mmask8) -> __mmask8;
		fn _kand_mask8(a: __mmask8, b: __mmask8) -> __mmask8;
		fn _kandn_mask8(a: __mmask8, b: __mmask8) -> __mmask8;
		fn _knot_mask8(a: __mmask8) -> __mmask8;
		fn _kor_mask8(a: __mmask8, b: __mmask8) -> __mmask8;
		fn _kxnor_mask8(a: __mmask8, b: __mmask8) -> __mmask8;
		fn _kxor_mask8(a: __mmask8, b: __mmask8) -> __mmask8;
		unsafe fn _kortest_mask8_u8(a: __mmask8, b: __mmask8, all_ones: *mut u8) -> u8;
		fn _kortestc_mask8_u8(a: __mmask8, b: __mmask8) -> u8;
		fn _kortestz_mask8_u8(a: __mmask8, b: __mmask8) -> u8;
		fn _kshiftli_mask8<const COUNT: u32>(a: __mmask8) -> __mmask8;
		fn _kshiftri_mask8<const COUNT: u32>(a: __mmask8) -> __mmask8;
		unsafe fn _ktest_mask16_u8(a: __mmask16, b: __mmask16, and_not: *mut u8) -> u8;
		unsafe fn _ktest_mask8_u8(a: __mmask8, b: __mmask8, and_not: *mut u8) -> u8;
		fn _ktestc_mask16_u8(a: __mmask16, b: __mmask16) -> u8;
		fn _ktestc_mask8_u8(a: __mmask8, b: __mmask8) -> u8;
		fn _ktestz_mask16_u8(a: __mmask16, b: __mmask16) -> u8;
		fn _ktestz_mask8_u8(a: __mmask8, b: __mmask8) -> u8;
		unsafe fn _load_mask8(mem_addr: *const __mmask8) -> __mmask8;
		unsafe fn _store_mask8(mem_addr: *mut __mmask8, a: __mmask8);
		fn _mm_movepi32_mask(a: __m128i) -> __mmask8;
		fn _mm256_movepi32_mask(a: __m256i) -> __mmask8;
		fn _mm512_movepi32_mask(a: __m512i) -> __mmask16;
		fn _mm_movepi64_mask(a: __m128i) -> __mmask8;
		fn _mm256_movepi64_mask(a: __m256i) -> __mmask8;
		fn _mm512_movepi64_mask(a: __m512i) -> __mmask8;
		fn _mm_movm_epi32(k: __mmask8) -> __m128i;
		fn _mm256_movm_epi32(k: __mmask8) -> __m256i;
		fn _mm512_movm_epi32(k: __mmask16) -> __m512i;
		fn _mm_movm_epi64(k: __mmask8) -> __m128i;
		fn _mm256_movm_epi64(k: __mmask8) -> __m256i;
		fn _mm512_movm_epi64(k: __mmask8) -> __m512i;
		fn _mm512_range_round_pd<const IMM8: i32, const SAE: i32>(
			a: __m512d,
			b: __m512d,
		) -> __m512d;
		fn _mm512_mask_range_round_pd<const IMM8: i32, const SAE: i32>(
			src: __m512d,
			k: __mmask8,
			a: __m512d,
			b: __m512d,
		) -> __m512d;
		fn _mm512_maskz_range_round_pd<const IMM8: i32, const SAE: i32>(
			k: __mmask8,
			a: __m512d,
			b: __m512d,
		) -> __m512d;
		fn _mm_range_pd<const IMM8: i32>(a: __m128d, b: __m128d) -> __m128d;
		fn _mm_mask_range_pd<const IMM8: i32>(
			src: __m128d,
			k: __mmask8,
			a: __m128d,
			b: __m128d,
		) -> __m128d;
		fn _mm_maskz_range_pd<const IMM8: i32>(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
		fn _mm256_range_pd<const IMM8: i32>(a: __m256d, b: __m256d) -> __m256d;
		fn _mm256_mask_range_pd<const IMM8: i32>(
			src: __m256d,
			k: __mmask8,
			a: __m256d,
			b: __m256d,
		) -> __m256d;
		fn _mm256_maskz_range_pd<const IMM8: i32>(k: __mmask8, a: __m256d, b: __m256d) -> __m256d;
		fn _mm512_range_pd<const IMM8: i32>(a: __m512d, b: __m512d) -> __m512d;
		fn _mm512_mask_range_pd<const IMM8: i32>(
			src: __m512d,
			k: __mmask8,
			a: __m512d,
			b: __m512d,
		) -> __m512d;
		fn _mm512_maskz_range_pd<const IMM8: i32>(k: __mmask8, a: __m512d, b: __m512d) -> __m512d;
		fn _mm512_range_round_ps<const IMM8: i32, const SAE: i32>(a: __m512, b: __m512) -> __m512;
		fn _mm512_mask_range_round_ps<const IMM8: i32, const SAE: i32>(
			src: __m512,
			k: __mmask16,
			a: __m512,
			b: __m512,
		) -> __m512;
		fn _mm512_maskz_range_round_ps<const IMM8: i32, const SAE: i32>(
			k: __mmask16,
			a: __m512,
			b: __m512,
		) -> __m512;
		fn _mm_range_ps<const IMM8: i32>(a: __m128, b: __m128) -> __m128;
		fn _mm_mask_range_ps<const IMM8: i32>(
			src: __m128,
			k: __mmask8,
			a: __m128,
			b: __m128,
		) -> __m128;
		fn _mm_maskz_range_ps<const IMM8: i32>(k: __mmask8, a: __m128, b: __m128) -> __m128;
		fn _mm256_range_ps<const IMM8: i32>(a: __m256, b: __m256) -> __m256;
		fn _mm256_mask_range_ps<const IMM8: i32>(
			src: __m256,
			k: __mmask8,
			a: __m256,
			b: __m256,
		) -> __m256;
		fn _mm256_maskz_range_ps<const IMM8: i32>(k: __mmask8, a: __m256, b: __m256) -> __m256;
		fn _mm512_range_ps<const IMM8: i32>(a: __m512, b: __m512) -> __m512;
		fn _mm512_mask_range_ps<const IMM8: i32>(
			src: __m512,
			k: __mmask16,
			a: __m512,
			b: __m512,
		) -> __m512;
		fn _mm512_maskz_range_ps<const IMM8: i32>(k: __mmask16, a: __m512, b: __m512) -> __m512;
		fn _mm_range_round_sd<const IMM8: i32, const SAE: i32>(a: __m128d, b: __m128d) -> __m128d;
		fn _mm_mask_range_round_sd<const IMM8: i32, const SAE: i32>(
			src: __m128d,
			k: __mmask8,
			a: __m128d,
			b: __m128d,
		) -> __m128d;
		fn _mm_maskz_range_round_sd<const IMM8: i32, const SAE: i32>(
			k: __mmask8,
			a: __m128d,
			b: __m128d,
		) -> __m128d;
		fn _mm_mask_range_sd<const IMM8: i32>(
			src: __m128d,
			k: __mmask8,
			a: __m128d,
			b: __m128d,
		) -> __m128d;
		fn _mm_maskz_range_sd<const IMM8: i32>(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
		fn _mm_range_round_ss<const IMM8: i32, const SAE: i32>(a: __m128, b: __m128) -> __m128;
		fn _mm_mask_range_round_ss<const IMM8: i32, const SAE: i32>(
			src: __m128,
			k: __mmask8,
			a: __m128,
			b: __m128,
		) -> __m128;
		fn _mm_maskz_range_round_ss<const IMM8: i32, const SAE: i32>(
			k: __mmask8,
			a: __m128,
			b: __m128,
		) -> __m128;
		fn _mm_mask_range_ss<const IMM8: i32>(
			src: __m128,
			k: __mmask8,
			a: __m128,
			b: __m128,
		) -> __m128;
		fn _mm_maskz_range_ss<const IMM8: i32>(k: __mmask8, a: __m128, b: __m128) -> __m128;
		fn _mm512_reduce_round_pd<const IMM8: i32, const SAE: i32>(a: __m512d) -> __m512d;
		fn _mm512_mask_reduce_round_pd<const IMM8: i32, const SAE: i32>(
			src: __m512d,
			k: __mmask8,
			a: __m512d,
		) -> __m512d;
		fn _mm512_maskz_reduce_round_pd<const IMM8: i32, const SAE: i32>(
			k: __mmask8,
			a: __m512d,
		) -> __m512d;
		fn _mm_reduce_pd<const IMM8: i32>(a: __m128d) -> __m128d;
		fn _mm_mask_reduce_pd<const IMM8: i32>(src: __m128d, k: __mmask8, a: __m128d) -> __m128d;
		fn _mm_maskz_reduce_pd<const IMM8: i32>(k: __mmask8, a: __m128d) -> __m128d;
		fn _mm256_reduce_pd<const IMM8: i32>(a: __m256d) -> __m256d;
		fn _mm256_mask_reduce_pd<const IMM8: i32>(src: __m256d, k: __mmask8, a: __m256d)
		-> __m256d;
		fn _mm256_maskz_reduce_pd<const IMM8: i32>(k: __mmask8, a: __m256d) -> __m256d;
		fn _mm512_reduce_pd<const IMM8: i32>(a: __m512d) -> __m512d;
		fn _mm512_mask_reduce_pd<const IMM8: i32>(src: __m512d, k: __mmask8, a: __m512d)
		-> __m512d;
		fn _mm512_maskz_reduce_pd<const IMM8: i32>(k: __mmask8, a: __m512d) -> __m512d;
		fn _mm512_reduce_round_ps<const IMM8: i32, const SAE: i32>(a: __m512) -> __m512;
		fn _mm512_mask_reduce_round_ps<const IMM8: i32, const SAE: i32>(
			src: __m512,
			k: __mmask16,
			a: __m512,
		) -> __m512;
		fn _mm512_maskz_reduce_round_ps<const IMM8: i32, const SAE: i32>(
			k: __mmask16,
			a: __m512,
		) -> __m512;
		fn _mm_reduce_ps<const IMM8: i32>(a: __m128) -> __m128;
		fn _mm_mask_reduce_ps<const IMM8: i32>(src: __m128, k: __mmask8, a: __m128) -> __m128;
		fn _mm_maskz_reduce_ps<const IMM8: i32>(k: __mmask8, a: __m128) -> __m128;
		fn _mm256_reduce_ps<const IMM8: i32>(a: __m256) -> __m256;
		fn _mm256_mask_reduce_ps<const IMM8: i32>(src: __m256, k: __mmask8, a: __m256) -> __m256;
		fn _mm256_maskz_reduce_ps<const IMM8: i32>(k: __mmask8, a: __m256) -> __m256;
		fn _mm512_reduce_ps<const IMM8: i32>(a: __m512) -> __m512;
		fn _mm512_mask_reduce_ps<const IMM8: i32>(src: __m512, k: __mmask16, a: __m512) -> __m512;
		fn _mm512_maskz_reduce_ps<const IMM8: i32>(k: __mmask16, a: __m512) -> __m512;
		fn _mm_reduce_round_sd<const IMM8: i32, const SAE: i32>(a: __m128d, b: __m128d) -> __m128d;
		fn _mm_mask_reduce_round_sd<const IMM8: i32, const SAE: i32>(
			src: __m128d,
			k: __mmask8,
			a: __m128d,
			b: __m128d,
		) -> __m128d;
		fn _mm_maskz_reduce_round_sd<const IMM8: i32, const SAE: i32>(
			k: __mmask8,
			a: __m128d,
			b: __m128d,
		) -> __m128d;
		fn _mm_reduce_sd<const IMM8: i32>(a: __m128d, b: __m128d) -> __m128d;
		fn _mm_mask_reduce_sd<const IMM8: i32>(
			src: __m128d,
			k: __mmask8,
			a: __m128d,
			b: __m128d,
		) -> __m128d;
		fn _mm_maskz_reduce_sd<const IMM8: i32>(k: __mmask8, a: __m128d, b: __m128d) -> __m128d;
		fn _mm_reduce_round_ss<const IMM8: i32, const SAE: i32>(a: __m128, b: __m128) -> __m128;
		fn _mm_mask_reduce_round_ss<const IMM8: i32, const SAE: i32>(
			src: __m128,
			k: __mmask8,
			a: __m128,
			b: __m128,
		) -> __m128;
		fn _mm_maskz_reduce_round_ss<const IMM8: i32, const SAE: i32>(
			k: __mmask8,
			a: __m128,
			b: __m128,
		) -> __m128;
		fn _mm_reduce_ss<const IMM8: i32>(a: __m128, b: __m128) -> __m128;
		fn _mm_mask_reduce_ss<const IMM8: i32>(
			src: __m128,
			k: __mmask8,
			a: __m128,
			b: __m128,
		) -> __m128;
		fn _mm_maskz_reduce_ss<const IMM8: i32>(k: __mmask8, a: __m128, b: __m128) -> __m128;
		fn _mm_fpclass_pd_mask<const IMM8: i32>(a: __m128d) -> __mmask8;
		fn _mm_mask_fpclass_pd_mask<const IMM8: i32>(k1: __mmask8, a: __m128d) -> __mmask8;
		fn _mm256_fpclass_pd_mask<const IMM8: i32>(a: __m256d) -> __mmask8;
		fn _mm256_mask_fpclass_pd_mask<const IMM8: i32>(k1: __mmask8, a: __m256d) -> __mmask8;
		fn _mm512_fpclass_pd_mask<const IMM8: i32>(a: __m512d) -> __mmask8;
		fn _mm512_mask_fpclass_pd_mask<const IMM8: i32>(k1: __mmask8, a: __m512d) -> __mmask8;
		fn _mm_fpclass_ps_mask<const IMM8: i32>(a: __m128) -> __mmask8;
		fn _mm_mask_fpclass_ps_mask<const IMM8: i32>(k1: __mmask8, a: __m128) -> __mmask8;
		fn _mm256_fpclass_ps_mask<const IMM8: i32>(a: __m256) -> __mmask8;
		fn _mm256_mask_fpclass_ps_mask<const IMM8: i32>(k1: __mmask8, a: __m256) -> __mmask8;
		fn _mm512_fpclass_ps_mask<const IMM8: i32>(a: __m512) -> __mmask16;
		fn _mm512_mask_fpclass_ps_mask<const IMM8: i32>(k1: __mmask16, a: __m512) -> __mmask16;
		fn _mm_fpclass_sd_mask<const IMM8: i32>(a: __m128d) -> __mmask8;
		fn _mm_mask_fpclass_sd_mask<const IMM8: i32>(k1: __mmask8, a: __m128d) -> __mmask8;
		fn _mm_fpclass_ss_mask<const IMM8: i32>(a: __m128) -> __mmask8;
		fn _mm_mask_fpclass_ss_mask<const IMM8: i32>(k1: __mmask8, a: __m128) -> __mmask8;
	});
}
