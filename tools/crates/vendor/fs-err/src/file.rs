use std::fs;
use std::io::{self, Read, Seek, Write};
use std::path::{Path, PathBuf};

// For file time methods added in Rust 1.75.
#[cfg(rustc_1_75)]
use std::{fs::FileTimes, time::SystemTime};

use crate::errors::{Error, ErrorKind};
use crate::OpenOptions;

/// Wrapper around [`std::fs::File`][std::fs::File] which adds more helpful
/// information to all errors.
///
/// [std::fs::File]: https://doc.rust-lang.org/stable/std/fs/struct.File.html
#[derive(Debug)]
pub struct File {
    file: fs::File,
    path: PathBuf,
}

// Opens a std File and returns it or an error generator which only needs the path to produce the error.
// Exists for the `crate::read*` functions so they don't unconditionally build a PathBuf.
pub(crate) fn open(path: &Path) -> Result<std::fs::File, impl FnOnce(PathBuf) -> io::Error> {
    fs::File::open(path).map_err(|err| |path| Error::build(err, ErrorKind::OpenFile, path))
}

// like `open()` but for `crate::write`
pub(crate) fn create(path: &Path) -> Result<std::fs::File, impl FnOnce(PathBuf) -> io::Error> {
    fs::File::create(path).map_err(|err| |path| Error::build(err, ErrorKind::CreateFile, path))
}

/// Wrappers for methods from [`std::fs::File`][std::fs::File].
///
/// [std::fs::File]: https://doc.rust-lang.org/stable/std/fs/struct.File.html
impl File {
    /// Attempts to open a file in read-only mode.
    ///
    /// Wrapper for [`File::open`](https://doc.rust-lang.org/stable/std/fs/struct.File.html#method.open).
    pub fn open<P>(path: P) -> Result<Self, io::Error>
    where
        P: Into<PathBuf>,
    {
        let path = path.into();
        match open(&path) {
            Ok(file) => Ok(File::from_parts(file, path)),
            Err(err_gen) => Err(err_gen(path)),
        }
    }

    /// Opens a file in write-only mode.
    ///
    /// Wrapper for [`File::create`](https://doc.rust-lang.org/stable/std/fs/struct.File.html#method.create).
    pub fn create<P>(path: P) -> Result<Self, io::Error>
    where
        P: Into<PathBuf>,
    {
        let path = path.into();
        match create(&path) {
            Ok(file) => Ok(File::from_parts(file, path)),
            Err(err_gen) => Err(err_gen(path)),
        }
    }

    /// Opens a file in read-write mode.
    ///
    /// Wrapper for [`File::create_new`](https://doc.rust-lang.org/stable/std/fs/struct.File.html#method.create_new).
    pub fn create_new<P>(path: P) -> Result<Self, io::Error>
    where
        P: Into<PathBuf>,
    {
        let path = path.into();
        // TODO: Use fs::File::create_new once MSRV is at least 1.77
        match fs::OpenOptions::new()
            .read(true)
            .write(true)
            .create_new(true)
            .open(&path)
        {
            Ok(file) => Ok(File::from_parts(file, path)),
            Err(err) => Err(Error::build(err, ErrorKind::CreateFile, path)),
        }
    }

    /// Returns a new `OpenOptions` object.
    ///
    /// Wrapper for [`File::options`](https://doc.rust-lang.org/stable/std/fs/struct.File.html#method.options).
    pub fn options() -> OpenOptions {
        OpenOptions::new()
    }

    /// Attempts to sync all OS-internal metadata to disk.
    ///
    /// Wrapper for [`File::sync_all`](https://doc.rust-lang.org/stable/std/fs/struct.File.html#method.sync_all).
    pub fn sync_all(&self) -> Result<(), io::Error> {
        self.file
            .sync_all()
            .map_err(|source| self.error(source, ErrorKind::SyncFile))
    }

    /// This function is similar to [`sync_all`], except that it might not synchronize file metadata to the filesystem.
    ///
    /// Wrapper for [`File::sync_data`](https://doc.rust-lang.org/stable/std/fs/struct.File.html#method.sync_data).
    pub fn sync_data(&self) -> Result<(), io::Error> {
        self.file
            .sync_data()
            .map_err(|source| self.error(source, ErrorKind::SyncFile))
    }

