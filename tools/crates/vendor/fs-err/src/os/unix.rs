/// Unix-specific extensions to wrappers in fs-err for [`std::fs`] types.
pub mod fs {
    use std::io;
    use std::path::Path;

    #[allow(unused_imports)]
    use crate::{Error, ErrorKind};
    use crate::{SourceDestError, SourceDestErrorKind};

    /// Creates a new symbolic link on the filesystem.
    ///
    /// The `link` path will be a symbolic link pointing to the `original` path.
    ///
    /// Wrapper for [`std::os::unix::fs::symlink`].
    pub fn symlink<P: AsRef<Path>, Q: AsRef<Path>>(original: P, link: Q) -> io::Result<()> {
        let original = original.as_ref();
        let link = link.as_ref();
        std::os::unix::fs::symlink(original, link).map_err(|err| {
            SourceDestError::build(err, SourceDestErrorKind::Symlink, link, original)
        })
    }

    /// Change the owner and group of the specified path.
    ///
    /// Specifying either the uid or gid as `None` will leave it unchanged.
    ///
    /// Wrapper for [`std::os::unix::fs::chown`]
    #[cfg(rustc_1_73)]
    pub fn chown<P: AsRef<Path>>(path: P, uid: Option<u32>, gid: Option<u32>) -> io::Result<()> {
        let path = path.as_ref();
        std::os::unix::fs::chown(path, uid, gid)
            .map_err(|err| Error::build(err, ErrorKind::Chown, path))
    }

    /// Change the owner and group of the specified path, without dereferencing symbolic links.
    ///
    /// Identical to [`chown`], except that if called on a symbolic link, this will change the owner
    /// and group of the link itself rather than the owner and group of the link target.
    ///
    /// Wrapper for [`std::os::unix::fs::lchown`]
    #[cfg(rustc_1_73)]
    pub fn lchown<P: AsRef<Path>>(path: P, uid: Option<u32>, gid: Option<u32>) -> io::Result<()> {
        let path = path.as_ref();
        std::os::unix::fs::lchown(path, uid, gid)
            .map_err(|err| Error::build(err, ErrorKind::Lchown, path))
    }

    /// Change the root directory of the current process to the specified path.
    ///
    /// This typically requires privileges, such as root or a specific capability.
    ///
    /// Wrapper for [`std::os::unix::fs::chroot`]
    #[cfg(rustc_1_56)]
    pub fn chroot<P: AsRef<Path>>(path: P) -> io::Result<()> {
        let path = path.as_ref();
        std::os::unix::fs::chroot(path).map_err(|err| Error::build(err, ErrorKind::Chroot, path))
    }

    /// Wrapper for [`std::os::unix::fs::FileExt`].
    ///
    /// The std traits might be extended in the future (See issue [#49961](https://github.com/rust-lang/rust/issues/49961#issuecomment-382751777)).
    /// This trait is sealed and can not be implemented by other crates.
    pub trait FileExt: crate::Sealed {
        /// Wrapper for [`std::os::unix::fs::FileExt::read_at`].
        fn read_at(&self, buf: &mut [u8], offset: u64) -> io::Result<usize>;
        /// Wrapper for [`std::os::unix::fs::FileExt::read_exact_at`].
        fn read_exact_at(&self, buf: &mut [u8], offset: u64) -> io::Result<()>;
        /// Wrapper for [`std::os::unix::fs::FileExt::write_at`].
        fn write_at(&self, buf: &[u8], offset: u64) -> io::Result<usize>;
        /// Wrapper for [`std::os::unix::fs::FileExt::write_all_at`].
        fn write_all_at(&self, buf: &[u8], offset: u64) -> io::Result<()>;
    }

    /// Wrapper for [`std::os::unix::fs::OpenOptionsExt`].
    ///
    /// The std traits might be extended in the future (See issue [#49961](https://github.com/rust-lang/rust/issues/49961#issuecomment-382751777)).
    /// This trait is sealed and can not be implemented by other crates.
    pub trait OpenOptionsExt: crate::Sealed {
        /// Wrapper for [`std::os::unix::fs::OpenOptionsExt::mode`].
        fn mode(&mut self, mode: u32) -> &mut Self;
        /// Wrapper for [`std::os::unix::fs::OpenOptionsExt::custom_flags`].
        fn custom_flags(&mut self, flags: i32) -> &mut Self;
    }
}
