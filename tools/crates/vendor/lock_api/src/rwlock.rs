use core::cell::UnsafeCell;
use core::fmt;
use core::marker::PhantomData;
use core::mem;
use core::ops::{Deref, DerefMut};

use crate::guard::{ExclusiveGuardData, SharedGuardData, abort_on_panic};

#[cfg(feature = "arc_lock")]
use alloc::sync::Arc;
use core::mem::ManuallyDrop;
use core::ptr;

#[cfg(feature = "owning_ref")]
use owning_ref::StableAddress;

#[cfg(feature = "serde")]
use serde::{Deserialize, Deserializer, Serialize, Serializer};

/// Basic operations for a reader-writer lock.
///
/// Types implementing this trait can be used by `RwLock` to form a safe and
/// fully-functioning `RwLock` type.
///
/// # Safety
///
/// Implementations must allow either shared acquisitions or one exclusive
/// acquisition, with acquire semantics on successful locking and release
/// semantics on unlocking. These requirements also apply to subtraits.
///
/// Each successful lock operation returns guard state representing an
/// acquisition in a particular mode. Operations on a held lock must receive the
/// state for that acquisition and mode. Unlocking consumes the state; mode
/// transitions return state for the resulting mode. A transition that returns
/// failure must return valid state for the original acquisition and mode.
///
/// Guard state may release its acquisition when dropped, but forgetting it must
/// be safe. A caller with exclusive ownership of the lock object may move or
/// drop it even while it is locked; this must also be safe.
///
/// If a lock operation unwinds, it must not leave the caller holding a new
/// acquisition. If an operation consuming guard state unwinds, it must release
/// the acquisition represented by that state, even if it changed modes or
/// re-acquired the lock before panicking. Neither case may release an
/// acquisition held independently of the call.
pub unsafe trait RawRwLock {
    /// Initial value for an unlocked `RwLock`.
    // A “non-constant” const item is a legacy way to supply an initialized value to downstream
    // static items. Can hopefully be replaced with `const fn new() -> Self` at some point.
    #[allow(clippy::declare_interior_mutable_const)]
    const INIT: Self;

    /// Per-acquisition state stored in the lock guard.
    ///
    /// This type also determines whether guards are `Send` and `Sync`.
    /// Stateless implementations can use [`GuardSend`](crate::GuardSend) or
    /// [`GuardNoSend`](crate::GuardNoSend).
    type SharedGuard;

    /// Per-acquisition state for an exclusive lock.
    ///
    /// This type also determines whether guards are `Send` and `Sync`.
    /// Stateless implementations can use [`GuardSend`](crate::GuardSend) or
    /// [`GuardNoSend`](crate::GuardNoSend).
    type ExclusiveGuard;

    /// Acquires a shared lock, blocking the current thread until it is able to do so.
    fn lock_shared(&self) -> Self::SharedGuard;

    /// Attempts to acquire a shared lock without blocking.
    fn try_lock_shared(&self) -> Option<Self::SharedGuard>;

    /// Releases a shared lock.
    ///
    /// # Safety
    ///
    /// This method requires the state for a shared acquisition of this lock held
    /// in the current context.
    unsafe fn unlock_shared(&self, guard: Self::SharedGuard);

    /// Acquires an exclusive lock, blocking the current thread until it is able to do so.
    fn lock_exclusive(&self) -> Self::ExclusiveGuard;

    /// Attempts to acquire an exclusive lock without blocking.
    fn try_lock_exclusive(&self) -> Option<Self::ExclusiveGuard>;

    /// Releases an exclusive lock.
    ///
    /// # Safety
    ///
    /// This method requires the state for an exclusive acquisition of this lock
    /// held in the current context.
    unsafe fn unlock_exclusive(&self, guard: Self::ExclusiveGuard);

    /// Checks if this `RwLock` is currently locked in any way.
    ///
    /// The result is a momentary snapshot and may be stale by the time it is
    /// returned.
    fn is_locked(&self) -> bool;

    /// Check if this `RwLock` is currently exclusively locked.
    ///
    /// The result is a momentary snapshot and may be stale by the time it is
    /// returned.
    fn is_locked_exclusive(&self) -> bool;
}

/// Additional methods for `RwLock`s which support fair unlocking.
///
/// Fair unlocking gives waiting threads priority over new attempts to acquire
/// the lock, without giving other threads the opportunity to "steal" it in the
/// meantime. This is typically slower than unfair unlocking, but may be
/// necessary in certain circumstances.
///
/// # Safety
///
/// Implementations must uphold the safety requirements of [`RawRwLock`] for
/// the additional methods provided by this trait.
pub unsafe trait RawRwLockFair: RawRwLock {
    /// Releases a shared lock using a fair unlock protocol.
    ///
    /// # Safety
    ///
    /// This method requires the state for a shared acquisition of this lock held
    /// in the current context.
    unsafe fn unlock_shared_fair(&self, guard: Self::SharedGuard);

    /// Releases an exclusive lock using a fair unlock protocol.
    ///
    /// # Safety
    ///
    /// This method requires the state for an exclusive acquisition of this lock
    /// held in the current context.
    unsafe fn unlock_exclusive_fair(&self, guard: Self::ExclusiveGuard);

    /// Temporarily yields a shared lock to a waiting thread if there is one.
    ///
    /// This method is functionally equivalent to calling `unlock_shared_fair` followed
    /// by `lock_shared`, however it can be much more efficient in the case where there
    /// are no waiting threads.
    ///
    /// # Safety
    ///
    /// This method requires the state for a shared acquisition of this lock held
    /// in the current context.
    ///
    /// Implementations may unwind only with ownership in the original mode
    /// and valid guard state restored. If restoration fails, they must abort.
    ///
    /// # Aborts
    ///
    /// The default implementation aborts if re-locking the `RwLock` panics.
    unsafe fn bump_shared(&self, guard: &mut Self::SharedGuard) {
        // Move the state out, then restore it on both normal return and unwind.
        let state = unsafe { ptr::read(guard) };
        defer!(abort_on_panic(|| unsafe {
            ptr::write(guard, self.lock_shared())
        }));
        unsafe { self.unlock_shared_fair(state) };
    }

    /// Temporarily yields an exclusive lock to a waiting thread if there is one.
    ///
    /// This method is functionally equivalent to calling `unlock_exclusive_fair` followed
    /// by `lock_exclusive`, however it can be much more efficient in the case where there
    /// are no waiting threads.
    ///
    /// # Safety
    ///
    /// This method requires the state for an exclusive acquisition of this lock
    /// held in the current context.
    ///
    /// Implementations may unwind only with ownership in the original mode
    /// and valid guard state restored. If restoration fails, they must abort.
    ///
    /// # Aborts
    ///
    /// The default implementation aborts if re-locking the `RwLock` panics.
    unsafe fn bump_exclusive(&self, guard: &mut Self::ExclusiveGuard) {
        // Move the state out, then restore it on both normal return and unwind.
        let state = unsafe { ptr::read(guard) };
        defer!(abort_on_panic(|| unsafe {
            ptr::write(guard, self.lock_exclusive())
        }));
        unsafe { self.unlock_exclusive_fair(state) };
    }
}

/// Additional methods for `RwLock`s which support atomically downgrading an
/// exclusive lock to a shared lock.
///
/// # Safety
///
/// Implementations must uphold the safety requirements of [`RawRwLock`] for
/// the additional methods provided by this trait. Downgrading from exclusive
/// access must release to subsequent lock acquisitions.
pub unsafe trait RawRwLockDowngrade: RawRwLock {
    /// Atomically downgrades an exclusive lock into a shared lock without
    /// allowing any thread to take an exclusive lock in the meantime.
    ///
    /// # Safety
    ///
    /// This method requires the state for an exclusive acquisition of this lock
    /// held in the current context.
    unsafe fn downgrade(&self, guard: Self::ExclusiveGuard) -> Self::SharedGuard;
}

/// Additional methods for `RwLock`s which support locking with timeouts.
///
/// The `Duration` and `Instant` types are specified as associated types so that
/// this trait is usable even in `no_std` environments.
///
/// # Safety
///
/// Implementations must uphold the safety requirements of [`RawRwLock`] for
/// the additional methods provided by this trait.
pub unsafe trait RawRwLockTimed: RawRwLock {
    /// Duration type used for `try_lock_for`.
    type Duration;

    /// Instant type used for `try_lock_until`.
    type Instant;

    /// Attempts to acquire a shared lock until a timeout is reached.
    ///
    /// A successful operation may return early. An unsuccessful operation must
    /// not return before the timeout, but may return later due to scheduling or
    /// platform-specific behavior.
    fn try_lock_shared_for(&self, timeout: Self::Duration) -> Option<Self::SharedGuard>;

    /// Attempts to acquire a shared lock until a timeout is reached.
    ///
    /// A successful operation may return early. An unsuccessful operation must
    /// not return before the timeout, but may return later due to scheduling or
    /// platform-specific behavior.
    fn try_lock_shared_until(&self, timeout: Self::Instant) -> Option<Self::SharedGuard>;

    /// Attempts to acquire an exclusive lock until a timeout is reached.
    ///
    /// A successful operation may return early. An unsuccessful operation must
    /// not return before the timeout, but may return later due to scheduling or
    /// platform-specific behavior.
    fn try_lock_exclusive_for(&self, timeout: Self::Duration) -> Option<Self::ExclusiveGuard>;

    /// Attempts to acquire an exclusive lock until a timeout is reached.
    ///
    /// A successful operation may return early. An unsuccessful operation must
    /// not return before the timeout, but may return later due to scheduling or
    /// platform-specific behavior.
    fn try_lock_exclusive_until(&self, timeout: Self::Instant) -> Option<Self::ExclusiveGuard>;
}

/// Additional methods for `RwLock`s which support atomically upgrading a shared
/// lock to an exclusive lock.
///
/// This requires acquiring a special "upgradable read lock" instead of a
/// normal shared lock. There may only be one upgradable lock at any time,
/// otherwise deadlocks could occur when upgrading.
///
/// # Safety
///
/// Implementations must uphold the safety requirements of [`RawRwLock`] for
/// the additional methods provided by this trait. A successful upgrade to
/// exclusive access must acquire from the shared-lock releases it replaces.
pub unsafe trait RawRwLockUpgrade: RawRwLock {
    /// Per-acquisition state for an upgradable read lock.
    ///
    /// This type also determines whether guards are `Send` and `Sync`.
    /// Stateless implementations can use [`GuardSend`](crate::GuardSend) or
    /// [`GuardNoSend`](crate::GuardNoSend).
    type UpgradableGuard;

    /// Acquires an upgradable lock, blocking the current thread until it is able to do so.
    fn lock_upgradable(&self) -> Self::UpgradableGuard;

    /// Attempts to acquire an upgradable lock without blocking.
    fn try_lock_upgradable(&self) -> Option<Self::UpgradableGuard>;

    /// Releases an upgradable lock.
    ///
    /// # Safety
    ///
    /// This method requires the state for an upgradable acquisition of this lock
    /// held in the current context.
    unsafe fn unlock_upgradable(&self, guard: Self::UpgradableGuard);

    /// Upgrades an upgradable lock to an exclusive lock.
    ///
    /// # Safety
    ///
    /// This method requires the state for an upgradable acquisition of this lock
    /// held in the current context.
    unsafe fn upgrade(&self, guard: Self::UpgradableGuard) -> Self::ExclusiveGuard;

    /// Attempts to upgrade an upgradable lock to an exclusive lock without
    /// blocking.
    ///
    /// On success, returns `Ok` with the exclusive state. On failure, the
    /// upgradable lock remains held and its state is returned in `Err`.
    ///
    /// # Safety
    ///
    /// This method requires the state for an upgradable acquisition of this lock
    /// held in the current context.
    unsafe fn try_upgrade(
        &self,
        guard: Self::UpgradableGuard,
    ) -> Result<Self::ExclusiveGuard, Self::UpgradableGuard>;
}

/// Additional methods for `RwLock`s which support upgradable locks and fair
/// unlocking.
///
/// # Safety
///
/// Implementations must uphold the safety requirements of [`RawRwLock`] for
/// the additional methods provided by this trait.
pub unsafe trait RawRwLockUpgradeFair: RawRwLockUpgrade + RawRwLockFair {
    /// Releases an upgradable lock using a fair unlock protocol.
    ///
    /// # Safety
    ///
    /// This method requires the state for an upgradable acquisition of this lock
    /// held in the current context.
    unsafe fn unlock_upgradable_fair(&self, guard: Self::UpgradableGuard);

    /// Temporarily yields an upgradable lock to a waiting thread if there is one.
    ///
    /// This method is functionally equivalent to calling `unlock_upgradable_fair` followed
    /// by `lock_upgradable`, however it can be much more efficient in the case where there
    /// are no waiting threads.
    ///
    /// # Safety
    ///
    /// This method requires the state for an upgradable acquisition of this lock
    /// held in the current context.
    ///
    /// Implementations may unwind only with ownership in the original mode
    /// and valid guard state restored. If restoration fails, they must abort.
    ///
    /// # Aborts
    ///
    /// The default implementation aborts if re-locking the `RwLock` panics.
    unsafe fn bump_upgradable(&self, guard: &mut Self::UpgradableGuard) {
        // Move the state out, then restore it on both normal return and unwind.
        let state = unsafe { ptr::read(guard) };
        defer!(abort_on_panic(|| unsafe {
            ptr::write(guard, self.lock_upgradable())
        }));
        unsafe { self.unlock_upgradable_fair(state) };
    }
}

/// Additional methods for `RwLock`s which support upgradable locks and lock
/// downgrading.
///
/// # Safety
///
/// Implementations must uphold the safety requirements of [`RawRwLock`] for
/// the additional methods provided by this trait. Downgrading from exclusive
/// to upgradable access must release to subsequent lock acquisitions.
pub unsafe trait RawRwLockUpgradeDowngrade: RawRwLockUpgrade + RawRwLockDowngrade {
    /// Downgrades an upgradable lock to a shared lock.
    ///
    /// # Safety
    ///
    /// This method requires the state for an upgradable acquisition of this lock
    /// held in the current context.
    unsafe fn downgrade_upgradable(&self, guard: Self::UpgradableGuard) -> Self::SharedGuard;

    /// Downgrades an exclusive lock to an upgradable lock.
    ///
    /// # Safety
    ///
    /// This method requires the state for an exclusive acquisition of this lock
    /// held in the current context.
    unsafe fn downgrade_to_upgradable(&self, guard: Self::ExclusiveGuard) -> Self::UpgradableGuard;
}

/// Additional methods for `RwLock`s which support upgradable locks and locking
/// with timeouts.
///
/// # Safety
///
/// Implementations must uphold the safety requirements of [`RawRwLock`] for
/// the additional methods provided by this trait.
pub unsafe trait RawRwLockUpgradeTimed: RawRwLockUpgrade + RawRwLockTimed {
    /// Attempts to acquire an upgradable lock until a timeout is reached.
    ///
    /// See [`RawRwLockTimed::try_lock_shared_for`] for timeout behavior.
    fn try_lock_upgradable_for(&self, timeout: Self::Duration) -> Option<Self::UpgradableGuard>;

    /// Attempts to acquire an upgradable lock until a timeout is reached.
    ///
    /// See [`RawRwLockTimed::try_lock_shared_until`] for timeout behavior.
    fn try_lock_upgradable_until(&self, timeout: Self::Instant) -> Option<Self::UpgradableGuard>;

    /// Attempts to upgrade an upgradable lock to an exclusive lock until a
    /// timeout is reached.
    ///
    /// See [`RawRwLockTimed::try_lock_exclusive_for`] for timeout behavior.
    ///
    /// On success, returns `Ok` with the exclusive state. On failure, the
    /// upgradable lock remains held and its state is returned in `Err`.
    ///
    /// # Safety
    ///
    /// This method requires the state for an upgradable acquisition of this lock
    /// held in the current context.
    unsafe fn try_upgrade_for(
        &self,
        guard: Self::UpgradableGuard,
        timeout: Self::Duration,
    ) -> Result<Self::ExclusiveGuard, Self::UpgradableGuard>;

    /// Attempts to upgrade an upgradable lock to an exclusive lock until a
    /// timeout is reached.
    ///
    /// See [`RawRwLockTimed::try_lock_exclusive_until`] for timeout behavior.
    ///
    /// On success, returns `Ok` with the exclusive state. On failure, the
    /// upgradable lock remains held and its state is returned in `Err`.
    ///
    /// # Safety
    ///
    /// This method requires the state for an upgradable acquisition of this lock
    /// held in the current context.
    unsafe fn try_upgrade_until(
        &self,
        guard: Self::UpgradableGuard,
        timeout: Self::Instant,
    ) -> Result<Self::ExclusiveGuard, Self::UpgradableGuard>;
}

