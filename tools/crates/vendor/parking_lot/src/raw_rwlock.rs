use crate::{
    deadlock,
    raw_mutex::{TOKEN_HANDOFF, TOKEN_NORMAL},
    util,
};
use core::{
    cell::Cell,
    sync::atomic::{AtomicUsize, Ordering},
};
use lock_api::{RawRwLock as RawRwLock_, RawRwLockUpgrade};
use parking_lot_core::{
    self, FilterOp, ParkResult, ParkToken, SpinWait, UnparkResult, UnparkToken,
};
use std::time::{Duration, Instant};

// This reader-writer lock implementation is based on Boost's upgrade_mutex:
// https://github.com/boostorg/thread/blob/fc08c1fe2840baeeee143440fba31ef9e9a813c8/include/boost/thread/v2/shared_mutex.hpp#L432
//
// This implementation uses 2 wait queues. The primary queue at key [addr] is
// used for all new waiting threads. The secondary queue at key [addr + 1] is
// used by the thread which has acquired WRITER_BIT but is waiting for the
// remaining readers to exit the lock.
//
// In nonrecursive mode, readers block behind a pending writer. Recursive mode
// admits new readers while other readers hold the lock, which can starve
// writers.

// There is at least one thread in the main queue.
const PARKED_BIT: usize = 0b0001;
// There is a parked thread holding WRITER_BIT. WRITER_BIT must be set.
const WRITER_PARKED_BIT: usize = 0b0010;
// A reader is holding an upgradable lock. The reader count must be non-zero and
// WRITER_BIT must not be set.
const UPGRADABLE_BIT: usize = 0b0100;
// If the reader count is zero: a writer is currently holding an exclusive lock.
// Otherwise: a writer is waiting for the remaining readers to exit the lock.
const WRITER_BIT: usize = 0b1000;
// Mask of bits used to count readers.
const READERS_MASK: usize = !0b1111;
// Base unit for counting readers.
const ONE_READER: usize = 0b10000;

// Token indicating what type of lock a queued thread is trying to acquire
const TOKEN_SHARED: ParkToken = ParkToken(ONE_READER);
const TOKEN_EXCLUSIVE: ParkToken = ParkToken(WRITER_BIT);
const TOKEN_UPGRADABLE: ParkToken = ParkToken(ONE_READER | UPGRADABLE_BIT);

/// Raw reader-writer lock type backed by the parking lot.
///
/// If `RECURSIVE` is `true`, readers are prioritized over writers: acquiring a
/// read lock is guaranteed not to block unless a writer currently holds the
/// lock. This permits recursive read locking, but may starve writers.
pub struct RawRwLock<const RECURSIVE: bool = false> {
    state: AtomicUsize,
}

/// Raw reader-writer lock which permits recursive read locking.
pub type RawRwLockRecursive = RawRwLock<true>;

unsafe impl<const RECURSIVE: bool> lock_api::RawRwLock for RawRwLock<RECURSIVE> {
    const INIT: Self = Self {
        state: AtomicUsize::new(0),
    };

    type SharedGuard = crate::GuardMarker;
    type ExclusiveGuard = crate::GuardMarker;

    #[inline]
    fn lock_exclusive(&self) -> Self::ExclusiveGuard {
        if self
            .state
            .compare_exchange_weak(0, WRITER_BIT, Ordering::Acquire, Ordering::Relaxed)
            .is_ok()
        {
            self.deadlock_acquire_all();
            Self::ExclusiveGuard::new()
        } else {
            self.lock_exclusive_slow(None).unwrap()
        }
    }

    #[inline]
    fn try_lock_exclusive(&self) -> Option<Self::ExclusiveGuard> {
        self.state
            .compare_exchange(0, WRITER_BIT, Ordering::Acquire, Ordering::Relaxed)
            .ok()?;
        self.deadlock_acquire_all();
        Some(Self::ExclusiveGuard::new())
    }

    #[inline]
    unsafe fn unlock_exclusive(&self, _guard: Self::ExclusiveGuard) {
        self.deadlock_release_all();
        let state = self.state.fetch_sub(WRITER_BIT, Ordering::Release);
        if state & PARKED_BIT != 0 {
            self.unlock_exclusive_or_upgradable_slow();
        }
    }

    #[inline]
    fn lock_shared(&self) -> Self::SharedGuard {
        let guard = self
            .try_lock_shared_fast()
            .unwrap_or_else(|| self.lock_shared_slow(None).unwrap());
        self.deadlock_acquire_secondary();
        guard
    }

    #[inline]
    fn try_lock_shared(&self) -> Option<Self::SharedGuard> {
        let guard = self
            .try_lock_shared_fast()
            .or_else(|| self.try_lock_shared_slow())?;
        self.deadlock_acquire_secondary();
        Some(guard)
    }

