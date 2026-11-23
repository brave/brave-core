use core::{
    ptr::{self, NonNull},
    sync::atomic::{AtomicBool, AtomicI32, Ordering},
};
use std::thread;
use std::time::{Duration, Instant};

fn errno() -> libc::c_int {
    #[cfg(target_os = "linux")]
    unsafe {
        *libc::__errno_location()
    }
    #[cfg(target_os = "android")]
    unsafe {
        *libc::__errno()
    }
}

#[inline]
fn raw_futex_syscall(
    futex: *const AtomicI32,
    syscall: libc::c_long,
    operation: libc::c_int,
    timeout: *const libc::c_void,
) -> Result<libc::c_long, libc::c_int> {
    let result = unsafe {
        libc::syscall(
            syscall,
            futex.cast_mut().cast::<u32>(),
            operation,
            1,
            timeout.cast_mut().cast::<libc::timespec>(),
        )
    };
    if result == -1 {
        Err(errno())
    } else {
        Ok(result)
    }
}

// The futex timeout uses the kernel syscall ABI, which may differ from libc's
// `timespec` ABI. Native 64-bit, x32 and futex_time64 syscalls use 64-bit
// storage slots. Legacy 32-bit futex syscalls use two 32-bit fields.
cfg_select! {
    target_arch = "m68k" => {
        const SYS_FUTEX_TIME32: Option<libc::c_long> = Some(libc::SYS_futex_time32);
        const SYS_FUTEX_TIME64: Option<libc::c_long> = Some(libc::SYS_futex as libc::c_long);
    }
    any(target_arch = "mips", target_arch = "mips32r6") => {
        const SYS_FUTEX_TIME32: Option<libc::c_long> = Some(libc::SYS_futex as libc::c_long);
        // MIPS syscall numbers include the o32 ABI base.
        const SYS_FUTEX_TIME64: Option<libc::c_long> = Some(4000 + 422);
    }
    any(target_arch = "hexagon", target_arch = "riscv32") => {
        const SYS_FUTEX_TIME32: Option<libc::c_long> = None;
        const SYS_FUTEX_TIME64: Option<libc::c_long> = Some(422);
    }
    all(target_pointer_width = "32", not(target_arch = "x86_64")) => {
        const SYS_FUTEX_TIME32: Option<libc::c_long> = Some(libc::SYS_futex as libc::c_long);
        const SYS_FUTEX_TIME64: Option<libc::c_long> = Some(422);
    }
    _ => {
        const SYS_FUTEX_TIME32: Option<libc::c_long> = None;
        const SYS_FUTEX_TIME64: Option<libc::c_long> = Some(libc::SYS_futex as libc::c_long);
    }
}

const _: () = assert!(SYS_FUTEX_TIME32.is_some() || SYS_FUTEX_TIME64.is_some());

#[repr(C)]
struct Timespec32 {
    tv_sec: i32,
    tv_nsec: i32,
}

#[repr(C)]
struct Timespec64 {
    tv_sec: i64,
    tv_nsec: i64,
}

// Prefer the time64 syscall when both variants are available. Kernel syscall
// support is process-wide, so cache an ENOSYS result without increasing the
// size of each ThreadParker.
static FUTEX_TIME64_SUPPORTED: AtomicBool = AtomicBool::new(true);

#[inline]
fn futex_syscall(
    futex: *const AtomicI32,
    operation: libc::c_int,
    timeout32: *const libc::c_void,
    timeout64: *const libc::c_void,
) -> Result<libc::c_long, libc::c_int> {
    match (SYS_FUTEX_TIME32, SYS_FUTEX_TIME64) {
        (Some(time32), Some(time64)) => {
            if FUTEX_TIME64_SUPPORTED.load(Ordering::Relaxed) {
                match raw_futex_syscall(futex, time64, operation, timeout64) {
                    Err(libc::ENOSYS) => {
                        FUTEX_TIME64_SUPPORTED.store(false, Ordering::Relaxed);
                    }
                    result => return result,
                }
            }
            raw_futex_syscall(futex, time32, operation, timeout32)
        }
        (Some(time32), None) => raw_futex_syscall(futex, time32, operation, timeout32),
        (None, Some(time64)) => raw_futex_syscall(futex, time64, operation, timeout64),
        (None, None) => unreachable!(),
    }
}

