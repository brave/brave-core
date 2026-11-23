use core::{
    fmt, mem,
    sync::atomic::{AtomicU8, Ordering, fence},
};
use parking_lot_core::{self, DEFAULT_PARK_TOKEN, DEFAULT_UNPARK_TOKEN, SpinWait};

use crate::deadlock;

const DONE_BIT: u8 = 1;
const POISON_BIT: u8 = 2;
const LOCKED_BIT: u8 = 4;
const PARKED_BIT: u8 = 8;

/// Current state of a `Once`.
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum OnceState {
    /// A closure has not been executed yet.
    New,

    /// A closure was executed but panicked.
    Poisoned,

    /// A thread is currently executing a closure.
    InProgress,

    /// A closure has completed successfully.
    Done,
}

impl OnceState {
    /// Returns `true` if this is the [`Poisoned`](OnceState::Poisoned) state.
    /// When a state is passed to [`Once::call_once_force`], this indicates that
    /// the [`Once`] was poisoned before the closure was invoked.
    ///
    /// # Examples
    ///
    /// A poisoned [`Once`]:
    ///
    /// ```
    /// use parking_lot::Once;
    /// use std::thread;
    ///
    /// static INIT: Once = Once::new();
    ///
    /// let handle = thread::spawn(|| {
    ///     INIT.call_once(|| panic!());
    /// });
    /// assert!(handle.join().is_err());
    ///
    /// INIT.call_once_force(|state| {
    ///     assert!(state.is_poisoned());
    /// });
    /// ```
    ///
    /// An unpoisoned [`Once`]:
    ///
    /// ```
    /// use parking_lot::Once;
    ///
    /// static INIT: Once = Once::new();
    ///
    /// INIT.call_once_force(|state| {
    ///     assert!(!state.is_poisoned());
    /// });
    /// ```
    #[inline]
    pub const fn is_poisoned(self) -> bool {
        matches!(self, OnceState::Poisoned)
    }

    /// Alias for [`is_poisoned`](Self::is_poisoned).
    #[inline]
    pub const fn poisoned(self) -> bool {
        self.is_poisoned()
    }

    /// Returns `true` if this is the [`Done`](OnceState::Done) state.
    #[inline]
    pub const fn is_completed(self) -> bool {
        matches!(self, OnceState::Done)
    }

    /// Alias for [`is_completed`](Self::is_completed).
    #[inline]
    pub const fn done(self) -> bool {
        self.is_completed()
    }
}

/// A synchronization primitive which can be used to run a one-time
/// initialization. Useful for one-time initialization for globals, FFI or
/// related functionality.
///
/// When the initialization produces a value, [`std::sync::OnceLock`] is often
/// more convenient.
///
/// # Examples
///
/// ```
/// use parking_lot::Once;
///
/// static START: Once = Once::new();
///
/// START.call_once(|| {
///     // run initialization here
/// });
/// ```
pub struct Once(AtomicU8);

impl Once {
    /// Creates a new `Once` value.
    #[inline]
    pub const fn new() -> Once {
        Once(AtomicU8::new(0))
    }

    /// Creates a new `Once` value that is already in the completed state.
    ///
    /// A `Once` created this way will never invoke a closure passed to
    /// [`call_once`](Self::call_once) or [`call_once_force`](Self::call_once_force),
    /// and its [`state`](Self::state) will always be [`OnceState::Done`].
    ///
    /// # Examples
    ///
    /// ```
    /// use parking_lot::{Once, OnceState};
    ///
    /// static INIT: Once = Once::new_completed();
    /// assert_eq!(INIT.state(), OnceState::Done);
    ///
    /// // The closure is never executed.
    /// INIT.call_once(|| unreachable!());
    /// ```
    #[inline]
    pub const fn new_completed() -> Once {
        Once(AtomicU8::new(DONE_BIT))
    }

    /// Returns a snapshot of the current state of this `Once`.
    ///
    /// The state may change immediately after this function returns.
    #[inline]
    pub fn state(&self) -> OnceState {
        let state = self.0.load(Ordering::Acquire);
        if state & DONE_BIT != 0 {
            OnceState::Done
        } else if state & LOCKED_BIT != 0 {
            OnceState::InProgress
        } else if state & POISON_BIT != 0 {
            OnceState::Poisoned
        } else {
            OnceState::New
        }
    }