/// A reader-writer lock.
///
/// This type of lock allows a number of readers or at most one writer at any
/// point in time. The write portion of this lock typically allows modification
/// of the underlying data (exclusive access) and the read portion of this lock
/// typically allows for read-only access (shared access).
///
/// The type parameter `T` represents the data that this lock protects. It is
/// required that `T` satisfies `Send` to be shared across threads and `Sync` to
/// allow concurrent access through readers. The RAII guards returned from the
/// locking methods implement `Deref` (and `DerefMut` for the `write` methods)
/// to allow access to the contents of the lock.
pub struct RwLock<R, T: ?Sized> {
    raw: R,
    data: UnsafeCell<T>,
}

// Copied and modified from serde
#[cfg(feature = "serde")]
impl<R, T> Serialize for RwLock<R, T>
where
    R: RawRwLock,
    T: Serialize + ?Sized,
{
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        self.read().serialize(serializer)
    }
}

#[cfg(feature = "serde")]
impl<'de, R, T> Deserialize<'de> for RwLock<R, T>
where
    R: RawRwLock,
    T: Deserialize<'de>,
{
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        Deserialize::deserialize(deserializer).map(RwLock::new)
    }
}

unsafe impl<R: RawRwLock + Sync, T: ?Sized + Send + Sync> Sync for RwLock<R, T> {}

impl<R: RawRwLock, T> RwLock<R, T> {
    /// Creates a new instance of an `RwLock<T>` which is unlocked.
    #[inline]
    pub const fn new(val: T) -> RwLock<R, T> {
        RwLock {
            data: UnsafeCell::new(val),
            raw: R::INIT,
        }
    }

    /// Consumes this `RwLock`, returning the underlying data.
    #[inline]
    #[allow(unused_unsafe)]
    pub fn into_inner(self) -> T {
        unsafe { self.data.into_inner() }
    }
}

impl<R, T> RwLock<R, T> {
    /// Creates a new instance of an `RwLock<T>` based on a pre-existing raw
    /// rwlock.
    #[inline]
    pub const fn from_raw(raw_rwlock: R, val: T) -> RwLock<R, T> {
        RwLock {
            data: UnsafeCell::new(val),
            raw: raw_rwlock,
        }
    }

    /// Consumes this read-write lock, returning the underlying data and raw lock.
    #[inline]
    pub fn into_inner_with_raw(self) -> (R, T) {
        (self.raw, self.data.into_inner())
    }
}

impl<R: RawRwLock, T: ?Sized> RwLock<R, T> {
    /// Creates a new `RwLockReadGuard` without checking if the lock is held.
    ///
    /// # Safety
    ///
    /// This method must only be called if the thread logically holds a read lock.
    ///
    /// This function does not increment the read count of the lock. Calling this function when a
    /// guard has already been produced is undefined behavior unless the guard was forgotten
    /// with `mem::forget`.
    ///
    /// The caller must ensure that existing references to the protected data
    /// remain valid when the returned guard is used or dropped. In particular,
    /// the returned guard must not permit accesses that conflict with existing
    /// references.
    ///
    /// `guard` must be the state for this acquisition of this lock.
    #[inline]
    pub unsafe fn make_read_guard_unchecked(
        &self,
        guard: R::SharedGuard,
    ) -> RwLockReadGuard<'_, R, T> {
        RwLockReadGuard {
            rwlock: self,
            marker: PhantomData,
            guard: ManuallyDrop::new(guard),
        }
    }

    /// Creates a new `RwLockWriteGuard` without checking if the lock is held.
    ///
    /// # Safety
    ///
    /// This method must only be called if the thread logically holds a write lock.
    ///
    /// Calling this function when a guard has already been produced is undefined behavior unless
    /// the guard was forgotten with `mem::forget`.
    ///
    /// The caller must ensure that existing references to the protected data
    /// remain valid when the returned guard is used or dropped. In particular,
    /// the returned guard must not permit accesses that conflict with existing
    /// references.
    ///
    /// `guard` must be the state for this acquisition of this lock.
    #[inline]
    pub unsafe fn make_write_guard_unchecked(
        &self,
        guard: R::ExclusiveGuard,
    ) -> RwLockWriteGuard<'_, R, T> {
        RwLockWriteGuard {
            rwlock: self,
            marker: PhantomData,
            guard: ManuallyDrop::new(guard),
        }
    }

    /// Locks this `RwLock` with shared read access, blocking the current thread
    /// until it can be acquired.
    ///
    /// The calling thread will be blocked until there are no more writers which
    /// hold the lock. There may be other readers currently inside the lock when
    /// this method returns.
    ///
    /// Whether recursive read locking is supported depends on the raw lock
    /// implementation. Implementations which block new readers while a writer
    /// is waiting may deadlock if the current thread already holds a read lock.
    ///
    /// Returns an RAII guard which will release this thread's shared access
    /// once it is dropped.
    ///
    /// # Panics
    ///
    /// This method may panic if the maximum number of readers supported by the
    /// raw lock has been reached.
    #[inline]
    #[track_caller]
    pub fn read(&self) -> RwLockReadGuard<'_, R, T> {
        let guard = self.raw.lock_shared();
        // SAFETY: The lock is held, as required.
        unsafe { self.make_read_guard_unchecked(guard) }
    }

    /// Attempts to acquire this `RwLock` with shared read access.
    ///
    /// If the access could not be granted at this time, then `None` is returned.
    /// Otherwise, an RAII guard is returned which will release the shared access
    /// when it is dropped.
    ///
    /// This function does not block.
    ///
    /// # Panics
    ///
    /// This method may panic if the maximum number of readers supported by the
    /// raw lock has been reached.
    #[inline]
    #[track_caller]
    pub fn try_read(&self) -> Option<RwLockReadGuard<'_, R, T>> {
        if let Some(guard) = self.raw.try_lock_shared() {
            // SAFETY: The lock is held, as required.
            Some(unsafe { self.make_read_guard_unchecked(guard) })
        } else {
            None
        }
    }

    /// Locks this `RwLock` with exclusive write access, blocking the current
    /// thread until it can be acquired.
    ///
    /// This function will not return while other writers or other readers
    /// currently have access to the lock.
    ///
    /// The exact behavior when the current thread already holds the lock is
    /// unspecified. The call will not return; it may panic or deadlock, for
    /// example.
    ///
    /// Returns an RAII guard which will drop the write access of this `RwLock`
    /// when dropped.
    ///
    /// # Panics
    ///
    /// This function may panic if the lock is already held by the current
    /// thread.
    #[inline]
    #[track_caller]
    pub fn write(&self) -> RwLockWriteGuard<'_, R, T> {
        let guard = self.raw.lock_exclusive();
        // SAFETY: The lock is held, as required.
        unsafe { self.make_write_guard_unchecked(guard) }
    }

    /// Attempts to lock this `RwLock` with exclusive write access.
    ///
    /// If the lock could not be acquired at this time, then `None` is returned.
    /// Otherwise, an RAII guard is returned which will release the lock when
    /// it is dropped.
    ///
    /// This function does not block.
    #[inline]
    #[track_caller]
    pub fn try_write(&self) -> Option<RwLockWriteGuard<'_, R, T>> {
        if let Some(guard) = self.raw.try_lock_exclusive() {
            // SAFETY: The lock is held, as required.
            Some(unsafe { self.make_write_guard_unchecked(guard) })
        } else {
            None
        }
    }

    /// Returns a mutable reference to the underlying data.
    ///
    /// Since this call borrows the `RwLock` mutably, no actual locking needs to
    /// take place -- the mutable borrow statically guarantees no new locks can
    /// be acquired while the reference exists. This method does not clear locks
    /// previously abandoned by forgetting a [`RwLockReadGuard`] or
    /// [`RwLockWriteGuard`].
    #[inline]
    pub const fn get_mut(&mut self) -> &mut T {
        self.data.get_mut()
    }

    /// Checks whether this `RwLock` is currently locked in any way.
    ///
    /// The result is a momentary snapshot and may be stale by the time it is
    /// returned.
    #[inline]
    #[track_caller]
    pub fn is_locked(&self) -> bool {
        self.raw.is_locked()
    }

    /// Checks whether this `RwLock` is currently exclusively locked.
    ///
    /// The result is a momentary snapshot and may be stale by the time it is
    /// returned.
    #[inline]
    #[track_caller]
    pub fn is_locked_exclusive(&self) -> bool {
        self.raw.is_locked_exclusive()
    }

    /// Forcibly unlocks a read lock.
    ///
    /// This is useful when the raw acquisition state is managed separately,
    /// for example when dealing with FFI.
    ///
    /// # Safety
    ///
    /// The current context must hold the lock using the supplied acquisition
    /// state, with no live guard responsible for unlocking it.
    /// Behavior is undefined if a rwlock is read-unlocked when not read-locked.
    ///
    /// The caller must ensure that releasing the lock does not invalidate any
    /// outstanding references to the protected data. Any subsequent access
    /// through previously obtained pointers must be properly synchronized.
    ///
    /// `guard` must be the state for this acquisition of this lock.
    #[inline]
    #[track_caller]
    pub unsafe fn force_unlock_read(&self, guard: R::SharedGuard) {
        unsafe { self.raw.unlock_shared(guard) };
    }

    /// Forcibly unlocks a write lock.
    ///
    /// This is useful when the raw acquisition state is managed separately,
    /// for example when dealing with FFI.
    ///
    /// # Safety
    ///
    /// The current context must hold the lock using the supplied acquisition
    /// state, with no live guard responsible for unlocking it.
    /// Behavior is undefined if a rwlock is write-unlocked when not write-locked.
    ///
    /// The caller must ensure that releasing the lock does not invalidate any
    /// outstanding references to the protected data. Any subsequent access
    /// through previously obtained pointers must be properly synchronized.
    ///
    /// `guard` must be the state for this acquisition of this lock.
    #[inline]
    #[track_caller]
    pub unsafe fn force_unlock_write(&self, guard: R::ExclusiveGuard) {
        unsafe { self.raw.unlock_exclusive(guard) };
    }

    /// Returns the underlying raw reader-writer lock object.
    ///
    /// Note that you will most likely need to import the `RawRwLock` trait from
    /// `lock_api` to be able to call functions on the raw
    /// reader-writer lock.
    ///
    /// # Safety
    ///
    /// The caller must ensure that operations on the raw lock preserve the
    /// validity of all live guards and references to the protected data.
    /// A guard must not be used or dropped while its lock is released, and any
    /// access through previously obtained pointers must be properly synchronized.
    pub unsafe fn raw(&self) -> &R {
        &self.raw
    }

    /// Returns a raw pointer to the underlying data.
    ///
    /// The returned pointer is always non-null and properly aligned, but the
    /// caller must ensure that reads and writes through it are properly
    /// synchronized and that the pointer is not used after the rwlock is
    /// dropped.
    #[inline]
    pub const fn data_ptr(&self) -> *mut T {
        self.data.get()
    }

    /// Creates a new `ArcRwLockReadGuard` without checking if the lock is held.
    ///
    /// # Safety
    ///
    /// This method must only be called if the thread logically holds a read lock.
    ///
    /// This function does not increment the read count of the lock. Calling this function when a
    /// guard has already been produced is undefined behavior unless the guard was forgotten
    /// with `mem::forget`.
    ///
    /// The caller must ensure that existing references to the protected data
    /// remain valid when the returned guard is used or dropped. In particular,
    /// the returned guard must not permit accesses that conflict with existing
    /// references.
    ///
    /// `guard` must be the state for this acquisition of this lock.
    #[cfg(feature = "arc_lock")]
    #[inline]
    pub unsafe fn make_arc_read_guard_unchecked(
        self: &Arc<Self>,
        guard: R::SharedGuard,
    ) -> ArcRwLockReadGuard<R, T> {
        ArcRwLockReadGuard {
            rwlock: self.clone(),
            guard: ManuallyDrop::new(guard),
        }
    }

    /// Creates a new `ArcRwLockWriteGuard` without checking if the lock is held.
    ///
    /// # Safety
    ///
    /// This method must only be called if the thread logically holds a write lock.
    ///
    /// Calling this function when a guard has already been produced is undefined behavior unless
    /// the guard was forgotten with `mem::forget`.
    ///
    /// The caller must ensure that existing references to the protected data
    /// remain valid when the returned guard is used or dropped. In particular,
    /// the returned guard must not permit accesses that conflict with existing
    /// references.
    ///
    /// `guard` must be the state for this acquisition of this lock.
    #[cfg(feature = "arc_lock")]
    #[inline]
    pub unsafe fn make_arc_write_guard_unchecked(
        self: &Arc<Self>,
        guard: R::ExclusiveGuard,
    ) -> ArcRwLockWriteGuard<R, T> {
        ArcRwLockWriteGuard {
            rwlock: self.clone(),
            guard: ManuallyDrop::new(guard),
        }
    }

    /// Locks this `RwLock` with read access, through an `Arc`.
    ///
    /// This method is similar to the `read` method; however, it requires the `RwLock` to be inside of an `Arc`
    /// and the resulting read guard owns a clone of the `Arc` instead of borrowing the lock.
    ///
    /// # Panics
    ///
    /// This method may panic if the maximum number of readers supported by the
    /// raw lock has been reached.
    #[cfg(feature = "arc_lock")]
    #[inline]
    #[track_caller]
    pub fn read_arc(self: &Arc<Self>) -> ArcRwLockReadGuard<R, T> {
        let guard = self.raw.lock_shared();
        // SAFETY: locking guarantee is upheld
        unsafe { self.make_arc_read_guard_unchecked(guard) }
    }

    /// Attempts to lock this `RwLock` with read access, through an `Arc`.
    ///
    /// This method is similar to the `try_read` method; however, it requires the `RwLock` to be inside of an
    /// `Arc` and the resulting read guard owns a clone of the `Arc` instead of borrowing the lock.
    ///
    /// # Panics
    ///
    /// This method may panic if the maximum number of readers supported by the
    /// raw lock has been reached.
    #[cfg(feature = "arc_lock")]
    #[inline]
    #[track_caller]
    pub fn try_read_arc(self: &Arc<Self>) -> Option<ArcRwLockReadGuard<R, T>> {
        if let Some(guard) = self.raw.try_lock_shared() {
            // SAFETY: locking guarantee is upheld
            Some(unsafe { self.make_arc_read_guard_unchecked(guard) })
        } else {
            None
        }
    }

    /// Locks this `RwLock` with write access, through an `Arc`.
    ///
    /// This method is similar to the `write` method; however, it requires the `RwLock` to be inside of an `Arc`
    /// and the resulting write guard owns a clone of the `Arc` instead of borrowing the lock.
    #[cfg(feature = "arc_lock")]
    #[inline]
    #[track_caller]
    pub fn write_arc(self: &Arc<Self>) -> ArcRwLockWriteGuard<R, T> {
        let guard = self.raw.lock_exclusive();
        // SAFETY: locking guarantee is upheld
        unsafe { self.make_arc_write_guard_unchecked(guard) }
    }

    /// Attempts to lock this `RwLock` with write access, through an `Arc`.
    ///
    /// This method is similar to the `try_write` method; however, it requires the `RwLock` to be inside of an
    /// `Arc` and the resulting write guard owns a clone of the `Arc` instead of borrowing the lock.
    #[cfg(feature = "arc_lock")]
    #[inline]
    #[track_caller]
    pub fn try_write_arc(self: &Arc<Self>) -> Option<ArcRwLockWriteGuard<R, T>> {
        if let Some(guard) = self.raw.try_lock_exclusive() {
            // SAFETY: locking guarantee is upheld
            Some(unsafe { self.make_arc_write_guard_unchecked(guard) })
        } else {
            None
        }
    }
}

impl<R: RawRwLockFair, T: ?Sized> RwLock<R, T> {
    /// Forcibly unlocks a read lock using a fair unlock protocol.
    ///
    /// This is useful when the raw acquisition state is managed separately,
    /// for example when dealing with FFI.
    ///
    /// # Safety
    ///
    /// The current context must hold the lock using the supplied acquisition
    /// state, with no live guard responsible for unlocking it.
    /// Behavior is undefined if a rwlock is read-unlocked when not read-locked.
    ///
    /// The caller must ensure that releasing the lock does not invalidate any
    /// outstanding references to the protected data. Any subsequent access
    /// through previously obtained pointers must be properly synchronized.
    ///
    /// `guard` must be the state for this acquisition of this lock.
    #[inline]
    #[track_caller]
    pub unsafe fn force_unlock_read_fair(&self, guard: R::SharedGuard) {
        unsafe { self.raw.unlock_shared_fair(guard) };
    }

