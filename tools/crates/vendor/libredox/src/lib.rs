//! Redox-specific system library.
#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(feature = "call")]
use self::error::{Error, Result};

#[cfg(feature = "base")]
pub mod error {
    use super::*;

    #[derive(Clone, Copy, Eq, Hash, Ord, PartialEq, PartialOrd)]
    // TODO: Define as enum?
    pub struct Error {
        // TODO: NonZeroU16? "success" is not an error.
        errno: u16,
    }

    impl Error {
        pub fn new(errno: i32) -> Self {
            Self {
                errno: errno.try_into().unwrap_or(u16::MAX),
            }
        }
        pub fn errno(self) -> i32 {
            self.errno.into()
        }
        #[expect(
            unreachable_patterns,
            reason = "EAGAIN and EWOULDBLOCK share the same number"
        )]
        pub fn is_wouldblock(self) -> bool {
            matches!(self.errno(), errno::EAGAIN | errno::EWOULDBLOCK)
        }
        pub fn is_interrupt(self) -> bool {
            self.errno() == errno::EINTR
        }
        pub fn mux(res: Result<usize, Self>) -> usize {
            match res {
                // TODO: Ensure success cannot overlap with error values.
                Ok(success) => success,
                Err(error) => usize::wrapping_neg(usize::from(error.errno)),
            }
        }
        pub fn demux(res: usize) -> Result<usize, Self> {
            if res > usize::wrapping_neg(4096) {
                Err(Self {
                    errno: res.wrapping_neg().try_into().expect("2^BITS - res < 4096"),
                })
            } else {
                Ok(res)
            }
        }
        #[cfg(feature = "call")]
        pub fn description(self, buf: &mut [u8]) -> &str {
            call::strerror(self.errno, buf).map_or("unknown error", |(desc, _)| desc)
        }
    }

    impl core::fmt::Display for Error {
        fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
            #[cfg(feature = "call")]
            {
                write!(f, "{}", self.description(&mut [0; 256]))
            }
            #[cfg(not(feature = "call"))]
            {
                write!(f, "Error {} (strerror unavailable)", self.errno)
            }
        }
    }
    impl core::fmt::Debug for Error {
        fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
            #[cfg(feature = "call")]
            {
                write!(
                    f,
                    "Error `{}` {}",
                    self.description(&mut [0; 256]),
                    self.errno
                )
            }
            #[cfg(not(feature = "call"))]
            {
                write!(f, "Error {} (strerror unavailable)", self.errno)
            }
        }
    }
    #[cfg(feature = "redox_syscall")]
    impl From<syscall::Error> for Error {
        fn from(value: syscall::Error) -> Self {
            Self {
                errno: value.errno.try_into().unwrap_or(u16::MAX),
            }
        }
    }
    #[cfg(feature = "redox_syscall")]
    impl From<Error> for syscall::Error {
        fn from(value: Error) -> Self {
            Self::new(value.errno())
        }
    }
    #[cfg(feature = "std")]
    impl From<Error> for std::io::Error {
        fn from(value: Error) -> Self {
            Self::from_raw_os_error(value.errno.into())
        }
    }

    #[cfg(feature = "std")]
    impl std::error::Error for Error {}

    pub type Result<T, E = Error> = core::result::Result<T, E>;
}

#[cfg(feature = "base")]
pub mod flag {
    pub use libc::{
        O_ACCMODE, O_APPEND, O_ASYNC, O_CLOEXEC, O_CREAT, O_DIRECTORY, O_EXCL, O_FSYNC, O_NOFOLLOW,
        O_NONBLOCK, O_PATH, O_RDONLY, O_RDWR, O_TRUNC, O_WRONLY,
    };

    pub use libc::{MSG_DONTROUTE, MSG_DONTWAIT, MSG_EOR, MSG_OOB, MSG_PEEK};

    pub use libc::{CLOCK_MONOTONIC, CLOCK_REALTIME};

    pub use libc::{SIG_BLOCK, SIG_SETMASK, SIG_UNBLOCK};

    pub use libc::{
        SIGABRT,
        SIGALRM,
        SIGBUS,
        SIGCHLD,
        SIGCONT,
        SIGFPE,
        // TODO: Const rather than use, to convert to u32
        SIGHUP,
        SIGILL,
        SIGINT,
        SIGIO,
        SIGKILL,
        SIGPIPE,
        SIGPROF,
        SIGPWR,
        SIGQUIT,
        SIGSEGV,
        SIGSTKFLT,
        SIGSYS,
        SIGTERM,
        SIGTRAP,
        SIGTSTP,
        SIGTTIN,
        SIGTTOU,
        SIGURG,
        SIGUSR1,
        SIGUSR2,
        SIGVTALRM,
        SIGWINCH,
        SIGXFSZ,
    };

    pub use libc::{F_DUPFD, F_DUPFD_CLOEXEC, F_GETFD, F_SETFD};

    pub use libc::RENAME_NOREPLACE;

    #[cfg(target_os = "redox")]
    pub use libc::{O_EXLOCK, O_SHLOCK, O_SYMLINK};

    pub const MAP_SHARED: u32 = libc::MAP_SHARED as u32;
    pub const MAP_PRIVATE: u32 = libc::MAP_PRIVATE as u32;

    pub const PROT_NONE: u32 = libc::PROT_NONE as u32;
    pub const PROT_READ: u32 = libc::PROT_READ as u32;
    pub const PROT_WRITE: u32 = libc::PROT_WRITE as u32;
    pub const PROT_EXEC: u32 = libc::PROT_EXEC as u32;

    pub const WNOHANG: u32 = libc::WNOHANG as u32;
    pub const WUNTRACED: u32 = libc::WUNTRACED as u32;
    pub const WCONTINUED: u32 = libc::WCONTINUED as u32;
}