    /// Returns `true` if this `Once` is in the completed state.
    ///
    /// A `Once` becomes completed when an initialization closure finishes
    /// successfully, or when it is created with
    /// [`new_completed`](Self::new_completed). This method returns `false` if
    /// initialization has not started, is still in progress, or the `Once` is
    /// poisoned.
    ///
    /// A `false` result may be stale. For example, initialization may complete
    /// between the state being read and this function returning.
    ///
    /// # Examples
    ///
    /// ```
    /// use parking_lot::Once;
    ///
    /// static INIT: Once = Once::new();
    ///
    /// assert!(!INIT.is_completed());
    /// INIT.call_once(|| {
    ///     assert!(!INIT.is_completed());
    /// });
    /// assert!(INIT.is_completed());
    /// assert!(Once::new_completed().is_completed());
    /// ```
    ///
    /// A poisoned [`Once`] has not completed successfully:
    ///
    /// ```
    /// use parking_lot::Once;
    /// use std::thread;
    ///
    /// static INIT: Once = Once::new();
    ///
    /// let handle = thread::spawn(|| {
    ///     INIT.call_once(|| panic!());
    /// });
    /// assert!(handle.join().is_err());
    /// assert!(!INIT.is_completed());
    /// ```
    #[inline]
    pub fn is_completed(&self) -> bool {
        self.0.load(Ordering::Acquire) & DONE_BIT != 0
    }

    /// Blocks the current thread until initialization has completed.
    ///
    /// Calling this method from an initialization closure for the same `Once`
    /// may panic or deadlock.
    ///
    /// # Examples
    ///
    /// ```
    /// use parking_lot::Once;
    /// use std::thread;
    ///
    /// static READY: Once = Once::new();
    ///
    /// let thread = thread::spawn(|| {
    ///     READY.wait();
    ///     println!("everything is ready");
    /// });
    ///
    /// READY.call_once(|| println!("performing setup"));
    /// thread.join().unwrap();
    /// ```
    ///
    /// # Panics
    ///
    /// If this `Once` has been poisoned because an initialization closure has
    /// panicked, this method will also panic. Use
    /// [`wait_force`](Self::wait_force) if this behavior is not desired.
    #[inline]
    pub fn wait(&self) {
        if !self.is_completed() {
            self.wait_slow(false);
        }
    }

    /// Blocks the current thread until initialization has completed, ignoring
    /// poisoning.
    ///
    /// If this `Once` has been poisoned, this function blocks until it becomes
    /// completed, unlike [`Once::wait`], which panics in this case.
    ///
    /// Calling this method from an initialization closure for the same `Once`
    /// may panic or deadlock.
    #[inline]
    pub fn wait_force(&self) {
        if !self.is_completed() {
            self.wait_slow(true);
        }
    }

    /// Performs an initialization routine once and only once. The given closure
    /// will be executed if this is the first time `call_once` has been called,
    /// and otherwise the routine will *not* be invoked.
    ///
    /// This method will block the calling thread if another initialization
    /// routine is currently running.
    ///
    /// When this function returns, the `Once` is in the completed state. Unless
    /// it was created with [`new_completed`](Self::new_completed), some
    /// initialization has run and completed, though it might not be the closure
    /// specified. Any memory writes performed by the executed closure can be
    /// reliably observed after this function returns; there is a happens-before
    /// relation between the closure and code executing after the return.
    ///
    /// If the given closure recursively invokes `call_once` on the same `Once`
    /// instance, the exact behavior is not specified: allowed outcomes are a
    /// panic or a deadlock.
    ///
    /// # Examples
    ///
    /// ```
    /// use parking_lot::Once;
    ///
    /// static mut VAL: usize = 0;
    /// static INIT: Once = Once::new();
    ///
    /// // Accessing a `static mut` is unsafe much of the time, but if we do so
    /// // in a synchronized fashion (e.g. write once or read all) then we're
    /// // good to go!
    /// //
    /// // This function will only call `expensive_computation` once, and will
    /// // otherwise always return the value returned from the first invocation.
    /// fn get_cached_val() -> usize {
    ///     unsafe {
    ///         INIT.call_once(|| {
    ///             VAL = expensive_computation();
    ///         });
    ///         VAL
    ///     }
    /// }
    ///
    /// fn expensive_computation() -> usize {
    ///     // ...
    /// # 2
    /// }
    /// ```
    ///
    /// # Panics
    ///
    /// The closure `f` will only be executed once even if this is called
    /// concurrently amongst many threads. If that closure panics, however, then
    /// it will *poison* this `Once` instance, causing all future invocations of
    /// `call_once` to also panic.
    #[inline]
    pub fn call_once<F>(&self, f: F)
    where
        F: FnOnce(),
    {
        if self.0.load(Ordering::Acquire) == DONE_BIT {
            return;
        }

        let mut f = Some(f);
        self.call_once_slow(false, &mut |_| {
            let f = f.take();
            unsafe { f.unwrap_unchecked()() }
        });
    }

