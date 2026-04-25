macro_rules! async_file_ext {
    ($file: ty, $file_name: literal) => {
        use std::io::Result;

        #[doc = concat!("Extension trait for `", $file_name, "` which provides allocation, duplication and locking methods.")]
        ///
        /// ## Notes on File Locks
        ///
        /// This library provides whole-file locks in both shared (read) and exclusive
        /// (read-write) varieties.
        ///
        /// File locks are a cross-platform hazard since the file lock APIs exposed by
        /// operating system kernels vary in subtle and not-so-subtle ways.
        ///
        /// The API exposed by this library can be safely used across platforms as long
        /// as the following rules are followed:
        ///
        ///   * Multiple locks should not be created on an individual `File` instance
        ///     concurrently.
        ///   * Duplicated files should not be locked without great care.
        ///   * Files to be locked should be opened with at least read or write
        ///     permissions.
        ///   * File locks may only be relied upon to be advisory.
        ///
        /// See the tests in `lib.rs` for cross-platform lock behavior that may be
        /// relied upon; see the tests in `unix.rs` and `windows.rs` for examples of
        /// platform-specific behavior. File locks are implemented with
        /// [`flock(2)`](http://man7.org/linux/man-pages/man2/flock.2.html) on Unix and
        /// [`LockFile`](https://msdn.microsoft.com/en-us/library/windows/desktop/aa365202(v=vs.85).aspx)
        /// on Windows.
        #[async_trait::async_trait]
        pub trait AsyncFileExt {

            /// Returns the amount of physical space allocated for a file.
            async fn allocated_size(&self) -> Result<u64>;

            /// Ensures that at least `len` bytes of disk space are allocated for the
            /// file, and the file size is at least `len` bytes. After a successful call
            /// to `allocate`, subsequent writes to the file within the specified length
            /// are guaranteed not to fail because of lack of disk space.
            async fn allocate(&self, len: u64) -> Result<()>;

            /// Locks the file for shared usage, blocking if the file is currently
            /// locked exclusively.
            fn lock_shared(&self) -> Result<()>;

            /// Locks the file for exclusive usage, blocking if the file is currently
            /// locked.
            fn lock_exclusive(&self) -> Result<()>;

            /// Locks the file for shared usage, or returns a an error if the file is
            /// currently locked (see `lock_contended_error`).
            fn try_lock_shared(&self) -> Result<()>;

            /// Locks the file for shared usage, or returns a an error if the file is
            /// currently locked (see `lock_contended_error`).
            fn try_lock_exclusive(&self) -> Result<()>;

            /// Unlocks the file.
            fn unlock(&self) -> Result<()>;
        }

        #[async_trait::async_trait]
        impl AsyncFileExt for $file {
            async fn allocated_size(&self) -> Result<u64> {
                sys::allocated_size(self).await
            }
            async fn allocate(&self, len: u64) -> Result<()> {
                sys::allocate(self, len).await
            }
            fn lock_shared(&self) -> Result<()> {
                sys::lock_shared(self)
            }
            fn lock_exclusive(&self) -> Result<()> {
                sys::lock_exclusive(self)
            }
            fn try_lock_shared(&self) -> Result<()> {
                sys::try_lock_shared(self)
            }
            fn try_lock_exclusive(&self) -> Result<()> {
                sys::try_lock_exclusive(self)
            }
            fn unlock(&self) -> Result<()> {
                sys::unlock(self)
            }
        }
    }
}

cfg_async_std! {
    pub(crate) mod async_std_impl;
}

cfg_smol! {
    pub(crate) mod smol_impl;
}

cfg_tokio! {
    pub(crate) mod tokio_impl;
}

