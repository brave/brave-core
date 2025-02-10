use crate::spin_lock_object_pool::SpinLockObjectPool;
use std::mem::ManuallyDrop;
use std::ops::{Deref, DerefMut};

/// Wrapper over T used by [`SpinLockObjectPool`].
///
/// Access is allowed with [`std::ops::Deref`] or [`std::ops::DerefMut`]
/// # Example
/// ```rust
///  use lockfree_object_pool::SpinLockObjectPool;
///
///  let pool = SpinLockObjectPool::<u32>::new(
///    ||  Default::default(),
///    |v| {
///      *v = 0;
///    }
///  );
///  let mut item = pool.pull();
///
///  *item = 5;
///  let work = *item * 5;
/// ```
pub struct SpinLockReusable<'a, T> {
    pool: &'a SpinLockObjectPool<T>,
    data: ManuallyDrop<T>,
}

impl<'a, T> SpinLockReusable<'a, T> {
    /// Create new element
    ///
    /// # Arguments
    /// * `pool` object pool owner
    /// * `data` element to wrappe
    #[inline]
    pub fn new(pool: &'a SpinLockObjectPool<T>, data: ManuallyDrop<T>) -> Self {
        Self { pool, data }
    }
}

impl<'a, T> DerefMut for SpinLockReusable<'a, T> {
    #[inline]
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.data
    }
}

impl<'a, T> Deref for SpinLockReusable<'a, T> {
    type Target = T;

    #[inline]
    fn deref(&self) -> &Self::Target {
        &self.data
    }
}

impl<'a, T> Drop for SpinLockReusable<'a, T> {
    #[inline]
    fn drop(&mut self) {
        let data = unsafe {
            // SAFETY: self.data is never referenced again and it isn't dropped
            ManuallyDrop::take(&mut self.data)
        };
        self.pool.attach(data);
    }
}
