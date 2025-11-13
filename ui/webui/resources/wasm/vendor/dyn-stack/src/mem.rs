use crate::stack_req::StackReq;
use alloc::alloc::handle_alloc_error;
use core::alloc::Layout;
use core::mem::{ManuallyDrop, MaybeUninit};
use core::ptr::NonNull;

use crate::alloc::*;
extern crate alloc;

impl core::fmt::Display for AllocError {
	fn fmt(&self, fmt: &mut core::fmt::Formatter<'_>) -> Result<(), core::fmt::Error> {
		fmt.write_str("memory allocation failed")
	}
}

#[cfg(any(feature = "std", feature = "core-error"))]
impl crate::Error for AllocError {}

use super::*;

#[inline]
fn to_layout(req: StackReq) -> Result<Layout, AllocError> {
	req.layout().ok().ok_or(AllocError)
}

#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
impl MemBuffer {
	/// Allocate a memory buffer with sufficient storage for the given stack requirements, using the
	/// global allocator.
	///
	/// Calls [`alloc::alloc::handle_alloc_error`] in the case of failure.
	///
	/// # Example
	/// ```
	/// use dyn_stack::{MemBuffer, MemStack, StackReq};
	///
	/// let req = StackReq::new::<i32>(3);
	/// let mut buf = MemBuffer::new(req);
	/// let stack = MemStack::new(&mut buf);
	///
	/// // use the stack
	/// let (arr, _) = stack.make_with::<i32>(3, |i| i as i32);
	/// ```
	pub fn new(req: StackReq) -> Self {
		Self::new_in(req, Global)
	}

	/// Allocate a memory buffer with sufficient storage for the given stack requirements, using the
	/// global allocator, or an error if the allocation did not succeed.
	///
	/// # Example
	/// ```
	/// use dyn_stack::{MemBuffer, MemStack, StackReq};
	///
	/// let req = StackReq::new::<i32>(3);
	/// let mut buf = MemBuffer::new(req);
	/// let stack = MemStack::new(&mut buf);
	///
	/// // use the stack
	/// let (arr, _) = stack.make_with::<i32>(3, |i| i as i32);
	/// ```
	pub fn try_new(req: StackReq) -> Result<Self, AllocError> {
		Self::try_new_in(req, Global)
	}

	/// Creates a `MemBuffer` from its raw components.
	///
	/// # Safety
	///
	/// The arguments to this function must have been acquired from a call to
	/// [`MemBuffer::into_raw_parts`]
	#[inline]
	pub unsafe fn from_raw_parts(ptr: *mut u8, len: usize, align: usize) -> Self {
		Self {
			ptr: NonNull::new_unchecked(ptr),
			len,
			align,
			alloc: Global,
		}
	}

	/// Decomposes a `MemBuffer` into its raw components in this order: ptr, length and
	/// alignment.
	#[inline]
	pub fn into_raw_parts(self) -> (*mut u8, usize, usize) {
		let no_drop = ManuallyDrop::new(self);
		(no_drop.ptr.as_ptr(), no_drop.len, no_drop.align)
	}
}

#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
impl PodBuffer {
	/// Allocate a memory buffer with sufficient storage for the given stack requirements, using the
	/// global allocator.
	///
	/// Calls [`alloc::alloc::handle_alloc_error`] in the case of failure.
	///
	/// # Example
	/// ```
	/// use dyn_stack::{PodBuffer, PodStack, StackReq};
	///
	/// let req = StackReq::new::<i32>(3);
	/// let mut buf = PodBuffer::new(req);
	/// let stack = PodStack::new(&mut buf);
	///
	/// // use the stack
	/// let (arr, _) = stack.make_with::<i32>(3, |i| i as i32);
	/// ```
	pub fn new(req: StackReq) -> Self {
		Self::new_in(req, Global)
	}

	/// Allocate a memory buffer with sufficient storage for the given stack requirements, using the
	/// global allocator, or an error if the allocation did not succeed.
	///
	/// # Example
	/// ```
	/// use dyn_stack::{PodBuffer, PodStack, StackReq};
	///
	/// let req = StackReq::new::<i32>(3);
	/// let mut buf = PodBuffer::new(req);
	/// let stack = PodStack::new(&mut buf);
	///
	/// // use the stack
	/// let (arr, _) = stack.make_with::<i32>(3, |i| i as i32);
	/// ```
	pub fn try_new(req: StackReq) -> Result<Self, AllocError> {
		Self::try_new_in(req, Global)
	}

	/// Creates a `PodBuffer` from its raw components.
	///
	/// # Safety
	///
	/// The arguments to this function must have been acquired from a call to
	/// [`PodBuffer::into_raw_parts`]
	#[inline]
	pub unsafe fn from_raw_parts(ptr: *mut u8, len: usize, align: usize) -> Self {
		Self {
			ptr: NonNull::new_unchecked(ptr),
			len,
			align,
			alloc: Global,
		}
	}

