use crate::stack_req::StackReq;
use alloc::alloc::handle_alloc_error;
use alloc::alloc::Layout;
use core::mem::ManuallyDrop;
use core::mem::MaybeUninit;
use core::ptr::NonNull;

/// Buffer of uninitialized bytes to serve as workspace for dynamic arrays.
pub struct GlobalMemBuffer {
    ptr: NonNull<u8>,
    len: usize,
    align: usize,
}

/// Buffer of initialized bytes to serve as workspace for dynamic POD arrays.
pub struct GlobalPodBuffer {
    inner: GlobalMemBuffer,
}

impl GlobalPodBuffer {
    /// Allocate a memory buffer with sufficient storage for the given stack requirements, using the
    /// global allocator.
    ///
    /// Calls [`alloc::alloc::handle_alloc_error`] in the case of failure.
    ///
    /// # Example
    /// ```
    /// use dyn_stack::{PodStack, StackReq, GlobalPodBuffer};
    ///
    /// let req = StackReq::new::<i32>(3);
    /// let mut buf = GlobalPodBuffer::new(req);
    /// let stack = PodStack::new(&mut buf);
    ///
    /// // use the stack
    /// let (arr, _) = stack.make_with::<i32, _>(3, |i| i as i32);
    /// ```
    pub fn new(req: StackReq) -> Self {
        Self::try_new(req).unwrap_or_else(|_| handle_alloc_error(to_layout(req)))
    }

    /// Allocate a memory buffer with sufficient storage for the given stack requirements, using the
    /// global allocator, or an error if the allocation did not succeed.
    ///
    /// # Example
    /// ```
    /// use dyn_stack::{PodStack, StackReq, GlobalPodBuffer};
    ///
    /// let req = StackReq::try_new::<i32>(3).unwrap();
    /// let mut buf = GlobalPodBuffer::new(req);
    /// let stack = PodStack::new(&mut buf);
    ///
    /// // use the stack
    /// let (arr, _) = stack.make_with::<i32, _>(3, |i| i as i32);
    /// ```
    pub fn try_new(req: StackReq) -> Result<Self, AllocError> {
        unsafe {
            if req.size_bytes() == 0 {
                let ptr = core::ptr::null_mut::<u8>().wrapping_add(req.align_bytes());
                Ok(GlobalPodBuffer {
                    inner: GlobalMemBuffer {
                        ptr: NonNull::<u8>::new_unchecked(ptr),
                        len: 0,
                        align: req.align_bytes(),
                    },
                })
            } else {
                let layout = to_layout(req);
                let ptr = alloc::alloc::alloc_zeroed(layout);
                if ptr.is_null() {
                    return Err(AllocError);
                }
                let len = layout.size();
                let ptr = NonNull::<u8>::new_unchecked(ptr);
                Ok(GlobalPodBuffer {
                    inner: GlobalMemBuffer {
                        ptr,
                        len,
                        align: req.align_bytes(),
                    },
                })
            }
        }
    }

    /// Creates a [`GlobalPodBuffer`]	from its raw components.
    ///
    /// # Safety
    ///
    /// The arguments to this function must have been acquired from a call to
    /// [`GlobalPodBuffer::into_raw_parts`]
    #[inline]
    pub unsafe fn from_raw_parts(ptr: *mut u8, len: usize, align: usize) -> Self {
        GlobalPodBuffer {
            inner: GlobalMemBuffer {
                ptr: NonNull::new_unchecked(ptr),
                len,
                align,
            },
        }
    }

    /// Decomposes a [`GlobalPodBuffer`] into its raw components in this order: ptr, length and
    /// alignment.
    #[inline]
    pub fn into_raw_parts(self) -> (*mut u8, usize, usize) {
        let no_drop = ManuallyDrop::new(self);
        (
            no_drop.inner.ptr.as_ptr(),
            no_drop.inner.len,
            no_drop.inner.align,
        )
    }
}

