//! Path newtypes which are always guaranteed to be canonical
//!
//! In the same way a `str` "guarantees" a `&[u8]` contains only valid UTF-8 data,
//! `CanonicalPath` and `CanonicalPathBuf` guarantee that the paths they represent
//! are canonical, or at least, were canonical at the time they were created.

#![deny(
    warnings,
    missing_docs,
    trivial_numeric_casts,
    unused_import_braces,
    unused_qualifications
)]
#![doc(html_root_url = "https://docs.rs/canonical-path/2.0.2")]

use std::{
    borrow::Borrow,
    env,
    ffi::{OsStr, OsString},
    fs::{Metadata, ReadDir},
    io::{Error, ErrorKind, Result},
    path::{Components, Display, Iter, Path, PathBuf},
};

/// Common methods of `CanonicalPath` and `CanonicalPathBuf`
macro_rules! impl_path {
    () => {
        /// Return a `Path` reference.
        #[inline]
        pub fn as_path(&self) -> &Path {
            self.0.as_ref()
        }

        /// Return an `OsStr` reference.
        #[inline]
        pub fn as_os_str(&self) -> &OsStr {
            self.0.as_os_str()
        }

        /// Yields a `&str` slice if the path is valid unicode.
        #[inline]
        pub fn to_str(&self) -> Option<&str> {
            self.0.to_str()
        }

        /// Return a canonical parent path of this path, or `io::Error` if the
        /// path is the root directory or another canonicalization error occurs.
        pub fn parent(&self) -> Result<CanonicalPathBuf> {
            CanonicalPathBuf::new(&self.0
                .parent()
                .ok_or_else(|| Error::new(ErrorKind::InvalidInput, "can't get parent of '/'"))?)
        }

        /// Returns the final component of the path, if there is one.
        #[inline]
        pub fn file_name(&self) -> Option<&OsStr> {
            self.0.file_name()
        }

        /// Determines whether base is a prefix of self.
        #[inline]
        pub fn starts_with<P: AsRef<Path>>(&self, base: P) -> bool {
            self.0.starts_with(base)
        }

        /// Determines whether child is a suffix of self.
        #[inline]
        pub fn ends_with<P: AsRef<Path>>(&self, child: P) -> bool {
            self.0.ends_with(child)
        }

        /// Extracts the stem (non-extension) portion of `self.file_name`.
        #[inline]
        pub fn file_stem(&self) -> Option<&OsStr> {
            self.0.file_stem()
        }

        /// Extracts the extension of `self.file_name`, if possible.
        #[inline]
        pub fn extension(&self) -> Option<&OsStr> {
            self.0.extension()
        }

        /// Creates an owned `CanonicalPathBuf` like self but with the given file name.
        #[inline]
        pub fn with_file_name<S: AsRef<OsStr>>(&self, file_name: S) -> Result<CanonicalPathBuf> {
            CanonicalPathBuf::new(&self.0.with_file_name(file_name))
        }

        /// Creates an owned `CanonicalPathBuf` like self but with the given extension.
        #[inline]
        pub fn with_extension<S: AsRef<OsStr>>(&self, extension: S) -> Result<CanonicalPathBuf> {
            CanonicalPathBuf::new(&self.0.with_extension(extension))
        }

        /// Produces an iterator over the `Component`s of a path
        #[inline]
        pub fn components(&self) -> Components {
            self.0.components()
        }

        /// Produces an iterator over the path's components viewed as
        /// `OsStr` slices.
         #[inline]
        pub fn iter(&self) -> Iter {
            self.0.iter()
        }

        /// Returns an object that implements `Display` for safely printing
        /// paths that may contain non-Unicode data.
        #[inline]
        pub fn display(&self) -> Display {
            self.0.display()
        }

        /// Queries the file system to get information about a file, directory, etc.
        ///
        /// Unlike the `std` version of this method, it will not follow symlinks,
        /// since as a canonical path we should be symlink-free.
        #[inline]
        pub fn metadata(&self) -> Result<Metadata> {
            // Counterintuitively this is the version of this method which
            // does not traverse symlinks
            self.0.symlink_metadata()
        }

        /// Join a path onto a canonical path, returning a `CanonicalPathBuf`.
        #[inline]
        pub fn join<P: AsRef<Path>>(&self, path: P) -> Result<CanonicalPathBuf> {
            CanonicalPathBuf::new(&self.0.join(path))
        }

        /// Returns an iterator over the entries within a directory.
        ///
        /// The iterator will yield instances of io::Result<DirEntry>. New
        /// errors may be encountered after an iterator is initially
        /// constructed.
        ///
        /// This is an alias to fs::read_dir.
        #[inline]
        pub fn read_dir(&self) -> Result<ReadDir> {
            self.0.read_dir()
        }

        /// Does this path exist?
        #[inline]
        pub fn exists(&self) -> bool {
            self.0.exists()
        }

        /// Is this path a file?
        #[inline]
        pub fn is_file(&self) -> bool {
            self.0.is_file()
        }

        /// Is this path a directory?
        #[inline]
        pub fn is_dir(&self) -> bool {
            self.0.is_file()
        }
    }
}