    /// Truncates or extends the underlying file, updating the size of this file to become `size`.
    ///
    /// Wrapper for [`File::set_len`](https://doc.rust-lang.org/stable/std/fs/struct.File.html#method.set_len).
    pub fn set_len(&self, size: u64) -> Result<(), io::Error> {
        self.file
            .set_len(size)
            .map_err(|source| self.error(source, ErrorKind::SetLen))
    }

    /// Queries metadata about the underlying file.
    ///
    /// Wrapper for [`File::metadata`](https://doc.rust-lang.org/stable/std/fs/struct.File.html#method.metadata).
    pub fn metadata(&self) -> Result<fs::Metadata, io::Error> {
        self.file
            .metadata()
            .map_err(|source| self.error(source, ErrorKind::Metadata))
    }

    /// Creates a new `File` instance that shares the same underlying file handle as the
    /// existing `File` instance. Reads, writes, and seeks will affect both `File`
    /// instances simultaneously.
    ///
    /// Wrapper for [`File::try_clone`](https://doc.rust-lang.org/stable/std/fs/struct.File.html#method.try_clone).
    pub fn try_clone(&self) -> Result<Self, io::Error> {
        self.file
            .try_clone()
            .map(|file| File {
                file,
                path: self.path.clone(),
            })
            .map_err(|source| self.error(source, ErrorKind::Clone))
    }

    /// Changes the permissions on the underlying file.
    ///
    /// Wrapper for [`File::set_permissions`](https://doc.rust-lang.org/stable/std/fs/struct.File.html#method.set_permissions).
    pub fn set_permissions(&self, perm: fs::Permissions) -> Result<(), io::Error> {
        self.file
            .set_permissions(perm)
            .map_err(|source| self.error(source, ErrorKind::SetPermissions))
    }
}

/// File time methods added in Rust 1.75.
#[cfg(rustc_1_75)]
impl File {
    /// Changes the timestamps of the underlying file.
    ///
    /// Wrapper for [`File::set_times`](https://doc.rust-lang.org/stable/std/fs/struct.File.html#method.set_times).
    pub fn set_times(&self, times: FileTimes) -> Result<(), io::Error> {
        self.file
            .set_times(times)
            .map_err(|source| self.error(source, ErrorKind::SetTimes))
    }

    /// Changes the modification time of the underlying file.
    ///
    /// Wrapper for [`File::set_modified`](https://doc.rust-lang.org/stable/std/fs/struct.File.html#method.set_modified).
    pub fn set_modified(&self, time: SystemTime) -> Result<(), io::Error> {
        self.file
            .set_modified(time)
            .map_err(|source| self.error(source, ErrorKind::SetModified))
    }
}

/// Locking methods added in Rust 1.89.
#[cfg(rustc_1_89)]
impl File {
    /// Acquire an exclusive lock on the file. Blocks until the lock can be acquired.
    ///
    /// Wrapper for [`File::lock()`](https://doc.rust-lang.org/nightly/std/fs/struct.File.html#method.lock).
    pub fn lock(&self) -> Result<(), io::Error> {
        self.file
            .lock()
            .map_err(|source| self.error(source, ErrorKind::Lock))
    }

    /// Acquire a shared (non-exclusive) lock on the file. Blocks until the lock can be acquired.
    ///
    /// Wrapper for [`File::lock_shared()`](https://doc.rust-lang.org/nightly/std/fs/struct.File.html#method.lock_shared).
    pub fn lock_shared(&self) -> Result<(), io::Error> {
        self.file
            .lock_shared()
            .map_err(|source| self.error(source, ErrorKind::Lock))
    }

    /// Try to acquire an exclusive lock on the file.
    ///
    /// Wrapper for [`File::try_lock()`](https://doc.rust-lang.org/nightly/std/fs/struct.File.html#method.try_lock).
    pub fn try_lock(&self) -> Result<(), fs::TryLockError> {
        self.file.try_lock()
    }