#[cfg(feature = "base")]
pub mod errno {
    pub use libc::{
        E2BIG, EACCES, EADDRINUSE, EADDRNOTAVAIL, EADV, EAFNOSUPPORT, EAGAIN, EALREADY, EBADE,
        EBADF, EBADFD, EBADMSG, EBADR, EBADRQC, EBADSLT, EBFONT, EBUSY, ECANCELED, ECHILD, ECHRNG,
        ECOMM, ECONNABORTED, ECONNREFUSED, ECONNRESET, EDEADLK, EDEADLOCK, EDESTADDRREQ, EDOM,
        EDOTDOT, EDQUOT, EEXIST, EFAULT, EFBIG, EHOSTDOWN, EHOSTUNREACH, EIDRM, EILSEQ,
        EINPROGRESS, EINTR, EINVAL, EIO, EISCONN, EISDIR, EISNAM, EKEYEXPIRED, EKEYREJECTED,
        EKEYREVOKED, EL2HLT, EL2NSYNC, EL3HLT, EL3RST, ELIBACC, ELIBBAD, ELIBEXEC, ELIBMAX,
        ELIBSCN, ELNRNG, ELOOP, EMEDIUMTYPE, EMFILE, EMLINK, EMSGSIZE, EMULTIHOP, ENAMETOOLONG,
        ENAVAIL, ENETDOWN, ENETRESET, ENETUNREACH, ENFILE, ENOANO, ENOBUFS, ENOCSI, ENODEV, ENOENT,
        ENOEXEC, ENOKEY, ENOLCK, ENOMEDIUM, ENOMEM, ENOMSG, ENONET, ENOPKG, ENOPROTOOPT, ENOSPC,
        ENOSR, ENOSTR, ENOSYS, ENOTBLK, ENOTCONN, ENOTDIR, ENOTEMPTY, ENOTNAM, ENOTRECOVERABLE,
        ENOTSOCK, ENOTTY, ENOTUNIQ, ENXIO, EOPNOTSUPP, EOVERFLOW, EOWNERDEAD, EPERM, EPFNOSUPPORT,
        EPIPE, EPROTO, EPROTONOSUPPORT, EPROTOTYPE, ERANGE, EREMCHG, EREMOTE, EREMOTEIO, ERESTART,
        EROFS, ESHUTDOWN, ESOCKTNOSUPPORT, ESPIPE, ESRCH, ESRMNT, ESTALE, ESTRPIPE, ETIME,
        ETIMEDOUT, ETOOMANYREFS, ETXTBSY, EUCLEAN, EUNATCH, EUSERS, EWOULDBLOCK, EXDEV, EXFULL,
    };
}
#[cfg(feature = "base")]
pub mod data {
    pub use libc::iovec as IoVec;
    pub use libc::sigaction as SigAction;
    pub use libc::stat as Stat;
    pub use libc::statvfs as StatVfs;
    pub use libc::timespec as TimeSpec;

    // TODO: Remove
    pub fn timespec_from_mut_bytes(bytes: &mut [u8]) -> &mut TimeSpec {
        assert!(bytes.len() >= core::mem::size_of::<TimeSpec>());
        assert_eq!(
            bytes.as_ptr() as usize % core::mem::align_of::<TimeSpec>(),
            0
        );
        unsafe { &mut *bytes.as_mut_ptr().cast() }
    }
    // TODO: Remove
    pub fn timespec_from_bytes(bytes: &[u8]) -> &TimeSpec {
        assert!(bytes.len() >= core::mem::size_of::<TimeSpec>());
        assert_eq!(
            bytes.as_ptr() as usize % core::mem::align_of::<TimeSpec>(),
            0
        );
        unsafe { &*bytes.as_ptr().cast() }
    }

    #[cfg(target_os = "redox")]
    pub use libc::sigset_t as SigSet;

    // TODO: Should libredox compile on non-Redox platforms?
    #[cfg(not(target_os = "redox"))]
    pub type SigSet = u64;
}

#[cfg(feature = "call")]
type RawResult = usize;

