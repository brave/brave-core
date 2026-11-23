use crate::{deadlock, util};
use core::{
    sync::atomic::{AtomicU8, Ordering},
    time::Duration,
};
use lock_api::RawMutex as RawMutex_;
use parking_lot_core::{self, DEFAULT_PARK_TOKEN, ParkResult, SpinWait, UnparkResult, UnparkToken};
use std::time::Instant;

// UnparkToken used to indicate that the target thread should attempt to
// lock the mutex again as soon as it is unparked.
pub(crate) const TOKEN_NORMAL: UnparkToken = UnparkToken(0);

// UnparkToken used to indicate that the mutex is being handed off to the target
// thread directly without unlocking it.
pub(crate) const TOKEN_HANDOFF: UnparkToken = UnparkToken(1);

/// This bit is set in the `state` of a `RawMutex` when that mutex is locked by some thread.
const LOCKED_BIT: u8 = 0b01;
/// This bit is set in the `state` of a `RawMutex` just before parking a thread. A thread is being
/// parked if it wants to lock the mutex, but it is currently being held by some other thread.
const PARKED_BIT: u8 = 0b10;

/// Raw mutex type backed by the parking lot.
pub struct RawMutex {
    /// This atomic integer holds the current state of the mutex instance. Only the two lowest bits
    /// are used. See `LOCKED_BIT` and `PARKED_BIT` for the bitmask for these bits.
    ///
    /// # State table:
    ///
    /// PARKED_BIT | LOCKED_BIT | Description
    ///     0      |     0      | The mutex is not locked, nor is anyone waiting for it.
    /// -----------+------------+------------------------------------------------------------------
    ///     0      |     1      | The mutex is locked by exactly one thread. No other thread is
    ///            |            | waiting for it.
    /// -----------+------------+------------------------------------------------------------------
    ///     1      |     0      | The mutex is not locked. One or more threads are parked or about to
    ///            |            | park. At least one of the parked threads is just about to be
    ///            |            | unparked, or a thread heading for parking might abort the park.
    /// -----------+------------+------------------------------------------------------------------
    ///     1      |     1      | The mutex is locked by exactly one thread. One or more threads are
    ///            |            | parked or about to park, waiting for the lock to become available.
    ///            |            | In this state, PARKED_BIT is only ever cleared when a bucket lock
    ///            |            | is held (i.e. in a parking_lot_core callback). This ensures that
    ///            |            | we never end up in a situation where there are parked threads but
    ///            |            | PARKED_BIT is not set (which would result in those threads
    ///            |            | potentially never getting woken up).
    state: AtomicU8,
}

unsafe impl lock_api::RawMutex for RawMutex {
    const INIT: RawMutex = RawMutex {
        state: AtomicU8::new(0),
    };

    type Guard = crate::GuardMarker;

    #[inline]
    fn lock(&self) -> Self::Guard {
        let guard = if self
            .state
            .compare_exchange_weak(0, LOCKED_BIT, Ordering::Acquire, Ordering::Relaxed)
            .is_err()
        {
            self.lock_slow(None).unwrap()
        } else {
            Self::Guard::new()
        };
        unsafe { deadlock::acquire_resource(core::ptr::from_ref(self).addr()) };
        guard
    }

    #[inline]
    fn try_lock(&self) -> Option<Self::Guard> {
        self.state
            .try_update(Ordering::Acquire, Ordering::Relaxed, |state| {
                (state & LOCKED_BIT == 0).then_some(state | LOCKED_BIT)
            })
            .ok()?;
        unsafe { deadlock::acquire_resource(core::ptr::from_ref(self).addr()) };
        Some(Self::Guard::new())
    }

    #[inline]
    unsafe fn unlock(&self, _guard: Self::Guard) {
        unsafe { deadlock::release_resource(core::ptr::from_ref(self).addr()) };
        unsafe { self.unlock_inner() };
    }

    #[inline]
    fn is_locked(&self) -> bool {
        let state = self.state.load(Ordering::Relaxed);
        state & LOCKED_BIT != 0
    }
}

unsafe impl lock_api::RawMutexFair for RawMutex {
    #[inline]
    unsafe fn unlock_fair(&self, _guard: Self::Guard) {
        unsafe { deadlock::release_resource(core::ptr::from_ref(self).addr()) };
        if self
            .state
            .compare_exchange(LOCKED_BIT, 0, Ordering::Release, Ordering::Relaxed)
            .is_err()
        {
            self.unlock_fair_slow();
        }
    }

    #[inline]
    unsafe fn bump(&self, _guard: &mut Self::Guard) {
        if self.state.load(Ordering::Relaxed) & PARKED_BIT != 0 {
            self.bump_slow();
        }
    }
}

unsafe impl lock_api::RawMutexTimed for RawMutex {
    type Duration = Duration;
    type Instant = Instant;

    #[inline]
    fn try_lock_until(&self, timeout: Instant) -> Option<Self::Guard> {
        let guard = if self
            .state
            .compare_exchange_weak(0, LOCKED_BIT, Ordering::Acquire, Ordering::Relaxed)
            .is_ok()
        {
            Self::Guard::new()
        } else {
            self.lock_slow(Some(timeout))?
        };
        unsafe { deadlock::acquire_resource(core::ptr::from_ref(self).addr()) };
        Some(guard)
    }

    #[inline]
    fn try_lock_for(&self, timeout: Duration) -> Option<Self::Guard> {
        let guard = if self
            .state
            .compare_exchange_weak(0, LOCKED_BIT, Ordering::Acquire, Ordering::Relaxed)
            .is_ok()
        {
            Self::Guard::new()
        } else {
            self.lock_slow(util::to_deadline(timeout))?
        };
        unsafe { deadlock::acquire_resource(core::ptr::from_ref(self).addr()) };
        Some(guard)
    }
}