impl GlobalMemBuffer {
    /// Allocate a memory buffer with sufficient storage for the given stack requirements, using the
    /// global allocator.
    ///
    /// Calls [`alloc::alloc::handle_alloc_error`] in the case of failure.
    ///
    /// # Example
    /// ```
    /// use dyn_stack::{DynStack, StackReq, GlobalMemBuffer};
    ///
    /// let req = StackReq::new::<i32>(3);
    /// let mut buf = GlobalMemBuffer::new(req);
    /// let stack = DynStack::new(&mut buf);
    ///
    /// // use the stack
    /// let (arr, _) = stack.make_with::<i32, _>(3, |i| i as i32);
    /// ```
    pub fn new(req: StackReq) -> Self {
        Self::try_new(req).unwrap_or_else(|_| handle_alloc_error(to_layout(req)))
    }

    /// Allocate a memory buffer with sufficient storage for the given stack requirements, using the
    /// global allocator, or an error if the allocation did not succeed.
    ///
    /// # Example
    /// ```
    /// use dyn_stack::{DynStack, StackReq, GlobalMemBuffer};
    ///
    /// let req = StackReq::try_new::<i32>(3).unwrap();
    /// let mut buf = GlobalMemBuffer::new(req);
    /// let stack = DynStack::new(&mut buf);
    ///
    /// // use the stack
    /// let (arr, _) = stack.make_with::<i32, _>(3, |i| i as i32);
    /// ```
    pub fn try_new(req: StackReq) -> Result<Self, AllocError> {
        unsafe {
            if req.size_bytes() == 0 {
                let ptr = core::ptr::null_mut::<u8>().wrapping_add(req.align_bytes());
                Ok(GlobalMemBuffer {
                    ptr: NonNull::<u8>::new_unchecked(ptr),
                    len: 0,
                    align: req.align_bytes(),
                })
            } else {
                let layout = to_layout(req);
                let ptr = alloc::alloc::alloc(layout);
                if ptr.is_null() {
                    return Err(AllocError);
                }
                let len = layout.size();
                let ptr = NonNull::<u8>::new_unchecked(ptr);
                Ok(GlobalMemBuffer {
                    ptr,
                    len,
                    align: req.align_bytes(),
                })
            }
        }
    }

    /// Creates a `GlobalMemBuffer`	from its raw components.
    ///
    /// # Safety
    ///
    /// The arguments to this function must have been acquired from a call to
    /// [`GlobalMemBuffer::into_raw_parts`]
    #[inline]
    pub unsafe fn from_raw_parts(ptr: *mut u8, len: usize, align: usize) -> Self {
        Self {
            ptr: NonNull::new_unchecked(ptr),
            len,
            align,
        }
    }

    /// Decomposes a `GlobalMemBuffer` into its raw components in this order: ptr, length and
    /// alignment.
    #[inline]
    pub fn into_raw_parts(self) -> (*mut u8, usize, usize) {
        let no_drop = ManuallyDrop::new(self);
        (no_drop.ptr.as_ptr(), no_drop.len, no_drop.align)
    }
}

unsafe impl Sync for GlobalMemBuffer {}
unsafe impl Send for GlobalMemBuffer {}

fn to_layout(req: StackReq) -> Layout {
    unsafe { Layout::from_size_align_unchecked(req.size_bytes(), req.align_bytes()) }
}

impl Drop for GlobalMemBuffer {
    #[inline]
    fn drop(&mut self) {
        unsafe {
            if self.len != 0 {
                alloc::alloc::dealloc(
                    self.ptr.as_ptr(),
                    Layout::from_size_align_unchecked(self.len, self.align),
                );
            }
        }
    }
}

impl core::ops::Deref for GlobalMemBuffer {
    type Target = [MaybeUninit<u8>];

    #[inline]
    fn deref(&self) -> &Self::Target {
        unsafe {
            core::slice::from_raw_parts(self.ptr.as_ptr() as *const MaybeUninit<u8>, self.len)
        }
    }
}

impl core::ops::DerefMut for GlobalMemBuffer {
    #[inline]
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe {
            core::slice::from_raw_parts_mut(self.ptr.as_ptr() as *mut MaybeUninit<u8>, self.len)
        }
    }
}

impl core::ops::Deref for GlobalPodBuffer {
    type Target = [u8];