#[cfg(feature = "call")]
#[allow(dead_code)]
extern "C" {
    // NOTE: Although there are version suffixes, there'd have to be strong reasons for adding new
    // version.
    fn redox_open_v1(path_base: *const u8, path_len: usize, flags: u32, mode: u16) -> RawResult;
    fn redox_openat_v1(
        fd: usize,
        buf: *const u8,
        path_len: usize,
        flags: u32,
        fcntl_flags: u32,
    ) -> RawResult;
    fn redox_dup_v1(fd: usize, buf: *const u8, len: usize) -> RawResult;
    fn redox_dup2_v1(old_fd: usize, new_fd: usize, buf: *const u8, len: usize) -> RawResult;
    fn redox_read_v1(fd: usize, dst_base: *mut u8, dst_len: usize) -> RawResult;
    fn redox_write_v1(fd: usize, src_base: *const u8, src_len: usize) -> RawResult;
    fn redox_fchmod_v1(fd: usize, new_mode: u16) -> RawResult;
    fn redox_fchown_v1(fd: usize, new_uid: u32, new_gid: u32) -> RawResult;
    fn redox_getdents_v0(fd: usize, buf: *mut u8, buf_len: usize, opaque: u64) -> RawResult;
    fn redox_fstat_v1(fd: usize, dst: *mut data::Stat) -> RawResult;
    fn redox_fstatvfs_v1(fd: usize, dst: *mut data::StatVfs) -> RawResult;
    fn redox_fsync_v1(fd: usize) -> RawResult;
    fn redox_fdatasync_v1(fd: usize) -> RawResult;
    fn redox_ftruncate_v0(fd: usize, len: usize) -> RawResult;
    fn redox_futimens_v1(fd: usize, times: *const data::TimeSpec) -> RawResult;
    /* TODO: Support unlinkat using std_fs_call
    fn redox_unlinkat_v0(fd: usize, buf: *const u8, path_len: usize, flags: u32) -> RawResult;
    */
    fn redox_fpath_v1(fd: usize, dst_base: *mut u8, dst_len: usize) -> RawResult;
    fn redox_relpathat_v0(dirfd: usize, fd: usize, dst_base: *mut u8, dst_len: usize) -> RawResult;
    fn redox_close_v1(fd: usize) -> RawResult;

    // NOTE: While the Redox kernel currently doesn't distinguish between threads and processes,
    // the return value of this function is expected to be treated as a process ID and not a thread
    // ID.
    fn redox_get_pid_v1() -> RawResult;

    fn redox_get_euid_v1() -> RawResult;
    fn redox_get_ruid_v1() -> RawResult;
    fn redox_get_egid_v1() -> RawResult;
    fn redox_get_rgid_v1() -> RawResult;

    fn redox_get_ens_v0() -> RawResult;
    fn redox_get_ns_v0() -> RawResult;
    // This function is used to get the credentials, pid, euid, egid etc. of the process with the target pid.
    fn redox_get_proc_credentials_v1(cap_fd: usize, target_pid: usize, buf: &mut [u8])
        -> RawResult;

    fn redox_setrens_v1(rns: usize, ens: usize) -> RawResult;
    fn redox_mkns_v1(names: *const data::IoVec, num_names: usize, _flags: u32) -> RawResult;

    fn redox_kill_v1(pid: usize, signal: u32) -> RawResult;
    fn redox_waitpid_v1(pid: usize, status: *mut i32, options: u32) -> RawResult;

    fn redox_sigprocmask_v1(how: u32, new: *const u64, old: *mut u64) -> RawResult;
    fn redox_sigaction_v1(
        signal: u32,
        new: *const data::SigAction,
        old: *mut data::SigAction,
    ) -> RawResult;

    fn redox_clock_gettime_v1(clock: usize, ts: *mut data::TimeSpec) -> RawResult;

    fn redox_mmap_v1(
        addr: *mut (),
        unaligned_len: usize,
        prot: u32,
        flags: u32,
        fd: usize,
        offset: u64,
    ) -> RawResult;
    fn redox_munmap_v1(addr: *mut (), unaligned_len: usize) -> RawResult;

    fn redox_strerror_v1(dst: *mut u8, dst_len: *mut usize, error: u32) -> RawResult;

    fn redox_sys_call_v0(
        fd: usize,
        payload: *mut u8,
        payload_len: usize,
        flags: usize,
        metadata: *const u64,
        metadata_len: usize,
    ) -> RawResult;
    fn redox_sys_call_multiple_v0(
        fds: *const usize,
        fds_len: usize,
        payload: *mut u8,
        payload_len: usize,
        flags: usize,
        metadata: *const u64,
        metadata_len: usize,
    ) -> RawResult;

    fn redox_get_socket_token_v0(fd: usize, payload: *mut u8, payload_len: usize) -> RawResult;

    fn redox_setns_v0(fd: usize) -> RawResult;

    fn redox_register_scheme_to_ns_v0(
        ns_fd: usize,
        name_base: *const u8,
        name_len: usize,
        cap_fd: usize,
    ) -> RawResult;

    fn redox_fcntl_v0(fd: usize, cmd: usize, arg: usize) -> RawResult;

}

#[cfg(feature = "call")]
pub struct Fd(usize);

#[cfg(feature = "call")]
impl Fd {
    pub fn new(raw: usize) -> Self {
        Self(raw)
    }
    #[inline]
    pub fn open(path: &str, flags: i32, mode: u16) -> Result<Self> {
        Ok(Self(call::open(path, flags, mode)?))
    }
    #[inline]
    pub fn openat(&self, path: &str, flags: i32, fcntl_flags: u32) -> Result<Self> {
        Ok(Self(call::openat(self.raw(), path, flags, fcntl_flags)?))
    }
    #[inline]
    pub fn dup(&self, buf: &[u8]) -> Result<usize> {
        call::dup(self.raw(), buf)
    }
    #[inline]
    pub fn dup2(&self, new_fd: usize, buf: &[u8]) -> Result<usize> {
        call::dup2(self.raw(), new_fd, buf)
    }

    #[inline]
    pub const fn raw(&self) -> usize {
        self.0
    }

    #[inline]
    pub fn into_raw(self) -> usize {
        let raw = self.raw();
        core::mem::forget(self);
        raw
    }

    #[inline]
    pub fn read(&self, buf: &mut [u8]) -> Result<usize> {
        call::read(self.raw(), buf)
    }
    #[inline]
    pub fn write(&self, buf: &[u8]) -> Result<usize> {
        call::write(self.raw(), buf)
    }
    #[inline]
    pub fn chmod(&self, new_mode: u16) -> Result<()> {
        call::fchmod(self.raw(), new_mode)
    }
    #[inline]
    pub fn chown(&self, new_uid: u32, new_gid: u32) -> Result<()> {
        call::fchown(self.raw(), new_uid, new_gid)
    }
    #[inline]
    pub fn getdents(self, buf: &mut [u8], opaque: u64) -> Result<usize> {
        call::getdents(self.raw(), buf, opaque)
    }
    pub fn stat(&self) -> Result<data::Stat> {
        call::fstat(self.raw())
    }
    pub fn statvfs(&self) -> Result<data::StatVfs> {
        call::fstatvfs(self.raw())
    }
    #[inline]
    pub fn fsync(&self) -> Result<()> {
        call::fsync(self.raw())
    }
    #[inline]
    pub fn fdatasync(&self) -> Result<()> {
        call::fdatasync(self.raw())
    }
    #[inline]
    pub fn ftruncate(self, len: usize) -> Result<()> {
        call::ftruncate(self.raw(), len)
    }
    #[inline]
    pub fn futimens(self, times: &[data::TimeSpec; 2]) -> Result<()> {
        call::futimens(self.raw(), times)
    }
    /* TODO: Support unlinkat using std_fs_call
    #[inline]
    pub fn unlinkat(&self, path: &str, flags: i32) -> Result<()> {
        call::unlinkat(self.raw(), path, flags)
    }
    */
    #[inline]
    pub fn fpath(&self, path: &mut [u8]) -> Result<usize> {
        call::fpath(self.raw(), path)
    }
    #[inline]
    pub fn relpathat(&self, fd: usize, path: &mut [u8]) -> Result<usize> {
        call::relpathat(self.raw(), fd, path)
    }
    #[inline]
    pub fn close(self) -> Result<()> {
        call::close(self.into_raw())
    }