/// An owned path on the filesystem which is guaranteed to be canonical.
///
/// More specifically: it is at least guaranteed to be canonical at
/// the time it is created. There are potential TOCTTOU problems if the
/// underlying filesystem structure changes after path canonicalization.
#[derive(Clone, Debug, PartialOrd, Ord, PartialEq, Eq, Hash)]
pub struct CanonicalPathBuf(PathBuf);

impl CanonicalPathBuf {
    /// Create a canonical path by first canonicalizing the given path.
    pub fn canonicalize<P>(path: P) -> Result<Self>
    where
        P: AsRef<Path>,
    {
        Ok(CanonicalPathBuf(path.as_ref().canonicalize()?))
    }

    /// Create a canonical path, returning error if the supplied path is not canonical.
    // TODO: rename this to `from_path` or `try_new` to satisfy clippy? (breaking API change)
    #[allow(clippy::new_ret_no_self)]
    pub fn new<P>(path: P) -> Result<Self>
    where
        P: AsRef<Path>,
    {
        let p = path.as_ref();
        let canonical_path_buf = Self::canonicalize(p)?;

        if canonical_path_buf.as_path() != p {
            return Err(Error::new(
                ErrorKind::InvalidInput,
                format!("non-canonical input path: {}", p.display()),
            ));
        }

        Ok(canonical_path_buf)
    }

    /// Return a `CanonicalPath` reference.
    #[inline]
    pub fn as_canonical_path(&self) -> &CanonicalPath {
        unsafe { CanonicalPath::from_path_unchecked(&self.0) }
    }

    /// Updates `self`'s filename ala the same method on `PathBuf`
    pub fn set_file_name<S: AsRef<OsStr>>(&mut self, file_name: S) {
        self.0.set_file_name(file_name);
    }

    /// Updates `self.extension` to extension.
    ///
    /// Returns `false` and does nothing if `self.file_name` is `None`,
    /// returns `true` and updates the extension otherwise.
    /// If `self.extension` is `None`, the extension is added; otherwise it is replaced.
    pub fn set_extension<S: AsRef<OsStr>>(&mut self, extension: S) -> bool {
        self.0.set_extension(extension)
    }

    /// Consumes the `CanonicalPathBuf`, yielding its internal `PathBuf` storage.
    pub fn into_path_buf(self) -> PathBuf {
        self.0
    }

    /// Consumes the `CanonicalPathBuf`, yielding its internal `OsString` storage.
    pub fn into_os_string(self) -> OsString {
        self.0.into_os_string()
    }

    impl_path!();
}

impl AsRef<Path> for CanonicalPathBuf {
    fn as_ref(&self) -> &Path {
        self.as_path()
    }
}

impl AsRef<CanonicalPath> for CanonicalPathBuf {
    fn as_ref(&self) -> &CanonicalPath {
        self.as_canonical_path()
    }
}

impl AsRef<OsStr> for CanonicalPathBuf {
    fn as_ref(&self) -> &OsStr {
        self.as_os_str()
    }
}

impl Borrow<CanonicalPath> for CanonicalPathBuf {
    fn borrow(&self) -> &CanonicalPath {
        self.as_canonical_path()
    }
}

/// A reference type for a canonical filesystem path
///
/// More specifically: it is at least guaranteed to be canonical at
/// the time it is created. There are potential TOCTTOU problems if the
/// underlying filesystem structure changes after path canonicalization.
#[derive(Debug, PartialOrd, Ord, PartialEq, Eq, Hash)]
pub struct CanonicalPath(Path);

