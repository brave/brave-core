use super::{
    arch::*,
    data::{Map, Stat, StdFsCallMeta, TimeSpec},
    error::Result,
    flag::*,
    number::*,
};

use core::mem;

/// Get the current system time
pub fn clock_gettime(clock: usize, tp: &mut TimeSpec) -> Result<usize> {
    unsafe { syscall2(SYS_CLOCK_GETTIME, clock, tp as *mut TimeSpec as usize) }
}

/// Copy and transform a file descriptor into specified fd number
pub fn dup_into(fd: usize, out: usize, buf: &[u8]) -> Result<usize> {
    unsafe { syscall4(SYS_DUP_INTO, fd, buf.as_ptr() as usize, buf.len(), out) }
}

/// Copy and transform a file descriptor
pub fn dup2(fd: usize, newfd: usize, buf: &[u8]) -> Result<usize> {
    unsafe { syscall4(SYS_DUP2, fd, newfd, buf.as_ptr() as usize, buf.len()) }
}

/// Change file ownership
pub fn fchown(fd: usize, uid: u32, gid: u32) -> Result<usize> {
    unsafe { syscall3(SYS_FCHOWN, fd, uid as usize, gid as usize) }
}

/// Change file descriptor flags
pub fn fcntl(fd: usize, cmd: usize, arg: usize) -> Result<usize> {
    unsafe { syscall3(SYS_FCNTL, fd, cmd, arg) }
}

/// Map a file into memory, but with the ability to set the address to map into, either as a hint
/// or as a requirement of the map.
///
/// # Errors
/// `EACCES` - the file descriptor was not open for reading
/// `EBADF` - if the file descriptor was invalid
/// `ENODEV` - mmapping was not supported
/// `EINVAL` - invalid combination of flags
/// `EEXIST` - if [`MapFlags::MAP_FIXED`] was set, and the address specified was already in use.
///
pub unsafe fn fmap(fd: usize, map: &Map) -> Result<usize> {
    syscall3(
        SYS_FMAP,
        fd,
        map as *const Map as usize,
        mem::size_of::<Map>(),
    )
}

/// Unmap whole (or partial) continous memory-mapped files
pub unsafe fn funmap(addr: usize, len: usize) -> Result<usize> {
    syscall2(SYS_FUNMAP, addr, len)
}

/// Retrieve the canonical path of a file
pub fn fpath(fd: usize, buf: &mut [u8]) -> Result<usize> {
    unsafe { syscall3(SYS_FPATH, fd, buf.as_mut_ptr() as usize, buf.len()) }
}

/// Create a link to a file
pub fn flink<T: AsRef<str>>(fd: usize, path: T) -> Result<usize> {
    let path = path.as_ref();
    unsafe { syscall3(SYS_FLINK, fd, path.as_ptr() as usize, path.len()) }
}

/// Rename a file
pub fn frename<T: AsRef<str>>(fd: usize, path: T) -> Result<usize> {
    let path = path.as_ref();
    unsafe { syscall3(SYS_FRENAME, fd, path.as_ptr() as usize, path.len()) }
}

/// Get metadata about a file
pub fn fstat(fd: usize, stat: &mut Stat) -> Result<usize> {
    unsafe {
        syscall3(
            SYS_FSTAT,
            fd,
            stat as *mut Stat as usize,
            mem::size_of::<Stat>(),
        )
    }
}

/// Sync a file descriptor to its underlying medium
pub fn fsync(fd: usize) -> Result<usize> {
    unsafe { syscall1(SYS_FSYNC, fd) }
}

/// Fast userspace mutex
pub unsafe fn futex(
    addr: *mut i32,
    op: usize,
    val: i32,
    val2: usize,
    addr2: *mut i32,
) -> Result<usize> {
    syscall5(
        SYS_FUTEX,
        addr as usize,
        op,
        (val as isize) as usize,
        val2,
        addr2 as usize,
    )
}

/// Seek to `offset` bytes in a file descriptor
pub fn lseek(fd: usize, offset: isize, whence: usize) -> Result<usize> {
    unsafe { syscall3(SYS_LSEEK, fd, offset as usize, whence) }
}

/// Change mapping flags
pub unsafe fn mprotect(addr: usize, size: usize, flags: MapFlags) -> Result<usize> {
    syscall3(SYS_MPROTECT, addr, size, flags.bits())
}

/// Sleep for the time specified in `req`
pub fn nanosleep(req: &TimeSpec, rem: &mut TimeSpec) -> Result<usize> {
    unsafe {
        syscall2(
            SYS_NANOSLEEP,
            req as *const TimeSpec as usize,
            rem as *mut TimeSpec as usize,
        )
    }
}

/// Open a file at a specific path into specified fd number
pub fn openat_into(
    fd: usize,
    out: usize,
    path: impl AsRef<str>,
    flags: usize,
    fcntl_flags: usize,
) -> Result<usize> {
    let path = path.as_ref();
    unsafe {
        syscall5(
            SYS_OPENAT_INTO2,
            fd,
            path.as_ptr() as usize,
            path.len() | (fcntl_flags & O_FCNTL_MASK),
            flags,
            out,
        )
    }
}

