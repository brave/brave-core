use crate::{
    GuardNoSend,
    guard::{SharedGuardData, abort_on_panic},
    mutex::{RawMutex, RawMutexFair, RawMutexTimed},
};
use core::{
    cell::{Cell, UnsafeCell},
    fmt,
    marker::PhantomData,
    mem::{self, MaybeUninit},
    num::NonZeroUsize,
    ops::Deref,
    sync::atomic::{AtomicUsize, Ordering},
};

#[cfg(feature = "arc_lock")]
use alloc::sync::Arc;
use core::mem::ManuallyDrop;
#[cfg(feature = "arc_lock")]
use core::ptr;

#[cfg(feature = "owning_ref")]
use owning_ref::StableAddress;

#[cfg(feature = "serde")]
use serde::{Deserialize, Deserializer, Serialize, Serializer};

/// Helper trait which returns a non-zero thread ID.
///
/// The simplest way to implement this trait is to return the address of a
/// thread-local variable.
///
/// # Safety
///
/// Implementations of this trait must ensure that no two active threads share
/// the same thread ID. Repeated calls on the same `GetThreadId` value from the
/// same active thread must return the same thread ID. However the ID of a
/// thread that has exited can be reused since that thread is no longer active.
pub unsafe trait GetThreadId {
    /// Initial value.
    // A “non-constant” const item is a legacy way to supply an initialized value to downstream
    // static items. Can hopefully be replaced with `const fn new() -> Self` at some point.
    #[allow(clippy::declare_interior_mutable_const)]
    const INIT: Self;

    /// Returns a non-zero thread ID which identifies the current thread of
    /// execution.
    fn nonzero_thread_id(&self) -> NonZeroUsize;
}

/// A raw mutex type that wraps another raw mutex to provide reentrancy.
///
/// Although this has the same methods as the [`RawMutex`] trait, it does
/// not implement it, and should not be used in the same way, since this
/// mutex can successfully acquire a lock multiple times in the same thread.
/// Only use this when you know you want a raw mutex that can be locked
/// reentrantly; you probably want [`ReentrantMutex`] instead.
pub struct RawReentrantMutex<R: RawMutex, G> {
    owner: AtomicUsize,
    lock_count: Cell<usize>,
    mutex: R,
    guard: UnsafeCell<MaybeUninit<R::Guard>>,
    get_thread_id: G,
}

// The acquisition state is only accessed by the owning thread and is leaked
// if the raw mutex is dropped while locked, just like a forgotten guard.
unsafe impl<R: RawMutex + Send, G: Send> Send for RawReentrantMutex<R, G> {}

unsafe impl<R: RawMutex + Sync, G: GetThreadId + Sync> Sync for RawReentrantMutex<R, G> {}

impl<R: RawMutex, G: GetThreadId> RawReentrantMutex<R, G> {
    /// Initial value for an unlocked mutex.
    #[allow(clippy::declare_interior_mutable_const)]
    pub const INIT: Self = RawReentrantMutex {
        owner: AtomicUsize::new(0),
        lock_count: Cell::new(0),
        guard: UnsafeCell::new(MaybeUninit::uninit()),
        mutex: R::INIT,
        get_thread_id: G::INIT,
    };

    #[inline]
    fn lock_internal<F: FnOnce() -> Option<R::Guard>>(&self, try_lock: F) -> bool {
        let id = self.get_thread_id.nonzero_thread_id().get();
        if self.owner.load(Ordering::Relaxed) == id {
            self.lock_count.set(
                self.lock_count
                    .get()
                    .checked_add(1)
                    .expect("ReentrantMutex lock count overflow"),
            );
        } else {
            let Some(guard) = try_lock() else {
                return false;
            };
            // SAFETY: We have just acquired the underlying mutex.
            unsafe {
                (*self.guard.get()).write(guard);
            }
            self.owner.store(id, Ordering::Relaxed);
            debug_assert_eq!(self.lock_count.get(), 0);
            self.lock_count.set(1);
        }
        true
    }

    /// Acquires this mutex, blocking if it is held by another thread.
    ///
    /// # Panics
    ///
    /// Panics if the recursive lock count overflows.
    #[inline]
    pub fn lock(&self) {
        self.lock_internal(|| Some(self.mutex.lock()));
    }

    /// Attempts to acquire this mutex without blocking. Returns `true`
    /// if the lock was successfully acquired and `false` otherwise.
    ///
    /// # Panics
    ///
    /// Panics if the recursive lock count overflows.
    #[inline]
    pub fn try_lock(&self) -> bool {
        self.lock_internal(|| self.mutex.try_lock())
    }

    /// Unlocks this mutex. The inner mutex may not be unlocked if
    /// this mutex was acquired previously in the current thread.
    ///
    /// # Safety
    ///
    /// This method may only be called if the mutex is held by the current thread.
    #[inline]
    pub unsafe fn unlock(&self) {
        let lock_count = self.lock_count.get() - 1;
        self.lock_count.set(lock_count);
        if lock_count == 0 {
            self.owner.store(0, Ordering::Relaxed);
            // SAFETY: The final owner takes the state before releasing the mutex.
            let guard = unsafe { (*self.guard.get()).assume_init_read() };
            unsafe { self.mutex.unlock(guard) };
        }
    }

    /// Checks whether the mutex is currently locked.
    #[inline]
    pub fn is_locked(&self) -> bool {
        self.mutex.is_locked()
    }

    /// Checks whether the mutex is currently held by the current thread.
    #[inline]
    pub fn is_owned_by_current_thread(&self) -> bool {
        let id = self.get_thread_id.nonzero_thread_id().get();
        self.owner.load(Ordering::Relaxed) == id
    }
}

