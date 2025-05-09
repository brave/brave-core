#![allow(clippy::len_without_is_empty)]

use std::mem::size_of;
use std::ops::{Deref, Index, RangeBounds};
use std::vec::Drain;

use super::{MemoryLimitExceededError, SharedMemoryLimiter};

#[derive(Debug)]
pub(crate) struct LimitedVec<T> {
    limiter: SharedMemoryLimiter,
    vec: Vec<T>,
}

impl<T> LimitedVec<T> {
    pub const fn new(limiter: SharedMemoryLimiter) -> Self {
        Self {
            vec: vec![],
            limiter,
        }
    }

    pub fn push(&mut self, element: T) -> Result<(), MemoryLimitExceededError> {
        self.limiter.increase_usage(size_of::<T>())?;
        self.vec.push(element);
        Ok(())
    }

    /// Returns the number of elements in the vector, also referred to as its 'length'.
    #[inline]
    pub fn len(&self) -> usize {
        self.vec.len()
    }

    /// Returns the last element of the slice, or None if it is empty.
    #[inline]
    pub fn last(&self) -> Option<&T> {
        self.vec.last()
    }

    /// Returns a mutable pointer to the last item in the slice.
    #[inline]
    pub fn last_mut(&mut self) -> Option<&mut T> {
        self.vec.last_mut()
    }

    /// Creates a draining iterator that removes the specified range in the
    /// vector and yields the removed items.
    pub fn drain<R>(&mut self, range: R) -> Drain<'_, T>
    where
        R: RangeBounds<usize>,
    {
        use std::ops::Bound::*;

        let start = match range.start_bound() {
            Included(&n) => n,
            Excluded(&n) => n + 1,
            Unbounded => 0,
        };

        let end = match range.end_bound() {
            Included(&n) => n + 1,
            Excluded(&n) => n,
            Unbounded => self.len(),
        };

        self.limiter.decrease_usage(size_of::<T>() * (end - start));

        self.vec.drain(range)
    }
}

impl<T> Deref for LimitedVec<T> {
    type Target = [T];

    #[inline]
    fn deref(&self) -> &Self::Target {
        self.vec.as_slice()
    }
}

impl<T> Index<usize> for LimitedVec<T> {
    type Output = T;

    fn index(&self, index: usize) -> &Self::Output {
        &self.vec[index]
    }
}

impl<T> Drop for LimitedVec<T> {
    fn drop(&mut self) {
        self.limiter.decrease_usage(size_of::<T>() * self.vec.len());
    }
}

#[cfg(test)]
mod tests {
    use super::super::SharedMemoryLimiter;
    use super::*;

    #[test]
    fn current_usage() {
        {
            let limiter = SharedMemoryLimiter::new(10);
            let mut vec_u8: LimitedVec<u8> = LimitedVec::new(limiter.clone());

            vec_u8.push(1).unwrap();
            vec_u8.push(2).unwrap();
            assert_eq!(limiter.current_usage(), 2);
        }

        {
            let limiter = SharedMemoryLimiter::new(10);
            let mut vec_u32: LimitedVec<u32> = LimitedVec::new(limiter.clone());

            vec_u32.push(1).unwrap();
            vec_u32.push(2).unwrap();
            assert_eq!(limiter.current_usage(), 8);
        }
    }

    #[test]
    fn max_limit() {
        let limiter = SharedMemoryLimiter::new(2);
        let mut vector: LimitedVec<u8> = LimitedVec::new(limiter);

        vector.push(1).unwrap();
        vector.push(2).unwrap();

        let err = vector.push(3).unwrap_err();

        assert_eq!(err, MemoryLimitExceededError);
    }

    #[test]
    fn drop() {
        let limiter = SharedMemoryLimiter::new(1);

        {
            let mut vector: LimitedVec<u8> = LimitedVec::new(limiter.clone());

            vector.push(1).unwrap();
            assert_eq!(limiter.current_usage(), 1);
        }

        assert_eq!(limiter.current_usage(), 0);
    }

    #[test]
    fn drain() {
        let limiter = SharedMemoryLimiter::new(10);
        let mut vector: LimitedVec<u8> = LimitedVec::new(limiter.clone());

        vector.push(1).unwrap();
        vector.push(2).unwrap();
        vector.push(3).unwrap();
        assert_eq!(limiter.current_usage(), 3);

        vector.drain(0..3);
        assert_eq!(limiter.current_usage(), 0);

        vector.push(1).unwrap();
        vector.push(2).unwrap();
        vector.push(3).unwrap();
        vector.push(4).unwrap();
        assert_eq!(limiter.current_usage(), 4);

        vector.drain(1..=2);
        assert_eq!(limiter.current_usage(), 2);
    }
}
