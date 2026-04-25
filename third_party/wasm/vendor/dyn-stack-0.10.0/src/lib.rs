#![cfg_attr(feature = "nightly", feature(allocator_api, dropck_eyepatch))]
#![cfg_attr(not(feature = "std"), no_std)]
#![cfg_attr(docsrs, feature(doc_cfg))]

//! Stack that allows users to allocate dynamically sized arrays.
//!
//! The stack wraps a buffer of bytes that it uses as a workspace.
//! Allocating an array takes a chunk of memory from the stack, which can be reused once the array
//! is dropped.
//!
//! # Examples:
//! ```
//! use core::mem::MaybeUninit;
//! use dyn_stack::{DynStack, StackReq};
//! use reborrow::ReborrowMut;
//!
//! // We allocate enough storage for 3 `i32` and 4 `u8`.
//! let mut buf = [MaybeUninit::uninit();
//!     StackReq::new::<i32>(3)
//!         .and(StackReq::new::<u8>(4))
//!         .unaligned_bytes_required()];
//! let mut stack = DynStack::new(&mut buf);
//!
//! {
//!     // We can have nested allocations.
//!     // 3×`i32`
//!     let (array_i32, substack) = stack.rb_mut().make_with::<i32, _>(3, |i| i as i32);
//!     // and 4×`u8`
//!     let (mut array_u8, _) = substack.make_with::<u8, _>(4, |_| 0);
//!
//!     // We can read from the arrays,
//!     assert_eq!(array_i32[0], 0);
//!     assert_eq!(array_i32[1], 1);
//!     assert_eq!(array_i32[2], 2);
//!
//!     // and write to them.
//!     array_u8[0] = 1;
//!
//!     assert_eq!(array_u8[0], 1);
//!     assert_eq!(array_u8[1], 0);
//!     assert_eq!(array_u8[2], 0);
//!     assert_eq!(array_u8[3], 0);
//! }
//!
//! // We can also have disjoint allocations.
//! {
//!     // 3×`i32`
//!     let (mut array_i32, _) = stack.rb_mut().make_with::<i32, _>(3, |i| i as i32);
//!     assert_eq!(array_i32[0], 0);
//!     assert_eq!(array_i32[1], 1);
//!     assert_eq!(array_i32[2], 2);
//! }
//!
//! {
//!     // or 4×`u8`
//!     let (mut array_u8, _) = stack.rb_mut().make_with::<i32, _>(4, |i| i as i32 + 3);
//!     assert_eq!(array_u8[0], 3);
//!     assert_eq!(array_u8[1], 4);
//!     assert_eq!(array_u8[2], 5);
//!     assert_eq!(array_u8[3], 6);
//! }
//! ```

extern crate alloc;

pub mod mem;

use bytemuck::Pod;
#[cfg(feature = "nightly")]
pub use mem::MemBuffer;

pub use mem::GlobalMemBuffer;
pub use mem::GlobalPodBuffer;

mod stack_req;
pub use stack_req::{SizeOverflow, StackReq};

use core::fmt::Debug;
use core::marker::PhantomData;
use core::mem::MaybeUninit;
use core::ptr::NonNull;
pub use reborrow::ReborrowMut;

/// Stack wrapper around a buffer of uninitialized bytes.
pub struct DynStack<'a> {
    buffer: &'a mut [MaybeUninit<u8>],
}

/// Stack wrapper around a buffer of bytes.
pub struct PodStack<'a> {
    buffer: &'a mut [u8],
}

/// Owns an unsized array of data, allocated from some stack.
pub struct DynArray<'a, T> {
    ptr: NonNull<T>,
    len: usize,
    _marker: (PhantomData<&'a ()>, PhantomData<T>),
}

impl<'a, T: Debug> Debug for DynArray<'a, T> {
    fn fmt(&self, fmt: &mut core::fmt::Formatter<'_>) -> Result<(), core::fmt::Error> {
        fmt.debug_list().entries(&**self).finish()
    }
}

unsafe impl<'a, T> Send for DynArray<'a, T> where T: Send {}
unsafe impl<'a, T> Sync for DynArray<'a, T> where T: Sync {}

impl<'a, T> DynArray<'a, T> {
    #[inline]
    fn get_data(self) -> &'a mut [T] {
        let len = self.len;
        let data = self.ptr.as_ptr();
        core::mem::forget(self);
        unsafe { core::slice::from_raw_parts_mut(data, len) }
    }
}

