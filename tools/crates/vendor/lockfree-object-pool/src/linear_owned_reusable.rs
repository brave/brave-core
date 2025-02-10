use crate::linear_object_pool::LinearObjectPool;
use crate::page::{Page, PageId};
use std::ops::{Deref, DerefMut};
use std::sync::Arc;

/// Wrapper over T used by [`LinearObjectPool`].
///
/// Access is allowed with [`std::ops::Deref`] or [`std::ops::DerefMut`]
/// # Example
/// ```rust
///  use lockfree_object_pool::LinearObjectPool;
///  use std::sync::Arc;
///
///  let pool = Arc::new(LinearObjectPool::<u32>::new(
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
pub struct LinearOwnedReusable<T> {
    pool: Arc<LinearObjectPool<T>>,
    page_id: PageId,
    page: *const Page<T>,
}

impl<T> LinearOwnedReusable<T> {
    /// Create new element
    ///
    /// # Arguments
    /// * `pool` object pool owner
    /// * `page_id` page id
    /// * `page`    page that contains data
    /// # Safety
    /// * `page` has to be a valid pointer to a page in `pool`
    /// * `pool_id` has to be a valid id for `page`
    #[inline]
    pub(crate) unsafe fn new(
        pool: Arc<LinearObjectPool<T>>,
        page_id: PageId,
        page: &Page<T>,
    ) -> Self {
        Self {
            pool,
            page_id,
            page,
        }
    }
}

impl<T> DerefMut for LinearOwnedReusable<T> {
    #[inline]
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe {
            // SAFETY: there exists only this `LinearOwnedReusable` with this page_id
            self.page.as_ref().unwrap().get_mut(&self.page_id)
        }
    }
}

impl<T> Deref for LinearOwnedReusable<T> {
    type Target = T;

    #[inline]
    fn deref(&self) -> &Self::Target {
        unsafe {
            // SAFETY: there exists only this `LinearOwnedReusable` with this page_id
            self.page.as_ref().unwrap().get(&self.page_id)
        }
    }
}

impl<T> Drop for LinearOwnedReusable<T> {
    #[inline]
    fn drop(&mut self) {
        unsafe {
            // SAFETY: there exists only this `LinearOwnedReusable` with this page_id
            let page = self.page.as_ref().unwrap();
            (self.pool.get_reset_callback())(page.get_mut(&self.page_id));
            page.free(&self.page_id);
        }
    }
}

unsafe impl<T: Send> Send for LinearOwnedReusable<T> {} // SAFETY: sending the data is allowed if it's Send
unsafe impl<T: Send> Sync for LinearOwnedReusable<T> {} // SAFETY: the Mutex manages synchronization so only Send is required