	/// Decomposes a `PodBuffer` into its raw components in this order: ptr, length and
	/// alignment.
	#[inline]
	pub fn into_raw_parts(self) -> (*mut u8, usize, usize) {
		let no_drop = ManuallyDrop::new(self);
		(no_drop.ptr.as_ptr(), no_drop.len, no_drop.align)
	}
}

#[cfg(feature = "alloc")]
/// Buffer of uninitialized bytes to serve as workspace for dynamic arrays.
pub struct MemBuffer<A: Allocator = Global> {
	ptr: NonNull<u8>,
	len: usize,
	align: usize,
	alloc: A,
}

#[cfg(feature = "alloc")]
/// Buffer of initialized bytes to serve as workspace for dynamic arrays.
pub struct PodBuffer<A: Allocator = Global> {
	ptr: NonNull<u8>,
	len: usize,
	align: usize,
	alloc: A,
}

#[cfg(not(feature = "alloc"))]
/// Buffer of uninitialized bytes to serve as workspace for dynamic arrays.
pub struct MemBuffer<A: Allocator> {
	ptr: NonNull<u8>,
	len: usize,
	align: usize,
	alloc: A,
}

#[cfg(not(feature = "alloc"))]
/// Buffer of initialized bytes to serve as workspace for dynamic arrays.
pub struct PodBuffer<A: Allocator> {
	ptr: NonNull<u8>,
	len: usize,
	align: usize,
	alloc: A,
}

unsafe impl<A: Allocator + Sync> Sync for MemBuffer<A> {}
unsafe impl<A: Allocator + Send> Send for MemBuffer<A> {}

unsafe impl<A: Allocator + Sync> Sync for PodBuffer<A> {}
unsafe impl<A: Allocator + Send> Send for PodBuffer<A> {}

impl<A: Allocator> Drop for MemBuffer<A> {
	#[inline]
	fn drop(&mut self) {
		// SAFETY: this was initialized with std::alloc::alloc
		unsafe { self.alloc.deallocate(self.ptr, Layout::from_size_align_unchecked(self.len, self.align)) }
	}
}

impl<A: Allocator> Drop for PodBuffer<A> {
	#[inline]
	fn drop(&mut self) {
		// SAFETY: this was initialized with std::alloc::alloc
		unsafe { self.alloc.deallocate(self.ptr, Layout::from_size_align_unchecked(self.len, self.align)) }
	}
}

impl<A: Allocator> PodBuffer<A> {
	/// Allocate a memory buffer with sufficient storage for the given stack requirements, using the
	/// provided allocator.
	///
	/// Calls [`alloc::alloc::handle_alloc_error`] in the case of failure.
	///
	/// # Example
	/// ```
	/// use dyn_stack::alloc::Global;
	/// use dyn_stack::{PodBuffer, PodStack, StackReq};
	///
	/// let req = StackReq::new::<i32>(3);
	/// let mut buf = PodBuffer::new_in(req, Global);
	/// let stack = PodStack::new(&mut buf);
	///
	/// // use the stack
	/// let (arr, _) = stack.make_with::<i32>(3, |i| i as i32);
	/// ```
	pub fn new_in(req: StackReq, alloc: A) -> Self {
		Self::try_new_in(req, alloc).unwrap_or_else(|_| handle_alloc_error(to_layout(req).unwrap()))
	}

	/// Allocate a memory buffer with sufficient storage for the given stack requirements, using the
	/// provided allocator, or an `AllocError` in the case of failure.
	///
	/// # Example
	/// ```
	/// use dyn_stack::alloc::Global;
	/// use dyn_stack::{PodBuffer, PodStack, StackReq};
	///
	/// let req = StackReq::new::<i32>(3);
	/// let mut buf = PodBuffer::new_in(req, Global);
	/// let stack = PodStack::new(&mut buf);
	///
	/// // use the stack
	/// let (arr, _) = stack.make_with::<i32>(3, |i| i as i32);
	/// ```
	pub fn try_new_in(req: StackReq, alloc: A) -> Result<Self, AllocError> {
		unsafe {
			let ptr = &mut *(alloc.allocate_zeroed(to_layout(req)?).map_err(|_| AllocError)?.as_ptr() as *mut [MaybeUninit<u8>]);
			#[cfg(debug_assertions)]
			ptr.fill(MaybeUninit::new(0xCD));

			let len = ptr.len();
			let ptr = NonNull::new_unchecked(ptr.as_mut_ptr() as *mut u8);
			Ok(PodBuffer {
				alloc,
				ptr,
				len,
				align: req.align_bytes(),
			})
		}
	}

	/// Creates a `PodBuffer` from its raw components.
	///
	/// # Safety
	///
	/// The arguments to this function must have been acquired from a call to
	/// [`PodBuffer::into_raw_parts`]
	#[inline]
	pub unsafe fn from_raw_parts_in(ptr: *mut u8, len: usize, align: usize, alloc: A) -> Self {
		Self {
			ptr: NonNull::new_unchecked(ptr),
			len,
			align,
			alloc,
		}
	}

