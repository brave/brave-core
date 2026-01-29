#![no_std]
#![cfg_attr(docsrs, feature(doc_cfg))]
#![deny(elided_lifetimes_in_paths)]
#![allow(clippy::missing_transmute_annotations, clippy::type_complexity)]

//! Stack that allows users to allocate dynamically sized arrays.
//!
//! The stack wraps a buffer of bytes that it uses as a workspace.
//! Allocating an array takes a chunk of memory from the stack, which can be reused once the array
//! is dropped.
//!
//! # Examples:
//! ```
//! use core::mem::MaybeUninit;
//! use dyn_stack::{MemStack, StackReq};
//!
//! // We allocate enough storage for 3 `i32` and 4 `u8`.
//! let mut buf = [MaybeUninit::uninit();
//! 	StackReq::new::<i32>(3)
//! 		.and(StackReq::new::<u8>(4))
//! 		.unaligned_bytes_required()];
//! let stack = MemStack::new(&mut buf);
//!
//! {
//! 	// We can have nested allocations.
//! 	// 3×`i32`
//! 	let (array_i32, substack) = stack.make_with::<i32>(3, |i| i as i32);
//! 	// and 4×`u8`
//! 	let (mut array_u8, _) = substack.make_with::<u8>(4, |_| 0);
//!
//! 	// We can read from the arrays,
//! 	assert_eq!(array_i32[0], 0);
//! 	assert_eq!(array_i32[1], 1);
//! 	assert_eq!(array_i32[2], 2);
//!
//! 	// and write to them.
//! 	array_u8[0] = 1;
//!
//! 	assert_eq!(array_u8[0], 1);
//! 	assert_eq!(array_u8[1], 0);
//! 	assert_eq!(array_u8[2], 0);
//! 	assert_eq!(array_u8[3], 0);
//! }
//!
//! // We can also have disjoint allocations.
//! {
//! 	// 3×`i32`
//! 	let (mut array_i32, _) = stack.make_with::<i32>(3, |i| i as i32);
//! 	assert_eq!(array_i32[0], 0);
//! 	assert_eq!(array_i32[1], 1);
//! 	assert_eq!(array_i32[2], 2);
//! }
//!
//! {
//! 	// or 4×`u8`
//! 	let (mut array_u8, _) = stack.make_with::<i32>(4, |i| i as i32 + 3);
//! 	assert_eq!(array_u8[0], 3);
//! 	assert_eq!(array_u8[1], 4);
//! 	assert_eq!(array_u8[2], 5);
//! 	assert_eq!(array_u8[3], 6);
//! }
//! ```

#[doc(hidden)]
pub use dyn_stack_macros::alloc_impl_proc;

#[doc(hidden)]
#[macro_export]
macro_rules! alloc_impl_rules {
	(
		$stack:ident
		let $pat:pat = $($mac:ident)::+!($($input:tt)*)
	) => {
		$($mac)::*!(@ alloc($stack)($pat)($($input)*) );
	};
	(
		$stack:ident
		let $pat:pat = $($mac:ident)::+![$($input:tt)*]
	) => {
		$($mac)::*!(@ alloc($stack)($pat)($($input)*) );
	};
	(
		$stack:ident
		let $pat:pat = $($mac:ident)::+!{$($input:tt)*}
	) => {
		$($mac)::*!(@ alloc($stack)($pat)($($input)*) );
	};
	(
		$stack:ident
		let $pat:pat = unsafe {$($mac:ident)::+!($($input:tt)*)}
	) => {
		$($mac)::*!(@ alloc unsafe ($stack)($pat)($($input)*) );
	};
	(
		$stack:ident
		let $pat:pat = unsafe {$($mac:ident)::+![$($input:tt)*]}
	) => {
		$($mac)::*!(@ alloc unsafe ($stack)($pat)($($input)*) );
	};
	(
		$stack:ident
		let $pat:pat = unsafe {$($mac:ident)::+!{$($input:tt)*}}
	) => {
		$($mac)::*!(@ alloc unsafe ($stack)($pat)($($input)*) );
	};
}

#[macro_export]
macro_rules! alloc {
	($stack:lifetime : {
		$($tt:tt)*
	}) => {
		$crate::alloc_impl_proc!(($crate) $stack {$($tt)*});
	};
}

#[macro_export]
macro_rules! slice {
	(@ alloc $($unsafe:ident)? ($stack:ident)  ($var:pat) ($($arg: expr),+ $(,)?)) => {
		let (mut __slice__, $stack) = $($unsafe)? { $stack.collect(::core::iter::repeat_n($($arg,)*)) };
		let $var = &mut *__slice__;
	};
}

#[cfg(feature = "std")]
extern crate std;

#[cfg(feature = "std")]
pub use std::error::Error;

#[cfg(all(feature = "core-error", not(feature = "std")))]
pub use core::error::Error;

pub mod alloc;

pub mod mem;

pub type DynStack = MemStack;

use bytemuck::Pod;

#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
pub use mem::MemBuffer;
#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
pub use mem::PodBuffer;

mod stack_req;
pub use stack_req::StackReq;

use core::fmt::Debug;
use core::marker::PhantomData;
use core::mem::MaybeUninit;
use core::ops::{Deref, DerefMut};
use core::ptr::NonNull;
use core::{fmt, slice};

/// Stack wrapper around a buffer of uninitialized bytes.
#[repr(transparent)]
pub struct MemStack {
	buffer: [MaybeUninit<u8>],
}
/// Stack wrapper around a buffer of bytes.
#[repr(transparent)]
pub struct PodStack {
	buffer: [u8],
}

/// Owns an unsized array of data, allocated from some stack.
pub struct DynArray<'a, T> {
	ptr: NonNull<T>,
	len: usize,
	__marker: PhantomData<(&'a T, T)>,
}