    #[inline]
    unsafe fn unlock_shared(&self, _guard: Self::SharedGuard) {
        self.deadlock_release_secondary();
        let state = self.state.fetch_sub(ONE_READER, Ordering::Release);
        if state & (READERS_MASK | WRITER_PARKED_BIT) == (ONE_READER | WRITER_PARKED_BIT) {
            self.unlock_shared_slow();
        }
    }

    #[inline]
    fn is_locked(&self) -> bool {
        let state = self.state.load(Ordering::Relaxed);
        state & (WRITER_BIT | READERS_MASK) != 0
    }

    #[inline]
    fn is_locked_exclusive(&self) -> bool {
        let state = self.state.load(Ordering::Relaxed);
        state & (WRITER_BIT | READERS_MASK) == WRITER_BIT
    }
}

unsafe impl<const RECURSIVE: bool> lock_api::RawRwLockFair for RawRwLock<RECURSIVE> {
    #[inline]
    unsafe fn unlock_shared_fair(&self, guard: Self::SharedGuard) {
        // Shared unlocking is always fair in this implementation.
        unsafe { self.unlock_shared(guard) };
    }

    #[inline]
    unsafe fn unlock_exclusive_fair(&self, _guard: Self::ExclusiveGuard) {
        self.deadlock_release_all();
        if self
            .state
            .compare_exchange(WRITER_BIT, 0, Ordering::Release, Ordering::Relaxed)
            .is_ok()
        {
            return;
        }
        self.unlock_exclusive_fair_slow();
    }

    #[inline]
    unsafe fn bump_shared(&self, _guard: &mut Self::SharedGuard) {
        let state = self.state.load(Ordering::Relaxed);
        if state & WRITER_BIT != 0 && (!RECURSIVE || state & READERS_MASK == ONE_READER) {
            unsafe { self.bump_shared_slow() };
        }
    }

    #[inline]
    unsafe fn bump_exclusive(&self, _guard: &mut Self::ExclusiveGuard) {
        if self.state.load(Ordering::Relaxed) & PARKED_BIT != 0 {
            self.bump_exclusive_slow();
        }
    }
}

unsafe impl<const RECURSIVE: bool> lock_api::RawRwLockDowngrade for RawRwLock<RECURSIVE> {
    #[inline]
    unsafe fn downgrade(&self, guard: Self::ExclusiveGuard) -> Self::SharedGuard {
        self.deadlock_release_primary();
        let state = self
            .state
            .fetch_add(ONE_READER - WRITER_BIT, Ordering::Release);

        // Wake up parked shared and upgradable threads if there are any
        if state & PARKED_BIT != 0 {
            self.downgrade_slow();
        }
        guard
    }
}

unsafe impl<const RECURSIVE: bool> lock_api::RawRwLockTimed for RawRwLock<RECURSIVE> {
    type Duration = Duration;
    type Instant = Instant;

    #[inline]
    fn try_lock_shared_for(&self, timeout: Self::Duration) -> Option<Self::SharedGuard> {
        let guard = self
            .try_lock_shared_fast()
            .or_else(|| self.lock_shared_slow(util::to_deadline(timeout)))?;
        self.deadlock_acquire_secondary();
        Some(guard)
    }

    #[inline]
    fn try_lock_shared_until(&self, timeout: Self::Instant) -> Option<Self::SharedGuard> {
        let guard = self
            .try_lock_shared_fast()
            .or_else(|| self.lock_shared_slow(Some(timeout)))?;
        self.deadlock_acquire_secondary();
        Some(guard)
    }

    #[inline]
    fn try_lock_exclusive_for(&self, timeout: Duration) -> Option<Self::ExclusiveGuard> {
        if self
            .state
            .compare_exchange_weak(0, WRITER_BIT, Ordering::Acquire, Ordering::Relaxed)
            .is_ok()
        {
            self.deadlock_acquire_all();
            Some(Self::ExclusiveGuard::new())
        } else {
            self.lock_exclusive_slow(util::to_deadline(timeout))
        }
    }

    #[inline]
    fn try_lock_exclusive_until(&self, timeout: Instant) -> Option<Self::ExclusiveGuard> {
        if self
            .state
            .compare_exchange_weak(0, WRITER_BIT, Ordering::Acquire, Ordering::Relaxed)
            .is_ok()
        {
            self.deadlock_acquire_all();
            Some(Self::ExclusiveGuard::new())
        } else {
            self.lock_exclusive_slow(Some(timeout))
        }
    }
}

