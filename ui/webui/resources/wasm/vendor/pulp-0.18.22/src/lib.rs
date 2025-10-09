//! `pulp` is a safe abstraction over SIMD instructions, that allows you to write a function once
//! and dispatch to equivalent vectorized versions based on the features detected at runtime.
//!
//! # Autovectorization example
//!
//! ```
//! use pulp::Arch;
//!
//! let mut v = (0..1000).map(|i| i as f64).collect::<Vec<_>>();
//! let arch = Arch::new();
//!
//! arch.dispatch(|| {
//!     for x in &mut v {
//!         *x *= 2.0;
//!     }
//! });
//!
//! for (i, x) in v.into_iter().enumerate() {
//!     assert_eq!(x, 2.0 * i as f64);
//! }
//! ```
//!
//! # Manual vectorization example
//!
//! ```
//! use pulp::{Arch, Simd, WithSimd};
//!
//! struct TimesThree<'a>(&'a mut [f64]);
//! impl<'a> WithSimd for TimesThree<'a> {
//!     type Output = ();
//!
//!     #[inline(always)]
//!     fn with_simd<S: Simd>(self, simd: S) -> Self::Output {
//!         let v = self.0;
//!         let (head, tail) = S::f64s_as_mut_simd(v);
//!
//!         let three = simd.f64s_splat(3.0);
//!         for x in head {
//!             *x = simd.f64s_mul(three, *x);
//!         }
//!
//!         for x in tail {
//!             *x = *x * 3.0;
//!         }
//!     }
//! }
//!
//! let mut v = (0..1000).map(|i| i as f64).collect::<Vec<_>>();
//! let arch = Arch::new();
//!
//! arch.dispatch(TimesThree(&mut v));
//!
//! for (i, x) in v.into_iter().enumerate() {
//!     assert_eq!(x, 3.0 * i as f64);
//! }
//! ```

#![allow(
    non_camel_case_types,
    unknown_lints,
    clippy::zero_prefixed_literal,
    clippy::identity_op,
    clippy::too_many_arguments,
    clippy::type_complexity,
    clippy::missing_transmute_annotations
)]
#![cfg_attr(
    all(feature = "nightly", any(target_arch = "x86", target_arch = "x86_64")),
    feature(stdarch_x86_avx512),
    feature(avx512_target_feature)
)]
#![cfg_attr(
    all(feature = "nightly", any(target_arch = "aarch64")),
    feature(stdarch_neon_i8mm),
    feature(stdarch_neon_sm4),
    feature(stdarch_neon_ftts),
    feature(stdarch_neon_fcma),
    feature(stdarch_neon_dotprod)
)]
#![cfg_attr(not(feature = "std"), no_std)]
#![cfg_attr(docsrs, feature(doc_cfg))]

use bytemuck::{AnyBitPattern, NoUninit, Pod, Zeroable};
use core::fmt::Debug;
use core::marker::PhantomData;
use core::mem::MaybeUninit;
use core::slice::{from_raw_parts, from_raw_parts_mut};
use num_complex::Complex;
use reborrow::*;
use seal::Seal;

/// Requires the first non-lifetime generic parameter, as well as the function's
/// first input parameter to be the SIMD type.
/// Also currently requires that all the lifetimes be explicitly specified.
#[cfg(feature = "macro")]
#[cfg_attr(docsrs, doc(cfg(feature = "macro")))]
pub use pulp_macro::with_simd;

pub type c32 = Complex<f32>;
pub type c64 = Complex<f64>;

mod seal {
    pub trait Seal {}
}

pub trait NullaryFnOnce {
    type Output;

    fn call(self) -> Self::Output;
}

impl<R, F: FnOnce() -> R> NullaryFnOnce for F {
    type Output = R;

    #[inline(always)]
    fn call(self) -> Self::Output {
        self()
    }
}

pub trait WithSimd {
    type Output;

    fn with_simd<S: Simd>(self, simd: S) -> Self::Output;
}

impl<F: NullaryFnOnce> WithSimd for F {
    type Output = F::Output;

    #[inline(always)]
    fn with_simd<S: Simd>(self, simd: S) -> Self::Output {
        let _simd = &simd;
        self.call()
    }
}

