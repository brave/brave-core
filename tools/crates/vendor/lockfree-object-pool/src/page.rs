use std::{
    cell::UnsafeCell,
    sync::atomic::{AtomicU32, Ordering},
};

pub struct Page<T> {
    data: [UnsafeCell<T>; 32],
    free: AtomicU32,
}

pub type PageId = u8;

impl<T> Page<T> {
    #[inline]
    pub fn new<I>(init: I) -> Self
    where
        I: Fn() -> T,
    {
        Self {
            data: [
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
                UnsafeCell::new(init()),
            ],
            free: AtomicU32::new(u32::MAX),
        }
    }

    #[cfg(test)]
    pub(crate) fn is_full(&self) -> bool {
        self.free.load(Ordering::Relaxed) == 0
    }

    #[cfg(test)]
    pub(crate) fn get_mask(&self) -> u32 {
        self.free.load(Ordering::Relaxed)
    }

    #[inline]
    pub fn alloc(&self) -> Option<PageId> {
        self.free
            .fetch_update(Ordering::SeqCst, Ordering::Relaxed, |free| {
                if free == 0 {
                    None
                } else {
                    Some(free & (free - 1))
                }
            })
            .ok()
            .map(|free| free.trailing_zeros() as u8)
    }

    #[inline]
    pub fn free(&self, id: &PageId) {
        let mask: u32 = 1 << id;
        self.free.fetch_or(mask, Ordering::SeqCst);
    }

    #[inline]
    pub unsafe fn get(&self, id: &PageId) -> &T {
        &*self.data[*id as usize].get()
    }

    #[inline]
    #[allow(clippy::mut_from_ref)] // the function is marked as unsafe for a reason
    pub unsafe fn get_mut(&self, id: &PageId) -> &mut T {
        &mut *self.data[*id as usize].get()
    }
}

unsafe impl<T: Send> Send for Page<T> {} // normal rules apply
unsafe impl<T: Sync> Sync for Page<T> {} // normal rules apply

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn test_page_01() {
        let page = Page::<u32>::new(|| 0);
        assert!(!page.is_full());
        assert_eq!(page.get_mask(), u32::MAX);
    }

    #[test]
    fn test_page_02() {
        let page = Page::<u32>::new(|| 0);

        let item1 = page.alloc();
        assert!(item1.is_some());
        assert_eq!(item1.unwrap(), 0);
        assert_eq!(
            format!("{:b}", page.get_mask()),
            "11111111111111111111111111111110"
        );

        let item2 = page.alloc();
        assert!(item2.is_some());
        assert_eq!(item2.unwrap(), 1);
        assert_eq!(
            format!("{:b}", page.get_mask()),
            "11111111111111111111111111111100"
        );

        let item3 = page.alloc();
        assert!(item3.is_some());
        assert_eq!(item3.unwrap(), 2);
        assert_eq!(
            format!("{:b}", page.get_mask()),
            "11111111111111111111111111111000"
        );

        page.free(&item2.unwrap());
        assert_eq!(
            format!("{:b}", page.get_mask()),
            "11111111111111111111111111111010"
        );

        page.free(&item1.unwrap());
        assert_eq!(
            format!("{:b}", page.get_mask()),
            "11111111111111111111111111111011"
        );

        page.free(&item3.unwrap());
        assert_eq!(
            format!("{:b}", page.get_mask()),
            "11111111111111111111111111111111"
        );
    }

    #[test]
    fn test_page_03() {
        let page = Page::<u32>::new(|| 0);
        for i in 0..32 {
            assert!(!page.is_full());

            let item = page.alloc();
            assert!(item.is_some());
            assert_eq!(item.unwrap(), i);
        }
        assert!(page.is_full());
        let item = page.alloc();
        assert!(item.is_none());
    }
}
