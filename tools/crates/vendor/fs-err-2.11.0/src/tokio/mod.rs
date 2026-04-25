//! Tokio-specific wrappers that use `fs_err` error messages.

use crate::errors::{Error, ErrorKind, SourceDestError, SourceDestErrorKind};
use std::fs::{Metadata, Permissions};
use std::path::{Path, PathBuf};
use tokio::io;
mod dir_builder;
mod file;
mod open_options;
mod read_dir;

pub use self::open_options::OpenOptions;
pub use self::read_dir::{read_dir, DirEntry, ReadDir};
pub use dir_builder::DirBuilder;
pub use file::File;

/// Returns the canonical, absolute form of a path with all intermediate
/// components normalized and symbolic links resolved.
///
/// Wrapper for [`tokio::fs::canonicalize`].
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub async fn canonicalize(path: impl AsRef<Path>) -> io::Result<PathBuf> {
    let path = path.as_ref();
    tokio::fs::canonicalize(path)
        .await
        .map_err(|err| Error::build(err, ErrorKind::Canonicalize, path))
}

/// Copies the contents of one file to another. This function will also copy the permission bits
/// of the original file to the destination file.
/// This function will overwrite the contents of to.
///
/// Wrapper for [`tokio::fs::copy`].
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub async fn copy(from: impl AsRef<Path>, to: impl AsRef<Path>) -> Result<u64, io::Error> {
    let (from, to) = (from.as_ref(), to.as_ref());
    tokio::fs::copy(from, to)
        .await
        .map_err(|err| SourceDestError::build(err, SourceDestErrorKind::Copy, from, to))
}

/// Creates a new, empty directory at the provided path.
///
/// Wrapper for [`tokio::fs::create_dir`].
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub async fn create_dir(path: impl AsRef<Path>) -> io::Result<()> {
    let path = path.as_ref();
    tokio::fs::create_dir(path)
        .await
        .map_err(|err| Error::build(err, ErrorKind::CreateDir, path))
}

/// Recursively creates a directory and all of its parent components if they
/// are missing.
///
/// Wrapper for [`tokio::fs::create_dir_all`].
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub async fn create_dir_all(path: impl AsRef<Path>) -> io::Result<()> {
    let path = path.as_ref();
    tokio::fs::create_dir_all(path)
        .await
        .map_err(|err| Error::build(err, ErrorKind::CreateDir, path))
}

/// Creates a new hard link on the filesystem.
///
/// Wrapper for [`tokio::fs::hard_link`].
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub async fn hard_link(src: impl AsRef<Path>, dst: impl AsRef<Path>) -> io::Result<()> {
    let (src, dst) = (src.as_ref(), dst.as_ref());
    tokio::fs::hard_link(src, dst)
        .await
        .map_err(|err| SourceDestError::build(err, SourceDestErrorKind::HardLink, src, dst))
}

/// Given a path, queries the file system to get information about a file,
/// directory, etc.
///
/// Wrapper for [`tokio::fs::metadata`].
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub async fn metadata(path: impl AsRef<Path>) -> io::Result<Metadata> {
    let path = path.as_ref();
    tokio::fs::metadata(path)
        .await
        .map_err(|err| Error::build(err, ErrorKind::Metadata, path))
}

/// Reads the entire contents of a file into a bytes vector.
///
/// Wrapper for [`tokio::fs::read`].
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub async fn read(path: impl AsRef<Path>) -> io::Result<Vec<u8>> {
    let path = path.as_ref();
    tokio::fs::read(path)
        .await
        .map_err(|err| Error::build(err, ErrorKind::Read, path))
}

/// Reads a symbolic link, returning the file that the link points to.
///
/// Wrapper for [`tokio::fs::read_link`].
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub async fn read_link(path: impl AsRef<Path>) -> io::Result<PathBuf> {
    let path = path.as_ref();
    tokio::fs::read_link(path)
        .await
        .map_err(|err| Error::build(err, ErrorKind::ReadLink, path))
}

/// Creates a future which will open a file for reading and read the entire
/// contents into a string and return said string.
///
/// Wrapper for [`tokio::fs::read_to_string`].
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub async fn read_to_string(path: impl AsRef<Path>) -> io::Result<String> {
    let path = path.as_ref();
    tokio::fs::read_to_string(path)
        .await
        .map_err(|err| Error::build(err, ErrorKind::Read, path))
}

/// Removes an existing, empty directory.
///
/// Wrapper for [`tokio::fs::remove_dir`].
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub async fn remove_dir(path: impl AsRef<Path>) -> io::Result<()> {
    let path = path.as_ref();
    tokio::fs::remove_dir(path)
        .await
        .map_err(|err| Error::build(err, ErrorKind::RemoveDir, path))
}