pub trait Simd: Seal + Debug + Copy + Send + Sync + 'static {
    type m32s: Debug + Copy + Send + Sync + Zeroable + 'static;
    type f32s: Debug + Copy + Send + Sync + Pod + 'static;
    type c32s: Debug + Copy + Send + Sync + Pod + 'static;
    type i32s: Debug + Copy + Send + Sync + Pod + 'static;
    type u32s: Debug + Copy + Send + Sync + Pod + 'static;

    type m64s: Debug + Copy + Send + Sync + Zeroable + 'static;
    type f64s: Debug + Copy + Send + Sync + Pod + 'static;
    type c64s: Debug + Copy + Send + Sync + Pod + 'static;
    type i64s: Debug + Copy + Send + Sync + Pod + 'static;
    type u64s: Debug + Copy + Send + Sync + Pod + 'static;

    fn vectorize<Op: WithSimd>(self, op: Op) -> Op::Output;

    #[inline(always)]
    fn f32s_as_simd(slice: &[f32]) -> (&[Self::f32s], &[f32]) {
        unsafe { split_slice(slice) }
    }
    #[inline(always)]
    fn f32s_as_mut_simd(slice: &mut [f32]) -> (&mut [Self::f32s], &mut [f32]) {
        unsafe { split_mut_slice(slice) }
    }
    #[inline(always)]
    fn f32s_as_uninit_mut_simd(
        slice: &mut [MaybeUninit<f32>],
    ) -> (&mut [MaybeUninit<Self::f32s>], &mut [MaybeUninit<f32>]) {
        unsafe { split_mut_slice(slice) }
    }
    #[inline(always)]
    fn c32s_as_simd(slice: &[c32]) -> (&[Self::c32s], &[c32]) {
        unsafe { split_slice(slice) }
    }
    #[inline(always)]
    fn c32s_as_mut_simd(slice: &mut [c32]) -> (&mut [Self::c32s], &mut [c32]) {
        unsafe { split_mut_slice(slice) }
    }
    #[inline(always)]
    fn c32s_as_uninit_mut_simd(
        slice: &mut [MaybeUninit<c32>],
    ) -> (&mut [MaybeUninit<Self::c32s>], &mut [MaybeUninit<c32>]) {
        unsafe { split_mut_slice(slice) }
    }
    #[inline(always)]
    fn i32s_as_simd(slice: &[i32]) -> (&[Self::i32s], &[i32]) {
        unsafe { split_slice(slice) }
    }
    #[inline(always)]
    fn i32s_as_mut_simd(slice: &mut [i32]) -> (&mut [Self::i32s], &mut [i32]) {
        unsafe { split_mut_slice(slice) }
    }
    #[inline(always)]
    fn i32s_as_uninit_mut_simd(
        slice: &mut [MaybeUninit<i32>],
    ) -> (&mut [MaybeUninit<Self::i32s>], &mut [MaybeUninit<i32>]) {
        unsafe { split_mut_slice(slice) }
    }
    #[inline(always)]
    fn u32s_as_simd(slice: &[u32]) -> (&[Self::u32s], &[u32]) {
        unsafe { split_slice(slice) }
    }
    #[inline(always)]
    fn u32s_as_mut_simd(slice: &mut [u32]) -> (&mut [Self::u32s], &mut [u32]) {
        unsafe { split_mut_slice(slice) }
    }
    #[inline(always)]
    fn u32s_as_uninit_mut_simd(
        slice: &mut [MaybeUninit<u32>],
    ) -> (&mut [MaybeUninit<Self::u32s>], &mut [MaybeUninit<u32>]) {
        unsafe { split_mut_slice(slice) }
    }
    #[inline(always)]
    fn f64s_as_simd(slice: &[f64]) -> (&[Self::f64s], &[f64]) {
        unsafe { split_slice(slice) }
    }
    #[inline(always)]
    fn f64s_as_mut_simd(slice: &mut [f64]) -> (&mut [Self::f64s], &mut [f64]) {
        unsafe { split_mut_slice(slice) }
    }
    #[inline(always)]
    fn f64s_as_uninit_mut_simd(
        slice: &mut [MaybeUninit<f64>],
    ) -> (&mut [MaybeUninit<Self::f64s>], &mut [MaybeUninit<f64>]) {
        unsafe { split_mut_slice(slice) }
    }
    #[inline(always)]
    fn c64s_as_simd(slice: &[c64]) -> (&[Self::c64s], &[c64]) {
        unsafe { split_slice(slice) }
    }
    #[inline(always)]
    fn c64s_as_mut_simd(slice: &mut [c64]) -> (&mut [Self::c64s], &mut [c64]) {
        unsafe { split_mut_slice(slice) }
    }
    #[inline(always)]
    fn c64s_as_uninit_mut_simd(
        slice: &mut [MaybeUninit<c64>],
    ) -> (&mut [MaybeUninit<Self::c64s>], &mut [MaybeUninit<c64>]) {
        unsafe { split_mut_slice(slice) }
    }
    #[inline(always)]
    fn i64s_as_simd(slice: &[i64]) -> (&[Self::i64s], &[i64]) {
        unsafe { split_slice(slice) }
    }
    #[inline(always)]
    fn i64s_as_mut_simd(slice: &mut [i64]) -> (&mut [Self::i64s], &mut [i64]) {
        unsafe { split_mut_slice(slice) }
    }
    #[inline(always)]
    fn i64s_as_uninit_mut_simd(
        slice: &mut [MaybeUninit<i64>],
    ) -> (&mut [MaybeUninit<Self::i64s>], &mut [MaybeUninit<i64>]) {
        unsafe { split_mut_slice(slice) }
    }
    #[inline(always)]
    fn u64s_as_simd(slice: &[u64]) -> (&[Self::u64s], &[u64]) {
        unsafe { split_slice(slice) }
    }
    #[inline(always)]
    fn u64s_as_mut_simd(slice: &mut [u64]) -> (&mut [Self::u64s], &mut [u64]) {
        unsafe { split_mut_slice(slice) }
    }
    #[inline(always)]
    fn u64s_as_uninit_mut_simd(
        slice: &mut [MaybeUninit<u64>],
    ) -> (&mut [MaybeUninit<Self::u64s>], &mut [MaybeUninit<u64>]) {
        unsafe { split_mut_slice(slice) }
    }

    #[inline(always)]
    fn f32s_as_rsimd(slice: &[f32]) -> (&[f32], &[Self::f32s]) {
        unsafe { rsplit_slice(slice) }
    }
    #[inline(always)]
    fn f32s_as_mut_rsimd(slice: &mut [f32]) -> (&mut [f32], &mut [Self::f32s]) {
        unsafe { rsplit_mut_slice(slice) }
    }
    #[inline(always)]
    fn f32s_as_uninit_mut_rsimd(
        slice: &mut [MaybeUninit<f32>],
    ) -> (&mut [MaybeUninit<f32>], &mut [MaybeUninit<Self::f32s>]) {
        unsafe { rsplit_mut_slice(slice) }
    }
    #[inline(always)]
    fn c32s_as_rsimd(slice: &[c32]) -> (&[c32], &[Self::c32s]) {
        unsafe { rsplit_slice(slice) }
    }
    #[inline(always)]
    fn c32s_as_mut_rsimd(slice: &mut [c32]) -> (&mut [c32], &mut [Self::c32s]) {
        unsafe { rsplit_mut_slice(slice) }
    }
    #[inline(always)]
    fn c32s_as_uninit_mut_rsimd(
        slice: &mut [MaybeUninit<c32>],
    ) -> (&mut [MaybeUninit<c32>], &mut [MaybeUninit<Self::c32s>]) {
        unsafe { rsplit_mut_slice(slice) }
    }
    #[inline(always)]
    fn i32s_as_rsimd(slice: &[i32]) -> (&[i32], &[Self::i32s]) {
        unsafe { rsplit_slice(slice) }
    }
    #[inline(always)]
    fn i32s_as_mut_rsimd(slice: &mut [i32]) -> (&mut [i32], &mut [Self::i32s]) {
        unsafe { rsplit_mut_slice(slice) }
    }
    #[inline(always)]
    fn i32s_as_uninit_mut_rsimd(
        slice: &mut [MaybeUninit<i32>],
    ) -> (&mut [MaybeUninit<i32>], &mut [MaybeUninit<Self::i32s>]) {
        unsafe { rsplit_mut_slice(slice) }
    }
    #[inline(always)]
    fn u32s_as_rsimd(slice: &[u32]) -> (&[u32], &[Self::u32s]) {
        unsafe { rsplit_slice(slice) }
    }
    #[inline(always)]
    fn u32s_as_mut_rsimd(slice: &mut [u32]) -> (&mut [u32], &mut [Self::u32s]) {
        unsafe { rsplit_mut_slice(slice) }
    }
    #[inline(always)]
    fn u32s_as_uninit_mut_rsimd(
        slice: &mut [MaybeUninit<u32>],
    ) -> (&mut [MaybeUninit<u32>], &mut [MaybeUninit<Self::u32s>]) {
        unsafe { rsplit_mut_slice(slice) }
    }
    #[inline(always)]
    fn f64s_as_rsimd(slice: &[f64]) -> (&[f64], &[Self::f64s]) {
        unsafe { rsplit_slice(slice) }
    }
    #[inline(always)]
    fn f64s_as_mut_rsimd(slice: &mut [f64]) -> (&mut [f64], &mut [Self::f64s]) {
        unsafe { rsplit_mut_slice(slice) }
    }
    #[inline(always)]
    fn f64s_as_uninit_mut_rsimd(
        slice: &mut [MaybeUninit<f64>],
    ) -> (&mut [MaybeUninit<f64>], &mut [MaybeUninit<Self::f64s>]) {
        unsafe { rsplit_mut_slice(slice) }
    }
    #[inline(always)]
    fn c64s_as_rsimd(slice: &[c64]) -> (&[c64], &[Self::c64s]) {
        unsafe { rsplit_slice(slice) }
    }
    #[inline(always)]
    fn c64s_as_mut_rsimd(slice: &mut [c64]) -> (&mut [c64], &mut [Self::c64s]) {
        unsafe { rsplit_mut_slice(slice) }
    }
    #[inline(always)]
    fn c64s_as_uninit_mut_rsimd(
        slice: &mut [MaybeUninit<c64>],
    ) -> (&mut [MaybeUninit<c64>], &mut [MaybeUninit<Self::c64s>]) {
        unsafe { rsplit_mut_slice(slice) }
    }
    #[inline(always)]
    fn i64s_as_rsimd(slice: &[i64]) -> (&[i64], &[Self::i64s]) {
        unsafe { rsplit_slice(slice) }
    }
    #[inline(always)]
    fn i64s_as_mut_rsimd(slice: &mut [i64]) -> (&mut [i64], &mut [Self::i64s]) {
        unsafe { rsplit_mut_slice(slice) }
    }
    #[inline(always)]
    fn i64s_as_uninit_mut_rsimd(
        slice: &mut [MaybeUninit<i64>],
    ) -> (&mut [MaybeUninit<i64>], &mut [MaybeUninit<Self::i64s>]) {
        unsafe { rsplit_mut_slice(slice) }
    }
    #[inline(always)]
    fn u64s_as_rsimd(slice: &[u64]) -> (&[u64], &[Self::u64s]) {
        unsafe { rsplit_slice(slice) }
    }
    #[inline(always)]
    fn u64s_as_mut_rsimd(slice: &mut [u64]) -> (&mut [u64], &mut [Self::u64s]) {
        unsafe { rsplit_mut_slice(slice) }
    }
    #[inline(always)]
    fn u64s_as_uninit_mut_rsimd(
        slice: &mut [MaybeUninit<u64>],
    ) -> (&mut [MaybeUninit<u64>], &mut [MaybeUninit<Self::u64s>]) {
        unsafe { rsplit_mut_slice(slice) }
    }

    #[inline(always)]
    fn i32s_align_offset(self, ptr: *const i32, len: usize) -> Offset<Self::m32s> {
        align_offset_u32::<Self, i32, Self::i32s>(
            self,
            ptr,
            len,
            core::mem::size_of::<Self::i32s>(),
        )
    }
    #[inline(always)]
    fn f32s_align_offset(self, ptr: *const f32, len: usize) -> Offset<Self::m32s> {
        align_offset_u32::<Self, f32, Self::f32s>(
            self,
            ptr,
            len,
            core::mem::size_of::<Self::f32s>(),
        )
    }
    #[inline(always)]
    fn u32s_align_offset(self, ptr: *const u32, len: usize) -> Offset<Self::m32s> {
        align_offset_u32::<Self, u32, Self::u32s>(
            self,
            ptr,
            len,
            core::mem::size_of::<Self::u32s>(),
        )
    }
    #[inline(always)]
    fn c32s_align_offset(self, ptr: *const c32, len: usize) -> Offset<Self::m32s> {
        align_offset_u32x2::<Self, c32, Self::c32s>(
            self,
            ptr,
            len,
            core::mem::size_of::<Self::c32s>(),
        )
    }

    #[inline(always)]
    fn i64s_align_offset(self, ptr: *const i64, len: usize) -> Offset<Self::m64s> {
        align_offset_u64::<Self, i64, Self::i64s>(
            self,
            ptr,
            len,
            core::mem::size_of::<Self::i64s>(),
        )
    }
    #[inline(always)]
    fn f64s_align_offset(self, ptr: *const f64, len: usize) -> Offset<Self::m64s> {
        align_offset_u64::<Self, f64, Self::f64s>(
            self,
            ptr,
            len,
            core::mem::size_of::<Self::f64s>(),
        )
    }
    #[inline(always)]
    fn u64s_align_offset(self, ptr: *const u64, len: usize) -> Offset<Self::m64s> {
        align_offset_u64::<Self, u64, Self::u64s>(
            self,
            ptr,
            len,
            core::mem::size_of::<Self::u64s>(),
        )
    }
    #[inline(always)]
    fn c64s_align_offset(self, ptr: *const c64, len: usize) -> Offset<Self::m64s> {
        align_offset_u64x2::<Self, c64, Self::c64s>(
            self,
            ptr,
            len,
            core::mem::size_of::<Self::c64s>(),
        )
    }

    #[inline(always)]
    #[track_caller]
    fn i32s_as_aligned_simd(
        self,
        slice: &[i32],
        offset: Offset<Self::m32s>,
    ) -> (
        Prefix<i32, Self, Self::m32s>,
        &[Self::i32s],
        Suffix<i32, Self, Self::m32s>,
    ) {
        unsafe { split_slice_aligned_like(self, slice, offset) }
    }
    #[inline(always)]
    #[track_caller]
    fn f32s_as_aligned_simd(
        self,
        slice: &[f32],
        offset: Offset<Self::m32s>,
    ) -> (
        Prefix<f32, Self, Self::m32s>,
        &[Self::f32s],
        Suffix<f32, Self, Self::m32s>,
    ) {
        unsafe { split_slice_aligned_like(self, slice, offset) }
    }
    #[inline(always)]
    #[track_caller]
    fn u32s_as_aligned_simd(
        self,
        slice: &[u32],
        offset: Offset<Self::m32s>,
    ) -> (
        Prefix<u32, Self, Self::m32s>,
        &[Self::u32s],
        Suffix<u32, Self, Self::m32s>,
    ) {
        unsafe { split_slice_aligned_like(self, slice, offset) }
    }
    #[inline(always)]
    #[track_caller]
    fn c32s_as_aligned_simd(
        self,
        slice: &[c32],
        offset: Offset<Self::m32s>,
    ) -> (
        Prefix<c32, Self, Self::m32s>,
        &[Self::c32s],
        Suffix<c32, Self, Self::m32s>,
    ) {
        unsafe { split_slice_aligned_like(self, slice, offset) }
    }

    #[inline(always)]
    #[track_caller]
    fn i64s_as_aligned_simd(
        self,
        slice: &[i64],
        offset: Offset<Self::m64s>,
    ) -> (
        Prefix<i64, Self, Self::m64s>,
        &[Self::i64s],
        Suffix<i64, Self, Self::m64s>,
    ) {
        unsafe { split_slice_aligned_like(self, slice, offset) }
    }
    #[inline(always)]
    #[track_caller]
    fn f64s_as_aligned_simd(
        self,
        slice: &[f64],
        offset: Offset<Self::m64s>,
    ) -> (
        Prefix<f64, Self, Self::m64s>,
        &[Self::f64s],
        Suffix<f64, Self, Self::m64s>,
    ) {
        unsafe { split_slice_aligned_like(self, slice, offset) }
    }
    #[inline(always)]
    #[track_caller]
    fn u64s_as_aligned_simd(
        self,
        slice: &[u64],
        offset: Offset<Self::m64s>,
    ) -> (
        Prefix<u64, Self, Self::m64s>,
        &[Self::u64s],
        Suffix<u64, Self, Self::m64s>,
    ) {
        unsafe { split_slice_aligned_like(self, slice, offset) }
    }
    #[inline(always)]
    #[track_caller]
    fn c64s_as_aligned_simd(
        self,
        slice: &[c64],
        offset: Offset<Self::m64s>,
    ) -> (
        Prefix<c64, Self, Self::m64s>,
        &[Self::c64s],
        Suffix<c64, Self, Self::m64s>,
    ) {
        unsafe { split_slice_aligned_like(self, slice, offset) }
    }

    #[inline(always)]
    #[track_caller]
    fn i32s_as_aligned_mut_simd(
        self,
        slice: &mut [i32],
        offset: Offset<Self::m32s>,
    ) -> (
        PrefixMut<i32, Self, Self::m32s>,
        &mut [Self::i32s],
        SuffixMut<i32, Self, Self::m32s>,
    ) {
        unsafe { split_mut_slice_aligned_like(self, slice, offset) }
    }
    #[inline(always)]
    #[track_caller]
    fn f32s_as_aligned_mut_simd(
        self,
        slice: &mut [f32],
        offset: Offset<Self::m32s>,
    ) -> (
        PrefixMut<f32, Self, Self::m32s>,
        &mut [Self::f32s],
        SuffixMut<f32, Self, Self::m32s>,
    ) {
        unsafe { split_mut_slice_aligned_like(self, slice, offset) }
    }
    #[inline(always)]
    #[track_caller]
    fn u32s_as_aligned_mut_simd(
        self,
        slice: &mut [u32],
        offset: Offset<Self::m32s>,
    ) -> (
        PrefixMut<u32, Self, Self::m32s>,
        &mut [Self::u32s],
        SuffixMut<u32, Self, Self::m32s>,
    ) {
        unsafe { split_mut_slice_aligned_like(self, slice, offset) }
    }
    #[inline(always)]
    #[track_caller]
    fn c32s_as_aligned_mut_simd(
        self,
        slice: &mut [c32],
        offset: Offset<Self::m32s>,
    ) -> (
        PrefixMut<c32, Self, Self::m32s>,
        &mut [Self::c32s],
        SuffixMut<c32, Self, Self::m32s>,
    ) {
        unsafe { split_mut_slice_aligned_like(self, slice, offset) }
    }

    #[inline(always)]
    #[track_caller]
    fn i64s_as_aligned_mut_simd(
        self,
        slice: &mut [i64],
        offset: Offset<Self::m64s>,
    ) -> (
        PrefixMut<i64, Self, Self::m64s>,
        &mut [Self::i64s],
        SuffixMut<i64, Self, Self::m64s>,
    ) {
        unsafe { split_mut_slice_aligned_like(self, slice, offset) }
    }
    #[inline(always)]
    #[track_caller]
    fn f64s_as_aligned_mut_simd(
        self,
        slice: &mut [f64],
        offset: Offset<Self::m64s>,
    ) -> (
        PrefixMut<f64, Self, Self::m64s>,
        &mut [Self::f64s],
        SuffixMut<f64, Self, Self::m64s>,
    ) {
        unsafe { split_mut_slice_aligned_like(self, slice, offset) }
    }
    #[inline(always)]
    #[track_caller]
    fn u64s_as_aligned_mut_simd(
        self,
        slice: &mut [u64],
        offset: Offset<Self::m64s>,
    ) -> (
        PrefixMut<u64, Self, Self::m64s>,
        &mut [Self::u64s],
        SuffixMut<u64, Self, Self::m64s>,
    ) {
        unsafe { split_mut_slice_aligned_like(self, slice, offset) }
    }
    #[inline(always)]
    #[track_caller]
    fn c64s_as_aligned_mut_simd(
        self,
        slice: &mut [c64],
        offset: Offset<Self::m64s>,
    ) -> (
        PrefixMut<c64, Self, Self::m64s>,
        &mut [Self::c64s],
        SuffixMut<c64, Self, Self::m64s>,
    ) {
        unsafe { split_mut_slice_aligned_like(self, slice, offset) }
    }

    #[inline(always)]
    #[track_caller]
    fn i32s_as_aligned_uninit_mut_simd(
        self,
        slice: &mut [MaybeUninit<i32>],
        offset: Offset<Self::m32s>,
    ) -> (
        PrefixMut<MaybeUninit<i32>, Self, Self::m32s>,
        &mut [MaybeUninit<Self::i32s>],
        SuffixMut<MaybeUninit<i32>, Self, Self::m32s>,
    ) {
        unsafe { split_mut_slice_aligned_like(self, slice, offset) }
    }
    #[inline(always)]
    #[track_caller]
    fn f32s_as_aligned_uninit_mut_simd(
        self,
        slice: &mut [MaybeUninit<f32>],
        offset: Offset<Self::m32s>,
    ) -> (
        PrefixMut<MaybeUninit<f32>, Self, Self::m32s>,
        &mut [MaybeUninit<Self::f32s>],
        SuffixMut<MaybeUninit<f32>, Self, Self::m32s>,
    ) {
        unsafe { split_mut_slice_aligned_like(self, slice, offset) }
    }
    #[inline(always)]
    #[track_caller]
    fn u32s_as_aligned_uninit_mut_simd(
        self,
        slice: &mut [MaybeUninit<u32>],
        offset: Offset<Self::m32s>,
    ) -> (
        PrefixMut<MaybeUninit<u32>, Self, Self::m32s>,
        &mut [MaybeUninit<Self::u32s>],
        SuffixMut<MaybeUninit<u32>, Self, Self::m32s>,
    ) {
        unsafe { split_mut_slice_aligned_like(self, slice, offset) }
    }
    #[inline(always)]
    #[track_caller]
    fn c32s_as_aligned_uninit_mut_simd(
        self,
        slice: &mut [MaybeUninit<c32>],
        offset: Offset<Self::m32s>,
    ) -> (
        PrefixMut<MaybeUninit<c32>, Self, Self::m32s>,
        &mut [MaybeUninit<Self::c32s>],
        SuffixMut<MaybeUninit<c32>, Self, Self::m32s>,
    ) {
        unsafe { split_mut_slice_aligned_like(self, slice, offset) }
    }

    #[inline(always)]
    #[track_caller]
    fn i64s_as_aligned_uninit_mut_simd(
        self,
        slice: &mut [MaybeUninit<i64>],
        offset: Offset<Self::m64s>,
    ) -> (
        PrefixMut<MaybeUninit<i64>, Self, Self::m64s>,
        &mut [MaybeUninit<Self::i64s>],
        SuffixMut<MaybeUninit<i64>, Self, Self::m64s>,
    ) {
        unsafe { split_mut_slice_aligned_like(self, slice, offset) }
    }
    #[inline(always)]
    #[track_caller]
    fn f64s_as_aligned_uninit_mut_simd(
        self,
        slice: &mut [MaybeUninit<f64>],
        offset: Offset<Self::m64s>,
    ) -> (
        PrefixMut<MaybeUninit<f64>, Self, Self::m64s>,
        &mut [MaybeUninit<Self::f64s>],
        SuffixMut<MaybeUninit<f64>, Self, Self::m64s>,
    ) {
        unsafe { split_mut_slice_aligned_like(self, slice, offset) }
    }
    #[inline(always)]
    #[track_caller]
    fn u64s_as_aligned_uninit_mut_simd(
        self,
        slice: &mut [MaybeUninit<u64>],
        offset: Offset<Self::m64s>,
    ) -> (
        PrefixMut<MaybeUninit<u64>, Self, Self::m64s>,
        &mut [MaybeUninit<Self::u64s>],
        SuffixMut<MaybeUninit<u64>, Self, Self::m64s>,
    ) {
        unsafe { split_mut_slice_aligned_like(self, slice, offset) }
    }
    #[inline(always)]
    #[track_caller]
    fn c64s_as_aligned_uninit_mut_simd(
        self,
        slice: &mut [MaybeUninit<c64>],
        offset: Offset<Self::m64s>,
    ) -> (
        PrefixMut<MaybeUninit<c64>, Self, Self::m64s>,
        &mut [MaybeUninit<Self::c64s>],
        SuffixMut<MaybeUninit<c64>, Self, Self::m64s>,
    ) {
        unsafe { split_mut_slice_aligned_like(self, slice, offset) }
    }

    /// # Safety
    ///
    /// Addresses corresponding to enabled lanes in the mask have the same restrictions as
    /// [`core::ptr::read`].
    #[inline(always)]
    unsafe fn i32s_mask_load_ptr(
        self,
        mask: Self::m32s,
        ptr: *const i32,
        or: Self::i32s,
    ) -> Self::i32s {
        self.u32s_transmute_i32s(self.u32s_mask_load_ptr(
            mask,
            ptr as *const u32,
            self.i32s_transmute_u32s(or),
        ))
    }
    /// # Safety
    ///
    /// Addresses corresponding to enabled lanes in the mask have the same restrictions as
    /// [`core::ptr::read`].
    #[inline(always)]
    unsafe fn f32s_mask_load_ptr(
        self,
        mask: Self::m32s,
        ptr: *const f32,
        or: Self::f32s,
    ) -> Self::f32s {
        self.u32s_transmute_f32s(self.u32s_mask_load_ptr(
            mask,
            ptr as *const u32,
            self.f32s_transmute_u32s(or),
        ))
    }
    /// # Safety
    ///
    /// Addresses corresponding to enabled lanes in the mask have the same restrictions as
    /// [`core::ptr::read`].
    unsafe fn u32s_mask_load_ptr(
        self,
        mask: Self::m32s,
        ptr: *const u32,
        or: Self::u32s,
    ) -> Self::u32s;
    /// # Safety
    ///
    /// Addresses corresponding to enabled lanes in the mask have the same restrictions as
    /// [`core::ptr::read`].
    unsafe fn c32s_mask_load_ptr(
        self,
        mask: Self::m32s,
        ptr: *const c32,
        or: Self::c32s,
    ) -> Self::c32s;
    /// # Safety
    ///
    /// Addresses corresponding to enabled lanes in the mask have the same restrictions as
    /// [`core::ptr::write`].
    #[inline(always)]
    unsafe fn i32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut i32, values: Self::i32s) {
        self.u32s_mask_store_ptr(mask, ptr as *mut u32, self.i32s_transmute_u32s(values));
    }
    /// # Safety
    ///
    /// Addresses corresponding to enabled lanes in the mask have the same restrictions as
    /// [`core::ptr::write`].
    #[inline(always)]
    unsafe fn f32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut f32, values: Self::f32s) {
        self.u32s_mask_store_ptr(mask, ptr as *mut u32, self.f32s_transmute_u32s(values));
    }
    /// # Safety
    ///
    /// Addresses corresponding to enabled lanes in the mask have the same restrictions as
    /// [`core::ptr::write`].
    unsafe fn u32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut u32, values: Self::u32s);
    /// # Safety
    ///
    /// Addresses corresponding to enabled lanes in the mask have the same restrictions as
    /// [`core::ptr::write`].
    unsafe fn c32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut c32, values: Self::c32s);

    /// # Safety
    ///
    /// Addresses corresponding to enabled lanes in the mask have the same restrictions as
    /// [`core::ptr::read`].
    #[inline(always)]
    unsafe fn i64s_mask_load_ptr(
        self,
        mask: Self::m64s,
        ptr: *const i64,
        or: Self::i64s,
    ) -> Self::i64s {
        self.u64s_transmute_i64s(self.u64s_mask_load_ptr(
            mask,
            ptr as *const u64,
            self.i64s_transmute_u64s(or),
        ))
    }
    /// # Safety
    ///
    /// Addresses corresponding to enabled lanes in the mask have the same restrictions as
    /// [`core::ptr::read`].
    #[inline(always)]
    unsafe fn f64s_mask_load_ptr(
        self,
        mask: Self::m64s,
        ptr: *const f64,
        or: Self::f64s,
    ) -> Self::f64s {
        self.u64s_transmute_f64s(self.u64s_mask_load_ptr(
            mask,
            ptr as *const u64,
            self.f64s_transmute_u64s(or),
        ))
    }
    /// # Safety
    ///
    /// Addresses corresponding to enabled lanes in the mask have the same restrictions as
    /// [`core::ptr::read`].
    unsafe fn u64s_mask_load_ptr(
        self,
        mask: Self::m64s,
        ptr: *const u64,
        or: Self::u64s,
    ) -> Self::u64s;
    /// # Safety
    ///
    /// Addresses corresponding to enabled lanes in the mask have the same restrictions as
    /// [`core::ptr::read`].
    unsafe fn c64s_mask_load_ptr(
        self,
        mask: Self::m64s,
        ptr: *const c64,
        or: Self::c64s,
    ) -> Self::c64s;
    /// # Safety
    ///
    /// Addresses corresponding to enabled lanes in the mask have the same restrictions as
    /// [`core::ptr::write`].
    #[inline(always)]
    unsafe fn i64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut i64, values: Self::i64s) {
        self.u64s_mask_store_ptr(mask, ptr as *mut u64, self.i64s_transmute_u64s(values));
    }
    /// # Safety
    ///
    /// Addresses corresponding to enabled lanes in the mask have the same restrictions as
    /// [`core::ptr::write`].
    #[inline(always)]
    unsafe fn f64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut f64, values: Self::f64s) {
        self.u64s_mask_store_ptr(mask, ptr as *mut u64, self.f64s_transmute_u64s(values));
    }
    /// # Safety
    ///
    /// Addresses corresponding to enabled lanes in the mask have the same restrictions as
    /// [`core::ptr::write`].
    unsafe fn u64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut u64, values: Self::u64s);
    /// # Safety
    ///
    /// Addresses corresponding to enabled lanes in the mask have the same restrictions as
    /// [`core::ptr::write`].
    unsafe fn c64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut c64, values: Self::c64s);

    fn u32s_partial_load(self, slice: &[u32]) -> Self::u32s;
    fn u32s_partial_store(self, slice: &mut [u32], values: Self::u32s);
    fn u64s_partial_load(self, slice: &[u64]) -> Self::u64s;
    fn u64s_partial_store(self, slice: &mut [u64], values: Self::u64s);

    #[inline(always)]
    fn i32s_partial_load(self, slice: &[i32]) -> Self::i32s {
        cast(self.u32s_partial_load(bytemuck::cast_slice(slice)))
    }
    #[inline(always)]
    fn i32s_partial_store(self, slice: &mut [i32], values: Self::i32s) {
        self.u32s_partial_store(bytemuck::cast_slice_mut(slice), cast(values))
    }
    #[inline(always)]
    fn i64s_partial_load(self, slice: &[i64]) -> Self::i64s {
        cast(self.u64s_partial_load(bytemuck::cast_slice(slice)))
    }
    #[inline(always)]
    fn i64s_partial_store(self, slice: &mut [i64], values: Self::i64s) {
        self.u64s_partial_store(bytemuck::cast_slice_mut(slice), cast(values))
    }

    #[inline(always)]
    fn f32s_partial_load(self, slice: &[f32]) -> Self::f32s {
        cast(self.u32s_partial_load(bytemuck::cast_slice(slice)))
    }
    #[inline(always)]
    fn f32s_partial_store(self, slice: &mut [f32], values: Self::f32s) {
        self.u32s_partial_store(bytemuck::cast_slice_mut(slice), cast(values))
    }
    #[inline(always)]
    fn f64s_partial_load(self, slice: &[f64]) -> Self::f64s {
        cast(self.u64s_partial_load(bytemuck::cast_slice(slice)))
    }
    #[inline(always)]
    fn f64s_partial_store(self, slice: &mut [f64], values: Self::f64s) {
        self.u64s_partial_store(bytemuck::cast_slice_mut(slice), cast(values))
    }

    #[inline(always)]
    fn c32s_partial_load(self, slice: &[c32]) -> Self::c32s {
        cast(self.f64s_partial_load(bytemuck::cast_slice(slice)))
    }
    #[inline(always)]
    fn c32s_partial_store(self, slice: &mut [c32], values: Self::c32s) {
        self.f64s_partial_store(bytemuck::cast_slice_mut(slice), cast(values))
    }
    #[inline(always)]
    fn c64s_partial_load(self, slice: &[c64]) -> Self::c64s {
        cast(self.f64s_partial_load(bytemuck::cast_slice(slice)))
    }
    #[inline(always)]
    fn c64s_partial_store(self, slice: &mut [c64], values: Self::c64s) {
        self.f64s_partial_store(bytemuck::cast_slice_mut(slice), cast(values))
    }

    fn u32s_partial_load_last(self, slice: &[u32]) -> Self::u32s;
    fn u32s_partial_store_last(self, slice: &mut [u32], values: Self::u32s);
    fn u64s_partial_load_last(self, slice: &[u64]) -> Self::u64s;
    fn u64s_partial_store_last(self, slice: &mut [u64], values: Self::u64s);

    #[inline(always)]
    fn i32s_partial_load_last(self, slice: &[i32]) -> Self::i32s {
        cast(self.u32s_partial_load_last(bytemuck::cast_slice(slice)))
    }
    #[inline(always)]
    fn i32s_partial_store_last(self, slice: &mut [i32], values: Self::i32s) {
        self.u32s_partial_store_last(bytemuck::cast_slice_mut(slice), cast(values))
    }
    #[inline(always)]
    fn i64s_partial_load_last(self, slice: &[i64]) -> Self::i64s {
        cast(self.u64s_partial_load_last(bytemuck::cast_slice(slice)))
    }
    #[inline(always)]
    fn i64s_partial_store_last(self, slice: &mut [i64], values: Self::i64s) {
        self.u64s_partial_store_last(bytemuck::cast_slice_mut(slice), cast(values))
    }

    #[inline(always)]
    fn f32s_partial_load_last(self, slice: &[f32]) -> Self::f32s {
        cast(self.u32s_partial_load_last(bytemuck::cast_slice(slice)))
    }
    #[inline(always)]
    fn f32s_partial_store_last(self, slice: &mut [f32], values: Self::f32s) {
        self.u32s_partial_store_last(bytemuck::cast_slice_mut(slice), cast(values))
    }
    #[inline(always)]
    fn f64s_partial_load_last(self, slice: &[f64]) -> Self::f64s {
        cast(self.u64s_partial_load_last(bytemuck::cast_slice(slice)))
    }
    #[inline(always)]
    fn f64s_partial_store_last(self, slice: &mut [f64], values: Self::f64s) {
        self.u64s_partial_store_last(bytemuck::cast_slice_mut(slice), cast(values))
    }

    #[inline(always)]
    fn c32s_partial_load_last(self, slice: &[c32]) -> Self::c32s {
        cast(self.f64s_partial_load_last(bytemuck::cast_slice(slice)))
    }
    #[inline(always)]
    fn c32s_partial_store_last(self, slice: &mut [c32], values: Self::c32s) {
        self.f64s_partial_store_last(bytemuck::cast_slice_mut(slice), cast(values))
    }
    #[inline(always)]
    fn c64s_partial_load_last(self, slice: &[c64]) -> Self::c64s {
        cast(self.f64s_partial_load_last(bytemuck::cast_slice(slice)))
    }
    #[inline(always)]
    fn c64s_partial_store_last(self, slice: &mut [c64], values: Self::c64s) {
        self.f64s_partial_store_last(bytemuck::cast_slice_mut(slice), cast(values))
    }

    fn m32s_not(self, a: Self::m32s) -> Self::m32s;
    fn m32s_and(self, a: Self::m32s, b: Self::m32s) -> Self::m32s;
    fn m32s_or(self, a: Self::m32s, b: Self::m32s) -> Self::m32s;
    fn m32s_xor(self, a: Self::m32s, b: Self::m32s) -> Self::m32s;

    fn m64s_not(self, a: Self::m64s) -> Self::m64s;
    fn m64s_and(self, a: Self::m64s, b: Self::m64s) -> Self::m64s;
    fn m64s_or(self, a: Self::m64s, b: Self::m64s) -> Self::m64s;
    fn m64s_xor(self, a: Self::m64s, b: Self::m64s) -> Self::m64s;

    fn u32s_not(self, a: Self::u32s) -> Self::u32s;
    fn u32s_and(self, a: Self::u32s, b: Self::u32s) -> Self::u32s;
    fn u32s_or(self, a: Self::u32s, b: Self::u32s) -> Self::u32s;
    fn u32s_xor(self, a: Self::u32s, b: Self::u32s) -> Self::u32s;

    fn u64s_not(self, a: Self::u64s) -> Self::u64s;
    fn u64s_and(self, a: Self::u64s, b: Self::u64s) -> Self::u64s;
    fn u64s_or(self, a: Self::u64s, b: Self::u64s) -> Self::u64s;
    fn u64s_xor(self, a: Self::u64s, b: Self::u64s) -> Self::u64s;

    fn m32s_select_u32s(
        self,
        mask: Self::m32s,
        if_true: Self::u32s,
        if_false: Self::u32s,
    ) -> Self::u32s;
    fn m64s_select_u64s(
        self,
        mask: Self::m64s,
        if_true: Self::u64s,
        if_false: Self::u64s,
    ) -> Self::u64s;

    #[inline]
    fn i32s_not(self, a: Self::i32s) -> Self::i32s {
        self.u32s_transmute_i32s(self.u32s_not(self.i32s_transmute_u32s(a)))
    }
    #[inline]
    fn i32s_and(self, a: Self::i32s, b: Self::i32s) -> Self::i32s {
        self.u32s_transmute_i32s(
            self.u32s_and(self.i32s_transmute_u32s(a), self.i32s_transmute_u32s(b)),
        )
    }
    #[inline]
    fn i32s_or(self, a: Self::i32s, b: Self::i32s) -> Self::i32s {
        self.u32s_transmute_i32s(
            self.u32s_or(self.i32s_transmute_u32s(a), self.i32s_transmute_u32s(b)),
        )
    }
    #[inline]
    fn i32s_xor(self, a: Self::i32s, b: Self::i32s) -> Self::i32s {
        self.u32s_transmute_i32s(
            self.u32s_xor(self.i32s_transmute_u32s(a), self.i32s_transmute_u32s(b)),
        )
    }

    #[inline]
    fn i64s_not(self, a: Self::i64s) -> Self::i64s {
        self.u64s_transmute_i64s(self.u64s_not(self.i64s_transmute_u64s(a)))
    }
    #[inline]
    fn i64s_and(self, a: Self::i64s, b: Self::i64s) -> Self::i64s {
        self.u64s_transmute_i64s(
            self.u64s_and(self.i64s_transmute_u64s(a), self.i64s_transmute_u64s(b)),
        )
    }
    #[inline]
    fn i64s_or(self, a: Self::i64s, b: Self::i64s) -> Self::i64s {
        self.u64s_transmute_i64s(
            self.u64s_or(self.i64s_transmute_u64s(a), self.i64s_transmute_u64s(b)),
        )
    }
    #[inline]
    fn i64s_xor(self, a: Self::i64s, b: Self::i64s) -> Self::i64s {
        self.u64s_transmute_i64s(
            self.u64s_xor(self.i64s_transmute_u64s(a), self.i64s_transmute_u64s(b)),
        )
    }

    #[inline]
    fn f32s_not(self, a: Self::f32s) -> Self::f32s {
        self.u32s_transmute_f32s(self.u32s_not(self.f32s_transmute_u32s(a)))
    }
    #[inline]
    fn f32s_and(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        self.u32s_transmute_f32s(
            self.u32s_and(self.f32s_transmute_u32s(a), self.f32s_transmute_u32s(b)),
        )
    }
    #[inline]
    fn f32s_or(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        self.u32s_transmute_f32s(
            self.u32s_or(self.f32s_transmute_u32s(a), self.f32s_transmute_u32s(b)),
        )
    }
    #[inline]
    fn f32s_xor(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        self.u32s_transmute_f32s(
            self.u32s_xor(self.f32s_transmute_u32s(a), self.f32s_transmute_u32s(b)),
        )
    }

    #[inline]
    fn f64s_not(self, a: Self::f64s) -> Self::f64s {
        self.u64s_transmute_f64s(self.u64s_not(self.f64s_transmute_u64s(a)))
    }
    #[inline]
    fn f64s_and(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        self.u64s_transmute_f64s(
            self.u64s_and(self.f64s_transmute_u64s(a), self.f64s_transmute_u64s(b)),
        )
    }
    #[inline]
    fn f64s_or(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        self.u64s_transmute_f64s(
            self.u64s_or(self.f64s_transmute_u64s(a), self.f64s_transmute_u64s(b)),
        )
    }
    #[inline]
    fn f64s_xor(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        self.u64s_transmute_f64s(
            self.u64s_xor(self.f64s_transmute_u64s(a), self.f64s_transmute_u64s(b)),
        )
    }

    #[inline]
    fn m32s_select_i32s(
        self,
        mask: Self::m32s,
        if_true: Self::i32s,
        if_false: Self::i32s,
    ) -> Self::i32s {
        self.u32s_transmute_i32s(self.m32s_select_u32s(
            mask,
            self.i32s_transmute_u32s(if_true),
            self.i32s_transmute_u32s(if_false),
        ))
    }
    #[inline]
    fn m32s_select_f32s(
        self,
        mask: Self::m32s,
        if_true: Self::f32s,
        if_false: Self::f32s,
    ) -> Self::f32s {
        self.u32s_transmute_f32s(self.m32s_select_u32s(
            mask,
            self.f32s_transmute_u32s(if_true),
            self.f32s_transmute_u32s(if_false),
        ))
    }
    #[inline]
    fn m64s_select_i64s(
        self,
        mask: Self::m64s,
        if_true: Self::i64s,
        if_false: Self::i64s,
    ) -> Self::i64s {
        self.u64s_transmute_i64s(self.m64s_select_u64s(
            mask,
            self.i64s_transmute_u64s(if_true),
            self.i64s_transmute_u64s(if_false),
        ))
    }
    #[inline]
    fn m64s_select_f64s(
        self,
        mask: Self::m64s,
        if_true: Self::f64s,
        if_false: Self::f64s,
    ) -> Self::f64s {
        self.u64s_transmute_f64s(self.m64s_select_u64s(
            mask,
            self.f64s_transmute_u64s(if_true),
            self.f64s_transmute_u64s(if_false),
        ))
    }

    fn u32s_splat(self, value: u32) -> Self::u32s;
    fn u32s_add(self, a: Self::u32s, b: Self::u32s) -> Self::u32s;
    fn u32s_sub(self, a: Self::u32s, b: Self::u32s) -> Self::u32s;
    fn u32s_less_than(self, a: Self::u32s, b: Self::u32s) -> Self::m32s;
    fn u32s_greater_than(self, a: Self::u32s, b: Self::u32s) -> Self::m32s;
    fn u32s_less_than_or_equal(self, a: Self::u32s, b: Self::u32s) -> Self::m32s;
    fn u32s_greater_than_or_equal(self, a: Self::u32s, b: Self::u32s) -> Self::m32s;
    fn u32s_wrapping_dyn_shl(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s;
    fn u32s_wrapping_dyn_shr(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s;
    fn u32s_widening_mul(self, a: Self::u32s, b: Self::u32s) -> (Self::u32s, Self::u32s);

    fn u64s_splat(self, value: u64) -> Self::u64s;
    fn u64s_add(self, a: Self::u64s, b: Self::u64s) -> Self::u64s;
    fn u64s_sub(self, a: Self::u64s, b: Self::u64s) -> Self::u64s;
    fn u64s_less_than(self, a: Self::u64s, b: Self::u64s) -> Self::m64s;
    fn u64s_greater_than(self, a: Self::u64s, b: Self::u64s) -> Self::m64s;
    fn u64s_less_than_or_equal(self, a: Self::u64s, b: Self::u64s) -> Self::m64s;
    fn u64s_greater_than_or_equal(self, a: Self::u64s, b: Self::u64s) -> Self::m64s;

    #[inline]
    fn i32s_splat(self, value: i32) -> Self::i32s {
        self.u32s_transmute_i32s(self.u32s_splat(value as u32))
    }
    #[inline]
    fn i32s_add(self, a: Self::i32s, b: Self::i32s) -> Self::i32s {
        self.u32s_transmute_i32s(
            self.u32s_add(self.i32s_transmute_u32s(a), self.i32s_transmute_u32s(b)),
        )
    }
    #[inline]
    fn i32s_sub(self, a: Self::i32s, b: Self::i32s) -> Self::i32s {
        self.u32s_transmute_i32s(
            self.u32s_sub(self.i32s_transmute_u32s(a), self.i32s_transmute_u32s(b)),
        )
    }

    #[inline]
    fn i64s_splat(self, value: i64) -> Self::i64s {
        self.u64s_transmute_i64s(self.u64s_splat(value as u64))
    }
    #[inline]
    fn i64s_add(self, a: Self::i64s, b: Self::i64s) -> Self::i64s {
        self.u64s_transmute_i64s(
            self.u64s_add(self.i64s_transmute_u64s(a), self.i64s_transmute_u64s(b)),
        )
    }
    #[inline]
    fn i64s_sub(self, a: Self::i64s, b: Self::i64s) -> Self::i64s {
        self.u64s_transmute_i64s(
            self.u64s_sub(self.i64s_transmute_u64s(a), self.i64s_transmute_u64s(b)),
        )
    }

    fn f32s_splat(self, value: f32) -> Self::f32s;
    #[inline]
    fn f32s_abs(self, a: Self::f32s) -> Self::f32s {
        self.f32s_and(self.f32s_not(self.f32s_splat(-0.0)), a)
    }
    #[inline]
    fn f32s_neg(self, a: Self::f32s) -> Self::f32s {
        self.f32s_xor(self.f32s_splat(-0.0), a)
    }
    fn f32s_add(self, a: Self::f32s, b: Self::f32s) -> Self::f32s;
    fn f32s_sub(self, a: Self::f32s, b: Self::f32s) -> Self::f32s;
    fn f32s_mul(self, a: Self::f32s, b: Self::f32s) -> Self::f32s;
    fn f32s_div(self, a: Self::f32s, b: Self::f32s) -> Self::f32s;
    #[inline]
    fn f32s_mul_add_e(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
        self.f32s_add(self.f32s_mul(a, b), c)
    }
    #[inline]
    fn f32_scalar_mul_add_e(self, a: f32, b: f32, c: f32) -> f32 {
        a * b + c
    }

    fn f32s_mul_add(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s;
    #[inline]
    fn f32_scalar_mul_add(self, a: f32, b: f32, c: f32) -> f32 {
        #[cfg(feature = "std")]
        {
            f32::mul_add(a, b, c)
        }
        #[cfg(not(feature = "std"))]
        {
            libm::fmaf(a, b, c)
        }
    }
    fn f32s_equal(self, a: Self::f32s, b: Self::f32s) -> Self::m32s;
    fn f32s_less_than(self, a: Self::f32s, b: Self::f32s) -> Self::m32s;
    fn f32s_less_than_or_equal(self, a: Self::f32s, b: Self::f32s) -> Self::m32s;
    #[inline]
    fn f32s_greater_than(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        self.f32s_less_than(b, a)
    }
    #[inline]
    fn f32s_greater_than_or_equal(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        self.f32s_less_than_or_equal(b, a)
    }
    fn f32s_min(self, a: Self::f32s, b: Self::f32s) -> Self::f32s;
    fn f32s_max(self, a: Self::f32s, b: Self::f32s) -> Self::f32s;
    fn f32s_reduce_sum(self, a: Self::f32s) -> f32;
    fn f32s_reduce_product(self, a: Self::f32s) -> f32;
    fn f32s_reduce_min(self, a: Self::f32s) -> f32;
    fn f32s_reduce_max(self, a: Self::f32s) -> f32;

    fn c32s_splat(self, value: c32) -> Self::c32s;
    fn c32s_conj(self, a: Self::c32s) -> Self::c32s;
    fn c32s_neg(self, a: Self::c32s) -> Self::c32s;
    fn c32s_swap_re_im(self, a: Self::c32s) -> Self::c32s;
    fn c32s_add(self, a: Self::c32s, b: Self::c32s) -> Self::c32s;
    fn c32s_sub(self, a: Self::c32s, b: Self::c32s) -> Self::c32s;
    /// Computes `a * b`
    #[inline]
    fn c32s_mul_e(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        self.c32s_mul(a, b)
    }
    #[inline]
    fn c32_scalar_mul_e(self, a: c32, b: c32) -> c32 {
        a * b
    }
    fn c32s_mul(self, a: Self::c32s, b: Self::c32s) -> Self::c32s;
    #[inline]
    fn c32_scalar_mul(self, a: c32, b: c32) -> c32 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;

        let re = self.f32_scalar_mul_add(a_re, b_re, -a_im * b_im);
        let im = self.f32_scalar_mul_add(a_re, b_im, a_im * b_re);

        c32 { re, im }
    }
    /// Computes `conj(a) * b`
    #[inline]
    fn c32s_conj_mul_e(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        self.c32s_conj_mul(a, b)
    }
    #[inline]
    fn c32_scalar_conj_mul_e(self, a: c32, b: c32) -> c32 {
        a.conj() * b
    }
    fn c32s_conj_mul(self, a: Self::c32s, b: Self::c32s) -> Self::c32s;
    #[inline]
    fn c32_scalar_conj_mul(self, a: c32, b: c32) -> c32 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;

        let re = self.f32_scalar_mul_add(a_re, b_re, a_im * b_im);
        let im = self.f32_scalar_mul_add(a_re, b_im, -a_im * b_re);

        c32 { re, im }
    }

    /// Computes `a * b + c`
    #[inline]
    fn c32s_mul_add_e(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
        self.c32s_mul_add(a, b, c)
    }
    #[inline]
    fn c32_scalar_mul_add_e(self, a: c32, b: c32, c: c32) -> c32 {
        a * b + c
    }
    fn c32s_mul_add(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s;
    #[inline]
    fn c32_scalar_mul_add(self, a: c32, b: c32, c: c32) -> c32 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;

        let re = self.f32_scalar_mul_add(a_re, b_re, self.f32_scalar_mul_add(-a_im, b_im, c.re));
        let im = self.f32_scalar_mul_add(a_re, b_im, self.f32_scalar_mul_add(a_im, b_re, c.im));

        c32 { re, im }
    }

    /// Computes `conj(a) * b + c`
    #[inline]
    fn c32s_conj_mul_add_e(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
        self.c32s_conj_mul_add(a, b, c)
    }
    #[inline]
    fn c32_scalar_conj_mul_add_e(self, a: c32, b: c32, c: c32) -> c32 {
        a.conj() * b + c
    }
    fn c32s_conj_mul_add(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s;
    #[inline]
    fn c32_scalar_conj_mul_add(self, a: c32, b: c32, c: c32) -> c32 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;

        let re = self.f32_scalar_mul_add(a_re, b_re, self.f32_scalar_mul_add(a_im, b_im, c.re));
        let im = self.f32_scalar_mul_add(a_re, b_im, self.f32_scalar_mul_add(-a_im, b_re, c.im));

        c32 { re, im }
    }

    /// Contains the square of the norm in both the real and imaginary components.
    fn c32s_abs2(self, a: Self::c32s) -> Self::c32s;
    fn c32s_reduce_sum(self, a: Self::c32s) -> c32;

    fn f64s_splat(self, value: f64) -> Self::f64s;
    #[inline]
    fn f64s_abs(self, a: Self::f64s) -> Self::f64s {
        self.f64s_and(self.f64s_not(self.f64s_splat(-0.0)), a)
    }
    #[inline]
    fn f64s_neg(self, a: Self::f64s) -> Self::f64s {
        self.f64s_xor(a, self.f64s_splat(-0.0))
    }
    fn f64s_add(self, a: Self::f64s, b: Self::f64s) -> Self::f64s;
    fn f64s_sub(self, a: Self::f64s, b: Self::f64s) -> Self::f64s;
    fn f64s_mul(self, a: Self::f64s, b: Self::f64s) -> Self::f64s;
    fn f64s_div(self, a: Self::f64s, b: Self::f64s) -> Self::f64s;
    #[inline]
    fn f64s_mul_add_e(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
        self.f64s_add(self.f64s_mul(a, b), c)
    }
    #[inline]
    fn f64_scalar_mul_add_e(self, a: f64, b: f64, c: f64) -> f64 {
        a * b + c
    }
    fn f64s_mul_add(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s;
    #[inline]
    fn f64_scalar_mul_add(self, a: f64, b: f64, c: f64) -> f64 {
        #[cfg(feature = "std")]
        {
            f64::mul_add(a, b, c)
        }
        #[cfg(not(feature = "std"))]
        {
            libm::fma(a, b, c)
        }
    }
    fn f64s_equal(self, a: Self::f64s, b: Self::f64s) -> Self::m64s;
    fn f64s_less_than(self, a: Self::f64s, b: Self::f64s) -> Self::m64s;
    fn f64s_less_than_or_equal(self, a: Self::f64s, b: Self::f64s) -> Self::m64s;
    #[inline]
    fn f64s_greater_than(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        self.f64s_less_than(b, a)
    }
    #[inline]
    fn f64s_greater_than_or_equal(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        self.f64s_less_than_or_equal(b, a)
    }
    fn f64s_min(self, a: Self::f64s, b: Self::f64s) -> Self::f64s;
    fn f64s_max(self, a: Self::f64s, b: Self::f64s) -> Self::f64s;
    fn f64s_reduce_sum(self, a: Self::f64s) -> f64;
    fn f64s_reduce_product(self, a: Self::f64s) -> f64;
    fn f64s_reduce_min(self, a: Self::f64s) -> f64;
    fn f64s_reduce_max(self, a: Self::f64s) -> f64;

    fn c64s_splat(self, value: c64) -> Self::c64s;
    fn c64s_conj(self, a: Self::c64s) -> Self::c64s;
    fn c64s_neg(self, a: Self::c64s) -> Self::c64s;
    fn c64s_swap_re_im(self, a: Self::c64s) -> Self::c64s;
    fn c64s_add(self, a: Self::c64s, b: Self::c64s) -> Self::c64s;
    fn c64s_sub(self, a: Self::c64s, b: Self::c64s) -> Self::c64s;
    /// Computes `a * b`
    fn c64s_mul_e(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        self.c64s_mul(a, b)
    }
    #[inline]
    fn c64_scalar_mul_e(self, a: c64, b: c64) -> c64 {
        a * b
    }
    fn c64s_mul(self, a: Self::c64s, b: Self::c64s) -> Self::c64s;
    #[inline]
    fn c64_scalar_mul(self, a: c64, b: c64) -> c64 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;

        let re = self.f64_scalar_mul_add(a_re, b_re, -a_im * b_im);
        let im = self.f64_scalar_mul_add(a_re, b_im, a_im * b_re);

        c64 { re, im }
    }
    /// Computes `conj(a) * b`
    #[inline]
    fn c64s_conj_mul_e(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        self.c64s_conj_mul(a, b)
    }
    #[inline]
    fn c64_scalar_conj_mul_e(self, a: c64, b: c64) -> c64 {
        a.conj() * b
    }
    fn c64s_conj_mul(self, a: Self::c64s, b: Self::c64s) -> Self::c64s;
    #[inline]
    fn c64_scalar_conj_mul(self, a: c64, b: c64) -> c64 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;

        let re = self.f64_scalar_mul_add(a_re, b_re, a_im * b_im);
        let im = self.f64_scalar_mul_add(a_re, b_im, -a_im * b_re);

        c64 { re, im }
    }

    /// Computes `a * b + c`
    #[inline]
    fn c64s_mul_add_e(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
        self.c64s_mul_add(a, b, c)
    }
    #[inline]
    fn c64_scalar_mul_add_e(self, a: c64, b: c64, c: c64) -> c64 {
        a * b + c
    }
    fn c64s_mul_add(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s;
    #[inline]
    fn c64_scalar_mul_add(self, a: c64, b: c64, c: c64) -> c64 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;

        let re = self.f64_scalar_mul_add(a_re, b_re, self.f64_scalar_mul_add(-a_im, b_im, c.re));
        let im = self.f64_scalar_mul_add(a_re, b_im, self.f64_scalar_mul_add(a_im, b_re, c.im));

        c64 { re, im }
    }

    /// Computes `conj(a) * b + c`
    #[inline]
    fn c64s_conj_mul_add_e(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
        self.c64s_conj_mul_add(a, b, c)
    }
    #[inline]
    fn c64_scalar_conj_mul_add_e(self, a: c64, b: c64, c: c64) -> c64 {
        a.conj() * b + c
    }
    fn c64s_conj_mul_add(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s;
    #[inline]
    fn c64_scalar_conj_mul_add(self, a: c64, b: c64, c: c64) -> c64 {
        let a_re = a.re;
        let a_im = a.im;
        let b_re = b.re;
        let b_im = b.im;

        let re = self.f64_scalar_mul_add(a_re, b_re, self.f64_scalar_mul_add(a_im, b_im, c.re));
        let im = self.f64_scalar_mul_add(a_re, b_im, self.f64_scalar_mul_add(-a_im, b_re, c.im));

        c64 { re, im }
    }

    /// Contains the square of the norm in both the real and imaginary components.
    fn c64s_abs2(self, a: Self::c64s) -> Self::c64s;
    fn c64s_reduce_sum(self, a: Self::c64s) -> c64;

    #[inline]
    fn f32s_transmute_i32s(self, a: Self::f32s) -> Self::i32s {
        cast(a)
    }
    #[inline]
    fn f32s_transmute_u32s(self, a: Self::f32s) -> Self::u32s {
        cast(a)
    }
    #[inline]
    fn i32s_transmute_f32s(self, a: Self::i32s) -> Self::f32s {
        cast(a)
    }
    #[inline]
    fn i32s_transmute_u32s(self, a: Self::i32s) -> Self::u32s {
        cast(a)
    }
    #[inline]
    fn u32s_transmute_f32s(self, a: Self::u32s) -> Self::f32s {
        cast(a)
    }
    #[inline]
    fn u32s_transmute_i32s(self, a: Self::u32s) -> Self::i32s {
        cast(a)
    }

    #[inline]
    fn f64s_transmute_i64s(self, a: Self::f64s) -> Self::i64s {
        cast(a)
    }
    #[inline]
    fn f64s_transmute_u64s(self, a: Self::f64s) -> Self::u64s {
        cast(a)
    }
    #[inline]
    fn i64s_transmute_f64s(self, a: Self::i64s) -> Self::f64s {
        cast(a)
    }
    #[inline]
    fn i64s_transmute_u64s(self, a: Self::i64s) -> Self::u64s {
        cast(a)
    }
    #[inline]
    fn u64s_transmute_f64s(self, a: Self::u64s) -> Self::f64s {
        cast(a)
    }
    #[inline]
    fn u64s_transmute_i64s(self, a: Self::u64s) -> Self::i64s {
        cast(a)
    }

    #[inline(always)]
    fn i32s_rotate_right(self, a: Self::i32s, amount: usize) -> Self::i32s {
        cast(self.u32s_rotate_right(cast(a), amount))
    }
    #[inline(always)]
    fn f32s_rotate_right(self, a: Self::f32s, amount: usize) -> Self::f32s {
        cast(self.u32s_rotate_right(cast(a), amount))
    }
    fn u32s_rotate_right(self, a: Self::u32s, amount: usize) -> Self::u32s;
    fn c32s_rotate_right(self, a: Self::c32s, amount: usize) -> Self::c32s;

    #[inline(always)]
    fn i64s_rotate_right(self, a: Self::i64s, amount: usize) -> Self::i64s {
        cast(self.u64s_rotate_right(cast(a), amount))
    }
    #[inline(always)]
    fn f64s_rotate_right(self, a: Self::f64s, amount: usize) -> Self::f64s {
        cast(self.u64s_rotate_right(cast(a), amount))
    }
    fn u64s_rotate_right(self, a: Self::u64s, amount: usize) -> Self::u64s;
    fn c64s_rotate_right(self, a: Self::c64s, amount: usize) -> Self::c64s;

    #[inline(always)]
    fn i32s_rotate_left(self, a: Self::i32s, amount: usize) -> Self::i32s {
        cast(self.u32s_rotate_left(cast(a), amount))
    }
    #[inline(always)]
    fn f32s_rotate_left(self, a: Self::f32s, amount: usize) -> Self::f32s {
        cast(self.u32s_rotate_left(cast(a), amount))
    }
    #[inline(always)]
    fn u32s_rotate_left(self, a: Self::u32s, amount: usize) -> Self::u32s {
        self.u32s_rotate_right(a, amount.wrapping_neg())
    }
    #[inline(always)]
    fn c32s_rotate_left(self, a: Self::c32s, amount: usize) -> Self::c32s {
        self.c32s_rotate_right(a, amount.wrapping_neg())
    }

    #[inline(always)]
    fn i64s_rotate_left(self, a: Self::i64s, amount: usize) -> Self::i64s {
        cast(self.u64s_rotate_left(cast(a), amount))
    }
    #[inline(always)]
    fn f64s_rotate_left(self, a: Self::f64s, amount: usize) -> Self::f64s {
        cast(self.u64s_rotate_left(cast(a), amount))
    }
    #[inline(always)]
    fn u64s_rotate_left(self, a: Self::u64s, amount: usize) -> Self::u64s {
        self.u64s_rotate_right(a, amount.wrapping_neg())
    }
    #[inline(always)]
    fn c64s_rotate_left(self, a: Self::c64s, amount: usize) -> Self::c64s {
        self.c64s_rotate_right(a, amount.wrapping_neg())
    }
}