impl<R: RawMutexFair, G: GetThreadId> RawReentrantMutex<R, G> {
    /// Unlocks this mutex using a fair unlock protocol. The inner mutex
    /// may not be unlocked if this mutex was acquired previously in the
    /// current thread.
    ///
    /// # Safety
    ///
    /// This method may only be called if the mutex is held by the current thread.
    #[inline]
    pub unsafe fn unlock_fair(&self) {
        let lock_count = self.lock_count.get() - 1;
        self.lock_count.set(lock_count);
        if lock_count == 0 {
            self.owner.store(0, Ordering::Relaxed);
            // SAFETY: The final owner takes the state before releasing the mutex.
            let guard = unsafe { (*self.guard.get()).assume_init_read() };
            unsafe { self.mutex.unlock_fair(guard) };
        }
    }

    /// Temporarily yields the mutex to a waiting thread if there is one.
    ///
    /// This method is functionally equivalent to calling `unlock_fair` followed
    /// by `lock`, however it can be much more efficient in the case where there
    /// are no waiting threads.
    ///
    /// # Safety
    ///
    /// This method may only be called if the mutex is held by the current thread.
    #[inline]
    pub unsafe fn bump(&self) {
        if self.lock_count.get() == 1 {
            let id = self.owner.load(Ordering::Relaxed);
            self.owner.store(0, Ordering::Relaxed);
            self.lock_count.set(0);
            // Other threads may use the token storage while bump releases the
            // mutex. Restore our token and metadata once ownership is restored,
            // including when the underlying bump unwinds.
            let state = unsafe { (*self.guard.get()).assume_init_read() };
            let mut guard = scopeguard::guard(state, |state| {
                unsafe {
                    (*self.guard.get()).write(state);
                }
                self.owner.store(id, Ordering::Relaxed);
                self.lock_count.set(1);
            });
            unsafe { self.mutex.bump(&mut guard) };
        }
    }
}

impl<R: RawMutexTimed, G: GetThreadId> RawReentrantMutex<R, G> {
    /// Attempts to acquire this lock until a timeout is reached.
    ///
    /// See [`RawMutexTimed::try_lock_until`] for timeout behavior.
    ///
    /// # Panics
    ///
    /// Panics if the recursive lock count overflows.
    #[inline]
    pub fn try_lock_until(&self, timeout: R::Instant) -> bool {
        self.lock_internal(|| self.mutex.try_lock_until(timeout))
    }

    /// Attempts to acquire this lock until a timeout is reached.
    ///
    /// See [`RawMutexTimed::try_lock_for`] for timeout behavior.
    ///
    /// # Panics
    ///
    /// Panics if the recursive lock count overflows.
    #[inline]
    pub fn try_lock_for(&self, timeout: R::Duration) -> bool {
        self.lock_internal(|| self.mutex.try_lock_for(timeout))
    }
}

/// A reentrant mutual exclusion lock.
///
/// This lock blocks other threads waiting for it to become available. A thread
/// which already holds the lock can acquire it additional times without
/// blocking.
///
/// Unlike [`Mutex`](crate::Mutex), [`ReentrantMutexGuard`] does not provide
/// mutable references to the locked data, because multiple guards can coexist
/// on the same thread. Use interior mutability, such as
/// [`Cell`](core::cell::Cell) or [`RefCell`](core::cell::RefCell), to mutate the
/// guarded data.
pub struct ReentrantMutex<R: RawMutex, G, T: ?Sized> {
    raw: RawReentrantMutex<R, G>,
    data: UnsafeCell<T>,
}

unsafe impl<R: RawMutex + Sync, G: GetThreadId + Sync, T: ?Sized + Send> Sync
    for ReentrantMutex<R, G, T>
{
}

impl<R: RawMutex, G: GetThreadId, T> ReentrantMutex<R, G, T> {
    /// Creates a new reentrant mutex in an unlocked state ready for use.
    #[inline]
    pub const fn new(val: T) -> ReentrantMutex<R, G, T> {
        ReentrantMutex {
            data: UnsafeCell::new(val),
            raw: RawReentrantMutex {
                owner: AtomicUsize::new(0),
                lock_count: Cell::new(0),
                guard: UnsafeCell::new(MaybeUninit::uninit()),
                mutex: R::INIT,
                get_thread_id: G::INIT,
            },
        }
    }

    /// Consumes this mutex, returning the underlying data.
    #[inline]
    pub fn into_inner(self) -> T {
        self.data.into_inner()
    }
}

impl<R: RawMutex, G, T> ReentrantMutex<R, G, T> {
    /// Creates a new reentrant mutex based on a pre-existing raw mutex and a
    /// helper to get the thread ID.
    #[inline]
    pub const fn from_raw(raw_mutex: R, get_thread_id: G, val: T) -> ReentrantMutex<R, G, T> {
        ReentrantMutex {
            data: UnsafeCell::new(val),
            raw: RawReentrantMutex {
                owner: AtomicUsize::new(0),
                lock_count: Cell::new(0),
                guard: UnsafeCell::new(MaybeUninit::uninit()),
                mutex: raw_mutex,
                get_thread_id,
            },
        }
    }

    /// Consumes this mutex, returning the underlying data, raw mutex and
    /// thread ID helper.
    #[inline]
    pub fn into_inner_with_raw(self) -> (R, G, T) {
        (
            self.raw.mutex,
            self.raw.get_thread_id,
            self.data.into_inner(),
        )
    }
}