	/// Decomposes a `PodBuffer` into its raw components in this order: ptr, length and
	/// alignment.
	#[inline]
	pub fn into_raw_parts_with_alloc(self) -> (*mut u8, usize, usize, A) {
		let me = ManuallyDrop::new(self);
		(me.ptr.as_ptr(), me.len, me.align, unsafe {
			core::ptr::read(core::ptr::addr_of!(me.alloc))
		})
	}
}

impl<A: Allocator> MemBuffer<A> {
	/// Allocate a memory buffer with sufficient storage for the given stack requirements, using the
	/// provided allocator.
	///
	/// Calls [`alloc::alloc::handle_alloc_error`] in the case of failure.
	///
	/// # Example
	/// ```
	/// use dyn_stack::alloc::Global;
	/// use dyn_stack::{MemBuffer, MemStack, StackReq};
	///
	/// let req = StackReq::new::<i32>(3);
	/// let mut buf = MemBuffer::new_in(req, Global);
	/// let stack = MemStack::new(&mut buf);
	///
	/// // use the stack
	/// let (arr, _) = stack.make_with::<i32>(3, |i| i as i32);
	/// ```
	pub fn new_in(req: StackReq, alloc: A) -> Self {
		Self::try_new_in(req, alloc).unwrap_or_else(|_| handle_alloc_error(to_layout(req).unwrap()))
	}

	/// Allocate a memory buffer with sufficient storage for the given stack requirements, using the
	/// provided allocator, or an `AllocError` in the case of failure.
	///
	/// # Example
	/// ```
	/// use dyn_stack::alloc::Global;
	/// use dyn_stack::{MemBuffer, MemStack, StackReq};
	///
	/// let req = StackReq::new::<i32>(3);
	/// let mut buf = MemBuffer::new_in(req, Global);
	/// let stack = MemStack::new(&mut buf);
	///
	/// // use the stack
	/// let (arr, _) = stack.make_with::<i32>(3, |i| i as i32);
	/// ```
	pub fn try_new_in(req: StackReq, alloc: A) -> Result<Self, AllocError> {
		unsafe {
			let ptr = &mut *(alloc.allocate(to_layout(req)?).map_err(|_| AllocError)?.as_ptr() as *mut [MaybeUninit<u8>]);
			let len = ptr.len();
			let ptr = NonNull::new_unchecked(ptr.as_mut_ptr() as *mut u8);
			Ok(MemBuffer {
				alloc,
				ptr,
				len,
				align: req.align_bytes(),
			})
		}
	}

	/// Creates a `MemBuffer` from its raw components.
	///
	/// # Safety
	///
	/// The arguments to this function must have been acquired from a call to
	/// [`MemBuffer::into_raw_parts`]
	#[inline]
	pub unsafe fn from_raw_parts_in(ptr: *mut u8, len: usize, align: usize, alloc: A) -> Self {
		Self {
			ptr: NonNull::new_unchecked(ptr),
			len,
			align,
			alloc,
		}
	}

	/// Decomposes a `MemBuffer` into its raw components in this order: ptr, length and
	/// alignment.
	#[inline]
	pub fn into_raw_parts_with_alloc(self) -> (*mut u8, usize, usize, A) {
		let me = ManuallyDrop::new(self);
		(me.ptr.as_ptr(), me.len, me.align, unsafe {
			core::ptr::read(core::ptr::addr_of!(me.alloc))
		})
	}
}

impl<A: Allocator> core::ops::Deref for MemBuffer<A> {
	type Target = [MaybeUninit<u8>];

	#[inline]
	fn deref(&self) -> &Self::Target {
		unsafe { core::slice::from_raw_parts(self.ptr.as_ptr() as *const MaybeUninit<u8>, self.len) }
	}
}

impl<A: Allocator> core::ops::DerefMut for MemBuffer<A> {
	#[inline]
	fn deref_mut(&mut self) -> &mut Self::Target {
		unsafe { core::slice::from_raw_parts_mut(self.ptr.as_ptr() as *mut MaybeUninit<u8>, self.len) }
	}
}

impl<A: Allocator> core::ops::Deref for PodBuffer<A> {
	type Target = [u8];

	#[inline]
	fn deref(&self) -> &Self::Target {
		unsafe { core::slice::from_raw_parts(self.ptr.as_ptr(), self.len) }
	}
}

impl<A: Allocator> core::ops::DerefMut for PodBuffer<A> {
	#[inline]
	fn deref_mut(&mut self) -> &mut Self::Target {
		unsafe { core::slice::from_raw_parts_mut(self.ptr.as_ptr(), self.len) }
	}
}

/// Error during memory allocation.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct AllocError;
