/// Windows-specific extensions to wrappers in fs-err for [`std::fs`] types.
pub mod fs {
    use crate::{SourceDestError, SourceDestErrorKind};
    use std::io;
    use std::path::Path;

    /// Creates a new symlink to a directory on the filesystem.
    ///
    /// The `link` path will be a symbolic link pointing to the `original` path.
    ///
    /// Wrapper for [`std::os::windows::fs::symlink_dir`].
    pub fn symlink_dir<P: AsRef<Path>, Q: AsRef<Path>>(original: P, link: Q) -> io::Result<()> {
        let original = original.as_ref();
        let link = link.as_ref();
        std::os::windows::fs::symlink_dir(original, link).map_err(|err| {
            SourceDestError::build(err, SourceDestErrorKind::SymlinkDir, link, original)
        })
    }

    /// Creates a new symlink to a non-directory file on the filesystem.
    ///
    /// The `link` path will be a symbolic link pointing to the `original` path.
    ///
    /// Wrapper for [`std::os::windows::fs::symlink_file`].
    pub fn symlink_file<P: AsRef<Path>, Q: AsRef<Path>>(original: P, link: Q) -> io::Result<()> {
        let original = original.as_ref();
        let link = link.as_ref();
        std::os::windows::fs::symlink_file(original, link).map_err(|err| {
            SourceDestError::build(err, SourceDestErrorKind::SymlinkFile, link, original)
        })
    }

    /// Wrapper for [`std::os::windows::fs::FileExt`].
    ///
    /// The std traits might be extended in the future (See issue [#49961](https://github.com/rust-lang/rust/issues/49961#issuecomment-382751777)).
    /// This trait is sealed and can not be implemented by other crates.
    pub trait FileExt: crate::Sealed {
        /// Wrapper for [`std::os::windows::fs::FileExt::seek_read`].
        fn seek_read(&self, buf: &mut [u8], offset: u64) -> io::Result<usize>;
        /// Wrapper for [`std::os::windows::fs::FileExt::seek_write`].
        fn seek_write(&self, buf: &[u8], offset: u64) -> io::Result<usize>;
    }

    /// Wrapper for [`std::os::windows::fs::OpenOptionsExt`].
    ///
    /// The std traits might be extended in the future (See issue [#49961](https://github.com/rust-lang/rust/issues/49961#issuecomment-382751777)).
    /// This trait is sealed and can not be implemented by other crates.
    pub trait OpenOptionsExt: crate::Sealed {
        /// Wrapper for [`std::os::windows::fs::OpenOptionsExt::access_mode`].
        fn access_mode(&mut self, access: u32) -> &mut Self;
        /// Wrapper for [`std::os::windows::fs::OpenOptionsExt::share_mode`].
        fn share_mode(&mut self, val: u32) -> &mut Self;
        /// Wrapper for [`std::os::windows::fs::OpenOptionsExt::custom_flags`].
        fn custom_flags(&mut self, flags: u32) -> &mut Self;
        /// Wrapper for [`std::os::windows::fs::OpenOptionsExt::attributes`].
        fn attributes(&mut self, val: u32) -> &mut Self;
        /// Wrapper for [`std::os::windows::fs::OpenOptionsExt::security_qos_flags`].
        fn security_qos_flags(&mut self, flags: u32) -> &mut Self;
    }
}