impl<R: RawMutex, G: GetThreadId, T: ?Sized> ReentrantMutex<R, G, T> {
    /// Creates a new `ReentrantMutexGuard` without checking if the lock is held.
    ///
    /// # Safety
    ///
    /// This method must only be called if the thread logically holds the lock.
    ///
    /// Calling this function when a guard has already been produced is undefined behavior unless
    /// the guard was forgotten with `mem::forget`.
    ///
    /// The caller must ensure that existing references to the protected data
    /// remain valid when the returned guard is used or dropped. In particular,
    /// the returned guard must not permit accesses that conflict with existing
    /// references.
    #[inline]
    pub unsafe fn make_guard_unchecked(&self) -> ReentrantMutexGuard<'_, R, G, T> {
        ReentrantMutexGuard {
            remutex: self,
            marker: PhantomData,
        }
    }

    /// Acquires a reentrant mutex, blocking the current thread until it is able
    /// to do so.
    ///
    /// If the mutex is held by another thread then this function will block the
    /// local thread until it is available to acquire the mutex. If the mutex is
    /// already held by the current thread then this function will increment the
    /// lock reference count and return immediately. Upon returning,
    /// the thread is the only thread with the mutex held. An RAII guard is
    /// returned to allow scoped unlock of the lock. When the guard goes out of
    /// scope, the mutex will be unlocked.
    ///
    /// # Panics
    ///
    /// Panics if the recursive lock count overflows.
    #[inline]
    #[track_caller]
    pub fn lock(&self) -> ReentrantMutexGuard<'_, R, G, T> {
        self.raw.lock();
        // SAFETY: The lock is held, as required.
        unsafe { self.make_guard_unchecked() }
    }

    /// Attempts to acquire this lock.
    ///
    /// If the lock could not be acquired at this time, then `None` is returned.
    /// Otherwise, an RAII guard is returned. The lock will be unlocked when the
    /// guard is dropped.
    ///
    /// This function does not block.
    ///
    /// # Panics
    ///
    /// Panics if the recursive lock count overflows.
    #[inline]
    #[track_caller]
    pub fn try_lock(&self) -> Option<ReentrantMutexGuard<'_, R, G, T>> {
        if self.raw.try_lock() {
            // SAFETY: The lock is held, as required.
            Some(unsafe { self.make_guard_unchecked() })
        } else {
            None
        }
    }

    /// Returns a mutable reference to the underlying data.
    ///
    /// Since this call borrows the `ReentrantMutex` mutably, no actual locking
    /// needs to take place -- the mutable borrow statically guarantees no new
    /// locks can be acquired while the reference exists. This method does not
    /// clear a lock previously abandoned by forgetting a
    /// [`ReentrantMutexGuard`].
    #[inline]
    pub const fn get_mut(&mut self) -> &mut T {
        self.data.get_mut()
    }

    /// Checks whether the mutex is currently locked.
    ///
    /// The result is a momentary snapshot and may be stale by the time it is
    /// returned.
    #[inline]
    #[track_caller]
    pub fn is_locked(&self) -> bool {
        self.raw.is_locked()
    }

    /// Checks whether the mutex is currently held by the current thread.
    #[inline]
    #[track_caller]
    pub fn is_owned_by_current_thread(&self) -> bool {
        self.raw.is_owned_by_current_thread()
    }

    /// Forcibly unlocks the mutex.
    ///
    /// This is useful when combined with `mem::forget` to hold a lock without
    /// the need to maintain a `ReentrantMutexGuard` object alive, for example when
    /// dealing with FFI.
    ///
    /// # Safety
    ///
    /// This method must only be called if the current thread logically owns a
    /// `ReentrantMutexGuard` but that guard has been discarded using
    /// `mem::forget`.
    /// Behavior is undefined if a mutex is unlocked when not locked.
    ///
    /// The caller must ensure that releasing the lock does not invalidate any
    /// outstanding references to the protected data. Any subsequent access
    /// through previously obtained pointers must be properly synchronized.
    #[inline]
    #[track_caller]
    pub unsafe fn force_unlock(&self) {
        unsafe { self.raw.unlock() };
    }

    /// Returns the underlying raw mutex object.
    ///
    /// Note that you will most likely need to import the `RawMutex` trait from
    /// `lock_api` to be able to call functions on the raw mutex.
    ///
    /// # Safety
    ///
    /// The caller must ensure that operations on the raw lock preserve the
    /// validity of all live guards and references to the protected data.
    /// A guard must not be used or dropped while its lock is released, and any
    /// access through previously obtained pointers must be properly synchronized.
    /// Raw operations must also preserve the reentrant mutex's owner and
    /// recursion count bookkeeping.
    #[inline]
    pub unsafe fn raw(&self) -> &R {
        &self.raw.mutex
    }

    /// Returns a raw pointer to the underlying data.
    ///
    /// The returned pointer is always non-null and properly aligned, but the
    /// caller must ensure that accesses through it are valid and properly
    /// synchronized and that the pointer is not used after the mutex is
    /// dropped. In particular, writing requires exclusive access to the data;
    /// merely holding the reentrant mutex does not provide that because
    /// multiple guards can coexist on the same thread.
    #[inline]
    pub const fn data_ptr(&self) -> *mut T {
        self.data.get()
    }

    /// Creates a new `ArcReentrantMutexGuard` without checking if the lock is held.
    ///
    /// # Safety
    ///
    /// This method must only be called if the thread logically holds the lock.
    ///
    /// Calling this function when a guard has already been produced is undefined behavior unless
    /// the guard was forgotten with `mem::forget`.
    ///
    /// The caller must ensure that existing references to the protected data
    /// remain valid when the returned guard is used or dropped. In particular,
    /// the returned guard must not permit accesses that conflict with existing
    /// references.
    #[cfg(feature = "arc_lock")]
    #[inline]
    pub unsafe fn make_arc_guard_unchecked(self: &Arc<Self>) -> ArcReentrantMutexGuard<R, G, T> {
        ArcReentrantMutexGuard {
            remutex: self.clone(),
            data_marker: PhantomData,
            marker: PhantomData,
        }
    }

    /// Acquires a reentrant mutex through an `Arc`.
    ///
    /// This method is similar to the `lock` method; however, it requires the `ReentrantMutex` to be inside of an
    /// `Arc` and the resulting mutex guard owns a clone of the `Arc` instead of borrowing the lock.
    ///
    /// # Panics
    ///
    /// Panics if the recursive lock count overflows.
    #[cfg(feature = "arc_lock")]
    #[inline]
    #[track_caller]
    pub fn lock_arc(self: &Arc<Self>) -> ArcReentrantMutexGuard<R, G, T> {
        self.raw.lock();
        // SAFETY: locking guarantee is upheld
        unsafe { self.make_arc_guard_unchecked() }
    }

    /// Attempts to acquire a reentrant mutex through an `Arc`.
    ///
    /// This method is similar to the `try_lock` method; however, it requires the `ReentrantMutex` to be inside
    /// of an `Arc` and the resulting mutex guard owns a clone of the `Arc` instead of borrowing the lock.
    ///
    /// # Panics
    ///
    /// Panics if the recursive lock count overflows.
    #[cfg(feature = "arc_lock")]
    #[inline]
    #[track_caller]
    pub fn try_lock_arc(self: &Arc<Self>) -> Option<ArcReentrantMutexGuard<R, G, T>> {
        if self.raw.try_lock() {
            // SAFETY: locking guarantee is upheld
            Some(unsafe { self.make_arc_guard_unchecked() })
        } else {
            None
        }
    }
}

