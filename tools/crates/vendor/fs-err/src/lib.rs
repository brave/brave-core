/*!
fs-err is a drop-in replacement for [`std::fs`][std::fs] that provides more
helpful messages on errors. Extra information includes which operations was
attempted and any involved paths.

# Error Messages

Using [`std::fs`][std::fs], if this code fails:

```no_run
# use std::fs::File;
let file = File::open("does not exist.txt")?;
# Ok::<(), std::io::Error>(())
```

The error message that Rust gives you isn't very useful:

```txt
The system cannot find the file specified. (os error 2)
```

...but if we use fs-err instead, our error contains more actionable information:

```txt
failed to open file `does not exist.txt`: The system cannot find the file specified. (os error 2)
```

# Usage

fs-err's API is the same as [`std::fs`][std::fs], so migrating code to use it is easy.

```no_run
// use std::fs;
use fs_err as fs;

let contents = fs::read_to_string("foo.txt")?;

println!("Read foo.txt: {}", contents);

# Ok::<(), std::io::Error>(())
```

fs-err uses [`std::io::Error`][std::io::Error] for all errors. This helps fs-err
compose well with traits from the standard library like
[`std::io::Read`][std::io::Read] and crates that use them like
[`serde_json`][serde_json]:

```no_run
use fs_err::File;

let file = File::open("my-config.json")?;

// If an I/O error occurs inside serde_json, the error will include a file path
// as well as what operation was being performed.
let decoded: Vec<String> = serde_json::from_reader(file)?;

println!("Program config: {:?}", decoded);

# Ok::<(), Box<dyn std::error::Error>>(())
```

# Feature flags

* `expose_original_error`: when enabled, the [`Error::source()`](https://doc.rust-lang.org/stable/std/error/trait.Error.html#method.source) method of errors returned by this crate return the original `io::Error`. To avoid duplication in error messages,
  this also suppresses printing its message in their `Display` implementation, so make sure that you are printing the full error chain.
* `debug`: Debug filesystem errors faster by exposing more information. When a filesystem command
  fails, the error message might say "file does not exist." But it won't say **why** it doesn't exist.
  Perhaps the programmer misspelled the filename, perhaps that directory doesn't exist, or if it does,
  but the current user doesn't have permissions to see the contents. This feature analyzes the filesystem
  to output various "facts" that will help a developer debug the root of the current error.
  * Warning: Exposes filesystem metadata. This feature exposes additional metadata about your filesystem
    such as directory contents and permissions, which may be sensitive. Only enable `debug` when
    error messages won't be displayed to the end user, or they have access to filesystem metadata some
    other way.
  * Warning: This may slow down your program. This feature will trigger additional filesystem calls when
    errors occur, which may cause performance issues. Do not use if filesystem errors are common on a
    performance-sensitive "hotpath." Use in scenarios where developer hours are more expensive than
    compute time.
  * To mitigate performance and security concerns, consider only enabling this feature in `dev-dependencies`:
  * Requires Rust 1.79 or later

```toml
[dev-dependencies]
fs-err = { features = ["debug"] }
```

To use with the `tokio` feature, use `debug_tokio`:

```toml
[dependencies]
fs-err = { features = ["debug_tokio", "tokio"] }
```

# Minimum Supported Rust Version

The oldest rust version this crate is tested on is **1.40**.

This crate will generally be conservative with rust version updates. It uses the [`autocfg`](https://crates.io/crates/autocfg) crate to allow wrapping new APIs without incrementing the MSRV.

If the `tokio` feature is enabled, this crate will inherit the MSRV of the selected [`tokio`](https://crates.io/crates/tokio) version.

[std::fs]: https://doc.rust-lang.org/stable/std/fs/
[std::io::Error]: https://doc.rust-lang.org/stable/std/io/struct.Error.html
[std::io::Read]: https://doc.rust-lang.org/stable/std/io/trait.Read.html
[serde_json]: https://crates.io/crates/serde_json
*/

#![doc(html_root_url = "https://docs.rs/fs-err/3.2.2")]
#![deny(missing_debug_implementations, missing_docs)]
#![cfg_attr(docsrs, feature(doc_cfg))]

mod dir;
mod errors;
mod file;
mod open_options;
pub mod os;
mod path;
#[cfg(feature = "tokio")]
#[cfg_attr(docsrs, doc(cfg(feature = "tokio")))]
pub mod tokio;

use std::fs;
use std::io::{self, Read, Write};
use std::path::{Path, PathBuf};

use errors::{Error, ErrorKind, SourceDestError, SourceDestErrorKind};

pub use dir::*;
pub use file::*;
pub use open_options::OpenOptions;
pub use path::PathExt;