    /// Try to acquire a shared (non-exclusive) lock on the file.
    ///
    /// Wrapper for [`File::try_lock_shared()`](https://doc.rust-lang.org/nightly/std/fs/struct.File.html#method.try_lock_shared).
    pub fn try_lock_shared(&self) -> Result<(), fs::TryLockError> {
        self.file.try_lock_shared()
    }

    /// Release all locks on the file.
    ///
    /// Wrapper for [`File::unlock()`](https://doc.rust-lang.org/nightly/std/fs/struct.File.html#method.unlock).
    pub fn unlock(&self) -> Result<(), io::Error> {
        self.file
            .unlock()
            .map_err(|source| self.error(source, ErrorKind::Unlock))
    }
}

/// Methods added by fs-err that are not available on
/// [`std::fs::File`][std::fs::File].
///
/// [std::fs::File]: https://doc.rust-lang.org/stable/std/fs/struct.File.html
impl File {
    /// Creates a [`File`](struct.File.html) from a raw file and its path.
    pub fn from_parts<P>(file: fs::File, path: P) -> Self
    where
        P: Into<PathBuf>,
    {
        File {
            file,
            path: path.into(),
        }
    }

    /// Extract the raw file and its path from this [`File`](struct.File.html)
    pub fn into_parts(self) -> (fs::File, PathBuf) {
        (self.file, self.path)
    }

    /// Consumes this [`File`](struct.File.html) and returns the underlying
    /// [`std::fs::File`][std::fs::File].
    pub fn into_file(self) -> fs::File {
        self.file
    }

    /// Consumes this [`File`](struct.File.html) and returns the underlying
    /// path as a [`PathBuf`].
    pub fn into_path(self) -> PathBuf {
        self.path
    }

    /// Returns a reference to the underlying [`std::fs::File`][std::fs::File].
    ///
    /// [std::fs::File]: https://doc.rust-lang.org/stable/std/fs/struct.File.html
    pub fn file(&self) -> &fs::File {
        &self.file
    }

    /// Returns a mutable reference to the underlying [`std::fs::File`][std::fs::File].
    ///
    /// [std::fs::File]: https://doc.rust-lang.org/stable/std/fs/struct.File.html
    pub fn file_mut(&mut self) -> &mut fs::File {
        &mut self.file
    }

    /// Returns a reference to the path that this file was created with.
    pub fn path(&self) -> &Path {
        &self.path
    }

    /// Wrap the error in information specific to this `File` object.
    fn error(&self, source: io::Error, kind: ErrorKind) -> io::Error {
        Error::build(source, kind, &self.path)
    }
}

impl Read for File {
    fn read(&mut self, buf: &mut [u8]) -> std::io::Result<usize> {
        self.file
            .read(buf)
            .map_err(|source| self.error(source, ErrorKind::Read))
    }

    fn read_vectored(&mut self, bufs: &mut [std::io::IoSliceMut<'_>]) -> std::io::Result<usize> {
        self.file
            .read_vectored(bufs)
            .map_err(|source| self.error(source, ErrorKind::Read))
    }
}

impl Read for &File {
    fn read(&mut self, buf: &mut [u8]) -> std::io::Result<usize> {
        (&self.file)
            .read(buf)
            .map_err(|source| self.error(source, ErrorKind::Read))
    }

    fn read_vectored(&mut self, bufs: &mut [std::io::IoSliceMut<'_>]) -> std::io::Result<usize> {
        (&self.file)
            .read_vectored(bufs)
            .map_err(|source| self.error(source, ErrorKind::Read))
    }
}

impl From<File> for fs::File {
    fn from(file: File) -> Self {
        file.into_file()
    }
}

impl Seek for File {
    fn seek(&mut self, pos: std::io::SeekFrom) -> std::io::Result<u64> {
        self.file
            .seek(pos)
            .map_err(|source| self.error(source, ErrorKind::Seek))
    }
}

impl Seek for &File {
    fn seek(&mut self, pos: std::io::SeekFrom) -> std::io::Result<u64> {
        (&self.file)
            .seek(pos)
            .map_err(|source| self.error(source, ErrorKind::Seek))
    }
}

impl Write for File {
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        self.file
            .write(buf)
            .map_err(|source| self.error(source, ErrorKind::Write))
    }

