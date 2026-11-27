use crate::thread_parker;
use core::hint::spin_loop;

// Wastes some CPU time for the given number of iterations,
// using a hint to indicate to the CPU that we are spinning.
#[inline]
fn cpu_relax(iterations: u32) {
    for _ in 0..iterations {
        spin_loop()
    }
}

/// A counter used to perform exponential backoff in spin loops.
#[derive(Default)]
pub struct SpinWait {
    counter: u32,
}

impl SpinWait {
    /// Creates a new `SpinWait`.
    #[inline]
    pub const fn new() -> Self {
        Self { counter: 0 }
    }

    /// Resets a `SpinWait` to its initial state.
    #[inline]
    pub const fn reset(&mut self) {
        self.counter = 0;
    }

    /// Spins until the sleep threshold has been reached.
    ///
    /// This function returns `false` if the sleep threshold has been reached,
    /// at which point further spinning has diminishing returns and the thread
    /// should be parked instead.
    ///
    /// The spin strategy will initially use a CPU-bound loop but will fall back
    /// to yielding the CPU to the OS after a few iterations.
    #[inline]
    pub fn spin(&mut self) -> bool {
        if self.counter >= 10 {
            return false;
        }
        self.counter += 1;
        if self.counter <= 3 {
            cpu_relax(1 << (self.counter - 1));
        } else {
            thread_parker::thread_yield();
        }
        true
    }

    /// Spins without yielding the thread to the OS.
    ///
    /// Instead, the backoff is simply capped at a maximum value. This can be
    /// used to improve throughput in `compare_exchange` loops that have high
    /// contention.
    #[inline]
    pub fn spin_no_yield(&mut self) {
        self.counter += 1;
        if self.counter > 10 {
            self.counter = 10;
        }
        cpu_relax(1 << (self.counter - 1));
    }
}
