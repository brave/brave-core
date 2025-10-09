// copied from libcore/liballoc

use core::alloc::Layout;
use core::cell::UnsafeCell;
use core::marker::PhantomData;
use core::mem::MaybeUninit;
use core::ptr::NonNull;
use core::{fmt, ptr};

extern crate alloc;

#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub struct AllocError;

#[cfg(any(feature = "std", feature = "core-error"))]
impl crate::Error for AllocError {}

impl fmt::Display for AllocError {
	fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
		f.write_str("memory allocation failed")
	}
}

/// An implementation of `Allocator` can allocate, grow, shrink, and deallocate arbitrary blocks of
/// data described via [`Layout`][].
///
/// `Allocator` is designed to be implemented on ZSTs, references, or smart pointers because having
/// an allocator like `MyAllocator([u8; N])` cannot be moved, without updating the pointers to the
/// allocated memory.
///
/// Unlike [`alloc::alloc::GlobalAlloc`][], zero-sized allocations are allowed in `Allocator`. If an
/// underlying allocator does not support this (like jemalloc) or return a null pointer (such as
/// `libc::malloc`), this must be caught by the implementation.
///
/// ### Currently allocated memory
///
/// Some of the methods require that a memory block be *currently allocated* via an allocator. This
/// means that:
///
/// * the starting address for that memory block was previously returned by [`allocate`], [`grow`],
///   or [`shrink`], and
///
/// * the memory block has not been subsequently deallocated, where blocks are either deallocated
///   directly by being passed to [`deallocate`] or were changed by being passed to [`grow`] or
///   [`shrink`] that returns `Ok`. If `grow` or `shrink` have returned `Err`, the passed pointer
///   remains valid.
///
/// [`allocate`]: Allocator::allocate
/// [`grow`]: Allocator::grow
/// [`shrink`]: Allocator::shrink
/// [`deallocate`]: Allocator::deallocate
///
/// ### Memory fitting
///
/// Some of the methods require that a layout *fit* a memory block. What it means for a layout to
/// "fit" a memory block means (or equivalently, for a memory block to "fit" a layout) is that the
/// following conditions must hold:
///
/// * The block must be allocated with the same alignment as [`layout.align()`], and
///
/// * The provided [`layout.size()`] must fall in the range `min ..= max`, where:
///   - `min` is the size of the layout most recently used to allocate the block, and
///   - `max` is the latest actual size returned from [`allocate`], [`grow`], or [`shrink`].
///
/// [`layout.align()`]: Layout::align
/// [`layout.size()`]: Layout::size
///
/// # Safety
///
/// * Memory blocks returned from an allocator that are [*currently allocated*] must point to valid
///   memory and retain their validity while they are [*currently allocated*] and the shorter of:
///   - the borrow-checker lifetime of the allocator type itself.
///
/// * any pointer to a memory block which is [*currently allocated*] may be passed to any other
///   method of the allocator.
///
/// [*currently allocated*]: #currently-allocated-memory
pub unsafe trait Allocator {
	/// Attempts to allocate a block of memory.
	///
	/// On success, returns a [`NonNull<[u8]>`][NonNull] meeting the size and alignment guarantees
	/// of `layout`.
	///
	/// The returned block may have a larger size than specified by `layout.size()`, and may or may
	/// not have its contents initialized.
	///
	/// The returned block of memory remains valid as long as it is [*currently allocated*] and the
	/// shorter of:
	///   - the borrow-checker lifetime of the allocator type itself.
	///
	/// # Errors
	///
	/// Returning `Err` indicates that either memory is exhausted or `layout` does not meet
	/// allocator's size or alignment constraints.
	///
	/// Implementations are encouraged to return `Err` on memory exhaustion rather than panicking or
	/// aborting, but this is not a strict requirement. (Specifically: it is *legal* to implement
	/// this trait atop an underlying native allocation library that aborts on memory exhaustion.)
	///
	/// Clients wishing to abort computation in response to an allocation error are encouraged to
	/// call the [`handle_alloc_error`] function, rather than directly invoking `panic!` or similar.
	///
	/// [`handle_alloc_error`]: ../../alloc/alloc/fn.handle_alloc_error.html
	fn allocate(&self, layout: Layout) -> Result<NonNull<[u8]>, AllocError>;

