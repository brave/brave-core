use crate::mutex::{MutexGuard, RawMutex, RawMutexTimed};
use core::{fmt, ops::DerefMut};

/// Basic operations for a condition variable associated with a particular
/// [`RawMutex`] implementation.
///
/// Types implementing this trait can be used by [`Condvar`] to form a safe
/// condition variable type.
///
/// Implementations must document whether they guarantee no spurious wakeups. If
/// they do, notification methods report exactly how many threads were notified.
/// Otherwise, notification methods must return `false` or `0`, and all
/// non-timeout wakeups are treated as potentially spurious. In either case,
/// notification methods must still wake waiting threads as specified.
///
/// # Safety
///
/// [`wait`](RawCondvar::wait) must atomically unlock the mutex and begin
/// waiting with respect to calls to [`notify_one`](RawCondvar::notify_one) and
/// [`notify_all`](RawCondvar::notify_all), then re-lock the mutex before it
/// returns.
///
/// If a wait panics after unlocking the mutex, it must re-lock the mutex before
/// unwinding, and replace `guard` with the state of the new acquisition.
/// `guard` must remain valid on both normal return and unwind. If restoring
/// ownership or valid guard state fails, the implementation must abort.
/// Implementations which cannot wait on distinct mutexes simultaneously may
/// panic when asked to do so, but must not cause undefined behavior.
pub unsafe trait RawCondvar {
    /// Initial value for a new condvar.
    // A “non-constant” const item is a legacy way to supply an initialized value to downstream
    // static items. Can hopefully be replaced with `const fn new() -> Self` at some point.
    #[allow(clippy::declare_interior_mutable_const)]
    const INIT: Self;

    /// The type of [`RawMutex`] this condvar can work with.
    type RawMutex: RawMutex;

    /// Atomically unlocks the mutex and waits, then re-locks the mutex before
    /// returning.
    ///
    /// This method may return spuriously. See the implementation's
    /// documentation for any stronger guarantees.
    ///
    /// # Safety
    ///
    /// The caller must logically hold `mutex`, with `guard` containing the
    /// state of that acquisition. The protected data must not be accessed from
    /// the time this method unlocks the mutex until it re-locks it.
    unsafe fn wait(&self, mutex: &Self::RawMutex, guard: &mut <Self::RawMutex as RawMutex>::Guard);

    /// Notifies one waiting thread.
    ///
    /// If the implementation guarantees no spurious wakeups, returns whether
    /// this call notified a waiting thread. Otherwise, must return `false`.
    /// Threads whose waits end because of a timeout are not counted.
    fn notify_one(&self) -> bool;

    /// Notifies all waiting threads.
    ///
    /// If the implementation guarantees no spurious wakeups, returns the exact
    /// number of waiting threads this call notified. Otherwise, must return `0`.
    /// Threads whose waits end because of a timeout are not counted.
    fn notify_all(&self) -> usize;
}

/// Additional methods for condition variables which support timeouts.
///
/// # Safety
///
/// Implementations must uphold the safety requirements of [`RawCondvar`] for
/// the additional methods provided by this trait.
pub unsafe trait RawCondvarTimed: RawCondvar
where
    Self::RawMutex: RawMutexTimed,
{
    /// Converts a relative timeout into an absolute timeout.
    ///
    /// Returns `None` if the resulting instant cannot be represented.
    fn checked_duration_to_instant(
        timeout: &<Self::RawMutex as RawMutexTimed>::Duration,
    ) -> Option<<Self::RawMutex as RawMutexTimed>::Instant>;

    /// Atomically unlocks the mutex and waits for a notification, a spurious
    /// wakeup, or the timeout to be reached, then re-locks the mutex before
    /// returning.
    ///
    /// Returns `true` if the wait is known to have timed out and `false`
    /// otherwise. See the [`RawCondvar`] implementation's documentation for
    /// whether a `false` result guarantees that a notification was received.
    ///
    /// A timed-out wait must not return before `timeout`, but may return later
    /// due to scheduling or platform-specific behavior.
    ///
    /// # Safety
    ///
    /// The caller must uphold the safety requirements of
    /// [`RawCondvar::wait`].
    unsafe fn wait_until(
        &self,
        mutex: &Self::RawMutex,
        guard: &mut <Self::RawMutex as RawMutex>::Guard,
        timeout: &<Self::RawMutex as RawMutexTimed>::Instant,
    ) -> bool;

    /// Atomically unlocks the mutex and waits for a notification, a spurious
    /// wakeup, or the timeout to be reached, then re-locks the mutex before
    /// returning.
    ///
    /// Returns `true` if the wait is known to have timed out and `false`
    /// otherwise. See the [`RawCondvar`] implementation's documentation for
    /// whether a `false` result guarantees that a notification was received.
    ///
    /// A timed-out wait must not return before `timeout` has elapsed, but may
    /// return later due to scheduling or platform-specific behavior.
    ///
    /// # Safety
    ///
    /// The caller must uphold the safety requirements of
    /// [`RawCondvar::wait`].
    unsafe fn wait_for(
        &self,
        mutex: &Self::RawMutex,
        guard: &mut <Self::RawMutex as RawMutex>::Guard,
        timeout: &<Self::RawMutex as RawMutexTimed>::Duration,
    ) -> bool {
        // SAFETY: `RawCondvar::wait` and `RawCondvarTimed::wait_until` have the
        // same safety condition as this function, which is assured by the caller.
        unsafe {
            match Self::checked_duration_to_instant(timeout) {
                Some(timeout) => self.wait_until(mutex, guard, &timeout),
                None => {
                    // No absolute deadline can be constructed, so fall back to
                    // an untimed wait.
                    <Self as RawCondvar>::wait(self, mutex, guard);
                    false
                }
            }
        }
    }
}