/// Read the entire contents of a file into a bytes vector.
///
/// Wrapper for [`fs::read`](https://doc.rust-lang.org/stable/std/fs/fn.read.html).
pub fn read<P: AsRef<Path>>(path: P) -> io::Result<Vec<u8>> {
    let path = path.as_ref();
    let mut file = file::open(path).map_err(|err_gen| err_gen(path.to_path_buf()))?;
    let mut bytes = Vec::with_capacity(initial_buffer_size(&file));
    file.read_to_end(&mut bytes)
        .map_err(|err| Error::build(err, ErrorKind::Read, path))?;
    Ok(bytes)
}

/// Read the entire contents of a file into a string.
///
/// Wrapper for [`fs::read_to_string`](https://doc.rust-lang.org/stable/std/fs/fn.read_to_string.html).
pub fn read_to_string<P: AsRef<Path>>(path: P) -> io::Result<String> {
    let path = path.as_ref();
    let mut file = file::open(path).map_err(|err_gen| err_gen(path.to_path_buf()))?;
    let mut string = String::with_capacity(initial_buffer_size(&file));
    file.read_to_string(&mut string)
        .map_err(|err| Error::build(err, ErrorKind::Read, path))?;
    Ok(string)
}

/// Write a slice as the entire contents of a file.
///
/// Wrapper for [`fs::write`](https://doc.rust-lang.org/stable/std/fs/fn.write.html).
pub fn write<P: AsRef<Path>, C: AsRef<[u8]>>(path: P, contents: C) -> io::Result<()> {
    let path = path.as_ref();
    file::create(path)
        .map_err(|err_gen| err_gen(path.to_path_buf()))?
        .write_all(contents.as_ref())
        .map_err(|err| Error::build(err, ErrorKind::Write, path))
}

/// Copies the contents of one file to another. This function will also copy the
/// permission bits of the original file to the destination file.
///
/// Wrapper for [`fs::copy`](https://doc.rust-lang.org/stable/std/fs/fn.copy.html).
pub fn copy<P, Q>(from: P, to: Q) -> io::Result<u64>
where
    P: AsRef<Path>,
    Q: AsRef<Path>,
{
    let from = from.as_ref();
    let to = to.as_ref();
    fs::copy(from, to)
        .map_err(|source| SourceDestError::build(source, SourceDestErrorKind::Copy, from, to))
}

/// Creates a new, empty directory at the provided path.
///
/// Wrapper for [`fs::create_dir`](https://doc.rust-lang.org/stable/std/fs/fn.create_dir.html).
pub fn create_dir<P>(path: P) -> io::Result<()>
where
    P: AsRef<Path>,
{
    let path = path.as_ref();
    fs::create_dir(path).map_err(|source| Error::build(source, ErrorKind::CreateDir, path))
}

/// Recursively create a directory and all of its parent components if they are missing.
///
/// Wrapper for [`fs::create_dir_all`](https://doc.rust-lang.org/stable/std/fs/fn.create_dir_all.html).
pub fn create_dir_all<P>(path: P) -> io::Result<()>
where
    P: AsRef<Path>,
{
    let path = path.as_ref();
    fs::create_dir_all(path).map_err(|source| Error::build(source, ErrorKind::CreateDir, path))
}

/// Removes an empty directory.
///
/// Wrapper for [`fs::remove_dir`](https://doc.rust-lang.org/stable/std/fs/fn.remove_dir.html).
pub fn remove_dir<P>(path: P) -> io::Result<()>
where
    P: AsRef<Path>,
{
    let path = path.as_ref();
    fs::remove_dir(path).map_err(|source| Error::build(source, ErrorKind::RemoveDir, path))
}

/// Removes a directory at this path, after removing all its contents. Use carefully!
///
/// Wrapper for [`fs::remove_dir_all`](https://doc.rust-lang.org/stable/std/fs/fn.remove_dir_all.html).
pub fn remove_dir_all<P>(path: P) -> io::Result<()>
where
    P: AsRef<Path>,
{
    let path = path.as_ref();
    fs::remove_dir_all(path).map_err(|source| Error::build(source, ErrorKind::RemoveDir, path))
}

/// Removes a file from the filesystem.
///
/// Wrapper for [`fs::remove_file`](https://doc.rust-lang.org/stable/std/fs/fn.remove_file.html).
pub fn remove_file<P>(path: P) -> io::Result<()>
where
    P: AsRef<Path>,
{
    let path = path.as_ref();
    fs::remove_file(path).map_err(|source| Error::build(source, ErrorKind::RemoveFile, path))
}

/// Given a path, query the file system to get information about a file, directory, etc.
///
/// Wrapper for [`fs::metadata`](https://doc.rust-lang.org/stable/std/fs/fn.metadata.html).
pub fn metadata<P: AsRef<Path>>(path: P) -> io::Result<fs::Metadata> {
    let path = path.as_ref();
    fs::metadata(path).map_err(|source| Error::build(source, ErrorKind::Metadata, path))
}

