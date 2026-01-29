#![no_std]

#[cfg(any(target_arch = "wasm32", target_arch = "wasm64"))]
mod wasm {
	use core::sync::atomic;
	static SIMD128: atomic::AtomicBool = atomic::AtomicBool::new(cfg!(target_feature = "simd128"));
	static RELAXED_SIMD: atomic::AtomicBool =
		atomic::AtomicBool::new(cfg!(target_feature = "relaxed-simd"));

	#[inline]
	pub fn enable_simd128() {
		SIMD128.store(true, atomic::Ordering::Relaxed);
	}

	#[inline]
	pub fn disable_simd128() {
		disable_relaxed_simd();
		SIMD128.store(false, atomic::Ordering::Relaxed);
	}

	#[inline]
	pub fn is_simd128_enabled() -> bool {
		cfg!(target_feature = "simd128") || SIMD128.load(atomic::Ordering::Relaxed)
	}

	#[inline]
	pub fn enable_relaxed_simd() {
		enable_simd128();
		RELAXED_SIMD.store(true, atomic::Ordering::Relaxed);
	}

	#[inline]
	pub fn disable_relaxed_simd() {
		RELAXED_SIMD.store(false, atomic::Ordering::Relaxed);
	}

	#[inline]
	pub fn is_relaxed_simd_enabled() -> bool {
		cfg!(target_feature = "relaxed-simd") || RELAXED_SIMD.load(atomic::Ordering::Relaxed)
	}
}
#[cfg(any(target_arch = "wasm32", target_arch = "wasm64"))]
pub use wasm::*;