unsafe impl<const RECURSIVE: bool> lock_api::RawRwLockUpgrade for RawRwLock<RECURSIVE> {
    type UpgradableGuard = crate::GuardMarker;
    #[inline]
    fn lock_upgradable(&self) -> Self::UpgradableGuard {
        let guard = self
            .try_lock_upgradable_fast()
            .unwrap_or_else(|| self.lock_upgradable_slow(None).unwrap());
        self.deadlock_acquire_all();
        guard
    }

    #[inline]
    fn try_lock_upgradable(&self) -> Option<Self::UpgradableGuard> {
        let guard = self
            .try_lock_upgradable_fast()
            .or_else(|| self.try_lock_upgradable_slow())?;
        self.deadlock_acquire_all();
        Some(guard)
    }

    #[inline]
    unsafe fn unlock_upgradable(&self, _guard: Self::UpgradableGuard) {
        self.deadlock_release_all();
        let state = self
            .state
            .fetch_sub(ONE_READER | UPGRADABLE_BIT, Ordering::Release);
        if state & PARKED_BIT != 0 {
            self.unlock_exclusive_or_upgradable_slow();
        }
    }

    #[inline]
    unsafe fn upgrade(&self, guard: Self::UpgradableGuard) -> Self::ExclusiveGuard {
        // Stop owning the reader stage before replacing it with WRITER_BIT.
        self.deadlock_release_secondary();
        let state = self.state.fetch_sub(
            (ONE_READER | UPGRADABLE_BIT) - WRITER_BIT,
            Ordering::Acquire,
        );
        if state & READERS_MASK != ONE_READER {
            self.upgrade_slow(guard, None)
                .unwrap_or_else(|_| unreachable!())
        } else {
            self.deadlock_acquire_secondary();
            guard
        }
    }

    #[inline]
    unsafe fn try_upgrade(
        &self,
        guard: Self::UpgradableGuard,
    ) -> Result<Self::ExclusiveGuard, Self::UpgradableGuard> {
        if self
            .state
            .compare_exchange_weak(
                ONE_READER | UPGRADABLE_BIT,
                WRITER_BIT,
                Ordering::Acquire,
                Ordering::Relaxed,
            )
            .is_ok()
        {
            Ok(guard)
        } else {
            self.try_upgrade_slow(guard)
        }
    }
}

unsafe impl<const RECURSIVE: bool> lock_api::RawRwLockUpgradeFair for RawRwLock<RECURSIVE> {
    #[inline]
    unsafe fn unlock_upgradable_fair(&self, _guard: Self::UpgradableGuard) {
        self.deadlock_release_all();
        let state = self.state.load(Ordering::Relaxed);
        #[allow(clippy::collapsible_if)]
        if state & PARKED_BIT == 0 {
            if self
                .state
                .compare_exchange_weak(
                    state,
                    state - (ONE_READER | UPGRADABLE_BIT),
                    Ordering::Release,
                    Ordering::Relaxed,
                )
                .is_ok()
            {
                return;
            }
        }
        self.unlock_upgradable_fair_slow();
    }

    #[inline]
    unsafe fn bump_upgradable(&self, _guard: &mut Self::UpgradableGuard) {
        if self.state.load(Ordering::Relaxed) & PARKED_BIT != 0 {
            self.bump_upgradable_slow();
        }
    }
}

unsafe impl<const RECURSIVE: bool> lock_api::RawRwLockUpgradeDowngrade for RawRwLock<RECURSIVE> {
    #[inline]
    unsafe fn downgrade_upgradable(&self, guard: Self::UpgradableGuard) -> Self::SharedGuard {
        self.deadlock_release_primary();
        let state = self.state.fetch_sub(UPGRADABLE_BIT, Ordering::Relaxed);

        // Wake up parked upgradable threads if there are any
        if state & PARKED_BIT != 0 {
            self.downgrade_slow();
        }
        guard
    }

    #[inline]
    unsafe fn downgrade_to_upgradable(&self, guard: Self::ExclusiveGuard) -> Self::UpgradableGuard {
        let state = self.state.fetch_add(
            (ONE_READER | UPGRADABLE_BIT) - WRITER_BIT,
            Ordering::Release,
        );

        // Wake up parked shared threads if there are any
        if state & PARKED_BIT != 0 {
            self.downgrade_to_upgradable_slow();
        }
        guard
    }
}

unsafe impl<const RECURSIVE: bool> lock_api::RawRwLockUpgradeTimed for RawRwLock<RECURSIVE> {
    #[inline]
    fn try_lock_upgradable_until(&self, timeout: Instant) -> Option<Self::UpgradableGuard> {
        let guard = self
            .try_lock_upgradable_fast()
            .or_else(|| self.lock_upgradable_slow(Some(timeout)))?;
        self.deadlock_acquire_all();
        Some(guard)
    }