	/// Behaves like `allocate`, but also ensures that the returned memory is zero-initialized.
	///
	/// # Errors
	///
	/// Returning `Err` indicates that either memory is exhausted or `layout` does not meet
	/// allocator's size or alignment constraints.
	///
	/// Implementations are encouraged to return `Err` on memory exhaustion rather than panicking or
	/// aborting, but this is not a strict requirement. (Specifically: it is *legal* to implement
	/// this trait atop an underlying native allocation library that aborts on memory exhaustion.)
	///
	/// Clients wishing to abort computation in response to an allocation error are encouraged to
	/// call the [`handle_alloc_error`] function, rather than directly invoking `panic!` or similar.
	///
	/// [`handle_alloc_error`]: ../../alloc/alloc/fn.handle_alloc_error.html
	fn allocate_zeroed(&self, layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		let ptr = self.allocate(layout)?;
		// SAFETY: `alloc` returns a valid memory block
		unsafe { (ptr.as_ptr() as *mut u8).write_bytes(0, ptr.len()) }
		Ok(ptr)
	}

	/// Deallocates the memory referenced by `ptr`.
	///
	/// # Safety
	///
	/// * `ptr` must denote a block of memory [*currently allocated*] via this allocator, and
	/// * `layout` must [*fit*] that block of memory.
	///
	/// [*currently allocated*]: #currently-allocated-memory
	/// [*fit*]: #memory-fitting
	unsafe fn deallocate(&self, ptr: NonNull<u8>, layout: Layout);

	/// Attempts to extend the memory block.
	///
	/// Returns a new [`NonNull<[u8]>`][NonNull] containing a pointer and the actual size of the
	/// allocated memory. The pointer is suitable for holding data described by `new_layout`. To
	/// accomplish this, the allocator may extend the allocation referenced by `ptr` to fit the new
	/// layout.
	///
	/// If this returns `Ok`, then ownership of the memory block referenced by `ptr` has been
	/// transferred to this allocator. Any access to the old `ptr` is Undefined Behavior, even if
	/// the allocation was grown in-place. The newly returned pointer is the only valid pointer
	/// for accessing this memory now.
	///
	/// If this method returns `Err`, then ownership of the memory block has not been transferred to
	/// this allocator, and the contents of the memory block are unaltered.
	///
	/// # Safety
	///
	/// * `ptr` must denote a block of memory [*currently allocated*] via this allocator.
	/// * `old_layout` must [*fit*] that block of memory (The `new_layout` argument need not fit
	///   it.).
	/// * `new_layout.size()` must be greater than or equal to `old_layout.size()`.
	///
	/// Note that `new_layout.align()` need not be the same as `old_layout.align()`.
	///
	/// [*currently allocated*]: #currently-allocated-memory
	/// [*fit*]: #memory-fitting
	///
	/// # Errors
	///
	/// Returns `Err` if the new layout does not meet the allocator's size and alignment
	/// constraints of the allocator, or if growing otherwise fails.
	///
	/// Implementations are encouraged to return `Err` on memory exhaustion rather than panicking or
	/// aborting, but this is not a strict requirement. (Specifically: it is *legal* to implement
	/// this trait atop an underlying native allocation library that aborts on memory exhaustion.)
	///
	/// Clients wishing to abort computation in response to an allocation error are encouraged to
	/// call the [`handle_alloc_error`] function, rather than directly invoking `panic!` or similar.
	///
	/// [`handle_alloc_error`]: ../../alloc/alloc/fn.handle_alloc_error.html
	unsafe fn grow(&self, ptr: NonNull<u8>, old_layout: Layout, new_layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		debug_assert!(
			new_layout.size() >= old_layout.size(),
			"`new_layout.size()` must be greater than or equal to `old_layout.size()`"
		);

		let new_ptr = self.allocate(new_layout)?;

		// SAFETY: because `new_layout.size()` must be greater than or equal to
		// `old_layout.size()`, both the old and new memory allocation are valid for reads and
		// writes for `old_layout.size()` bytes. Also, because the old allocation wasn't yet
		// deallocated, it cannot overlap `new_ptr`. Thus, the call to `copy_nonoverlapping` is
		// safe. The safety contract for `dealloc` must be upheld by the caller.
		unsafe {
			ptr::copy_nonoverlapping(ptr.as_ptr(), new_ptr.as_ptr() as *mut u8, old_layout.size());
			self.deallocate(ptr, old_layout);
		}

		Ok(new_ptr)
	}