impl<T> DynArray<'_, T> {
	#[inline]
	#[doc(hidden)]
	pub fn into_raw_parts(self) -> (*mut T, usize) {
		let this = core::mem::ManuallyDrop::new(self);
		(this.ptr.as_ptr(), this.len)
	}

	#[inline]
	#[doc(hidden)]
	pub unsafe fn from_raw_parts(ptr: *mut T, len: usize) -> Self {
		Self {
			ptr: NonNull::new_unchecked(ptr),
			len,
			__marker: PhantomData,
		}
	}
}

/// Owns an unsized array of data, allocated from some stack.
pub struct UnpodStack<'a> {
	ptr: NonNull<u8>,
	len: usize,
	__marker: PhantomData<&'a ()>,
}

impl<T: Debug> Debug for DynArray<'_, T> {
	fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> Result<(), fmt::Error> {
		(**self).fmt(fmt)
	}
}

unsafe impl<T> Send for DynArray<'_, T> where T: Send {}
unsafe impl<T> Sync for DynArray<'_, T> where T: Sync {}

unsafe impl Send for UnpodStack<'_> {}
unsafe impl Sync for UnpodStack<'_> {}

impl<T> Drop for DynArray<'_, T> {
	#[inline]
	fn drop(&mut self) {
		unsafe { core::ptr::drop_in_place(core::ptr::slice_from_raw_parts_mut(self.ptr.as_ptr(), self.len)) };
	}
}

macro_rules! if_cfg {
	(if $cfg: meta $if_true: block else $if_false: block $(,)?) => {
		#[cfg($cfg)]
		{
			$if_true
		}
		#[cfg(not($cfg))]
		{
			$if_false
		}
	};
}

/// fool the compiler into thinking we init the data
///
/// # Safety
/// `[ptr, ptr + len)` must have been fully initialized at some point before this is called
#[inline(always)]
unsafe fn launder(ptr: *mut u8, len: usize) {
	unsafe {
		if_cfg!(if all(
			not(debug_assertions),
			not(miri),
			any(
				target_arch = "x86",
				target_arch = "x86_64",
				target_arch = "arm",
				target_arch = "aarch64",
				target_arch = "loongarch64",
				target_arch = "riscv32",
				target_arch = "riscv64",
			)
		) {
			_ = len;
			core::arch::asm! { "/* {0} */", in(reg) ptr, options(nostack) }
		} else {
			const ARBITRARY_BYTE: u8 = 0xCD;
			core::ptr::write_bytes(ptr, ARBITRARY_BYTE, len)
		});
	}
}

impl Drop for UnpodStack<'_> {
	#[inline]
	fn drop(&mut self) {
		unsafe { launder(self.ptr.as_ptr(), self.len) };
	}
}

impl<T> Deref for DynArray<'_, T> {
	type Target = [T];

	#[inline]
	fn deref(&self) -> &'_ Self::Target {
		unsafe { slice::from_raw_parts(self.ptr.as_ptr(), self.len) }
	}
}

impl<T> DerefMut for DynArray<'_, T> {
	#[inline]
	fn deref_mut(&mut self) -> &mut Self::Target {
		unsafe { slice::from_raw_parts_mut(self.ptr.as_ptr(), self.len) }
	}
}

impl<T> AsRef<[T]> for DynArray<'_, T> {
	#[inline]
	fn as_ref(&self) -> &'_ [T] {
		unsafe { slice::from_raw_parts(self.ptr.as_ptr(), self.len) }
	}
}

impl<T> AsMut<[T]> for DynArray<'_, T> {
	#[inline]
	fn as_mut(&mut self) -> &'_ mut [T] {
		unsafe { slice::from_raw_parts_mut(self.ptr.as_ptr(), self.len) }
	}
}

impl Deref for UnpodStack<'_> {
	type Target = MemStack;

	#[inline]
	fn deref(&self) -> &'_ Self::Target {
		unsafe { &*(core::ptr::slice_from_raw_parts(self.ptr.as_ptr(), self.len) as *const MemStack) }
	}
}

impl DerefMut for UnpodStack<'_> {
	#[inline]
	fn deref_mut(&mut self) -> &mut Self::Target {
		unsafe { &mut *(core::ptr::slice_from_raw_parts_mut(self.ptr.as_ptr(), self.len) as *mut MemStack) }
	}
}

#[inline]
unsafe fn transmute_slice<T>(slice: &mut [MaybeUninit<u8>], size: usize) -> &mut [T] {
	slice::from_raw_parts_mut(slice.as_mut_ptr() as *mut T, size)
}
#[inline]
unsafe fn transmute_pod_slice<T: Pod>(slice: &mut [u8], size: usize) -> &mut [T] {
	slice::from_raw_parts_mut(slice.as_mut_ptr() as *mut T, size)
}

struct DropGuard<T> {
	ptr: *mut T,
	len: usize,
}

impl<T> Drop for DropGuard<T> {
	#[inline]
	fn drop(&mut self) {
		unsafe { core::ptr::drop_in_place(core::ptr::slice_from_raw_parts_mut(self.ptr, self.len)) };
	}
}

#[inline]
fn init_array_with<T>(mut f: impl FnMut(usize) -> T, array: &mut [MaybeUninit<T>]) -> &mut [T] {
	let len = array.len();
	let ptr = array.as_mut_ptr() as *mut T;

	let mut guard = DropGuard { ptr, len: 0 };

	for i in 0..len {
		guard.len = i;
		unsafe { ptr.add(i).write(f(i)) };
	}
	core::mem::forget(guard);

	unsafe { slice::from_raw_parts_mut(ptr, len) }
}

#[inline]
fn init_pod_array_with<T: Pod>(mut f: impl FnMut(usize) -> T, array: &mut [T]) -> &mut [T] {
	for (i, x) in array.iter_mut().enumerate() {
		*x = f(i);
	}
	array
}