impl<R: RawMutexFair, G: GetThreadId, T: ?Sized> ReentrantMutex<R, G, T> {
    /// Forcibly unlocks the mutex using a fair unlock protocol.
    ///
    /// This is useful when combined with `mem::forget` to hold a lock without
    /// the need to maintain a `ReentrantMutexGuard` object alive, for example when
    /// dealing with FFI.
    ///
    /// # Safety
    ///
    /// This method must only be called if the current thread logically owns a
    /// `ReentrantMutexGuard` but that guard has been discarded using
    /// `mem::forget`.
    /// Behavior is undefined if a mutex is unlocked when not locked.
    ///
    /// The caller must ensure that releasing the lock does not invalidate any
    /// outstanding references to the protected data. Any subsequent access
    /// through previously obtained pointers must be properly synchronized.
    #[inline]
    #[track_caller]
    pub unsafe fn force_unlock_fair(&self) {
        unsafe { self.raw.unlock_fair() };
    }
}

impl<R: RawMutexTimed, G: GetThreadId, T: ?Sized> ReentrantMutex<R, G, T> {
    /// Attempts to acquire this lock until a timeout is reached.
    ///
    /// If the lock could not be acquired before the timeout expired, then
    /// `None` is returned. Otherwise, an RAII guard is returned. The lock will
    /// be unlocked when the guard is dropped.
    ///
    /// A successful operation may return early. An unsuccessful operation will
    /// not return before the timeout, but may return later due to scheduling or
    /// platform-specific behavior.
    ///
    /// # Panics
    ///
    /// Panics if the recursive lock count overflows.
    #[inline]
    #[track_caller]
    pub fn try_lock_for(&self, timeout: R::Duration) -> Option<ReentrantMutexGuard<'_, R, G, T>> {
        if self.raw.try_lock_for(timeout) {
            // SAFETY: The lock is held, as required.
            Some(unsafe { self.make_guard_unchecked() })
        } else {
            None
        }
    }

    /// Attempts to acquire this lock until a timeout is reached.
    ///
    /// If the lock could not be acquired before the timeout expired, then
    /// `None` is returned. Otherwise, an RAII guard is returned. The lock will
    /// be unlocked when the guard is dropped.
    ///
    /// A successful operation may return early. An unsuccessful operation will
    /// not return before the timeout, but may return later due to scheduling or
    /// platform-specific behavior.
    ///
    /// # Panics
    ///
    /// Panics if the recursive lock count overflows.
    #[inline]
    #[track_caller]
    pub fn try_lock_until(&self, timeout: R::Instant) -> Option<ReentrantMutexGuard<'_, R, G, T>> {
        if self.raw.try_lock_until(timeout) {
            // SAFETY: The lock is held, as required.
            Some(unsafe { self.make_guard_unchecked() })
        } else {
            None
        }
    }

    /// Attempts to acquire this lock until a timeout is reached, through an `Arc`.
    ///
    /// This method is similar to the `try_lock_for` method; however, it requires the `ReentrantMutex` to be
    /// inside of an `Arc` and the resulting mutex guard owns a clone of the `Arc` instead of borrowing the lock.
    ///
    /// See [`try_lock_for`](Self::try_lock_for) for timeout behavior.
    ///
    /// # Panics
    ///
    /// Panics if the recursive lock count overflows.
    #[cfg(feature = "arc_lock")]
    #[inline]
    #[track_caller]
    pub fn try_lock_arc_for(
        self: &Arc<Self>,
        timeout: R::Duration,
    ) -> Option<ArcReentrantMutexGuard<R, G, T>> {
        if self.raw.try_lock_for(timeout) {
            // SAFETY: locking guarantee is upheld
            Some(unsafe { self.make_arc_guard_unchecked() })
        } else {
            None
        }
    }

    /// Attempts to acquire this lock until a timeout is reached, through an `Arc`.
    ///
    /// This method is similar to the `try_lock_until` method; however, it requires the `ReentrantMutex` to be
    /// inside of an `Arc` and the resulting mutex guard owns a clone of the `Arc` instead of borrowing the lock.
    ///
    /// See [`try_lock_until`](Self::try_lock_until) for timeout behavior.
    ///
    /// # Panics
    ///
    /// Panics if the recursive lock count overflows.
    #[cfg(feature = "arc_lock")]
    #[inline]
    #[track_caller]
    pub fn try_lock_arc_until(
        self: &Arc<Self>,
        timeout: R::Instant,
    ) -> Option<ArcReentrantMutexGuard<R, G, T>> {
        if self.raw.try_lock_until(timeout) {
            // SAFETY: locking guarantee is upheld
            Some(unsafe { self.make_arc_guard_unchecked() })
        } else {
            None
        }
    }
}