    #[inline]
    fn try_lock_upgradable_for(&self, timeout: Duration) -> Option<Self::UpgradableGuard> {
        let guard = self
            .try_lock_upgradable_fast()
            .or_else(|| self.lock_upgradable_slow(util::to_deadline(timeout)))?;
        self.deadlock_acquire_all();
        Some(guard)
    }

    #[inline]
    unsafe fn try_upgrade_until(
        &self,
        guard: Self::UpgradableGuard,
        timeout: Instant,
    ) -> Result<Self::ExclusiveGuard, Self::UpgradableGuard> {
        // Stop owning the reader stage before replacing it with WRITER_BIT.
        self.deadlock_release_secondary();
        let state = self.state.fetch_sub(
            (ONE_READER | UPGRADABLE_BIT) - WRITER_BIT,
            Ordering::Acquire,
        );
        if state & READERS_MASK == ONE_READER {
            self.deadlock_acquire_secondary();
            Ok(guard)
        } else {
            self.upgrade_slow(guard, Some(timeout))
        }
    }

    #[inline]
    unsafe fn try_upgrade_for(
        &self,
        guard: Self::UpgradableGuard,
        timeout: Duration,
    ) -> Result<Self::ExclusiveGuard, Self::UpgradableGuard> {
        // Stop owning the reader stage before replacing it with WRITER_BIT.
        self.deadlock_release_secondary();
        let state = self.state.fetch_sub(
            (ONE_READER | UPGRADABLE_BIT) - WRITER_BIT,
            Ordering::Acquire,
        );
        if state & READERS_MASK == ONE_READER {
            self.deadlock_acquire_secondary();
            Ok(guard)
        } else {
            self.upgrade_slow(guard, util::to_deadline(timeout))
        }
    }
}

impl<const RECURSIVE: bool> RawRwLock<RECURSIVE> {
    #[inline(always)]
    fn try_lock_shared_fast(&self) -> Option<crate::GuardMarker> {
        let state = self.state.load(Ordering::Relaxed);

        // Recursive readers may skip ahead of a pending writer while another
        // reader still holds the lock.
        if state & WRITER_BIT != 0 && (!RECURSIVE || state & READERS_MASK == 0) {
            return None;
        }

        let new_state = state.checked_add(ONE_READER)?;
        self.state
            .compare_exchange_weak(state, new_state, Ordering::Acquire, Ordering::Relaxed)
            .ok()?;
        Some(crate::GuardMarker::new())
    }

    #[cold]
    fn try_lock_shared_slow(&self) -> Option<crate::GuardMarker> {
        self.state
            .try_update(Ordering::Acquire, Ordering::Relaxed, |state| {
                if state & WRITER_BIT != 0 && (!RECURSIVE || state & READERS_MASK == 0) {
                    None
                } else {
                    Some(
                        state
                            .checked_add(ONE_READER)
                            .expect("RwLock reader count overflow"),
                    )
                }
            })
            .ok()?;
        Some(crate::GuardMarker::new())
    }

    #[inline(always)]
    fn try_lock_upgradable_fast(&self) -> Option<crate::GuardMarker> {
        let state = self.state.load(Ordering::Relaxed);

        // We can't grab an upgradable lock if there is already a writer or
        // upgradable reader.
        if state & (WRITER_BIT | UPGRADABLE_BIT) != 0 {
            return None;
        }

        let new_state = state.checked_add(ONE_READER | UPGRADABLE_BIT)?;
        self.state
            .compare_exchange_weak(state, new_state, Ordering::Acquire, Ordering::Relaxed)
            .ok()?;
        Some(crate::GuardMarker::new())
    }

    #[cold]
    fn try_lock_upgradable_slow(&self) -> Option<crate::GuardMarker> {
        self.state
            .try_update(Ordering::Acquire, Ordering::Relaxed, |state| {
                // This mirrors the condition in try_lock_upgradable_fast.
                if state & (WRITER_BIT | UPGRADABLE_BIT) != 0 {
                    None
                } else {
                    Some(
                        state
                            .checked_add(ONE_READER | UPGRADABLE_BIT)
                            .expect("RwLock reader count overflow"),
                    )
                }
            })
            .ok()?;
        Some(crate::GuardMarker::new())
    }