/// Returns `Ok(true)` if the path points at an existing entity.
///
/// Wrapper for [`fs::exists`](https://doc.rust-lang.org/stable/std/fs/fn.exists.html).
#[cfg(rustc_1_81)]
pub fn exists<P: AsRef<Path>>(path: P) -> io::Result<bool> {
    let path = path.as_ref();
    fs::exists(path).map_err(|source| Error::build(source, ErrorKind::FileExists, path))
}

/// Returns the canonical, absolute form of a path with all intermediate components
/// normalized and symbolic links resolved.
///
/// Wrapper for [`fs::canonicalize`](https://doc.rust-lang.org/stable/std/fs/fn.canonicalize.html).
pub fn canonicalize<P: AsRef<Path>>(path: P) -> io::Result<PathBuf> {
    let path = path.as_ref();
    fs::canonicalize(path).map_err(|source| Error::build(source, ErrorKind::Canonicalize, path))
}

/// Creates a new hard link on the filesystem.
///
/// The `link` path will be a link pointing to the `original` path. Note that
/// systems often require these two paths to both be located on the same
/// filesystem.
///
/// Wrapper for [`fs::hard_link`](https://doc.rust-lang.org/stable/std/fs/fn.hard_link.html).
pub fn hard_link<P: AsRef<Path>, Q: AsRef<Path>>(original: P, link: Q) -> io::Result<()> {
    let original = original.as_ref();
    let link = link.as_ref();
    fs::hard_link(original, link).map_err(|source| {
        SourceDestError::build(source, SourceDestErrorKind::HardLink, link, original)
    })
}

/// Reads a symbolic link, returning the file that the link points to.
///
/// Wrapper for [`fs::read_link`](https://doc.rust-lang.org/stable/std/fs/fn.read_link.html).
pub fn read_link<P: AsRef<Path>>(path: P) -> io::Result<PathBuf> {
    let path = path.as_ref();
    fs::read_link(path).map_err(|source| Error::build(source, ErrorKind::ReadLink, path))
}

/// Rename a file or directory to a new name, replacing the original file if to already exists.
///
/// Wrapper for [`fs::rename`](https://doc.rust-lang.org/stable/std/fs/fn.rename.html).
pub fn rename<P: AsRef<Path>, Q: AsRef<Path>>(from: P, to: Q) -> io::Result<()> {
    let from = from.as_ref();
    let to = to.as_ref();
    fs::rename(from, to)
        .map_err(|source| SourceDestError::build(source, SourceDestErrorKind::Rename, from, to))
}

/// Creates a new symbolic link on the filesystem.
///
/// The `link` path will be a symbolic link pointing to the `original` path.
///
/// Wrapper for [`fs::soft_link`](https://doc.rust-lang.org/stable/std/fs/fn.soft_link.html).
#[deprecated = "replaced with std::os::unix::fs::symlink and \
std::os::windows::fs::{symlink_file, symlink_dir}"]
pub fn soft_link<P: AsRef<Path>, Q: AsRef<Path>>(original: P, link: Q) -> io::Result<()> {
    let original = original.as_ref();
    let link = link.as_ref();
    #[allow(deprecated)]
    fs::soft_link(original, link).map_err(|source| {
        SourceDestError::build(source, SourceDestErrorKind::SoftLink, link, original)
    })
}

/// Query the metadata about a file without following symlinks.
///
/// Wrapper for [`fs::symlink_metadata`](https://doc.rust-lang.org/stable/std/fs/fn.symlink_metadata.html).
pub fn symlink_metadata<P: AsRef<Path>>(path: P) -> io::Result<fs::Metadata> {
    let path = path.as_ref();
    fs::symlink_metadata(path)
        .map_err(|source| Error::build(source, ErrorKind::SymlinkMetadata, path))
}

/// Changes the permissions found on a file or a directory.
///
/// Wrapper for [`fs::set_permissions`](https://doc.rust-lang.org/stable/std/fs/fn.set_permissions.html).
pub fn set_permissions<P: AsRef<Path>>(path: P, perm: fs::Permissions) -> io::Result<()> {
    let path = path.as_ref();
    fs::set_permissions(path, perm)
        .map_err(|source| Error::build(source, ErrorKind::SetPermissions, path))
}

fn initial_buffer_size(file: &std::fs::File) -> usize {
    file.metadata().map(|m| m.len() as usize + 1).unwrap_or(0)
}

pub(crate) use private::Sealed;
mod private {
    pub trait Sealed {}

    impl Sealed for crate::File {}
    impl Sealed for std::path::Path {}
    impl Sealed for crate::OpenOptions {}
}
