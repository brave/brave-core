use crate::none_reusable::NoneReusable;
use std::sync::Arc;

/// Basic allocation without pull. Used to compare default rust allocation with different kind of object pool.
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
pub struct NoneObjectPool<T> {
    init: Box<dyn Fn() -> T + Send + Sync>,
}

impl<T> NoneObjectPool<T> {
    ///
    /// Create an new [`NoneObjectPool`]
    ///
    /// # Arguments
    /// * `init` closure to create new item
    ///
    /// # Example
    /// ```rust
    ///  use lockfree_object_pool::NoneObjectPool;
    ///
    ///  let pool = NoneObjectPool::<u32>::new(|| Default::default());
    /// ```
    #[inline]
    pub fn new<I>(init: I) -> Self
    where
        I: Fn() -> T + Send + Sync + 'static,
    {
        Self {
            init: Box::new(init),
        }
    }

    ///
    /// Create a new element. When the element is dropped, it doesn't return in the pull.
    ///
    /// # Example
    /// ```rust
    ///  use lockfree_object_pool::NoneObjectPool;
    ///
    ///  let pool = NoneObjectPool::<u32>::new(|| Default::default());
    ///  let mut item = pool.pull();
    /// ```
    #[inline]
    pub fn pull(&self) -> NoneReusable<T> {
        NoneReusable::new((self.init)())
    }

    ///
    /// Create a new element. When the element is dropped, it doesn't return in the pull.
    ///
    /// # Example
    /// ```rust
    ///  use std::sync::Arc;
    ///  use lockfree_object_pool::NoneObjectPool;
    ///
    ///  let pool = Arc::new(NoneObjectPool::<u32>::new(|| Default::default()));
    ///  let mut item = pool.pull_owned();
    /// ```
    #[inline]
    pub fn pull_owned(self: &Arc<Self>) -> NoneReusable<T> {
        NoneReusable::new((self.init)())
    }
}
