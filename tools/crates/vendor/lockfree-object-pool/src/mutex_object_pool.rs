use crate::{mutex_owned_reusable::MutexOwnedReusable, mutex_reusable::MutexReusable};
use std::mem::ManuallyDrop;
use std::sync::{Arc, Mutex};

/// ObjectPool use a [`std::sync::Mutex`] over vector to secure multithread access to pull.
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
pub struct MutexObjectPool<T> {
    objects: Mutex<Vec<T>>,
    reset: Box<dyn Fn(&mut T) + Send + Sync>,
    init: Box<dyn Fn() -> T + Send + Sync>,
}

impl<T> MutexObjectPool<T> {
    ///
    /// Create an new [`MutexObjectPool`]
    ///
    /// # Arguments
    /// * `init`  closure to create new item
    /// * `reset` closure to reset item before reusage
    ///
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
    /// ```
    #[inline]
    pub fn new<R, I>(init: I, reset: R) -> Self
    where
        R: Fn(&mut T) + Send + Sync + 'static,
        I: Fn() -> T + Send + Sync + 'static,
    {
        Self {
            objects: Mutex::new(Vec::new()),
            reset: Box::new(reset),
            init: Box::new(init),
        }
    }

    ///
    /// Create a new element. When the element is dropped, it returns in the pull.
    ///
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
    /// ```
    #[inline]
    pub fn pull(&self) -> MutexReusable<T> {
        MutexReusable::new(
            self,
            ManuallyDrop::new(
                self.objects
                    .lock()
                    .unwrap()
                    .pop()
                    .unwrap_or_else(&self.init),
            ),
        )
    }

    ///
    /// Create a new element. When the element is dropped, it returns in the pull.
    ///
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
    /// ```
    #[inline]
    pub fn pull_owned(self: &Arc<Self>) -> MutexOwnedReusable<T> {
        MutexOwnedReusable::new(
            self.clone(),
            ManuallyDrop::new(
                self.objects
                    .lock()
                    .unwrap()
                    .pop()
                    .unwrap_or_else(&self.init),
            ),
        )
    }

    #[inline]
    pub(crate) fn attach(&self, mut data: T) {
        (self.reset)(&mut data);
        self.objects.lock().unwrap().push(data);
    }
}