    #[inline]
    fn deref(&self) -> &Self::Target {
        unsafe { core::slice::from_raw_parts(self.inner.ptr.as_ptr(), self.inner.len) }
    }
}

impl core::ops::DerefMut for GlobalPodBuffer {
    #[inline]
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe { core::slice::from_raw_parts_mut(self.inner.ptr.as_ptr(), self.inner.len) }
    }
}

/// Error during memory allocation.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct AllocError;

impl core::fmt::Display for AllocError {
    fn fmt(&self, fmt: &mut core::fmt::Formatter<'_>) -> Result<(), core::fmt::Error> {
        fmt.write_str("memory allocation failed")
    }
}

#[cfg(feature = "std")]
#[cfg_attr(docsrs, doc(cfg(feature = "std")))]
impl std::error::Error for AllocError {}

#[cfg(feature = "nightly")]
pub use nightly::*;

#[cfg(feature = "nightly")]
mod nightly {
    use super::*;
    use alloc::alloc::{Allocator, Global};

    #[cfg_attr(docsrs, doc(cfg(feature = "nightly")))]
    /// Buffer of uninitialized bytes to serve as workspace for dynamic arrays.
    pub struct MemBuffer<A: Allocator = Global> {
        ptr: NonNull<u8>,
        len: usize,
        align: usize,
        alloc: A,
    }

    unsafe impl<A: Allocator> Sync for MemBuffer<A> {}
    unsafe impl<A: Allocator> Send for MemBuffer<A> {}

    impl<A: Allocator> Drop for MemBuffer<A> {
        #[inline]
        fn drop(&mut self) {
            // SAFETY: this was initialized with std::alloc::alloc
            unsafe {
                self.alloc.deallocate(
                    self.ptr,
                    Layout::from_size_align_unchecked(self.len, self.align),
                )
            }
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
        /// #![feature(allocator_api)]
        ///
        /// use dyn_stack::{DynStack, StackReq, MemBuffer};
        /// use std::alloc::Global;
        ///
        /// let req = StackReq::new::<i32>(3);
        /// let mut buf = MemBuffer::new(Global, req);
        /// let stack = DynStack::new(&mut buf);
        ///
        /// // use the stack
        /// let (arr, _) = stack.make_with::<i32, _>(3, |i| i as i32);
        /// ```
        pub fn new(alloc: A, req: StackReq) -> Self {
            Self::try_new(alloc, req).unwrap_or_else(|_| handle_alloc_error(to_layout(req)))
        }

        /// Allocate a memory buffer with sufficient storage for the given stack requirements, using the
        /// provided allocator, or an `AllocError` in the case of failure.
        ///
        /// # Example
        /// ```
        /// #![feature(allocator_api)]
        ///
        /// use dyn_stack::{DynStack, StackReq, MemBuffer};
        /// use std::alloc::Global;
        ///
        /// let req = StackReq::new::<i32>(3);
        /// let mut buf = MemBuffer::try_new(Global, req).unwrap();
        /// let stack = DynStack::new(&mut buf);
        ///
        /// // use the stack
        /// let (arr, _) = stack.make_with::<i32, _>(3, |i| i as i32);
        /// ```
        pub fn try_new(alloc: A, req: StackReq) -> Result<Self, AllocError> {
            unsafe {
                let ptr = &mut *(alloc
                    .allocate(to_layout(req))
                    .map_err(|_| AllocError)?
                    .as_ptr() as *mut [MaybeUninit<u8>]);
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
    }

    impl<A: Allocator> core::ops::Deref for MemBuffer<A> {
        type Target = [MaybeUninit<u8>];

        #[inline]
        fn deref(&self) -> &Self::Target {
            unsafe {
                core::slice::from_raw_parts(self.ptr.as_ptr() as *const MaybeUninit<u8>, self.len)
            }
        }
    }

    impl<A: Allocator> core::ops::DerefMut for MemBuffer<A> {
        #[inline]
        fn deref_mut(&mut self) -> &mut Self::Target {
            unsafe {
                core::slice::from_raw_parts_mut(self.ptr.as_ptr() as *mut MaybeUninit<u8>, self.len)
            }
        }
    }
}