#[cfg(feature = "nightly")]
unsafe impl<#[may_dangle] 'a, #[may_dangle] T> Drop for DynArray<'a, T> {
    #[inline]
    fn drop(&mut self) {
        unsafe {
            core::ptr::drop_in_place(
                core::slice::from_raw_parts_mut(self.ptr.as_ptr(), self.len) as *mut [T]
            )
        };
    }
}

#[cfg(not(feature = "nightly"))]
impl<'a, T> Drop for DynArray<'a, T> {
    #[inline]
    fn drop(&mut self) {
        unsafe {
            core::ptr::drop_in_place(
                core::slice::from_raw_parts_mut(self.ptr.as_ptr(), self.len) as *mut [T]
            )
        };
    }
}

impl<'a, T> core::ops::Deref for DynArray<'a, T> {
    type Target = [T];

    #[inline]
    fn deref(&self) -> &'_ Self::Target {
        unsafe { core::slice::from_raw_parts(self.ptr.as_ptr(), self.len) }
    }
}

impl<'a, T> core::ops::DerefMut for DynArray<'a, T> {
    #[inline]
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe { core::slice::from_raw_parts_mut(self.ptr.as_ptr(), self.len) }
    }
}

impl<'a, T> AsRef<[T]> for DynArray<'a, T> {
    #[inline]
    fn as_ref(&self) -> &'_ [T] {
        unsafe { core::slice::from_raw_parts(self.ptr.as_ptr(), self.len) }
    }
}

impl<'a, T> AsMut<[T]> for DynArray<'a, T> {
    #[inline]
    fn as_mut(&mut self) -> &'_ mut [T] {
        unsafe { core::slice::from_raw_parts_mut(self.ptr.as_ptr(), self.len) }
    }
}

#[inline]
unsafe fn transmute_slice<T>(slice: &mut [MaybeUninit<u8>], size: usize) -> &mut [T] {
    core::slice::from_raw_parts_mut(slice.as_mut_ptr() as *mut T, size)
}
#[inline]
unsafe fn transmute_pod_slice<T: Pod>(slice: &mut [u8], size: usize) -> &mut [T] {
    core::slice::from_raw_parts_mut(slice.as_mut_ptr() as *mut T, size)
}

struct DropGuard<T> {
    ptr: *mut T,
    len: usize,
}

impl<T> Drop for DropGuard<T> {
    #[inline]
    fn drop(&mut self) {
        unsafe {
            core::ptr::drop_in_place(core::slice::from_raw_parts_mut(self.ptr, self.len) as *mut [T])
        };
    }
}

#[inline]
fn init_array_with<T, F: FnMut(usize) -> T>(mut f: F, array: &mut [MaybeUninit<T>]) -> &mut [T] {
    let len = array.len();
    let ptr = array.as_mut_ptr() as *mut T;

    let mut guard = DropGuard { ptr, len: 0 };

    for i in 0..len {
        guard.len = i;
        unsafe { ptr.add(i).write(f(i)) };
    }
    core::mem::forget(guard);

    unsafe { core::slice::from_raw_parts_mut(ptr, len) }
}

#[inline]
fn init_pod_array_with<T: Pod, F: FnMut(usize) -> T>(mut f: F, array: &mut [T]) -> &mut [T] {
    for (i, x) in array.iter_mut().enumerate() {
        *x = f(i);
    }
    array
}

#[inline]
unsafe fn init_array_with_iter<T, I: Iterator<Item = T>>(
    iter: I,
    ptr: &mut [MaybeUninit<T>],
) -> usize {
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

impl<'a, 'b> ReborrowMut<'b> for DynStack<'a>
where
    'a: 'b,
{
    type Target = DynStack<'b>;

    #[inline]
    fn rb_mut(&'b mut self) -> Self::Target {
        DynStack {
            buffer: self.buffer,
        }
    }
}

impl<'a, 'b> ReborrowMut<'b> for PodStack<'a>
where
    'a: 'b,
{
    type Target = PodStack<'b>;

    #[inline]
    fn rb_mut(&'b mut self) -> Self::Target {
        PodStack {
            buffer: self.buffer,
        }
    }
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
        align,
        align_offset,
        len,
    );
}

