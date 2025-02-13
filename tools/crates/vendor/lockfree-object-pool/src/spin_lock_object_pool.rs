use crate::{
    spin_lock::SpinLock, spin_lock_owned_reusable::SpinLockOwnedReusable,
    spin_lock_reusable::SpinLockReusable,
};
use std::mem::ManuallyDrop;
use std::sync::Arc;

/// ObjectPool use a spin lock over vector to secure multithread access to pull.
///
/// The spin lock works like [`std::sync::Mutex`] but
/// * use [`std::sync::atomic::AtomicBool`] for synchro
/// * active waiting
///
/// cf [wikipedia](https://en.wikipedia.org/wiki/Spinlock) for more information.
///
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
pub struct SpinLockObjectPool<T> {
    objects: SpinLock<Vec<T>>,
    reset: Box<dyn Fn(&mut T) + Send + Sync>,
    init: Box<dyn Fn() -> T + Send + Sync>,
}

impl<T> SpinLockObjectPool<T> {
    ///
    /// Create an new [`SpinLockObjectPool`]
    ///
    /// # Arguments
    /// * `init`  closure to create new item
    /// * `reset` closure to reset item before reusage
    ///
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
    /// ```
    #[inline]
    pub fn new<R, I>(init: I, reset: R) -> Self
    where
        R: Fn(&mut T) + Send + Sync + 'static,
        I: Fn() -> T + Send + Sync + 'static,
    {
        Self {
            objects: SpinLock::new(Vec::new()),
            reset: Box::new(reset),
            init: Box::new(init),
        }
    }

    ///
    /// Create a new element. When the element is dropped, it returns in the pull.
    ///
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
    /// ```
    #[inline]
    pub fn pull(&self) -> SpinLockReusable<T> {
        SpinLockReusable::new(
            self,
            ManuallyDrop::new(self.objects.lock().pop().unwrap_or_else(&self.init)),
        )
    }

    ///
    /// Create a new element. When the element is dropped, it returns in the pull.
    ///
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
    /// ```
    #[inline]
    pub fn pull_owned(self: &Arc<Self>) -> SpinLockOwnedReusable<T> {
        SpinLockOwnedReusable::new(
            self.clone(),
            ManuallyDrop::new(self.objects.lock().pop().unwrap_or_else(&self.init)),
        )
    }

    #[inline]
    pub(crate) fn attach(&self, mut data: T) {
        (self.reset)(&mut data);
        self.objects.lock().push(data);
    }
}
