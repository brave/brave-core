use crate::mutex_object_pool::MutexObjectPool;
use std::mem::ManuallyDrop;
use std::ops::{Deref, DerefMut};
use std::sync::Arc;

/// Wrapper over T used by [`MutexObjectPool`].
///
/// Access is allowed with [`std::ops::Deref`] or [`std::ops::DerefMut`]
/// # Example
/// ```rust
///  use lockfree_object_pool::MutexObjectPool;
///  use std::sync::Arc;
///
///  let pool = Arc::new(MutexObjectPool::<u32>::new(
///    ||  Default::default(),
///    |v| {
///      *v = 0;
///    }
///  ));
///  let mut item = pool.pull_owned();
///
///  *item = 5;
///  let work = *item * 5;
/// ```
pub struct MutexOwnedReusable<T> {
    pool: Arc<MutexObjectPool<T>>,
    data: ManuallyDrop<T>,
}

impl<T> MutexOwnedReusable<T> {
    /// Create new element
    ///
    /// # Arguments
    /// * `pool` object pool owner
    /// * `data` element to wrap
    #[inline]
    pub fn new(pool: Arc<MutexObjectPool<T>>, data: ManuallyDrop<T>) -> Self {
        Self { pool, data }
    }
}

impl<T> DerefMut for MutexOwnedReusable<T> {
    #[inline]
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.data
    }
}

impl<T> Deref for MutexOwnedReusable<T> {
    type Target = T;

    #[inline]
    fn deref(&self) -> &Self::Target {
        &self.data
    }
}

impl<T> Drop for MutexOwnedReusable<T> {
    #[inline]
    fn drop(&mut self) {
        let data = unsafe {
            // SAFETY: self.data is never referenced again and it isn't dropped
            ManuallyDrop::take(&mut self.data)
        };
        self.pool.attach(data);
    }
}