    /// Performs the same function as [`call_once`](Self::call_once) except it
    /// ignores poisoning.
    ///
    /// Unlike [`call_once`](Self::call_once), if this `Once` has been poisoned
    /// by a previous initialization panic, this function will still invoke the
    /// closure `f` instead of immediately panicking. If `f` panics, the `Once`
    /// remains poisoned. If `f` does not panic, the `Once` is no longer
    /// poisoned and all future calls to `call_once` or `call_once_force` are
    /// no-ops.
    ///
    /// The closure `f` is passed a [`OnceState`] which can be used to query the
    /// poison status of this `Once`.
    ///
    /// If the given closure recursively invokes `call_once` or
    /// `call_once_force` on the same `Once` instance, the exact behavior is not
    /// specified: allowed outcomes are a panic or a deadlock.
    ///
    /// # Examples
    ///
    /// ```
    /// use parking_lot::Once;
    /// use std::thread;
    ///
    /// static INIT: Once = Once::new();
    ///
    /// // Poison the once.
    /// let handle = thread::spawn(|| {
    ///     INIT.call_once(|| panic!());
    /// });
    /// assert!(handle.join().is_err());
    ///
    /// // Poisoning propagates.
    /// let handle = thread::spawn(|| {
    ///     INIT.call_once(|| {});
    /// });
    /// assert!(handle.join().is_err());
    ///
    /// // call_once_force still runs and clears the poisoned state.
    /// INIT.call_once_force(|state| {
    ///     assert!(state.is_poisoned());
    /// });
    ///
    /// // Once initialization succeeds, future calls are no-ops.
    /// INIT.call_once(|| {});
    /// ```
    #[inline]
    pub fn call_once_force<F>(&self, f: F)
    where
        F: FnOnce(OnceState),
    {
        if self.0.load(Ordering::Acquire) == DONE_BIT {
            return;
        }

        let mut f = Some(f);
        self.call_once_slow(true, &mut |state| {
            let f = f.take();
            unsafe { f.unwrap_unchecked()(state) }
        });
    }

    #[cold]
    fn wait_slow(&self, ignore_poison: bool) {
        let mut spinwait = SpinWait::new();
        let mut state = self.0.load(Ordering::Relaxed);
        loop {
            if state & DONE_BIT != 0 {
                // Synchronize with the initialization routine.
                fence(Ordering::Acquire);
                return;
            }

            if state & POISON_BIT != 0 && !ignore_poison {
                fence(Ordering::Acquire);
                panic!("Once instance has previously been poisoned");
            }

            // Only spin while an initialization routine is actively running.
            if state & LOCKED_BIT != 0 && state & PARKED_BIT == 0 && spinwait.spin() {
                state = self.0.load(Ordering::Relaxed);
                continue;
            }

            // Register that a thread may be parked. `try_update` rechecks the
            // terminal states on every retry so we cannot miss completion or
            // poisoning while setting PARKED_BIT.
            match self
                .0
                .try_update(Ordering::Relaxed, Ordering::Relaxed, |state| {
                    if state & DONE_BIT != 0 || (state & POISON_BIT != 0 && !ignore_poison) {
                        None
                    } else {
                        Some(state | PARKED_BIT)
                    }
                }) {
                Ok(_) => {}
                Err(x) => {
                    state = x;
                    continue;
                }
            }

            let addr = core::ptr::from_ref(self).addr();
            let validate = || {
                let state = self.0.load(Ordering::Relaxed);
                state & PARKED_BIT != 0
                    && state & DONE_BIT == 0
                    && (ignore_poison || state & POISON_BIT == 0)
            };
            let before_sleep = || {};
            let timed_out = |_, _| unreachable!();
            // SAFETY:
            // * `addr` is an address we control.
            // * `validate` does not panic or call into `parking_lot`.
            // * `before_sleep` does not call `park` or panic.
            // * `timed_out` cannot be called because no timeout is specified.
            unsafe {
                parking_lot_core::park(
                    addr,
                    validate,
                    before_sleep,
                    timed_out,
                    DEFAULT_PARK_TOKEN,
                    None,
                );
            }

            spinwait.reset();
            state = self.0.load(Ordering::Relaxed);
        }
    }

