pub mod f64 {
    #[allow(unused_imports)]
    use gemm_common::gemm::c64;

    type T = f64;
    gemm_common::gemm_cplx_def!(f64, c64, 1);
}
