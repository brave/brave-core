//! Provides facilities for file locks on unix and windows

use crate::{Error, Path, PathBuf};
use std::{fs, time::Duration};

#[cfg_attr(unix, path = "flock/unix.rs")]
#[cfg_attr(windows, path = "flock/windows.rs")]
mod sys;

/// An error pertaining to a failed file lock
#[derive(Debug, thiserror::Error)]
#[error("failed to obtain lock file '{path}'")]
pub struct FileLockError {
    /// The path of the file lock
    pub path: PathBuf,
    /// The underlying failure reason
    pub source: LockError,
}

/// Errors that can occur when attempting to acquire a [`FileLock`]
#[derive(Debug, thiserror::Error)]
pub enum LockError {
    /// An I/O error occurred attempting to open the lock file
    #[error(transparent)]
    Open(std::io::Error),
    /// Exclusive locks cannot be take on read-only file systems
    #[error("attempted to take an exclusive lock on a read-only path")]
    Readonly,
    /// Failed to create parents directories to lock file
    #[error("failed to create parent directories for lock path")]
    CreateDir(std::io::Error),
    /// Locking is not supported if the lock file is on an NFS, though note this
    /// is a bit more nuanced as `NFSv4` _does_ support file locking, but is out
    /// of scope, at least for now
    #[error("NFS do not support locking")]
    Nfs,
    /// This could happen on eg. _extremely_ old and outdated OSes or some filesystems
    /// and is only present for completeness
    #[error("locking is not supported on the filesystem and/or in the kernel")]
    NotSupported,
    /// An I/O error occurred attempting to un/lock the file
    #[error("failed to acquire or release file lock")]
    Lock(std::io::Error),
    /// The lock could not be acquired within the caller provided timeout
    #[error("failed to acquire lock within the specified duration")]
    TimedOut,
    /// The lock is currently held by another
    #[error("the lock is currently held by another")]
    Contested,
}

/// Provides options for creating a [`FileLock`]
pub struct LockOptions<'pb> {
    path: std::borrow::Cow<'pb, Path>,
    exclusive: bool,
    shared_fallback: bool,
}

impl<'pb> LockOptions<'pb> {
    /// Creates a new [`Self`] for the specified path
    #[inline]
    pub fn new(path: &'pb Path) -> Self {
        Self {
            path: path.into(),
            exclusive: false,
            shared_fallback: false,
        }
    }

    /// Creates a new [`Self`] for locking cargo's global package lock
    ///
    /// If specified, the path is used as the root, otherwise it is rooted at
    /// the path determined by `$CARGO_HOME`
    #[inline]
    pub fn cargo_package_lock(root: Option<PathBuf>) -> Result<Self, Error> {
        let mut path = if let Some(root) = root {
            root
        } else {
            crate::utils::cargo_home()?
        };
        path.push(".package-cache");

        Ok(Self {
            path: path.into(),
            exclusive: true,
            shared_fallback: false,
        })
    }

    /// Will attempt to acquire a shared lock rather than an exclusive one
    #[inline]
    pub fn shared(mut self) -> Self {
        self.exclusive = false;
        self
    }

    /// Will attempt to acquire an exclusive lock, which can optionally fallback
    /// to a shared lock if the lock file is for a read only filesystem
    #[inline]
    pub fn exclusive(mut self, shared_fallback: bool) -> Self {
        self.exclusive = true;
        self.shared_fallback = shared_fallback;
        self
    }

    /// Attempts to acquire a lock, but fails immediately if the lock is currently
    /// held
    #[inline]
    pub fn try_lock(&self) -> Result<FileLock, Error> {
        self.open_and_lock(Option::<fn(&Path) -> Option<Duration>>::None)
    }

    /// Attempts to acquire a lock, waiting if the lock is currently held.
    ///
    /// Unlike [`Self::try_lock`], if the lock is currently held, the specified
    /// callback is called to inform the caller that a wait is about to
    /// be performed, then waits for the amount of time specified by the return
    /// of the callback, or infinitely in the case of `None`.
    #[inline]
    pub fn lock(&self, wait: impl Fn(&Path) -> Option<Duration>) -> Result<FileLock, Error> {
        self.open_and_lock(Some(wait))
    }

    fn open(&self, opts: &fs::OpenOptions) -> Result<fs::File, FileLockError> {
        opts.open(self.path.as_std_path()).or_else(|err| {
            if err.kind() == std::io::ErrorKind::NotFound && self.exclusive {
                fs::create_dir_all(self.path.parent().unwrap()).map_err(|e| FileLockError {
                    path: self.path.parent().unwrap().to_owned(),
                    source: LockError::CreateDir(e),
                })?;
                self.open(opts)
            } else {
                // Note we just use the 30 EROFS constant here, which won't work on WASI, Haiku, or some other
                // niche targets, but none of them are intended targets for this crate, but can be fixed later
                // if someone actually uses them
                let source = if err.kind() == std::io::ErrorKind::PermissionDenied
                    || cfg!(unix) && err.raw_os_error() == Some(30 /* EROFS */)
                {
                    LockError::Readonly
                } else {
                    LockError::Open(err)
                };

                Err(FileLockError {
                    path: self.path.as_ref().to_owned(),
                    source,
                })
            }
        })
    }