/// A type indicating whether a timed wait on a condition variable timed out.
#[derive(Debug, PartialEq, Eq, Copy, Clone)]
pub struct WaitTimeoutResult(bool);

impl WaitTimeoutResult {
    /// Returns `true` if the wait was known to have timed out.
    #[inline]
    pub const fn timed_out(self) -> bool {
        self.0
    }
}

/// A condition variable.
///
/// Condition variables represent the ability to block a thread such that it
/// consumes no CPU time while waiting for an event to occur. Condition
/// variables are typically associated with a boolean predicate (a condition)
/// and a mutex. The predicate is always verified inside of the mutex before
/// determining that thread must block.
///
/// If the underlying [`RawCondvar`] guarantees no spurious wakeups, notification
/// return values are exact. Otherwise, they are always `false` or `0`. Consult
/// the documentation of `C` for whether it provides this guarantee.
pub struct Condvar<C> {
    inner: C,
}

impl<C: RawCondvar> Condvar<C> {
    /// Creates a new condition variable which is ready to be waited on and
    /// notified.
    #[inline]
    pub const fn new() -> Condvar<C> {
        Condvar { inner: C::INIT }
    }

    /// Returns the underlying raw condvar object.
    ///
    /// Note that you will most likely need to import the `RawCondvar` trait from
    /// `lock_api` to be able to call functions on the raw condvar.
    #[inline]
    pub fn raw(&self) -> &C {
        &self.inner
    }

    /// Wakes up one blocked thread on this condvar.
    ///
    /// If `C` guarantees no spurious wakeups, returns whether this call notified
    /// a waiting thread. Otherwise, always returns `false`. Threads whose waits
    /// end because of a timeout are not counted. Consult the documentation of
    /// `C` for whether it provides this guarantee.
    ///
    /// If there is a blocked thread on this condition variable, then it will
    /// be woken up from its call to [`wait`](Self::wait),
    /// [`wait_for`](Self::wait_for), or [`wait_until`](Self::wait_until).
    /// Calls to `notify_one` are not buffered in any way.
    ///
    /// To wake up all threads, see [`notify_all`](Self::notify_all).
    #[inline]
    pub fn notify_one(&self) -> bool {
        self.inner.notify_one()
    }

    /// Wakes up all blocked threads on this condvar.
    ///
    /// If `C` guarantees no spurious wakeups, returns the exact number of
    /// waiting threads this call notified. Otherwise, always returns `0`.
    /// Threads whose waits end because of a timeout are not counted. Consult
    /// the documentation of `C` for whether it provides this guarantee.
    ///
    /// This method will ensure that any current waiters on the condition
    /// variable are awoken. Calls to `notify_all` are not buffered in any way.
    ///
    /// To wake up only one thread, see [`notify_one`](Self::notify_one).
    #[inline]
    pub fn notify_all(&self) -> usize {
        self.inner.notify_all()
    }