	/// Behaves like `grow`, but also ensures that the new contents are set to zero before being
	/// returned.
	///
	/// The memory block will contain the following contents after a successful call to
	/// `grow_zeroed`:
	///   * Bytes `0..old_layout.size()` are preserved from the original allocation.
	///   * Bytes `old_layout.size()..old_size` will either be preserved or zeroed, depending on the
	///     allocator implementation. `old_size` refers to the size of the memory block prior to the
	///     `grow_zeroed` call, which may be larger than the size that was originally requested when
	///     it was allocated.
	///   * Bytes `old_size..new_size` are zeroed. `new_size` refers to the size of the memory block
	///     returned by the `grow_zeroed` call.
	///
	/// # Safety
	///
	/// * `ptr` must denote a block of memory [*currently allocated*] via this allocator.
	/// * `old_layout` must [*fit*] that block of memory (The `new_layout` argument need not fit
	///   it.).
	/// * `new_layout.size()` must be greater than or equal to `old_layout.size()`.
	///
	/// Note that `new_layout.align()` need not be the same as `old_layout.align()`.
	///
	/// [*currently allocated*]: #currently-allocated-memory
	/// [*fit*]: #memory-fitting
	///
	/// # Errors
	///
	/// Returns `Err` if the new layout does not meet the allocator's size and alignment
	/// constraints of the allocator, or if growing otherwise fails.
	///
	/// Implementations are encouraged to return `Err` on memory exhaustion rather than panicking or
	/// aborting, but this is not a strict requirement. (Specifically: it is *legal* to implement
	/// this trait atop an underlying native allocation library that aborts on memory exhaustion.)
	///
	/// Clients wishing to abort computation in response to an allocation error are encouraged to
	/// call the [`handle_alloc_error`] function, rather than directly invoking `panic!` or similar.
	///
	/// [`handle_alloc_error`]: ../../alloc/alloc/fn.handle_alloc_error.html
	unsafe fn grow_zeroed(&self, ptr: NonNull<u8>, old_layout: Layout, new_layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		debug_assert!(
			new_layout.size() >= old_layout.size(),
			"`new_layout.size()` must be greater than or equal to `old_layout.size()`"
		);

		let new_ptr = self.allocate_zeroed(new_layout)?;

		// SAFETY: because `new_layout.size()` must be greater than or equal to
		// `old_layout.size()`, both the old and new memory allocation are valid for reads and
		// writes for `old_layout.size()` bytes. Also, because the old allocation wasn't yet
		// deallocated, it cannot overlap `new_ptr`. Thus, the call to `copy_nonoverlapping` is
		// safe. The safety contract for `dealloc` must be upheld by the caller.
		unsafe {
			ptr::copy_nonoverlapping(ptr.as_ptr(), new_ptr.as_ptr() as *mut u8, old_layout.size());
			self.deallocate(ptr, old_layout);
		}

		Ok(new_ptr)
	}