    /// Forcibly unlocks a write lock using a fair unlock protocol.
    ///
    /// This is useful when the raw acquisition state is managed separately,
    /// for example when dealing with FFI.
    ///
    /// # Safety
    ///
    /// The current context must hold the lock using the supplied acquisition
    /// state, with no live guard responsible for unlocking it.
    /// Behavior is undefined if a rwlock is write-unlocked when not write-locked.
    ///
    /// The caller must ensure that releasing the lock does not invalidate any
    /// outstanding references to the protected data. Any subsequent access
    /// through previously obtained pointers must be properly synchronized.
    ///
    /// `guard` must be the state for this acquisition of this lock.
    #[inline]
    #[track_caller]
    pub unsafe fn force_unlock_write_fair(&self, guard: R::ExclusiveGuard) {
        unsafe { self.raw.unlock_exclusive_fair(guard) };
    }
}

impl<R: RawRwLockTimed, T: ?Sized> RwLock<R, T> {
    /// Attempts to acquire this `RwLock` with shared read access until a timeout
    /// is reached.
    ///
    /// If the access could not be granted before the timeout expires, then
    /// `None` is returned. Otherwise, an RAII guard is returned which will
    /// release the shared access when it is dropped.
    ///
    /// See [`RawRwLockTimed::try_lock_shared_for`] for timeout behavior.
    ///
    /// # Panics
    ///
    /// This method may panic if the maximum number of readers supported by the
    /// raw lock has been reached.
    #[inline]
    #[track_caller]
    pub fn try_read_for(&self, timeout: R::Duration) -> Option<RwLockReadGuard<'_, R, T>> {
        if let Some(guard) = self.raw.try_lock_shared_for(timeout) {
            // SAFETY: The lock is held, as required.
            Some(unsafe { self.make_read_guard_unchecked(guard) })
        } else {
            None
        }
    }

    /// Attempts to acquire this `RwLock` with shared read access until a timeout
    /// is reached.
    ///
    /// If the access could not be granted before the timeout expires, then
    /// `None` is returned. Otherwise, an RAII guard is returned which will
    /// release the shared access when it is dropped.
    ///
    /// See [`RawRwLockTimed::try_lock_shared_until`] for timeout behavior.
    ///
    /// # Panics
    ///
    /// This method may panic if the maximum number of readers supported by the
    /// raw lock has been reached.
    #[inline]
    #[track_caller]
    pub fn try_read_until(&self, timeout: R::Instant) -> Option<RwLockReadGuard<'_, R, T>> {
        if let Some(guard) = self.raw.try_lock_shared_until(timeout) {
            // SAFETY: The lock is held, as required.
            Some(unsafe { self.make_read_guard_unchecked(guard) })
        } else {
            None
        }
    }

    /// Attempts to acquire this `RwLock` with exclusive write access until a
    /// timeout is reached.
    ///
    /// If the access could not be granted before the timeout expires, then
    /// `None` is returned. Otherwise, an RAII guard is returned which will
    /// release the exclusive access when it is dropped.
    ///
    /// See [`RawRwLockTimed::try_lock_exclusive_for`] for timeout behavior.
    #[inline]
    #[track_caller]
    pub fn try_write_for(&self, timeout: R::Duration) -> Option<RwLockWriteGuard<'_, R, T>> {
        if let Some(guard) = self.raw.try_lock_exclusive_for(timeout) {
            // SAFETY: The lock is held, as required.
            Some(unsafe { self.make_write_guard_unchecked(guard) })
        } else {
            None
        }
    }

    /// Attempts to acquire this `RwLock` with exclusive write access until a
    /// timeout is reached.
    ///
    /// If the access could not be granted before the timeout expires, then
    /// `None` is returned. Otherwise, an RAII guard is returned which will
    /// release the exclusive access when it is dropped.
    ///
    /// See [`RawRwLockTimed::try_lock_exclusive_until`] for timeout behavior.
    #[inline]
    #[track_caller]
    pub fn try_write_until(&self, timeout: R::Instant) -> Option<RwLockWriteGuard<'_, R, T>> {
        if let Some(guard) = self.raw.try_lock_exclusive_until(timeout) {
            // SAFETY: The lock is held, as required.
            Some(unsafe { self.make_write_guard_unchecked(guard) })
        } else {
            None
        }
    }

    /// Attempts to acquire this `RwLock` with read access until a timeout is reached, through an `Arc`.
    ///
    /// This method is similar to the `try_read_for` method; however, it requires the `RwLock` to be inside of an
    /// `Arc` and the resulting read guard owns a clone of the `Arc` instead of borrowing the lock.
    ///
    /// See [`try_read_for`](Self::try_read_for) for timeout behavior.
    ///
    /// # Panics
    ///
    /// This method may panic if the maximum number of readers supported by the
    /// raw lock has been reached.
    #[cfg(feature = "arc_lock")]
    #[inline]
    #[track_caller]
    pub fn try_read_arc_for(
        self: &Arc<Self>,
        timeout: R::Duration,
    ) -> Option<ArcRwLockReadGuard<R, T>> {
        if let Some(guard) = self.raw.try_lock_shared_for(timeout) {
            // SAFETY: locking guarantee is upheld
            Some(unsafe { self.make_arc_read_guard_unchecked(guard) })
        } else {
            None
        }
    }

    /// Attempts to acquire this `RwLock` with read access until a timeout is reached, through an `Arc`.
    ///
    /// This method is similar to the `try_read_until` method; however, it requires the `RwLock` to be inside of
    /// an `Arc` and the resulting read guard owns a clone of the `Arc` instead of borrowing the lock.
    ///
    /// See [`try_read_until`](Self::try_read_until) for timeout behavior.
    ///
    /// # Panics
    ///
    /// This method may panic if the maximum number of readers supported by the
    /// raw lock has been reached.
    #[cfg(feature = "arc_lock")]
    #[inline]
    #[track_caller]
    pub fn try_read_arc_until(
        self: &Arc<Self>,
        timeout: R::Instant,
    ) -> Option<ArcRwLockReadGuard<R, T>> {
        if let Some(guard) = self.raw.try_lock_shared_until(timeout) {
            // SAFETY: locking guarantee is upheld
            Some(unsafe { self.make_arc_read_guard_unchecked(guard) })
        } else {
            None
        }
    }

    /// Attempts to acquire this `RwLock` with write access until a timeout is reached, through an `Arc`.
    ///
    /// This method is similar to the `try_write_for` method; however, it requires the `RwLock` to be inside of
    /// an `Arc` and the resulting write guard owns a clone of the `Arc` instead of borrowing the lock.
    ///
    /// See [`try_write_for`](Self::try_write_for) for timeout behavior.
    #[cfg(feature = "arc_lock")]
    #[inline]
    #[track_caller]
    pub fn try_write_arc_for(
        self: &Arc<Self>,
        timeout: R::Duration,
    ) -> Option<ArcRwLockWriteGuard<R, T>> {
        if let Some(guard) = self.raw.try_lock_exclusive_for(timeout) {
            // SAFETY: locking guarantee is upheld
            Some(unsafe { self.make_arc_write_guard_unchecked(guard) })
        } else {
            None
        }
    }

    /// Attempts to acquire this `RwLock` with write access until a timeout is reached, through an `Arc`.
    ///
    /// This method is similar to the `try_write_until` method; however, it requires the `RwLock` to be inside of
    /// an `Arc` and the resulting write guard owns a clone of the `Arc` instead of borrowing the lock.
    ///
    /// See [`try_write_until`](Self::try_write_until) for timeout behavior.
    #[cfg(feature = "arc_lock")]
    #[inline]
    #[track_caller]
    pub fn try_write_arc_until(
        self: &Arc<Self>,
        timeout: R::Instant,
    ) -> Option<ArcRwLockWriteGuard<R, T>> {
        if let Some(guard) = self.raw.try_lock_exclusive_until(timeout) {
            // SAFETY: locking guarantee is upheld
            Some(unsafe { self.make_arc_write_guard_unchecked(guard) })
        } else {
            None
        }
    }
}

impl<R: RawRwLockUpgrade, T: ?Sized> RwLock<R, T> {
    /// Creates a new `RwLockUpgradableReadGuard` without checking if the lock is held.
    ///
    /// # Safety
    ///
    /// This method must only be called if the thread logically holds an upgradable read lock.
    ///
    /// This function does not increment the read count of the lock. Calling this function when a
    /// guard has already been produced is undefined behavior unless the guard was forgotten
    /// with `mem::forget`.
    ///
    /// The caller must ensure that existing references to the protected data
    /// remain valid when the returned guard is used or dropped. In particular,
    /// the returned guard must not permit accesses that conflict with existing
    /// references.
    ///
    /// `guard` must be the state for this acquisition of this lock.
    #[inline]
    pub unsafe fn make_upgradable_guard_unchecked(
        &self,
        guard: R::UpgradableGuard,
    ) -> RwLockUpgradableReadGuard<'_, R, T> {
        RwLockUpgradableReadGuard {
            rwlock: self,
            marker: PhantomData,
            guard: ManuallyDrop::new(guard),
        }
    }

    /// Locks this `RwLock` with upgradable read access, blocking the current thread
    /// until it can be acquired.
    ///
    /// The calling thread will be blocked until there are no more writers or other
    /// upgradable reads which hold the lock. There may be other readers currently
    /// inside the lock when this method returns.
    ///
    /// Returns an RAII guard which will release this thread's shared access
    /// once it is dropped.
    ///
    /// # Panics
    ///
    /// This method may panic if the maximum number of readers supported by the
    /// raw lock has been reached.
    #[inline]
    #[track_caller]
    pub fn upgradable_read(&self) -> RwLockUpgradableReadGuard<'_, R, T> {
        let guard = self.raw.lock_upgradable();
        // SAFETY: The lock is held, as required.
        unsafe { self.make_upgradable_guard_unchecked(guard) }
    }

    /// Attempts to acquire this `RwLock` with upgradable read access.
    ///
    /// If the access could not be granted at this time, then `None` is returned.
    /// Otherwise, an RAII guard is returned which will release the shared access
    /// when it is dropped.
    ///
    /// This function does not block.
    ///
    /// # Panics
    ///
    /// This method may panic if the maximum number of readers supported by the
    /// raw lock has been reached.
    #[inline]
    #[track_caller]
    pub fn try_upgradable_read(&self) -> Option<RwLockUpgradableReadGuard<'_, R, T>> {
        if let Some(guard) = self.raw.try_lock_upgradable() {
            // SAFETY: The lock is held, as required.
            Some(unsafe { self.make_upgradable_guard_unchecked(guard) })
        } else {
            None
        }
    }

    /// Creates a new `ArcRwLockUpgradableReadGuard` without checking if the lock is held.
    ///
    /// # Safety
    ///
    /// This method must only be called if the thread logically holds an upgradable read lock.
    ///
    /// This function does not increment the read count of the lock. Calling this function when a
    /// guard has already been produced is undefined behavior unless the guard was forgotten
    /// with `mem::forget`.
    ///
    /// The caller must ensure that existing references to the protected data
    /// remain valid when the returned guard is used or dropped. In particular,
    /// the returned guard must not permit accesses that conflict with existing
    /// references.
    ///
    /// `guard` must be the state for this acquisition of this lock.
    #[cfg(feature = "arc_lock")]
    #[inline]
    pub unsafe fn make_upgradable_arc_guard_unchecked(
        self: &Arc<Self>,
        guard: R::UpgradableGuard,
    ) -> ArcRwLockUpgradableReadGuard<R, T> {
        ArcRwLockUpgradableReadGuard {
            rwlock: self.clone(),
            guard: ManuallyDrop::new(guard),
        }
    }

    /// Locks this `RwLock` with upgradable read access, through an `Arc`.
    ///
    /// This method is similar to the `upgradable_read` method; however, it requires the `RwLock` to be
    /// inside of an `Arc` and the resulting read guard owns a clone of the `Arc` instead of borrowing the lock.
    ///
    /// # Panics
    ///
    /// This method may panic if the maximum number of readers supported by the
    /// raw lock has been reached.
    #[cfg(feature = "arc_lock")]
    #[inline]
    #[track_caller]
    pub fn upgradable_read_arc(self: &Arc<Self>) -> ArcRwLockUpgradableReadGuard<R, T> {
        let guard = self.raw.lock_upgradable();
        // SAFETY: locking guarantee is upheld
        unsafe { self.make_upgradable_arc_guard_unchecked(guard) }
    }

    /// Attempts to lock this `RwLock` with upgradable read access, through an `Arc`.
    ///
    /// This method is similar to the `try_upgradable_read` method; however, it requires the `RwLock` to be
    /// inside of an `Arc` and the resulting read guard owns a clone of the `Arc` instead of borrowing the lock.
    ///
    /// # Panics
    ///
    /// This method may panic if the maximum number of readers supported by the
    /// raw lock has been reached.
    #[cfg(feature = "arc_lock")]
    #[inline]
    #[track_caller]
    pub fn try_upgradable_read_arc(self: &Arc<Self>) -> Option<ArcRwLockUpgradableReadGuard<R, T>> {
        if let Some(guard) = self.raw.try_lock_upgradable() {
            // SAFETY: locking guarantee is upheld
            Some(unsafe { self.make_upgradable_arc_guard_unchecked(guard) })
        } else {
            None
        }
    }
}

impl<R: RawRwLockUpgradeTimed, T: ?Sized> RwLock<R, T> {
    /// Attempts to acquire this `RwLock` with upgradable read access until a timeout
    /// is reached.
    ///
    /// If the access could not be granted before the timeout expires, then
    /// `None` is returned. Otherwise, an RAII guard is returned which will
    /// release the shared access when it is dropped.
    ///
    /// See [`RawRwLockUpgradeTimed::try_lock_upgradable_for`] for timeout
    /// behavior.
    ///
    /// # Panics
    ///
    /// This method may panic if the maximum number of readers supported by the
    /// raw lock has been reached.
    #[inline]
    #[track_caller]
    pub fn try_upgradable_read_for(
        &self,
        timeout: R::Duration,
    ) -> Option<RwLockUpgradableReadGuard<'_, R, T>> {
        if let Some(guard) = self.raw.try_lock_upgradable_for(timeout) {
            // SAFETY: The lock is held, as required.
            Some(unsafe { self.make_upgradable_guard_unchecked(guard) })
        } else {
            None
        }
    }

    /// Attempts to acquire this `RwLock` with upgradable read access until a timeout
    /// is reached.
    ///
    /// If the access could not be granted before the timeout expires, then
    /// `None` is returned. Otherwise, an RAII guard is returned which will
    /// release the shared access when it is dropped.
    ///
    /// See [`RawRwLockUpgradeTimed::try_lock_upgradable_until`] for timeout
    /// behavior.
    ///
    /// # Panics
    ///
    /// This method may panic if the maximum number of readers supported by the
    /// raw lock has been reached.
    #[inline]
    #[track_caller]
    pub fn try_upgradable_read_until(
        &self,
        timeout: R::Instant,
    ) -> Option<RwLockUpgradableReadGuard<'_, R, T>> {
        if let Some(guard) = self.raw.try_lock_upgradable_until(timeout) {
            // SAFETY: The lock is held, as required.
            Some(unsafe { self.make_upgradable_guard_unchecked(guard) })
        } else {
            None
        }
    }

    /// Attempts to lock this `RwLock` with upgradable access until a timeout is reached, through an `Arc`.
    ///
    /// This method is similar to the `try_upgradable_read_for` method; however, it requires the `RwLock` to be
    /// inside of an `Arc` and the resulting read guard owns a clone of the `Arc` instead of borrowing the lock.
    ///
    /// # Panics
    ///
    /// This method may panic if the maximum number of readers supported by the
    /// raw lock has been reached.
    #[cfg(feature = "arc_lock")]
    #[inline]
    #[track_caller]
    pub fn try_upgradable_read_arc_for(
        self: &Arc<Self>,
        timeout: R::Duration,
    ) -> Option<ArcRwLockUpgradableReadGuard<R, T>> {
        if let Some(guard) = self.raw.try_lock_upgradable_for(timeout) {
            // SAFETY: locking guarantee is upheld
            Some(unsafe { self.make_upgradable_arc_guard_unchecked(guard) })
        } else {
            None
        }
    }

    /// Attempts to lock this `RwLock` with upgradable access until a timeout is reached, through an `Arc`.
    ///
    /// This method is similar to the `try_upgradable_read_until` method; however, it requires the `RwLock` to be
    /// inside of an `Arc` and the resulting read guard owns a clone of the `Arc` instead of borrowing the lock.
    ///
    /// # Panics
    ///
    /// This method may panic if the maximum number of readers supported by the
    /// raw lock has been reached.
    #[cfg(feature = "arc_lock")]
    #[inline]
    #[track_caller]
    pub fn try_upgradable_read_arc_until(
        self: &Arc<Self>,
        timeout: R::Instant,
    ) -> Option<ArcRwLockUpgradableReadGuard<R, T>> {
        if let Some(guard) = self.raw.try_lock_upgradable_until(timeout) {
            // SAFETY: locking guarantee is upheld
            Some(unsafe { self.make_upgradable_arc_guard_unchecked(guard) })
        } else {
            None
        }
    }
}

