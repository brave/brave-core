pub mod f32 {
    type T = f32;
    gemm_common::gemm_def!(f32, 2);
}