	/// Attempts to shrink the memory block.
	///
	/// Returns a new [`NonNull<[u8]>`][NonNull] containing a pointer and the actual size of the
	/// allocated memory. The pointer is suitable for holding data described by `new_layout`. To
	/// accomplish this, the allocator may shrink the allocation referenced by `ptr` to fit the new
	/// layout.
	///
	/// If this returns `Ok`, then ownership of the memory block referenced by `ptr` has been
	/// transferred to this allocator. Any access to the old `ptr` is Undefined Behavior, even if
	/// the allocation was shrunk in-place. The newly returned pointer is the only valid pointer
	/// for accessing this memory now.
	///
	/// If this method returns `Err`, then ownership of the memory block has not been transferred to
	/// this allocator, and the contents of the memory block are unaltered.
	///
	/// # Safety
	///
	/// * `ptr` must denote a block of memory [*currently allocated*] via this allocator.
	/// * `old_layout` must [*fit*] that block of memory (The `new_layout` argument need not fit
	///   it.).
	/// * `new_layout.size()` must be smaller than or equal to `old_layout.size()`.
	///
	/// Note that `new_layout.align()` need not be the same as `old_layout.align()`.
	///
	/// [*currently allocated*]: #currently-allocated-memory
	/// [*fit*]: #memory-fitting
	///
	/// # Errors
	///
	/// Returns `Err` if the new layout does not meet the allocator's size and alignment
	/// constraints of the allocator, or if shrinking otherwise fails.
	///
	/// Implementations are encouraged to return `Err` on memory exhaustion rather than panicking or
	/// aborting, but this is not a strict requirement. (Specifically: it is *legal* to implement
	/// this trait atop an underlying native allocation library that aborts on memory exhaustion.)
	///
	/// Clients wishing to abort computation in response to an allocation error are encouraged to
	/// call the [`handle_alloc_error`] function, rather than directly invoking `panic!` or similar.
	///
	/// [`handle_alloc_error`]: ../../alloc/alloc/fn.handle_alloc_error.html
	unsafe fn shrink(&self, ptr: NonNull<u8>, old_layout: Layout, new_layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		debug_assert!(
			new_layout.size() <= old_layout.size(),
			"`new_layout.size()` must be smaller than or equal to `old_layout.size()`"
		);

		let new_ptr = self.allocate(new_layout)?;

		// SAFETY: because `new_layout.size()` must be lower than or equal to
		// `old_layout.size()`, both the old and new memory allocation are valid for reads and
		// writes for `new_layout.size()` bytes. Also, because the old allocation wasn't yet
		// deallocated, it cannot overlap `new_ptr`. Thus, the call to `copy_nonoverlapping` is
		// safe. The safety contract for `dealloc` must be upheld by the caller.
		unsafe {
			ptr::copy_nonoverlapping(ptr.as_ptr(), new_ptr.as_ptr() as *mut u8, new_layout.size());
			self.deallocate(ptr, old_layout);
		}

		Ok(new_ptr)
	}

	/// Creates a "by reference" adapter for this instance of `Allocator`.
	///
	/// The returned adapter also implements `Allocator` and will simply borrow this.
	#[inline(always)]
	fn by_ref(&self) -> &Self
	where
		Self: Sized,
	{
		self
	}
}

unsafe impl<T: ?Sized + Allocator> Allocator for &T {
	#[inline(always)]
	fn allocate(&self, layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		(**self).allocate(layout)
	}

	#[inline(always)]
	unsafe fn deallocate(&self, ptr: NonNull<u8>, layout: Layout) {
		(**self).deallocate(ptr, layout)
	}

	#[inline(always)]
	fn allocate_zeroed(&self, layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		(**self).allocate_zeroed(layout)
	}

	#[inline(always)]
	unsafe fn grow(&self, ptr: NonNull<u8>, old_layout: Layout, new_layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		(**self).grow(ptr, old_layout, new_layout)
	}

	#[inline(always)]
	unsafe fn grow_zeroed(&self, ptr: NonNull<u8>, old_layout: Layout, new_layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		(**self).grow_zeroed(ptr, old_layout, new_layout)
	}

	#[inline(always)]
	unsafe fn shrink(&self, ptr: NonNull<u8>, old_layout: Layout, new_layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		(**self).shrink(ptr, old_layout, new_layout)
	}
}

unsafe impl<T: ?Sized + Allocator> Allocator for &mut T {
	#[inline(always)]
	fn allocate(&self, layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		(**self).allocate(layout)
	}

	#[inline(always)]
	unsafe fn deallocate(&self, ptr: NonNull<u8>, layout: Layout) {
		(**self).deallocate(ptr, layout)
	}

	#[inline(always)]
	fn allocate_zeroed(&self, layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		(**self).allocate_zeroed(layout)
	}

	#[inline(always)]
	unsafe fn grow(&self, ptr: NonNull<u8>, old_layout: Layout, new_layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		(**self).grow(ptr, old_layout, new_layout)
	}

	#[inline(always)]
	unsafe fn grow_zeroed(&self, ptr: NonNull<u8>, old_layout: Layout, new_layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		(**self).grow_zeroed(ptr, old_layout, new_layout)
	}

	#[inline(always)]
	unsafe fn shrink(&self, ptr: NonNull<u8>, old_layout: Layout, new_layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		(**self).shrink(ptr, old_layout, new_layout)
	}
}