    // This is a non-generic function to reduce the monomorphization cost of
    // using `call_once` (this isn't exactly a trivial or small implementation).
    //
    // Additionally, this is tagged with `#[cold]` as it should indeed be cold
    // and it helps let LLVM know that calls to this function should be off the
    // fast path. Essentially, this should help generate more straight line code
    // in LLVM.
    //
    // Finally, this takes an `FnMut` instead of a `FnOnce` because there's
    // currently no way to take an `FnOnce` and call it via virtual dispatch
    // without some allocation overhead.
    #[cold]
    fn call_once_slow(&self, ignore_poison: bool, f: &mut dyn FnMut(OnceState)) {
        let mut spinwait = SpinWait::new();
        let mut state = self.0.load(Ordering::Relaxed);
        loop {
            // If another thread called the closure, we're done
            if state & DONE_BIT != 0 {
                // An acquire fence is needed here since we didn't load the
                // state with Ordering::Acquire.
                fence(Ordering::Acquire);
                return;
            }

            // If the state has been poisoned and we aren't forcing, then panic
            if state & POISON_BIT != 0 && !ignore_poison {
                // Need the fence here as well for the same reason
                fence(Ordering::Acquire);
                panic!("Once instance has previously been poisoned");
            }

            // Grab the lock if it isn't locked, even if there is a queue on it.
            // We also clear the poison bit since we are going to try running
            // the closure again.
            if state & LOCKED_BIT == 0 {
                match self.0.compare_exchange_weak(
                    state,
                    (state | LOCKED_BIT) & !POISON_BIT,
                    Ordering::Acquire,
                    Ordering::Relaxed,
                ) {
                    Ok(_) => break,
                    Err(x) => state = x,
                }
                continue;
            }

            // If there is no queue, try spinning a few times
            if state & PARKED_BIT == 0 && spinwait.spin() {
                state = self.0.load(Ordering::Relaxed);
                continue;
            }

            // Set the parked bit
            if state & PARKED_BIT == 0
                && let Err(x) = self.0.compare_exchange_weak(
                    state,
                    state | PARKED_BIT,
                    Ordering::Relaxed,
                    Ordering::Relaxed,
                )
            {
                state = x;
                continue;
            }

            // Park our thread until we are woken up by the thread that owns the
            // lock.
            let addr = core::ptr::from_ref(self).addr();
            let validate = || self.0.load(Ordering::Relaxed) == LOCKED_BIT | PARKED_BIT;
            let before_sleep = || {};
            let timed_out = |_, _| unreachable!();
            unsafe {
                parking_lot_core::park(
                    addr,
                    validate,
                    before_sleep,
                    timed_out,
                    DEFAULT_PARK_TOKEN,
                    None,
                );
            }

            // Loop back and check if the done bit was set
            spinwait.reset();
            state = self.0.load(Ordering::Relaxed);
        }

        struct PanicGuard<'a>(&'a Once);
        impl<'a> Drop for PanicGuard<'a> {
            fn drop(&mut self) {
                // Stop recording ownership, mark the state as poisoned,
                // unlock it and unpark all threads.
                let once = self.0;
                unsafe { deadlock::release_resource(core::ptr::from_ref(once).addr()) };
                let state = once.0.swap(POISON_BIT, Ordering::Release);
                if state & PARKED_BIT != 0 {
                    let addr = core::ptr::from_ref(once).addr();
                    unsafe {
                        parking_lot_core::unpark_all(addr, DEFAULT_UNPARK_TOKEN);
                    }
                }
            }
        }

        // At this point we have the lock, so record its ownership and run the
        // closure. Make sure we properly clean up if the closure panics.
        unsafe { deadlock::acquire_resource(core::ptr::from_ref(self).addr()) };
        let guard = PanicGuard(self);
        let once_state = if state & POISON_BIT != 0 {
            OnceState::Poisoned
        } else {
            OnceState::New
        };
        f(once_state);
        unsafe { deadlock::release_resource(core::ptr::from_ref(self).addr()) };
        mem::forget(guard);

        // Now unlock the state, set the done bit and unpark all threads
        let state = self.0.swap(DONE_BIT, Ordering::Release);
        if state & PARKED_BIT != 0 {
            let addr = core::ptr::from_ref(self).addr();
            unsafe {
                parking_lot_core::unpark_all(addr, DEFAULT_UNPARK_TOKEN);
            }
        }
    }
}

impl Default for Once {
    #[inline]
    fn default() -> Once {
        Once::new()
    }
}

impl fmt::Debug for Once {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Once")
            .field("state", &self.state())
            .finish()
    }
}

#[cfg(test)]
mod tests {
    use super::PARKED_BIT;
    use crate::Once;
    use std::panic;
    use std::sync::Arc;
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::sync::mpsc::channel;
    use std::thread;

