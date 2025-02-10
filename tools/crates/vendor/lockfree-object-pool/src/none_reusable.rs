use std::ops::{Deref, DerefMut};

#[allow(unused_imports)]
use crate::none_object_pool::NoneObjectPool;

/// Wrapper over T used by [`NoneObjectPool`].
///
/// Access is allowed with [`std::ops::Deref`] or [`std::ops::DerefMut`]
/// # Example
/// ```rust
///  use lockfree_object_pool::NoneObjectPool;
///
///  let pool = NoneObjectPool::<u32>::new(|| Default::default());
///  let mut item = pool.pull();
///
///  *item = 5;
///  let work = *item * 5;
/// ```
pub struct NoneReusable<T> {
    data: T,
}

impl<T> NoneReusable<T> {
    /// Create new element
    ///
    /// # Arguments
    /// * `data` element to wrappe
    #[inline]
    pub fn new(data: T) -> Self {
        Self { data }
    }
}

impl<T> DerefMut for NoneReusable<T> {
    #[inline]
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.data
    }
}

impl<T> Deref for NoneReusable<T> {
    type Target = T;

    #[inline]
    fn deref(&self) -> &Self::Target {
        &self.data
    }
}
