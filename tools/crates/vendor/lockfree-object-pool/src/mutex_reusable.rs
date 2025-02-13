use crate::mutex_object_pool::MutexObjectPool;
use std::mem::ManuallyDrop;
use std::ops::{Deref, DerefMut};

/// Wrapper over T used by [`MutexObjectPool`].
///
/// Access is allowed with [`std::ops::Deref`] or [`std::ops::DerefMut`]
/// # Example
/// ```rust
///  use lockfree_object_pool::MutexObjectPool;
///
///  let pool = MutexObjectPool::<u32>::new(
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
pub struct MutexReusable<'a, T> {
    pool: &'a MutexObjectPool<T>,
    data: ManuallyDrop<T>,
}

impl<'a, T> MutexReusable<'a, T> {
    /// Create new element
    ///
    /// # Arguments
    /// * `pool` object pool owner
    /// * `data` element to wrap
    #[inline]
    pub fn new(pool: &'a MutexObjectPool<T>, data: ManuallyDrop<T>) -> Self {
        Self { pool, data }
    }
}

impl<'a, T> DerefMut for MutexReusable<'a, T> {
    #[inline]
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.data
    }
}

impl<'a, T> Deref for MutexReusable<'a, T> {
    type Target = T;

    #[inline]
    fn deref(&self) -> &Self::Target {
        &self.data
    }
}

impl<'a, T> Drop for MutexReusable<'a, T> {
    #[inline]
    fn drop(&mut self) {
        let data = unsafe {
            // SAFETY: self.data is never referenced again and it isn't dropped
            ManuallyDrop::take(&mut self.data)
        };
        self.pool.attach(data);
    }
}