    fn write_vectored(&mut self, bufs: &[std::io::IoSlice<'_>]) -> std::io::Result<usize> {
        self.file
            .write_vectored(bufs)
            .map_err(|source| self.error(source, ErrorKind::Write))
    }

    fn flush(&mut self) -> std::io::Result<()> {
        self.file
            .flush()
            .map_err(|source| self.error(source, ErrorKind::Flush))
    }
}

impl Write for &File {
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        (&self.file)
            .write(buf)
            .map_err(|source| self.error(source, ErrorKind::Write))
    }

    fn write_vectored(&mut self, bufs: &[std::io::IoSlice<'_>]) -> std::io::Result<usize> {
        (&self.file)
            .write_vectored(bufs)
            .map_err(|source| self.error(source, ErrorKind::Write))
    }

    fn flush(&mut self) -> std::io::Result<()> {
        (&self.file)
            .flush()
            .map_err(|source| self.error(source, ErrorKind::Flush))
    }
}

#[cfg(unix)]
mod unix {
    use crate::os::unix::fs::FileExt;
    use crate::ErrorKind;
    use std::io;
    use std::os::unix::fs::FileExt as _;
    use std::os::unix::io::{AsRawFd, IntoRawFd, RawFd};

    impl AsRawFd for crate::File {
        fn as_raw_fd(&self) -> RawFd {
            self.file().as_raw_fd()
        }
    }

    impl IntoRawFd for crate::File {
        fn into_raw_fd(self) -> RawFd {
            self.file.into_raw_fd()
        }
    }

    impl FileExt for crate::File {
        fn read_at(&self, buf: &mut [u8], offset: u64) -> io::Result<usize> {
            self.file()
                .read_at(buf, offset)
                .map_err(|err| self.error(err, ErrorKind::ReadAt))
        }
        fn write_at(&self, buf: &[u8], offset: u64) -> io::Result<usize> {
            self.file()
                .write_at(buf, offset)
                .map_err(|err| self.error(err, ErrorKind::WriteAt))
        }
    }

    #[cfg(rustc_1_63)]
    mod io_safety {
        use std::os::unix::io::{AsFd, BorrowedFd, OwnedFd};

        impl AsFd for crate::File {
            fn as_fd(&self) -> BorrowedFd<'_> {
                self.file().as_fd()
            }
        }

        impl From<crate::File> for OwnedFd {
            fn from(file: crate::File) -> Self {
                file.into_file().into()
            }
        }
    }
}

#[cfg(windows)]
mod windows {
    use crate::os::windows::fs::FileExt;
    use crate::ErrorKind;
    use std::io;
    use std::os::windows::{
        fs::FileExt as _,
        io::{AsRawHandle, IntoRawHandle, RawHandle},
    };

    impl FileExt for crate::File {
        fn seek_read(&self, buf: &mut [u8], offset: u64) -> io::Result<usize> {
            self.file()
                .seek_read(buf, offset)
                .map_err(|err| self.error(err, ErrorKind::SeekRead))
        }

        fn seek_write(&self, buf: &[u8], offset: u64) -> io::Result<usize> {
            self.file()
                .seek_write(buf, offset)
                .map_err(|err| self.error(err, ErrorKind::SeekWrite))
        }
    }

    impl AsRawHandle for crate::File {
        fn as_raw_handle(&self) -> RawHandle {
            self.file().as_raw_handle()
        }
    }

    // can't be implemented, because the trait doesn't give us a Path
    // impl std::os::windows::io::FromRawHandle for crate::File {
    // }

    impl IntoRawHandle for crate::File {
        fn into_raw_handle(self) -> RawHandle {
            self.file.into_raw_handle()
        }
    }

    #[cfg(rustc_1_63)]
    mod io_safety {
        use std::os::windows::io::{AsHandle, BorrowedHandle, OwnedHandle};

        impl AsHandle for crate::File {
            fn as_handle(&self) -> BorrowedHandle<'_> {
                self.file().as_handle()
            }
        }

        impl From<crate::File> for OwnedHandle {
            fn from(file: crate::File) -> Self {
                file.into_parts().0.into()
            }
        }
    }
}