#[inline]
unsafe fn init_array_with_iter<T, I: Iterator<Item = T>>(iter: I, ptr: &mut [MaybeUninit<T>]) -> usize {
	let max_len = ptr.len();
	let ptr = ptr.as_mut_ptr();
	let mut guard = DropGuard { ptr, len: 0 };

	iter.take(max_len).enumerate().for_each(|(i, item)| {
		*ptr.add(i) = MaybeUninit::new(item);
		guard.len += 1;
	});

	let len = guard.len;
	core::mem::forget(guard);

	len
}

#[inline]
fn init_pod_array_with_iter<T: Pod, I: Iterator<Item = T>>(iter: I, ptr: &mut [T]) -> usize {
	let mut len = 0;
	iter.zip(ptr).for_each(|(item, dst)| {
		*dst = item;
		len += 1;
	});
	len
}

#[track_caller]
#[inline]
fn check_alignment(align: usize, alignof_val: usize, type_name: &'static str) {
	assert!(
		(align & (align.wrapping_sub(1))) == 0,
		r#"
requested alignment is not a power of two:
 - requested alignment: {}
"#,
		align
	);
	assert!(
		alignof_val <= align,
		r#"
requested alignment is less than the minimum valid alignment for `{}`:
 - requested alignment: {}
 - minimum alignment: {}
"#,
		type_name,
		align,
		alignof_val,
	);
}

#[track_caller]
#[inline]
fn check_enough_space_for_align_offset(len: usize, align: usize, align_offset: usize) {
	assert!(
		len >= align_offset,
		r#"
buffer is not large enough to accomodate the requested alignment
 - buffer length: {}
 - requested alignment: {}
 - byte offset for alignment: {}
"#,
		len,
		align,
		align_offset,
	);
}

#[track_caller]
#[inline]
fn check_enough_space_for_array(remaining_len: usize, sizeof_val: usize, array_len: usize, type_name: &'static str) {
	if sizeof_val == 0 {
		return;
	}
	assert!(
		remaining_len / sizeof_val >= array_len,
		r#"
buffer is not large enough to allocate an array of type `{}` of the requested length:
 - remaining buffer length (after adjusting for alignment): {},
 - requested array length: {} ({} bytes),
"#,
		type_name,
		remaining_len,
		array_len,
		array_len * sizeof_val,
	);
}

#[repr(transparent)]
pub struct Bump<'stack> {
	ptr: core::cell::UnsafeCell<&'stack mut MemStack>,
}

unsafe impl alloc::Allocator for Bump<'_> {
	fn allocate(&self, layout: core::alloc::Layout) -> Result<NonNull<[u8]>, alloc::AllocError> {
		let ptr = unsafe { &mut *self.ptr.get() };
		let old = core::mem::replace(ptr, MemStack::new(&mut []));

		if old.can_hold(StackReq::new_aligned::<u8>(layout.size(), layout.align())) {
			let (alloc, new) = old.make_aligned_uninit::<u8>(layout.size(), layout.align());
			*ptr = new;

			let len = alloc.len();
			let ptr = alloc.as_mut_ptr() as *mut u8;
			Ok(unsafe { NonNull::new_unchecked(core::ptr::slice_from_raw_parts_mut(ptr, len)) })
		} else {
			Err(alloc::AllocError)
		}
	}

	#[inline]
	unsafe fn deallocate(&self, ptr: NonNull<u8>, layout: core::alloc::Layout) {
		let _ = (ptr, layout);
	}
}

impl MemStack {
	/// Returns a new [`MemStack`] from the provided memory buffer.
	#[inline]
	pub fn new(buffer: &mut [MaybeUninit<u8>]) -> &mut Self {
		unsafe { &mut *(buffer as *mut [MaybeUninit<u8>] as *mut Self) }
	}

	/// Returns a new [`MemStack`] from the provided memory buffer.
	#[inline]
	pub fn new_any<T>(buffer: &mut [MaybeUninit<T>]) -> &mut Self {
		let len = core::mem::size_of_val(buffer);
		Self::new(unsafe { slice::from_raw_parts_mut(buffer.as_mut_ptr() as *mut _, len) })
	}

	/// Returns `true` if the stack can hold an allocation with the given size and alignment
	/// requirements.
	#[inline]
	#[must_use]
	pub fn can_hold(&self, alloc_req: StackReq) -> bool {
		let align = alloc_req.align_bytes();
		let size = alloc_req.size_bytes();
		let align_offset = self.buffer.as_ptr().align_offset(align);
		let self_size = self.buffer.len();
		(self_size >= align_offset) && (self_size - align_offset >= size)
	}

	/// Returns the number of bytes that this stack can hold.
	#[inline]
	pub fn len_bytes(&self) -> usize {
		self.buffer.len()
	}

	/// Returns a pointer to the (possibly uninitialized) stack memory.
	#[inline]
	pub fn as_ptr(&self) -> *const u8 {
		self.buffer.as_ptr() as _
	}

