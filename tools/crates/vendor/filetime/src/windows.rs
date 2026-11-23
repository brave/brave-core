use crate::FileTime;
use std::fs::{self, OpenOptions};
use std::io;
use std::os::windows::fs::OpenOptionsExt;
use std::path::Path;

const FILE_FLAG_BACKUP_SEMANTICS: u32 = 0x2000000;
const FILE_FLAG_OPEN_REPARSE_POINT: u32 = 0x200000;

pub fn open(p: &Path) -> io::Result<fs::File> {
    OpenOptions::new()
        .write(true)
        .custom_flags(FILE_FLAG_BACKUP_SEMANTICS)
        .open(p)
}

pub fn set_symlink_file_times(p: &Path, atime: FileTime, mtime: FileTime) -> io::Result<()> {
    let f = OpenOptions::new()
        .write(true)
        .custom_flags(FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS)
        .open(p)?;
    crate::set_file_handle_times(&f, Some(atime), Some(mtime))
}