impl<R: RawMutex, G: GetThreadId, T: Default> Default for ReentrantMutex<R, G, T> {
    #[inline]
    fn default() -> ReentrantMutex<R, G, T> {
        ReentrantMutex::new(Default::default())
    }
}

impl<R: RawMutex, G: GetThreadId, T> From<T> for ReentrantMutex<R, G, T> {
    #[inline]
    fn from(t: T) -> ReentrantMutex<R, G, T> {
        ReentrantMutex::new(t)
    }
}

impl<R: RawMutex, G: GetThreadId, T: ?Sized + fmt::Debug> fmt::Debug for ReentrantMutex<R, G, T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self.try_lock() {
            Some(guard) => f
                .debug_struct("ReentrantMutex")
                .field("data", &&*guard)
                .finish(),
            None => {
                struct LockedPlaceholder;
                impl fmt::Debug for LockedPlaceholder {
                    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
                        f.write_str("<locked>")
                    }
                }

                f.debug_struct("ReentrantMutex")
                    .field("data", &LockedPlaceholder)
                    .finish()
            }
        }
    }
}

// Copied and modified from serde
#[cfg(feature = "serde")]
impl<R, G, T> Serialize for ReentrantMutex<R, G, T>
where
    R: RawMutex,
    G: GetThreadId,
    T: Serialize + ?Sized,
{
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        self.lock().serialize(serializer)
    }
}

#[cfg(feature = "serde")]
impl<'de, R, G, T> Deserialize<'de> for ReentrantMutex<R, G, T>
where
    R: RawMutex,
    G: GetThreadId,
    T: Deserialize<'de>,
{
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        Deserialize::deserialize(deserializer).map(ReentrantMutex::new)
    }
}

/// An RAII guard which releases one level of recursive locking when dropped.
///
/// The data protected by the mutex can be accessed through this guard via its
/// [`Deref`] implementation.
///
/// This structure is created by the [`lock`](ReentrantMutex::lock) and
/// [`try_lock`](ReentrantMutex::try_lock) methods on [`ReentrantMutex`].
///
/// # Mutability
///
/// Unlike [`MutexGuard`](crate::MutexGuard), `ReentrantMutexGuard` does not
/// implement [`DerefMut`](core::ops::DerefMut), because doing so would violate Rust's reference
/// aliasing rules when multiple guards exist on one thread. Use interior
/// mutability, such as [`Cell`](core::cell::Cell) or
/// [`RefCell`](core::cell::RefCell), to mutate the guarded data.
#[clippy::has_significant_drop]
#[must_use = "if unused the ReentrantMutex will immediately unlock"]
pub struct ReentrantMutexGuard<'a, R: RawMutex, G: GetThreadId, T: ?Sized> {
    remutex: &'a ReentrantMutex<R, G, T>,
    marker: PhantomData<(&'a T, GuardNoSend)>,
}