    #[cfg(feature = "redox_syscall")]
    #[inline]
    pub fn call_ro(
        &self,
        payload: &mut [u8],
        flags: syscall::CallFlags,
        metadata: &[u64],
    ) -> Result<usize> {
        call::call_ro(self.raw(), payload, flags, metadata)
    }
    #[cfg(feature = "redox_syscall")]
    #[inline]
    pub fn call_wo(
        &self,
        payload: &[u8],
        flags: syscall::CallFlags,
        metadata: &[u64],
    ) -> Result<usize> {
        call::call_wo(self.raw(), payload, flags, metadata)
    }
    #[cfg(feature = "redox_syscall")]
    #[inline]
    pub fn call_rw(
        &self,
        payload: &mut [u8],
        flags: syscall::CallFlags,
        metadata: &[u64],
    ) -> Result<usize> {
        call::call_rw(self.raw(), payload, flags, metadata)
    }

    #[inline]
    pub fn fcntl(&self, cmd: usize, arg: usize) -> Result<usize> {
        call::fcntl(self.raw(), cmd, arg)
    }
}
#[cfg(feature = "call")]
impl Drop for Fd {
    fn drop(&mut self) {
        let _ = unsafe { redox_close_v1(self.0) };
    }
}

#[cfg(feature = "call")]
pub mod call {
    use core::mem::MaybeUninit;

    use super::*;

    #[cfg(feature = "redox_syscall")]
    pub trait Call {
        unsafe fn raw_call(
            &self,
            payload: *mut u8,
            payload_len: usize,
            flags: syscall::CallFlags,
            metadata: &[u64],
        ) -> Result<usize>;
    }

    #[cfg(feature = "redox_syscall")]
    impl Call for usize {
        unsafe fn raw_call(
            &self,
            payload: *mut u8,
            payload_len: usize,
            flags: syscall::CallFlags,
            metadata: &[u64],
        ) -> Result<usize> {
            Ok(Error::demux(unsafe {
                redox_sys_call_v0(
                    *self,
                    payload,
                    payload_len,
                    flags.bits(),
                    metadata.as_ptr(),
                    metadata.len(),
                )
            })?)
        }
    }

    #[cfg(feature = "redox_syscall")]
    impl Call for &[usize] {
        /// # Errors
        ///
        /// * `EXDEV` - The file descriptors belong to different schemes.
        unsafe fn raw_call(
            &self,
            payload: *mut u8,
            payload_len: usize,
            flags: syscall::CallFlags,
            metadata: &[u64],
        ) -> Result<usize> {
            Error::demux(unsafe {
                redox_sys_call_multiple_v0(
                    self.as_ptr(),
                    self.len(),
                    payload,
                    payload_len,
                    flags.bits(),
                    metadata.as_ptr(),
                    metadata.len(),
                )
            })
        }
    }