#[derive(Debug, Copy, Clone)]
pub struct Scalar;

impl Default for Scalar {
    #[inline]
    fn default() -> Self {
        Self::new()
    }
}

impl Scalar {
    #[inline]
    pub fn new() -> Self {
        Self
    }
}

impl Seal for Scalar {}
impl Simd for Scalar {
    #[inline]
    fn vectorize<Op: WithSimd>(self, op: Op) -> Op::Output {
        op.with_simd(self)
    }

    type m32s = bool;
    type f32s = f32;
    type c32s = c32;
    type i32s = i32;
    type u32s = u32;

    type m64s = bool;
    type f64s = f64;
    type c64s = c64;
    type i64s = i64;
    type u64s = u64;

    #[inline]
    fn m32s_not(self, a: Self::m32s) -> Self::m32s {
        !a
    }
    #[inline]
    fn m32s_and(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        a & b
    }
    #[inline]
    fn m32s_or(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        a | b
    }
    #[inline]
    fn m32s_xor(self, a: Self::m32s, b: Self::m32s) -> Self::m32s {
        a ^ b
    }

    #[inline]
    fn f32s_splat(self, value: f32) -> Self::f32s {
        value
    }
    #[inline]
    fn f32s_add(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        a + b
    }
    #[inline]
    fn f32s_sub(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        a - b
    }
    #[inline]
    fn f32s_mul(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        a * b
    }
    #[inline]
    fn f32s_div(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        a / b
    }
    #[inline]
    fn f32s_equal(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        a == b
    }
    #[inline]
    fn f32s_less_than(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        a < b
    }
    #[inline]
    fn f32s_less_than_or_equal(self, a: Self::f32s, b: Self::f32s) -> Self::m32s {
        a <= b
    }

