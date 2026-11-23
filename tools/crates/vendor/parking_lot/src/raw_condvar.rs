use crate::raw_mutex::{RawMutex, TOKEN_HANDOFF, TOKEN_NORMAL};
use crate::{deadlock, util};
use core::{
    ptr,
    sync::atomic::{AtomicPtr, Ordering},
};
use lock_api::RawMutex as RawMutex_;
use parking_lot_core::{self, DEFAULT_PARK_TOKEN, ParkResult, RequeueOp, UnparkResult};
use std::time::{Duration, Instant};

/// A raw condition variable.
///
/// Waits never return spuriously. [`notify_one`](lock_api::RawCondvar::notify_one)
/// returns exactly whether a waiting thread was selected for notification, and
/// [`notify_all`](lock_api::RawCondvar::notify_all) returns the exact number of
/// waiting threads selected. A thread requeued directly onto the associated
/// mutex is considered notified.
pub struct RawCondvar {
    state: AtomicPtr<RawMutex>,
}

// SAFETY: Waiting atomically releases and re-acquires the mutex, does not wake
// spuriously, and safely panics if used with distinct mutexes simultaneously.
unsafe impl lock_api::RawCondvar for RawCondvar {
    const INIT: Self = RawCondvar {
        state: AtomicPtr::new(ptr::null_mut()),
    };

    type RawMutex = RawMutex;

    unsafe fn wait(&self, mutex: &RawMutex, _guard: &mut crate::GuardMarker) {
        self.wait_until_internal(mutex, None);
    }

    #[inline]
    fn notify_one(&self) -> bool {
        // Nothing to do if there are no waiting threads
        let state = self.state.load(Ordering::Relaxed);
        if state.is_null() {
            return false;
        }

        self.notify_one_slow(state)
    }

    #[inline]
    fn notify_all(&self) -> usize {
        // Nothing to do if there are no waiting threads
        let state = self.state.load(Ordering::Relaxed);
        if state.is_null() {
            return 0;
        }

        self.notify_all_slow(state)
    }
}

// SAFETY: The timed waits uphold the same requirements as the untimed wait and
// accurately distinguish notification from timeout.
unsafe impl lock_api::RawCondvarTimed for RawCondvar {
    fn checked_duration_to_instant(timeout: &Duration) -> Option<Instant> {
        util::to_deadline(*timeout)
    }

    unsafe fn wait_for(
        &self,
        mutex: &RawMutex,
        _guard: &mut crate::GuardMarker,
        timeout: &Duration,
    ) -> bool {
        let deadline = util::to_deadline(*timeout);
        self.wait_until_internal(mutex, deadline)
    }

    unsafe fn wait_until(
        &self,
        mutex: &RawMutex,
        _guard: &mut crate::GuardMarker,
        timeout: &Instant,
    ) -> bool {
        self.wait_until_internal(mutex, Some(*timeout))
    }
}

impl RawCondvar {
    #[cold]
    fn notify_one_slow(&self, mutex: *mut RawMutex) -> bool {
        // Unpark one thread or requeue it onto the mutex
        let from = ptr::from_ref(self).addr();
        let to = mutex.addr();
        let validate = || {
            // Make sure that our atomic state still points to the same
            // mutex. If not then it means that all threads on the current
            // mutex were woken up and a new waiting thread switched to a
            // different mutex. In that case we can get away with doing
            // nothing.
            if self.state.load(Ordering::Relaxed) != mutex {
                return RequeueOp::Abort;
            }

            // Unpark one thread if the mutex is unlocked, otherwise
            // requeue it onto the mutex. This is safe to do here
            // since unlocking the mutex when the parked bit is set requires
            // locking the queue. There is the possibility of a race if the
            // mutex gets locked after we check, but that doesn't matter in
            // this case.
            if unsafe { (*mutex).mark_parked_if_locked() } {
                RequeueOp::RequeueOne
            } else {
                RequeueOp::UnparkOne
            }
        };
        let callback = |_op, result: UnparkResult| {
            // Clear our state if there are no more waiting threads
            if !result.have_more_threads {
                self.state.store(ptr::null_mut(), Ordering::Relaxed);
            }
            TOKEN_NORMAL
        };
        let res = unsafe { parking_lot_core::unpark_requeue(from, to, validate, callback) };

        res.unparked_threads + res.requeued_threads != 0
    }