    fn open_and_lock(
        &self,
        wait: Option<impl Fn(&Path) -> Option<Duration>>,
    ) -> Result<FileLock, Error> {
        let (state, file) = if self.exclusive {
            match self.open(&sys::open_opts(true)) {
                Ok(file) => (LockState::Exclusive, file),
                Err(err) => {
                    // If the user requested it, check if the error is due to a read only error,
                    // and if so, fallback to a shared lock instead of an exclusive lock, just
                    // as cargo does
                    //
                    // https://github.com/rust-lang/cargo/blob/0b6cc3c75f1813df857fb54421edf7f8fee548e3/src/cargo/util/config/mod.rs#L1907-L1935
                    if self.shared_fallback && matches!(err.source, LockError::Readonly) {
                        (LockState::Shared, self.open(&sys::open_opts(false))?)
                    } else {
                        return Err(err.into());
                    }
                }
            }
        } else {
            (LockState::Shared, self.open(&sys::open_opts(false))?)
        };

        self.do_lock(state, &file, wait)
            .map_err(|source| FileLockError {
                path: self.path.as_ref().to_owned(),
                source,
            })?;

        Ok(FileLock {
            file: Some(file),
            state,
        })
    }

    fn do_lock(
        &self,
        state: LockState,
        file: &fs::File,
        wait: Option<impl Fn(&Path) -> Option<std::time::Duration>>,
    ) -> Result<(), LockError> {
        #[cfg(all(target_os = "linux", not(target_env = "musl")))]
        fn is_on_nfs_mount(path: &crate::Path) -> bool {
            use std::os::unix::prelude::*;

            let path = match std::ffi::CString::new(path.as_os_str().as_bytes()) {
                Ok(path) => path,
                Err(_) => return false,
            };

            #[allow(unsafe_code)]
            unsafe {
                let mut buf: libc::statfs = std::mem::zeroed();
                let r = libc::statfs(path.as_ptr(), &mut buf);

                r == 0 && buf.f_type as u32 == libc::NFS_SUPER_MAGIC as u32
            }
        }

        #[cfg(any(not(target_os = "linux"), target_env = "musl"))]
        fn is_on_nfs_mount(_path: &crate::Path) -> bool {
            false
        }

        // File locking on Unix is currently implemented via `flock`, which is known
        // to be broken on NFS. We could in theory just ignore errors that happen on
        // NFS, but apparently the failure mode [1] for `flock` on NFS is **blocking
        // forever**, even if the "non-blocking" flag is passed!
        //
        // As a result, we just skip all file locks entirely on NFS mounts. That
        // should avoid calling any `flock` functions at all, and it wouldn't work
        // there anyway.
        //
        // [1]: https://github.com/rust-lang/cargo/issues/2615
        if is_on_nfs_mount(&self.path) {
            return Err(LockError::Nfs);
        }

        match sys::try_lock(file, state) {
            Ok(()) => return Ok(()),

            // In addition to ignoring NFS which is commonly not working we also
            // just ignore locking on filesystems that look like they don't
            // implement file locking.
            Err(e) if sys::is_unsupported(&e) => return Err(LockError::NotSupported),

            Err(e) => {
                if !sys::is_contended(&e) {
                    return Err(LockError::Lock(e));
                }
            }
        }

        // Signal to the caller that we are about to enter a blocking operation
        // and whether they want to assign a timeout to it
        if let Some(wait) = wait {
            let timeout = wait(&self.path);

            sys::lock(file, state, timeout).map_err(|e| {
                if sys::is_timed_out(&e) {
                    LockError::TimedOut
                } else {
                    LockError::Lock(e)
                }
            })
        } else {
            Err(LockError::Contested)
        }
    }
}

#[derive(PartialEq, Copy, Clone, Debug)]
enum LockState {
    Exclusive,
    Shared,
    Unlocked,
}

/// A currently held file lock.
///
/// The lock is released when this is dropped, or the program exits for any reason,
/// including `SIGKILL` or power loss
pub struct FileLock {
    file: Option<std::fs::File>,
    state: LockState,
}

impl FileLock {
    /// Creates a [`Self`] in an unlocked state.
    ///
    /// This allows for easy testing or use in situations where you don't care
    /// about file locking, or have other ways to ensure something is uncontested
    pub fn unlocked() -> Self {
        Self {
            file: None,
            state: LockState::Unlocked,
        }
    }
}

impl Drop for FileLock {
    fn drop(&mut self) {
        if self.state != LockState::Unlocked {
            if let Some(f) = self.file.take() {
                let _ = sys::unlock(&f);
            }
        }
    }
}
