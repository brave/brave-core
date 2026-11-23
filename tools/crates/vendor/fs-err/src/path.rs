#[allow(unused_imports)]
use crate::errors::{Error, ErrorKind};
use std::fs;
use std::io;
use std::path::{Path, PathBuf};

use crate as fs_err; // for docs

/// Defines aliases on [`std::path::Path`] for fs-err functions.
///
/// This trait is sealed and can not be implemented by other crates.
//
// Because no one else can implement it, we can add methods backwards-compatibly.
pub trait PathExt: crate::Sealed {
    /// Returns Ok(true) if the path points at an existing entity.
    ///
    /// Wrapper for [`std::path::Path::try_exists`].
    #[cfg(rustc_1_63)]
    fn fs_err_try_exists(&self) -> io::Result<bool>;
    /// Given a path, query the file system to get information about a file, directory, etc.
    ///
    /// Wrapper for [`fs_err::metadata`].
    fn fs_err_metadata(&self) -> io::Result<fs::Metadata>;
    /// Query the metadata about a file without following symlinks.
    ///
    /// Wrapper for [`fs_err::symlink_metadata`].
    fn fs_err_symlink_metadata(&self) -> io::Result<fs::Metadata>;
    /// Returns the canonical, absolute form of a path with all intermediate components
    /// normalized and symbolic links resolved.
    ///
    /// Wrapper for [`fs_err::canonicalize`].
    fn fs_err_canonicalize(&self) -> io::Result<PathBuf>;
    /// Reads a symbolic link, returning the file that the link points to.
    ///
    /// Wrapper for [`fs_err::read_link`].
    fn fs_err_read_link(&self) -> io::Result<PathBuf>;
    /// Returns an iterator over the entries within a directory.
    ///
    /// Wrapper for [`fs_err::read_dir`].
    fn fs_err_read_dir(&self) -> io::Result<fs_err::ReadDir>;
}

impl PathExt for Path {
    #[cfg(rustc_1_63)]
    fn fs_err_try_exists(&self) -> io::Result<bool> {
        self.try_exists()
            .map_err(|source| Error::build(source, ErrorKind::FileExists, self))
    }

    fn fs_err_metadata(&self) -> io::Result<fs::Metadata> {
        crate::metadata(self)
    }

    fn fs_err_symlink_metadata(&self) -> io::Result<fs::Metadata> {
        crate::symlink_metadata(self)
    }

    fn fs_err_canonicalize(&self) -> io::Result<PathBuf> {
        crate::canonicalize(self)
    }

    fn fs_err_read_link(&self) -> io::Result<PathBuf> {
        crate::read_link(self)
    }

    fn fs_err_read_dir(&self) -> io::Result<fs_err::ReadDir> {
        crate::read_dir(self)
    }
}