	#[track_caller]
	#[inline]
	fn split_buffer<'out>(
		buffer: &'out mut [MaybeUninit<u8>],
		size: usize,
		align: usize,
		sizeof_val: usize,
		alignof_val: usize,
		type_name: &'static str,
	) -> (&'out mut [MaybeUninit<u8>], &'out mut [MaybeUninit<u8>]) {
		let len = buffer.len();
		let align_offset = buffer.as_mut_ptr().align_offset(align);

		check_alignment(align, alignof_val, type_name);
		check_enough_space_for_align_offset(len, align, align_offset);
		check_enough_space_for_array(len - align_offset, sizeof_val, size, type_name);

		let buffer = unsafe { buffer.get_unchecked_mut(align_offset..) };
		let len = len - align_offset;

		let begin = buffer.as_mut_ptr();
		let begin_len = size * sizeof_val;
		let mid = unsafe { begin.add(begin_len) };
		let mid_len = len - begin_len;
		unsafe { (slice::from_raw_parts_mut(begin, begin_len), slice::from_raw_parts_mut(mid, mid_len)) }
	}

	/// Returns a new aligned and uninitialized [`DynArray`] and a stack over the remainder of the
	/// buffer.
	///
	/// # Panics
	///
	/// Panics if the stack isn't large enough to allocate the array.
	#[track_caller]
	#[inline]
	#[must_use]
	pub fn make_aligned_uninit<T>(&mut self, size: usize, align: usize) -> (&mut [MaybeUninit<T>], &mut Self) {
		let (taken, remaining) = Self::split_buffer(
			&mut self.buffer,
			size,
			align,
			core::mem::size_of::<T>(),
			core::mem::align_of::<T>(),
			core::any::type_name::<T>(),
		);

		(unsafe { transmute_slice::<MaybeUninit<T>>(taken, size) }, MemStack::new(remaining))
	}

	/// Returns a new aligned [`DynArray`], initialized with the provided function, and a stack
	/// over the remainder of the buffer.
	///
	/// # Panics
	///
	/// Panics if the stack isn't large enough to allocate the array, or if the provided function
	/// panics.
	#[track_caller]
	#[inline]
	#[must_use]
	pub fn make_aligned_with<T>(&mut self, size: usize, align: usize, f: impl FnMut(usize) -> T) -> (DynArray<'_, T>, &mut Self) {
		let (taken, remaining) = self.make_aligned_uninit(size, align);
		let (len, ptr) = {
			let taken = init_array_with(f, taken);
			(taken.len(), taken.as_mut_ptr())
		};
		(
			DynArray {
				ptr: unsafe { NonNull::<T>::new_unchecked(ptr) },
				len,
				__marker: PhantomData,
			},
			remaining,
		)
	}

	#[track_caller]
	#[inline]
	#[must_use]
	#[doc(hidden)]
	pub unsafe fn make_raw<T: Pod>(&mut self, size: usize) -> (&mut [T], &mut Self) {
		self.make_aligned_raw(size, core::mem::align_of::<T>())
	}

	#[track_caller]
	#[inline]
	#[must_use]
	#[doc(hidden)]
	pub unsafe fn make_aligned_raw<T: Pod>(&mut self, size: usize, align: usize) -> (&mut [T], &mut Self) {
		let (mem, stack) = self.make_aligned_uninit::<T>(size, align);
		unsafe { (&mut *(mem as *mut [MaybeUninit<T>] as *mut [T]), stack) }
	}

	/// Returns a new uninitialized [`DynArray`] and a stack over the remainder of the buffer.
	///
	/// # Panics
	///
	/// Panics if the stack isn't large enough to allocate the array.
	#[track_caller]
	#[inline]
	#[must_use]
	pub fn make_uninit<T>(&mut self, size: usize) -> (&mut [MaybeUninit<T>], &mut Self) {
		self.make_aligned_uninit(size, core::mem::align_of::<T>())
	}

	/// Returns a new [`DynArray`], initialized with the provided function, and a stack over the
	/// remainder of the buffer.
	///
	/// # Panics
	///
	/// Panics if the stack isn't large enough to allocate the array, or if the provided function
	/// panics.
	#[track_caller]
	#[inline]
	#[must_use]
	pub fn make_with<T>(&mut self, size: usize, f: impl FnMut(usize) -> T) -> (DynArray<'_, T>, &mut Self) {
		self.make_aligned_with(size, core::mem::align_of::<T>(), f)
	}

	/// Returns a new aligned [`DynArray`], initialized with the provided iterator, and a stack
	/// over the remainder of the buffer.  
	/// If there isn't enough space for all the iterator items, then the returned array only
	/// contains the first elements that fit into the stack.
	///
	/// # Panics
	///
	/// Panics if the provided iterator panics.
	#[track_caller]
	#[inline]
	#[must_use]
	pub fn collect_aligned<I>(&mut self, align: usize, iter: impl IntoIterator<Item = I>) -> (DynArray<'_, I>, &mut Self) {
		self.collect_aligned_impl(align, iter.into_iter())
	}

	/// Returns a new [`DynArray`], initialized with the provided iterator, and a stack over the
	/// remainder of the buffer.  
	/// If there isn't enough space for all the iterator items, then the returned array only
	/// contains the first elements that fit into the stack.
	///
	/// # Panics
	///
	/// Panics if the provided iterator panics.
	#[track_caller]
	#[inline]
	#[must_use]
	pub fn collect<I>(&mut self, iter: impl IntoIterator<Item = I>) -> (DynArray<'_, I>, &mut Self) {
		self.collect_aligned_impl(core::mem::align_of::<I>(), iter.into_iter())
	}

	#[track_caller]
	#[inline]
	fn collect_aligned_impl<I: Iterator>(&mut self, align: usize, iter: I) -> (DynArray<'_, I::Item>, &mut Self) {
		let sizeof_val = core::mem::size_of::<I::Item>();
		let alignof_val = core::mem::align_of::<I::Item>();
		let align_offset = self.buffer.as_mut_ptr().align_offset(align);

		check_alignment(align, alignof_val, core::any::type_name::<I::Item>());
		check_enough_space_for_align_offset(self.buffer.len(), align, align_offset);

		let buffer = unsafe { self.buffer.get_unchecked_mut(align_offset..) };
		let buffer_len = buffer.len();
		let buffer_ptr = buffer.as_mut_ptr();
		unsafe {
			let len = init_array_with_iter(
				iter,
				slice::from_raw_parts_mut(
					buffer_ptr as *mut MaybeUninit<I::Item>,
					if sizeof_val == 0 { usize::MAX } else { buffer_len / sizeof_val },
				),
			);

			let remaining_slice = slice::from_raw_parts_mut(buffer_ptr.add(len * sizeof_val), buffer.len() - len * sizeof_val);
			(
				DynArray {
					ptr: NonNull::new_unchecked(buffer_ptr as *mut I::Item),
					len,
					__marker: PhantomData,
				},
				Self::new(remaining_slice),
			)
		}
	}

	#[inline]
	pub fn bump<'bump, 'stack>(self: &'bump mut &'stack mut Self) -> &'bump mut Bump<'stack> {
		unsafe { &mut *(self as *mut &mut Self as *mut Bump<'stack>) }
	}
}

impl PodStack {
	/// Returns a new [`PodStack`] from the provided memory buffer.
	#[inline]
	pub fn new(buffer: &mut [u8]) -> &mut Self {
		unsafe { &mut *(buffer as *mut [u8] as *mut Self) }
	}

	/// Returns a new [`MemStack`] from the provided memory buffer.
	#[inline]
	pub fn new_any<T: Pod>(buffer: &mut [T]) -> &mut Self {
		let len = core::mem::size_of_val(buffer);
		Self::new(unsafe { slice::from_raw_parts_mut(buffer.as_mut_ptr() as *mut _, len) })
	}

	/// Returns `true` if the stack can hold an allocation with the given size and alignment
	/// requirements.
	#[inline]
	#[must_use]
	pub fn can_hold(&self, alloc_req: StackReq) -> bool {
		let align = alloc_req.align_bytes();
		let size = alloc_req.size_bytes();
		let align_offset = self.buffer.as_ptr().align_offset(align);
		let self_size = self.buffer.len();
		(self_size >= align_offset) && (self_size - align_offset >= size)
	}

	/// Returns the number of bytes that this stack can hold.
	#[inline]
	pub fn len_bytes(&self) -> usize {
		self.buffer.len()
	}

	/// Returns a pointer to the stack memory.
	#[inline]
	pub fn as_ptr(&self) -> *const u8 {
		self.buffer.as_ptr() as _
	}

	#[track_caller]
	#[inline]
	fn split_buffer<'out>(
		buffer: &'out mut [u8],
		size: usize,
		align: usize,
		sizeof_val: usize,
		alignof_val: usize,
		type_name: &'static str,
	) -> (&'out mut [u8], &'out mut [u8]) {
		let len = buffer.len();
		let align_offset = buffer.as_mut_ptr().align_offset(align);

		check_alignment(align, alignof_val, type_name);
		check_enough_space_for_align_offset(len, align, align_offset);
		check_enough_space_for_array(len - align_offset, sizeof_val, size, type_name);

		let buffer = unsafe { buffer.get_unchecked_mut(align_offset..) };
		let len = len - align_offset;

		let begin = buffer.as_mut_ptr();
		let begin_len = size * sizeof_val;
		let mid = unsafe { begin.add(begin_len) };
		let mid_len = len - begin_len;
		unsafe { (slice::from_raw_parts_mut(begin, begin_len), slice::from_raw_parts_mut(mid, mid_len)) }
	}

	/// Returns a new aligned and uninitialized slice and a stack over the remainder of the
	/// buffer.
	///
	/// # Panics
	///
	/// Panics if the stack isn't large enough to allocate the array.
	#[track_caller]
	#[inline]
	#[must_use]
	pub fn make_aligned_raw<T: Pod>(&mut self, size: usize, align: usize) -> (&mut [T], &mut Self) {
		let (taken, remaining) = Self::split_buffer(
			&mut self.buffer,
			size,
			align,
			core::mem::size_of::<T>(),
			core::mem::align_of::<T>(),
			core::any::type_name::<T>(),
		);

		let taken = unsafe { transmute_pod_slice::<T>(taken, size) };
		(taken, Self::new(remaining))
	}

	/// Returns a new aligned and uninitialized slice and a stack over the remainder of the
	/// buffer.
	///
	/// # Panics
	///
	/// Panics if the stack isn't large enough to allocate the array.
	///
	/// # Safety
	///
	/// The return value must be dropped if any uninitialized values are written to the bytes by the
	/// time the borrow ends.
	pub unsafe fn make_aligned_unpod(&mut self, size: usize, align: usize) -> (UnpodStack<'_>, &mut Self) {
		let (taken, remaining) = Self::split_buffer(&mut self.buffer, size, align, 1, 1, "[Bytes]");
		(
			UnpodStack {
				ptr: NonNull::new_unchecked(taken.as_mut_ptr()),
				len: size,
				__marker: PhantomData,
			},
			Self::new(remaining),
		)
	}

	/// Returns a new aligned slice, initialized with the provided function, and a stack
	/// over the remainder of the buffer.
	///
	/// # Panics
	///
	/// Panics if the stack isn't large enough to allocate the array, or if the provided function
	/// panics.
	#[track_caller]
	#[inline]
	#[must_use]
	pub fn make_aligned_with<T: Pod>(&mut self, size: usize, align: usize, f: impl FnMut(usize) -> T) -> (&mut [T], &mut Self) {
		let (taken, remaining) = self.make_aligned_raw(size, align);
		let taken = init_pod_array_with(f, taken);
		(taken, remaining)
	}

	/// Returns a new uninitialized slice and a stack over the remainder of the buffer.
	///
	/// # Panics
	///
	/// Panics if the stack isn't large enough to allocate the array.
	#[track_caller]
	#[inline]
	#[must_use]
	pub fn make_raw<T: Pod>(&mut self, size: usize) -> (&mut [T], &mut Self) {
		self.make_aligned_raw(size, core::mem::align_of::<T>())
	}

	/// Returns a new slice, initialized with the provided function, and a stack over the
	/// remainder of the buffer.
	///
	/// # Panics
	///
	/// Panics if the stack isn't large enough to allocate the array, or if the provided function
	/// panics.
	#[track_caller]
	#[inline]
	#[must_use]
	pub fn make_with<T: Pod>(&mut self, size: usize, f: impl FnMut(usize) -> T) -> (&mut [T], &mut Self) {
		self.make_aligned_with(size, core::mem::align_of::<T>(), f)
	}

	/// Returns a new aligned slice, initialized with the provided iterator, and a stack
	/// over the remainder of the buffer.  
	/// If there isn't enough space for all the iterator items, then the returned array only
	/// contains the first elements that fit into the stack.
	///
	/// # Panics
	///
	/// Panics if the provided iterator panics.
	#[track_caller]
	#[inline]
	#[must_use]
	pub fn collect_aligned<I: Pod>(&mut self, align: usize, iter: impl IntoIterator<Item = I>) -> (&mut [I], &mut Self) {
		self.collect_aligned_impl(align, iter.into_iter())
	}

	/// Returns a new slice, initialized with the provided iterator, and a stack over the
	/// remainder of the buffer.  
	/// If there isn't enough space for all the iterator items, then the returned array only
	/// contains the first elements that fit into the stack.
	///
	/// # Panics
	///
	/// Panics if the provided iterator panics.
	#[track_caller]
	#[inline]
	#[must_use]
	pub fn collect<I: Pod>(&mut self, iter: impl IntoIterator<Item = I>) -> (&mut [I], &mut Self) {
		self.collect_aligned_impl(core::mem::align_of::<I>(), iter.into_iter())
	}

	#[track_caller]
	#[inline]
	fn collect_aligned_impl<I: Iterator>(&mut self, align: usize, iter: I) -> (&mut [I::Item], &mut Self)
	where
		I::Item: Pod,
	{
		let sizeof_val = core::mem::size_of::<I::Item>();
		let alignof_val = core::mem::align_of::<I::Item>();
		let align_offset = self.buffer.as_mut_ptr().align_offset(align);

		check_alignment(align, alignof_val, core::any::type_name::<I::Item>());
		check_enough_space_for_align_offset(self.buffer.len(), align, align_offset);

		let buffer = unsafe { self.buffer.get_unchecked_mut(align_offset..) };
		let buffer_len = buffer.len();
		let buffer_ptr = buffer.as_mut_ptr();
		unsafe {
			let len = init_pod_array_with_iter(
				iter,
				slice::from_raw_parts_mut(
					buffer_ptr as *mut I::Item,
					if sizeof_val == 0 { usize::MAX } else { buffer_len / sizeof_val },
				),
			);

			let taken = slice::from_raw_parts_mut(buffer_ptr as *mut I::Item, len);
			let remaining_slice = slice::from_raw_parts_mut(buffer_ptr.add(len * sizeof_val), buffer_len - len * sizeof_val);
			(taken, Self::new(remaining_slice))
		}
	}
}