    #[inline]
    fn c32s_splat(self, value: c32) -> Self::c32s {
        value
    }
    #[inline]
    fn c32s_add(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        a + b
    }
    #[inline]
    fn c32s_sub(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        a - b
    }
    #[inline]
    fn c32s_mul_e(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        a * b
    }

    #[inline]
    fn c64s_splat(self, value: c64) -> Self::c64s {
        value
    }
    #[inline]
    fn c64s_add(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        a + b
    }
    #[inline]
    fn c64s_sub(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        a - b
    }
    #[inline]
    fn c64s_mul_e(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        a * b
    }

    #[inline]
    fn m64s_not(self, a: Self::m64s) -> Self::m64s {
        !a
    }
    #[inline]
    fn m64s_and(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        a & b
    }
    #[inline]
    fn m64s_or(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        a | b
    }
    #[inline]
    fn m64s_xor(self, a: Self::m64s, b: Self::m64s) -> Self::m64s {
        a ^ b
    }

    #[inline]
    fn f64s_splat(self, value: f64) -> Self::f64s {
        value
    }
    #[inline]
    fn f64s_add(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        a + b
    }
    #[inline]
    fn f64s_sub(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        a - b
    }
    #[inline]
    fn f64s_mul(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        a * b
    }
    #[inline]
    fn f64s_div(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        a / b
    }
    #[inline]
    fn f64s_equal(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        a == b
    }
    #[inline]
    fn f64s_less_than(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        a < b
    }
    #[inline]
    fn f64s_less_than_or_equal(self, a: Self::f64s, b: Self::f64s) -> Self::m64s {
        a <= b
    }

    #[inline]
    fn u32s_not(self, a: Self::u32s) -> Self::u32s {
        !a
    }
    #[inline]
    fn u32s_and(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        a & b
    }
    #[inline]
    fn u32s_or(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        a | b
    }
    #[inline]
    fn u32s_xor(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        a ^ b
    }

    #[inline]
    fn u64s_not(self, a: Self::u64s) -> Self::u64s {
        !a
    }
    #[inline]
    fn u64s_and(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        a & b
    }
    #[inline]
    fn u64s_or(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        a | b
    }
    #[inline]
    fn u64s_xor(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        a ^ b
    }

    #[inline]
    fn m32s_select_u32s(
        self,
        mask: Self::m32s,
        if_true: Self::u32s,
        if_false: Self::u32s,
    ) -> Self::u32s {
        if mask {
            if_true
        } else {
            if_false
        }
    }
    #[inline]
    fn m64s_select_u64s(
        self,
        mask: Self::m64s,
        if_true: Self::u64s,
        if_false: Self::u64s,
    ) -> Self::u64s {
        if mask {
            if_true
        } else {
            if_false
        }
    }

    #[inline]
    fn f32s_min(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        a.min(b)
    }
    #[inline]
    fn f32s_max(self, a: Self::f32s, b: Self::f32s) -> Self::f32s {
        a.max(b)
    }

    #[inline]
    fn f64s_min(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        a.min(b)
    }
    #[inline]
    fn f64s_max(self, a: Self::f64s, b: Self::f64s) -> Self::f64s {
        a.max(b)
    }

    #[inline]
    fn u32s_add(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        a.wrapping_add(b)
    }
    #[inline]
    fn u32s_sub(self, a: Self::u32s, b: Self::u32s) -> Self::u32s {
        a.wrapping_sub(b)
    }
    #[inline]
    fn u64s_add(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        a.wrapping_add(b)
    }
    #[inline]
    fn u64s_sub(self, a: Self::u64s, b: Self::u64s) -> Self::u64s {
        a.wrapping_sub(b)
    }

    #[inline]
    fn u32s_splat(self, value: u32) -> Self::u32s {
        value
    }
    #[inline]
    fn u64s_splat(self, value: u64) -> Self::u64s {
        value
    }

    #[inline]
    fn f32s_reduce_sum(self, a: Self::f32s) -> f32 {
        a
    }
    #[inline]
    fn f32s_reduce_product(self, a: Self::f32s) -> f32 {
        a
    }
    #[inline]
    fn f32s_reduce_min(self, a: Self::f32s) -> f32 {
        a
    }
    #[inline]
    fn f32s_reduce_max(self, a: Self::f32s) -> f32 {
        a
    }
    #[inline]
    fn f64s_reduce_sum(self, a: Self::f64s) -> f64 {
        a
    }
    #[inline]
    fn f64s_reduce_product(self, a: Self::f64s) -> f64 {
        a
    }
    #[inline]
    fn f64s_reduce_min(self, a: Self::f64s) -> f64 {
        a
    }
    #[inline]
    fn f64s_reduce_max(self, a: Self::f64s) -> f64 {
        a
    }