    /// Blocks the current thread until this condition variable receives a
    /// notification.
    ///
    /// This function will atomically unlock the mutex specified (represented by
    /// `mutex_guard`) and block the current thread. This means that calls to
    /// [`notify_one`](Self::notify_one) or [`notify_all`](Self::notify_all)
    /// which happen logically after the mutex is unlocked are candidates to
    /// wake this thread. When this function returns, the lock will have been
    /// re-acquired.
    ///
    /// This function may return spuriously depending on the underlying
    /// [`RawCondvar`]. Consult the documentation of `C` for its guarantees.
    ///
    /// # Panics
    ///
    /// The underlying [`RawCondvar`] implementation may panic if another thread
    /// is waiting on this condition variable with a different mutex.
    #[inline]
    pub fn wait<T: ?Sized>(&self, mutex_guard: &mut MutexGuard<'_, C::RawMutex, T>) {
        // SAFETY: The exclusively borrowed guard holds the mutex; wait restores it.
        unsafe {
            self.inner
                .wait(MutexGuard::mutex(mutex_guard).raw(), &mut mutex_guard.guard);
        }
    }

    /// Blocks the current thread until the provided condition becomes false.
    ///
    /// `condition` is checked immediately. If it returns `true`, this function
    /// waits and checks the condition again whenever the wait returns. This
    /// repeats until `condition` returns `false`.
    ///
    /// This function will atomically unlock the mutex specified (represented by
    /// `mutex_guard`) and block the current thread. This means that any calls
    /// to `notify_*()` which happen logically after the mutex is unlocked are
    /// candidates to wake this thread up. When this function call returns, the
    /// lock specified will have been re-acquired.
    ///
    /// # Panics
    ///
    /// The underlying [`RawCondvar`] implementation may panic if another thread
    /// is waiting on this condition variable with a different mutex.
    #[inline]
    pub fn wait_while<T, F>(
        &self,
        mutex_guard: &mut MutexGuard<'_, C::RawMutex, T>,
        mut condition: F,
    ) where
        T: ?Sized,
        F: FnMut(&mut T) -> bool,
    {
        while condition(mutex_guard.deref_mut()) {
            // SAFETY: The exclusively borrowed guard holds the mutex; wait restores it.
            unsafe {
                self.inner
                    .wait(MutexGuard::mutex(mutex_guard).raw(), &mut mutex_guard.guard);
            }
        }
    }
}

impl<R: RawMutexTimed, C: RawCondvarTimed<RawMutex = R>> Condvar<C> {
    /// Waits on this condition variable for a notification, timing out after
    /// the specified time instant.
    ///
    /// The semantics of this function are equivalent to [`wait`](Self::wait),
    /// except that it stops waiting after `timeout` is reached. A notification
    /// or spurious wakeup may make the function return earlier. If the
    /// operation times out, it will not return before `timeout`, but it may
    /// return later because of scheduling or platform-specific behavior.
    ///
    /// Note that the best effort is made to ensure that the time waited is
    /// measured with a monotonic clock, and not affected by the changes made to
    /// the system time.
    ///
    /// The returned [`WaitTimeoutResult`] indicates whether the wait is known
    /// to have timed out. Consult the documentation of `C` for whether a result
    /// which is not timed out guarantees that a notification was received.
    ///
    /// Like [`wait`](Self::wait), the lock will be re-acquired before this
    /// function returns, regardless of whether the timeout elapsed.
    ///
    /// # Panics
    ///
    /// The underlying [`RawCondvar`] implementation may panic if another thread
    /// is waiting on this condition variable with a different mutex.
    #[inline]
    pub fn wait_until<T: ?Sized>(
        &self,
        mutex_guard: &mut MutexGuard<'_, C::RawMutex, T>,
        timeout: <C::RawMutex as RawMutexTimed>::Instant,
    ) -> WaitTimeoutResult {
        // SAFETY: The exclusively borrowed guard holds the mutex; wait restores it.
        WaitTimeoutResult(unsafe {
            self.inner.wait_until(
                MutexGuard::mutex(mutex_guard).raw(),
                &mut mutex_guard.guard,
                &timeout,
            )
        })
    }

    /// Waits on this condition variable for a notification, timing out after a
    /// specified duration.
    ///
    /// The semantics of this function are equivalent to [`wait`](Self::wait),
    /// except that it stops waiting after the specified duration. A
    /// notification or spurious wakeup may make the function return earlier.
    /// If the operation times out, it will not return before `timeout` has
    /// elapsed, but it may return later because of scheduling or
    /// platform-specific behavior.
    ///
    /// Note that the best effort is made to ensure that the time waited is
    /// measured with a monotonic clock, and not affected by the changes made to
    /// the system time.
    ///
    /// The returned [`WaitTimeoutResult`] indicates whether the wait is known
    /// to have timed out. Consult the documentation of `C` for whether a result
    /// which is not timed out guarantees that a notification was received.
    ///
    /// Like [`wait`](Self::wait), the lock will be re-acquired before this
    /// function returns, regardless of whether the timeout elapsed.
    ///
    /// # Panics
    ///
    /// The underlying [`RawCondvar`] implementation may panic if another thread
    /// is waiting on this condition variable with a different mutex.
    #[inline]
    pub fn wait_for<T: ?Sized>(
        &self,
        mutex_guard: &mut MutexGuard<'_, C::RawMutex, T>,
        timeout: <C::RawMutex as RawMutexTimed>::Duration,
    ) -> WaitTimeoutResult {
        // SAFETY: The exclusively borrowed guard holds the mutex; wait restores it.
        WaitTimeoutResult(unsafe {
            self.inner.wait_for(
                MutexGuard::mutex(mutex_guard).raw(),
                &mut mutex_guard.guard,
                &timeout,
            )
        })
    }

