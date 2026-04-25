use std::os::unix::fs::MetadataExt;
use std::os::unix::io::AsRawFd;
use async_std::fs::File;

lock_impl!(File);
allocate!(File);
allocate_size!(File);

#[cfg(test)]
mod test {
    extern crate tempdir;

    use async_std::fs;

    use crate::{lock_contended_error, async_std::AsyncFileExt};

    /// Tests that locking a file descriptor will replace any existing locks
    /// held on the file descriptor.
    #[async_std::test]
    async fn lock_replace() {
        let tempdir = tempdir::TempDir::new("fs4").unwrap();
        let path = tempdir.path().join("fs4");
        let file1 = fs::OpenOptions::new().write(true).create(true).open(&path).await.unwrap();
        let file2 = fs::OpenOptions::new().write(true).create(true).open(&path).await.unwrap();

        // Creating a shared lock will drop an exclusive lock.
        file1.lock_exclusive().unwrap();
        file1.lock_shared().unwrap();
        file2.lock_shared().unwrap();

        // Attempting to replace a shared lock with an exclusive lock will fail
        // with multiple lock holders, and remove the original shared lock.
        assert_eq!(file2.try_lock_exclusive().unwrap_err().raw_os_error(),
                   lock_contended_error().raw_os_error());
        file1.lock_shared().unwrap();
    }
}