    /// flags and mode are binary compatible with libc
    #[inline]
    pub fn open(path: impl AsRef<str>, flags: i32, mode: u16) -> Result<usize> {
        let path = path.as_ref();
        Ok(Error::demux(unsafe {
            redox_open_v1(path.as_ptr(), path.len(), flags as u32, mode)
        })?)
    }
    #[inline]
    pub fn openat(
        fd: usize,
        path: impl AsRef<[u8]>,
        flags: i32,
        fcntl_flags: u32,
    ) -> Result<usize> {
        let path = path.as_ref();
        Ok(Error::demux(unsafe {
            redox_openat_v1(fd, path.as_ptr(), path.len(), flags as u32, fcntl_flags)
        })?)
    }
    #[inline]
    pub fn dup(fd: usize, buf: impl AsRef<[u8]>) -> Result<usize> {
        let buf = buf.as_ref();
        Ok(Error::demux(unsafe {
            redox_dup_v1(fd, buf.as_ptr(), buf.len())
        })?)
    }
    #[inline]
    pub fn dup2(old_fd: usize, new_fd: usize, buf: impl AsRef<[u8]>) -> Result<usize> {
        let buf = buf.as_ref();
        Ok(Error::demux(unsafe {
            redox_dup2_v1(old_fd, new_fd, buf.as_ptr(), buf.len())
        })?)
    }
    #[inline]
    pub fn read(raw_fd: usize, buf: &mut [u8]) -> Result<usize> {
        Ok(Error::demux(unsafe {
            redox_read_v1(raw_fd, buf.as_mut_ptr(), buf.len())
        })?)
    }
    #[inline]
    pub fn write(raw_fd: usize, buf: &[u8]) -> Result<usize> {
        Error::demux(unsafe { redox_write_v1(raw_fd, buf.as_ptr(), buf.len()) })
    }
    #[inline]
    pub fn fchmod(raw_fd: usize, new_mode: u16) -> Result<()> {
        Error::demux(unsafe { redox_fchmod_v1(raw_fd, new_mode) })?;
        Ok(())
    }
    #[inline]
    pub fn fchown(raw_fd: usize, new_uid: u32, new_gid: u32) -> Result<()> {
        Error::demux(unsafe { redox_fchown_v1(raw_fd, new_uid, new_gid) })?;
        Ok(())
    }
    #[inline]
    pub fn getdents(fd: usize, buf: &mut [u8], opaque: u64) -> Result<usize> {
        Error::demux(unsafe { redox_getdents_v0(fd, buf.as_mut_ptr(), buf.len(), opaque) })
    }
    #[inline]
    pub fn fstat(raw_fd: usize) -> Result<data::Stat> {
        unsafe {
            let mut ret = MaybeUninit::uninit();
            Error::demux(redox_fstat_v1(raw_fd, ret.as_mut_ptr()))?;
            Ok(ret.assume_init())
        }
    }
    #[inline]
    pub fn fstatvfs(raw_fd: usize) -> Result<data::StatVfs> {
        unsafe {
            let mut ret = MaybeUninit::uninit();
            Error::demux(redox_fstatvfs_v1(raw_fd, ret.as_mut_ptr()))?;
            Ok(ret.assume_init())
        }
    }
    #[inline]
    pub fn fsync(raw_fd: usize) -> Result<()> {
        Error::demux(unsafe { redox_fsync_v1(raw_fd) }).map(|_| ())
    }
    #[inline]
    pub fn fdatasync(raw_fd: usize) -> Result<()> {
        Error::demux(unsafe { redox_fdatasync_v1(raw_fd) }).map(|_| ())
    }
    #[inline]
    pub fn ftruncate(raw_fd: usize, new_size: usize) -> Result<()> {
        Error::demux(unsafe { redox_ftruncate_v0(raw_fd, new_size) }).map(|_| ())
    }
    #[inline]
    pub fn futimens(raw_fd: usize, times: &[data::TimeSpec; 2]) -> Result<()> {
        Error::demux(unsafe { redox_futimens_v1(raw_fd, times.as_ptr()) })?;
        Ok(())
    }
    /* TODO: Support unlinkat using std_fs_call
    #[inline]
    pub fn unlinkat(fd: usize, path: impl AsRef<[u8]>, flags: i32) -> Result<()> {
        let path = path.as_ref();
        Error::demux(unsafe { redox_unlinkat_v0(fd, path.as_ptr(), path.len(), flags as u32) })
            .map(|_| ())
    }
    */
    #[inline]
    pub fn fpath(raw_fd: usize, buf: &mut [u8]) -> Result<usize> {
        Error::demux(unsafe { redox_fpath_v1(raw_fd, buf.as_mut_ptr(), buf.len()) })
    }
    #[inline]
    pub fn relpathat(raw_fd: usize, fd: usize, buf: &mut [u8]) -> Result<usize> {
        Error::demux(unsafe { redox_relpathat_v0(raw_fd, fd, buf.as_mut_ptr(), buf.len()) })
    }
    #[inline]
    pub fn close(raw_fd: usize) -> Result<()> {
        Error::demux(unsafe { redox_close_v1(raw_fd) })?;
        Ok(())
    }

    #[cfg(feature = "redox_syscall")]
    #[inline]
    pub fn call_ro<T: Call>(
        fd: T,
        payload: &mut [u8],
        flags: syscall::CallFlags,
        metadata: &[u64],
    ) -> Result<usize> {
        if flags.contains(syscall::CallFlags::WRITE) {
            return Err(Error::new(syscall::EINVAL));
        }
        unsafe {
            fd.raw_call(
                payload.as_mut_ptr(),
                payload.len(),
                flags | syscall::CallFlags::READ,
                metadata,
            )
        }
    }
    #[cfg(feature = "redox_syscall")]
    #[inline]
    pub fn call_wo<T: Call>(
        fd: T,
        payload: &[u8],
        flags: syscall::CallFlags,
        metadata: &[u64],
    ) -> Result<usize> {
        if flags.contains(syscall::CallFlags::READ) {
            return Err(Error::new(syscall::EINVAL));
        }
        unsafe {
            fd.raw_call(
                payload.as_ptr().cast_mut(),
                payload.len(),
                flags | syscall::CallFlags::WRITE,
                metadata,
            )
        }
    }
    #[cfg(feature = "redox_syscall")]
    #[inline]
    pub fn call_rw<T: Call>(
        fd: T,
        payload: &mut [u8],
        flags: syscall::CallFlags,
        metadata: &[u64],
    ) -> Result<usize> {
        unsafe {
            fd.raw_call(
                payload.as_mut_ptr(),
                payload.len(),
                flags | syscall::CallFlags::READ | syscall::CallFlags::WRITE,
                metadata,
            )
        }
    }

    #[inline]
    pub fn geteuid() -> Result<usize> {
        Error::demux(unsafe { redox_get_euid_v1() })
    }

    #[inline]
    pub fn getruid() -> Result<usize> {
        Error::demux(unsafe { redox_get_ruid_v1() })
    }

    #[inline]
    pub fn getegid() -> Result<usize> {
        Error::demux(unsafe { redox_get_egid_v1() })
    }
    #[inline]
    pub fn getrgid() -> Result<usize> {
        Error::demux(unsafe { redox_get_rgid_v1() })
    }
    #[inline]
    pub fn getpid() -> Result<usize> {
        Error::demux(unsafe { redox_get_pid_v1() })
    }
    #[inline]
    pub fn getens() -> Result<usize> {
        Error::demux(unsafe { redox_get_ens_v0() })
    }

