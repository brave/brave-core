use std::io::{Error, Result};
use std::mem;
use std::os::windows::io::AsRawHandle;

use windows_sys::Win32::Foundation::HANDLE;
use windows_sys::Win32::Storage::FileSystem::{
    FileAllocationInfo, FileStandardInfo, GetFileInformationByHandleEx,
    SetFileInformationByHandle, UnlockFile, FILE_ALLOCATION_INFO,
    FILE_STANDARD_INFO, LOCKFILE_EXCLUSIVE_LOCK, LOCKFILE_FAIL_IMMEDIATELY, LockFileEx
};

use tokio::fs::File;

lock_impl!(File);
allocate!(File);
allocate_size!(File);

#[cfg(test)]
mod test {

    extern crate tempdir;

    use tokio::fs;

    use crate::{lock_contended_error, tokio::AsyncFileExt};

    /// A file handle may not be exclusively locked multiple times, or exclusively locked and then
    /// shared locked.
    #[tokio::test]
    async fn lock_non_reentrant() {
        let tempdir = tempdir::TempDir::new("fs4").unwrap();
        let path = tempdir.path().join("fs4");
        let file = fs::OpenOptions::new().read(true).write(true).create(true).open(&path).await.unwrap();

        // Multiple exclusive locks fails.
        file.lock_exclusive().unwrap();
        assert_eq!(file.try_lock_exclusive().unwrap_err().raw_os_error(),
                   lock_contended_error().raw_os_error());
        file.unlock().unwrap();

        // Shared then Exclusive locks fails.
        file.lock_shared().unwrap();
        assert_eq!(file.try_lock_exclusive().unwrap_err().raw_os_error(),
                   lock_contended_error().raw_os_error());
    }

    /// A file handle can hold an exclusive lock and any number of shared locks, all of which must
    /// be unlocked independently.
    #[tokio::test]
    async fn lock_layering() {
        let tempdir = tempdir::TempDir::new("fs4").unwrap();
        let path = tempdir.path().join("fs4");
        let file = fs::OpenOptions::new().read(true).write(true).create(true).open(&path).await.unwrap();

        // Open two shared locks on the file, and then try and fail to open an exclusive lock.
        file.lock_exclusive().unwrap();
        file.lock_shared().unwrap();
        file.lock_shared().unwrap();
        assert_eq!(file.try_lock_exclusive().unwrap_err().raw_os_error(),
                   lock_contended_error().raw_os_error());

        // Pop one of the shared locks and try again.
        file.unlock().unwrap();
        assert_eq!(file.try_lock_exclusive().unwrap_err().raw_os_error(),
                   lock_contended_error().raw_os_error());

        // Pop the second shared lock and try again.
        file.unlock().unwrap();
        assert_eq!(file.try_lock_exclusive().unwrap_err().raw_os_error(),
                   lock_contended_error().raw_os_error());

        // Pop the exclusive lock and finally succeed.
        file.unlock().unwrap();
        file.lock_exclusive().unwrap();
    }

    /// A file handle with multiple open locks will have all locks closed on drop.
    #[tokio::test]
    async fn lock_layering_cleanup() {
        let tempdir = tempdir::TempDir::new("fs4").unwrap();
        let path = tempdir.path().join("fs4");
        let file1 = fs::OpenOptions::new().read(true).write(true).create(true).open(&path).await.unwrap();
        let file2 = fs::OpenOptions::new().read(true).write(true).create(true).open(&path).await.unwrap();

        // Open two shared locks on the file, and then try and fail to open an exclusive lock.
        file1.lock_shared().unwrap();
        assert_eq!(file2.try_lock_exclusive().unwrap_err().raw_os_error(),
                   lock_contended_error().raw_os_error());

        drop(file1);
        file2.lock_exclusive().unwrap();
    }
}