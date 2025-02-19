use super::{
    arch::*,
    data::{Map, Stat, StatVfs, TimeSpec},
    error::Result,
    flag::*,
    number::*,
};

use core::mem;

/// Close a file
pub fn close(fd: usize) -> Result<usize> {
    unsafe { syscall1(SYS_CLOSE, fd) }
}

/// Get the current system time
pub fn clock_gettime(clock: usize, tp: &mut TimeSpec) -> Result<usize> {
    unsafe { syscall2(SYS_CLOCK_GETTIME, clock, tp as *mut TimeSpec as usize) }
}

/// Copy and transform a file descriptor
pub fn dup(fd: usize, buf: &[u8]) -> Result<usize> {
    unsafe { syscall3(SYS_DUP, fd, buf.as_ptr() as usize, buf.len()) }
}

/// Copy and transform a file descriptor
pub fn dup2(fd: usize, newfd: usize, buf: &[u8]) -> Result<usize> {
    unsafe { syscall4(SYS_DUP2, fd, newfd, buf.as_ptr() as usize, buf.len()) }
}

/// Exit the current process
pub fn exit(status: usize) -> Result<usize> {
    unsafe { syscall1(SYS_EXIT, status) }
}

/// Change file permissions
pub fn fchmod(fd: usize, mode: u16) -> Result<usize> {
    unsafe { syscall2(SYS_FCHMOD, fd, mode as usize) }
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

/// Rename a file
pub fn frename<T: AsRef<str>>(fd: usize, path: T) -> Result<usize> {
    unsafe {
        syscall3(
            SYS_FRENAME,
            fd,
            path.as_ref().as_ptr() as usize,
            path.as_ref().len(),
        )
    }
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

/// Get metadata about a filesystem
pub fn fstatvfs(fd: usize, stat: &mut StatVfs) -> Result<usize> {
    unsafe {
        syscall3(
            SYS_FSTATVFS,
            fd,
            stat as *mut StatVfs as usize,
            mem::size_of::<StatVfs>(),
        )
    }
}

/// Sync a file descriptor to its underlying medium
pub fn fsync(fd: usize) -> Result<usize> {
    unsafe { syscall1(SYS_FSYNC, fd) }
}

/// Truncate or extend a file to a specified length
pub fn ftruncate(fd: usize, len: usize) -> Result<usize> {
    unsafe { syscall2(SYS_FTRUNCATE, fd, len) }
}

// Change modify and/or access times
pub fn futimens(fd: usize, times: &[TimeSpec]) -> Result<usize> {
    unsafe {
        syscall3(
            SYS_FUTIMENS,
            fd,
            times.as_ptr() as usize,
            times.len() * mem::size_of::<TimeSpec>(),
        )
    }
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

/// Get the effective group ID
pub fn getegid() -> Result<usize> {
    unsafe { syscall0(SYS_GETEGID) }
}

/// Get the effective namespace
pub fn getens() -> Result<usize> {
    unsafe { syscall0(SYS_GETENS) }
}

/// Get the effective user ID
pub fn geteuid() -> Result<usize> {
    unsafe { syscall0(SYS_GETEUID) }
}

/// Get the current group ID
pub fn getgid() -> Result<usize> {
    unsafe { syscall0(SYS_GETGID) }
}

/// Get the current namespace
pub fn getns() -> Result<usize> {
    unsafe { syscall0(SYS_GETNS) }
}

/// Get the current process ID
pub fn getpid() -> Result<usize> {
    unsafe { syscall0(SYS_GETPID) }
}

/// Get the process group ID
pub fn getpgid(pid: usize) -> Result<usize> {
    unsafe { syscall1(SYS_GETPGID, pid) }
}

/// Get the parent process ID
pub fn getppid() -> Result<usize> {
    unsafe { syscall0(SYS_GETPPID) }
}

/// Get the current user ID
pub fn getuid() -> Result<usize> {
    unsafe { syscall0(SYS_GETUID) }
}

/// Set the I/O privilege level
///
/// # Errors
///
/// * `EPERM` - `uid != 0`
/// * `EINVAL` - `level > 3`
pub unsafe fn iopl(level: usize) -> Result<usize> {
    syscall1(SYS_IOPL, level)
}

/// Send a signal `sig` to the process identified by `pid`
pub fn kill(pid: usize, sig: usize) -> Result<usize> {
    unsafe { syscall2(SYS_KILL, pid, sig) }
}

/// Create a link to a file
pub unsafe fn link(old: *const u8, new: *const u8) -> Result<usize> {
    syscall2(SYS_LINK, old as usize, new as usize)
}

/// Seek to `offset` bytes in a file descriptor
pub fn lseek(fd: usize, offset: isize, whence: usize) -> Result<usize> {
    unsafe { syscall3(SYS_LSEEK, fd, offset as usize, whence) }
}

/// Make a new scheme namespace
pub fn mkns(schemes: &[[usize; 2]]) -> Result<usize> {
    unsafe { syscall2(SYS_MKNS, schemes.as_ptr() as usize, schemes.len()) }
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

/// Open a file
pub fn open<T: AsRef<str>>(path: T, flags: usize) -> Result<usize> {
    unsafe {
        syscall3(
            SYS_OPEN,
            path.as_ref().as_ptr() as usize,
            path.as_ref().len(),
            flags,
        )
    }
}

/// Read from a file descriptor into a buffer
pub fn read(fd: usize, buf: &mut [u8]) -> Result<usize> {
    unsafe { syscall3(SYS_READ, fd, buf.as_mut_ptr() as usize, buf.len()) }
}

/// Remove a directory
pub fn rmdir<T: AsRef<str>>(path: T) -> Result<usize> {
    unsafe {
        syscall2(
            SYS_RMDIR,
            path.as_ref().as_ptr() as usize,
            path.as_ref().len(),
        )
    }
}

/// Set the process group ID
pub fn setpgid(pid: usize, pgid: usize) -> Result<usize> {
    unsafe { syscall2(SYS_SETPGID, pid, pgid) }
}

/// Set the current process group IDs
pub fn setregid(rgid: usize, egid: usize) -> Result<usize> {
    unsafe { syscall2(SYS_SETREGID, rgid, egid) }
}

/// Make a new scheme namespace
pub fn setrens(rns: usize, ens: usize) -> Result<usize> {
    unsafe { syscall2(SYS_SETRENS, rns, ens) }
}

/// Set the current process user IDs
pub fn setreuid(ruid: usize, euid: usize) -> Result<usize> {
    unsafe { syscall2(SYS_SETREUID, ruid, euid) }
}

/// Remove a file
pub fn unlink<T: AsRef<str>>(path: T) -> Result<usize> {
    unsafe {
        syscall2(
            SYS_UNLINK,
            path.as_ref().as_ptr() as usize,
            path.as_ref().len(),
        )
    }
}

/// Convert a virtual address to a physical one
///
/// # Errors
///
/// * `EPERM` - `uid != 0`
pub unsafe fn virttophys(virtual_address: usize) -> Result<usize> {
    syscall1(SYS_VIRTTOPHYS, virtual_address)
}

/// Check if a child process has exited or received a signal
pub fn waitpid(pid: usize, status: &mut usize, options: WaitFlags) -> Result<usize> {
    unsafe {
        syscall3(
            SYS_WAITPID,
            pid,
            status as *mut usize as usize,
            options.bits(),
        )
    }
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

/// Send a file descriptor `fd`, handled by the scheme providing `receiver_socket`. `flags` is
/// currently unused (must be zero), and `arg` is included in the scheme call.
///
/// The scheme can return an arbitrary value.
pub fn sendfd(receiver_socket: usize, fd: usize, flags: usize, arg: u64) -> Result<usize> {
    #[cfg(target_pointer_width = "32")]
    unsafe {
        syscall5(
            SYS_SENDFD,
            receiver_socket,
            fd,
            flags,
            arg as u32 as usize,
            (arg >> 32) as u32 as usize,
        )
    }

    #[cfg(target_pointer_width = "64")]
    unsafe {
        syscall4(SYS_SENDFD, receiver_socket, fd, flags, arg as usize)
    }
}
