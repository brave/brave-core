//! Filesystem manipulation operations.

use std::fs::File;
use std::io::{Result, Write};
use std::path::Path;

use crate::Close;

/// Write a slice as the entire contents of a file.
///
/// This function is identical to [`std::fs::write`] but uses [`Close`]
/// to drop the [`File`](std::fs::File) created from path, returning any
/// errors encountered when doing so.
pub fn write<P: AsRef<Path>, C: AsRef<[u8]>>(
    path: P,
    contents: C,
) -> Result<()> {
    fn inner(path: &Path, contents: &[u8]) -> Result<()> {
        let mut f = File::create(path)?;
        f.write_all(contents)?;
        f.close()
    }
    inner(path.as_ref(), contents.as_ref())
}