impl<R: RawRwLock, T: Default> Default for RwLock<R, T> {
    #[inline]
    fn default() -> RwLock<R, T> {
        RwLock::new(Default::default())
    }
}

impl<R: RawRwLock, T> From<T> for RwLock<R, T> {
    #[inline]
    fn from(t: T) -> RwLock<R, T> {
        RwLock::new(t)
    }
}

impl<R: RawRwLock, T: ?Sized + fmt::Debug> fmt::Debug for RwLock<R, T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let mut d = f.debug_struct("RwLock");
        match self.try_read() {
            Some(guard) => d.field("data", &&*guard),
            None => {
                // Additional format_args! here is to remove quotes around <locked> in debug output.
                d.field("data", &format_args!("<locked>"))
            }
        };
        d.finish()
    }
}

/// An RAII guard which releases shared read access when dropped.
///
/// This structure is created by the [`read`](RwLock::read) and
/// [`try_read`](RwLock::try_read) methods on [`RwLock`].
#[clippy::has_significant_drop]
#[must_use = "if unused the RwLock will immediately unlock"]
pub struct RwLockReadGuard<'a, R: RawRwLock, T: ?Sized> {
    rwlock: &'a RwLock<R, T>,
    marker: PhantomData<&'a T>,
    // The raw unlock operation consumes this state in Drop.
    guard: ManuallyDrop<R::SharedGuard>,
}

impl<'a, R: RawRwLock + 'a, T: ?Sized + 'a> RwLockReadGuard<'a, R, T> {
    /// Returns a reference to the original reader-writer lock object.
    pub fn rwlock(s: &Self) -> &'a RwLock<R, T> {
        s.rwlock
    }

    /// Makes a new `MappedRwLockReadGuard` for a component of the locked data.
    ///
    /// This is an associated function that needs to be
    /// used as `RwLockReadGuard::map(...)`. A method would interfere with methods of
    /// the same name on the contents of the locked data.
    ///
    /// # Panics
    ///
    /// If `f` panics, the original guard is dropped.
    #[inline]
    pub fn map<U: ?Sized, F>(s: Self, f: F) -> MappedRwLockReadGuard<'a, R, U>
    where
        F: FnOnce(&T) -> &U,
    {
        let raw = &s.rwlock.raw;
        let data = f(unsafe { s.rwlock.data.get().as_ref_unchecked() });
        let guard = unsafe { ptr::read(&s.guard) };
        mem::forget(s);
        MappedRwLockReadGuard {
            raw,
            data: SharedGuardData::new(data),
            guard,
        }
    }

    /// Attempts to make a new `MappedRwLockReadGuard` for a component of the
    /// locked data. Returns the original guard if the closure returns `None`.
    ///
    /// This is an associated function that needs to be
    /// used as `RwLockReadGuard::try_map(...)`. A method would interfere with methods of
    /// the same name on the contents of the locked data.
    ///
    /// # Panics
    ///
    /// If `f` panics, the original guard is dropped.
    #[inline]
    pub fn try_map<U: ?Sized, F>(s: Self, f: F) -> Result<MappedRwLockReadGuard<'a, R, U>, Self>
    where
        F: FnOnce(&T) -> Option<&U>,
    {
        let raw = &s.rwlock.raw;
        let Some(data) = f(unsafe { s.rwlock.data.get().as_ref_unchecked() }) else {
            return Err(s);
        };
        let guard = unsafe { ptr::read(&s.guard) };
        mem::forget(s);
        Ok(MappedRwLockReadGuard {
            raw,
            data: SharedGuardData::new(data),
            guard,
        })
    }

    /// Attempts to make a new `MappedRwLockReadGuard` for a component of the
    /// locked data. The original guard is returned alongside arbitrary user data
    /// if the closure returns `Err`.
    ///
    /// This is an associated function that needs to be
    /// used as `RwLockReadGuard::try_map_or_err(...)`. A method would interfere with methods of
    /// the same name on the contents of the locked data.
    ///
    /// # Panics
    ///
    /// If `f` panics, the original guard is dropped.
    #[inline]
    pub fn try_map_or_err<U: ?Sized, F, E>(
        s: Self,
        f: F,
    ) -> Result<MappedRwLockReadGuard<'a, R, U>, (Self, E)>
    where
        F: FnOnce(&T) -> Result<&U, E>,
    {
        let raw = &s.rwlock.raw;
        let data = match f(unsafe { s.rwlock.data.get().as_ref_unchecked() }) {
            Ok(data) => data,
            Err(e) => return Err((s, e)),
        };
        let guard = unsafe { ptr::read(&s.guard) };
        mem::forget(s);
        Ok(MappedRwLockReadGuard {
            raw,
            data: SharedGuardData::new(data),
            guard,
        })
    }

    /// Temporarily unlocks the `RwLock` to execute the given function.
    ///
    /// The mutable reference ensures that no references derived from this guard
    /// are live while the lock is temporarily released.
    ///
    /// # Aborts
    ///
    /// Aborts if re-locking the `RwLock` panics.
    #[inline]
    #[track_caller]
    pub fn unlocked<F, U>(s: &mut Self, f: F) -> U
    where
        F: FnOnce() -> U,
    {
        // SAFETY: Valid shared state; cleanup restores it before reuse or drop.
        let state = unsafe { ManuallyDrop::take(&mut s.guard) };
        // A panicking raw unlock releases the acquisition too.
        defer!(abort_on_panic(
            || s.guard = ManuallyDrop::new(s.rwlock.raw.lock_shared())
        ));
        // SAFETY: The state belongs to the acquisition held by this guard.
        unsafe { s.rwlock.raw.unlock_shared(state) };
        f()
    }
}

impl<'a, R: RawRwLockFair + 'a, T: ?Sized + 'a> RwLockReadGuard<'a, R, T> {
    /// Unlocks the `RwLock` using a fair unlock protocol.
    ///
    /// A fair unlock gives waiting threads priority over new acquisition attempts.
    /// This can prevent starvation, but may reduce throughput by forcing a context
    /// switch.
    #[inline]
    #[track_caller]
    pub fn unlock_fair(s: Self) {
        let mut s = ManuallyDrop::new(s);
        // Safety: An RwLockReadGuard always holds a shared lock.
        unsafe {
            s.rwlock
                .raw
                .unlock_shared_fair(ManuallyDrop::take(&mut s.guard));
        }
    }

    /// Temporarily unlocks the `RwLock` to execute the given function.
    ///
    /// The `RwLock` is unlocked using a fair unlock protocol.
    ///
    /// The mutable reference ensures that no references derived from this guard
    /// are live while the lock is temporarily released.
    ///
    /// # Aborts
    ///
    /// Aborts if re-locking the `RwLock` panics.
    #[inline]
    #[track_caller]
    pub fn unlocked_fair<F, U>(s: &mut Self, f: F) -> U
    where
        F: FnOnce() -> U,
    {
        // SAFETY: Valid shared state; cleanup restores it before reuse or drop.
        let state = unsafe { ManuallyDrop::take(&mut s.guard) };
        // A panicking raw unlock releases the acquisition too.
        defer!(abort_on_panic(
            || s.guard = ManuallyDrop::new(s.rwlock.raw.lock_shared())
        ));
        // SAFETY: The state belongs to the acquisition held by this guard.
        unsafe { s.rwlock.raw.unlock_shared_fair(state) };
        f()
    }

    /// Temporarily yields the `RwLock` to a waiting thread if there is one.
    ///
    /// This method is functionally equivalent to calling `unlock_fair` followed
    /// by `read`, however it can be much more efficient in the case where there
    /// are no waiting threads.
    #[inline]
    #[track_caller]
    pub fn bump(s: &mut Self) {
        // SAFETY: Valid shared state; bump preserves ownership even on unwind.
        unsafe { s.rwlock.raw.bump_shared(&mut s.guard) };
    }
}

impl<'a, R: RawRwLock + 'a, T: ?Sized + 'a> Deref for RwLockReadGuard<'a, R, T> {
    type Target = T;
    #[inline]
    fn deref(&self) -> &T {
        unsafe { self.rwlock.data.get().as_ref_unchecked() }
    }
}

impl<'a, R: RawRwLock + 'a, T: ?Sized + 'a> Drop for RwLockReadGuard<'a, R, T> {
    #[inline]
    fn drop(&mut self) {
        // Safety: An RwLockReadGuard always holds a shared lock.
        unsafe {
            self.rwlock
                .raw
                .unlock_shared(ManuallyDrop::take(&mut self.guard));
        }
    }
}

impl<'a, R: RawRwLock + 'a, T: fmt::Debug + ?Sized + 'a> fmt::Debug for RwLockReadGuard<'a, R, T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Debug::fmt(&**self, f)
    }
}

impl<'a, R: RawRwLock + 'a, T: fmt::Display + ?Sized + 'a> fmt::Display
    for RwLockReadGuard<'a, R, T>
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        (**self).fmt(f)
    }
}

#[cfg(feature = "owning_ref")]
unsafe impl<'a, R: RawRwLock + 'a, T: ?Sized + 'a> StableAddress for RwLockReadGuard<'a, R, T> {}

/// An RAII read guard returned by the `Arc` locking operations on [`RwLock`].
///
/// Unlike [`RwLockReadGuard`], this guard owns the [`Arc`] used to unlock the
/// reader-writer lock instead of borrowing it, so its lifetime is not tied to a
/// lock reference.
#[cfg(feature = "arc_lock")]
#[clippy::has_significant_drop]
#[must_use = "if unused the RwLock will immediately unlock"]
pub struct ArcRwLockReadGuard<R: RawRwLock, T: ?Sized> {
    // The raw unlock operation consumes this state in Drop.
    guard: ManuallyDrop<R::SharedGuard>,
    rwlock: Arc<RwLock<R, T>>,
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLock, T: ?Sized> ArcRwLockReadGuard<R, T> {
    /// Returns a reference to the rwlock, contained in its `Arc`.
    pub fn rwlock(s: &Self) -> &Arc<RwLock<R, T>> {
        &s.rwlock
    }

    /// Unlocks the `RwLock` and returns the `Arc` that was held by the [`ArcRwLockReadGuard`].
    #[inline]
    pub fn into_arc(s: Self) -> Arc<RwLock<R, T>> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: ManuallyDrop lets us move the Arc into a local that drops on unwind.
        let rwlock = unsafe { ptr::read(&s.rwlock) };
        // SAFETY: Valid shared state; ManuallyDrop prevents a second release.
        let guard = unsafe { ManuallyDrop::take(&mut s.guard) };
        // SAFETY: The state belongs to the acquisition held by this guard.
        unsafe { rwlock.raw.unlock_shared(guard) };
        rwlock
    }

