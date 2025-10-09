pub mod f32 {
    #[allow(unused_imports)]
    use gemm_common::gemm::c32;

    type T = f32;
    gemm_common::gemm_cplx_def!(f32, c32, 2);
}