    #[cold]
    fn notify_all_slow(&self, mutex: *mut RawMutex) -> usize {
        // Unpark one thread and requeue the rest onto the mutex
        let from = ptr::from_ref(self).addr();
        let to = mutex.addr();
        let validate = || {
            // Make sure that our atomic state still points to the same
            // mutex. If not then it means that all threads on the current
            // mutex were woken up and a new waiting thread switched to a
            // different mutex. In that case we can get away with doing
            // nothing.
            if self.state.load(Ordering::Relaxed) != mutex {
                return RequeueOp::Abort;
            }

            // Clear our state since we are going to unpark or requeue all
            // threads.
            self.state.store(ptr::null_mut(), Ordering::Relaxed);

            // Unpark one thread if the mutex is unlocked, otherwise just
            // requeue everything to the mutex. This is safe to do here
            // since unlocking the mutex when the parked bit is set requires
            // locking the queue. There is the possibility of a race if the
            // mutex gets locked after we check, but that doesn't matter in
            // this case.
            if unsafe { (*mutex).mark_parked_if_locked() } {
                RequeueOp::RequeueAll
            } else {
                RequeueOp::UnparkOneRequeueRest
            }
        };
        let callback = |op, result: UnparkResult| {
            // If we requeued threads to the mutex, mark it as having
            // parked threads. The RequeueAll case is already handled above.
            if op == RequeueOp::UnparkOneRequeueRest && result.requeued_threads != 0 {
                unsafe { (*mutex).mark_parked() };
            }
            TOKEN_NORMAL
        };
        let res = unsafe { parking_lot_core::unpark_requeue(from, to, validate, callback) };

        res.unparked_threads + res.requeued_threads
    }

    // This is a non-generic function to reduce the monomorphization cost of
    // using `wait_until`.
    fn wait_until_internal(&self, mutex: &RawMutex, timeout: Option<Instant>) -> bool {
        let result;
        let mut requeued = false;

        // Stop recording ownership before ThreadData is transferred to the
        // parking lot. The physical unlock remains in `before_sleep` so that
        // unlocking and parking are atomic with respect to notification.
        let mutex_addr = ptr::from_ref(mutex).addr();
        unsafe { deadlock::release_resource(mutex_addr) };

        {
            let addr = ptr::from_ref(self).addr();
            let lock_addr = mutex as *const _ as *mut _;
            let validate = || {
                // Ensure we don't use two different mutexes with the same
                // Condvar at the same time. This is done while locked to
                // avoid races with notify_one
                let state = self.state.load(Ordering::Relaxed);
                if state.is_null() {
                    self.state.store(lock_addr, Ordering::Relaxed);
                } else if state != lock_addr {
                    return false;
                }
                true
            };
            let before_sleep = || {
                // Unlock the mutex before sleeping...
                unsafe { mutex.unlock_inner() };
            };
            let timed_out = |k, was_last_thread| {
                // If we were requeued to a mutex, then we did not time out.
                // We'll just park ourselves on the mutex again when we try
                // to lock it later.
                requeued = k != addr;

                // If we were the last thread on the queue then we need to
                // clear our state. This is normally done by the
                // notify_{one,all} functions when not timing out.
                if !requeued && was_last_thread {
                    self.state.store(ptr::null_mut(), Ordering::Relaxed);
                }
            };
            result = unsafe {
                parking_lot_core::park(
                    addr,
                    validate,
                    before_sleep,
                    timed_out,
                    DEFAULT_PARK_TOKEN,
                    timeout,
                )
            };
        }

        // Panic if we tried to use multiple mutexes with a Condvar. Note
        // that at this point the MutexGuard is still locked. It will be
        // unlocked by the unwinding logic.
        if result == ParkResult::Invalid {
            // We never got around to unlocking the mutex.
            unsafe { deadlock::acquire_resource(mutex_addr) };
            panic!("attempted to use a condition variable with more than one mutex");
        }

        // ... and re-lock it once we are done sleeping
        if result == ParkResult::Unparked(TOKEN_HANDOFF) {
            unsafe { deadlock::acquire_resource(mutex_addr) };
        } else {
            mutex.lock();
        }

        !(result.is_unparked() || requeued)
    }
}