#[track_caller]
#[inline]
fn check_enough_space_for_array(
    remaining_len: usize,
    sizeof_val: usize,
    array_len: usize,
    type_name: &'static str,
) {
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

impl<'a> DynStack<'a> {
    /// Returns a new [`DynStack`] from the provided memory buffer.
    #[inline]
    pub fn new(buffer: &'a mut [MaybeUninit<u8>]) -> Self {
        Self { buffer }
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
        unsafe {
            (
                core::slice::from_raw_parts_mut(begin, begin_len),
                core::slice::from_raw_parts_mut(mid, mid_len),
            )
        }
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
    pub fn make_aligned_uninit<T>(
        self,
        size: usize,
        align: usize,
    ) -> (DynArray<'a, MaybeUninit<T>>, Self) {
        let (taken, remaining) = Self::split_buffer(
            self.buffer,
            size,
            align,
            core::mem::size_of::<T>(),
            core::mem::align_of::<T>(),
            core::any::type_name::<T>(),
        );

        let (len, ptr) = {
            let taken = unsafe { transmute_slice::<MaybeUninit<T>>(taken, size) };
            (taken.len(), taken.as_mut_ptr())
        };
        (
            DynArray {
                ptr: unsafe { NonNull::<MaybeUninit<T>>::new_unchecked(ptr) },
                len,
                _marker: (PhantomData, PhantomData),
            },
            DynStack::new(remaining),
        )
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
    pub fn make_aligned_with<T, F: FnMut(usize) -> T>(
        self,
        size: usize,
        align: usize,
        f: F,
    ) -> (DynArray<'a, T>, Self) {
        let (taken, remaining) = self.make_aligned_uninit(size, align);
        let (len, ptr) = {
            let taken = init_array_with(f, taken.get_data());
            (taken.len(), taken.as_mut_ptr())
        };
        (
            DynArray {
                ptr: unsafe { NonNull::<T>::new_unchecked(ptr) },
                len,
                _marker: (PhantomData, PhantomData),
            },
            remaining,
        )
    }

    /// Returns a new uninitialized [`DynArray`] and a stack over the remainder of the buffer.
    ///
    /// # Panics
    ///
    /// Panics if the stack isn't large enough to allocate the array.
    #[track_caller]
    #[inline]
    #[must_use]
    pub fn make_uninit<T>(self, size: usize) -> (DynArray<'a, MaybeUninit<T>>, Self) {
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
    pub fn make_with<T, F: FnMut(usize) -> T>(self, size: usize, f: F) -> (DynArray<'a, T>, Self) {
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
    pub fn collect_aligned<I: IntoIterator>(
        self,
        align: usize,
        iter: I,
    ) -> (DynArray<'a, I::Item>, Self) {
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
    pub fn collect<I: IntoIterator>(self, iter: I) -> (DynArray<'a, I::Item>, Self) {
        self.collect_aligned_impl(core::mem::align_of::<I::Item>(), iter.into_iter())
    }

    #[track_caller]
    #[inline]
    fn collect_aligned_impl<I: Iterator>(
        self,
        align: usize,
        iter: I,
    ) -> (DynArray<'a, I::Item>, Self) {
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
                core::slice::from_raw_parts_mut(
                    buffer_ptr as *mut MaybeUninit<I::Item>,
                    if sizeof_val == 0 {
                        usize::MAX
                    } else {
                        buffer_len / sizeof_val
                    },
                ),
            );

            let remaining_slice = core::slice::from_raw_parts_mut(
                buffer_ptr.add(len * sizeof_val),
                buffer.len() - len * sizeof_val,
            );
            (
                DynArray {
                    ptr: NonNull::new_unchecked(buffer_ptr as *mut I::Item),
                    len,
                    _marker: (PhantomData, PhantomData),
                },
                Self {
                    buffer: remaining_slice,
                },
            )
        }
    }
}

impl<'a> PodStack<'a> {
    /// Returns a new [`PodStack`] from the provided memory buffer.
    #[inline]
    pub fn new(buffer: &'a mut [u8]) -> Self {
        Self { buffer }
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
        unsafe {
            (
                core::slice::from_raw_parts_mut(begin, begin_len),
                core::slice::from_raw_parts_mut(mid, mid_len),
            )
        }
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
    pub fn make_aligned_raw<T: Pod>(self, size: usize, align: usize) -> (&'a mut [T], Self) {
        let (taken, remaining) = Self::split_buffer(
            self.buffer,
            size,
            align,
            core::mem::size_of::<T>(),
            core::mem::align_of::<T>(),
            core::any::type_name::<T>(),
        );

        let taken = unsafe { transmute_pod_slice::<T>(taken, size) };
        (taken, Self::new(remaining))
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
    pub fn make_aligned_with<T: Pod, F: FnMut(usize) -> T>(
        self,
        size: usize,
        align: usize,
        f: F,
    ) -> (&'a mut [T], Self) {
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
    pub fn make_raw<T: Pod>(self, size: usize) -> (&'a mut [T], Self) {
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
    pub fn make_with<T: Pod, F: FnMut(usize) -> T>(self, size: usize, f: F) -> (&'a mut [T], Self) {
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
    pub fn collect_aligned<I: IntoIterator>(
        self,
        align: usize,
        iter: I,
    ) -> (&'a mut [I::Item], Self)
    where
        I::Item: Pod,
    {
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
    pub fn collect<I: IntoIterator>(self, iter: I) -> (&'a mut [I::Item], Self)
    where
        I::Item: Pod,
    {
        self.collect_aligned_impl(core::mem::align_of::<I::Item>(), iter.into_iter())
    }

    #[track_caller]
    #[inline]
    fn collect_aligned_impl<I: Iterator>(self, align: usize, iter: I) -> (&'a mut [I::Item], Self)
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
                core::slice::from_raw_parts_mut(
                    buffer_ptr as *mut I::Item,
                    if sizeof_val == 0 {
                        usize::MAX
                    } else {
                        buffer_len / sizeof_val
                    },
                ),
            );

            let taken = core::slice::from_raw_parts_mut(buffer_ptr as *mut I::Item, len);
            let remaining_slice = core::slice::from_raw_parts_mut(
                buffer_ptr.add(len * sizeof_val),
                buffer_len - len * sizeof_val,
            );
            (
                taken,
                Self {
                    buffer: remaining_slice,
                },
            )
        }
    }
}

#[cfg(all(feature = "nightly", test))]
mod tests_nightly {
    use super::*;

    use alloc::alloc::Global;

    #[test]
    fn empty() {
        let mut buf = MemBuffer::new(Global, StackReq::new::<i32>(0));
        let stack = DynStack::new(&mut buf);
        let (_arr0, _stack) = stack.make_with::<i32, _>(0, |i| i as i32);
    }

    #[test]
    #[should_panic]
    fn empty_overflow() {
        let mut buf = MemBuffer::new(Global, StackReq::new::<i32>(0));
        let stack = DynStack::new(&mut buf);
        let (_arr0, _stack) = stack.make_with::<i32, _>(1, |i| i as i32);
    }

    #[test]
    fn empty_collect() {
        let mut buf = MemBuffer::new(Global, StackReq::new::<i32>(0));
        let stack = DynStack::new(&mut buf);
        let (_arr0, _stack) = stack.collect(0..0);
    }

    #[test]
    fn empty_collect_overflow() {
        let mut buf = MemBuffer::new(Global, StackReq::new::<i32>(0));
        let stack = DynStack::new(&mut buf);
        let (arr0, _stack) = stack.collect(0..1);
        assert!(arr0.is_empty());
    }

    #[test]
    #[should_panic]
    fn overflow() {
        let mut buf = MemBuffer::new(Global, StackReq::new::<i32>(1));
        let stack = DynStack::new(&mut buf);
        let (_arr0, _stack) = stack.make_with::<i32, _>(2, |i| i as i32);
    }

    #[test]
    fn collect_overflow() {
        let mut buf = MemBuffer::new(Global, StackReq::new::<i32>(1));
        let stack = DynStack::new(&mut buf);
        let (arr0, _stack) = stack.collect(1..3);
        assert_eq!(arr0.len(), 1);
        assert_eq!(arr0[0], 1)
    }
}

#[cfg(test)]
mod dyn_stack_tests {
    use super::*;

    #[test]
    fn empty() {
        let mut buf = GlobalMemBuffer::new(StackReq::new::<i32>(0));
        let stack = DynStack::new(&mut buf);
        let (_arr0, _stack) = stack.make_with::<i32, _>(0, |i| i as i32);
    }

    #[test]
    #[should_panic]
    fn empty_overflow() {
        let mut buf = GlobalMemBuffer::new(StackReq::new::<i32>(0));
        let stack = DynStack::new(&mut buf);
        let (_arr0, _stack) = stack.make_with::<i32, _>(1, |i| i as i32);
    }

    #[test]
    fn empty_collect() {
        let mut buf = GlobalMemBuffer::new(StackReq::new::<i32>(0));
        let stack = DynStack::new(&mut buf);
        let (_arr0, _stack) = stack.collect(0..0);
    }

    #[test]
    fn empty_collect_overflow() {
        let mut buf = GlobalMemBuffer::new(StackReq::new::<i32>(0));
        let stack = DynStack::new(&mut buf);
        let (arr0, _stack) = stack.collect(0..1);
        assert!(arr0.is_empty());
    }

    #[test]
    #[should_panic]
    fn overflow() {
        let mut buf = GlobalMemBuffer::new(StackReq::new::<i32>(1));
        let stack = DynStack::new(&mut buf);
        let (_arr0, _stack) = stack.make_with::<i32, _>(2, |i| i as i32);
    }

    #[test]
    fn collect_overflow() {
        let mut buf = GlobalMemBuffer::new(StackReq::new::<i32>(1));
        let stack = DynStack::new(&mut buf);
        let (arr0, _stack) = stack.collect(1..3);
        assert_eq!(arr0.len(), 1);
        assert_eq!(arr0[0], 1)
    }

    #[test]
    fn basic_nested() {
        let mut buf = GlobalMemBuffer::new(StackReq::new::<i32>(6));

        let stack = DynStack::new(&mut buf);
        assert!(stack.can_hold(StackReq::new::<i32>(6)));
        assert!(!stack.can_hold(StackReq::new::<i32>(7)));

        let (arr0, stack) = stack.make_with::<i32, _>(3, |i| i as i32);
        assert_eq!(arr0[0], 0);
        assert_eq!(arr0[1], 1);
        assert_eq!(arr0[2], 2);

        let (arr1, _) = stack.make_with::<i32, _>(3, |i| i as i32 + 3);

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
        let mut buf = GlobalMemBuffer::new(StackReq::new::<i32>(3));

        let mut stack = DynStack::new(&mut buf);

        {
            let (arr0, _) = stack.rb_mut().make_with::<i32, _>(3, |i| i as i32);
            assert_eq!(arr0[0], 0);
            assert_eq!(arr0[1], 1);
            assert_eq!(arr0[2], 2);
        }
        {
            let (arr1, _) = stack.rb_mut().make_with::<i32, _>(3, |i| i as i32 + 3);

            assert_eq!(arr1[0], 3);
            assert_eq!(arr1[1], 4);
            assert_eq!(arr1[2], 5);
        }
    }

    #[test]
    fn basic_nested_collect() {
        let mut buf = GlobalMemBuffer::new(StackReq::new::<i32>(6));
        let stack = DynStack::new(&mut buf);

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
        let mut buf = GlobalMemBuffer::new(StackReq::new::<i32>(3));

        let mut stack = DynStack::new(&mut buf);

        {
            let (arr0, _) = stack.rb_mut().collect(0..3_i32);
            assert_eq!(arr0[0], 0);
            assert_eq!(arr0[1], 1);
            assert_eq!(arr0[2], 2);
        }
        {
            let (arr1, _) = stack.rb_mut().collect(3..6_i32);

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

        let mut buf = GlobalMemBuffer::new(StackReq::new::<CountedDrop>(6));
        let stack = DynStack::new(&mut buf);

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

        let mut buf = GlobalMemBuffer::new(StackReq::new::<CountedDrop>(6));
        let mut stack = DynStack::new(&mut buf);

        {
            let _ = stack.rb_mut().make_with(3, |_| CountedDrop);
            assert_eq!(DROP_COUNT.load(Ordering::SeqCst), 3);
        }

        {
            let _ = stack.rb_mut().make_with(4, |_| CountedDrop);
            assert_eq!(DROP_COUNT.load(Ordering::SeqCst), 7);
        }
    }
}

#[cfg(test)]
mod pod_stack_tests {
    use super::*;

    #[test]
    fn empty() {
        let mut buf = GlobalPodBuffer::new(StackReq::new::<i32>(0));
        let stack = PodStack::new(&mut buf);
        let (_arr0, _stack) = stack.make_with::<i32, _>(0, |i| i as i32);
    }

    #[test]
    #[should_panic]
    fn empty_overflow() {
        let mut buf = GlobalPodBuffer::new(StackReq::new::<i32>(0));
        let stack = PodStack::new(&mut buf);
        let (_arr0, _stack) = stack.make_with::<i32, _>(1, |i| i as i32);
    }

    #[test]
    fn empty_collect() {
        let mut buf = GlobalPodBuffer::new(StackReq::new::<i32>(0));
        let stack = PodStack::new(&mut buf);
        let (_arr0, _stack) = stack.collect(0..0);
    }

    #[test]
    fn empty_collect_overflow() {
        let mut buf = GlobalPodBuffer::new(StackReq::new::<i32>(0));
        let stack = PodStack::new(&mut buf);
        let (arr0, _stack) = stack.collect(0..1);
        assert!(arr0.is_empty());
    }

    #[test]
    #[should_panic]
    fn overflow() {
        let mut buf = GlobalPodBuffer::new(StackReq::new::<i32>(1));
        let stack = PodStack::new(&mut buf);
        let (_arr0, _stack) = stack.make_with::<i32, _>(2, |i| i as i32);
    }

    #[test]
    fn collect_overflow() {
        let mut buf = GlobalPodBuffer::new(StackReq::new::<i32>(1));
        let stack = PodStack::new(&mut buf);
        let (arr0, _stack) = stack.collect(1..3);
        assert_eq!(arr0.len(), 1);
        assert_eq!(arr0[0], 1)
    }

    #[test]
    fn basic_nested() {
        let mut buf = GlobalPodBuffer::new(StackReq::new::<i32>(6));

        let stack = PodStack::new(&mut buf);
        assert!(stack.can_hold(StackReq::new::<i32>(6)));
        assert!(!stack.can_hold(StackReq::new::<i32>(7)));

        let (arr0, stack) = stack.make_with::<i32, _>(3, |i| i as i32);
        assert_eq!(arr0[0], 0);
        assert_eq!(arr0[1], 1);
        assert_eq!(arr0[2], 2);

        let (arr1, _) = stack.make_with::<i32, _>(3, |i| i as i32 + 3);

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
        let mut buf = GlobalPodBuffer::new(StackReq::new::<i32>(3));

        let mut stack = PodStack::new(&mut buf);

        {
            let (arr0, _) = stack.rb_mut().make_with::<i32, _>(3, |i| i as i32);
            assert_eq!(arr0[0], 0);
            assert_eq!(arr0[1], 1);
            assert_eq!(arr0[2], 2);
        }
        {
            let (arr1, _) = stack.rb_mut().make_with::<i32, _>(3, |i| i as i32 + 3);

            assert_eq!(arr1[0], 3);
            assert_eq!(arr1[1], 4);
            assert_eq!(arr1[2], 5);
        }
    }

    #[test]
    fn basic_nested_collect() {
        let mut buf = GlobalPodBuffer::new(StackReq::new::<i32>(6));
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
        let mut buf = GlobalPodBuffer::new(StackReq::new::<i32>(3));

        let mut stack = PodStack::new(&mut buf);

        {
            let (arr0, _) = stack.rb_mut().collect(0..3_i32);
            assert_eq!(arr0[0], 0);
            assert_eq!(arr0[1], 1);
            assert_eq!(arr0[2], 2);
        }
        {
            let (arr1, _) = stack.rb_mut().collect(3..6_i32);

            assert_eq!(arr1[0], 3);
            assert_eq!(arr1[1], 4);
            assert_eq!(arr1[2], 5);
        }
    }

    #[test]
    fn make_raw() {
        let mut buf = GlobalPodBuffer::new(StackReq::new::<i32>(3));

        let mut stack = PodStack::new(&mut buf);

        {
            let (arr0, _) = stack.rb_mut().make_raw::<i32>(3);
            assert_eq!(arr0[0], 0);
            assert_eq!(arr0[1], 0);
            assert_eq!(arr0[2], 0);
        }
        {
            let (arr0, _) = stack.rb_mut().collect(0..3_i32);
            assert_eq!(arr0[0], 0);
            assert_eq!(arr0[1], 1);
            assert_eq!(arr0[2], 2);
        }
        {
            let (arr1, _) = stack.rb_mut().make_raw::<i32>(3);

            assert_eq!(arr1[0], 0);
            assert_eq!(arr1[1], 1);
            assert_eq!(arr1[2], 2);
        }
    }
}