    #[cold]
    fn lock_exclusive_slow(&self, timeout: Option<Instant>) -> Option<crate::GuardMarker> {
        let try_lock = |state: &mut usize| {
            loop {
                if *state & (WRITER_BIT | UPGRADABLE_BIT) != 0 {
                    return false;
                }

                // Grab WRITER_BIT if it isn't set, even if there are parked threads.
                match self.state.compare_exchange_weak(
                    *state,
                    *state | WRITER_BIT,
                    Ordering::Acquire,
                    Ordering::Relaxed,
                ) {
                    Ok(_) => return true,
                    Err(x) => *state = x,
                }
            }
        };

        // Step 1: grab exclusive ownership of WRITER_BIT
        let timed_out = !self.lock_common(timeout, TOKEN_EXCLUSIVE, try_lock, |state| {
            state & (WRITER_BIT | UPGRADABLE_BIT) != 0
        });
        if timed_out {
            return None;
        }

        // WRITER_BIT prevents new writers and upgradable readers from
        // acquiring the lock, so it represents ownership of the primary key.
        self.deadlock_acquire_primary();

        // Step 2: wait for all remaining readers to exit the lock.
        if !self.wait_for_readers(timeout, 0) {
            return None;
        }
        self.deadlock_acquire_secondary();
        Some(crate::GuardMarker::new())
    }

    #[cold]
    fn unlock_exclusive_or_upgradable_slow(&self) {
        let callback = |_, result: UnparkResult| {
            // Ownership has already been released. Preserve any reader count
            // and ownership bits acquired while we were accessing the queue.
            if !result.have_more_threads {
                self.state.fetch_and(!PARKED_BIT, Ordering::Relaxed);
            }
            TOKEN_NORMAL
        };
        // SAFETY: `callback` does not panic or call into any function of `parking_lot`.
        unsafe {
            self.wake_parked_threads(0, callback);
        }
    }

    #[cold]
    fn unlock_exclusive_fair_slow(&self) {
        // There are threads to unpark. Try to unpark as many as we can.
        let callback = |mut new_state, result: UnparkResult| {
            // If we are using a fair unlock then we should keep the
            // rwlock locked and hand it off to the unparked threads.
            if result.unparked_threads != 0 {
                if result.have_more_threads {
                    new_state |= PARKED_BIT;
                }
                self.state.store(new_state, Ordering::Release);
                TOKEN_HANDOFF
            } else {
                // Clear the parked bit if there are no more parked threads.
                if result.have_more_threads {
                    self.state.store(PARKED_BIT, Ordering::Release);
                } else {
                    self.state.store(0, Ordering::Release);
                }
                TOKEN_NORMAL
            }
        };
        // SAFETY: `callback` does not panic or call into any function of `parking_lot`.
        unsafe {
            self.wake_parked_threads(0, callback);
        }
    }

    #[cold]
    fn lock_shared_slow(&self, timeout: Option<Instant>) -> Option<crate::GuardMarker> {
        let try_lock = |state: &mut usize| {
            let mut spinwait_shared = SpinWait::new();
            loop {
                if *state & WRITER_BIT != 0 && (!RECURSIVE || *state & READERS_MASK == 0) {
                    return false;
                }

                if self
                    .state
                    .compare_exchange_weak(
                        *state,
                        state
                            .checked_add(ONE_READER)
                            .expect("RwLock reader count overflow"),
                        Ordering::Acquire,
                        Ordering::Relaxed,
                    )
                    .is_ok()
                {
                    return true;
                }

                // If there is high contention on the reader count then we want
                // to leave some time between attempts to acquire the lock to
                // let other threads make progress.
                spinwait_shared.spin_no_yield();
                *state = self.state.load(Ordering::Relaxed);
            }
        };
        self.lock_common(timeout, TOKEN_SHARED, try_lock, |state| {
            state & WRITER_BIT != 0 && (!RECURSIVE || state & READERS_MASK == 0)
        })
        .then_some(crate::GuardMarker::new())
    }