#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
unsafe impl<T: ?Sized + Allocator> Allocator for alloc::boxed::Box<T> {
	#[inline(always)]
	fn allocate(&self, layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		(**self).allocate(layout)
	}

	#[inline(always)]
	unsafe fn deallocate(&self, ptr: NonNull<u8>, layout: Layout) {
		(**self).deallocate(ptr, layout)
	}

	#[inline(always)]
	fn allocate_zeroed(&self, layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		(**self).allocate_zeroed(layout)
	}

	#[inline(always)]
	unsafe fn grow(&self, ptr: NonNull<u8>, old_layout: Layout, new_layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		(**self).grow(ptr, old_layout, new_layout)
	}

	#[inline(always)]
	unsafe fn grow_zeroed(&self, ptr: NonNull<u8>, old_layout: Layout, new_layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		(**self).grow_zeroed(ptr, old_layout, new_layout)
	}

	#[inline(always)]
	unsafe fn shrink(&self, ptr: NonNull<u8>, old_layout: Layout, new_layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		(**self).shrink(ptr, old_layout, new_layout)
	}
}

#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
pub struct Global;

#[cfg(feature = "alloc")]
#[cfg_attr(docsrs, doc(cfg(feature = "alloc")))]
unsafe impl Allocator for Global {
	fn allocate(&self, layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		let ptr = if layout.size() == 0 {
			core::ptr::null_mut::<u8>().wrapping_add(layout.align())
		} else {
			unsafe { alloc::alloc::alloc(layout) }
		};

		if ptr.is_null() {
			Err(AllocError)
		} else {
			Ok(unsafe { NonNull::new_unchecked(core::ptr::slice_from_raw_parts_mut(ptr, layout.size())) })
		}
	}

	fn allocate_zeroed(&self, layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		let ptr = if layout.size() == 0 {
			core::ptr::null_mut::<u8>().wrapping_add(layout.align())
		} else {
			unsafe { alloc::alloc::alloc_zeroed(layout) }
		};

		if ptr.is_null() {
			Err(AllocError)
		} else {
			Ok(unsafe { NonNull::new_unchecked(core::ptr::slice_from_raw_parts_mut(ptr, layout.size())) })
		}
	}

	unsafe fn deallocate(&self, ptr: NonNull<u8>, layout: Layout) {
		if layout.size() != 0 {
			alloc::alloc::dealloc(ptr.as_ptr(), layout);
		}
	}

	unsafe fn grow(&self, ptr: NonNull<u8>, old_layout: Layout, new_layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		core::debug_assert!(
			new_layout.size() >= old_layout.size(),
			"`new_layout.size()` must be greater than or equal to `old_layout.size()`"
		);

		if old_layout.align() == new_layout.align() {
			let ptr = if new_layout.size() == 0 {
				core::ptr::null_mut::<u8>().wrapping_add(new_layout.align())
			} else {
				alloc::alloc::realloc(ptr.as_ptr(), old_layout, new_layout.size())
			};
			if ptr.is_null() {
				Err(AllocError)
			} else {
				Ok(unsafe { NonNull::new_unchecked(core::ptr::slice_from_raw_parts_mut(ptr, new_layout.size())) })
			}
		} else {
			let new_ptr = self.allocate(new_layout)?;

			// SAFETY: because `new_layout.size()` must be greater than or equal to
			// `old_layout.size()`, both the old and new memory allocation are valid for reads and
			// writes for `old_layout.size()` bytes. Also, because the old allocation wasn't yet
			// deallocated, it cannot overlap `new_ptr`. Thus, the call to `copy_nonoverlapping` is
			// safe. The safety contract for `dealloc` must be upheld by the caller.
			unsafe {
				ptr::copy_nonoverlapping(ptr.as_ptr(), new_ptr.as_ptr() as *mut u8, old_layout.size());
				self.deallocate(ptr, old_layout);
			}

			Ok(new_ptr)
		}
	}

