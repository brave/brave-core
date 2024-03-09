use std::cell::RefCell;
use std::rc::Rc;
use thiserror::Error;

pub type SharedMemoryLimiter = Rc<RefCell<MemoryLimiter>>;

/// An error that occures when rewriter exceedes the memory limit specified in the
/// [`MemorySettings`].
///
/// [`MemorySettings`]: ../struct.MemorySettings.html
#[derive(Error, Debug, PartialEq, Copy, Clone)]
#[error("The memory limit has been exceeded.")]
pub struct MemoryLimitExceededError;

#[derive(Debug)]
pub struct MemoryLimiter {
    current_usage: usize,
    max: usize,
}

impl MemoryLimiter {
    pub fn new_shared(max: usize) -> SharedMemoryLimiter {
        Rc::new(RefCell::new(MemoryLimiter {
            max,
            current_usage: 0,
        }))
    }

    #[cfg(test)]
    pub fn current_usage(&self) -> usize {
        self.current_usage
    }

    #[inline]
    pub fn increase_usage(&mut self, byte_count: usize) -> Result<(), MemoryLimitExceededError> {
        self.current_usage += byte_count;

        if self.current_usage > self.max {
            Err(MemoryLimitExceededError)
        } else {
            Ok(())
        }
    }

    #[inline]
    pub fn preallocate(&mut self, byte_count: usize) {
        self.increase_usage(byte_count).expect(
            "Total preallocated memory size should be less than `MemorySettings::max_allowed_memory_usage`.",
        );
    }

    #[inline]
    pub fn decrease_usage(&mut self, byte_count: usize) {
        self.current_usage -= byte_count;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn current_usage() {
        let limiter = MemoryLimiter::new_shared(10);
        let mut limiter = limiter.borrow_mut();

        assert_eq!(limiter.current_usage(), 0);

        limiter.increase_usage(3).unwrap();
        assert_eq!(limiter.current_usage(), 3);

        limiter.increase_usage(5).unwrap();
        assert_eq!(limiter.current_usage(), 8);

        limiter.decrease_usage(4);
        assert_eq!(limiter.current_usage(), 4);

        let err = limiter.increase_usage(15).unwrap_err();

        assert_eq!(err, MemoryLimitExceededError);
    }

    #[test]
    #[should_panic(
        expected = "Total preallocated memory size should be less than `MemorySettings::max_allowed_memory_usage`."
    )]
    fn preallocate() {
        let limiter = MemoryLimiter::new_shared(10);
        let mut limiter = limiter.borrow_mut();

        limiter.preallocate(8);
        assert_eq!(limiter.current_usage(), 8);

        limiter.preallocate(10);
    }
}