    #[cold]
    fn unlock_shared_slow(&self) {
        // At this point WRITER_PARKED_BIT is set and READER_MASK is empty. We
        // just need to wake up a potentially sleeping pending writer.
        // Using the 2nd key at addr + 1
        let addr = core::ptr::from_ref(self).addr() + 1;
        let callback = |_result: UnparkResult| {
            // Clear the WRITER_PARKED_BIT here since there can only be one
            // parked writer thread.
            self.state.fetch_and(!WRITER_PARKED_BIT, Ordering::Relaxed);
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
    fn lock_upgradable_slow(&self, timeout: Option<Instant>) -> Option<crate::GuardMarker> {
        let try_lock = |state: &mut usize| {
            let mut spinwait_shared = SpinWait::new();
            loop {
                if *state & (WRITER_BIT | UPGRADABLE_BIT) != 0 {
                    return false;
                }

                if self
                    .state
                    .compare_exchange_weak(
                        *state,
                        state
                            .checked_add(ONE_READER | UPGRADABLE_BIT)
                            .expect("RwLock reader count overflow"),
                        Ordering::Acquire,
                        Ordering::Relaxed,
                    )
                    .is_ok()
                {
                    return true;
                }

                // If there is high contention on the reader count then we want
                // to leave some time between attempts to acquire the lock to
                // let other threads make progress.
                spinwait_shared.spin_no_yield();
                *state = self.state.load(Ordering::Relaxed);
            }
        };
        self.lock_common(timeout, TOKEN_UPGRADABLE, try_lock, |state| {
            state & (WRITER_BIT | UPGRADABLE_BIT) != 0
        })
        .then_some(crate::GuardMarker::new())
    }

    #[cold]
    fn unlock_upgradable_fair_slow(&self) {
        // Just release the lock if there are no parked threads.
        let mut state = self.state.load(Ordering::Relaxed);
        while state & PARKED_BIT == 0 {
            match self.state.compare_exchange_weak(
                state,
                state - (ONE_READER | UPGRADABLE_BIT),
                Ordering::Release,
                Ordering::Relaxed,
            ) {
                Ok(_) => return,
                Err(x) => state = x,
            }
        }

        // There are threads to unpark. Try to unpark as many as we can.
        let callback = |new_state, result: UnparkResult| {
            // If we are using a fair unlock then we should keep the
            // rwlock locked and hand it off to the unparked threads.
            let mut state = self.state.load(Ordering::Relaxed);
            // Fall back to normal unpark on overflow. Panicking is
            // not allowed in parking_lot callbacks.
            while let Some(mut new_state) =
                (state - (ONE_READER | UPGRADABLE_BIT)).checked_add(new_state)
            {
                if result.have_more_threads {
                    new_state |= PARKED_BIT;
                } else {
                    new_state &= !PARKED_BIT;
                }
                match self.state.compare_exchange_weak(
                    state,
                    new_state,
                    Ordering::Release,
                    Ordering::Relaxed,
                ) {
                    Ok(_) => return TOKEN_HANDOFF,
                    Err(x) => state = x,
                }
            }

            // Otherwise just release the upgradable lock and update PARKED_BIT.
            loop {
                let mut new_state = state - (ONE_READER | UPGRADABLE_BIT);
                if result.have_more_threads {
                    new_state |= PARKED_BIT;
                } else {
                    new_state &= !PARKED_BIT;
                }
                match self.state.compare_exchange_weak(
                    state,
                    new_state,
                    Ordering::Release,
                    Ordering::Relaxed,
                ) {
                    Ok(_) => return TOKEN_NORMAL,
                    Err(x) => state = x,
                }
            }
        };
        // SAFETY: `callback` does not panic or call into any function of `parking_lot`.
        unsafe {
            self.wake_parked_threads(0, callback);
        }
    }

    #[cold]
    fn try_upgrade_slow(
        &self,
        guard: crate::GuardMarker,
    ) -> Result<crate::GuardMarker, crate::GuardMarker> {
        match self
            .state
            .try_update(Ordering::Acquire, Ordering::Relaxed, |state| {
                (state & READERS_MASK == ONE_READER)
                    .then_some(state - (ONE_READER | UPGRADABLE_BIT) + WRITER_BIT)
            }) {
            Ok(_) => Ok(guard),
            Err(_) => Err(guard),
        }
    }

    #[cold]
    fn upgrade_slow(
        &self,
        guard: crate::GuardMarker,
        timeout: Option<Instant>,
    ) -> Result<crate::GuardMarker, crate::GuardMarker> {
        let result = self.wait_for_readers(timeout, ONE_READER | UPGRADABLE_BIT);
        // We either acquired exclusive access or restored the upgradable read
        // lock after timing out. Both states own the secondary key.
        self.deadlock_acquire_secondary();
        if result { Ok(guard) } else { Err(guard) }
    }

    #[cold]
    fn downgrade_slow(&self) {
        // We only reach this point if PARKED_BIT is set.
        let callback = |_, result: UnparkResult| {
            // Clear the parked bit if there no more parked threads
            if !result.have_more_threads {
                self.state.fetch_and(!PARKED_BIT, Ordering::Relaxed);
            }
            TOKEN_NORMAL
        };
        // SAFETY: `callback` does not panic or call into any function of `parking_lot`.
        unsafe {
            self.wake_parked_threads(ONE_READER, callback);
        }
    }

    #[cold]
    fn downgrade_to_upgradable_slow(&self) {
        // We only reach this point if PARKED_BIT is set.
        let callback = |_, result: UnparkResult| {
            // Clear the parked bit if there no more parked threads
            if !result.have_more_threads {
                self.state.fetch_and(!PARKED_BIT, Ordering::Relaxed);
            }
            TOKEN_NORMAL
        };
        // SAFETY: `callback` does not panic or call into any function of `parking_lot`.
        unsafe {
            self.wake_parked_threads(ONE_READER | UPGRADABLE_BIT, callback);
        }
    }

    #[cold]
    unsafe fn bump_shared_slow(&self) {
        unsafe { self.unlock_shared(crate::GuardMarker::new()) };
        util::abort_on_panic(|| self.lock_shared());
    }

    #[cold]
    fn bump_exclusive_slow(&self) {
        self.deadlock_release_all();
        self.unlock_exclusive_fair_slow();
        self.lock_exclusive();
    }

    #[cold]
    fn bump_upgradable_slow(&self) {
        self.deadlock_release_all();
        self.unlock_upgradable_fair_slow();
        util::abort_on_panic(|| self.lock_upgradable());
    }

    /// Common code for waking up parked threads after releasing `WRITER_BIT` or
    /// `UPGRADABLE_BIT`.
    ///
    /// # Safety
    ///
    /// `callback` must uphold the requirements of the `callback` parameter to
    /// `parking_lot_core::unpark_filter`: it must not unwind or call any
    /// parking-lot function.
    #[inline]
    unsafe fn wake_parked_threads(
        &self,
        new_state: usize,
        callback: impl FnOnce(usize, UnparkResult) -> UnparkToken,
    ) {
        // We must wake up at least one upgrader or writer if there is one,
        // otherwise they may end up parked indefinitely since unlock_shared
        // does not call wake_parked_threads.
        let new_state = Cell::new(new_state);
        let addr = core::ptr::from_ref(self).addr();
        let filter = |ParkToken(token)| {
            let s = new_state.get();

            // Normal rwlocks stop after selecting a writer. Recursive rwlocks
            // must continue scanning for readers that may be needed for
            // forward progress.
            if !RECURSIVE && s & WRITER_BIT != 0 {
                return FilterOp::Stop;
            }

            // Otherwise wake *all* readers and one upgrader/writer.
            if token & (UPGRADABLE_BIT | WRITER_BIT) != 0 && s & (UPGRADABLE_BIT | WRITER_BIT) != 0
            {
                // Skip writers and upgradable readers if we already have
                // a writer/upgradable reader.
                FilterOp::Skip
            } else {
                new_state.set(s + token);
                FilterOp::Unpark
            }
        };
        let callback = |result| callback(new_state.get(), result);
        // SAFETY:
        // * `addr` is an address we control.
        // * `filter` does not panic or call into any function of `parking_lot`.
        // * `callback` safety responsibility is on caller
        unsafe { parking_lot_core::unpark_filter(addr, filter, callback) };
    }

    // Common code for waiting for readers to exit the lock after acquiring
    // WRITER_BIT.
    #[inline]
    fn wait_for_readers(&self, timeout: Option<Instant>, prev_value: usize) -> bool {
        // At this point WRITER_BIT is already set, we just need to wait for the
        // remaining readers to exit the lock.
        let mut spinwait = SpinWait::new();
        let mut state = self.state.load(Ordering::Acquire);
        while state & READERS_MASK != 0 {
            // Spin a few times to wait for readers to exit
            if spinwait.spin() {
                state = self.state.load(Ordering::Acquire);
                continue;
            }

            // Set the parked bit
            if state & WRITER_PARKED_BIT == 0
                && let Err(x) = self.state.compare_exchange_weak(
                    state,
                    state | WRITER_PARKED_BIT,
                    Ordering::Acquire,
                    Ordering::Acquire,
                )
            {
                state = x;
                continue;
            }

            // Park our thread until we are woken up by an unlock
            // Using the 2nd key at addr + 1
            let addr = core::ptr::from_ref(self).addr() + 1;
            let validate = || {
                let state = self.state.load(Ordering::Relaxed);
                state & READERS_MASK != 0 && state & WRITER_PARKED_BIT != 0
            };
            let before_sleep = || {};
            let timed_out = |_, was_last_thread: bool| {
                // Clear the parked bit while holding the queue lock. There can
                // only be one thread parked (this one).
                debug_assert!(was_last_thread);
                self.state.fetch_and(!WRITER_PARKED_BIT, Ordering::Relaxed);
            };
            // SAFETY:
            //   * `addr` is an address we control.
            //   * `validate`/`timed_out` does not panic or call into any function of `parking_lot`.
            //   * `before_sleep` does not call `park`, nor does it panic.
            let park_result = unsafe {
                parking_lot_core::park(
                    addr,
                    validate,
                    before_sleep,
                    timed_out,
                    TOKEN_EXCLUSIVE,
                    timeout,
                )
            };
            match park_result {
                // We still need to re-check the state if we are unparked
                // since a previous writer timing-out could have allowed
                // another reader to sneak in before we parked.
                ParkResult::Unparked(_) | ParkResult::Invalid => {
                    state = self.state.load(Ordering::Acquire);
                    continue;
                }

                // Timeout expired
                ParkResult::TimedOut => {
                    // We need to release WRITER_BIT and revert back to
                    // our previous value. We also wake up any threads that
                    // might be waiting on WRITER_BIT.
                    if prev_value == 0 {
                        // A fresh writer no longer owns WRITER_BIT after the
                        // rollback. An upgrading reader retains ownership of
                        // the primary key when its upgradable lock is restored.
                        self.deadlock_release_primary();
                    }
                    let state = self
                        .state
                        .fetch_add(prev_value.wrapping_sub(WRITER_BIT), Ordering::Relaxed);
                    if state & PARKED_BIT != 0 {
                        let callback = |_, result: UnparkResult| {
                            // Clear the parked bit if there no more parked threads
                            if !result.have_more_threads {
                                self.state.fetch_and(!PARKED_BIT, Ordering::Relaxed);
                            }
                            TOKEN_NORMAL
                        };
                        // SAFETY: `callback` does not panic or call any function of `parking_lot`.
                        unsafe {
                            self.wake_parked_threads(prev_value, callback);
                        }
                    }
                    return false;
                }
            }
        }
        true
    }

    /// Common code for acquiring a lock
    #[inline]
    fn lock_common(
        &self,
        timeout: Option<Instant>,
        token: ParkToken,
        mut try_lock: impl FnMut(&mut usize) -> bool,
        should_park: impl Fn(usize) -> bool,
    ) -> bool {
        let mut spinwait = SpinWait::new();
        let mut state = self.state.load(Ordering::Relaxed);
        loop {
            // Attempt to grab the lock
            if try_lock(&mut state) {
                return true;
            }

            // If there are no parked threads, try spinning a few times.
            if state & (PARKED_BIT | WRITER_PARKED_BIT) == 0 && spinwait.spin() {
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
            let validate = || {
                let state = self.state.load(Ordering::Relaxed);
                state & PARKED_BIT != 0 && should_park(state)
            };
            let before_sleep = || {};
            let timed_out = |_, was_last_thread| {
                // Clear the parked bit if we were the last parked thread
                if was_last_thread {
                    self.state.fetch_and(!PARKED_BIT, Ordering::Relaxed);
                }
            };

            // SAFETY:
            // * `addr` is an address we control.
            // * `validate`/`timed_out` does not panic or call into any function of `parking_lot`.
            // * `before_sleep` does not call `park`, nor does it panic.
            let park_result = unsafe {
                parking_lot_core::park(addr, validate, before_sleep, timed_out, token, timeout)
            };
            match park_result {
                // The thread that unparked us passed the lock on to us
                // directly without unlocking it.
                ParkResult::Unparked(TOKEN_HANDOFF) => return true,

                // We were unparked normally, try acquiring the lock again
                ParkResult::Unparked(_) => (),

                // The validation function failed, try locking again
                ParkResult::Invalid => (),

                // Timeout expired
                ParkResult::TimedOut => return false,
            }

            // Loop back and try locking again
            spinwait.reset();
            state = self.state.load(Ordering::Relaxed);
        }
    }

    #[inline]
    fn deadlock_acquire_primary(&self) {
        unsafe { deadlock::acquire_resource(core::ptr::from_ref(self).addr()) };
    }

    #[inline]
    fn deadlock_release_primary(&self) {
        unsafe { deadlock::release_resource(core::ptr::from_ref(self).addr()) };
    }

    #[inline]
    fn deadlock_acquire_secondary(&self) {
        unsafe { deadlock::acquire_resource(core::ptr::from_ref(self).addr() + 1) };
    }

    #[inline]
    fn deadlock_release_secondary(&self) {
        unsafe { deadlock::release_resource(core::ptr::from_ref(self).addr() + 1) };
    }

    #[inline]
    fn deadlock_acquire_all(&self) {
        self.deadlock_acquire_primary();
        self.deadlock_acquire_secondary();
    }

    #[inline]
    fn deadlock_release_all(&self) {
        self.deadlock_release_primary();
        self.deadlock_release_secondary();
    }

    #[cfg(test)]
    pub(crate) fn has_parked_threads(&self) -> bool {
        self.state.load(Ordering::Relaxed) & (PARKED_BIT | WRITER_PARKED_BIT) != 0
    }
}