// Helper type for putting a thread to sleep until some other thread wakes it up
pub struct ThreadParker {
    futex: AtomicI32,
}

impl super::ThreadParkerT for ThreadParker {
    type UnparkHandle = UnparkHandle;

    const IS_CHEAP_TO_CONSTRUCT: bool = true;

    #[inline]
    fn new() -> ThreadParker {
        ThreadParker {
            futex: AtomicI32::new(0),
        }
    }

    #[inline]
    unsafe fn prepare_park(&self) {
        self.futex.store(1, Ordering::Relaxed);
    }

    #[inline]
    unsafe fn timed_out(&self) -> bool {
        self.futex.load(Ordering::Relaxed) != 0
    }

    #[inline]
    unsafe fn park(&self) {
        while self.futex.load(Ordering::Acquire) != 0 {
            self.futex_wait(None);
        }
    }

    #[inline]
    unsafe fn park_until(&self, timeout: Instant) -> bool {
        while self.futex.load(Ordering::Acquire) != 0 {
            let now = Instant::now();
            if timeout <= now {
                return false;
            }
            let diff = timeout - now;
            self.futex_wait(Some(diff));
        }
        true
    }

    // Marks the thread as unparked while holding the queue lock. A late futex
    // wake remains harmless even if the target's ThreadData has been freed.
    #[inline]
    unsafe fn unpark_lock(&self) -> UnparkHandle {
        // The target may destroy the parker as soon as the store below is
        // observed, so construct the handle first.
        let handle = UnparkHandle {
            futex: NonNull::from(&self.futex),
        };

        // We don't need to lock anything, just clear the state
        self.futex.store(0, Ordering::Release);
        handle
    }
}

impl ThreadParker {
    #[inline]
    fn futex_wait(&self, timeout: Option<Duration>) {
        let timed = timeout.is_some();
        let ts32 = timeout.map(|timeout| Timespec32 {
            tv_sec: i32::try_from(timeout.as_secs()).unwrap_or(i32::MAX),
            tv_nsec: timeout.subsec_nanos() as i32,
        });
        let ts64 = timeout.map(|timeout| Timespec64 {
            tv_sec: i64::try_from(timeout.as_secs()).unwrap_or(i64::MAX),
            tv_nsec: i64::from(timeout.subsec_nanos()),
        });
        let ts32_ptr = match &ts32 {
            Some(ts) => ptr::from_ref(ts).cast(),
            None => ptr::null(),
        };
        let ts64_ptr = match &ts64 {
            Some(ts) => ptr::from_ref(ts).cast(),
            None => ptr::null(),
        };
        let result = futex_syscall(
            &self.futex,
            libc::FUTEX_WAIT | libc::FUTEX_PRIVATE_FLAG,
            ts32_ptr,
            ts64_ptr,
        );

        match result {
            Ok(0) | Err(libc::EINTR) | Err(libc::EAGAIN) => {}
            Err(libc::ETIMEDOUT) if timed => {}
            Ok(result) => panic!("unexpected futex wait result: {result}"),
            Err(error) => panic!("futex wait failed with error {error}"),
        }
    }
}

pub struct UnparkHandle {
    futex: NonNull<AtomicI32>,
}

impl super::UnparkHandleT for UnparkHandle {
    #[inline]
    unsafe fn unpark(self) {
        // The thread data may have been freed at this point, but it doesn't
        // matter since the syscall will just return EFAULT in that case.
        let result = futex_syscall(
            self.futex.as_ptr(),
            libc::FUTEX_WAKE | libc::FUTEX_PRIVATE_FLAG,
            ptr::null(),
            ptr::null(),
        );

        match result {
            Ok(0) | Ok(1) | Err(libc::EFAULT) => {}
            Ok(result) => panic!("unexpected futex wake result: {result}"),
            Err(error) => panic!("futex wake failed with error {error}"),
        }
    }
}

#[inline]
pub fn thread_yield() {
    thread::yield_now();
}