	unsafe fn shrink(&self, ptr: NonNull<u8>, old_layout: Layout, new_layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		core::debug_assert!(
			new_layout.size() <= old_layout.size(),
			"`new_layout.size()` must be smaller than or equal to `old_layout.size()`"
		);

		if old_layout.align() == new_layout.align() {
			let ptr = if new_layout.size() == 0 {
				core::ptr::null_mut::<u8>().wrapping_add(new_layout.align())
			} else {
				alloc::alloc::realloc(ptr.as_ptr(), old_layout, new_layout.size())
			};

			if ptr.is_null() {
				Err(AllocError)
			} else {
				Ok(unsafe { NonNull::new_unchecked(core::ptr::slice_from_raw_parts_mut(ptr, new_layout.size())) })
			}
		} else {
			let new_ptr = self.allocate(new_layout)?;

			// SAFETY: because `new_layout.size()` must be lower than or equal to
			// `old_layout.size()`, both the old and new memory allocation are valid for reads and
			// writes for `new_layout.size()` bytes. Also, because the old allocation wasn't yet
			// deallocated, it cannot overlap `new_ptr`. Thus, the call to `copy_nonoverlapping` is
			// safe. The safety contract for `dealloc` must be upheld by the caller.
			unsafe {
				ptr::copy_nonoverlapping(ptr.as_ptr(), new_ptr.as_ptr() as *mut u8, new_layout.size());
				self.deallocate(ptr, old_layout);
			}

			Ok(new_ptr)
		}
	}
}

#[derive(Copy, Clone, Debug)]
pub(crate) struct VTable {
	pub allocate: unsafe fn(*const (), Layout) -> Result<NonNull<[u8]>, AllocError>,
	pub allocate_zeroed: unsafe fn(*const (), Layout) -> Result<NonNull<[u8]>, AllocError>,
	pub deallocate: unsafe fn(*const (), ptr: NonNull<u8>, Layout),
	pub grow: unsafe fn(*const (), NonNull<u8>, Layout, Layout) -> Result<NonNull<[u8]>, AllocError>,
	pub grow_zeroed: unsafe fn(*const (), NonNull<u8>, Layout, Layout) -> Result<NonNull<[u8]>, AllocError>,
	pub shrink: unsafe fn(*const (), NonNull<u8>, Layout, Layout) -> Result<NonNull<[u8]>, AllocError>,

	pub clone: Option<unsafe fn(*mut (), *const ())>,
	pub drop: unsafe fn(*mut ()),
}

pub struct DynAlloc<'a> {
	pub(crate) alloc: UnsafeCell<MaybeUninit<*const ()>>,
	pub(crate) vtable: &'static VTable,
	__marker: PhantomData<&'a ()>,
}

unsafe impl Send for DynAlloc<'_> {}

unsafe impl Allocator for DynAlloc<'_> {
	#[inline]
	fn allocate(&self, layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		unsafe { (self.vtable.allocate)(core::ptr::addr_of!(self.alloc) as *const (), layout) }
	}

	#[inline]
	unsafe fn deallocate(&self, ptr: NonNull<u8>, layout: Layout) {
		unsafe { (self.vtable.deallocate)(core::ptr::addr_of!(self.alloc) as *const (), ptr, layout) }
	}

	#[inline]
	fn allocate_zeroed(&self, layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		unsafe { (self.vtable.allocate_zeroed)(core::ptr::addr_of!(self.alloc) as *const (), layout) }
	}

	#[inline]
	unsafe fn grow(&self, ptr: NonNull<u8>, old_layout: Layout, new_layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		unsafe { (self.vtable.grow)(core::ptr::addr_of!(self.alloc) as *const (), ptr, old_layout, new_layout) }
	}

	#[inline]
	unsafe fn grow_zeroed(&self, ptr: NonNull<u8>, old_layout: Layout, new_layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		unsafe { (self.vtable.grow_zeroed)(core::ptr::addr_of!(self.alloc) as *const (), ptr, old_layout, new_layout) }
	}

	#[inline]
	unsafe fn shrink(&self, ptr: NonNull<u8>, old_layout: Layout, new_layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
		unsafe { (self.vtable.shrink)(core::ptr::addr_of!(self.alloc) as *const (), ptr, old_layout, new_layout) }
	}
}

impl Drop for DynAlloc<'_> {
	#[inline]
	fn drop(&mut self) {
		unsafe { (self.vtable.drop)(core::ptr::addr_of_mut!(self.alloc) as *mut ()) }
	}
}