impl<'a, R: RawMutex + 'a, G: GetThreadId + 'a, T: ?Sized + 'a> ReentrantMutexGuard<'a, R, G, T> {
    /// Returns a reference to the original `ReentrantMutex` object.
    pub fn remutex(s: &Self) -> &'a ReentrantMutex<R, G, T> {
        s.remutex
    }

    /// Makes a new `MappedReentrantMutexGuard` for a component of the locked data.
    ///
    /// This is an associated function that needs to be
    /// used as `ReentrantMutexGuard::map(...)`. A method would interfere with methods of
    /// the same name on the contents of the locked data.
    ///
    /// # Panics
    ///
    /// If `f` panics, the original guard is dropped.
    #[inline]
    pub fn map<U: ?Sized, F>(s: Self, f: F) -> MappedReentrantMutexGuard<'a, R, G, U>
    where
        F: FnOnce(&T) -> &U,
    {
        let raw = &s.remutex.raw;
        let data = f(unsafe { s.remutex.data.get().as_ref_unchecked() });
        mem::forget(s);
        MappedReentrantMutexGuard {
            raw,
            data: SharedGuardData::new(data),
            marker: PhantomData,
        }
    }

    /// Attempts to make a new `MappedReentrantMutexGuard` for a component of the
    /// locked data. The original guard is returned if the closure returns
    /// `None`.
    ///
    /// This is an associated function that needs to be
    /// used as `ReentrantMutexGuard::try_map(...)`. A method would interfere with methods of
    /// the same name on the contents of the locked data.
    ///
    /// # Panics
    ///
    /// If `f` panics, the original guard is dropped.
    #[inline]
    pub fn try_map<U: ?Sized, F>(
        s: Self,
        f: F,
    ) -> Result<MappedReentrantMutexGuard<'a, R, G, U>, Self>
    where
        F: FnOnce(&T) -> Option<&U>,
    {
        let raw = &s.remutex.raw;
        let Some(data) = f(unsafe { s.remutex.data.get().as_ref_unchecked() }) else {
            return Err(s);
        };
        mem::forget(s);
        Ok(MappedReentrantMutexGuard {
            raw,
            data: SharedGuardData::new(data),
            marker: PhantomData,
        })
    }

    /// Attempts to make a new `MappedReentrantMutexGuard` for a component of the
    /// locked data. The original guard is returned alongside arbitrary user data
    /// if the closure returns `Err`.
    ///
    /// This is an associated function that needs to be
    /// used as `ReentrantMutexGuard::try_map_or_err(...)`. A method would interfere with methods of
    /// the same name on the contents of the locked data.
    ///
    /// # Panics
    ///
    /// If `f` panics, the original guard is dropped.
    #[inline]
    pub fn try_map_or_err<U: ?Sized, F, E>(
        s: Self,
        f: F,
    ) -> Result<MappedReentrantMutexGuard<'a, R, G, U>, (Self, E)>
    where
        F: FnOnce(&T) -> Result<&U, E>,
    {
        let raw = &s.remutex.raw;
        let data = match f(unsafe { s.remutex.data.get().as_ref_unchecked() }) {
            Ok(data) => data,
            Err(e) => return Err((s, e)),
        };
        mem::forget(s);
        Ok(MappedReentrantMutexGuard {
            raw,
            data: SharedGuardData::new(data),
            marker: PhantomData,
        })
    }

    /// Temporarily unlocks the mutex to execute the given function.
    ///
    /// This releases only one level of recursive locking. The underlying mutex
    /// remains locked if the current thread has acquired it more than once.
    ///
    /// # Aborts
    ///
    /// Aborts if re-locking the mutex panics.
    #[inline]
    #[track_caller]
    pub fn unlocked<F, U>(s: &mut Self, f: F) -> U
    where
        F: FnOnce() -> U,
    {
        // Restore the recursive acquisition even if the final raw unlock panics.
        defer!(abort_on_panic(|| s.remutex.raw.lock()));
        // SAFETY: This guard owns one recursive acquisition on the current thread.
        unsafe { s.remutex.raw.unlock() };
        f()
    }
}

impl<'a, R: RawMutexFair + 'a, G: GetThreadId + 'a, T: ?Sized + 'a>
    ReentrantMutexGuard<'a, R, G, T>
{
    /// Releases one level of recursive locking. A fair unlock is performed
    /// only if this releases the final level.
    #[inline]
    #[track_caller]
    pub fn unlock_fair(s: Self) {
        let s = ManuallyDrop::new(s);
        // Safety: A ReentrantMutexGuard always holds the lock
        unsafe {
            s.remutex.raw.unlock_fair();
        }
    }

    /// Temporarily unlocks the mutex to execute the given function.
    ///
    /// This releases only one level of recursive locking. A fair unlock is
    /// performed only if this releases the final level.
    ///
    /// # Aborts
    ///
    /// Aborts if re-locking the mutex panics.
    #[inline]
    #[track_caller]
    pub fn unlocked_fair<F, U>(s: &mut Self, f: F) -> U
    where
        F: FnOnce() -> U,
    {
        // Restore the recursive acquisition even if the final raw unlock panics.
        defer!(abort_on_panic(|| s.remutex.raw.lock()));
        // SAFETY: This guard owns one recursive acquisition on the current thread.
        unsafe { s.remutex.raw.unlock_fair() };
        f()
    }

    /// Temporarily yields the mutex to a waiting thread if there is one.
    ///
    /// This method is functionally equivalent to calling `unlock_fair` followed
    /// by `lock`, however it can be much more efficient in the case where there
    /// are no waiting threads.
    #[inline]
    #[track_caller]
    pub fn bump(s: &mut Self) {
        // Safety: A ReentrantMutexGuard always holds the lock
        unsafe { s.remutex.raw.bump() };
    }
}

impl<'a, R: RawMutex + 'a, G: GetThreadId + 'a, T: ?Sized + 'a> Deref
    for ReentrantMutexGuard<'a, R, G, T>
{
    type Target = T;
    #[inline]
    fn deref(&self) -> &T {
        unsafe { self.remutex.data.get().as_ref_unchecked() }
    }
}

impl<'a, R: RawMutex + 'a, G: GetThreadId + 'a, T: ?Sized + 'a> Drop
    for ReentrantMutexGuard<'a, R, G, T>
{
    #[inline]
    fn drop(&mut self) {
        // Safety: A ReentrantMutexGuard always holds the lock.
        unsafe {
            self.remutex.raw.unlock();
        }
    }
}

impl<'a, R: RawMutex + 'a, G: GetThreadId + 'a, T: fmt::Debug + ?Sized + 'a> fmt::Debug
    for ReentrantMutexGuard<'a, R, G, T>
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Debug::fmt(&**self, f)
    }
}

impl<'a, R: RawMutex + 'a, G: GetThreadId + 'a, T: fmt::Display + ?Sized + 'a> fmt::Display
    for ReentrantMutexGuard<'a, R, G, T>
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        (**self).fmt(f)
    }
}

#[cfg(feature = "owning_ref")]
unsafe impl<'a, R: RawMutex + 'a, G: GetThreadId + 'a, T: ?Sized + 'a> StableAddress
    for ReentrantMutexGuard<'a, R, G, T>
{
}