    #[inline]
    // [u8; size_of::<crate::protocol::ProcMeta>()]
    pub fn get_proc_credentials(cap_fd: usize, target_pid: usize, buf: &mut [u8]) -> Result<usize> {
        Error::demux(unsafe { redox_get_proc_credentials_v1(cap_fd, target_pid, buf) })
    }

    #[inline]
    pub fn setrens(rns: usize, ens: usize) -> Result<usize> {
        Error::demux(unsafe { redox_setrens_v1(rns, ens) })
    }
    #[inline]
    pub fn waitpid(pid: usize, status: &mut i32, options: i32) -> Result<usize> {
        Error::demux(unsafe { redox_waitpid_v1(pid, status as *mut i32, options as u32) })
    }
    #[inline]
    pub fn kill(pid: usize, signal: u32) -> Result<()> {
        Error::demux(unsafe { redox_kill_v1(pid, signal) }).map(|_| ())
    }
    #[inline]
    pub fn clock_gettime(clock: i32) -> Result<data::TimeSpec> {
        unsafe {
            let mut ret = MaybeUninit::uninit();
            Error::demux(redox_clock_gettime_v1(clock as usize, ret.as_mut_ptr()))?;
            Ok(ret.assume_init())
        }
    }
    #[inline]
    pub fn sigprocmask(
        how: i32,
        newmask: Option<&data::SigSet>,
        oldmask: Option<&mut data::SigSet>,
    ) -> Result<()> {
        Error::demux(unsafe {
            redox_sigprocmask_v1(
                how as u32,
                newmask.map_or(core::ptr::null(), |m| m),
                oldmask.map_or(core::ptr::null_mut(), |m| m),
            )
        })
        .map(|_| ())
    }
    #[inline]
    pub fn sigaction(
        signal: i32,
        newact: Option<&data::SigAction>,
        oldact: Option<&mut data::SigAction>,
    ) -> Result<()> {
        Error::demux(unsafe {
            redox_sigaction_v1(
                signal as u32,
                newact.map_or(core::ptr::null(), |m| m),
                oldact.map_or(core::ptr::null_mut(), |m| m),
            )
        })
        .map(|_| ())
    }

    #[derive(Clone, Copy, Debug)]
    pub struct MmapArgs {
        pub addr: *mut (),
        pub length: usize,
        pub prot: u32,
        pub flags: u32,
        pub fd: usize,
        pub offset: u64,
    }
    #[inline]
    pub unsafe fn mmap(args: MmapArgs) -> Result<*mut ()> {
        Error::demux(redox_mmap_v1(
            args.addr,
            args.length,
            args.prot,
            args.flags,
            args.fd,
            args.offset,
        ))
        .map(|addr| addr as *mut ())
    }
    #[inline]
    pub unsafe fn munmap(addr: *mut (), length: usize) -> Result<()> {
        Error::demux(redox_munmap_v1(addr, length)).map(|_| ())
    }

    #[inline]
    pub fn strerror(error: u16, desc: &mut [u8]) -> Option<(&str, usize)> {
        unsafe {
            let mut len_inout = desc.len();
            let copied_len = Error::demux(redox_strerror_v1(
                desc.as_mut_ptr(),
                &mut len_inout,
                error.into(),
            ))
            .ok()?;
            Some((
                core::str::from_utf8_unchecked(&desc[..copied_len]),
                len_inout,
            ))
        }
    }

    #[inline]
    #[cfg(feature = "mkns")]
    pub fn mkns(names: &[ioslice::IoSlice]) -> Result<usize> {
        // no-op
        let iovecs = ioslice::IoSlice::cast_to_raw_iovecs(names);

        unsafe { Error::demux(redox_mkns_v1(iovecs.as_ptr(), iovecs.len(), 0)) }
    }

    #[inline]
    pub fn get_socket_token(fd: usize, buf: &mut [u8]) -> Result<usize> {
        Error::demux(unsafe { redox_get_socket_token_v0(fd, buf.as_mut_ptr(), buf.len()) })
    }

    #[inline]
    pub fn setns(fd: usize) -> Result<usize> {
        Error::demux(unsafe { redox_setns_v0(fd) })
    }
    #[inline]
    pub fn getns() -> Result<usize> {
        Error::demux(unsafe { redox_get_ns_v0() })
    }

    #[inline]
    pub fn register_scheme_to_ns(ns_fd: usize, name: impl AsRef<str>, cap_fd: usize) -> Result<()> {
        let name = name.as_ref();
        Error::demux(unsafe {
            redox_register_scheme_to_ns_v0(ns_fd, name.as_ptr(), name.len(), cap_fd)
        })
        .map(|_| ())
    }

    #[inline]
    pub fn fcntl(fd: usize, cmd: usize, arg: usize) -> Result<usize> {
        Error::demux(unsafe { redox_fcntl_v0(fd, cmd, arg) })
    }
}

#[cfg(feature = "protocol")]
pub mod protocol {
    use bitflags::bitflags;

    #[derive(Clone, Copy, Debug, Default)]
    #[repr(C)]
    pub struct ProcMeta {
        pub pid: u32,
        pub pgid: u32,
        pub ppid: u32,
        pub ruid: u32,
        pub euid: u32,
        pub suid: u32,
        pub rgid: u32,
        pub egid: u32,
        pub sgid: u32,
        pub ens: u32,
        pub rns: u32,
    }
    unsafe impl plain::Plain for ProcMeta {}

    #[derive(Clone, Copy, Debug, Eq, PartialEq)]
    #[repr(usize)]
    pub enum ProcCall {
        Waitpid = 0,
        Setrens = 1,
        Exit = 2,
        Waitpgid = 3,
        SetResugid = 4,
        Setpgid = 5,
        Getsid = 6,
        Setsid = 7,
        Kill = 8,
        Sigq = 9,