#[cfg(all(test, feature = "alloc"))]
mod dyn_stack_tests {
	use super::*;
	use alloc::Global;

	#[test]
	fn empty_in() {
		let mut buf = MemBuffer::new_in(StackReq::new::<i32>(0), Global);
		let stack = MemStack::new(&mut buf);
		let (_arr0, _stack) = stack.make_with::<i32>(0, |i| i as i32);
	}

	#[test]
	#[should_panic]
	fn empty_overflow_in() {
		let mut buf = MemBuffer::new_in(StackReq::new::<i32>(0), Global);
		let stack = MemStack::new(&mut buf);
		let (_arr0, _stack) = stack.make_with::<i32>(1, |i| i as i32);
	}

	#[test]
	fn empty_collect_in() {
		let mut buf = MemBuffer::new_in(StackReq::new::<i32>(0), Global);
		let stack = MemStack::new(&mut buf);
		let (_arr0, _stack) = stack.collect(0..0);
	}

	#[test]
	fn empty_collect_overflow_in() {
		let mut buf = MemBuffer::new_in(StackReq::new::<i32>(0), Global);
		let stack = MemStack::new(&mut buf);
		let (arr0, _stack) = stack.collect(0..1);
		assert!(arr0.is_empty());
	}

	#[test]
	#[should_panic]
	fn overflow_in() {
		let mut buf = MemBuffer::new_in(StackReq::new::<i32>(1), Global);
		let stack = MemStack::new(&mut buf);
		let (_arr0, _stack) = stack.make_with::<i32>(2, |i| i as i32);
	}