    #[inline]
    fn f32s_mul_add(self, a: Self::f32s, b: Self::f32s, c: Self::f32s) -> Self::f32s {
        self.f32_scalar_mul_add(a, b, c)
    }
    #[inline]
    fn f64s_mul_add(self, a: Self::f64s, b: Self::f64s, c: Self::f64s) -> Self::f64s {
        self.f64_scalar_mul_add(a, b, c)
    }

    #[inline]
    fn c32s_abs2(self, a: Self::c32s) -> Self::c32s {
        let norm2 = a.re * a.re + a.im * a.im;
        c32::new(norm2, norm2)
    }
    #[inline]
    fn c64s_abs2(self, a: Self::c64s) -> Self::c64s {
        let norm2 = a.re * a.re + a.im * a.im;
        c64::new(norm2, norm2)
    }

    #[inline]
    fn u32s_partial_load(self, slice: &[u32]) -> Self::u32s {
        if let Some((head, _)) = slice.split_first() {
            *head
        } else {
            0
        }
    }

    #[inline]
    fn u32s_partial_store(self, slice: &mut [u32], values: Self::u32s) {
        if let Some((head, _)) = slice.split_first_mut() {
            *head = values;
        }
    }

    #[inline]
    fn u64s_partial_load(self, slice: &[u64]) -> Self::u64s {
        if let Some((head, _)) = slice.split_first() {
            *head
        } else {
            0
        }
    }

    #[inline]
    fn u64s_partial_store(self, slice: &mut [u64], values: Self::u64s) {
        if let Some((head, _)) = slice.split_first_mut() {
            *head = values;
        }
    }

    #[inline]
    fn c64s_partial_load(self, slice: &[c64]) -> Self::c64s {
        if let Some((head, _)) = slice.split_first() {
            *head
        } else {
            c64 { re: 0.0, im: 0.0 }
        }
    }

    #[inline]
    fn c64s_partial_store(self, slice: &mut [c64], values: Self::c64s) {
        if let Some((head, _)) = slice.split_first_mut() {
            *head = values;
        }
    }

    #[inline]
    fn u32s_partial_load_last(self, slice: &[u32]) -> Self::u32s {
        if let Some((head, _)) = slice.split_last() {
            *head
        } else {
            0
        }
    }

    #[inline]
    fn u32s_partial_store_last(self, slice: &mut [u32], values: Self::u32s) {
        if let Some((head, _)) = slice.split_last_mut() {
            *head = values;
        }
    }

    #[inline]
    fn u64s_partial_load_last(self, slice: &[u64]) -> Self::u64s {
        if let Some((head, _)) = slice.split_last() {
            *head
        } else {
            0
        }
    }

    #[inline]
    fn u64s_partial_store_last(self, slice: &mut [u64], values: Self::u64s) {
        if let Some((head, _)) = slice.split_last_mut() {
            *head = values;
        }
    }

    #[inline]
    fn c64s_partial_load_last(self, slice: &[c64]) -> Self::c64s {
        if let Some((head, _)) = slice.split_last() {
            *head
        } else {
            c64 { re: 0.0, im: 0.0 }
        }
    }

    #[inline]
    fn c64s_partial_store_last(self, slice: &mut [c64], values: Self::c64s) {
        if let Some((head, _)) = slice.split_last_mut() {
            *head = values;
        }
    }

    #[inline]
    fn c32s_conj_mul_e(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        a.conj() * b
    }

    #[inline]
    fn c32s_mul_add_e(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
        a * b + c
    }

    #[inline]
    fn c32s_conj_mul_add_e(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
        a.conj() * b + c
    }

    #[inline]
    fn c64s_conj_mul_e(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        a.conj() * b
    }

    #[inline]
    fn c64s_mul_add_e(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
        a * b + c
    }

    #[inline]
    fn c64s_conj_mul_add_e(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
        a.conj() * b + c
    }

    #[inline]
    fn c32s_conj(self, a: Self::c32s) -> Self::c32s {
        a.conj()
    }

    #[inline]
    fn c64s_conj(self, a: Self::c64s) -> Self::c64s {
        a.conj()
    }

    #[inline]
    fn c32s_neg(self, a: Self::c32s) -> Self::c32s {
        -a
    }

    #[inline]
    fn c32s_swap_re_im(self, a: Self::c32s) -> Self::c32s {
        c32 { re: a.im, im: a.re }
    }

    #[inline]
    fn c32s_reduce_sum(self, a: Self::c32s) -> c32 {
        a
    }

    #[inline]
    fn c64s_neg(self, a: Self::c64s) -> Self::c64s {
        -a
    }

    fn c64s_swap_re_im(self, a: Self::c64s) -> Self::c64s {
        c64 { re: a.im, im: a.re }
    }

    #[inline]
    fn c64s_reduce_sum(self, a: Self::c64s) -> c64 {
        a
    }

    #[inline]
    fn u32s_wrapping_dyn_shl(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
        a.wrapping_shl(amount)
    }
    #[inline]
    fn u32s_wrapping_dyn_shr(self, a: Self::u32s, amount: Self::u32s) -> Self::u32s {
        a.wrapping_shr(amount)
    }

    #[inline]
    fn u32s_widening_mul(self, a: Self::u32s, b: Self::u32s) -> (Self::u32s, Self::u32s) {
        let c = a as u64 * b as u64;
        let lo = c as u32;
        let hi = (c >> 32) as u32;
        (lo, hi)
    }

    #[inline]
    fn u32s_less_than(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        a < b
    }

    #[inline]
    fn u32s_greater_than(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        a > b
    }

    #[inline]
    fn u32s_less_than_or_equal(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        a <= b
    }

    #[inline]
    fn u32s_greater_than_or_equal(self, a: Self::u32s, b: Self::u32s) -> Self::m32s {
        a >= b
    }

    #[inline]
    fn c32s_mul(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        self.c32_scalar_mul(a, b)
    }

    #[inline]
    fn c32s_conj_mul(self, a: Self::c32s, b: Self::c32s) -> Self::c32s {
        self.c32_scalar_conj_mul(a, b)
    }

    #[inline]
    fn c32s_mul_add(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
        self.c32_scalar_mul_add(a, b, c)
    }

    #[inline]
    fn c32s_conj_mul_add(self, a: Self::c32s, b: Self::c32s, c: Self::c32s) -> Self::c32s {
        self.c32_scalar_conj_mul_add(a, b, c)
    }

    #[inline]
    fn c64s_mul(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        self.c64_scalar_mul(a, b)
    }

    #[inline]
    fn c64s_conj_mul(self, a: Self::c64s, b: Self::c64s) -> Self::c64s {
        self.c64_scalar_conj_mul(a, b)
    }

    #[inline]
    fn c64s_mul_add(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
        self.c64_scalar_mul_add(a, b, c)
    }

    #[inline]
    fn c64s_conj_mul_add(self, a: Self::c64s, b: Self::c64s, c: Self::c64s) -> Self::c64s {
        self.c64_scalar_conj_mul_add(a, b, c)
    }

    #[inline(always)]
    unsafe fn u32s_mask_load_ptr(
        self,
        mask: Self::m32s,
        ptr: *const u32,
        or: Self::u32s,
    ) -> Self::u32s {
        if mask {
            *ptr
        } else {
            or
        }
    }

    #[inline(always)]
    unsafe fn c32s_mask_load_ptr(
        self,
        mask: Self::m32s,
        ptr: *const c32,
        or: Self::c32s,
    ) -> Self::c32s {
        if mask {
            *ptr
        } else {
            or
        }
    }

    #[inline(always)]
    unsafe fn u32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut u32, values: Self::u32s) {
        if mask {
            *ptr = values
        }
    }

    #[inline(always)]
    unsafe fn c32s_mask_store_ptr(self, mask: Self::m32s, ptr: *mut c32, values: Self::c32s) {
        if mask {
            *ptr = values
        }
    }

    #[inline(always)]
    unsafe fn u64s_mask_load_ptr(
        self,
        mask: Self::m64s,
        ptr: *const u64,
        or: Self::u64s,
    ) -> Self::u64s {
        if mask {
            *ptr
        } else {
            or
        }
    }

    #[inline(always)]
    unsafe fn c64s_mask_load_ptr(
        self,
        mask: Self::m64s,
        ptr: *const c64,
        or: Self::c64s,
    ) -> Self::c64s {
        if mask {
            *ptr
        } else {
            or
        }
    }

    #[inline(always)]
    unsafe fn u64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut u64, values: Self::u64s) {
        if mask {
            *ptr = values
        }
    }

    #[inline(always)]
    unsafe fn c64s_mask_store_ptr(self, mask: Self::m64s, ptr: *mut c64, values: Self::c64s) {
        if mask {
            *ptr = values
        }
    }

    #[inline(always)]
    fn u64s_less_than(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        a < b
    }

    #[inline(always)]
    fn u64s_greater_than(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        a > b
    }

    #[inline(always)]
    fn u64s_less_than_or_equal(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        a <= b
    }

    #[inline(always)]
    fn u64s_greater_than_or_equal(self, a: Self::u64s, b: Self::u64s) -> Self::m64s {
        a >= b
    }

    #[inline(always)]
    fn u32s_rotate_right(self, a: Self::u32s, _amount: usize) -> Self::u32s {
        a
    }

    #[inline(always)]
    fn c32s_rotate_right(self, a: Self::c32s, _amount: usize) -> Self::c32s {
        a
    }

    #[inline(always)]
    fn u64s_rotate_right(self, a: Self::u64s, _amount: usize) -> Self::u64s {
        a
    }

    #[inline(always)]
    fn c64s_rotate_right(self, a: Self::c64s, _amount: usize) -> Self::c64s {
        a
    }
}

#[derive(Copy, Clone)]
pub struct Prefix<'a, T, S: Simd, Mask: Copy> {
    simd: S,
    mask: Mask,
    base: *const T,
    __marker: PhantomData<&'a T>,
}
#[derive(Copy, Clone)]
pub struct Suffix<'a, T, S: Simd, Mask: Copy> {
    simd: S,
    mask: Mask,
    base: *const T,
    __marker: PhantomData<&'a T>,
}
pub struct PrefixMut<'a, T, S: Simd, Mask: Copy> {
    simd: S,
    mask: Mask,
    base: *mut T,
    __marker: PhantomData<&'a mut T>,
}
pub struct SuffixMut<'a, T, S: Simd, Mask: Copy> {
    simd: S,
    mask: Mask,
    base: *mut T,
    __marker: PhantomData<&'a mut T>,
}

impl<T, S: Simd> Prefix<'_, T, S, bool> {
    #[inline(always)]
    pub fn empty(simd: S) -> Self {
        Self {
            simd,
            mask: false,
            base: core::ptr::null(),
            __marker: PhantomData,
        }
    }
}
impl<T, S: Simd> PrefixMut<'_, T, S, bool> {
    #[inline(always)]
    pub fn empty(simd: S) -> Self {
        Self {
            simd,
            mask: false,
            base: core::ptr::null_mut(),
            __marker: PhantomData,
        }
    }
}
impl<T, S: Simd> SuffixMut<'_, T, S, bool> {
    #[inline(always)]
    pub fn empty(simd: S) -> Self {
        Self {
            simd,
            mask: false,
            base: core::ptr::null_mut(),
            __marker: PhantomData,
        }
    }
}
impl<T, S: Simd> Suffix<'_, T, S, bool> {
    #[inline(always)]
    pub fn empty(simd: S) -> Self {
        Self {
            simd,
            mask: false,
            base: core::ptr::null(),
            __marker: PhantomData,
        }
    }
}

impl<'a, T, S: Simd, Mask: Copy> IntoConst for SuffixMut<'a, T, S, Mask> {
    type Target = Suffix<'a, T, S, Mask>;

    #[inline(always)]
    fn into_const(self) -> Self::Target {
        Suffix {
            simd: self.simd,
            mask: self.mask,
            base: self.base,
            __marker: PhantomData,
        }
    }
}
impl<'a, T, S: Simd, Mask: Copy> IntoConst for PrefixMut<'a, T, S, Mask> {
    type Target = Prefix<'a, T, S, Mask>;

    #[inline(always)]
    fn into_const(self) -> Self::Target {
        Prefix {
            simd: self.simd,
            mask: self.mask,
            base: self.base,
            __marker: PhantomData,
        }
    }
}

impl<'short, T, S: Simd, Mask: Copy> ReborrowMut<'short> for SuffixMut<'_, T, S, Mask> {
    type Target = SuffixMut<'short, T, S, Mask>;

    #[inline(always)]
    fn rb_mut(&'short mut self) -> Self::Target {
        SuffixMut {
            simd: self.simd,
            mask: self.mask,
            base: self.base,
            __marker: PhantomData,
        }
    }
}
impl<'short, T, S: Simd, Mask: Copy> ReborrowMut<'short> for PrefixMut<'_, T, S, Mask> {
    type Target = PrefixMut<'short, T, S, Mask>;

    #[inline(always)]
    fn rb_mut(&'short mut self) -> Self::Target {
        PrefixMut {
            simd: self.simd,
            mask: self.mask,
            base: self.base,
            __marker: PhantomData,
        }
    }
}

impl<'short, T, S: Simd, Mask: Copy> Reborrow<'short> for SuffixMut<'_, T, S, Mask> {
    type Target = Suffix<'short, T, S, Mask>;

    #[inline(always)]
    fn rb(&'short self) -> Self::Target {
        Suffix {
            simd: self.simd,
            mask: self.mask,
            base: self.base,
            __marker: PhantomData,
        }
    }
}
impl<'short, T, S: Simd, Mask: Copy> Reborrow<'short> for PrefixMut<'_, T, S, Mask> {
    type Target = Prefix<'short, T, S, Mask>;

    #[inline(always)]
    fn rb(&'short self) -> Self::Target {
        Prefix {
            simd: self.simd,
            mask: self.mask,
            base: self.base,
            __marker: PhantomData,
        }
    }
}

pub trait Read: Debug {
    type Output;
    fn read_or(&self, or: Self::Output) -> Self::Output;
}
pub trait Write: Read {
    fn write(&mut self, values: Self::Output);
}

impl<T: Copy + Debug> Read for &[T] {
    type Output = T;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        if let [first, ..] = &**self {
            *first
        } else {
            or
        }
    }
}
impl<T: Copy + Debug> Read for &mut [T] {
    type Output = T;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        if let [first, ..] = &**self {
            *first
        } else {
            or
        }
    }
}
impl<T: Copy + Debug> Write for &mut [T] {
    #[inline(always)]
    fn write(&mut self, values: Self::Output) {
        if let [first, ..] = &mut **self {
            *first = values
        }
    }
}

impl<T: Copy + Debug> Read for &T {
    type Output = T;
    #[inline(always)]
    fn read_or(&self, _or: Self::Output) -> Self::Output {
        **self
    }
}
impl<T: Copy + Debug> Read for &mut T {
    type Output = T;
    #[inline(always)]
    fn read_or(&self, _or: Self::Output) -> Self::Output {
        **self
    }
}
impl<T: Copy + Debug> Write for &mut T {
    #[inline(always)]
    fn write(&mut self, values: Self::Output) {
        **self = values;
    }
}

impl<S: Simd> Read for Prefix<'_, u32, S, S::m32s> {
    type Output = S::u32s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.u32s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for Suffix<'_, u32, S, S::m32s> {
    type Output = S::u32s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.u32s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for Prefix<'_, i32, S, S::m32s> {
    type Output = S::i32s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.i32s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for Suffix<'_, i32, S, S::m32s> {
    type Output = S::i32s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.i32s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for Prefix<'_, f32, S, S::m32s> {
    type Output = S::f32s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.f32s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for Suffix<'_, f32, S, S::m32s> {
    type Output = S::f32s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.f32s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for Prefix<'_, c32, S, S::m32s> {
    type Output = S::c32s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.c32s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for Suffix<'_, c32, S, S::m32s> {
    type Output = S::c32s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.c32s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for Prefix<'_, u64, S, S::m64s> {
    type Output = S::u64s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.u64s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for Suffix<'_, u64, S, S::m64s> {
    type Output = S::u64s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.u64s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for Prefix<'_, i64, S, S::m64s> {
    type Output = S::i64s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.i64s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for Suffix<'_, i64, S, S::m64s> {
    type Output = S::i64s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.i64s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for Prefix<'_, f64, S, S::m64s> {
    type Output = S::f64s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.f64s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for Suffix<'_, f64, S, S::m64s> {
    type Output = S::f64s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.f64s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for Prefix<'_, c64, S, S::m64s> {
    type Output = S::c64s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.c64s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for Suffix<'_, c64, S, S::m64s> {
    type Output = S::c64s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.c64s_mask_load_ptr(self.mask, self.base, or) }
    }
}

impl<S: Simd> Read for PrefixMut<'_, u32, S, S::m32s> {
    type Output = S::u32s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.u32s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for SuffixMut<'_, u32, S, S::m32s> {
    type Output = S::u32s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.u32s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for PrefixMut<'_, i32, S, S::m32s> {
    type Output = S::i32s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.i32s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for SuffixMut<'_, i32, S, S::m32s> {
    type Output = S::i32s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.i32s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for PrefixMut<'_, f32, S, S::m32s> {
    type Output = S::f32s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.f32s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for SuffixMut<'_, f32, S, S::m32s> {
    type Output = S::f32s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.f32s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for PrefixMut<'_, c32, S, S::m32s> {
    type Output = S::c32s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.c32s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for SuffixMut<'_, c32, S, S::m32s> {
    type Output = S::c32s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.c32s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for PrefixMut<'_, u64, S, S::m64s> {
    type Output = S::u64s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.u64s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for SuffixMut<'_, u64, S, S::m64s> {
    type Output = S::u64s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.u64s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for PrefixMut<'_, i64, S, S::m64s> {
    type Output = S::i64s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.i64s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for SuffixMut<'_, i64, S, S::m64s> {
    type Output = S::i64s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.i64s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for PrefixMut<'_, f64, S, S::m64s> {
    type Output = S::f64s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.f64s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for SuffixMut<'_, f64, S, S::m64s> {
    type Output = S::f64s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.f64s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for PrefixMut<'_, c64, S, S::m64s> {
    type Output = S::c64s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.c64s_mask_load_ptr(self.mask, self.base, or) }
    }
}
impl<S: Simd> Read for SuffixMut<'_, c64, S, S::m64s> {
    type Output = S::c64s;
    #[inline(always)]
    fn read_or(&self, or: Self::Output) -> Self::Output {
        unsafe { self.simd.c64s_mask_load_ptr(self.mask, self.base, or) }
    }
}