        // TODO: replace with sendfd equivalent syscall for sending memory
        SyncSigPctl = 10,
        Sigdeq = 11,
        Getppid = 12,
        Rename = 13,
        DisableSetpgid = 14,

        // Temporary calls for getting process credentials
        GetProcCredentials = 15,

        SetProcPriority = 16,
        GetProcPriority = 17,

        ControlTerm = 18,
        Rlimit = 19,
    }

    const RLIM_INFINITY: u64 = !0;

    #[derive(Clone, Copy, Debug)]
    #[repr(C)]
    pub struct Rlimit {
        pub rlim_cur: u64,
        pub rlim_max: u64,
    }
    impl Rlimit {
        pub const fn const_default() -> Self {
            Self {
                rlim_cur: RLIM_INFINITY,
                rlim_max: RLIM_INFINITY,
            }
        }
    }
    unsafe impl plain::Plain for Rlimit {}

    #[derive(Clone, Copy, Debug, Eq, PartialEq)]
    #[repr(usize)]
    pub enum PidfdCall {
        SendSignal = 0,
    }
    #[derive(Clone, Copy, Debug, Eq, PartialEq)]
    #[repr(usize)]
    pub enum ThreadCall {
        // TODO: replace with sendfd equivalent syscall for sending memory, or force userspace to
        // obtain its TCB memory from this server
        SyncSigTctl = 0,
        SignalThread = 1,
    }

    #[derive(Clone, Copy, Debug, Eq, PartialEq)]
    #[repr(usize)]
    #[non_exhaustive]
    pub enum SocketCall {
        Bind = 0,
        Connect = 1,
        SetSockOpt = 2,
        GetSockOpt = 3,
        SendMsg = 4,
        RecvMsg = 5,
        Unbind = 6,
        GetToken = 7,
        GetPeerName = 8,
        Shutdown = 9,
    }

    #[derive(Clone, Copy, Debug, Eq, PartialEq)]
    #[repr(usize)]
    #[non_exhaustive]
    pub enum FsCall {
        Connect = 0,
    }

    #[derive(Clone, Copy, Debug, Eq, PartialEq)]
    #[repr(usize)]
    pub enum TtyCall {
        Termios = 0,
        Flush = 1,
        SendBreak = 2,
        Flow = 3,
        PtsName = 4,
        PtLock = 5,
        Pgrp = 6,
        Winsize = 7,
    }

    impl ProcCall {
        pub fn try_from_raw(raw: usize) -> Option<Self> {
            Some(match raw {
                0 => Self::Waitpid,
                1 => Self::Setrens,
                2 => Self::Exit,
                3 => Self::Waitpgid,
                4 => Self::SetResugid,
                5 => Self::Setpgid,
                6 => Self::Getsid,
                7 => Self::Setsid,
                8 => Self::Kill,
                9 => Self::Sigq,
                10 => Self::SyncSigPctl,
                11 => Self::Sigdeq,
                12 => Self::Getppid,
                13 => Self::Rename,
                14 => Self::DisableSetpgid,
                15 => Self::GetProcCredentials,
                16 => Self::SetProcPriority,
                17 => Self::GetProcPriority,
                18 => Self::ControlTerm,
                19 => Self::Rlimit,
                _ => return None,
            })
        }
    }
    impl PidfdCall {
        pub fn try_from_raw(raw: usize) -> Option<Self> {
            Some(match raw {
                0 => Self::SendSignal,
                _ => return None,
            })
        }
    }
    impl ThreadCall {
        pub fn try_from_raw(raw: usize) -> Option<Self> {
            Some(match raw {
                0 => Self::SyncSigTctl,
                1 => Self::SignalThread,
                _ => return None,
            })
        }
    }

    impl SocketCall {
        pub fn try_from_raw(raw: usize) -> Option<Self> {
            Some(match raw {
                0 => Self::Bind,
                1 => Self::Connect,
                2 => Self::SetSockOpt,
                3 => Self::GetSockOpt,
                4 => Self::SendMsg,
                5 => Self::RecvMsg,
                6 => Self::Unbind,
                7 => Self::GetToken,
                8 => Self::GetPeerName,
                9 => Self::Shutdown,
                _ => return None,
            })
        }
    }

    impl FsCall {
        pub fn try_from_raw(raw: usize) -> Option<Self> {
            Some(match raw {
                0 => Self::Connect,
                _ => return None,
            })
        }
    }

    impl TtyCall {
        pub fn try_from_raw(raw: usize) -> Option<Self> {
            Some(match raw {
                0 => Self::Termios,
                1 => Self::Flush,
                2 => Self::SendBreak,
                3 => Self::Flow,
                4 => Self::PtsName,
                5 => Self::PtLock,
                6 => Self::Pgrp,
                7 => Self::Winsize,
                _ => return None,
            })
        }
    }

    bitflags! {
        #[derive(Clone, Copy, Debug, Default, Eq, Ord, Hash, PartialEq, PartialOrd)]
        pub struct WaitFlags: usize {
            const WNOHANG =    0x01;
            const WUNTRACED =  0x02;
            const WCONTINUED = 0x08;
        }
    }
    /// True if status indicates the child is stopped.
    pub fn wifstopped(status: usize) -> bool {
        (status & 0xff) == 0x7f
    }

    /// If wifstopped(status), the signal that stopped the child.
    pub fn wstopsig(status: usize) -> usize {
        (status >> 8) & 0xff
    }

    /// True if status indicates the child continued after a stop.
    pub fn wifcontinued(status: usize) -> bool {
        status == 0xffff
    }

    /// True if STATUS indicates termination by a signal.
    pub fn wifsignaled(status: usize) -> bool {
        ((status & 0x7f) + 1) as i8 >= 2
    }

    /// If wifsignaled(status), the terminating signal.
    pub fn wtermsig(status: usize) -> usize {
        status & 0x7f
    }