    #[test]
    fn smoke_once() {
        static O: Once = Once::new();
        let mut a = 0;
        O.call_once(|| a += 1);
        assert_eq!(a, 1);
        O.call_once(|| a += 1);
        assert_eq!(a, 1);
    }

    #[test]
    fn wait_for_initialization() {
        let once = Arc::new(Once::new());
        let value = Arc::new(AtomicUsize::new(0));
        let waiter = {
            let once = Arc::clone(&once);
            let value = Arc::clone(&value);
            thread::spawn(move || {
                once.wait();
                assert_eq!(value.load(Ordering::Relaxed), 1);
            })
        };

        while once.0.load(Ordering::Relaxed) & PARKED_BIT == 0 {
            thread::yield_now();
        }
        once.call_once(|| value.store(1, Ordering::Relaxed));
        waiter.join().unwrap();
        assert!(once.is_completed());
    }

    #[test]
    fn wait_propagates_poison() {
        let once = Once::new();
        assert!(panic::catch_unwind(|| once.call_once(|| panic!())).is_err());
        assert!(panic::catch_unwind(|| once.wait()).is_err());
    }

    #[test]
    fn wait_force_ignores_poison() {
        let once = Arc::new(Once::new());
        assert!(panic::catch_unwind(|| once.call_once(|| panic!())).is_err());

        let waiter = {
            let once = Arc::clone(&once);
            thread::spawn(move || once.wait_force())
        };
        while once.0.load(Ordering::Relaxed) & PARKED_BIT == 0 {
            thread::yield_now();
        }
        once.call_once_force(|state| assert!(state.is_poisoned()));
        waiter.join().unwrap();
        assert!(once.is_completed());
    }

    #[test]
    fn stampede_once() {
        static O: Once = Once::new();
        static mut RUN: bool = false;

        let (tx, rx) = channel();
        for _ in 0..10 {
            let tx = tx.clone();
            thread::spawn(move || {
                for _ in 0..4 {
                    thread::yield_now()
                }
                unsafe {
                    O.call_once(|| {
                        assert!(!RUN);
                        RUN = true;
                    });
                    assert!(RUN);
                }
                tx.send(()).unwrap();
            });
        }

        unsafe {
            O.call_once(|| {
                assert!(!RUN);
                RUN = true;
            });
            assert!(RUN);
        }

        for _ in 0..10 {
            rx.recv().unwrap();
        }
    }

    #[test]
    fn poison_bad() {
        static O: Once = Once::new();

        // poison the once
        let t = panic::catch_unwind(|| {
            O.call_once(|| panic!());
        });
        assert!(t.is_err());

        // poisoning propagates
        let t = panic::catch_unwind(|| {
            O.call_once(|| {});
        });
        assert!(t.is_err());

        // we can subvert poisoning, however
        let mut called = false;
        O.call_once_force(|p| {
            called = true;
            assert!(p.poisoned())
        });
        assert!(called);

        // once any success happens, we stop propagating the poison
        O.call_once(|| {});
    }

    #[test]
    fn wait_for_force_to_finish() {
        static O: Once = Once::new();

        // poison the once
        let t = panic::catch_unwind(|| {
            O.call_once(|| panic!());
        });
        assert!(t.is_err());

        // make sure someone's waiting inside the once via a force
        let (tx1, rx1) = channel();
        let (tx2, rx2) = channel();
        let t1 = thread::spawn(move || {
            O.call_once_force(|p| {
                assert!(p.poisoned());
                tx1.send(()).unwrap();
                rx2.recv().unwrap();
            });
        });

        rx1.recv().unwrap();

        // put another waiter on the once
        let t2 = thread::spawn(|| {
            let mut called = false;
            O.call_once(|| {
                called = true;
            });
            assert!(!called);
        });

        tx2.send(()).unwrap();

        assert!(t1.join().is_ok());
        assert!(t2.join().is_ok());
    }

    #[test]
    fn test_once_debug() {
        static O: Once = Once::new();

        assert_eq!(format!("{:?}", O), "Once { state: New }");
    }

    #[test]
    fn new_completed_is_done() {
        use crate::OnceState;

        static O: Once = Once::new_completed();
        assert_eq!(O.state(), OnceState::Done);
        assert!(O.state().done());
        assert!(!O.state().poisoned());

        let mut called = false;
        O.call_once(|| called = true);
        assert!(!called);
        assert_eq!(O.state(), OnceState::Done);
    }

    #[test]
    fn new_default_is_new() {
        // Sanity check that the default constructor is unchanged.
        static O: Once = Once::new();
        let mut called = false;
        O.call_once(|| called = true);
        assert!(called);
    }
}