    /// Temporarily unlocks the `RwLock` to execute the given function.
    ///
    /// This is functionally identical to the `unlocked` method on [`RwLockReadGuard`].
    ///
    /// # Aborts
    ///
    /// Aborts if re-locking the `RwLock` panics.
    #[inline]
    #[track_caller]
    pub fn unlocked<F, U>(s: &mut Self, f: F) -> U
    where
        F: FnOnce() -> U,
    {
        // SAFETY: Valid shared state; cleanup restores it before reuse or drop.
        let state = unsafe { ManuallyDrop::take(&mut s.guard) };
        // A panicking raw unlock releases the acquisition too.
        defer!(abort_on_panic(
            || s.guard = ManuallyDrop::new(s.rwlock.raw.lock_shared())
        ));
        // SAFETY: The state belongs to the acquisition held by this guard.
        unsafe { s.rwlock.raw.unlock_shared(state) };
        f()
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLockFair, T: ?Sized> ArcRwLockReadGuard<R, T> {
    /// Unlocks the `RwLock` using a fair unlock protocol.
    ///
    /// This is functionally identical to the `unlock_fair` method on [`RwLockReadGuard`].
    #[inline]
    #[track_caller]
    pub fn unlock_fair(s: Self) {
        drop(Self::into_arc_fair(s));
    }

    /// Unlocks the `RwLock` using a fair unlock protocol and returns the `Arc` that was held by the [`ArcRwLockReadGuard`].
    #[inline]
    pub fn into_arc_fair(s: Self) -> Arc<RwLock<R, T>> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: ManuallyDrop lets us move the Arc into a local that drops on unwind.
        let rwlock = unsafe { ptr::read(&s.rwlock) };
        // SAFETY: Valid shared state; ManuallyDrop prevents a second release.
        let guard = unsafe { ManuallyDrop::take(&mut s.guard) };
        // SAFETY: The state belongs to the acquisition held by this guard.
        unsafe { rwlock.raw.unlock_shared_fair(guard) };
        rwlock
    }

    /// Temporarily unlocks the `RwLock` to execute the given function.
    ///
    /// This is functionally identical to the `unlocked_fair` method on [`RwLockReadGuard`].
    ///
    /// # Aborts
    ///
    /// Aborts if re-locking the `RwLock` panics.
    #[inline]
    #[track_caller]
    pub fn unlocked_fair<F, U>(s: &mut Self, f: F) -> U
    where
        F: FnOnce() -> U,
    {
        // SAFETY: Valid shared state; cleanup restores it before reuse or drop.
        let state = unsafe { ManuallyDrop::take(&mut s.guard) };
        // A panicking raw unlock releases the acquisition too.
        defer!(abort_on_panic(
            || s.guard = ManuallyDrop::new(s.rwlock.raw.lock_shared())
        ));
        // SAFETY: The state belongs to the acquisition held by this guard.
        unsafe { s.rwlock.raw.unlock_shared_fair(state) };
        f()
    }

    /// Temporarily yields the `RwLock` to a waiting thread if there is one.
    ///
    /// This is functionally identical to the `bump` method on [`RwLockReadGuard`].
    #[inline]
    #[track_caller]
    pub fn bump(s: &mut Self) {
        // SAFETY: Valid shared state; bump preserves ownership even on unwind.
        unsafe { s.rwlock.raw.bump_shared(&mut s.guard) };
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLock, T: ?Sized> Deref for ArcRwLockReadGuard<R, T> {
    type Target = T;
    #[inline]
    fn deref(&self) -> &T {
        unsafe { self.rwlock.data.get().as_ref_unchecked() }
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLock, T: ?Sized> Drop for ArcRwLockReadGuard<R, T> {
    #[inline]
    fn drop(&mut self) {
        // Safety: An RwLockReadGuard always holds a shared lock.
        unsafe {
            self.rwlock
                .raw
                .unlock_shared(ManuallyDrop::take(&mut self.guard));
        }
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLock, T: fmt::Debug + ?Sized> fmt::Debug for ArcRwLockReadGuard<R, T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Debug::fmt(&**self, f)
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLock, T: fmt::Display + ?Sized> fmt::Display for ArcRwLockReadGuard<R, T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        (**self).fmt(f)
    }
}

/// An RAII guard which releases exclusive write access when dropped.
///
/// This structure is created by the [`write`](RwLock::write) and
/// [`try_write`](RwLock::try_write) methods on [`RwLock`].
#[clippy::has_significant_drop]
#[must_use = "if unused the RwLock will immediately unlock"]
pub struct RwLockWriteGuard<'a, R: RawRwLock, T: ?Sized> {
    rwlock: &'a RwLock<R, T>,
    marker: PhantomData<&'a mut T>,
    // The raw unlock operation consumes this state in Drop.
    guard: ManuallyDrop<R::ExclusiveGuard>,
}

impl<'a, R: RawRwLock + 'a, T: ?Sized + 'a> RwLockWriteGuard<'a, R, T> {
    /// Returns a reference to the original reader-writer lock object.
    pub fn rwlock(s: &Self) -> &'a RwLock<R, T> {
        s.rwlock
    }

    /// Makes a new `MappedRwLockWriteGuard` for a component of the locked data.
    ///
    /// This is an associated function that needs to be
    /// used as `RwLockWriteGuard::map(...)`. A method would interfere with methods of
    /// the same name on the contents of the locked data.
    ///
    /// # Panics
    ///
    /// If `f` panics, the original guard is dropped.
    #[inline]
    pub fn map<U: ?Sized, F>(s: Self, f: F) -> MappedRwLockWriteGuard<'a, R, U>
    where
        F: FnOnce(&mut T) -> &mut U,
    {
        let raw = &s.rwlock.raw;
        let data = f(unsafe { s.rwlock.data.get().as_mut_unchecked() });
        let guard = unsafe { ptr::read(&s.guard) };
        mem::forget(s);
        MappedRwLockWriteGuard {
            raw,
            data: ExclusiveGuardData::new(data),
            guard,
        }
    }

    /// Attempts to make a new `MappedRwLockWriteGuard` for a component of the
    /// locked data. The original guard is returned if the closure returns `None`.
    ///
    /// This is an associated function that needs to be
    /// used as `RwLockWriteGuard::try_map(...)`. A method would interfere with methods of
    /// the same name on the contents of the locked data.
    ///
    /// # Panics
    ///
    /// If `f` panics, the original guard is dropped.
    #[inline]
    pub fn try_map<U: ?Sized, F>(s: Self, f: F) -> Result<MappedRwLockWriteGuard<'a, R, U>, Self>
    where
        F: FnOnce(&mut T) -> Option<&mut U>,
    {
        let raw = &s.rwlock.raw;
        let Some(data) = f(unsafe { s.rwlock.data.get().as_mut_unchecked() }) else {
            return Err(s);
        };
        let guard = unsafe { ptr::read(&s.guard) };
        mem::forget(s);
        Ok(MappedRwLockWriteGuard {
            raw,
            data: ExclusiveGuardData::new(data),
            guard,
        })
    }

    /// Attempts to make a new `MappedRwLockWriteGuard` for a component of the
    /// locked data. The original guard is returned alongside arbitrary user data
    /// if the closure returns `Err`.
    ///
    /// This is an associated function that needs to be
    /// used as `RwLockWriteGuard::try_map_or_err(...)`. A method would interfere with methods of
    /// the same name on the contents of the locked data.
    ///
    /// # Panics
    ///
    /// If `f` panics, the original guard is dropped.
    #[inline]
    pub fn try_map_or_err<U: ?Sized, F, E>(
        s: Self,
        f: F,
    ) -> Result<MappedRwLockWriteGuard<'a, R, U>, (Self, E)>
    where
        F: FnOnce(&mut T) -> Result<&mut U, E>,
    {
        let raw = &s.rwlock.raw;
        let data = match f(unsafe { s.rwlock.data.get().as_mut_unchecked() }) {
            Ok(data) => data,
            Err(e) => return Err((s, e)),
        };
        let guard = unsafe { ptr::read(&s.guard) };
        mem::forget(s);
        Ok(MappedRwLockWriteGuard {
            raw,
            data: ExclusiveGuardData::new(data),
            guard,
        })
    }

    /// Temporarily unlocks the `RwLock` to execute the given function.
    ///
    /// This is safe because `&mut` guarantees that there exist no other
    /// references to the data protected by the `RwLock`.
    ///
    /// # Aborts
    ///
    /// Aborts if re-locking the `RwLock` panics.
    #[inline]
    #[track_caller]
    pub fn unlocked<F, U>(s: &mut Self, f: F) -> U
    where
        F: FnOnce() -> U,
    {
        // SAFETY: Valid exclusive state; cleanup restores it before reuse or drop.
        let state = unsafe { ManuallyDrop::take(&mut s.guard) };
        // A panicking raw unlock releases the acquisition too.
        defer!(abort_on_panic(
            || s.guard = ManuallyDrop::new(s.rwlock.raw.lock_exclusive())
        ));
        // SAFETY: The state belongs to the acquisition held by this guard.
        unsafe { s.rwlock.raw.unlock_exclusive(state) };
        f()
    }
}

impl<'a, R: RawRwLockDowngrade + 'a, T: ?Sized + 'a> RwLockWriteGuard<'a, R, T> {
    /// Atomically downgrades a write lock into a read lock without allowing any
    /// writers to take exclusive access of the lock in the meantime.
    ///
    /// Note that if there are any writers currently waiting to take the lock
    /// then other readers may not be able to acquire the lock even if it was
    /// downgraded.
    #[track_caller]
    pub fn downgrade(s: Self) -> RwLockReadGuard<'a, R, T> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: Valid exclusive state; ManuallyDrop prevents a second release.
        let guard = unsafe { s.rwlock.raw.downgrade(ManuallyDrop::take(&mut s.guard)) };

        RwLockReadGuard {
            rwlock: s.rwlock,
            guard: ManuallyDrop::new(guard),
            marker: PhantomData,
        }
    }
}

impl<'a, R: RawRwLockUpgradeDowngrade + 'a, T: ?Sized + 'a> RwLockWriteGuard<'a, R, T> {
    /// Atomically downgrades a write lock into an upgradable read lock without allowing any
    /// writers to take exclusive access of the lock in the meantime.
    ///
    /// Note that if there are any writers currently waiting to take the lock
    /// then other readers may not be able to acquire the lock even if it was
    /// downgraded.
    #[track_caller]
    pub fn downgrade_to_upgradable(s: Self) -> RwLockUpgradableReadGuard<'a, R, T> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: Valid exclusive state; ManuallyDrop prevents a second release.
        let guard = unsafe {
            s.rwlock
                .raw
                .downgrade_to_upgradable(ManuallyDrop::take(&mut s.guard))
        };

        RwLockUpgradableReadGuard {
            rwlock: s.rwlock,
            guard: ManuallyDrop::new(guard),
            marker: PhantomData,
        }
    }
}

impl<'a, R: RawRwLockFair + 'a, T: ?Sized + 'a> RwLockWriteGuard<'a, R, T> {
    /// Unlocks the `RwLock` using a fair unlock protocol.
    ///
    /// A fair unlock gives waiting threads priority over new acquisition attempts.
    /// This can prevent starvation, but may reduce throughput by forcing a context
    /// switch.
    #[inline]
    #[track_caller]
    pub fn unlock_fair(s: Self) {
        let mut s = ManuallyDrop::new(s);
        // Safety: An RwLockWriteGuard always holds an exclusive lock.
        unsafe {
            s.rwlock
                .raw
                .unlock_exclusive_fair(ManuallyDrop::take(&mut s.guard));
        }
    }

    /// Temporarily unlocks the `RwLock` to execute the given function.
    ///
    /// The `RwLock` is unlocked using a fair unlock protocol.
    ///
    /// This is safe because `&mut` guarantees that there exist no other
    /// references to the data protected by the `RwLock`.
    ///
    /// # Aborts
    ///
    /// Aborts if re-locking the `RwLock` panics.
    #[inline]
    #[track_caller]
    pub fn unlocked_fair<F, U>(s: &mut Self, f: F) -> U
    where
        F: FnOnce() -> U,
    {
        // SAFETY: Valid exclusive state; cleanup restores it before reuse or drop.
        let state = unsafe { ManuallyDrop::take(&mut s.guard) };
        // A panicking raw unlock releases the acquisition too.
        defer!(abort_on_panic(
            || s.guard = ManuallyDrop::new(s.rwlock.raw.lock_exclusive())
        ));
        // SAFETY: The state belongs to the acquisition held by this guard.
        unsafe { s.rwlock.raw.unlock_exclusive_fair(state) };
        f()
    }

    /// Temporarily yields the `RwLock` to a waiting thread if there is one.
    ///
    /// This method is functionally equivalent to calling `unlock_fair` followed
    /// by `write`, however it can be much more efficient in the case where there
    /// are no waiting threads.
    #[inline]
    #[track_caller]
    pub fn bump(s: &mut Self) {
        // SAFETY: Valid exclusive state; bump preserves ownership even on unwind.
        unsafe { s.rwlock.raw.bump_exclusive(&mut s.guard) };
    }
}

impl<'a, R: RawRwLock + 'a, T: ?Sized + 'a> Deref for RwLockWriteGuard<'a, R, T> {
    type Target = T;
    #[inline]
    fn deref(&self) -> &T {
        unsafe { self.rwlock.data.get().as_ref_unchecked() }
    }
}

impl<'a, R: RawRwLock + 'a, T: ?Sized + 'a> DerefMut for RwLockWriteGuard<'a, R, T> {
    #[inline]
    fn deref_mut(&mut self) -> &mut T {
        unsafe { self.rwlock.data.get().as_mut_unchecked() }
    }
}

impl<'a, R: RawRwLock + 'a, T: ?Sized + 'a> Drop for RwLockWriteGuard<'a, R, T> {
    #[inline]
    fn drop(&mut self) {
        // Safety: An RwLockWriteGuard always holds an exclusive lock.
        unsafe {
            self.rwlock
                .raw
                .unlock_exclusive(ManuallyDrop::take(&mut self.guard));
        }
    }
}

impl<'a, R: RawRwLock + 'a, T: fmt::Debug + ?Sized + 'a> fmt::Debug for RwLockWriteGuard<'a, R, T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Debug::fmt(&**self, f)
    }
}

impl<'a, R: RawRwLock + 'a, T: fmt::Display + ?Sized + 'a> fmt::Display
    for RwLockWriteGuard<'a, R, T>
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        (**self).fmt(f)
    }
}

#[cfg(feature = "owning_ref")]
unsafe impl<'a, R: RawRwLock + 'a, T: ?Sized + 'a> StableAddress for RwLockWriteGuard<'a, R, T> {}

/// An RAII write guard returned by the `Arc` locking operations on [`RwLock`].
/// Unlike [`RwLockWriteGuard`], this guard owns the [`Arc`] used to unlock the
/// reader-writer lock instead of borrowing it, so its lifetime is not tied to a
/// lock reference.
#[cfg(feature = "arc_lock")]
#[clippy::has_significant_drop]
#[must_use = "if unused the RwLock will immediately unlock"]
pub struct ArcRwLockWriteGuard<R: RawRwLock, T: ?Sized> {
    // The raw unlock operation consumes this state in Drop.
    guard: ManuallyDrop<R::ExclusiveGuard>,
    rwlock: Arc<RwLock<R, T>>,
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLock, T: ?Sized> ArcRwLockWriteGuard<R, T> {
    /// Returns a reference to the rwlock, contained in its `Arc`.
    pub fn rwlock(s: &Self) -> &Arc<RwLock<R, T>> {
        &s.rwlock
    }

    /// Unlocks the `RwLock` and returns the `Arc` that was held by the [`ArcRwLockWriteGuard`].
    #[inline]
    pub fn into_arc(s: Self) -> Arc<RwLock<R, T>> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: ManuallyDrop lets us move the Arc into a local that drops on unwind.
        let rwlock = unsafe { ptr::read(&s.rwlock) };
        // SAFETY: Valid exclusive state; ManuallyDrop prevents a second release.
        let guard = unsafe { ManuallyDrop::take(&mut s.guard) };
        // SAFETY: The state belongs to the acquisition held by this guard.
        unsafe { rwlock.raw.unlock_exclusive(guard) };
        rwlock
    }

