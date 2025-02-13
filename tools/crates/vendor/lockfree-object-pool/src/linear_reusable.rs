use crate::linear_object_pool::LinearObjectPool;
use crate::page::{Page, PageId};
use std::ops::{Deref, DerefMut};

/// Wrapper over T used by [`LinearObjectPool`].
///
/// Access is allowed with [`std::ops::Deref`] or [`std::ops::DerefMut`]
/// # Example
/// ```rust
///  use lockfree_object_pool::LinearObjectPool;
///
///  let pool = LinearObjectPool::<u32>::new(
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
pub struct LinearReusable<'a, T> {
    pool: &'a LinearObjectPool<T>,
    page_id: PageId,
    page: &'a Page<T>,
}

impl<'a, T> LinearReusable<'a, T> {
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
        pool: &'a LinearObjectPool<T>,
        page_id: PageId,
        page: &'a Page<T>,
    ) -> Self {
        Self {
            pool,
            page_id,
            page,
        }
    }
}

impl<'a, T> DerefMut for LinearReusable<'a, T> {
    #[inline]
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe {
            // SAFETY: there exists only this `LinearReusable` with this page_id
            self.page.get_mut(&self.page_id)
        }
    }
}

impl<'a, T> Deref for LinearReusable<'a, T> {
    type Target = T;

    #[inline]
    fn deref(&self) -> &Self::Target {
        unsafe {
            // SAFETY: there exists only this `LinearReusable` with this page_id
            self.page.get(&self.page_id)
        }
    }
}

impl<'a, T> Drop for LinearReusable<'a, T> {
    #[inline]
    fn drop(&mut self) {
        let page = self.page;
        (self.pool.get_reset_callback())(unsafe {
            // SAFETY: there exists only this `LinearReusable` with this page_id
            page.get_mut(&self.page_id)
        });
        page.free(&self.page_id);
    }
}
