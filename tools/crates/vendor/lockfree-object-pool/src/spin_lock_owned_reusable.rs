use crate::spin_lock_object_pool::SpinLockObjectPool;
use std::mem::ManuallyDrop;
use std::ops::{Deref, DerefMut};
use std::sync::Arc;

/// Wrapper over T used by [`SpinLockObjectPool`].
///
/// Access is allowed with [`std::ops::Deref`] or [`std::ops::DerefMut`]
/// # Example
/// ```rust
///  use lockfree_object_pool::SpinLockObjectPool;
///  use std::sync::Arc;
///
///  let pool = Arc::new(SpinLockObjectPool::<u32>::new(
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
pub struct SpinLockOwnedReusable<T> {
    pool: Arc<SpinLockObjectPool<T>>,
    data: ManuallyDrop<T>,
}

impl<T> SpinLockOwnedReusable<T> {
    /// Create new element
    ///
    /// # Arguments
    /// * `pool` object pool owner
    /// * `data` element to wrappe
    #[inline]
    pub fn new(pool: Arc<SpinLockObjectPool<T>>, data: ManuallyDrop<T>) -> Self {
        Self { pool, data }
    }
}

impl<T> DerefMut for SpinLockOwnedReusable<T> {
    #[inline]
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.data
    }
}

impl<T> Deref for SpinLockOwnedReusable<T> {
    type Target = T;

    #[inline]
    fn deref(&self) -> &Self::Target {
        &self.data
    }
}

impl<T> Drop for SpinLockOwnedReusable<T> {
    #[inline]
    fn drop(&mut self) {
        let data = unsafe {
            // SAFETY: self.data is never referenced again and it isn't dropped
            ManuallyDrop::take(&mut self.data)
        };
        self.pool.attach(data);
    }
}
