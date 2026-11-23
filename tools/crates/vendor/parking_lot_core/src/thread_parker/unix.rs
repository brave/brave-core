#[cfg(not(target_vendor = "apple"))]
use core::mem::MaybeUninit;
use core::{
    cell::{Cell, UnsafeCell},
    ptr::NonNull,
};
use libc;
use std::time::Instant;
use std::{thread, time::Duration};

// x32 Linux uses a non-standard type for tv_nsec in timespec.
// See https://sourceware.org/bugzilla/show_bug.cgi?id=16437
#[cfg(all(target_arch = "x86_64", target_pointer_width = "32"))]
#[allow(non_camel_case_types)]
type tv_nsec_t = i64;
#[cfg(not(all(target_arch = "x86_64", target_pointer_width = "32")))]
#[allow(non_camel_case_types)]
type tv_nsec_t = libc::c_long;

// Helper type for putting a thread to sleep until some other thread wakes it up
pub struct ThreadParker {
    should_park: Cell<bool>,
    mutex: UnsafeCell<libc::pthread_mutex_t>,
    condvar: UnsafeCell<libc::pthread_cond_t>,
    initialized: Cell<bool>,
}

impl super::ThreadParkerT for ThreadParker {
    type UnparkHandle = UnparkHandle;

    const IS_CHEAP_TO_CONSTRUCT: bool = false;

    #[inline]
    fn new() -> ThreadParker {
        ThreadParker {
            should_park: Cell::new(false),
            mutex: UnsafeCell::new(libc::PTHREAD_MUTEX_INITIALIZER),
            condvar: UnsafeCell::new(libc::PTHREAD_COND_INITIALIZER),
            initialized: Cell::new(false),
        }
    }

    #[inline]
    unsafe fn prepare_park(&self) {
        self.should_park.set(true);
        if !self.initialized.get() {
            unsafe { self.init() };
            self.initialized.set(true);
        }
    }

    #[inline]
    unsafe fn timed_out(&self) -> bool {
        // We need to grab the mutex here because another thread may be
        // concurrently executing UnparkHandle::unpark, which is done without
        // holding the queue lock.
        let r = unsafe { libc::pthread_mutex_lock(self.mutex.get()) };
        assert_eq!(r, 0);
        let should_park = self.should_park.get();
        let r = unsafe { libc::pthread_mutex_unlock(self.mutex.get()) };
        assert_eq!(r, 0);
        should_park
    }

    #[inline]
    unsafe fn park(&self) {
        let r = unsafe { libc::pthread_mutex_lock(self.mutex.get()) };
        assert_eq!(r, 0);
        while self.should_park.get() {
            let r = unsafe { libc::pthread_cond_wait(self.condvar.get(), self.mutex.get()) };
            assert_eq!(r, 0);
        }
        let r = unsafe { libc::pthread_mutex_unlock(self.mutex.get()) };
        assert_eq!(r, 0);
    }

    #[inline]
    unsafe fn park_until(&self, timeout: Instant) -> bool {
        let r = unsafe { libc::pthread_mutex_lock(self.mutex.get()) };
        assert_eq!(r, 0);
        while self.should_park.get() {
            let now = Instant::now();
            if timeout <= now {
                let r = unsafe { libc::pthread_mutex_unlock(self.mutex.get()) };
                assert_eq!(r, 0);
                return false;
            }

            let remaining = timeout - now;
            if let Some(ts) = timeout_to_timespec(remaining) {
                let pthread_cond_timedwait =
                    cfg_select! {
                        target_vendor = "apple" => libc::pthread_cond_timedwait_relative_np,
                        _ => libc::pthread_cond_timedwait,
                    };
                let r =
                    unsafe { pthread_cond_timedwait(self.condvar.get(), self.mutex.get(), &ts) };
                if ts.tv_sec < 0 {
                    // On some systems, negative timeouts will return EINVAL. In
                    // that case we won't sleep and will just busy loop instead,
                    // which is the best we can do.
                    assert!(r == 0 || r == libc::ETIMEDOUT || r == libc::EINVAL);
                } else {
                    assert!(r == 0 || r == libc::ETIMEDOUT);
                }
            } else {
                // The platform cannot represent this absolute deadline, so use
                // an untimed wait.
                let r = unsafe { libc::pthread_cond_wait(self.condvar.get(), self.mutex.get()) };
                assert_eq!(r, 0);
            }
        }
        let r = unsafe { libc::pthread_mutex_unlock(self.mutex.get()) };
        assert_eq!(r, 0);
        true
    }

    #[inline]
    unsafe fn unpark_lock(&self) -> UnparkHandle {
        let r = unsafe { libc::pthread_mutex_lock(self.mutex.get()) };
        assert_eq!(r, 0);

        UnparkHandle {
            thread_parker: NonNull::from(self),
        }
    }
}