impl<S: Simd> Write for PrefixMut<'_, i32, S, S::m32s> {
    #[inline(always)]
    fn write(&mut self, values: Self::Output) {
        unsafe { self.simd.i32s_mask_store_ptr(self.mask, self.base, values) }
    }
}
impl<S: Simd> Write for SuffixMut<'_, i32, S, S::m32s> {
    #[inline(always)]
    fn write(&mut self, values: Self::Output) {
        unsafe { self.simd.i32s_mask_store_ptr(self.mask, self.base, values) }
    }
}
impl<S: Simd> Write for PrefixMut<'_, f32, S, S::m32s> {
    #[inline(always)]
    fn write(&mut self, values: Self::Output) {
        unsafe { self.simd.f32s_mask_store_ptr(self.mask, self.base, values) }
    }
}
impl<S: Simd> Write for SuffixMut<'_, f32, S, S::m32s> {
    #[inline(always)]
    fn write(&mut self, values: Self::Output) {
        unsafe { self.simd.f32s_mask_store_ptr(self.mask, self.base, values) }
    }
}
impl<S: Simd> Write for PrefixMut<'_, c32, S, S::m32s> {
    #[inline(always)]
    fn write(&mut self, values: Self::Output) {
        unsafe { self.simd.c32s_mask_store_ptr(self.mask, self.base, values) }
    }
}
impl<S: Simd> Write for SuffixMut<'_, c32, S, S::m32s> {
    #[inline(always)]
    fn write(&mut self, values: Self::Output) {
        unsafe { self.simd.c32s_mask_store_ptr(self.mask, self.base, values) }
    }
}
impl<S: Simd> Write for PrefixMut<'_, u32, S, S::m32s> {
    #[inline(always)]
    fn write(&mut self, values: Self::Output) {
        unsafe { self.simd.u32s_mask_store_ptr(self.mask, self.base, values) }
    }
}
impl<S: Simd> Write for SuffixMut<'_, u32, S, S::m32s> {
    #[inline(always)]
    fn write(&mut self, values: Self::Output) {
        unsafe { self.simd.u32s_mask_store_ptr(self.mask, self.base, values) }
    }
}
impl<S: Simd> Write for PrefixMut<'_, i64, S, S::m64s> {
    #[inline(always)]
    fn write(&mut self, values: Self::Output) {
        unsafe { self.simd.i64s_mask_store_ptr(self.mask, self.base, values) }
    }
}
impl<S: Simd> Write for SuffixMut<'_, i64, S, S::m64s> {
    #[inline(always)]
    fn write(&mut self, values: Self::Output) {
        unsafe { self.simd.i64s_mask_store_ptr(self.mask, self.base, values) }
    }
}
impl<S: Simd> Write for PrefixMut<'_, f64, S, S::m64s> {
    #[inline(always)]
    fn write(&mut self, values: Self::Output) {
        unsafe { self.simd.f64s_mask_store_ptr(self.mask, self.base, values) }
    }
}
impl<S: Simd> Write for SuffixMut<'_, f64, S, S::m64s> {
    #[inline(always)]
    fn write(&mut self, values: Self::Output) {
        unsafe { self.simd.f64s_mask_store_ptr(self.mask, self.base, values) }
    }
}
impl<S: Simd> Write for PrefixMut<'_, c64, S, S::m64s> {
    #[inline(always)]
    fn write(&mut self, values: Self::Output) {
        unsafe { self.simd.c64s_mask_store_ptr(self.mask, self.base, values) }
    }
}
impl<S: Simd> Write for SuffixMut<'_, c64, S, S::m64s> {
    #[inline(always)]
    fn write(&mut self, values: Self::Output) {
        unsafe { self.simd.c64s_mask_store_ptr(self.mask, self.base, values) }
    }
}
impl<S: Simd> Write for PrefixMut<'_, u64, S, S::m64s> {
    #[inline(always)]
    fn write(&mut self, values: Self::Output) {
        unsafe { self.simd.u64s_mask_store_ptr(self.mask, self.base, values) }
    }
}
impl<S: Simd> Write for SuffixMut<'_, u64, S, S::m64s> {
    #[inline(always)]
    fn write(&mut self, values: Self::Output) {
        unsafe { self.simd.u64s_mask_store_ptr(self.mask, self.base, values) }
    }
}

impl<S: Simd> Debug for Prefix<'_, u32, S, S::m32s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Prefix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for Suffix<'_, u32, S, S::m32s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Suffix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for Prefix<'_, i32, S, S::m32s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Prefix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for Suffix<'_, i32, S, S::m32s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Suffix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for Prefix<'_, f32, S, S::m32s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Prefix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for Suffix<'_, f32, S, S::m32s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Suffix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for Prefix<'_, c32, S, S::m32s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Prefix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for Suffix<'_, c32, S, S::m32s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Suffix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for Prefix<'_, u64, S, S::m64s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Prefix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for Suffix<'_, u64, S, S::m64s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Suffix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for Prefix<'_, i64, S, S::m64s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Prefix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for Suffix<'_, i64, S, S::m64s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Suffix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for Prefix<'_, f64, S, S::m64s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Prefix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for Suffix<'_, f64, S, S::m64s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Suffix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for Prefix<'_, c64, S, S::m64s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Prefix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for Suffix<'_, c64, S, S::m64s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Suffix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}

impl<S: Simd> Debug for PrefixMut<'_, u32, S, S::m32s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Prefix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for SuffixMut<'_, u32, S, S::m32s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Suffix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for PrefixMut<'_, i32, S, S::m32s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Prefix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for SuffixMut<'_, i32, S, S::m32s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Suffix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for PrefixMut<'_, f32, S, S::m32s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Prefix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for SuffixMut<'_, f32, S, S::m32s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Suffix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for PrefixMut<'_, c32, S, S::m32s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Prefix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for SuffixMut<'_, c32, S, S::m32s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Suffix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for PrefixMut<'_, u64, S, S::m64s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Prefix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for SuffixMut<'_, u64, S, S::m64s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Suffix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for PrefixMut<'_, i64, S, S::m64s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Prefix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for SuffixMut<'_, i64, S, S::m64s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Suffix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for PrefixMut<'_, f64, S, S::m64s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Prefix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for SuffixMut<'_, f64, S, S::m64s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Suffix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for PrefixMut<'_, c64, S, S::m64s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let val = self.simd.vectorize(
            #[inline(always)]
            || self.read_or(unsafe { core::mem::zeroed() }),
        );
        let ptr = self.base;
        f.debug_struct("Prefix")
            .field("value", &val)
            .field("base", &ptr)
            .finish()
    }
}
impl<S: Simd> Debug for SuffixMut<'_, c64, S, S::m64s> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        self.simd.vectorize(
            #[inline(always)]
            || {
                self.read_or(self.simd.c64s_splat(c64 { re: 0.0, im: 0.0 }))
                    .fmt(f)
            },
        )
    }
}

#[derive(Debug)]
#[allow(dead_code)]
pub struct Offset<Mask> {
    prefix_mask: Mask,
    suffix_mask: Mask,

    prefix_offset: usize,

    prefix_len: usize,
    body_len: usize,
    suffix_len: usize,
    simd_body_len: usize,
    total_len: usize,
}

impl Offset<bool> {
    #[inline]
    pub fn unaligned(len: usize) -> Self {
        Scalar::new().i32s_align_offset(core::ptr::null(), len)
    }
}

impl<Mask> Offset<Mask> {
    #[inline(always)]
    pub fn rotate_left_amount(&self) -> usize {
        self.prefix_offset
    }

    #[inline(always)]
    pub fn simd_body_len(&self) -> usize {
        self.simd_body_len
    }
    #[inline(always)]
    pub fn prefix_len(&self) -> usize {
        self.prefix_len
    }
    #[inline(always)]
    pub fn suffix_len(&self) -> usize {
        self.suffix_len
    }

    #[inline(always)]
    pub fn len(&self) -> usize {
        self.total_len
    }
}

impl<Mask: Copy> Clone for Offset<Mask> {
    #[inline]
    fn clone(&self) -> Self {
        *self
    }
}
impl<Mask: Copy> Copy for Offset<Mask> {}

#[inline(always)]
fn align_offset_u64_impl<S: Simd, T, U>(
    simd: S,
    ptr: *const T,
    len: usize,
    align: usize,
    iota: [u64; 32],
) -> Offset<S::m64s> {
    assert_eq!(core::mem::size_of::<U>() % core::mem::size_of::<T>(), 0);
    assert_eq!(core::mem::align_of::<U>(), core::mem::align_of::<T>());

    let chunk_size = core::mem::size_of::<U>() / core::mem::size_of::<T>();

    let align = Ord::max(align, core::mem::align_of::<u64>());

    let iota: S::u64s = cast_lossy(iota);
    let offset = 1 + ptr.wrapping_add(1).align_offset(align) % chunk_size;

    let prefix_offset = chunk_size - offset;
    let prefix_len = Ord::min(offset, len);
    let tail_len = len - prefix_len;
    let body_len = (tail_len.saturating_sub(1)) / chunk_size * chunk_size;
    let suffix_len = tail_len - body_len;

    let vprefix_min = simd.u64s_splat(prefix_offset as u64);
    let vprefix_max = simd.u64s_splat((prefix_offset + prefix_len) as u64);
    let vsuffix_max = simd.u64s_splat(suffix_len as u64);

    Offset {
        // iota >= prefix_offset &&
        // iota - prefix_offset < prefix_len
        prefix_mask: simd.m64s_and(
            simd.m64s_not(simd.u64s_less_than(iota, vprefix_min)),
            simd.u64s_less_than(iota, vprefix_max),
        ),
        // iota - prefix_offset < suffix_len
        suffix_mask: simd.u64s_less_than(iota, vsuffix_max),
        prefix_offset,
        prefix_len,
        body_len,
        suffix_len,
        total_len: len,
        simd_body_len: body_len / chunk_size,
    }
}

#[inline(always)]
fn align_offset_u32_impl<S: Simd, T, U>(
    simd: S,
    ptr: *const T,
    len: usize,
    align: usize,
    iota: [u32; 32],
) -> Offset<S::m32s> {
    assert_eq!(core::mem::size_of::<U>() % core::mem::size_of::<T>(), 0);
    assert_eq!(core::mem::align_of::<U>(), core::mem::align_of::<T>());

    let chunk_size = core::mem::size_of::<U>() / core::mem::size_of::<T>();

    let align = Ord::max(align, core::mem::align_of::<u32>());

    let iota: S::u32s = cast_lossy(iota);
    let offset = 1 + ptr.wrapping_add(1).align_offset(align) % chunk_size;

    let prefix_offset = chunk_size - offset;
    let prefix_len = Ord::min(offset, len);
    let tail_len = len - prefix_len;
    let body_len = (tail_len.saturating_sub(1)) / chunk_size * chunk_size;
    let suffix_len = tail_len - body_len;

    let vprefix_min = simd.u32s_splat(prefix_offset as u32);
    let vprefix_max = simd.u32s_splat((prefix_offset + prefix_len) as u32);
    let vsuffix_max = simd.u32s_splat(suffix_len as u32);

    Offset {
        // iota >= prefix_offset &&
        // iota - prefix_offset < prefix_len
        prefix_mask: simd.m32s_and(
            simd.m32s_not(simd.u32s_less_than(iota, vprefix_min)),
            simd.u32s_less_than(iota, vprefix_max),
        ),
        // iota - prefix_offset < suffix_len
        suffix_mask: simd.u32s_less_than(iota, vsuffix_max),
        prefix_offset,
        prefix_len,
        body_len,
        simd_body_len: body_len / chunk_size,
        suffix_len,
        total_len: len,
    }
}

#[inline(always)]
fn align_offset_u64x2<S: Simd, T, U>(
    simd: S,
    ptr: *const T,
    len: usize,
    align: usize,
) -> Offset<S::m64s> {
    align_offset_u64_impl::<S, T, U>(
        simd,
        ptr,
        len,
        align,
        [
            0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13,
            13, 14, 14, 15, 15u64,
        ],
    )
}

#[inline(always)]
fn align_offset_u64<S: Simd, T, U>(
    simd: S,
    ptr: *const T,
    len: usize,
    align: usize,
) -> Offset<S::m64s> {
    align_offset_u64_impl::<S, T, U>(
        simd,
        ptr,
        len,
        align,
        [
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
            24, 25, 26, 27, 28, 29, 30, 31u64,
        ],
    )
}

#[inline(always)]
fn align_offset_u32x2<S: Simd, T, U>(
    simd: S,
    ptr: *const T,
    len: usize,
    align: usize,
) -> Offset<S::m32s> {
    align_offset_u32_impl::<S, T, U>(
        simd,
        ptr,
        len,
        align,
        [
            0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13,
            13, 14, 14, 15, 15u32,
        ],
    )
}

#[inline(always)]
fn align_offset_u32<S: Simd, T, U>(
    simd: S,
    ptr: *const T,
    len: usize,
    align: usize,
) -> Offset<S::m32s> {
    align_offset_u32_impl::<S, T, U>(
        simd,
        ptr,
        len,
        align,
        [
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
            24, 25, 26, 27, 28, 29, 30, 31u32,
        ],
    )
}

#[inline(always)]
#[track_caller]
unsafe fn split_slice_aligned_like<S: Simd, Mask: Copy, T, U>(
    simd: S,
    slice: &[T],
    offset: Offset<Mask>,
) -> (Prefix<T, S, Mask>, &[U], Suffix<T, S, Mask>) {
    assert_eq!(core::mem::size_of::<U>() % core::mem::size_of::<T>(), 0);
    assert_eq!(core::mem::align_of::<U>(), core::mem::align_of::<T>());

    assert_eq!(slice.len(), offset.total_len);

    let data = slice.as_ptr();

    (
        Prefix {
            simd,
            mask: offset.prefix_mask,
            base: data.wrapping_sub(offset.prefix_offset),
            __marker: PhantomData,
        },
        from_raw_parts(
            data.add(offset.prefix_len) as *const U,
            offset.simd_body_len,
        ),
        Suffix {
            simd,
            mask: offset.suffix_mask,
            base: data.add(offset.prefix_len + offset.body_len),
            __marker: PhantomData,
        },
    )
}

#[inline(always)]
#[track_caller]
unsafe fn split_mut_slice_aligned_like<S: Simd, Mask: Copy, T, U>(
    simd: S,
    slice: &mut [T],
    offset: Offset<Mask>,
) -> (PrefixMut<T, S, Mask>, &mut [U], SuffixMut<T, S, Mask>) {
    assert_eq!(core::mem::size_of::<U>() % core::mem::size_of::<T>(), 0);
    assert_eq!(core::mem::align_of::<U>(), core::mem::align_of::<T>());

    assert_eq!(slice.len(), offset.total_len);

    let data = slice.as_mut_ptr();
    let chunk_size = core::mem::size_of::<U>() / core::mem::size_of::<T>();

    (
        PrefixMut {
            simd,
            mask: offset.prefix_mask,
            base: data.wrapping_sub(offset.prefix_offset),
            __marker: PhantomData,
        },
        from_raw_parts_mut(
            data.add(offset.prefix_len) as *mut U,
            offset.body_len / chunk_size,
        ),
        SuffixMut {
            simd,
            mask: offset.suffix_mask,
            base: data.add(offset.prefix_len + offset.body_len),
            __marker: PhantomData,
        },
    )
}

#[inline(always)]
unsafe fn split_slice<T, U>(slice: &[T]) -> (&[U], &[T]) {
    assert_eq!(core::mem::size_of::<U>() % core::mem::size_of::<T>(), 0);
    assert_eq!(core::mem::align_of::<U>(), core::mem::align_of::<T>());

    let chunk_size = core::mem::size_of::<U>() / core::mem::size_of::<T>();

    let len = slice.len();
    let data = slice.as_ptr();

    let div = len / chunk_size;
    let rem = len % chunk_size;
    (
        from_raw_parts(data as *const U, div),
        from_raw_parts(data.add(len - rem), rem),
    )
}

#[inline(always)]
unsafe fn split_mut_slice<T, U>(slice: &mut [T]) -> (&mut [U], &mut [T]) {
    assert_eq!(core::mem::size_of::<U>() % core::mem::size_of::<T>(), 0);
    assert_eq!(core::mem::align_of::<U>(), core::mem::align_of::<T>());

    let chunk_size = core::mem::size_of::<U>() / core::mem::size_of::<T>();

    let len = slice.len();
    let data = slice.as_mut_ptr();

    let div = len / chunk_size;
    let rem = len % chunk_size;
    (
        from_raw_parts_mut(data as *mut U, div),
        from_raw_parts_mut(data.add(len - rem), rem),
    )
}

#[inline(always)]
unsafe fn rsplit_slice<T, U>(slice: &[T]) -> (&[T], &[U]) {
    assert_eq!(core::mem::size_of::<U>() % core::mem::size_of::<T>(), 0);
    assert_eq!(core::mem::align_of::<U>(), core::mem::align_of::<T>());

    let chunk_size = core::mem::size_of::<U>() / core::mem::size_of::<T>();

    let len = slice.len();
    let data = slice.as_ptr();

    let div = len / chunk_size;
    let rem = len % chunk_size;
    (
        from_raw_parts(data, rem),
        from_raw_parts(data.add(rem) as *const U, div),
    )
}

#[inline(always)]
unsafe fn rsplit_mut_slice<T, U>(slice: &mut [T]) -> (&mut [T], &mut [U]) {
    assert_eq!(core::mem::size_of::<U>() % core::mem::size_of::<T>(), 0);
    assert_eq!(core::mem::align_of::<U>(), core::mem::align_of::<T>());

    let chunk_size = core::mem::size_of::<U>() / core::mem::size_of::<T>();

    let len = slice.len();
    let data = slice.as_mut_ptr();

    let div = len / chunk_size;
    let rem = len % chunk_size;
    (
        from_raw_parts_mut(data, rem),
        from_raw_parts_mut(data.add(rem) as *mut U, div),
    )
}