/// Removes a directory at this path, after removing all its contents. Use carefully!
///
/// Wrapper for [`tokio::fs::remove_dir_all`].
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub async fn remove_dir_all(path: impl AsRef<Path>) -> io::Result<()> {
    let path = path.as_ref();
    tokio::fs::remove_dir_all(path)
        .await
        .map_err(|err| Error::build(err, ErrorKind::RemoveDir, path))
}

/// Removes a file from the filesystem.
///
/// Wrapper for [`tokio::fs::remove_file`].
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub async fn remove_file(path: impl AsRef<Path>) -> io::Result<()> {
    let path = path.as_ref();
    tokio::fs::remove_file(path)
        .await
        .map_err(|err| Error::build(err, ErrorKind::RemoveFile, path))
}

/// Renames a file or directory to a new name, replacing the original file if
/// `to` already exists.
///
/// Wrapper for [`tokio::fs::rename`].
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub async fn rename(from: impl AsRef<Path>, to: impl AsRef<Path>) -> io::Result<()> {
    let (from, to) = (from.as_ref(), to.as_ref());
    tokio::fs::rename(from, to)
        .await
        .map_err(|err| SourceDestError::build(err, SourceDestErrorKind::Rename, from, to))
}

/// Changes the permissions found on a file or a directory.
///
/// Wrapper for [`tokio::fs::set_permissions`].
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub async fn set_permissions(path: impl AsRef<Path>, perm: Permissions) -> io::Result<()> {
    let path = path.as_ref();
    tokio::fs::set_permissions(path, perm)
        .await
        .map_err(|err| Error::build(err, ErrorKind::SetPermissions, path))
}

/// Queries the file system metadata for a path.
///
/// Wrapper for [`tokio::fs::symlink_metadata`].
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub async fn symlink_metadata(path: impl AsRef<Path>) -> io::Result<Metadata> {
    let path = path.as_ref();
    tokio::fs::symlink_metadata(path)
        .await
        .map_err(|err| Error::build(err, ErrorKind::SymlinkMetadata, path))
}

/// Creates a new symbolic link on the filesystem.
///
/// Wrapper for [`tokio::fs::symlink`].
#[cfg(unix)]
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub async fn symlink(src: impl AsRef<Path>, dst: impl AsRef<Path>) -> io::Result<()> {
    let (src, dst) = (src.as_ref(), dst.as_ref());
    tokio::fs::symlink(src, dst)
        .await
        .map_err(|err| SourceDestError::build(err, SourceDestErrorKind::Symlink, src, dst))
}

/// Creates a new directory symlink on the filesystem.
///
/// Wrapper for [`tokio::fs::symlink_dir`].
#[cfg(windows)]
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
#[deprecated = "use fs_err::tokio::symlink_dir instead"]
pub async fn symlink(src: impl AsRef<Path>, dst: impl AsRef<Path>) -> io::Result<()> {
    symlink_dir(src, dst).await
}

/// Creates a new directory symlink on the filesystem.
///
/// Wrapper for [`tokio::fs::symlink_dir`].
#[cfg(windows)]
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub async fn symlink_dir(src: impl AsRef<Path>, dst: impl AsRef<Path>) -> io::Result<()> {
    let (src, dst) = (src.as_ref(), dst.as_ref());
    tokio::fs::symlink_dir(src, dst)
        .await
        .map_err(|err| SourceDestError::build(err, SourceDestErrorKind::SymlinkDir, src, dst))
}

/// Creates a new file symbolic link on the filesystem.
///
/// Wrapper for [`tokio::fs::symlink_file`].
#[cfg(windows)]
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub async fn symlink_file(src: impl AsRef<Path>, dst: impl AsRef<Path>) -> io::Result<()> {
    let (src, dst) = (src.as_ref(), dst.as_ref());
    tokio::fs::symlink_file(src, dst)
        .await
        .map_err(|err| SourceDestError::build(err, SourceDestErrorKind::SymlinkFile, src, dst))
}

/// Creates a future that will open a file for writing and write the entire
/// contents of `contents` to it.
///
/// Wrapper for [`tokio::fs::write`].
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub async fn write(path: impl AsRef<Path>, contents: impl AsRef<[u8]>) -> io::Result<()> {
    let (path, contents) = (path.as_ref(), contents.as_ref());
    tokio::fs::write(path, contents)
        .await
        .map_err(|err| Error::build(err, ErrorKind::Write, path))
}
