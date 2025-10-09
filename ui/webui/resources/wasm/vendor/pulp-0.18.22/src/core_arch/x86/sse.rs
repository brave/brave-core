use super::*;

impl Sse {
    delegate! {
        fn _mm_add_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_add_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_sub_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_sub_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_mul_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_mul_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_div_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_div_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_sqrt_ss(a: __m128) -> __m128;
        fn _mm_sqrt_ps(a: __m128) -> __m128;
        fn _mm_rcp_ss(a: __m128) -> __m128;
        fn _mm_rcp_ps(a: __m128) -> __m128;
        fn _mm_rsqrt_ss(a: __m128) -> __m128;
        fn _mm_rsqrt_ps(a: __m128) -> __m128;
        fn _mm_min_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_min_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_max_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_max_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_and_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_andnot_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_or_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_xor_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpeq_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_cmplt_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_cmple_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpgt_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpge_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpneq_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpnlt_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpnle_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpngt_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpnge_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpord_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpunord_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpeq_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_cmplt_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_cmple_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpgt_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpge_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpneq_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpnlt_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpnle_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpngt_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpnge_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpord_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_cmpunord_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_comieq_ss(a: __m128, b: __m128) -> i32;
        fn _mm_comilt_ss(a: __m128, b: __m128) -> i32;
        fn _mm_comile_ss(a: __m128, b: __m128) -> i32;
        fn _mm_comigt_ss(a: __m128, b: __m128) -> i32;
        fn _mm_comige_ss(a: __m128, b: __m128) -> i32;
        fn _mm_comineq_ss(a: __m128, b: __m128) -> i32;
        fn _mm_ucomieq_ss(a: __m128, b: __m128) -> i32;
        fn _mm_ucomilt_ss(a: __m128, b: __m128) -> i32;
        fn _mm_ucomile_ss(a: __m128, b: __m128) -> i32;
        fn _mm_ucomigt_ss(a: __m128, b: __m128) -> i32;
        fn _mm_ucomige_ss(a: __m128, b: __m128) -> i32;
        fn _mm_ucomineq_ss(a: __m128, b: __m128) -> i32;
        fn _mm_cvtss_si32(a: __m128) -> i32;
        fn _mm_cvt_ss2si(a: __m128) -> i32;
        fn _mm_cvttss_si32(a: __m128) -> i32;
        fn _mm_cvtt_ss2si(a: __m128) -> i32;
        fn _mm_cvtss_f32(a: __m128) -> f32;
        fn _mm_cvtsi32_ss(a: __m128, b: i32) -> __m128;
        fn _mm_cvt_si2ss(a: __m128, b: i32) -> __m128;
        fn _mm_set_ss(a: f32) -> __m128;
        fn _mm_set1_ps(a: f32) -> __m128;
        fn _mm_set_ps1(a: f32) -> __m128;
        fn _mm_set_ps(a: f32, b: f32, c: f32, d: f32) -> __m128;
        fn _mm_setr_ps(a: f32, b: f32, c: f32, d: f32) -> __m128;
        fn _mm_setzero_ps() -> __m128;
        fn _mm_shuffle_ps<const MASK: i32>(a: __m128, b: __m128) -> __m128;
        fn _mm_unpackhi_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_unpacklo_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_movehl_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_movelh_ps(a: __m128, b: __m128) -> __m128;
        fn _mm_movemask_ps(a: __m128) -> i32;
        unsafe fn _mm_load_ss(p: *const f32) -> __m128;
        unsafe fn _mm_load1_ps(p: *const f32) -> __m128;
        unsafe fn _mm_load_ps1(p: *const f32) -> __m128;
        unsafe fn _mm_load_ps(p: *const f32) -> __m128;
        unsafe fn _mm_loadu_ps(p: *const f32) -> __m128;
        unsafe fn _mm_loadr_ps(p: *const f32) -> __m128;
        unsafe fn _mm_loadu_si64(mem_addr: *const u8) -> __m128i;
        unsafe fn _mm_store_ss(p: *mut f32, a: __m128);
        unsafe fn _mm_store1_ps(p: *mut f32, a: __m128);
        unsafe fn _mm_store_ps1(p: *mut f32, a: __m128);
        unsafe fn _mm_store_ps(p: *mut f32, a: __m128);
        unsafe fn _mm_storeu_ps(p: *mut f32, a: __m128);
        unsafe fn _mm_storer_ps(p: *mut f32, a: __m128);
        fn _mm_move_ss(a: __m128, b: __m128) -> __m128;
        fn _mm_sfence();
        #[allow(clippy::not_unsafe_ptr_arg_deref)]
        fn _mm_prefetch<const STRATEGY: i32>(p: *const i8);
        fn _mm_undefined_ps() -> __m128;
        #[allow(non_snake_case)]
        fn _MM_TRANSPOSE4_PS(
            row0: &mut __m128,
            row1: &mut __m128,
            row2: &mut __m128,
            row3: &mut __m128,
        );
        unsafe fn _mm_stream_ps(mem_addr: *mut f32, a: __m128);
    }
}