#[cfg(not(any(target_arch = "aarch64", target_arch = "x86", target_arch = "x86_64")))]
#[derive(Debug, Clone, Copy)]
#[non_exhaustive]
#[repr(u8)]
enum ArchInner {
    Scalar = 0,
    // improves codegen for some reason
    #[allow(dead_code)]
    Dummy = u8::MAX - 1,
}

#[cfg(not(any(target_arch = "aarch64", target_arch = "x86", target_arch = "x86_64")))]
impl ArchInner {
    #[inline]
    pub fn new() -> Self {
        Self::Scalar
    }

    #[inline(always)]
    pub fn dispatch<Op: WithSimd>(self, op: Op) -> Op::Output {
        match self {
            ArchInner::Scalar => crate::Scalar::new().vectorize(op),
            ArchInner::Dummy => unsafe { core::hint::unreachable_unchecked() },
        }
    }
}

#[cfg(target_arch = "aarch64")]
use aarch64::ArchInner;
#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
use x86::ArchInner;

impl Arch {
    #[inline(always)]
    fn __static_available() -> &'static ::core::sync::atomic::AtomicU8 {
        static AVAILABLE: ::core::sync::atomic::AtomicU8 =
            ::core::sync::atomic::AtomicU8::new(u8::MAX);
        &AVAILABLE
    }

    #[inline(never)]
    fn __detect_is_available() -> u8 {
        let out = unsafe {
            core::mem::transmute(Self {
                inner: ArchInner::new(),
            })
        };
        Self::__static_available().store(out, ::core::sync::atomic::Ordering::Relaxed);
        out
    }

    #[inline(always)]
    pub fn new() -> Self {
        let mut available =
            Self::__static_available().load(::core::sync::atomic::Ordering::Relaxed);
        if available == u8::MAX {
            available = Self::__detect_is_available();
        }

        unsafe { core::mem::transmute(available) }
    }
    #[inline(always)]
    pub fn dispatch<Op: WithSimd>(self, op: Op) -> Op::Output {
        self.inner.dispatch(op)
    }
}

impl Default for Arch {
    #[inline]
    fn default() -> Self {
        Self::new()
    }
}

impl Default for ScalarArch {
    #[inline]
    fn default() -> Self {
        Self::new()
    }
}

#[derive(Debug, Clone, Copy)]
#[repr(transparent)]
pub struct Arch {
    inner: ArchInner,
}

#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
use x86::ScalarArchInner;

#[cfg(not(any(target_arch = "x86", target_arch = "x86_64")))]
#[repr(u8)]
#[derive(Debug, Clone, Copy)]
enum ScalarArchInner {
    Scalar = 0,
    // improves codegen for some reason
    #[allow(dead_code)]
    Dummy = u8::MAX - 1,
}

#[cfg(not(any(target_arch = "x86", target_arch = "x86_64")))]
impl ScalarArchInner {
    #[inline]
    pub fn new() -> Self {
        Self::Scalar
    }

    #[inline(always)]
    pub fn dispatch<Op: WithSimd>(self, op: Op) -> Op::Output {
        match self {
            ScalarArchInner::Scalar => crate::Scalar::new().vectorize(op),
            ScalarArchInner::Dummy => unsafe { core::hint::unreachable_unchecked() },
        }
    }
}

#[derive(Debug, Clone, Copy)]
#[repr(transparent)]
pub struct ScalarArch {
    inner: ScalarArchInner,
}

impl ScalarArch {
    #[inline(always)]
    fn __static_available() -> &'static ::core::sync::atomic::AtomicU8 {
        static AVAILABLE: ::core::sync::atomic::AtomicU8 =
            ::core::sync::atomic::AtomicU8::new(u8::MAX);
        &AVAILABLE
    }

    #[inline(never)]
    fn __detect_is_available() -> u8 {
        let out = unsafe {
            core::mem::transmute(Self {
                inner: ScalarArchInner::new(),
            })
        };
        Self::__static_available().store(out, ::core::sync::atomic::Ordering::Relaxed);
        out
    }

    #[inline(always)]
    pub fn new() -> Self {
        let mut available =
            Self::__static_available().load(::core::sync::atomic::Ordering::Relaxed);
        if available == u8::MAX {
            available = Self::__detect_is_available();
        }

        unsafe { core::mem::transmute(available) }
    }
    #[inline(always)]
    pub fn dispatch<Op: WithSimd>(self, op: Op) -> Op::Output {
        self.inner.dispatch(op)
    }
}

#[doc(hidden)]
pub struct CheckSameSize<T, U>(PhantomData<(T, U)>);
impl<T, U> CheckSameSize<T, U> {
    pub const VALID: () = {
        assert!(core::mem::size_of::<T>() == core::mem::size_of::<U>());
    };
}

#[doc(hidden)]
pub struct CheckSizeLessThanOrEqual<T, U>(PhantomData<(T, U)>);
impl<T, U> CheckSizeLessThanOrEqual<T, U> {
    pub const VALID: () = {
        assert!(core::mem::size_of::<T>() <= core::mem::size_of::<U>());
    };
}

#[macro_export]
macro_rules! static_assert_same_size {
    ($t: ty, $u: ty) => {
        let _ = $crate::CheckSameSize::<$t, $u>::VALID;
    };
}
#[macro_export]
macro_rules! static_assert_size_less_than_or_equal {
    ($t: ty, $u: ty) => {
        let _ = $crate::CheckSizeLessThanOrEqual::<$t, $u>::VALID;
    };
}

/// Safe transmute function.
///
/// This function asserts at compile time that the two types have the same size.
#[inline(always)]
pub fn cast<T: NoUninit, U: AnyBitPattern>(value: T) -> U {
    static_assert_same_size!(T, U);
    let value = core::mem::ManuallyDrop::new(value);
    let ptr = &value as *const core::mem::ManuallyDrop<T> as *const U;
    unsafe { ptr.read_unaligned() }
}

/// Safe lossy transmute function, where the destination type may be smaller than the source type.
///
/// This property is checked at compile time.
#[inline(always)]
pub fn cast_lossy<T: NoUninit, U: AnyBitPattern>(value: T) -> U {
    static_assert_size_less_than_or_equal!(U, T);
    let value = core::mem::ManuallyDrop::new(value);
    let ptr = &value as *const core::mem::ManuallyDrop<T> as *const U;
    unsafe { ptr.read_unaligned() }
}

/// Splits a slice into chunks of equal size (known at compile time).
///
/// Returns the chunks and the remaining section of the input slice.
#[inline(always)]
pub fn as_arrays<const N: usize, T>(slice: &[T]) -> (&[[T; N]], &[T]) {
    let n = slice.len();
    let mid_div_n = n / N;
    let mid = mid_div_n * N;
    let ptr = slice.as_ptr();
    unsafe {
        (
            from_raw_parts(ptr as *const [T; N], mid_div_n),
            from_raw_parts(ptr.add(mid), n - mid),
        )
    }
}

/// Splits a slice into chunks of equal size (known at compile time).
///
/// Returns the chunks and the remaining section of the input slice.
#[inline(always)]
pub fn as_arrays_mut<const N: usize, T>(slice: &mut [T]) -> (&mut [[T; N]], &mut [T]) {
    let n = slice.len();
    let mid_div_n = n / N;
    let mid = mid_div_n * N;
    let ptr = slice.as_mut_ptr();
    unsafe {
        (
            from_raw_parts_mut(ptr as *mut [T; N], mid_div_n),
            from_raw_parts_mut(ptr.add(mid), n - mid),
        )
    }
}

/// Platform dependent intrinsics.
#[doc(hidden)]
pub mod core_arch;

#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
#[cfg_attr(docsrs, doc(cfg(any(target_arch = "x86", target_arch = "x86_64"))))]
/// Low level x86 API.
pub mod x86;

#[cfg(target_arch = "aarch64")]
#[cfg_attr(docsrs, doc(cfg(target_arch = "aarch64")))]
/// Low level aarch64 API.
pub mod aarch64;

/// Mask type with 8 bits. Its bit either all ones or all zeros.
#[derive(Copy, Clone, PartialEq, Eq)]
#[repr(transparent)]
pub struct m8(u8);
/// Mask type with 16 bits. Its bit either all ones or all zeros.
#[derive(Copy, Clone, PartialEq, Eq)]
#[repr(transparent)]
pub struct m16(u16);
/// Mask type with 32 bits. Its bit either all ones or all zeros.
#[derive(Copy, Clone, PartialEq, Eq)]
#[repr(transparent)]
pub struct m32(u32);
/// Mask type with 64 bits. Its bit either all ones or all zeros.
#[derive(Copy, Clone, PartialEq, Eq)]
#[repr(transparent)]
pub struct m64(u64);

/// Bitmask type for 8 elements, used for mask operations on AVX512.
#[derive(Copy, Clone, PartialEq, Eq)]
#[repr(transparent)]
pub struct b8(pub u8);
/// Bitmask type for 16 elements, used for mask operations on AVX512.
#[derive(Copy, Clone, PartialEq, Eq)]
#[repr(transparent)]
pub struct b16(pub u16);
/// Bitmask type for 32 elements, used for mask operations on AVX512.
#[derive(Copy, Clone, PartialEq, Eq)]
#[repr(transparent)]
pub struct b32(pub u32);
/// Bitmask type for 64 elements, used for mask operations on AVX512.
#[derive(Copy, Clone, PartialEq, Eq)]
#[repr(transparent)]
pub struct b64(pub u64);

impl core::ops::Not for b8 {
    type Output = b8;
    #[inline(always)]
    fn not(self) -> Self::Output {
        b8(!self.0)
    }
}
impl core::ops::BitAnd for b8 {
    type Output = b8;
    #[inline(always)]
    fn bitand(self, rhs: Self) -> Self::Output {
        b8(self.0 & rhs.0)
    }
}
impl core::ops::BitOr for b8 {
    type Output = b8;
    #[inline(always)]
    fn bitor(self, rhs: Self) -> Self::Output {
        b8(self.0 | rhs.0)
    }
}
impl core::ops::BitXor for b8 {
    type Output = b8;
    #[inline(always)]
    fn bitxor(self, rhs: Self) -> Self::Output {
        b8(self.0 ^ rhs.0)
    }
}

impl core::ops::Not for b16 {
    type Output = b16;
    #[inline(always)]
    fn not(self) -> Self::Output {
        b16(!self.0)
    }
}
impl core::ops::BitAnd for b16 {
    type Output = b16;
    #[inline(always)]
    fn bitand(self, rhs: Self) -> Self::Output {
        b16(self.0 & rhs.0)
    }
}
impl core::ops::BitOr for b16 {
    type Output = b16;
    #[inline(always)]
    fn bitor(self, rhs: Self) -> Self::Output {
        b16(self.0 | rhs.0)
    }
}
impl core::ops::BitXor for b16 {
    type Output = b16;
    #[inline(always)]
    fn bitxor(self, rhs: Self) -> Self::Output {
        b16(self.0 ^ rhs.0)
    }
}

impl core::ops::Not for b32 {
    type Output = b32;
    #[inline(always)]
    fn not(self) -> Self::Output {
        b32(!self.0)
    }
}
impl core::ops::BitAnd for b32 {
    type Output = b32;
    #[inline(always)]
    fn bitand(self, rhs: Self) -> Self::Output {
        b32(self.0 & rhs.0)
    }
}
impl core::ops::BitOr for b32 {
    type Output = b32;
    #[inline(always)]
    fn bitor(self, rhs: Self) -> Self::Output {
        b32(self.0 | rhs.0)
    }
}
impl core::ops::BitXor for b32 {
    type Output = b32;
    #[inline(always)]
    fn bitxor(self, rhs: Self) -> Self::Output {
        b32(self.0 ^ rhs.0)
    }
}

impl core::ops::Not for b64 {
    type Output = b64;
    #[inline(always)]
    fn not(self) -> Self::Output {
        b64(!self.0)
    }
}
impl core::ops::BitAnd for b64 {
    type Output = b64;
    #[inline(always)]
    fn bitand(self, rhs: Self) -> Self::Output {
        b64(self.0 & rhs.0)
    }
}
impl core::ops::BitOr for b64 {
    type Output = b64;
    #[inline(always)]
    fn bitor(self, rhs: Self) -> Self::Output {
        b64(self.0 | rhs.0)
    }
}
impl core::ops::BitXor for b64 {
    type Output = b64;
    #[inline(always)]
    fn bitxor(self, rhs: Self) -> Self::Output {
        b64(self.0 ^ rhs.0)
    }
}

impl Debug for b8 {
    fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
        #[allow(dead_code)]
        #[derive(Copy, Clone, Debug)]
        struct b8(bool, bool, bool, bool, bool, bool, bool, bool);
        b8(
            ((self.0 >> 0) & 1) == 1,
            ((self.0 >> 1) & 1) == 1,
            ((self.0 >> 2) & 1) == 1,
            ((self.0 >> 3) & 1) == 1,
            ((self.0 >> 4) & 1) == 1,
            ((self.0 >> 5) & 1) == 1,
            ((self.0 >> 6) & 1) == 1,
            ((self.0 >> 7) & 1) == 1,
        )
        .fmt(f)
    }
}
impl Debug for b16 {
    fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
        #[allow(dead_code)]
        #[derive(Copy, Clone, Debug)]
        struct b16(
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
        );
        b16(
            ((self.0 >> 00) & 1) == 1,
            ((self.0 >> 01) & 1) == 1,
            ((self.0 >> 02) & 1) == 1,
            ((self.0 >> 03) & 1) == 1,
            ((self.0 >> 04) & 1) == 1,
            ((self.0 >> 05) & 1) == 1,
            ((self.0 >> 06) & 1) == 1,
            ((self.0 >> 07) & 1) == 1,
            ((self.0 >> 08) & 1) == 1,
            ((self.0 >> 09) & 1) == 1,
            ((self.0 >> 10) & 1) == 1,
            ((self.0 >> 11) & 1) == 1,
            ((self.0 >> 12) & 1) == 1,
            ((self.0 >> 13) & 1) == 1,
            ((self.0 >> 14) & 1) == 1,
            ((self.0 >> 15) & 1) == 1,
        )
        .fmt(f)
    }
}
impl Debug for b32 {
    fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
        #[allow(dead_code)]
        #[derive(Copy, Clone, Debug)]
        struct b32(
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
        );
        b32(
            ((self.0 >> 00) & 1) == 1,
            ((self.0 >> 01) & 1) == 1,
            ((self.0 >> 02) & 1) == 1,
            ((self.0 >> 03) & 1) == 1,
            ((self.0 >> 04) & 1) == 1,
            ((self.0 >> 05) & 1) == 1,
            ((self.0 >> 06) & 1) == 1,
            ((self.0 >> 07) & 1) == 1,
            ((self.0 >> 08) & 1) == 1,
            ((self.0 >> 09) & 1) == 1,
            ((self.0 >> 10) & 1) == 1,
            ((self.0 >> 11) & 1) == 1,
            ((self.0 >> 12) & 1) == 1,
            ((self.0 >> 13) & 1) == 1,
            ((self.0 >> 14) & 1) == 1,
            ((self.0 >> 15) & 1) == 1,
            ((self.0 >> 16) & 1) == 1,
            ((self.0 >> 17) & 1) == 1,
            ((self.0 >> 18) & 1) == 1,
            ((self.0 >> 19) & 1) == 1,
            ((self.0 >> 20) & 1) == 1,
            ((self.0 >> 21) & 1) == 1,
            ((self.0 >> 22) & 1) == 1,
            ((self.0 >> 23) & 1) == 1,
            ((self.0 >> 24) & 1) == 1,
            ((self.0 >> 25) & 1) == 1,
            ((self.0 >> 26) & 1) == 1,
            ((self.0 >> 27) & 1) == 1,
            ((self.0 >> 28) & 1) == 1,
            ((self.0 >> 29) & 1) == 1,
            ((self.0 >> 30) & 1) == 1,
            ((self.0 >> 31) & 1) == 1,
        )
        .fmt(f)
    }
}
impl Debug for b64 {
    fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
        #[allow(dead_code)]
        #[derive(Copy, Clone, Debug)]
        struct b64(
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
            bool,
        );
        b64(
            ((self.0 >> 00) & 1) == 1,
            ((self.0 >> 01) & 1) == 1,
            ((self.0 >> 02) & 1) == 1,
            ((self.0 >> 03) & 1) == 1,
            ((self.0 >> 04) & 1) == 1,
            ((self.0 >> 05) & 1) == 1,
            ((self.0 >> 06) & 1) == 1,
            ((self.0 >> 07) & 1) == 1,
            ((self.0 >> 08) & 1) == 1,
            ((self.0 >> 09) & 1) == 1,
            ((self.0 >> 10) & 1) == 1,
            ((self.0 >> 11) & 1) == 1,
            ((self.0 >> 12) & 1) == 1,
            ((self.0 >> 13) & 1) == 1,
            ((self.0 >> 14) & 1) == 1,
            ((self.0 >> 15) & 1) == 1,
            ((self.0 >> 16) & 1) == 1,
            ((self.0 >> 17) & 1) == 1,
            ((self.0 >> 18) & 1) == 1,
            ((self.0 >> 19) & 1) == 1,
            ((self.0 >> 20) & 1) == 1,
            ((self.0 >> 21) & 1) == 1,
            ((self.0 >> 22) & 1) == 1,
            ((self.0 >> 23) & 1) == 1,
            ((self.0 >> 24) & 1) == 1,
            ((self.0 >> 25) & 1) == 1,
            ((self.0 >> 26) & 1) == 1,
            ((self.0 >> 27) & 1) == 1,
            ((self.0 >> 28) & 1) == 1,
            ((self.0 >> 29) & 1) == 1,
            ((self.0 >> 30) & 1) == 1,
            ((self.0 >> 31) & 1) == 1,
            ((self.0 >> 32) & 1) == 1,
            ((self.0 >> 33) & 1) == 1,
            ((self.0 >> 34) & 1) == 1,
            ((self.0 >> 35) & 1) == 1,
            ((self.0 >> 36) & 1) == 1,
            ((self.0 >> 37) & 1) == 1,
            ((self.0 >> 38) & 1) == 1,
            ((self.0 >> 39) & 1) == 1,
            ((self.0 >> 40) & 1) == 1,
            ((self.0 >> 41) & 1) == 1,
            ((self.0 >> 42) & 1) == 1,
            ((self.0 >> 43) & 1) == 1,
            ((self.0 >> 44) & 1) == 1,
            ((self.0 >> 45) & 1) == 1,
            ((self.0 >> 46) & 1) == 1,
            ((self.0 >> 47) & 1) == 1,
            ((self.0 >> 48) & 1) == 1,
            ((self.0 >> 49) & 1) == 1,
            ((self.0 >> 50) & 1) == 1,
            ((self.0 >> 51) & 1) == 1,
            ((self.0 >> 52) & 1) == 1,
            ((self.0 >> 53) & 1) == 1,
            ((self.0 >> 54) & 1) == 1,
            ((self.0 >> 55) & 1) == 1,
            ((self.0 >> 56) & 1) == 1,
            ((self.0 >> 57) & 1) == 1,
            ((self.0 >> 58) & 1) == 1,
            ((self.0 >> 59) & 1) == 1,
            ((self.0 >> 60) & 1) == 1,
            ((self.0 >> 61) & 1) == 1,
            ((self.0 >> 62) & 1) == 1,
            ((self.0 >> 63) & 1) == 1,
        )
        .fmt(f)
    }
}