    /// Waits on this condition variable for the provided condition to become
    /// false, timing out after the specified time instant.
    ///
    /// The semantics of this function are equivalent to
    /// [`wait_while`](Self::wait_while), except that it stops waiting after
    /// `timeout` is reached. If the operation times out, it will not return
    /// before `timeout`, but it may return later because of scheduling or
    /// platform-specific behavior.
    ///
    /// Note that the best effort is made to ensure that the time waited is
    /// measured with a monotonic clock, and not affected by the changes made to
    /// the system time.
    ///
    /// The returned [`WaitTimeoutResult`] indicates whether the wait timed out
    /// while the condition remained `true`.
    ///
    /// Like [`wait_while`](Self::wait_while), the lock will be re-acquired
    /// before this function returns, regardless of whether the timeout elapsed.
    ///
    /// # Panics
    ///
    /// The underlying [`RawCondvar`] implementation may panic if another thread
    /// is waiting on this condition variable with a different mutex.
    #[inline]
    pub fn wait_while_until<T, F>(
        &self,
        mutex_guard: &mut MutexGuard<'_, C::RawMutex, T>,
        mut condition: F,
        timeout: <C::RawMutex as RawMutexTimed>::Instant,
    ) -> WaitTimeoutResult
    where
        T: ?Sized,
        F: FnMut(&mut T) -> bool,
    {
        let mut result = WaitTimeoutResult(false);

        loop {
            if !condition(mutex_guard.deref_mut()) {
                return WaitTimeoutResult(false);
            }
            if result.timed_out() {
                return result;
            }
            // SAFETY: The exclusively borrowed guard holds the mutex; wait restores it.
            result = WaitTimeoutResult(unsafe {
                self.inner.wait_until(
                    MutexGuard::mutex(mutex_guard).raw(),
                    &mut mutex_guard.guard,
                    &timeout,
                )
            });
        }
    }

    /// Waits on this condition variable for the provided condition to become
    /// false, timing out after a specified duration.
    ///
    /// The semantics of this function are equivalent to
    /// [`wait_while`](Self::wait_while), except that it stops waiting after the
    /// specified duration. If the operation times out, it will not return
    /// before `timeout` has elapsed, but it may return later because of
    /// scheduling or platform-specific behavior.
    ///
    /// Note that the best effort is made to ensure that the time waited is
    /// measured with a monotonic clock, and not affected by the changes made to
    /// the system time.
    ///
    /// The returned [`WaitTimeoutResult`] indicates whether the wait timed out
    /// while the condition remained `true`.
    ///
    /// Like [`wait_while`](Self::wait_while), the lock will be re-acquired
    /// before this function returns, regardless of whether the timeout elapsed.
    ///
    /// # Panics
    ///
    /// The underlying [`RawCondvar`] implementation may panic if another thread
    /// is waiting on this condition variable with a different mutex.
    #[inline]
    pub fn wait_while_for<T: ?Sized, F>(
        &self,
        mutex_guard: &mut MutexGuard<'_, C::RawMutex, T>,
        condition: F,
        timeout: <C::RawMutex as RawMutexTimed>::Duration,
    ) -> WaitTimeoutResult
    where
        F: FnMut(&mut T) -> bool,
    {
        match C::checked_duration_to_instant(&timeout) {
            Some(timeout) => self.wait_while_until(mutex_guard, condition, timeout),
            None => {
                // No absolute deadline can be constructed, so fall back to an
                // untimed wait.
                self.wait_while(mutex_guard, condition);
                WaitTimeoutResult(false)
            }
        }
    }
}

impl<C: RawCondvar> Default for Condvar<C> {
    #[inline]
    fn default() -> Condvar<C> {
        Condvar::new()
    }
}

impl<C> fmt::Debug for Condvar<C> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.pad("Condvar { .. }")
    }
}