    /// Temporarily unlocks the `RwLock` to execute the given function.
    ///
    /// This is functionally equivalent to the `unlocked` method on [`RwLockWriteGuard`].
    ///
    /// # Aborts
    ///
    /// Aborts if re-locking the `RwLock` panics.
    #[inline]
    #[track_caller]
    pub fn unlocked<F, U>(s: &mut Self, f: F) -> U
    where
        F: FnOnce() -> U,
    {
        // SAFETY: Valid exclusive state; cleanup restores it before reuse or drop.
        let state = unsafe { ManuallyDrop::take(&mut s.guard) };
        // A panicking raw unlock releases the acquisition too.
        defer!(abort_on_panic(
            || s.guard = ManuallyDrop::new(s.rwlock.raw.lock_exclusive())
        ));
        // SAFETY: The state belongs to the acquisition held by this guard.
        unsafe { s.rwlock.raw.unlock_exclusive(state) };
        f()
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLockDowngrade, T: ?Sized> ArcRwLockWriteGuard<R, T> {
    /// Atomically downgrades a write lock into a read lock without allowing any
    /// writers to take exclusive access of the lock in the meantime.
    ///
    /// This is functionally equivalent to the `downgrade` method on [`RwLockWriteGuard`].
    #[track_caller]
    pub fn downgrade(s: Self) -> ArcRwLockReadGuard<R, T> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: ManuallyDrop lets us move the Arc into a local that drops on unwind.
        let rwlock = unsafe { ptr::read(&s.rwlock) };
        // SAFETY: Valid exclusive state; ManuallyDrop prevents a second release.
        let guard = unsafe { ManuallyDrop::take(&mut s.guard) };
        // SAFETY: The state belongs to the acquisition held by this guard.
        let guard = unsafe { rwlock.raw.downgrade(guard) };
        ArcRwLockReadGuard {
            rwlock,
            guard: ManuallyDrop::new(guard),
        }
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLockUpgradeDowngrade, T: ?Sized> ArcRwLockWriteGuard<R, T> {
    /// Atomically downgrades a write lock into an upgradable read lock without allowing any
    /// writers to take exclusive access of the lock in the meantime.
    ///
    /// This is functionally identical to the `downgrade_to_upgradable` method on [`RwLockWriteGuard`].
    #[track_caller]
    pub fn downgrade_to_upgradable(s: Self) -> ArcRwLockUpgradableReadGuard<R, T> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: ManuallyDrop lets us move the Arc into a local that drops on unwind.
        let rwlock = unsafe { ptr::read(&s.rwlock) };
        // SAFETY: Valid exclusive state; ManuallyDrop prevents a second release.
        let guard = unsafe { ManuallyDrop::take(&mut s.guard) };
        // SAFETY: The state belongs to the acquisition held by this guard.
        let guard = unsafe { rwlock.raw.downgrade_to_upgradable(guard) };
        ArcRwLockUpgradableReadGuard {
            rwlock,
            guard: ManuallyDrop::new(guard),
        }
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLockFair, T: ?Sized> ArcRwLockWriteGuard<R, T> {
    /// Unlocks the `RwLock` using a fair unlock protocol.
    ///
    /// This is functionally equivalent to the `unlock_fair` method on [`RwLockWriteGuard`].
    #[inline]
    #[track_caller]
    pub fn unlock_fair(s: Self) {
        drop(Self::into_arc_fair(s));
    }

    /// Unlocks the `RwLock` using a fair unlock protocol and returns the `Arc` that was held by the [`ArcRwLockWriteGuard`].
    #[inline]
    pub fn into_arc_fair(s: Self) -> Arc<RwLock<R, T>> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: ManuallyDrop lets us move the Arc into a local that drops on unwind.
        let rwlock = unsafe { ptr::read(&s.rwlock) };
        // SAFETY: Valid exclusive state; ManuallyDrop prevents a second release.
        let guard = unsafe { ManuallyDrop::take(&mut s.guard) };
        // SAFETY: The state belongs to the acquisition held by this guard.
        unsafe { rwlock.raw.unlock_exclusive_fair(guard) };
        rwlock
    }

    /// Temporarily unlocks the `RwLock` to execute the given function.
    ///
    /// This is functionally equivalent to the `unlocked_fair` method on [`RwLockWriteGuard`].
    ///
    /// # Aborts
    ///
    /// Aborts if re-locking the `RwLock` panics.
    #[inline]
    #[track_caller]
    pub fn unlocked_fair<F, U>(s: &mut Self, f: F) -> U
    where
        F: FnOnce() -> U,
    {
        // SAFETY: Valid exclusive state; cleanup restores it before reuse or drop.
        let state = unsafe { ManuallyDrop::take(&mut s.guard) };
        // A panicking raw unlock releases the acquisition too.
        defer!(abort_on_panic(
            || s.guard = ManuallyDrop::new(s.rwlock.raw.lock_exclusive())
        ));
        // SAFETY: The state belongs to the acquisition held by this guard.
        unsafe { s.rwlock.raw.unlock_exclusive_fair(state) };
        f()
    }

    /// Temporarily yields the `RwLock` to a waiting thread if there is one.
    ///
    /// This method is functionally equivalent to the `bump` method on [`RwLockWriteGuard`].
    #[inline]
    #[track_caller]
    pub fn bump(s: &mut Self) {
        // SAFETY: Valid exclusive state; bump preserves ownership even on unwind.
        unsafe { s.rwlock.raw.bump_exclusive(&mut s.guard) };
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLock, T: ?Sized> Deref for ArcRwLockWriteGuard<R, T> {
    type Target = T;
    #[inline]
    fn deref(&self) -> &T {
        unsafe { self.rwlock.data.get().as_ref_unchecked() }
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLock, T: ?Sized> DerefMut for ArcRwLockWriteGuard<R, T> {
    #[inline]
    fn deref_mut(&mut self) -> &mut T {
        unsafe { self.rwlock.data.get().as_mut_unchecked() }
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLock, T: ?Sized> Drop for ArcRwLockWriteGuard<R, T> {
    #[inline]
    fn drop(&mut self) {
        // Safety: An RwLockWriteGuard always holds an exclusive lock.
        unsafe {
            self.rwlock
                .raw
                .unlock_exclusive(ManuallyDrop::take(&mut self.guard));
        }
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLock, T: fmt::Debug + ?Sized> fmt::Debug for ArcRwLockWriteGuard<R, T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Debug::fmt(&**self, f)
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLock, T: fmt::Display + ?Sized> fmt::Display for ArcRwLockWriteGuard<R, T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        (**self).fmt(f)
    }
}

/// An RAII guard which releases upgradable read access when dropped.
///
/// This structure is created by the
/// [`upgradable_read`](RwLock::upgradable_read) and
/// [`try_upgradable_read`](RwLock::try_upgradable_read) methods on [`RwLock`].
#[clippy::has_significant_drop]
#[must_use = "if unused the RwLock will immediately unlock"]
pub struct RwLockUpgradableReadGuard<'a, R: RawRwLockUpgrade, T: ?Sized> {
    rwlock: &'a RwLock<R, T>,
    marker: PhantomData<&'a T>,
    // The raw unlock operation consumes this state in Drop.
    guard: ManuallyDrop<R::UpgradableGuard>,
}

impl<'a, R: RawRwLockUpgrade + 'a, T: ?Sized + 'a> RwLockUpgradableReadGuard<'a, R, T> {
    /// Returns a reference to the original reader-writer lock object.
    pub fn rwlock(s: &Self) -> &'a RwLock<R, T> {
        s.rwlock
    }

    /// Temporarily unlocks the `RwLock` to execute the given function.
    ///
    /// The mutable reference ensures that no references derived from this guard
    /// are live while the lock is temporarily released.
    ///
    /// # Aborts
    ///
    /// Aborts if re-locking the `RwLock` panics.
    #[inline]
    #[track_caller]
    pub fn unlocked<F, U>(s: &mut Self, f: F) -> U
    where
        F: FnOnce() -> U,
    {
        // SAFETY: Valid upgradable state; cleanup restores it before reuse or drop.
        let state = unsafe { ManuallyDrop::take(&mut s.guard) };
        // A panicking raw unlock releases the acquisition too.
        defer!(abort_on_panic(
            || s.guard = ManuallyDrop::new(s.rwlock.raw.lock_upgradable())
        ));
        // SAFETY: The state belongs to the acquisition held by this guard.
        unsafe { s.rwlock.raw.unlock_upgradable(state) };
        f()
    }

    /// Atomically upgrades an upgradable read lock into an exclusive write lock,
    /// blocking the current thread until it can be acquired.
    ///
    /// This cannot complete while any other read guards remain. Calling it
    /// while the current thread holds another read guard may therefore
    /// deadlock.
    #[track_caller]
    pub fn upgrade(s: Self) -> RwLockWriteGuard<'a, R, T> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: Valid upgradable state; ManuallyDrop prevents a second release.
        let guard = unsafe { s.rwlock.raw.upgrade(ManuallyDrop::take(&mut s.guard)) };

        RwLockWriteGuard {
            rwlock: s.rwlock,
            guard: ManuallyDrop::new(guard),
            marker: PhantomData,
        }
    }

    /// Tries to atomically upgrade an upgradable read lock into an exclusive write lock.
    ///
    /// If the access could not be granted at this time, then the current guard is returned.
    #[track_caller]
    pub fn try_upgrade(s: Self) -> Result<RwLockWriteGuard<'a, R, T>, Self> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: Valid upgradable state; ManuallyDrop prevents a second release.
        let result = unsafe { s.rwlock.raw.try_upgrade(ManuallyDrop::take(&mut s.guard)) };
        match result {
            Ok(guard) => Ok(RwLockWriteGuard {
                rwlock: s.rwlock,
                guard: ManuallyDrop::new(guard),
                marker: PhantomData,
            }),
            Err(guard) => {
                s.guard = ManuallyDrop::new(guard);
                Err(ManuallyDrop::into_inner(s))
            }
        }
    }
}

impl<'a, R: RawRwLockUpgradeFair + 'a, T: ?Sized + 'a> RwLockUpgradableReadGuard<'a, R, T> {
    /// Unlocks the `RwLock` using a fair unlock protocol.
    ///
    /// A fair unlock gives waiting threads priority over new acquisition attempts.
    /// This can prevent starvation, but may reduce throughput by forcing a context
    /// switch.
    #[inline]
    #[track_caller]
    pub fn unlock_fair(s: Self) {
        let mut s = ManuallyDrop::new(s);
        // Safety: An RwLockUpgradableReadGuard always holds an upgradable lock.
        unsafe {
            s.rwlock
                .raw
                .unlock_upgradable_fair(ManuallyDrop::take(&mut s.guard));
        }
    }

    /// Temporarily unlocks the `RwLock` to execute the given function.
    ///
    /// The `RwLock` is unlocked using a fair unlock protocol.
    ///
    /// The mutable reference ensures that no references derived from this guard
    /// are live while the lock is temporarily released.
    ///
    /// # Aborts
    ///
    /// Aborts if re-locking the `RwLock` panics.
    #[inline]
    #[track_caller]
    pub fn unlocked_fair<F, U>(s: &mut Self, f: F) -> U
    where
        F: FnOnce() -> U,
    {
        // SAFETY: Valid upgradable state; cleanup restores it before reuse or drop.
        let state = unsafe { ManuallyDrop::take(&mut s.guard) };
        // A panicking raw unlock releases the acquisition too.
        defer!(abort_on_panic(
            || s.guard = ManuallyDrop::new(s.rwlock.raw.lock_upgradable())
        ));
        // SAFETY: The state belongs to the acquisition held by this guard.
        unsafe { s.rwlock.raw.unlock_upgradable_fair(state) };
        f()
    }

    /// Temporarily yields the `RwLock` to a waiting thread if there is one.
    ///
    /// This method is functionally equivalent to calling `unlock_fair` followed
    /// by `upgradable_read`, however it can be much more efficient in the case where there
    /// are no waiting threads.
    #[inline]
    #[track_caller]
    pub fn bump(s: &mut Self) {
        // SAFETY: Valid upgradable state; bump preserves ownership even on unwind.
        unsafe { s.rwlock.raw.bump_upgradable(&mut s.guard) };
    }
}

impl<'a, R: RawRwLockUpgradeDowngrade + 'a, T: ?Sized + 'a> RwLockUpgradableReadGuard<'a, R, T> {
    /// Atomically downgrades an upgradable read lock into a shared read lock
    /// without allowing any writers to take exclusive access of the lock in the
    /// meantime.
    ///
    /// Note that if there are any writers currently waiting to take the lock
    /// then other readers may not be able to acquire the lock even if it was
    /// downgraded.
    #[track_caller]
    pub fn downgrade(s: Self) -> RwLockReadGuard<'a, R, T> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: Valid upgradable state; ManuallyDrop prevents a second release.
        let guard = unsafe {
            s.rwlock
                .raw
                .downgrade_upgradable(ManuallyDrop::take(&mut s.guard))
        };

        RwLockReadGuard {
            rwlock: s.rwlock,
            guard: ManuallyDrop::new(guard),
            marker: PhantomData,
        }
    }

    /// First, atomically upgrades an upgradable read lock into an exclusive write lock,
    /// blocking the current thread until it can be acquired.
    /// This cannot complete while any other read guards remain.
    ///
    /// Then, calls the provided closure with an exclusive reference to the lock's data.
    ///
    /// Finally, atomically downgrades the lock back to an upgradable read lock.
    /// The closure's return value is returned.
    ///
    /// This function only requires a mutable reference to the guard, unlike
    /// `upgrade` which takes the guard by value.
    ///
    /// # Aborts
    ///
    /// Aborts if upgrading or downgrading back to an upgradable read lock panics,
    /// since releasing and re-acquiring would break continuous lock ownership.
    #[track_caller]
    pub fn with_upgraded<Ret, F: FnOnce(&mut T) -> Ret>(&mut self, f: F) -> Ret {
        // SAFETY: Valid upgradable state; aborting on panic prevents reuse without ownership.
        let guard = abort_on_panic(|| unsafe {
            self.rwlock.raw.upgrade(ManuallyDrop::take(&mut self.guard))
        });
        // SAFETY: Valid exclusive state; cleanup restores upgradable ownership on return or unwind.
        defer! {
            self.guard = ManuallyDrop::new(abort_on_panic(|| unsafe {
                self.rwlock.raw.downgrade_to_upgradable(guard)
            }));
        };
        // SAFETY: The exclusive acquisition protects the data until cleanup.
        f(unsafe { self.rwlock.data.get().as_mut_unchecked() })
    }

    /// First, tries to atomically upgrade an upgradable read lock into an exclusive write lock.
    ///
    /// If the access could not be granted at this time, then `None` is returned.
    ///
    /// Otherwise, calls the provided closure with an exclusive reference to the lock's data,
    /// and finally downgrades the lock back to an upgradable read lock.
    /// The closure's return value is wrapped in `Some` and returned.
    ///
    /// This function only requires a mutable reference to the guard, unlike
    /// `try_upgrade` which takes the guard by value.
    ///
    /// # Aborts
    ///
    /// Aborts if upgrading or downgrading back to an upgradable read lock panics,
    /// since releasing and re-acquiring would break continuous lock ownership.
    #[track_caller]
    pub fn try_with_upgraded<Ret, F: FnOnce(&mut T) -> Ret>(&mut self, f: F) -> Option<Ret> {
        // SAFETY: Valid upgradable state; aborting on panic prevents reuse without ownership.
        match abort_on_panic(|| unsafe {
            self.rwlock
                .raw
                .try_upgrade(ManuallyDrop::take(&mut self.guard))
        }) {
            Ok(guard) => {
                // SAFETY: Valid exclusive state; cleanup restores upgradable ownership on return or unwind.
                defer! {
                    self.guard = ManuallyDrop::new(abort_on_panic(|| unsafe {
                        self.rwlock.raw.downgrade_to_upgradable(guard)
                    }));
                };
                // SAFETY: The exclusive acquisition protects the data until cleanup.
                Some(f(unsafe { self.rwlock.data.get().as_mut_unchecked() }))
            }
            Err(guard) => {
                self.guard = ManuallyDrop::new(guard);
                None
            }
        }
    }
}

impl<'a, R: RawRwLockUpgradeTimed + 'a, T: ?Sized + 'a> RwLockUpgradableReadGuard<'a, R, T> {
    /// Tries to atomically upgrade an upgradable read lock into an exclusive
    /// write lock, until a timeout is reached.
    ///
    /// If the access could not be granted before the timeout expires, then
    /// the current guard is returned.
    ///
    /// See [`RawRwLockUpgradeTimed::try_upgrade_for`] for timeout behavior.
    #[track_caller]
    pub fn try_upgrade_for(
        s: Self,
        timeout: R::Duration,
    ) -> Result<RwLockWriteGuard<'a, R, T>, Self> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: Valid upgradable state; ManuallyDrop prevents a second release.
        let result = unsafe {
            s.rwlock
                .raw
                .try_upgrade_for(ManuallyDrop::take(&mut s.guard), timeout)
        };
        match result {
            Ok(guard) => Ok(RwLockWriteGuard {
                rwlock: s.rwlock,
                guard: ManuallyDrop::new(guard),
                marker: PhantomData,
            }),
            Err(guard) => {
                s.guard = ManuallyDrop::new(guard);
                Err(ManuallyDrop::into_inner(s))
            }
        }
    }

    /// Tries to atomically upgrade an upgradable read lock into an exclusive
    /// write lock, until a timeout is reached.
    ///
    /// If the access could not be granted before the timeout expires, then
    /// the current guard is returned.
    ///
    /// See [`RawRwLockUpgradeTimed::try_upgrade_until`] for timeout behavior.
    #[inline]
    #[track_caller]
    pub fn try_upgrade_until(
        s: Self,
        timeout: R::Instant,
    ) -> Result<RwLockWriteGuard<'a, R, T>, Self> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: Valid upgradable state; ManuallyDrop prevents a second release.
        let result = unsafe {
            s.rwlock
                .raw
                .try_upgrade_until(ManuallyDrop::take(&mut s.guard), timeout)
        };
        match result {
            Ok(guard) => Ok(RwLockWriteGuard {
                rwlock: s.rwlock,
                guard: ManuallyDrop::new(guard),
                marker: PhantomData,
            }),
            Err(guard) => {
                s.guard = ManuallyDrop::new(guard);
                Err(ManuallyDrop::into_inner(s))
            }
        }
    }
}

impl<'a, R: RawRwLockUpgradeTimed + RawRwLockUpgradeDowngrade + 'a, T: ?Sized + 'a>
    RwLockUpgradableReadGuard<'a, R, T>
{
    /// Tries to atomically upgrade an upgradable read lock into an exclusive
    /// write lock, until a timeout is reached.
    ///
    /// If the access could not be granted before the timeout expires, then
    /// `None` is returned.
    ///
    /// See [`RawRwLockUpgradeTimed::try_upgrade_for`] for timeout behavior.
    ///
    /// Otherwise, calls the provided closure with an exclusive reference to the lock's data,
    /// and finally downgrades the lock back to an upgradable read lock.
    /// The closure's return value is wrapped in `Some` and returned.
    ///
    /// This function only requires a mutable reference to the guard, unlike
    /// `try_upgrade_for` which takes the guard by value.
    ///
    /// # Aborts
    ///
    /// Aborts if upgrading or downgrading back to an upgradable read lock panics,
    /// since releasing and re-acquiring would break continuous lock ownership.
    #[track_caller]
    pub fn try_with_upgraded_for<Ret, F: FnOnce(&mut T) -> Ret>(
        &mut self,
        timeout: R::Duration,
        f: F,
    ) -> Option<Ret> {
        // SAFETY: Valid upgradable state; aborting on panic prevents reuse without ownership.
        match abort_on_panic(|| unsafe {
            self.rwlock
                .raw
                .try_upgrade_for(ManuallyDrop::take(&mut self.guard), timeout)
        }) {
            Ok(guard) => {
                // SAFETY: Valid exclusive state; cleanup restores upgradable ownership on return or unwind.
                defer! {
                    self.guard = ManuallyDrop::new(abort_on_panic(|| unsafe {
                        self.rwlock.raw.downgrade_to_upgradable(guard)
                    }));
                };
                // SAFETY: The exclusive acquisition protects the data until cleanup.
                Some(f(unsafe { self.rwlock.data.get().as_mut_unchecked() }))
            }
            Err(guard) => {
                self.guard = ManuallyDrop::new(guard);
                None
            }
        }
    }

    /// Tries to atomically upgrade an upgradable read lock into an exclusive
    /// write lock, until a timeout is reached.
    ///
    /// If the access could not be granted before the timeout expires, then
    /// `None` is returned.
    ///
    /// See [`RawRwLockUpgradeTimed::try_upgrade_until`] for timeout behavior.
    ///
    /// Otherwise, calls the provided closure with an exclusive reference to the lock's data,
    /// and finally downgrades the lock back to an upgradable read lock.
    /// The closure's return value is wrapped in `Some` and returned.
    ///
    /// This function only requires a mutable reference to the guard, unlike
    /// `try_upgrade_until` which takes the guard by value.
    ///
    /// # Aborts
    ///
    /// Aborts if upgrading or downgrading back to an upgradable read lock panics,
    /// since releasing and re-acquiring would break continuous lock ownership.
    #[track_caller]
    pub fn try_with_upgraded_until<Ret, F: FnOnce(&mut T) -> Ret>(
        &mut self,
        timeout: R::Instant,
        f: F,
    ) -> Option<Ret> {
        // SAFETY: Valid upgradable state; aborting on panic prevents reuse without ownership.
        match abort_on_panic(|| unsafe {
            self.rwlock
                .raw
                .try_upgrade_until(ManuallyDrop::take(&mut self.guard), timeout)
        }) {
            Ok(guard) => {
                // SAFETY: Valid exclusive state; cleanup restores upgradable ownership on return or unwind.
                defer! {
                    self.guard = ManuallyDrop::new(abort_on_panic(|| unsafe {
                        self.rwlock.raw.downgrade_to_upgradable(guard)
                    }));
                };
                // SAFETY: The exclusive acquisition protects the data until cleanup.
                Some(f(unsafe { self.rwlock.data.get().as_mut_unchecked() }))
            }
            Err(guard) => {
                self.guard = ManuallyDrop::new(guard);
                None
            }
        }
    }
}

impl<'a, R: RawRwLockUpgrade + 'a, T: ?Sized + 'a> Deref for RwLockUpgradableReadGuard<'a, R, T> {
    type Target = T;
    #[inline]
    fn deref(&self) -> &T {
        unsafe { self.rwlock.data.get().as_ref_unchecked() }
    }
}

impl<'a, R: RawRwLockUpgrade + 'a, T: ?Sized + 'a> Drop for RwLockUpgradableReadGuard<'a, R, T> {
    #[inline]
    fn drop(&mut self) {
        // Safety: An RwLockUpgradableReadGuard always holds an upgradable lock.
        unsafe {
            self.rwlock
                .raw
                .unlock_upgradable(ManuallyDrop::take(&mut self.guard));
        }
    }
}

impl<'a, R: RawRwLockUpgrade + 'a, T: fmt::Debug + ?Sized + 'a> fmt::Debug
    for RwLockUpgradableReadGuard<'a, R, T>
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Debug::fmt(&**self, f)
    }
}