impl ThreadParker {
    /// No initialization is needed on platforms which don't support selecting
    /// the condvar clock.
    #[cfg(any(target_vendor = "apple", target_os = "espidf", target_os = "l4re"))]
    #[inline]
    unsafe fn init(&self) {}

    /// Initializes the condvar to use CLOCK_MONOTONIC instead of CLOCK_REALTIME.
    #[cfg(not(any(target_vendor = "apple", target_os = "espidf", target_os = "l4re")))]
    #[inline]
    unsafe fn init(&self) {
        let mut attr = MaybeUninit::<libc::pthread_condattr_t>::uninit();
        let r = unsafe { libc::pthread_condattr_init(attr.as_mut_ptr()) };
        assert_eq!(r, 0);
        let r =
            unsafe { libc::pthread_condattr_setclock(attr.as_mut_ptr(), libc::CLOCK_MONOTONIC) };
        assert_eq!(r, 0);
        let r = unsafe { libc::pthread_cond_init(self.condvar.get(), attr.as_ptr()) };
        assert_eq!(r, 0);
        let r = unsafe { libc::pthread_condattr_destroy(attr.as_mut_ptr()) };
        assert_eq!(r, 0);
    }
}

impl Drop for ThreadParker {
    #[inline]
    fn drop(&mut self) {
        // On DragonFly pthread_mutex_destroy() returns EINVAL if called on a
        // mutex that was just initialized with libc::PTHREAD_MUTEX_INITIALIZER.
        // Once it is used (locked/unlocked) or pthread_mutex_init() is called,
        // this behavior no longer occurs. The same applies to condvars.
        unsafe {
            let r = libc::pthread_mutex_destroy(self.mutex.get());
            assert!(r == 0 || r == libc::EINVAL);
            let r = libc::pthread_cond_destroy(self.condvar.get());
            assert!(r == 0 || r == libc::EINVAL);
        }
    }
}

pub struct UnparkHandle {
    thread_parker: NonNull<ThreadParker>,
}

impl super::UnparkHandleT for UnparkHandle {
    #[inline]
    unsafe fn unpark(self) {
        let thread_parker = unsafe { self.thread_parker.as_ref() };
        thread_parker.should_park.set(false);

        // We notify while holding the lock here to avoid races with the target
        // thread. In particular, the thread could exit after we unlock the
        // mutex, which would make the condvar access invalid memory.
        let r = unsafe { libc::pthread_cond_signal(thread_parker.condvar.get()) };
        assert_eq!(r, 0);
        let r = unsafe { libc::pthread_mutex_unlock(thread_parker.mutex.get()) };
        assert_eq!(r, 0);
    }
}

// Returns the current time on the clock used by pthread_cond_t as a timespec.
#[cfg(not(target_vendor = "apple"))]
#[inline]
fn timespec_now() -> libc::timespec {
    let mut now = MaybeUninit::<libc::timespec>::uninit();
    let clock = if cfg!(any(target_os = "espidf", target_os = "l4re")) {
        // These platforms don't support pthread_condattr_setclock, so we need
        // to specify the timeout in CLOCK_REALTIME.
        libc::CLOCK_REALTIME
    } else {
        libc::CLOCK_MONOTONIC
    };
    let r = unsafe { libc::clock_gettime(clock, now.as_mut_ptr()) };
    assert_eq!(r, 0);
    // SAFETY: We know `libc::clock_gettime` has initialized the value.
    unsafe { now.assume_init() }
}

// On Apple, generate a relative timespec directly for
// pthread_cond_timedwait_relative_np.
#[cfg(target_vendor = "apple")]
#[inline]
fn timeout_to_timespec(timeout: Duration) -> Option<libc::timespec> {
    // Older Apple condvar implementations convert relative timeouts to u64
    // nanoseconds without checking for overflow.
    const MAX_DURATION: Duration = Duration::from_nanos(u64::MAX);
    let timeout = timeout.min(MAX_DURATION);
    Some(libc::timespec {
        tv_sec: libc::time_t::try_from(timeout.as_secs()).ok()?,
        tv_nsec: timeout.subsec_nanos() as tv_nsec_t,
    })
}

// Converts a relative timeout into an absolute timeout in the clock used by
// pthread_cond_t.
#[cfg(not(target_vendor = "apple"))]
#[inline]
fn timeout_to_timespec(timeout: Duration) -> Option<libc::timespec> {
    // Handle overflows early on
    if timeout.as_secs() > libc::time_t::MAX as u64 {
        return None;
    }

    let now = timespec_now();
    let mut nsec = now.tv_nsec + timeout.subsec_nanos() as tv_nsec_t;
    let mut sec = now.tv_sec.checked_add(timeout.as_secs() as libc::time_t);
    if nsec >= 1_000_000_000 {
        nsec -= 1_000_000_000;
        sec = sec.and_then(|sec| sec.checked_add(1));
    }

    sec.map(|sec| libc::timespec {
        tv_nsec: nsec,
        tv_sec: sec,
    })
}

#[inline]
pub fn thread_yield() {
    thread::yield_now();
}