impl Debug for m8 {
    #[inline]
    fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
        self.is_set().fmt(f)
    }
}
impl Debug for m16 {
    #[inline]
    fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
        self.is_set().fmt(f)
    }
}
impl Debug for m32 {
    #[inline]
    fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
        self.is_set().fmt(f)
    }
}
impl Debug for m64 {
    #[inline]
    fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
        self.is_set().fmt(f)
    }
}

impl m8 {
    /// Returns a mask with all bits set one, if `flag` is true, otherwise returns a mask with all
    /// bits set to zero.
    #[inline(always)]
    pub const fn new(flag: bool) -> Self {
        Self(if flag { u8::MAX } else { 0 })
    }

    /// Returns `false` if the mask bits are all zero, otherwise returns `true`.
    #[inline(always)]
    pub const fn is_set(self) -> bool {
        self.0 != 0
    }
}
impl m16 {
    /// Returns a mask with all bits set one, if `flag` is true, otherwise returns a mask with all
    /// bits set to zero.
    #[inline(always)]
    pub const fn new(flag: bool) -> Self {
        Self(if flag { u16::MAX } else { 0 })
    }

    /// Returns `false` if the mask bits are all zero, otherwise returns `true`.
    #[inline(always)]
    pub const fn is_set(self) -> bool {
        self.0 != 0
    }
}
impl m32 {
    /// Returns a mask with all bits set one, if `flag` is true, otherwise returns a mask with all
    /// bits set to zero.
    #[inline(always)]
    pub const fn new(flag: bool) -> Self {
        Self(if flag { u32::MAX } else { 0 })
    }

    /// Returns `false` if the mask bits are all zero, otherwise returns `true`.
    #[inline(always)]
    pub const fn is_set(self) -> bool {
        self.0 != 0
    }
}
impl m64 {
    /// Returns a mask with all bits set one, if `flag` is true, otherwise returns a mask with all
    /// bits set to zero.
    #[inline(always)]
    pub const fn new(flag: bool) -> Self {
        Self(if flag { u64::MAX } else { 0 })
    }

    /// Returns `false` if the mask bits are all zero, otherwise returns `true`.
    #[inline(always)]
    pub const fn is_set(self) -> bool {
        self.0 != 0
    }
}

/// A 128-bit SIMD vector with 16 elements of type [`i8`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i8x16(
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
);
/// A 256-bit SIMD vector with 32 elements of type [`i8`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i8x32(
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
);
/// A 512-bit SIMD vector with 64 elements of type [`i8`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i8x64(
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
    pub i8,
);

/// A 128-bit SIMD vector with 16 elements of type [`u8`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u8x16(
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
);
/// A 256-bit SIMD vector with 32 elements of type [`u8`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u8x32(
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
);
/// A 512-bit SIMD vector with 64 elements of type [`u8`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u8x64(
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
    pub u8,
);

/// A 128-bit SIMD vector with 16 elements of type [`m8`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m8x16(
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
);
/// A 256-bit SIMD vector with 32 elements of type [`m8`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m8x32(
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
    pub m8,
);

/// A 128-bit SIMD vector with 8 elements of type [`i16`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i16x8(
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
);
/// A 256-bit SIMD vector with 16 elements of type [`i16`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i16x16(
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
);
/// A 512-bit SIMD vector with 32 elements of type [`i16`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i16x32(
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
    pub i16,
);

/// A 128-bit SIMD vector with 8 elements of type [`u16`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u16x8(
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
);
/// A 256-bit SIMD vector with 16 elements of type [`u16`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u16x16(
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
);
/// A 512-bit SIMD vector with 32 elements of type [`u16`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u16x32(
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
    pub u16,
);

/// A 128-bit SIMD vector with 8 elements of type [`m16`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m16x8(
    pub m16,
    pub m16,
    pub m16,
    pub m16,
    pub m16,
    pub m16,
    pub m16,
    pub m16,
);
/// A 256-bit SIMD vector with 16 elements of type [`m16`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m16x16(
    pub m16,
    pub m16,
    pub m16,
    pub m16,
    pub m16,
    pub m16,
    pub m16,
    pub m16,
    pub m16,
    pub m16,
    pub m16,
    pub m16,
    pub m16,
    pub m16,
    pub m16,
    pub m16,
);

/// A 128-bit SIMD vector with 4 elements of type [`f32`].
#[derive(Debug, Copy, Clone, PartialEq)]
#[repr(C)]
pub struct f32x4(pub f32, pub f32, pub f32, pub f32);
/// A 256-bit SIMD vector with 8 elements of type [`f32`].
#[derive(Debug, Copy, Clone, PartialEq)]
#[repr(C)]
pub struct f32x8(
    pub f32,
    pub f32,
    pub f32,
    pub f32,
    pub f32,
    pub f32,
    pub f32,
    pub f32,
);
/// A 512-bit SIMD vector with 16 elements of type [`f32`].
#[derive(Debug, Copy, Clone, PartialEq)]
#[repr(C)]
pub struct f32x16(
    pub f32,
    pub f32,
    pub f32,
    pub f32,
    pub f32,
    pub f32,
    pub f32,
    pub f32,
    pub f32,
    pub f32,
    pub f32,
    pub f32,
    pub f32,
    pub f32,
    pub f32,
    pub f32,
);

/// A 128-bit SIMD vector with 4 elements of type [`i32`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i32x4(pub i32, pub i32, pub i32, pub i32);
/// A 256-bit SIMD vector with 8 elements of type [`i32`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i32x8(
    pub i32,
    pub i32,
    pub i32,
    pub i32,
    pub i32,
    pub i32,
    pub i32,
    pub i32,
);
/// A 512-bit SIMD vector with 16 elements of type [`i32`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i32x16(
    pub i32,
    pub i32,
    pub i32,
    pub i32,
    pub i32,
    pub i32,
    pub i32,
    pub i32,
    pub i32,
    pub i32,
    pub i32,
    pub i32,
    pub i32,
    pub i32,
    pub i32,
    pub i32,
);

/// A 128-bit SIMD vector with 4 elements of type [`u32`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u32x4(pub u32, pub u32, pub u32, pub u32);
/// A 256-bit SIMD vector with 8 elements of type [`u32`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u32x8(
    pub u32,
    pub u32,
    pub u32,
    pub u32,
    pub u32,
    pub u32,
    pub u32,
    pub u32,
);
/// A 512-bit SIMD vector with 16 elements of type [`u32`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u32x16(
    pub u32,
    pub u32,
    pub u32,
    pub u32,
    pub u32,
    pub u32,
    pub u32,
    pub u32,
    pub u32,
    pub u32,
    pub u32,
    pub u32,
    pub u32,
    pub u32,
    pub u32,
    pub u32,
);

/// A 128-bit SIMD vector with 4 elements of type [`m32`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m32x4(pub m32, pub m32, pub m32, pub m32);
/// A 256-bit SIMD vector with 8 elements of type [`m32`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m32x8(
    pub m32,
    pub m32,
    pub m32,
    pub m32,
    pub m32,
    pub m32,
    pub m32,
    pub m32,
);

/// A 128-bit SIMD vector with 2 elements of type [`f64`].
#[derive(Debug, Copy, Clone, PartialEq)]
#[repr(C)]
pub struct f64x2(pub f64, pub f64);
/// A 256-bit SIMD vector with 4 elements of type [`f64`].
#[derive(Debug, Copy, Clone, PartialEq)]
#[repr(C)]
pub struct f64x4(pub f64, pub f64, pub f64, pub f64);
/// A 512-bit SIMD vector with 8 elements of type [`f64`].
#[derive(Debug, Copy, Clone, PartialEq)]
#[repr(C)]
pub struct f64x8(
    pub f64,
    pub f64,
    pub f64,
    pub f64,
    pub f64,
    pub f64,
    pub f64,
    pub f64,
);

/// A 128-bit SIMD vector with 2 elements of type [`i64`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i64x2(pub i64, pub i64);
/// A 256-bit SIMD vector with 4 elements of type [`i64`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i64x4(pub i64, pub i64, pub i64, pub i64);
/// A 512-bit SIMD vector with 8 elements of type [`i64`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct i64x8(
    pub i64,
    pub i64,
    pub i64,
    pub i64,
    pub i64,
    pub i64,
    pub i64,
    pub i64,
);

/// A 128-bit SIMD vector with 2 elements of type [`u64`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u64x2(pub u64, pub u64);
/// A 256-bit SIMD vector with 4 elements of type [`u64`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u64x4(pub u64, pub u64, pub u64, pub u64);
/// A 512-bit SIMD vector with 8 elements of type [`u64`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct u64x8(
    pub u64,
    pub u64,
    pub u64,
    pub u64,
    pub u64,
    pub u64,
    pub u64,
    pub u64,
);

/// A 128-bit SIMD vector with 2 elements of type [`m64`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m64x2(pub m64, pub m64);
/// A 256-bit SIMD vector with 4 elements of type [`m64`].
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct m64x4(pub m64, pub m64, pub m64, pub m64);

unsafe impl Zeroable for m8 {}
unsafe impl Zeroable for m16 {}
unsafe impl Zeroable for m32 {}
unsafe impl Zeroable for m64 {}
unsafe impl NoUninit for m8 {}
unsafe impl NoUninit for m16 {}
unsafe impl NoUninit for m32 {}
unsafe impl NoUninit for m64 {}

unsafe impl Zeroable for b8 {}
unsafe impl Pod for b8 {}
unsafe impl Zeroable for b16 {}
unsafe impl Pod for b16 {}
unsafe impl Zeroable for b32 {}
unsafe impl Pod for b32 {}
unsafe impl Zeroable for b64 {}
unsafe impl Pod for b64 {}

unsafe impl Zeroable for i8x16 {}
unsafe impl Zeroable for i8x32 {}
unsafe impl Zeroable for i8x64 {}
unsafe impl Pod for i8x16 {}
unsafe impl Pod for i8x32 {}
unsafe impl Pod for i8x64 {}
unsafe impl Zeroable for u8x16 {}
unsafe impl Zeroable for u8x32 {}
unsafe impl Zeroable for u8x64 {}
unsafe impl Pod for u8x16 {}
unsafe impl Pod for u8x32 {}
unsafe impl Pod for u8x64 {}
unsafe impl Zeroable for m8x16 {}
unsafe impl Zeroable for m8x32 {}
unsafe impl NoUninit for m8x16 {}
unsafe impl NoUninit for m8x32 {}

unsafe impl Zeroable for i16x8 {}
unsafe impl Zeroable for i16x16 {}
unsafe impl Zeroable for i16x32 {}
unsafe impl Pod for i16x8 {}
unsafe impl Pod for i16x16 {}
unsafe impl Pod for i16x32 {}
unsafe impl Zeroable for u16x8 {}
unsafe impl Zeroable for u16x16 {}
unsafe impl Zeroable for u16x32 {}
unsafe impl Pod for u16x8 {}
unsafe impl Pod for u16x16 {}
unsafe impl Pod for u16x32 {}
unsafe impl Zeroable for m16x8 {}
unsafe impl Zeroable for m16x16 {}
unsafe impl NoUninit for m16x8 {}
unsafe impl NoUninit for m16x16 {}

unsafe impl Zeroable for f32x4 {}
unsafe impl Zeroable for f32x8 {}
unsafe impl Zeroable for f32x16 {}
unsafe impl Pod for f32x4 {}
unsafe impl Pod for f32x8 {}
unsafe impl Pod for f32x16 {}
unsafe impl Zeroable for i32x4 {}
unsafe impl Zeroable for i32x8 {}
unsafe impl Zeroable for i32x16 {}
unsafe impl Pod for i32x4 {}
unsafe impl Pod for i32x8 {}
unsafe impl Pod for i32x16 {}
unsafe impl Zeroable for u32x4 {}
unsafe impl Zeroable for u32x8 {}
unsafe impl Zeroable for u32x16 {}
unsafe impl Pod for u32x4 {}
unsafe impl Pod for u32x8 {}
unsafe impl Pod for u32x16 {}
unsafe impl Zeroable for m32x4 {}
unsafe impl Zeroable for m32x8 {}
unsafe impl NoUninit for m32x4 {}
unsafe impl NoUninit for m32x8 {}

unsafe impl Zeroable for f64x2 {}
unsafe impl Zeroable for f64x4 {}
unsafe impl Zeroable for f64x8 {}
unsafe impl Pod for f64x2 {}
unsafe impl Pod for f64x4 {}
unsafe impl Pod for f64x8 {}
unsafe impl Zeroable for i64x2 {}
unsafe impl Zeroable for i64x4 {}
unsafe impl Zeroable for i64x8 {}
unsafe impl Pod for i64x2 {}
unsafe impl Pod for i64x4 {}
unsafe impl Pod for i64x8 {}
unsafe impl Zeroable for u64x2 {}
unsafe impl Zeroable for u64x4 {}
unsafe impl Zeroable for u64x8 {}
unsafe impl Pod for u64x2 {}
unsafe impl Pod for u64x4 {}
unsafe impl Pod for u64x8 {}
unsafe impl Zeroable for m64x2 {}
unsafe impl Zeroable for m64x4 {}
unsafe impl NoUninit for m64x2 {}
unsafe impl NoUninit for m64x4 {}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_aligned_sum() {
        #[repr(align(128))]
        #[derive(Copy, Clone, Debug)]
        struct Aligned<T>(T);

        use rand::{Rng, SeedableRng};

        let mut rng = rand::rngs::StdRng::seed_from_u64(2);

        let nan = f32::NAN;
        let data = core::array::from_fn::<f32, 33, _>(|_| rng.gen());
        let unaligned_data = Aligned(core::array::from_fn::<f32, 36, _>(|i| {
            if i < 3 {
                nan
            } else {
                data[i - 3]
            }
        }));
        let data = &unaligned_data.0[3..];

        let arch = Arch::new();

        struct Sum<'a> {
            slice: &'a [f32],
        }
        struct AlignedSum<'a> {
            slice: &'a [f32],
        }
        struct WrongAlignedSum<'a> {
            slice: &'a [f32],
        }

        impl WithSimd for Sum<'_> {
            type Output = f32;

            #[inline(always)]
            fn with_simd<S: Simd>(self, simd: S) -> Self::Output {
                let mut sum = simd.f32s_splat(0.0);
                let (head, tail) = S::f32s_as_simd(self.slice);

                for x in head {
                    sum = simd.f32s_add(sum, *x);
                }
                sum = simd.f32s_add(sum, simd.f32s_partial_load(tail));

                bytemuck::cast_slice::<_, f32>(&[sum]).iter().sum()
            }
        }

        impl WithSimd for AlignedSum<'_> {
            type Output = f32;

            #[inline(always)]
            fn with_simd<S: Simd>(self, simd: S) -> Self::Output {
                let offset = simd.f32s_align_offset(self.slice.as_ptr(), self.slice.len());
                let (prefix, body, suffix) = simd.f32s_as_aligned_simd(self.slice, offset);

                let mut sum = prefix.read_or(simd.f32s_splat(0.0));
                for x in body {
                    sum = simd.f32s_add(sum, *x);
                }
                sum = simd.f32s_add(sum, suffix.read_or(simd.f32s_splat(0.0)));
                let sum = simd.f32s_rotate_left(sum, offset.rotate_left_amount());

                bytemuck::cast_slice::<_, f32>(&[sum]).iter().sum()
            }
        }

        impl WithSimd for WrongAlignedSum<'_> {
            type Output = f32;

            #[inline(always)]
            fn with_simd<S: Simd>(self, simd: S) -> Self::Output {
                let offset = simd.f32s_align_offset(self.slice.as_ptr(), self.slice.len());
                let (prefix, body, suffix) = simd.f32s_as_aligned_simd(self.slice, offset);

                let mut sum = prefix.read_or(simd.f32s_splat(0.0));
                for x in body {
                    sum = simd.f32s_add(sum, *x);
                }
                sum = simd.f32s_add(sum, suffix.read_or(simd.f32s_splat(0.0)));

                bytemuck::cast_slice::<_, f32>(&[sum]).iter().sum()
            }
        }

        let sum = arch.dispatch(Sum { slice: data });
        let aligned_sum = arch.dispatch(AlignedSum { slice: data });
        let wrong_aligned_sum = arch.dispatch(WrongAlignedSum { slice: data });

        struct LaneCount;

        impl WithSimd for LaneCount {
            type Output = usize;

            fn with_simd<S: Simd>(self, _: S) -> Self::Output {
                core::mem::size_of::<S::f32s>() / core::mem::size_of::<f32>()
            }
        }

        assert_eq!(sum, aligned_sum);
        if arch.dispatch(LaneCount) > 2 {
            assert_ne!(sum, wrong_aligned_sum);
        }
    }
}
