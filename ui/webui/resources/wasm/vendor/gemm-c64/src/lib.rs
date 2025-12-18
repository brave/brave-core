#![cfg_attr(
    all(feature = "nightly", any(target_arch = "x86", target_arch = "x86_64")),
    feature(stdarch_x86_avx512),
    feature(avx512_target_feature)
)]
#![cfg_attr(not(feature = "std"), no_std)]

pub mod gemm;
mod microkernel;

#[macro_use]
extern crate gemm_common;
