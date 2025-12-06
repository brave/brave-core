pub mod f64 {
    type T = f64;
    gemm_common::gemm_def!(f64, 1);
}
