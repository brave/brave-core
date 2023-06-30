use super::{MemoryLimitExceededError, SharedMemoryLimiter};
use safemem::copy_over;

/// Preallocated region of memory that can grow and never deallocates during the lifetime of
/// the limiter.
#[derive(Debug)]
pub struct Arena {
    limiter: SharedMemoryLimiter,
    data: Vec<u8>,
}

impl Arena {
    pub fn new(limiter: SharedMemoryLimiter, preallocated_size: usize) -> Self {
        limiter.borrow_mut().preallocate(preallocated_size);

        Arena {
            limiter,
            data: Vec::with_capacity(preallocated_size),
        }
    }

    pub fn append(&mut self, slice: &[u8]) -> Result<(), MemoryLimitExceededError> {
        let new_len = self.data.len() + slice.len();
        let capacity = self.data.capacity();

        if new_len > capacity {
            let additional = new_len - capacity;

            // NOTE: approximate usage, as `Vec::reserve_exact` doesn't
            // give guarantees about exact capacity value :).
            self.limiter.borrow_mut().increase_usage(additional)?;

            // NOTE: with wicely choosen preallocated size this branch should be
            // executed quite rarely. We can't afford to use double capacity
            // strategy used by default (see: https://github.com/rust-lang/rust/blob/bdfd698f37184da42254a03ed466ab1f90e6fb6c/src/liballoc/raw_vec.rs#L424)
            // as we'll run out of the space allowance quite quickly.
            self.data.reserve_exact(slice.len());
        }

        self.data.extend_from_slice(slice);

        Ok(())
    }

    pub fn init_with(&mut self, slice: &[u8]) -> Result<(), MemoryLimitExceededError> {
        self.data.clear();
        self.append(slice)
    }

    pub fn shift(&mut self, byte_count: usize) {
        let remainder_len = self.data.len() - byte_count;

        copy_over(&mut self.data, byte_count, 0, remainder_len);
        self.data.truncate(remainder_len);
    }

    pub fn bytes(&self) -> &[u8] {
        &self.data
    }
}

#[cfg(test)]
mod tests {
    use super::super::limiter::MemoryLimiter;
    use super::*;
    use std::rc::Rc;

    #[test]
    fn append() {
        let limiter = MemoryLimiter::new_shared(10);
        let mut arena = Arena::new(Rc::clone(&limiter), 2);

        arena.append(&[1, 2]).unwrap();
        assert_eq!(arena.bytes(), &[1, 2]);
        assert_eq!(limiter.borrow().current_usage(), 2);

        arena.append(&[3, 4]).unwrap();
        assert_eq!(arena.bytes(), &[1, 2, 3, 4]);
        assert_eq!(limiter.borrow().current_usage(), 4);

        arena.append(&[5, 6, 7, 8, 9, 10]).unwrap();
        assert_eq!(arena.bytes(), &[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
        assert_eq!(limiter.borrow().current_usage(), 10);

        let err = arena.append(&[11]).unwrap_err();

        assert_eq!(err, MemoryLimitExceededError);
    }

    #[test]
    fn init_with() {
        let limiter = MemoryLimiter::new_shared(5);
        let mut arena = Arena::new(Rc::clone(&limiter), 0);

        arena.init_with(&[1]).unwrap();
        assert_eq!(arena.bytes(), &[1]);
        assert_eq!(limiter.borrow().current_usage(), 1);

        arena.append(&[1, 2]).unwrap();
        assert_eq!(arena.bytes(), &[1, 1, 2]);
        assert_eq!(limiter.borrow().current_usage(), 3);

        arena.init_with(&[1, 2, 3]).unwrap();
        assert_eq!(arena.bytes(), &[1, 2, 3]);
        assert_eq!(limiter.borrow().current_usage(), 3);

        arena.init_with(&[]).unwrap();
        assert_eq!(arena.bytes(), &[]);
        assert_eq!(limiter.borrow().current_usage(), 3);

        let err = arena.init_with(&[1, 2, 3, 4, 5, 6, 7]).unwrap_err();

        assert_eq!(err, MemoryLimitExceededError);
    }

    #[test]
    fn shift() {
        let limiter = MemoryLimiter::new_shared(10);
        let mut arena = Arena::new(Rc::clone(&limiter), 0);

        arena.append(&[0, 1, 2, 3]).unwrap();
        arena.shift(2);
        assert_eq!(arena.bytes(), &[2, 3]);
        assert_eq!(limiter.borrow().current_usage(), 4);

        arena.append(&[0, 1]).unwrap();
        assert_eq!(arena.bytes(), &[2, 3, 0, 1]);
        assert_eq!(limiter.borrow().current_usage(), 4);

        arena.shift(3);
        assert_eq!(arena.bytes(), &[1]);
        assert_eq!(limiter.borrow().current_usage(), 4);

        arena.append(&[2, 3, 4, 5]).unwrap();
        arena.shift(1);
        assert_eq!(arena.bytes(), &[2, 3, 4, 5]);
        assert_eq!(limiter.borrow().current_usage(), 5);
    }
}