impl Clone for DynAlloc<'_> {
	#[inline]
	fn clone(&self) -> Self {
		let mut alloc = UnsafeCell::new(MaybeUninit::uninit());
		unsafe {
			self.vtable.clone.unwrap()(core::ptr::addr_of_mut!(alloc) as *mut (), core::ptr::addr_of!(self.alloc) as *const ());
		}

		Self {
			alloc,
			vtable: self.vtable,
			__marker: PhantomData,
		}
	}
}

impl<'a> DynAlloc<'a> {
	#[inline]
	pub fn try_new_unclone<A: 'a + Allocator + Send>(alloc: A) -> Result<Self, A> {
		if core::mem::size_of::<A>() <= core::mem::size_of::<*const ()>() && core::mem::align_of::<A>() <= core::mem::align_of::<*const ()>() {
			trait AllocUnclone: Allocator + Send {
				const VTABLE: &'static VTable = &unsafe {
					VTable {
						allocate: core::mem::transmute(Self::allocate as fn(&Self, _) -> _),
						allocate_zeroed: core::mem::transmute(Self::allocate_zeroed as fn(&Self, _) -> _),
						deallocate: core::mem::transmute(Self::deallocate as unsafe fn(&Self, _, _) -> _),
						grow: core::mem::transmute(Self::grow as unsafe fn(&Self, _, _, _) -> _),
						grow_zeroed: core::mem::transmute(Self::grow_zeroed as unsafe fn(&Self, _, _, _) -> _),
						shrink: core::mem::transmute(Self::shrink as unsafe fn(&Self, _, _, _) -> _),

						clone: None,
						drop: core::mem::transmute(core::ptr::drop_in_place::<Self> as unsafe fn(_) -> _),
					}
				};
			}
			impl<A: Allocator + Send> AllocUnclone for A {}

			Ok(Self {
				alloc: unsafe { core::mem::transmute_copy(&core::mem::ManuallyDrop::new(alloc)) },
				vtable: <A as AllocUnclone>::VTABLE,
				__marker: PhantomData,
			})
		} else {
			Err(alloc)
		}
	}

	#[inline]
	pub fn try_new_clone<A: 'a + Clone + Allocator + Send>(alloc: A) -> Result<Self, A> {
		if core::mem::size_of::<A>() <= core::mem::size_of::<*const ()>() && core::mem::align_of::<A>() <= core::mem::align_of::<*const ()>() {
			trait AllocClone: Allocator + Send + Clone {
				const VTABLE: &'static VTable = &unsafe {
					VTable {
						allocate: core::mem::transmute(Self::allocate as fn(_, _) -> _),
						allocate_zeroed: core::mem::transmute(Self::allocate_zeroed as fn(_, _) -> _),
						deallocate: core::mem::transmute(Self::deallocate as unsafe fn(_, _, _) -> _),
						grow: core::mem::transmute(Self::grow as unsafe fn(_, _, _, _) -> _),
						grow_zeroed: core::mem::transmute(Self::grow_zeroed as unsafe fn(_, _, _, _) -> _),
						shrink: core::mem::transmute(Self::shrink as unsafe fn(_, _, _, _) -> _),

						clone: Some(|dst: *mut (), src: *const ()| (dst as *mut Self).write((*(src as *const Self)).clone())),
						drop: core::mem::transmute(core::ptr::drop_in_place::<Self> as unsafe fn(_) -> _),
					}
				};
			}
			impl<A: Allocator + Send + Clone> AllocClone for A {}

			Ok(Self {
				alloc: unsafe { core::mem::transmute_copy(&core::mem::ManuallyDrop::new(alloc)) },
				vtable: <A as AllocClone>::VTABLE,
				__marker: PhantomData,
			})
		} else {
			Err(alloc)
		}
	}

	#[inline]
	pub fn from_ref<A: Allocator + Sync>(alloc: &'a A) -> Self {
		match Self::try_new_clone(alloc) {
			Ok(me) => me,
			Err(_) => unreachable!(),
		}
	}

	#[inline]
	pub fn from_mut<A: Allocator + Send>(alloc: &'a mut A) -> Self {
		match Self::try_new_unclone(alloc) {
			Ok(me) => me,
			Err(_) => unreachable!(),
		}
	}

	#[inline]
	pub fn by_mut(&mut self) -> DynAlloc<'_> {
		DynAlloc::from_mut(self)
	}

	#[inline]
	pub fn cloneable(&self) -> bool {
		self.vtable.clone.is_some()
	}
}
