#![cfg_attr(
    all(feature = "nightly", any(target_arch = "x86", target_arch = "x86_64")),
    feature(stdarch_x86_avx512),
    feature(avx512_target_feature)
)]
#![cfg_attr(not(feature = "std"), no_std)]

use core::sync::atomic::{AtomicBool, Ordering::Relaxed};

#[cfg(feature = "wasm-simd128-enable")]
pub const DEFAULT_WASM_SIMD128: bool = true;

#[cfg(not(feature = "wasm-simd128-enable"))]
pub const DEFAULT_WASM_SIMD128: bool = false;

static WASM_SIMD128: AtomicBool = AtomicBool::new(DEFAULT_WASM_SIMD128);

#[inline]
pub fn get_wasm_simd128() -> bool {
    WASM_SIMD128.load(Relaxed)
}
#[inline]
pub fn set_wasm_simd128(enable: bool) {
    WASM_SIMD128.store(enable, Relaxed)
}

extern crate alloc;

pub mod cache;

pub mod gemm;
pub mod gemv;
pub mod gevv;

pub mod microkernel;
pub mod pack_operands;
pub mod simd;

#[derive(Copy, Clone, Debug)]
pub enum Parallelism {
    None,
    #[cfg(feature = "rayon")]
    Rayon(usize),
}

pub struct Ptr<T: ?Sized>(pub *mut T);

impl<T: ?Sized> Clone for Ptr<T> {
    #[inline]
    fn clone(&self) -> Self {
        *self
    }
}
impl<T: ?Sized> Copy for Ptr<T> {}

unsafe impl<T: ?Sized> Send for Ptr<T> {}
unsafe impl<T: ?Sized> Sync for Ptr<T> {}

impl<T> Ptr<T> {
    #[inline(always)]
    pub fn wrapping_offset(self, offset: isize) -> Self {
        Ptr::<T>(self.0.wrapping_offset(offset))
    }
    #[inline(always)]
    pub fn wrapping_add(self, offset: usize) -> Self {
        Ptr::<T>(self.0.wrapping_add(offset))
    }
}

#[cfg(not(feature = "std"))]
#[macro_export]
macro_rules! feature_detected {
    ($tt: tt) => {
        cfg!(feature = $tt)
    };
}

#[cfg(all(feature = "std", any(target_arch = "x86", target_arch = "x86_64")))]
#[macro_export]
macro_rules! feature_detected {
    ($tt: tt) => {
        ::std::arch::is_x86_feature_detected!($tt)
    };
}
#[cfg(all(feature = "std", target_arch = "aarch64"))]
#[macro_export]
macro_rules! feature_detected {
    ($tt: tt) => {
        ::std::arch::is_aarch64_feature_detected!($tt)
    };
}
#[cfg(all(feature = "std", target_family = "wasm"))]
#[macro_export]
macro_rules! feature_detected {
    ("simd128") => {
        $crate::get_wasm_simd128()
    };
}
