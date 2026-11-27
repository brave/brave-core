use std::ffi::OsString;
use std::fs;
use std::io;
use std::path::PathBuf;

use crate::errors::{Error, ErrorKind};

#[allow(unused_imports)]
use crate as fs_err; // for docs

/// Returns an iterator over the entries within a directory.
///
/// Wrapper for [`std::fs::read_dir`].
pub fn read_dir<P: Into<PathBuf>>(path: P) -> io::Result<ReadDir> {
    let path = path.into();

    match fs::read_dir(&path) {
        Ok(inner) => Ok(ReadDir { inner, path }),
        Err(source) => Err(Error::build(source, ErrorKind::ReadDir, path)),
    }
}

/// Wrapper around [`std::fs::ReadDir`] which adds more helpful information to
/// all errors.
///
/// This struct is created via [`fs_err::read_dir`].
#[derive(Debug)]
pub struct ReadDir {
    inner: fs::ReadDir,
    path: PathBuf,
}

impl Iterator for ReadDir {
    type Item = io::Result<DirEntry>;

    fn next(&mut self) -> Option<Self::Item> {
        Some(
            self.inner
                .next()?
                .map_err(|source| Error::build(source, ErrorKind::ReadDir, &self.path))
                .map(|inner| DirEntry { inner }),
        )
    }
}

/// Wrapper around [`std::fs::DirEntry`] which adds more
/// helpful information to all errors.
#[derive(Debug)]
pub struct DirEntry {
    inner: fs::DirEntry,
}

impl DirEntry {
    /// Returns the full path to the file that this entry represents.
    ///
    /// Wrapper for [`std::fs::DirEntry::path`].
    pub fn path(&self) -> PathBuf {
        self.inner.path()
    }

    /// Returns the metadata for the file that this entry points at.
    ///
    /// Wrapper for [`std::fs::DirEntry::metadata`].
    pub fn metadata(&self) -> io::Result<fs::Metadata> {
        self.inner
            .metadata()
            .map_err(|source| Error::build(source, ErrorKind::Metadata, self.path()))
    }

    /// Returns the file type for the file that this entry points at.
    ///
    /// Wrapper for [`std::fs::DirEntry::file_type`].
    pub fn file_type(&self) -> io::Result<fs::FileType> {
        self.inner
            .file_type()
            .map_err(|source| Error::build(source, ErrorKind::Metadata, self.path()))
    }

    /// Returns the file name of this directory entry without any leading path component(s).
    ///
    /// Wrapper for [`std::fs::DirEntry::file_name`].
    pub fn file_name(&self) -> OsString {
        self.inner.file_name()
    }
}

#[cfg(unix)]
mod unix {
    use std::os::unix::fs::DirEntryExt;

    use super::*;

    impl DirEntryExt for DirEntry {
        fn ino(&self) -> u64 {
            self.inner.ino()
        }
    }
}