impl<'a, R: RawRwLockUpgrade + 'a, T: fmt::Display + ?Sized + 'a> fmt::Display
    for RwLockUpgradableReadGuard<'a, R, T>
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        (**self).fmt(f)
    }
}

#[cfg(feature = "owning_ref")]
unsafe impl<'a, R: RawRwLockUpgrade + 'a, T: ?Sized + 'a> StableAddress
    for RwLockUpgradableReadGuard<'a, R, T>
{
}

/// An RAII upgradable read guard returned by the `Arc` locking operations on
/// [`RwLock`].
/// Unlike [`RwLockUpgradableReadGuard`], this guard owns the [`Arc`] used to
/// unlock the reader-writer lock instead of borrowing it, so its lifetime is
/// not tied to a lock reference.
#[cfg(feature = "arc_lock")]
#[clippy::has_significant_drop]
#[must_use = "if unused the RwLock will immediately unlock"]
pub struct ArcRwLockUpgradableReadGuard<R: RawRwLockUpgrade, T: ?Sized> {
    // The raw unlock operation consumes this state in Drop.
    guard: ManuallyDrop<R::UpgradableGuard>,
    rwlock: Arc<RwLock<R, T>>,
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLockUpgrade, T: ?Sized> ArcRwLockUpgradableReadGuard<R, T> {
    /// Returns a reference to the rwlock, contained in its original `Arc`.
    pub fn rwlock(s: &Self) -> &Arc<RwLock<R, T>> {
        &s.rwlock
    }

    /// Unlocks the `RwLock` and returns the `Arc` that was held by the [`ArcRwLockUpgradableReadGuard`].
    #[inline]
    pub fn into_arc(s: Self) -> Arc<RwLock<R, T>> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: ManuallyDrop lets us move the Arc into a local that drops on unwind.
        let rwlock = unsafe { ptr::read(&s.rwlock) };
        // SAFETY: Valid upgradable state; ManuallyDrop prevents a second release.
        let guard = unsafe { ManuallyDrop::take(&mut s.guard) };
        // SAFETY: The state belongs to the acquisition held by this guard.
        unsafe { rwlock.raw.unlock_upgradable(guard) };
        rwlock
    }

    /// Temporarily unlocks the `RwLock` to execute the given function.
    ///
    /// This is functionally identical to the `unlocked` method on [`RwLockUpgradableReadGuard`].
    ///
    /// # Aborts
    ///
    /// Aborts if re-locking the `RwLock` panics.
    #[inline]
    #[track_caller]
    pub fn unlocked<F, U>(s: &mut Self, f: F) -> U
    where
        F: FnOnce() -> U,
    {
        // SAFETY: Valid upgradable state; cleanup restores it before reuse or drop.
        let state = unsafe { ManuallyDrop::take(&mut s.guard) };
        // A panicking raw unlock releases the acquisition too.
        defer!(abort_on_panic(
            || s.guard = ManuallyDrop::new(s.rwlock.raw.lock_upgradable())
        ));
        // SAFETY: The state belongs to the acquisition held by this guard.
        unsafe { s.rwlock.raw.unlock_upgradable(state) };
        f()
    }