impl CanonicalPath {
    /// Create a canonical path, returning error if the supplied path is not canonical
    pub fn new<P>(path: &P) -> Result<&Self>
    where
        P: AsRef<Path> + ?Sized,
    {
        let p = path.as_ref();

        // TODO: non-allocating check that `P` is canonical
        //
        // This seems tricky as realpath(3) is our only real way of checking
        // that a path is canonical. It's also slightly terrifying in that,
        // at least in glibc, it is over 200 lines long and full of complex
        // logic and error handling:
        //
        // https://sourceware.org/git/?p=glibc.git;a=blob;f=stdlib/canonicalize.c;hb=HEAD
        if p != p.canonicalize()? {
            return Err(Error::new(
                ErrorKind::InvalidInput,
                format!("non-canonical input path: {}", p.display()),
            ));
        }

        Ok(unsafe { Self::from_path_unchecked(p) })
    }

    /// Create a canonical path from a path, skipping the canonicalization check
    ///
    /// This uses the same unsafe reference conversion tricks as `std` to
    /// convert from `AsRef<Path>` to `AsRef<CanonicalPath>`, i.e. `&CanonicalPath`
    /// is a newtype for `&Path` in the same way `&Path` is a newtype for `&OsStr`.
    pub unsafe fn from_path_unchecked<P>(path: &P) -> &Self
    where
        P: AsRef<Path> + ?Sized,
    {
        &*(path.as_ref() as *const Path as *const CanonicalPath)
    }

    /// Convert a canonical path reference into an owned `CanonicalPathBuf`
    pub fn to_canonical_path_buf(&self) -> CanonicalPathBuf {
        CanonicalPathBuf(self.0.to_owned())
    }

    impl_path!();
}

impl AsRef<Path> for CanonicalPath {
    fn as_ref(&self) -> &Path {
        &self.0
    }
}

impl ToOwned for CanonicalPath {
    type Owned = CanonicalPathBuf;

    fn to_owned(&self) -> CanonicalPathBuf {
        self.to_canonical_path_buf()
    }
}

/// Returns the full, canonicalized filesystem path of the current running
/// executable.
pub fn current_exe() -> Result<CanonicalPathBuf> {
    let p = env::current_exe()?;
    Ok(CanonicalPathBuf::canonicalize(p)?)
}

// TODO: test on Windows
#[cfg(all(test, not(windows)))]
mod tests {
    use std::fs::File;
    use std::os::unix::fs;
    use std::path::PathBuf;

    use super::{CanonicalPath, CanonicalPathBuf};
    use tempfile::TempDir;

    // We create a test file with this name
    const CANONICAL_FILENAME: &str = "canonical-file";

    // We create a symlink to "canonical-file" with this name
    const NON_CANONICAL_FILENAME: &str = "non-canonical-file";

    /// A directory full of test fixtures
    struct TestFixtureDir {
        /// The temporary directory itself (i.e. root directory of our tests)
        pub tempdir: TempDir,

        /// Canonical path to the test directory
        pub base_path: PathBuf,

        /// Path to a canonical file in our test fixture directory
        pub canonical_path: PathBuf,

        /// Path to a symlink in our test fixture directory
        pub symlink_path: PathBuf,
    }

    impl TestFixtureDir {
        pub fn new() -> Self {
            let tempdir = TempDir::new().unwrap();
            let base_path = tempdir.path().canonicalize().unwrap();

            let canonical_path = base_path.join(CANONICAL_FILENAME);
            File::create(&canonical_path).unwrap();

            let symlink_path = base_path.join(NON_CANONICAL_FILENAME);
            fs::symlink(&canonical_path, &symlink_path).unwrap();

            Self {
                tempdir,
                base_path,
                canonical_path,
                symlink_path,
            }
        }
    }

    #[test]
    fn create_canonical_path() {
        let test_fixtures = TestFixtureDir::new();
        let canonical_path = CanonicalPath::new(&test_fixtures.canonical_path).unwrap();
        assert_eq!(
            canonical_path.as_path(),
            test_fixtures.canonical_path.as_path()
        );
    }

    #[test]
    fn create_canonical_path_buf() {
        let test_fixtures = TestFixtureDir::new();
        let canonical_path_buf = CanonicalPathBuf::new(&test_fixtures.canonical_path).unwrap();
        assert_eq!(
            canonical_path_buf.as_path(),
            test_fixtures.canonical_path.as_path()
        );
    }

    #[test]
    fn reject_canonical_path_symlinks() {
        let test_fixtures = TestFixtureDir::new();
        let result = CanonicalPath::new(&test_fixtures.symlink_path);
        assert!(result.is_err(), "symlinks aren't canonical paths!");
    }

    #[test]
    fn reject_canonical_path_buf_symlinks() {
        let test_fixtures = TestFixtureDir::new();
        let result = CanonicalPathBuf::new(&test_fixtures.symlink_path);
        assert!(result.is_err(), "symlinks aren't canonical paths!");
    }
}
