use tokio::fs::File;
#[cfg(unix)]
use crate::unix::async_impl::tokio_impl as sys;
#[cfg(windows)]
use crate::windows::async_impl::tokio_impl as sys;

async_file_ext!(File, "tokio::fs::File");

#[cfg(test)]
mod test {

    extern crate tempdir;
    extern crate test;

    use tokio::fs;
    use crate::{allocation_granularity, available_space, tokio::AsyncFileExt, free_space, lock_contended_error, total_space}; 

    /// Tests shared file lock operations.
    #[tokio::test]
    async fn lock_shared() {
        let tempdir = tempdir::TempDir::new("fs4").unwrap();
        let path = tempdir.path().join("fs4");
        let file1 = fs::OpenOptions::new().read(true).write(true).create(true).open(&path).await.unwrap();
        let file2 = fs::OpenOptions::new().read(true).write(true).create(true).open(&path).await.unwrap();
        let file3 = fs::OpenOptions::new().read(true).write(true).create(true).open(&path).await.unwrap();

        // Concurrent shared access is OK, but not shared and exclusive.
        file1.lock_shared().unwrap();
        file2.lock_shared().unwrap();
        assert_eq!(file3.try_lock_exclusive().unwrap_err().kind(),
                   lock_contended_error().kind());
        file1.unlock().unwrap();
        assert_eq!(file3.try_lock_exclusive().unwrap_err().kind(),
                   lock_contended_error().kind());

        // Once all shared file locks are dropped, an exclusive lock may be created;
        file2.unlock().unwrap();
        file3.lock_exclusive().unwrap();
    }

    /// Tests exclusive file lock operations.
    #[tokio::test]
    async fn lock_exclusive() {
        let tempdir = tempdir::TempDir::new("fs4").unwrap();
        let path = tempdir.path().join("fs4");
        let file1 = fs::OpenOptions::new().read(true).write(true).create(true).open(&path).await.unwrap();
        let file2 = fs::OpenOptions::new().read(true).write(true).create(true).open(&path).await.unwrap();

        // No other access is possible once an exclusive lock is created.
        file1.lock_exclusive().unwrap();
        assert_eq!(file2.try_lock_exclusive().unwrap_err().kind(),
                   lock_contended_error().kind());
        assert_eq!(file2.try_lock_shared().unwrap_err().kind(),
                   lock_contended_error().kind());

        // Once the exclusive lock is dropped, the second file is able to create a lock.
        file1.unlock().unwrap();
        file2.lock_exclusive().unwrap();
    }

    /// Tests that a lock is released after the file that owns it is dropped.
    #[tokio::test]
    async fn lock_cleanup() {
        let tempdir = tempdir::TempDir::new("fs4").unwrap();
        let path = tempdir.path().join("fs4");
        let file1 = fs::OpenOptions::new().read(true).write(true).create(true).open(&path).await.unwrap();
        let file2 = fs::OpenOptions::new().read(true).write(true).create(true).open(&path).await.unwrap();

        file1.lock_exclusive().unwrap();
        assert_eq!(file2.try_lock_shared().unwrap_err().kind(),
                   lock_contended_error().kind());

        // Drop file1; the lock should be released.
        drop(file1);
        file2.lock_shared().unwrap();
    }

    /// Tests file allocation.
    #[tokio::test]
    async fn allocate() {
        let tempdir = tempdir::TempDir::new("fs4").unwrap();
        let path = tempdir.path().join("fs4");
        let file = fs::OpenOptions::new().write(true).create(true).open(&path).await.unwrap();
        let blksize = allocation_granularity(&path).unwrap();

        // New files are created with no allocated size.
        assert_eq!(0, file.allocated_size().await.unwrap());
        assert_eq!(0, file.metadata().await.unwrap().len());

        // Allocate space for the file, checking that the allocated size steps
        // up by block size, and the file length matches the allocated size.

        file.allocate(2 * blksize - 1).await.unwrap();
        assert_eq!(2 * blksize, file.allocated_size().await.unwrap());
        assert_eq!(2 * blksize - 1, file.metadata().await.unwrap().len());

        // Truncate the file, checking that the allocated size steps down by
        // block size.

        file.set_len(blksize + 1).await.unwrap();
        assert_eq!(2 * blksize, file.allocated_size().await.unwrap());
        assert_eq!(blksize + 1, file.metadata().await.unwrap().len());
    }

    /// Checks filesystem space methods.
    #[tokio::test]
    async fn filesystem_space() {
        let tempdir = tempdir::TempDir::new("fs4").unwrap();
        let total_space = total_space(&tempdir.path()).unwrap();
        let free_space = free_space(&tempdir.path()).unwrap();
        let available_space = available_space(&tempdir.path()).unwrap();

        assert!(total_space > free_space);
        assert!(total_space > available_space);
        assert!(available_space <= free_space);
    }
}