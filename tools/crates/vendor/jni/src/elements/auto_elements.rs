use log::error;
use std::marker::PhantomData;
use std::ptr::NonNull;

use crate::objects::{JPrimitiveArray, Reference, ReleaseMode, TypeArray};
use crate::sys::jboolean;
use crate::{JavaVM, env::Env, errors::*, sys};

#[cfg(doc)]
use crate::objects::JByteArray;

/// Auto-release wrapper for a mutable pointer to the elements of a [`JPrimitiveArray`]
/// (such as [`JByteArray`])
///
/// This type is used to wrap pointers returned by `Get<Type>ArrayElements`
/// and ensure the pointer is released via `Release<Type>ArrayElements` when dropped.
///
/// The wrapper is tied to the lifetime of the array reference that becomes
/// owned by the struct (the reference needs to be retained in order to call
/// `Release<Type>ArrayElements` later).
#[derive(Debug)]
pub struct AutoElements<'array_local, T, TArrayRef>
where
    T: TypeArray + 'array_local,
    TArrayRef: AsRef<JPrimitiveArray<'array_local, T>>,
{
    array: TArrayRef,
    len: usize,
    ptr: NonNull<T>,
    mode: ReleaseMode,
    is_copy: bool,
    _lifetime: PhantomData<&'array_local ()>,
}

// Note: since we require a Env reference to construct AutoElements, that
// means we can assume JavaVM::singleton() is initialized later when we need to
// release the array (so we don't need to somehow save a Env reference).
impl<'array_local, T, TArrayRef> AutoElements<'array_local, T, TArrayRef>
where
    T: TypeArray + 'array_local,
    TArrayRef: AsRef<JPrimitiveArray<'array_local, T>>,
{
    /// # Safety
    ///
    /// `len` must be the correct length (number of elements) of the given `array`
    unsafe fn new_with_len(
        env: &Env<'_>,
        array: TArrayRef,
        len: usize,
        mode: ReleaseMode,
    ) -> Result<Self> {
        let mut is_copy: jboolean = true;
        let ptr = unsafe { T::get_elements(env, array.as_ref().as_raw(), &mut is_copy) }?;
        Ok(AutoElements {
            array,
            len,
            ptr: NonNull::new(ptr).ok_or(Error::NullPtr("Non-null ptr expected"))?,
            mode,
            is_copy: is_copy == sys::JNI_TRUE,
            _lifetime: PhantomData,
        })
    }

    pub(crate) fn new(env: &Env<'_>, array: TArrayRef, mode: ReleaseMode) -> Result<Self> {
        let len = array.as_ref().len(env)?;
        unsafe { Self::new_with_len(env, array, len, mode) }
    }

    /// Get a reference to the wrapped pointer
    pub fn as_ptr(&self) -> *mut T {
        self.ptr.as_ptr()
    }

    /// Commits the changes to the array, if it is a copy
    pub fn commit(&mut self) -> Result<()> {
        unsafe { self.release_array_elements(sys::JNI_COMMIT) }
    }

    /// Calls the release function.
    ///
    /// # Safety
    ///
    /// `mode` must be a valid parameter to the JNI `Release<PrimitiveType>ArrayElements`' `mode`
    /// parameter.
    ///
    /// If `mode` is not [`sys::JNI_COMMIT`], then `self.ptr` must not have already been released.
    unsafe fn release_array_elements(&self, mode: i32) -> Result<()> {
        // Errors:
        // - Since we can't construct `AutoElements` without a valid `Env` reference we know
        //   we can call `JavaVM::singleton()` without an error.
        // - Since `self` is associated with a local reference frame lifetime we know that the
        //   thread is attached and so `with_top_local_frame()` can't return an error.
        JavaVM::singleton()?.with_top_local_frame(|env| unsafe {
            T::release_elements(env, self.array.as_ref().as_raw(), self.ptr, mode)
        })
    }

    /// Don't copy back the changes to the array on release (if it is a copy).
    ///
    /// This has no effect if the array is not a copy.
    ///
    /// This method is useful to change the release mode of an array originally created
    /// with `ReleaseMode::CopyBack`.
    pub fn discard(mut self) {
        self.mode = ReleaseMode::NoCopyBack;
        // drop
    }

    /// Indicates if the array is a copy or not
    pub fn is_copy(&self) -> bool {
        self.is_copy
    }

    /// Returns the array length (number of elements)
    pub fn len(&self) -> usize {
        self.len
    }

    /// Returns true if the vector contains no elements.
    pub fn is_empty(&self) -> bool {
        self.len == 0
    }
}

impl<'array_local, T, TArrayRef> AsRef<AutoElements<'array_local, T, TArrayRef>>
    for AutoElements<'array_local, T, TArrayRef>
where
    T: TypeArray + 'array_local,
    TArrayRef: AsRef<JPrimitiveArray<'array_local, T>> + Reference,
{
    fn as_ref(&self) -> &AutoElements<'array_local, T, TArrayRef> {
        self
    }
}

impl<'array_local, T, TArrayRef> Drop for AutoElements<'array_local, T, TArrayRef>
where
    T: TypeArray,
    TArrayRef: AsRef<JPrimitiveArray<'array_local, T>>,
{
    fn drop(&mut self) {
        // Safety: `self.mode` is valid and the array has not yet been released.
        let res = unsafe { self.release_array_elements(self.mode as i32) };

        match res {
            Ok(()) => {}
            Err(e) => error!("error releasing array: {:#?}", e),
        }
    }
}

impl<'array_local, T, TArrayRef> From<&AutoElements<'array_local, T, TArrayRef>> for *mut T
where
    T: TypeArray,
    TArrayRef: AsRef<JPrimitiveArray<'array_local, T>> + Reference,
{
    fn from(other: &AutoElements<'array_local, T, TArrayRef>) -> *mut T {
        other.as_ptr()
    }
}

impl<'array_local, T, TArrayRef> std::ops::Deref for AutoElements<'array_local, T, TArrayRef>
where
    T: TypeArray,
    TArrayRef: AsRef<JPrimitiveArray<'array_local, T>> + Reference,
{
    type Target = [T];

    fn deref(&self) -> &Self::Target {
        unsafe { std::slice::from_raw_parts(self.ptr.as_ptr(), self.len) }
    }
}

impl<'array_local, T, TArrayRef> std::ops::DerefMut for AutoElements<'array_local, T, TArrayRef>
where
    T: TypeArray,
    TArrayRef: AsRef<JPrimitiveArray<'array_local, T>> + Reference,
{
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe { std::slice::from_raw_parts_mut(self.ptr.as_mut(), self.len) }
    }
}