impl RawMutex {
    /// Unlocks the mutex without updating the deadlock detector.
    ///
    /// # Safety
    ///
    /// The caller must own the mutex.
    #[inline]
    pub(crate) unsafe fn unlock_inner(&self) {
        let state = self.state.fetch_sub(LOCKED_BIT, Ordering::Release);
        if state & PARKED_BIT != 0 {
            self.unlock_slow();
        }
    }

    // Used by Condvar when requeuing threads to us, must be called while
    // holding the queue lock.
    #[inline]
    pub(crate) fn mark_parked_if_locked(&self) -> bool {
        self.state
            .try_update(Ordering::Relaxed, Ordering::Relaxed, |state| {
                (state & LOCKED_BIT != 0).then_some(state | PARKED_BIT)
            })
            .is_ok()
    }

    // Used by Condvar when requeuing threads to us, must be called while
    // holding the queue lock.
    #[inline]
    pub(crate) fn mark_parked(&self) {
        self.state.fetch_or(PARKED_BIT, Ordering::Relaxed);
    }

    #[cold]
    fn lock_slow(&self, timeout: Option<Instant>) -> Option<crate::GuardMarker> {
        let mut spinwait = SpinWait::new();
        let mut state = self.state.load(Ordering::Relaxed);
        loop {
            // Grab the lock if it isn't locked, even if there is a queue on it
            if state & LOCKED_BIT == 0 {
                match self.state.compare_exchange_weak(
                    state,
                    state | LOCKED_BIT,
                    Ordering::Acquire,
                    Ordering::Relaxed,
                ) {
                    Ok(_) => return Some(crate::GuardMarker::new()),
                    Err(x) => state = x,
                }
                continue;
            }

            // If there is no queue, try spinning a few times
            if state & PARKED_BIT == 0 && spinwait.spin() {
                state = self.state.load(Ordering::Relaxed);
                continue;
            }

            // Set the parked bit
            if state & PARKED_BIT == 0
                && let Err(x) = self.state.compare_exchange_weak(
                    state,
                    state | PARKED_BIT,
                    Ordering::Relaxed,
                    Ordering::Relaxed,
                )
            {
                state = x;
                continue;
            }

            // Park our thread until we are woken up by an unlock
            let addr = core::ptr::from_ref(self).addr();
            let validate = || self.state.load(Ordering::Relaxed) == LOCKED_BIT | PARKED_BIT;
            let before_sleep = || {};
            let timed_out = |_, was_last_thread| {
                // Clear the parked bit if we were the last parked thread
                if was_last_thread {
                    self.state.fetch_and(!PARKED_BIT, Ordering::Relaxed);
                }
            };
            // SAFETY:
            //   * `addr` is an address we control.
            //   * `validate`/`timed_out` does not panic or call into any function of `parking_lot`.
            //   * `before_sleep` does not call `park`, nor does it panic.
            match unsafe {
                parking_lot_core::park(
                    addr,
                    validate,
                    before_sleep,
                    timed_out,
                    DEFAULT_PARK_TOKEN,
                    timeout,
                )
            } {
                // The thread that unparked us passed the lock on to us
                // directly without unlocking it.
                ParkResult::Unparked(TOKEN_HANDOFF) => return Some(crate::GuardMarker::new()),

                // We were unparked normally, try acquiring the lock again
                ParkResult::Unparked(_) => (),

                // The validation function failed, try locking again
                ParkResult::Invalid => (),

                // Timeout expired
                ParkResult::TimedOut => return None,
            }

            // Loop back and try locking again
            spinwait.reset();
            state = self.state.load(Ordering::Relaxed);
        }
    }

    #[cold]
    fn unlock_slow(&self) {
        let addr = core::ptr::from_ref(self).addr();
        let callback = |result: UnparkResult| {
            // Ownership has already been released. Preserve LOCKED_BIT since
            // another thread may have acquired the mutex in the meantime.
            if !result.have_more_threads {
                self.state.fetch_and(!PARKED_BIT, Ordering::Relaxed);
            }
            TOKEN_NORMAL
        };
        // SAFETY:
        //   * `addr` is an address we control.
        //   * `callback` does not panic or call into any function of `parking_lot`.
        unsafe {
            parking_lot_core::unpark_one(addr, callback);
        }
    }

    #[cold]
    fn unlock_fair_slow(&self) {
        // Unpark one thread and leave the parked bit set if there might
        // still be parked threads on this address.
        let addr = core::ptr::from_ref(self).addr();
        let callback = |result: UnparkResult| {
            // If we are using a fair unlock then we should keep the
            // mutex locked and hand it off to the unparked thread.
            if result.unparked_threads != 0 {
                // Clear the parked bit if there are no more parked
                // threads.
                if !result.have_more_threads {
                    self.state.store(LOCKED_BIT, Ordering::Relaxed);
                }
                return TOKEN_HANDOFF;
            }

            // Clear the locked bit, and the parked bit as well if there
            // are no more parked threads.
            if result.have_more_threads {
                self.state.store(PARKED_BIT, Ordering::Release);
            } else {
                self.state.store(0, Ordering::Release);
            }
            TOKEN_NORMAL
        };
        // SAFETY:
        //   * `addr` is an address we control.
        //   * `callback` does not panic or call into any function of `parking_lot`.
        unsafe {
            parking_lot_core::unpark_one(addr, callback);
        }
    }

    #[cold]
    fn bump_slow(&self) {
        unsafe { deadlock::release_resource(core::ptr::from_ref(self).addr()) };
        self.unlock_fair_slow();
        self.lock();
    }
}