/// Remove a file at at specific path
pub fn unlinkat<T: AsRef<str>>(fd: usize, path: T, flags: usize) -> Result<usize> {
    let path = path.as_ref();
    unsafe { syscall4(SYS_UNLINKAT, fd, path.as_ptr() as usize, path.len(), flags) }
}
/// Read from a file descriptor into a buffer
pub fn read(fd: usize, buf: &mut [u8]) -> Result<usize> {
    unsafe { syscall3(SYS_READ, fd, buf.as_mut_ptr() as usize, buf.len()) }
}

/// Write a buffer to a file descriptor
///
/// The kernel will attempt to write the bytes in `buf` to the file descriptor `fd`, returning
/// either an `Err`, explained below, or `Ok(count)` where `count` is the number of bytes which
/// were written.
///
/// # Errors
///
/// * `EAGAIN` - the file descriptor was opened with `O_NONBLOCK` and writing would block
/// * `EBADF` - the file descriptor is not valid or is not open for writing
/// * `EFAULT` - `buf` does not point to the process's addressible memory
/// * `EIO` - an I/O error occurred
/// * `ENOSPC` - the device containing the file descriptor has no room for data
/// * `EPIPE` - the file descriptor refers to a pipe or socket whose reading end is closed
pub fn write(fd: usize, buf: &[u8]) -> Result<usize> {
    unsafe { syscall3(SYS_WRITE, fd, buf.as_ptr() as usize, buf.len()) }
}

/// Yield the process's time slice to the kernel
///
/// This function will return Ok(0) on success
pub fn sched_yield() -> Result<usize> {
    unsafe { syscall0(SYS_YIELD) }
}

pub trait Call {
    #[inline]
    fn extra_flags(&self) -> CallFlags {
        CallFlags::empty()
    }
    unsafe fn raw_call(
        &self,
        payload_ptr: *const u8,
        len: usize,
        flags: CallFlags,
        metadata: &[u64],
    ) -> Result<usize>;
}

impl Call for usize {
    unsafe fn raw_call(
        &self,
        payload_ptr: *const u8,
        len: usize,
        flags: CallFlags,
        metadata: &[u64],
    ) -> Result<usize> {
        unsafe {
            syscall5(
                SYS_CALL,
                *self,
                payload_ptr as usize,
                len,
                metadata.len() | flags.bits(),
                metadata.as_ptr() as usize,
            )
        }
    }
}

impl Call for &[usize] {
    #[inline]
    fn extra_flags(&self) -> CallFlags {
        CallFlags::MULTIPLE_FDS2
    }
    unsafe fn raw_call(
        &self,
        payload_ptr: *const u8,
        len: usize,
        flags: CallFlags,
        metadata: &[u64],
    ) -> Result<usize> {
        // align_of::<usize>() is >= 4 on all architectures supported by Redox, whereas the kernel
        // currently only allows MULTIPLE_FDS2 calls to use at most 2.
        if self.len() > core::mem::align_of::<usize>() {
            return Err(crate::error::Error::new(crate::error::EINVAL));
        }
        unsafe {
            syscall5(
                SYS_CALL,
                self.as_ptr() as usize | self.len(),
                payload_ptr as usize,
                len,
                metadata.len() | flags.bits(),
                metadata.as_ptr() as usize,
            )
        }
    }
}

/// SYS_CALL interface, read-only variant
pub fn call_ro<T: Call>(
    fd: T,
    payload: &mut [u8],
    flags: CallFlags,
    metadata: &[u64],
) -> Result<usize> {
    let combined_flags = flags | fd.extra_flags();
    unsafe {
        fd.raw_call(
            payload.as_mut_ptr(),
            payload.len(),
            combined_flags | CallFlags::READ,
            metadata,
        )
    }
}
/// SYS_CALL interface, write-only variant
pub fn call_wo<T: Call>(
    fd: T,
    payload: &[u8],
    flags: CallFlags,
    metadata: &[u64],
) -> Result<usize> {
    let combined_flags = flags | fd.extra_flags();
    unsafe {
        fd.raw_call(
            payload.as_ptr(),
            payload.len(),
            combined_flags | CallFlags::WRITE,
            metadata,
        )
    }
}
/// SYS_CALL interface, read-write variant
pub fn call_rw<T: Call>(
    fd: T,
    payload: &mut [u8],
    flags: CallFlags,
    metadata: &[u64],
) -> Result<usize> {
    let combined_flags = flags | fd.extra_flags();
    unsafe {
        fd.raw_call(
            payload.as_mut_ptr(),
            payload.len(),
            combined_flags | CallFlags::READ | CallFlags::WRITE,
            metadata,
        )
    }
}

pub fn std_fs_call<T: Call>(fd: T, payload: &mut [u8], metadata: &StdFsCallMeta) -> Result<usize> {
    call_rw(fd, payload, CallFlags::STD_FS, metadata)
}
