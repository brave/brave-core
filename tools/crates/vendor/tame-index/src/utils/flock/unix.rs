#![allow(unsafe_code)]

use super::LockState;
use std::{fs::File, io::Error, os::unix::io::AsRawFd, time::Duration};

type Result = std::io::Result<()>;

macro_rules! flock_flag {
    ($state:expr) => {
        match $state {
            LockState::Shared => libc::LOCK_SH,
            LockState::Exclusive => libc::LOCK_EX,
            _ => unreachable!(),
        }
    };
}

macro_rules! error {
    ($func:expr) => {
        if $func != 0 {
            return Err(Error::last_os_error());
        }
    };
}

#[inline]
pub(super) fn open_opts(exclusive: bool) -> std::fs::OpenOptions {
    let mut o = std::fs::OpenOptions::new();
    o.read(true);

    if exclusive {
        o.write(true).create(true);
    }

    o
}

#[inline]
pub(super) fn try_lock(file: &File, state: LockState) -> Result {
    flock(file, flock_flag!(state) | libc::LOCK_NB)
}

#[inline]
pub(super) fn lock(file: &File, state: LockState, timeout: Option<Duration>) -> Result {
    if let Some(timeout) = timeout {
        static SIG_HANDLER: std::sync::Once = std::sync::Once::new();

        // Unfortunately due the global nature of signal handlers, we need to
        // register a signal handler that just ignores the signal we use to kill
        // the thread performing the lock if it is timed out, as otherwise the
        // default signal handler will terminate the process, not just interrupt
        // the one thread the signal was sent to.
        SIG_HANDLER.call_once(|| {
            unsafe {
                let mut act: libc::sigaction = std::mem::zeroed();
                if libc::sigemptyset(&mut act.sa_mask) != 0 {
                    eprintln!("unable to clear action mask: {:?}", Error::last_os_error());
                    return;
                }

                unsafe extern "C" fn on_sig(
                    _sig: libc::c_int,
                    _info: *mut libc::siginfo_t,
                    _uc: *mut libc::c_void,
                ) {
                }

                act.sa_flags = libc::SA_SIGINFO;
                act.sa_sigaction = on_sig as usize;

                // Register the action with the signal handler
                if libc::sigaction(libc::SIGUSR1, &act, std::ptr::null_mut()) != 0 {
                    eprintln!(
                        "unable to register signal handler: {:?}",
                        Error::last_os_error()
                    );
                }
            }
        });

        // We _could_ do this with a timer sending a SIGALRM to interupt the
        // syscall, but that involves global state, and is just generally not
        // nice, so we use a simpler approach of just spawning a thread and sending
        // a signal to it ourselves. Less efficient probably, but IMO cleaner
        let file_ptr = file as *const _ as usize;
        let mut thread_id: libc::pthread_t = unsafe { std::mem::zeroed() };
        let tid = &mut thread_id as *mut _ as usize;
        let (tx, rx) = std::sync::mpsc::channel();
        let lock_thread = std::thread::Builder::new()
            .name("flock wait".into())
            .spawn(move || unsafe {
                *(tid as *mut _) = libc::pthread_self();
                let res = flock(&*(file_ptr as *const _), flock_flag!(state));
                tx.send(res).unwrap();
            })?;

        match rx.recv_timeout(timeout) {
            Ok(res) => {
                let _ = lock_thread.join();
                res
            }
            Err(std::sync::mpsc::RecvTimeoutError::Timeout) => {
                // Send a signal to interrupt the lock syscall
                // Note that there is an edge case here, where the thread could be
                // finished and we just got unlucky by timing out at the exact
                // same moment. According to https://man7.org/linux/man-pages/man3/pthread_kill.3.html,
                // at least for glibc, pthread_kill _should_ set ESRCH if the thread
                // has already finished, but other implementations _might_ just
                // cause a SIGSEGV
                unsafe { libc::pthread_kill(thread_id, libc::SIGUSR1) };

                // Now that we've sent the signal, we can be fairly sure the thread
                // syscall will finish/has already finished so we block this time
                let res = rx.recv().unwrap();
                let _ = lock_thread.join();
                res
            }
            Err(_) => unreachable!(),
        }
    } else {
        flock(file, flock_flag!(state))
    }
}

#[inline]
pub(super) fn unlock(file: &File) -> Result {
    flock(file, libc::LOCK_UN)
}

#[inline]
fn flock(file: &File, flag: libc::c_int) -> Result {
    error!(unsafe { libc::flock(file.as_raw_fd(), flag) });
    Ok(())
}

#[inline]
pub(super) fn is_unsupported(err: &std::io::Error) -> bool {
    match err.raw_os_error() {
        // Unfortunately, depending on the target, these may or may not be the same.
        // For targets in which they are the same, the duplicate pattern causes a warning.
        #[allow(unreachable_patterns)]
        Some(libc::ENOTSUP | libc::EOPNOTSUPP | libc::ENOSYS) => true,
        _ => false,
    }
}

#[inline]
pub(super) fn is_contended(err: &Error) -> bool {
    err.raw_os_error() == Some(libc::EWOULDBLOCK)
}

#[inline]
pub(super) fn is_timed_out(err: &Error) -> bool {
    err.raw_os_error() == Some(libc::EINTR)
}