    /// Atomically upgrades an upgradable read lock into an exclusive write lock,
    /// blocking the current thread until it can be acquired.
    ///
    /// This cannot complete while any other read guards remain. Calling it
    /// while the current thread holds another read guard may therefore
    /// deadlock.
    #[track_caller]
    pub fn upgrade(s: Self) -> ArcRwLockWriteGuard<R, T> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: ManuallyDrop lets us move the Arc into a local that drops on unwind.
        let rwlock = unsafe { ptr::read(&s.rwlock) };
        // SAFETY: Valid upgradable state; ManuallyDrop prevents a second release.
        let guard = unsafe { ManuallyDrop::take(&mut s.guard) };
        // SAFETY: The state belongs to the acquisition held by this guard.
        let guard = unsafe { rwlock.raw.upgrade(guard) };
        ArcRwLockWriteGuard {
            rwlock,
            guard: ManuallyDrop::new(guard),
        }
    }

    /// Tries to atomically upgrade an upgradable read lock into an exclusive write lock.
    ///
    /// If the access could not be granted at this time, then the current guard is returned.
    #[track_caller]
    pub fn try_upgrade(s: Self) -> Result<ArcRwLockWriteGuard<R, T>, Self> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: ManuallyDrop lets us move the Arc into a local that drops on unwind.
        let rwlock = unsafe { ptr::read(&s.rwlock) };
        // SAFETY: Valid upgradable state; ManuallyDrop prevents a second release.
        let guard = unsafe { ManuallyDrop::take(&mut s.guard) };
        // SAFETY: The state belongs to the acquisition held by this guard.
        match unsafe { rwlock.raw.try_upgrade(guard) } {
            Ok(guard) => Ok(ArcRwLockWriteGuard {
                rwlock,
                guard: ManuallyDrop::new(guard),
            }),
            Err(guard) => Err(Self {
                rwlock,
                guard: ManuallyDrop::new(guard),
            }),
        }
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLockUpgradeFair, T: ?Sized> ArcRwLockUpgradableReadGuard<R, T> {
    /// Unlocks the `RwLock` using a fair unlock protocol.
    ///
    /// This is functionally identical to the `unlock_fair` method on [`RwLockUpgradableReadGuard`].
    #[inline]
    #[track_caller]
    pub fn unlock_fair(s: Self) {
        drop(Self::into_arc_fair(s));
    }

    /// Unlocks the `RwLock` using a fair unlock protocol and returns the `Arc` that was held by the [`ArcRwLockUpgradableReadGuard`].
    #[inline]
    pub fn into_arc_fair(s: Self) -> Arc<RwLock<R, T>> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: ManuallyDrop lets us move the Arc into a local that drops on unwind.
        let rwlock = unsafe { ptr::read(&s.rwlock) };
        // SAFETY: Valid upgradable state; ManuallyDrop prevents a second release.
        let guard = unsafe { ManuallyDrop::take(&mut s.guard) };
        // SAFETY: The state belongs to the acquisition held by this guard.
        unsafe { rwlock.raw.unlock_upgradable_fair(guard) };
        rwlock
    }

    /// Temporarily unlocks the `RwLock` to execute the given function.
    ///
    /// This is functionally equivalent to the `unlocked_fair` method on [`RwLockUpgradableReadGuard`].
    ///
    /// # Aborts
    ///
    /// Aborts if re-locking the `RwLock` panics.
    #[inline]
    #[track_caller]
    pub fn unlocked_fair<F, U>(s: &mut Self, f: F) -> U
    where
        F: FnOnce() -> U,
    {
        // SAFETY: Valid upgradable state; cleanup restores it before reuse or drop.
        let state = unsafe { ManuallyDrop::take(&mut s.guard) };
        // A panicking raw unlock releases the acquisition too.
        defer!(abort_on_panic(
            || s.guard = ManuallyDrop::new(s.rwlock.raw.lock_upgradable())
        ));
        // SAFETY: The state belongs to the acquisition held by this guard.
        unsafe { s.rwlock.raw.unlock_upgradable_fair(state) };
        f()
    }

    /// Temporarily yields the `RwLock` to a waiting thread if there is one.
    ///
    /// This method is functionally equivalent to calling `bump` on [`RwLockUpgradableReadGuard`].
    #[inline]
    #[track_caller]
    pub fn bump(s: &mut Self) {
        // SAFETY: Valid upgradable state; bump preserves ownership even on unwind.
        unsafe { s.rwlock.raw.bump_upgradable(&mut s.guard) };
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLockUpgradeDowngrade, T: ?Sized> ArcRwLockUpgradableReadGuard<R, T> {
    /// Atomically downgrades an upgradable read lock into a shared read lock
    /// without allowing any writers to take exclusive access of the lock in the
    /// meantime.
    ///
    /// Note that if there are any writers currently waiting to take the lock
    /// then other readers may not be able to acquire the lock even if it was
    /// downgraded.
    #[track_caller]
    pub fn downgrade(s: Self) -> ArcRwLockReadGuard<R, T> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: ManuallyDrop lets us move the Arc into a local that drops on unwind.
        let rwlock = unsafe { ptr::read(&s.rwlock) };
        // SAFETY: Valid upgradable state; ManuallyDrop prevents a second release.
        let guard = unsafe { ManuallyDrop::take(&mut s.guard) };
        // SAFETY: The state belongs to the acquisition held by this guard.
        let guard = unsafe { rwlock.raw.downgrade_upgradable(guard) };
        ArcRwLockReadGuard {
            rwlock,
            guard: ManuallyDrop::new(guard),
        }
    }

    /// First, atomically upgrades an upgradable read lock into an exclusive write lock,
    /// blocking the current thread until it can be acquired.
    /// This cannot complete while any other read guards remain.
    ///
    /// Then, calls the provided closure with an exclusive reference to the lock's data.
    ///
    /// Finally, atomically downgrades the lock back to an upgradable read lock.
    /// The closure's return value is returned.
    ///
    /// This function only requires a mutable reference to the guard, unlike
    /// `upgrade` which takes the guard by value.
    ///
    /// # Aborts
    ///
    /// Aborts if upgrading or downgrading back to an upgradable read lock panics,
    /// since releasing and re-acquiring would break continuous lock ownership.
    #[track_caller]
    pub fn with_upgraded<Ret, F: FnOnce(&mut T) -> Ret>(&mut self, f: F) -> Ret {
        // SAFETY: Valid upgradable state; aborting on panic prevents reuse without ownership.
        let guard = abort_on_panic(|| unsafe {
            self.rwlock.raw.upgrade(ManuallyDrop::take(&mut self.guard))
        });
        // SAFETY: Valid exclusive state; cleanup restores upgradable ownership on return or unwind.
        defer! {
            self.guard = ManuallyDrop::new(abort_on_panic(|| unsafe {
                self.rwlock.raw.downgrade_to_upgradable(guard)
            }));
        };
        // SAFETY: The exclusive acquisition protects the data until cleanup.
        f(unsafe { self.rwlock.data.get().as_mut_unchecked() })
    }

    /// First, tries to atomically upgrade an upgradable read lock into an exclusive write lock.
    ///
    /// If the access could not be granted at this time, then `None` is returned.
    ///
    /// Otherwise, calls the provided closure with an exclusive reference to the lock's data,
    /// and finally downgrades the lock back to an upgradable read lock.
    /// The closure's return value is wrapped in `Some` and returned.
    ///
    /// This function only requires a mutable reference to the guard, unlike
    /// `try_upgrade` which takes the guard by value.
    ///
    /// # Aborts
    ///
    /// Aborts if upgrading or downgrading back to an upgradable read lock panics,
    /// since releasing and re-acquiring would break continuous lock ownership.
    #[track_caller]
    pub fn try_with_upgraded<Ret, F: FnOnce(&mut T) -> Ret>(&mut self, f: F) -> Option<Ret> {
        // SAFETY: Valid upgradable state; aborting on panic prevents reuse without ownership.
        match abort_on_panic(|| unsafe {
            self.rwlock
                .raw
                .try_upgrade(ManuallyDrop::take(&mut self.guard))
        }) {
            Ok(guard) => {
                // SAFETY: Valid exclusive state; cleanup restores upgradable ownership on return or unwind.
                defer! {
                    self.guard = ManuallyDrop::new(abort_on_panic(|| unsafe {
                        self.rwlock.raw.downgrade_to_upgradable(guard)
                    }));
                };
                // SAFETY: The exclusive acquisition protects the data until cleanup.
                Some(f(unsafe { self.rwlock.data.get().as_mut_unchecked() }))
            }
            Err(guard) => {
                self.guard = ManuallyDrop::new(guard);
                None
            }
        }
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLockUpgradeTimed, T: ?Sized> ArcRwLockUpgradableReadGuard<R, T> {
    /// Tries to atomically upgrade an upgradable read lock into an exclusive
    /// write lock, until a timeout is reached.
    ///
    /// If the access could not be granted before the timeout expires, then
    /// the current guard is returned.
    ///
    /// See [`RawRwLockUpgradeTimed::try_upgrade_for`] for timeout behavior.
    #[track_caller]
    pub fn try_upgrade_for(
        s: Self,
        timeout: R::Duration,
    ) -> Result<ArcRwLockWriteGuard<R, T>, Self> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: ManuallyDrop lets us move the Arc into a local that drops on unwind.
        let rwlock = unsafe { ptr::read(&s.rwlock) };
        // SAFETY: Valid upgradable state; ManuallyDrop prevents a second release.
        let guard = unsafe { ManuallyDrop::take(&mut s.guard) };
        // SAFETY: The state belongs to the acquisition held by this guard.
        match unsafe { rwlock.raw.try_upgrade_for(guard, timeout) } {
            Ok(guard) => Ok(ArcRwLockWriteGuard {
                rwlock,
                guard: ManuallyDrop::new(guard),
            }),
            Err(guard) => Err(Self {
                rwlock,
                guard: ManuallyDrop::new(guard),
            }),
        }
    }

    /// Tries to atomically upgrade an upgradable read lock into an exclusive
    /// write lock, until a timeout is reached.
    ///
    /// If the access could not be granted before the timeout expires, then
    /// the current guard is returned.
    ///
    /// See [`RawRwLockUpgradeTimed::try_upgrade_until`] for timeout behavior.
    #[inline]
    #[track_caller]
    pub fn try_upgrade_until(
        s: Self,
        timeout: R::Instant,
    ) -> Result<ArcRwLockWriteGuard<R, T>, Self> {
        let mut s = ManuallyDrop::new(s);
        // SAFETY: ManuallyDrop lets us move the Arc into a local that drops on unwind.
        let rwlock = unsafe { ptr::read(&s.rwlock) };
        // SAFETY: Valid upgradable state; ManuallyDrop prevents a second release.
        let guard = unsafe { ManuallyDrop::take(&mut s.guard) };
        // SAFETY: The state belongs to the acquisition held by this guard.
        match unsafe { rwlock.raw.try_upgrade_until(guard, timeout) } {
            Ok(guard) => Ok(ArcRwLockWriteGuard {
                rwlock,
                guard: ManuallyDrop::new(guard),
            }),
            Err(guard) => Err(Self {
                rwlock,
                guard: ManuallyDrop::new(guard),
            }),
        }
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLockUpgradeTimed + RawRwLockUpgradeDowngrade, T: ?Sized>
    ArcRwLockUpgradableReadGuard<R, T>
{
    /// Tries to atomically upgrade an upgradable read lock into an exclusive
    /// write lock, until a timeout is reached.
    ///
    /// If the access could not be granted before the timeout expires, then
    /// `None` is returned.
    ///
    /// See [`RawRwLockUpgradeTimed::try_upgrade_for`] for timeout behavior.
    ///
    /// Otherwise, calls the provided closure with an exclusive reference to the lock's data,
    /// and finally downgrades the lock back to an upgradable read lock.
    /// The closure's return value is wrapped in `Some` and returned.
    ///
    /// This function only requires a mutable reference to the guard, unlike
    /// `try_upgrade_for` which takes the guard by value.
    ///
    /// # Aborts
    ///
    /// Aborts if upgrading or downgrading back to an upgradable read lock panics,
    /// since releasing and re-acquiring would break continuous lock ownership.
    #[track_caller]
    pub fn try_with_upgraded_for<Ret, F: FnOnce(&mut T) -> Ret>(
        &mut self,
        timeout: R::Duration,
        f: F,
    ) -> Option<Ret> {
        // SAFETY: Valid upgradable state; aborting on panic prevents reuse without ownership.
        match abort_on_panic(|| unsafe {
            self.rwlock
                .raw
                .try_upgrade_for(ManuallyDrop::take(&mut self.guard), timeout)
        }) {
            Ok(guard) => {
                // SAFETY: Valid exclusive state; cleanup restores upgradable ownership on return or unwind.
                defer! {
                    self.guard = ManuallyDrop::new(abort_on_panic(|| unsafe {
                        self.rwlock.raw.downgrade_to_upgradable(guard)
                    }));
                };
                // SAFETY: The exclusive acquisition protects the data until cleanup.
                Some(f(unsafe { self.rwlock.data.get().as_mut_unchecked() }))
            }
            Err(guard) => {
                self.guard = ManuallyDrop::new(guard);
                None
            }
        }
    }

    /// Tries to atomically upgrade an upgradable read lock into an exclusive
    /// write lock, until a timeout is reached.
    ///
    /// If the access could not be granted before the timeout expires, then
    /// `None` is returned.
    ///
    /// See [`RawRwLockUpgradeTimed::try_upgrade_until`] for timeout behavior.
    ///
    /// Otherwise, calls the provided closure with an exclusive reference to the lock's data,
    /// and finally downgrades the lock back to an upgradable read lock.
    /// The closure's return value is wrapped in `Some` and returned.
    ///
    /// This function only requires a mutable reference to the guard, unlike
    /// `try_upgrade_until` which takes the guard by value.
    ///
    /// # Aborts
    ///
    /// Aborts if upgrading or downgrading back to an upgradable read lock panics,
    /// since releasing and re-acquiring would break continuous lock ownership.
    #[track_caller]
    pub fn try_with_upgraded_until<Ret, F: FnOnce(&mut T) -> Ret>(
        &mut self,
        timeout: R::Instant,
        f: F,
    ) -> Option<Ret> {
        // SAFETY: Valid upgradable state; aborting on panic prevents reuse without ownership.
        match abort_on_panic(|| unsafe {
            self.rwlock
                .raw
                .try_upgrade_until(ManuallyDrop::take(&mut self.guard), timeout)
        }) {
            Ok(guard) => {
                // SAFETY: Valid exclusive state; cleanup restores upgradable ownership on return or unwind.
                defer! {
                    self.guard = ManuallyDrop::new(abort_on_panic(|| unsafe {
                        self.rwlock.raw.downgrade_to_upgradable(guard)
                    }));
                };
                // SAFETY: The exclusive acquisition protects the data until cleanup.
                Some(f(unsafe { self.rwlock.data.get().as_mut_unchecked() }))
            }
            Err(guard) => {
                self.guard = ManuallyDrop::new(guard);
                None
            }
        }
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLockUpgrade, T: ?Sized> Deref for ArcRwLockUpgradableReadGuard<R, T> {
    type Target = T;
    #[inline]
    fn deref(&self) -> &T {
        unsafe { self.rwlock.data.get().as_ref_unchecked() }
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLockUpgrade, T: ?Sized> Drop for ArcRwLockUpgradableReadGuard<R, T> {
    #[inline]
    fn drop(&mut self) {
        // Safety: An RwLockUpgradableReadGuard always holds an upgradable lock.
        unsafe {
            self.rwlock
                .raw
                .unlock_upgradable(ManuallyDrop::take(&mut self.guard));
        }
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLockUpgrade, T: fmt::Debug + ?Sized> fmt::Debug
    for ArcRwLockUpgradableReadGuard<R, T>
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Debug::fmt(&**self, f)
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawRwLockUpgrade, T: fmt::Display + ?Sized> fmt::Display
    for ArcRwLockUpgradableReadGuard<R, T>
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        (**self).fmt(f)
    }
}

/// An RAII read lock guard returned by [`RwLockReadGuard::map`], which can point to a
/// subfield of the protected data.
///
/// The main difference between `MappedRwLockReadGuard` and `RwLockReadGuard` is that the
/// former doesn't support temporarily unlocking and re-locking, since that
/// could introduce soundness issues if the locked object is modified by another
/// thread.
///
/// This structure is created by the [`map`](RwLockReadGuard::map) and
/// [`try_map`](RwLockReadGuard::try_map), and
/// [`try_map_or_err`](RwLockReadGuard::try_map_or_err) functions on
/// [`RwLockReadGuard`].
#[clippy::has_significant_drop]
#[must_use = "if unused the RwLock will immediately unlock"]
pub struct MappedRwLockReadGuard<'a, R: RawRwLock, T: ?Sized + 'a> {
    raw: &'a R,
    data: SharedGuardData<T>,
    // The raw unlock operation consumes this state in Drop.
    guard: ManuallyDrop<R::SharedGuard>,
}

impl<'a, R: RawRwLock + 'a, T: ?Sized + 'a> MappedRwLockReadGuard<'a, R, T> {
    /// Makes a new `MappedRwLockReadGuard` for a component of the locked data.
    ///
    /// This is an associated function that needs to be
    /// used as `MappedRwLockReadGuard::map(...)`. A method would interfere with methods of
    /// the same name on the contents of the locked data.
    ///
    /// # Panics
    ///
    /// If `f` panics, the original guard is dropped.
    #[inline]
    pub fn map<U: ?Sized, F>(s: Self, f: F) -> MappedRwLockReadGuard<'a, R, U>
    where
        F: FnOnce(&T) -> &U,
    {
        let raw = s.raw;
        let data = SharedGuardData::new(f(unsafe { s.data.as_ref() }));
        let guard = unsafe { ptr::read(&s.guard) };
        mem::forget(s);
        MappedRwLockReadGuard { raw, data, guard }
    }

    /// Attempts to make a new `MappedRwLockReadGuard` for a component of the
    /// locked data. The original guard is returned if the closure returns `None`.
    ///
    /// This is an associated function that needs to be
    /// used as `MappedRwLockReadGuard::try_map(...)`. A method would interfere with methods of
    /// the same name on the contents of the locked data.
    ///
    /// # Panics
    ///
    /// If `f` panics, the original guard is dropped.
    #[inline]
    pub fn try_map<U: ?Sized, F>(s: Self, f: F) -> Result<MappedRwLockReadGuard<'a, R, U>, Self>
    where
        F: FnOnce(&T) -> Option<&U>,
    {
        let raw = s.raw;
        let Some(data) = f(unsafe { s.data.as_ref() }) else {
            return Err(s);
        };
        let data = SharedGuardData::new(data);
        let guard = unsafe { ptr::read(&s.guard) };
        mem::forget(s);
        Ok(MappedRwLockReadGuard { raw, data, guard })
    }

    /// Attempts to make a new `MappedRwLockReadGuard` for a component of the
    /// locked data. The original guard is returned alongside arbitrary user data
    /// if the closure returns `Err`.
    ///
    /// This is an associated function that needs to be
    /// used as `MappedRwLockReadGuard::try_map_or_err(...)`. A method would interfere with methods of
    /// the same name on the contents of the locked data.
    ///
    /// # Panics
    ///
    /// If `f` panics, the original guard is dropped.
    #[inline]
    pub fn try_map_or_err<U: ?Sized, F, E>(
        s: Self,
        f: F,
    ) -> Result<MappedRwLockReadGuard<'a, R, U>, (Self, E)>
    where
        F: FnOnce(&T) -> Result<&U, E>,
    {
        let raw = s.raw;
        let data = match f(unsafe { s.data.as_ref() }) {
            Ok(data) => data,
            Err(e) => return Err((s, e)),
        };
        let data = SharedGuardData::new(data);
        let guard = unsafe { ptr::read(&s.guard) };
        mem::forget(s);
        Ok(MappedRwLockReadGuard { raw, data, guard })
    }
}

impl<'a, R: RawRwLockFair + 'a, T: ?Sized + 'a> MappedRwLockReadGuard<'a, R, T> {
    /// Unlocks the `RwLock` using a fair unlock protocol.
    ///
    /// A fair unlock gives waiting threads priority over new acquisition attempts.
    /// This can prevent starvation, but may reduce throughput by forcing a context
    /// switch.
    #[inline]
    #[track_caller]
    pub fn unlock_fair(s: Self) {
        let mut s = ManuallyDrop::new(s);
        // Safety: A MappedRwLockReadGuard always holds a shared lock.
        unsafe {
            s.raw.unlock_shared_fair(ManuallyDrop::take(&mut s.guard));
        }
    }
}

impl<'a, R: RawRwLock + 'a, T: ?Sized + 'a> Deref for MappedRwLockReadGuard<'a, R, T> {
    type Target = T;
    #[inline]
    fn deref(&self) -> &T {
        unsafe { self.data.as_ref() }
    }
}

impl<'a, R: RawRwLock + 'a, T: ?Sized + 'a> Drop for MappedRwLockReadGuard<'a, R, T> {
    #[inline]
    fn drop(&mut self) {
        // Safety: A MappedRwLockReadGuard always holds a shared lock.
        unsafe {
            self.raw.unlock_shared(ManuallyDrop::take(&mut self.guard));
        }
    }
}

impl<'a, R: RawRwLock + 'a, T: fmt::Debug + ?Sized + 'a> fmt::Debug
    for MappedRwLockReadGuard<'a, R, T>
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Debug::fmt(&**self, f)
    }
}

impl<'a, R: RawRwLock + 'a, T: fmt::Display + ?Sized + 'a> fmt::Display
    for MappedRwLockReadGuard<'a, R, T>
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        (**self).fmt(f)
    }
}

#[cfg(feature = "owning_ref")]
unsafe impl<'a, R: RawRwLock + 'a, T: ?Sized + 'a> StableAddress
    for MappedRwLockReadGuard<'a, R, T>
{
}

/// An RAII write lock guard returned by [`RwLockWriteGuard::map`], which can point to a
/// subfield of the protected data.
///
/// The main difference between `MappedRwLockWriteGuard` and `RwLockWriteGuard` is that the
/// former doesn't support temporarily unlocking and re-locking, since that
/// could introduce soundness issues if the locked object is modified by another
/// thread.
///
/// This structure is created by the [`map`](RwLockWriteGuard::map) and
/// [`try_map`](RwLockWriteGuard::try_map), and
/// [`try_map_or_err`](RwLockWriteGuard::try_map_or_err) functions on
/// [`RwLockWriteGuard`].
#[clippy::has_significant_drop]
#[must_use = "if unused the RwLock will immediately unlock"]
pub struct MappedRwLockWriteGuard<'a, R: RawRwLock, T: ?Sized + 'a> {
    raw: &'a R,
    data: ExclusiveGuardData<T>,
    // The raw unlock operation consumes this state in Drop.
    guard: ManuallyDrop<R::ExclusiveGuard>,
}

impl<'a, R: RawRwLock + 'a, T: ?Sized + 'a> MappedRwLockWriteGuard<'a, R, T> {
    /// Makes a new `MappedRwLockWriteGuard` for a component of the locked data.
    ///
    /// This is an associated function that needs to be
    /// used as `MappedRwLockWriteGuard::map(...)`. A method would interfere with methods of
    /// the same name on the contents of the locked data.
    ///
    /// # Panics
    ///
    /// If `f` panics, the original guard is dropped.
    #[inline]
    pub fn map<U: ?Sized, F>(mut s: Self, f: F) -> MappedRwLockWriteGuard<'a, R, U>
    where
        F: FnOnce(&mut T) -> &mut U,
    {
        let raw = s.raw;
        let data = ExclusiveGuardData::new(f(unsafe { s.data.as_mut() }));
        let guard = unsafe { ptr::read(&s.guard) };
        mem::forget(s);
        MappedRwLockWriteGuard { raw, data, guard }
    }

    /// Attempts to make a new `MappedRwLockWriteGuard` for a component of the
    /// locked data. The original guard is returned if the closure returns `None`.
    ///
    /// This is an associated function that needs to be
    /// used as `MappedRwLockWriteGuard::try_map(...)`. A method would interfere with methods of
    /// the same name on the contents of the locked data.
    ///
    /// # Panics
    ///
    /// If `f` panics, the original guard is dropped.
    #[inline]
    pub fn try_map<U: ?Sized, F>(
        mut s: Self,
        f: F,
    ) -> Result<MappedRwLockWriteGuard<'a, R, U>, Self>
    where
        F: FnOnce(&mut T) -> Option<&mut U>,
    {
        let raw = s.raw;
        let Some(data) = f(unsafe { s.data.as_mut() }) else {
            return Err(s);
        };
        let data = ExclusiveGuardData::new(data);
        let guard = unsafe { ptr::read(&s.guard) };
        mem::forget(s);
        Ok(MappedRwLockWriteGuard { raw, data, guard })
    }

    /// Attempts to make a new `MappedRwLockWriteGuard` for a component of the
    /// locked data. The original guard is returned alongside arbitrary user data
    /// if the closure returns `Err`.
    ///
    /// This is an associated function that needs to be
    /// used as `MappedRwLockWriteGuard::try_map_or_err(...)`. A method would interfere with methods of
    /// the same name on the contents of the locked data.
    ///
    /// # Panics
    ///
    /// If `f` panics, the original guard is dropped.
    #[inline]
    pub fn try_map_or_err<U: ?Sized, F, E>(
        mut s: Self,
        f: F,
    ) -> Result<MappedRwLockWriteGuard<'a, R, U>, (Self, E)>
    where
        F: FnOnce(&mut T) -> Result<&mut U, E>,
    {
        let raw = s.raw;
        let data = match f(unsafe { s.data.as_mut() }) {
            Ok(data) => data,
            Err(e) => return Err((s, e)),
        };
        let data = ExclusiveGuardData::new(data);
        let guard = unsafe { ptr::read(&s.guard) };
        mem::forget(s);
        Ok(MappedRwLockWriteGuard { raw, data, guard })
    }
}

impl<'a, R: RawRwLockFair + 'a, T: ?Sized + 'a> MappedRwLockWriteGuard<'a, R, T> {
    /// Unlocks the `RwLock` using a fair unlock protocol.
    ///
    /// A fair unlock gives waiting threads priority over new acquisition attempts.
    /// This can prevent starvation, but may reduce throughput by forcing a context
    /// switch.
    #[inline]
    #[track_caller]
    pub fn unlock_fair(s: Self) {
        let mut s = ManuallyDrop::new(s);
        // Safety: A MappedRwLockWriteGuard always holds an exclusive lock.
        unsafe {
            s.raw
                .unlock_exclusive_fair(ManuallyDrop::take(&mut s.guard));
        }
    }
}

impl<'a, R: RawRwLock + 'a, T: ?Sized + 'a> Deref for MappedRwLockWriteGuard<'a, R, T> {
    type Target = T;
    #[inline]
    fn deref(&self) -> &T {
        unsafe { self.data.as_ref() }
    }
}

impl<'a, R: RawRwLock + 'a, T: ?Sized + 'a> DerefMut for MappedRwLockWriteGuard<'a, R, T> {
    #[inline]
    fn deref_mut(&mut self) -> &mut T {
        unsafe { self.data.as_mut() }
    }
}

impl<'a, R: RawRwLock + 'a, T: ?Sized + 'a> Drop for MappedRwLockWriteGuard<'a, R, T> {
    #[inline]
    fn drop(&mut self) {
        // Safety: A MappedRwLockWriteGuard always holds an exclusive lock.
        unsafe {
            self.raw
                .unlock_exclusive(ManuallyDrop::take(&mut self.guard));
        }
    }
}

impl<'a, R: RawRwLock + 'a, T: fmt::Debug + ?Sized + 'a> fmt::Debug
    for MappedRwLockWriteGuard<'a, R, T>
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Debug::fmt(&**self, f)
    }
}

impl<'a, R: RawRwLock + 'a, T: fmt::Display + ?Sized + 'a> fmt::Display
    for MappedRwLockWriteGuard<'a, R, T>
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        (**self).fmt(f)
    }
}

#[cfg(feature = "owning_ref")]
unsafe impl<'a, R: RawRwLock + 'a, T: ?Sized + 'a> StableAddress
    for MappedRwLockWriteGuard<'a, R, T>
{
}
