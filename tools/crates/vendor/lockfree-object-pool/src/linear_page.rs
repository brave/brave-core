use crate::page::{Page, PageId};
use std::ptr;
use std::sync::atomic::{AtomicPtr, Ordering};

pub struct LinearPage<T> {
    page: Page<T>,
    next: AtomicPtr<LinearPage<T>>,
}

impl<T> LinearPage<T> {
    #[inline]
    pub fn new<I>(init: I) -> Self
    where
        I: Fn() -> T,
    {
        Self {
            page: Page::new(init),
            next: AtomicPtr::new(ptr::null_mut()),
        }
    }

    #[inline]
    pub fn get_or_create_next<I>(&self, init: I) -> &Self
    where
        I: Fn() -> T,
    {
        let mut current = self.next.load(Ordering::Relaxed);
        if current.is_null() {
            let new = Box::into_raw(Box::new(LinearPage::<T>::new(init)));
            match self
                .next
                .compare_exchange(current, new, Ordering::SeqCst, Ordering::Relaxed)
            {
                Ok(_) => {
                    current = new;
                }
                Err(x) => {
                    unsafe {
                        // SAFETY: new was been allocated by Box::new
                        drop(Box::from_raw(new))
                    };
                    current = x;
                }
            }
        }
        unsafe {
            // SAFETY: there are no mutable references to current
            current.as_ref().unwrap()
        }
    }

    #[inline]
    pub fn alloc<I>(&self, init: I) -> (&Page<T>, PageId)
    where
        I: Fn() -> T + Clone,
    {
        let mut linear_page = self;
        loop {
            match linear_page.page.alloc() {
                Some(id) => {
                    return (&linear_page.page, id);
                }
                None => {
                    linear_page = linear_page.get_or_create_next(init.clone());
                }
            };
        }
    }
}

impl<T> Drop for LinearPage<T> {
    #[inline]
    fn drop(&mut self) {
        let current = self.next.load(Ordering::Relaxed);
        if !current.is_null() {
            unsafe {
                // SAFETY: current was allocated with Box::new
                drop(Box::from_raw(current))
            };
        }
    }
}