/// An RAII mutex guard returned by the `Arc` locking operations on
/// [`ReentrantMutex`].
///
/// Unlike [`ReentrantMutexGuard`], this guard owns the [`Arc`] used to unlock
/// the mutex instead of borrowing the mutex, so its lifetime is not tied to a
/// lock reference.
#[cfg(feature = "arc_lock")]
#[clippy::has_significant_drop]
#[must_use = "if unused the ReentrantMutex will immediately unlock"]
pub struct ArcReentrantMutexGuard<R: RawMutex, G: GetThreadId, T: ?Sized> {
    remutex: Arc<ReentrantMutex<R, G, T>>,
    data_marker: PhantomData<T>,
    marker: PhantomData<GuardNoSend>,
}

#[cfg(feature = "arc_lock")]
impl<R: RawMutex, G: GetThreadId, T: ?Sized> ArcReentrantMutexGuard<R, G, T> {
    /// Returns a reference to the `ReentrantMutex` this object is guarding, contained in its `Arc`.
    pub fn remutex(s: &Self) -> &Arc<ReentrantMutex<R, G, T>> {
        &s.remutex
    }

    /// Releases one level of recursive locking and returns the `Arc` held by
    /// the [`ArcReentrantMutexGuard`]. The underlying mutex remains locked if
    /// the current thread has acquired it more than once.
    #[inline]
    pub fn into_arc(s: Self) -> Arc<ReentrantMutex<R, G, T>> {
        let s = ManuallyDrop::new(s);
        // SAFETY: ManuallyDrop lets us move the Arc into a local that drops on unwind.
        let remutex = unsafe { ptr::read(&s.remutex) };
        // SAFETY: We own this recursive acquisition; ManuallyDrop prevents a second unlock.
        unsafe { remutex.raw.unlock() };
        remutex
    }