    /// True if status indicates normal termination.
    pub fn wifexited(status: usize) -> bool {
        wtermsig(status) == 0
    }

    /// If wifexited(status), the exit status.
    pub fn wexitstatus(status: usize) -> usize {
        (status >> 8) & 0xff
    }

    /// True if status indicates a core dump was created.
    pub fn wcoredump(status: usize) -> bool {
        (status & 0x80) != 0
    }
    #[derive(Clone, Copy, Debug)]
    pub enum ProcKillTarget {
        ThisGroup,
        SingleProc(usize),
        ProcGroup(usize),
        All,
    }
    impl ProcKillTarget {
        pub fn raw(self) -> usize {
            match self {
                Self::ThisGroup => 0,
                Self::SingleProc(p) => p,
                Self::ProcGroup(g) => usize::wrapping_neg(g),
                Self::All => usize::wrapping_neg(1),
            }
        }
        pub fn from_raw(raw: usize) -> Self {
            let raw = raw as isize;
            if raw == 0 {
                Self::ThisGroup
            } else if raw == -1 {
                Self::All
            } else if raw < 0 {
                Self::ProcGroup(raw.wrapping_neg() as usize)
            } else {
                Self::SingleProc(raw as usize)
            }
        }
    }
    #[derive(Copy, Clone, Debug, Default, PartialEq)]
    #[repr(C)]
    pub struct RtSigInfo {
        pub arg: usize,
        pub code: i32,
        pub uid: u32,
        pub pid: u32, // TODO: usize?
    }
    unsafe impl plain::Plain for RtSigInfo {}

    pub const SIGHUP: usize = 1;
    pub const SIGINT: usize = 2;
    pub const SIGQUIT: usize = 3;
    pub const SIGILL: usize = 4;
    pub const SIGTRAP: usize = 5;
    pub const SIGABRT: usize = 6;
    pub const SIGBUS: usize = 7;
    pub const SIGFPE: usize = 8;
    pub const SIGKILL: usize = 9;
    pub const SIGUSR1: usize = 10;
    pub const SIGSEGV: usize = 11;
    pub const SIGUSR2: usize = 12;
    pub const SIGPIPE: usize = 13;
    pub const SIGALRM: usize = 14;
    pub const SIGTERM: usize = 15;
    pub const SIGSTKFLT: usize = 16;
    pub const SIGCHLD: usize = syscall::SIGCHLD;
    pub const SIGCONT: usize = 18;
    pub const SIGSTOP: usize = 19;
    pub const SIGTSTP: usize = syscall::SIGTSTP;
    pub const SIGTTIN: usize = syscall::SIGTTIN;
    pub const SIGTTOU: usize = syscall::SIGTTOU;
    pub const SIGURG: usize = 23;
    pub const SIGXCPU: usize = 24;
    pub const SIGXFSZ: usize = 25;
    pub const SIGVTALRM: usize = 26;
    pub const SIGPROF: usize = 27;
    pub const SIGWINCH: usize = 28;
    pub const SIGIO: usize = 29;
    pub const SIGPWR: usize = 30;
    pub const SIGSYS: usize = 31;

    bitflags! {
        #[derive(Clone, Copy, Debug, Default, Eq, Ord, Hash, PartialEq, PartialOrd)]
        pub struct NsPermissions: usize {
            /// List schemes in the namespace
            const LIST = 1 << 0;
            /// Register a new scheme in the namespace
            const INSERT = 1 << 1;
            /// Delete a scheme from the namespace
            const DELETE = 1 << 2;
            /// Get scheme creation capabilities of the namespace
            const SCHEME_CREATE = 1 << 3;
        }
    }

    #[derive(Clone, Copy, Debug, Eq, PartialEq)]
    #[repr(usize)]
    pub enum NsDup {
        ForkNs = 0,
        ShrinkPermissions = 1,
        IssueRegister = 2,
    }
    impl NsDup {
        pub fn try_from_raw(raw: usize) -> Option<Self> {
            Some(match raw {
                0 => Self::ForkNs,
                1 => Self::ShrinkPermissions,
                2 => Self::IssueRegister,
                _ => return None,
            })
        }
    }

    // CLOEXEC flag
    pub const O_CLOEXEC: usize = 0x0100_0000;

    // fcntl flags
    pub const F_DUPFD: usize = 0;
    pub const F_GETFD: usize = 1;
    pub const F_SETFD: usize = 2;
    pub const F_DUPFD_CLOEXEC: usize = 1030;
}

#[cfg(feature = "numa")]
pub mod numa {
    pub use syscall::NumaMemoryPolicy;

    unsafe extern "C" {
        fn redox_numa_set_mem_policy_v0(policy: *const u8, len: usize) -> RawResult;
        fn redox_numa_get_mem_policy_v0(policy: *mut u8, len: usize) -> RawResult;
    }

    use crate::RawResult;
    use syscall::Error;

    pub fn set_memory_policy(policy: NumaMemoryPolicy) -> syscall::Result<()> {
        let policy = (policy as u64).to_ne_bytes();
        Error::demux(unsafe { redox_numa_set_mem_policy_v0(policy.as_ptr(), policy.len()) })?;
        Ok(())
    }

    pub fn current_memory_policy() -> syscall::Result<NumaMemoryPolicy> {
        let mut policy = [0u8; size_of::<NumaMemoryPolicy>()];
        Error::demux(unsafe {
            redox_numa_get_mem_policy_v0(policy.as_mut_ptr(), size_of::<NumaMemoryPolicy>())
        })?;
        Ok(
            NumaMemoryPolicy::try_from(u32::from_ne_bytes(policy) as u64)
                .expect("Kernel must write a valid policy"),
        )
    }
}