	#[test]
	fn collect_overflow_in() {
		let mut buf = MemBuffer::new_in(StackReq::new::<i32>(1), Global);
		let stack = MemStack::new(&mut buf);
		let (arr0, _stack) = stack.collect(1..3);
		assert_eq!(arr0.len(), 1);
		assert_eq!(arr0[0], 1)
	}

	#[test]
	fn empty() {
		let mut buf = MemBuffer::new(StackReq::new::<i32>(0));
		let stack = MemStack::new(&mut buf);
		let (_arr0, _stack) = stack.make_with::<i32>(0, |i| i as i32);
	}

	#[test]
	#[should_panic]
	fn empty_overflow() {
		let mut buf = MemBuffer::new(StackReq::new::<i32>(0));
		let stack = MemStack::new(&mut buf);
		let (_arr0, _stack) = stack.make_with::<i32>(1, |i| i as i32);
	}

	#[test]
	fn empty_collect() {
		let mut buf = MemBuffer::new(StackReq::new::<i32>(0));
		let stack = MemStack::new(&mut buf);
		let (_arr0, _stack) = stack.collect(0..0);
	}

	#[test]
	fn empty_collect_overflow() {
		let mut buf = MemBuffer::new(StackReq::new::<i32>(0));
		let stack = MemStack::new(&mut buf);
		let (arr0, _stack) = stack.collect(0..1);
		assert!(arr0.is_empty());
	}