    /// Temporarily unlocks the mutex to execute the given function.
    ///
    /// This releases only one level of recursive locking. The underlying mutex
    /// remains locked if the current thread has acquired it more than once.
    ///
    /// # Aborts
    ///
    /// Aborts if re-locking the mutex panics.
    #[inline]
    #[track_caller]
    pub fn unlocked<F, U>(s: &mut Self, f: F) -> U
    where
        F: FnOnce() -> U,
    {
        // Restore the recursive acquisition even if the final raw unlock panics.
        defer!(abort_on_panic(|| s.remutex.raw.lock()));
        // SAFETY: This guard owns one recursive acquisition on the current thread.
        unsafe { s.remutex.raw.unlock() };
        f()
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawMutexFair, G: GetThreadId, T: ?Sized> ArcReentrantMutexGuard<R, G, T> {
    /// Releases one level of recursive locking. A fair unlock is performed
    /// only if this releases the final level.
    #[inline]
    #[track_caller]
    pub fn unlock_fair(s: Self) {
        drop(Self::into_arc_fair(s));
    }

    /// Releases one level of recursive locking and returns the `Arc` held by
    /// the [`ArcReentrantMutexGuard`]. A fair unlock is performed only if this
    /// releases the final level.
    #[inline]
    pub fn into_arc_fair(s: Self) -> Arc<ReentrantMutex<R, G, T>> {
        let s = ManuallyDrop::new(s);
        // SAFETY: ManuallyDrop lets us move the Arc into a local that drops on unwind.
        let remutex = unsafe { ptr::read(&s.remutex) };
        // SAFETY: We own this recursive acquisition; ManuallyDrop prevents a second unlock.
        unsafe { remutex.raw.unlock_fair() };
        remutex
    }

    /// Temporarily unlocks the mutex to execute the given function.
    ///
    /// This releases only one level of recursive locking. A fair unlock is
    /// performed only if this releases the final level.
    ///
    /// # Aborts
    ///
    /// Aborts if re-locking the mutex panics.
    #[inline]
    #[track_caller]
    pub fn unlocked_fair<F, U>(s: &mut Self, f: F) -> U
    where
        F: FnOnce() -> U,
    {
        // Restore the recursive acquisition even if the final raw unlock panics.
        defer!(abort_on_panic(|| s.remutex.raw.lock()));
        // SAFETY: This guard owns one recursive acquisition on the current thread.
        unsafe { s.remutex.raw.unlock_fair() };
        f()
    }

    /// Temporarily yields the mutex to a waiting thread if there is one.
    ///
    /// This is functionally equivalent to the `bump` method on [`ReentrantMutexGuard`].
    #[inline]
    #[track_caller]
    pub fn bump(s: &mut Self) {
        // Safety: A ReentrantMutexGuard always holds the lock
        unsafe { s.remutex.raw.bump() };
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawMutex, G: GetThreadId, T: ?Sized> Deref for ArcReentrantMutexGuard<R, G, T> {
    type Target = T;
    #[inline]
    fn deref(&self) -> &T {
        unsafe { self.remutex.data.get().as_ref_unchecked() }
    }
}

#[cfg(feature = "arc_lock")]
impl<R: RawMutex, G: GetThreadId, T: ?Sized> Drop for ArcReentrantMutexGuard<R, G, T> {
    #[inline]
    fn drop(&mut self) {
        // Safety: A ReentrantMutexGuard always holds the lock.
        unsafe {
            self.remutex.raw.unlock();
        }
    }
}

/// An RAII mutex guard returned by [`ReentrantMutexGuard::map`], which can point to a
/// subfield of the protected data.
///
/// The main difference between `MappedReentrantMutexGuard` and `ReentrantMutexGuard` is that the
/// former doesn't support temporarily unlocking and re-locking, since that
/// could introduce soundness issues if the locked object is modified by another
/// thread.
///
/// This structure is created by the [`map`](ReentrantMutexGuard::map) and
/// [`try_map`](ReentrantMutexGuard::try_map), and
/// [`try_map_or_err`](ReentrantMutexGuard::try_map_or_err) functions on
/// [`ReentrantMutexGuard`].
#[clippy::has_significant_drop]
#[must_use = "if unused the ReentrantMutex will immediately unlock"]
pub struct MappedReentrantMutexGuard<'a, R: RawMutex, G: GetThreadId, T: ?Sized + 'a> {
    raw: &'a RawReentrantMutex<R, G>,
    data: SharedGuardData<T>,
    marker: PhantomData<GuardNoSend>,
}

impl<'a, R: RawMutex + 'a, G: GetThreadId + 'a, T: ?Sized + 'a>
    MappedReentrantMutexGuard<'a, R, G, T>
{
    /// Makes a new `MappedReentrantMutexGuard` for a component of the locked data.
    ///
    /// This is an associated function that needs to be
    /// used as `MappedReentrantMutexGuard::map(...)`. A method would interfere with methods of
    /// the same name on the contents of the locked data.
    ///
    /// # Panics
    ///
    /// If `f` panics, the original guard is dropped.
    #[inline]
    pub fn map<U: ?Sized, F>(s: Self, f: F) -> MappedReentrantMutexGuard<'a, R, G, U>
    where
        F: FnOnce(&T) -> &U,
    {
        let raw = s.raw;
        let data = SharedGuardData::new(f(unsafe { s.data.as_ref() }));
        mem::forget(s);
        MappedReentrantMutexGuard {
            raw,
            data,
            marker: PhantomData,
        }
    }

    /// Attempts to make a new `MappedReentrantMutexGuard` for a component of the
    /// locked data. The original guard is returned if the closure returns `None`.
    ///
    /// This is an associated function that needs to be
    /// used as `MappedReentrantMutexGuard::try_map(...)`. A method would interfere with methods of
    /// the same name on the contents of the locked data.
    ///
    /// # Panics
    ///
    /// If `f` panics, the original guard is dropped.
    #[inline]
    pub fn try_map<U: ?Sized, F>(
        s: Self,
        f: F,
    ) -> Result<MappedReentrantMutexGuard<'a, R, G, U>, Self>
    where
        F: FnOnce(&T) -> Option<&U>,
    {
        let raw = s.raw;
        let Some(data) = f(unsafe { s.data.as_ref() }) else {
            return Err(s);
        };
        let data = SharedGuardData::new(data);
        mem::forget(s);
        Ok(MappedReentrantMutexGuard {
            raw,
            data,
            marker: PhantomData,
        })
    }

    /// Attempts to make a new `MappedReentrantMutexGuard` for a component of the
    /// locked data. The original guard is returned alongside arbitrary user data
    /// if the closure returns `Err`.
    ///
    /// This is an associated function that needs to be
    /// used as `MappedReentrantMutexGuard::try_map_or_err(...)`. A method would interfere with methods of
    /// the same name on the contents of the locked data.
    ///
    /// # Panics
    ///
    /// If `f` panics, the original guard is dropped.
    #[inline]
    pub fn try_map_or_err<U: ?Sized, F, E>(
        s: Self,
        f: F,
    ) -> Result<MappedReentrantMutexGuard<'a, R, G, U>, (Self, E)>
    where
        F: FnOnce(&T) -> Result<&U, E>,
    {
        let raw = s.raw;
        let data = match f(unsafe { s.data.as_ref() }) {
            Ok(data) => data,
            Err(e) => return Err((s, e)),
        };
        let data = SharedGuardData::new(data);
        mem::forget(s);
        Ok(MappedReentrantMutexGuard {
            raw,
            data,
            marker: PhantomData,
        })
    }
}

impl<'a, R: RawMutexFair + 'a, G: GetThreadId + 'a, T: ?Sized + 'a>
    MappedReentrantMutexGuard<'a, R, G, T>
{
    /// Releases one level of recursive locking. A fair unlock is performed
    /// only if this releases the final level.
    #[inline]
    #[track_caller]
    pub fn unlock_fair(s: Self) {
        let s = ManuallyDrop::new(s);
        // Safety: A MappedReentrantMutexGuard always holds the lock
        unsafe {
            s.raw.unlock_fair();
        }
    }
}

impl<'a, R: RawMutex + 'a, G: GetThreadId + 'a, T: ?Sized + 'a> Deref
    for MappedReentrantMutexGuard<'a, R, G, T>
{
    type Target = T;
    #[inline]
    fn deref(&self) -> &T {
        unsafe { self.data.as_ref() }
    }
}

impl<'a, R: RawMutex + 'a, G: GetThreadId + 'a, T: ?Sized + 'a> Drop
    for MappedReentrantMutexGuard<'a, R, G, T>
{
    #[inline]
    fn drop(&mut self) {
        // Safety: A MappedReentrantMutexGuard always holds the lock.
        unsafe {
            self.raw.unlock();
        }
    }
}

impl<'a, R: RawMutex + 'a, G: GetThreadId + 'a, T: fmt::Debug + ?Sized + 'a> fmt::Debug
    for MappedReentrantMutexGuard<'a, R, G, T>
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Debug::fmt(&**self, f)
    }
}

impl<'a, R: RawMutex + 'a, G: GetThreadId + 'a, T: fmt::Display + ?Sized + 'a> fmt::Display
    for MappedReentrantMutexGuard<'a, R, G, T>
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        (**self).fmt(f)
    }
}

#[cfg(feature = "owning_ref")]
unsafe impl<'a, R: RawMutex + 'a, G: GetThreadId + 'a, T: ?Sized + 'a> StableAddress
    for MappedReentrantMutexGuard<'a, R, G, T>
{
}