	#[test]
	#[should_panic]
	fn overflow() {
		let mut buf = MemBuffer::new(StackReq::new::<i32>(1));
		let stack = MemStack::new(&mut buf);
		let (_arr0, _stack) = stack.make_with::<i32>(2, |i| i as i32);
	}

	#[test]
	fn collect_overflow() {
		let mut buf = MemBuffer::new(StackReq::new::<i32>(1));
		let stack = MemStack::new(&mut buf);
		let (arr0, _stack) = stack.collect(1..3);
		assert_eq!(arr0.len(), 1);
		assert_eq!(arr0[0], 1)
	}

	#[test]
	fn basic_nested() {
		let mut buf = MemBuffer::new(StackReq::new::<i32>(6));

		let stack = MemStack::new(&mut buf);
		assert!(stack.can_hold(StackReq::new::<i32>(6)));
		assert!(!stack.can_hold(StackReq::new::<i32>(7)));

		let (arr0, stack) = stack.make_with::<i32>(3, |i| i as i32);
		assert_eq!(arr0[0], 0);
		assert_eq!(arr0[1], 1);
		assert_eq!(arr0[2], 2);

		let (arr1, _) = stack.make_with::<i32>(3, |i| i as i32 + 3);

		// first values are untouched
		assert_eq!(arr0[0], 0);
		assert_eq!(arr0[1], 1);
		assert_eq!(arr0[2], 2);

		assert_eq!(arr1[0], 3);
		assert_eq!(arr1[1], 4);
		assert_eq!(arr1[2], 5);
	}

	#[test]
	fn basic_disjoint() {
		let mut buf = MemBuffer::new(StackReq::new::<i32>(3));

		let stack = MemStack::new(&mut buf);

		{
			let (arr0, _) = stack.make_with::<i32>(3, |i| i as i32);
			assert_eq!(arr0[0], 0);
			assert_eq!(arr0[1], 1);
			assert_eq!(arr0[2], 2);
		}
		{
			let (arr1, _) = stack.make_with::<i32>(3, |i| i as i32 + 3);

			assert_eq!(arr1[0], 3);
			assert_eq!(arr1[1], 4);
			assert_eq!(arr1[2], 5);
		}
	}

	#[test]
	fn basic_nested_collect() {
		let mut buf = MemBuffer::new(StackReq::new::<i32>(6));
		let stack = MemStack::new(&mut buf);

		let (arr0, stack) = stack.collect(0..3_i32);
		assert_eq!(arr0[0], 0);
		assert_eq!(arr0[1], 1);
		assert_eq!(arr0[2], 2);

		let (arr1, _) = stack.collect(3..6_i32);

		// first values are untouched
		assert_eq!(arr0[0], 0);
		assert_eq!(arr0[1], 1);
		assert_eq!(arr0[2], 2);

		assert_eq!(arr1[0], 3);
		assert_eq!(arr1[1], 4);
		assert_eq!(arr1[2], 5);
	}

	#[test]
	fn basic_disjoint_collect() {
		let mut buf = MemBuffer::new(StackReq::new::<i32>(3));

		let stack = MemStack::new(&mut buf);

		{
			let (arr0, _) = stack.collect(0..3_i32);
			assert_eq!(arr0[0], 0);
			assert_eq!(arr0[1], 1);
			assert_eq!(arr0[2], 2);
		}
		{
			let (arr1, _) = stack.collect(3..6_i32);

			assert_eq!(arr1[0], 3);
			assert_eq!(arr1[1], 4);
			assert_eq!(arr1[2], 5);
		}
	}

	#[test]
	fn drop_nested() {
		use core::sync::atomic::{AtomicI32, Ordering};
		static DROP_COUNT: AtomicI32 = AtomicI32::new(0);

		struct CountedDrop;
		impl Drop for CountedDrop {
			fn drop(&mut self) {
				DROP_COUNT.fetch_add(1, Ordering::SeqCst);
			}
		}

		let mut buf = MemBuffer::new(StackReq::new::<CountedDrop>(6));
		let stack = MemStack::new(&mut buf);

		let stack = {
			let (_arr, stack) = stack.make_with(3, |_| CountedDrop);
			stack
		};
		assert_eq!(DROP_COUNT.load(Ordering::SeqCst), 3);
		let _stack = {
			let (_arr, stack) = stack.make_with(4, |_| CountedDrop);
			stack
		};
		assert_eq!(DROP_COUNT.load(Ordering::SeqCst), 7);
	}

	#[test]
	fn drop_disjoint() {
		use core::sync::atomic::{AtomicI32, Ordering};
		static DROP_COUNT: AtomicI32 = AtomicI32::new(0);

		struct CountedDrop;
		impl Drop for CountedDrop {
			fn drop(&mut self) {
				DROP_COUNT.fetch_add(1, Ordering::SeqCst);
			}
		}

		let mut buf = MemBuffer::new(StackReq::new::<CountedDrop>(6));
		let stack = MemStack::new(&mut buf);

		{
			let _ = stack.make_with(3, |_| CountedDrop);
			assert_eq!(DROP_COUNT.load(Ordering::SeqCst), 3);
		}

		{
			let _ = stack.make_with(4, |_| CountedDrop);
			assert_eq!(DROP_COUNT.load(Ordering::SeqCst), 7);
		}
	}
}

#[cfg(all(test, feature = "alloc"))]
mod pod_stack_tests {
	use super::*;

	#[test]
	fn empty() {
		let mut buf = PodBuffer::new(StackReq::new::<i32>(0));
		let stack = PodStack::new(&mut buf);
		let (_arr0, _stack) = stack.make_with::<i32>(0, |i| i as i32);
	}

	#[test]
	#[should_panic]
	fn empty_overflow() {
		let mut buf = PodBuffer::new(StackReq::new::<i32>(0));
		let stack = PodStack::new(&mut buf);
		let (_arr0, _stack) = stack.make_with::<i32>(1, |i| i as i32);
	}

	#[test]
	fn empty_collect() {
		let mut buf = PodBuffer::new(StackReq::new::<i32>(0));
		let stack = PodStack::new(&mut buf);
		let (_arr0, _stack) = stack.collect(0..0);
	}

	#[test]
	fn empty_collect_overflow() {
		let mut buf = PodBuffer::new(StackReq::new::<i32>(0));
		let stack = PodStack::new(&mut buf);
		let (arr0, _stack) = stack.collect(0..1);
		assert!(arr0.is_empty());
	}

	#[test]
	#[should_panic]
	fn overflow() {
		let mut buf = PodBuffer::new(StackReq::new::<i32>(1));
		let stack = PodStack::new(&mut buf);
		let (_arr0, _stack) = stack.make_with::<i32>(2, |i| i as i32);
	}

	#[test]
	fn collect_overflow() {
		let mut buf = PodBuffer::new(StackReq::new::<i32>(1));
		let stack = PodStack::new(&mut buf);
		let (arr0, _stack) = stack.collect(1..3);
		assert_eq!(arr0.len(), 1);
		assert_eq!(arr0[0], 1)
	}

	#[test]
	fn basic_nested() {
		let mut buf = PodBuffer::new(StackReq::new::<i32>(6));

		let stack = PodStack::new(&mut buf);
		assert!(stack.can_hold(StackReq::new::<i32>(6)));
		assert!(!stack.can_hold(StackReq::new::<i32>(7)));

		let (arr0, stack) = stack.make_with::<i32>(3, |i| i as i32);
		assert_eq!(arr0[0], 0);
		assert_eq!(arr0[1], 1);
		assert_eq!(arr0[2], 2);

		let (arr1, _) = stack.make_with::<i32>(3, |i| i as i32 + 3);

		// first values are untouched
		assert_eq!(arr0[0], 0);
		assert_eq!(arr0[1], 1);
		assert_eq!(arr0[2], 2);

		assert_eq!(arr1[0], 3);
		assert_eq!(arr1[1], 4);
		assert_eq!(arr1[2], 5);
	}

	#[test]
	fn basic_disjoint() {
		let mut buf = PodBuffer::new(StackReq::new::<i32>(3));

		let stack = PodStack::new(&mut buf);

		{
			let (arr0, _) = stack.make_with::<i32>(3, |i| i as i32);
			assert_eq!(arr0[0], 0);
			assert_eq!(arr0[1], 1);
			assert_eq!(arr0[2], 2);
		}
		{
			let (arr1, _) = stack.make_with::<i32>(3, |i| i as i32 + 3);

			assert_eq!(arr1[0], 3);
			assert_eq!(arr1[1], 4);
			assert_eq!(arr1[2], 5);
		}
	}

	#[test]
	fn basic_nested_collect() {
		let mut buf = PodBuffer::new(StackReq::new::<i32>(6));
		let stack = PodStack::new(&mut buf);

		let (arr0, stack) = stack.collect(0..3_i32);
		assert_eq!(arr0[0], 0);
		assert_eq!(arr0[1], 1);
		assert_eq!(arr0[2], 2);

		let (arr1, _) = stack.collect(3..6_i32);

		// first values are untouched
		assert_eq!(arr0[0], 0);
		assert_eq!(arr0[1], 1);
		assert_eq!(arr0[2], 2);

		assert_eq!(arr1[0], 3);
		assert_eq!(arr1[1], 4);
		assert_eq!(arr1[2], 5);
	}

	#[test]
	fn basic_disjoint_collect() {
		let mut buf = PodBuffer::new(StackReq::new::<i32>(3));

		let stack = PodStack::new(&mut buf);

		{
			let (arr0, _) = stack.collect(0..3_i32);
			assert_eq!(arr0[0], 0);
			assert_eq!(arr0[1], 1);
			assert_eq!(arr0[2], 2);
		}
		{
			let (arr1, _) = stack.collect(3..6_i32);

			assert_eq!(arr1[0], 3);
			assert_eq!(arr1[1], 4);
			assert_eq!(arr1[2], 5);
		}
	}

	#[test]
	fn make_raw() {
		let mut buf = PodBuffer::new(StackReq::new::<i32>(3));
		buf.fill(0);

		let stack = PodStack::new(&mut buf);

		{
			let (arr0, _) = stack.make_raw::<i32>(3);
			assert_eq!(arr0[0], 0);
			assert_eq!(arr0[1], 0);
			assert_eq!(arr0[2], 0);
		}
		{
			let (arr0, _) = stack.collect(0..3_i32);
			assert_eq!(arr0[0], 0);
			assert_eq!(arr0[1], 1);
			assert_eq!(arr0[2], 2);
		}
		{
			let (arr1, _) = stack.make_raw::<i32>(3);

			assert_eq!(arr1[0], 0);
			assert_eq!(arr1[1], 1);
			assert_eq!(arr1[2], 2);
		}
	}

	#[test]
	fn make_unpod() {
		let mut buf = PodBuffer::new(StackReq::new::<i32>(3));
		let stack = PodStack::new(&mut buf);

		{
			let (mut stack, _) = unsafe { stack.make_aligned_unpod(12, 4) };

			let stack = &mut *stack;
			let (mem, _) = stack.make_uninit::<u32>(3);
			mem.fill(MaybeUninit::uninit());

			let mut stack = stack;
			let mut buf = MemBuffer::new_in(StackReq::new::<u32>(3), alloc::DynAlloc::from_mut(stack.bump()));
			let stack = MemStack::new(&mut buf);
			let _ = stack.make_uninit::<u32>(3);
		}

		let (mem, _) = stack.make_raw::<u32>(3);
		for x in mem {
			*x = *x;
		}
	}
}
