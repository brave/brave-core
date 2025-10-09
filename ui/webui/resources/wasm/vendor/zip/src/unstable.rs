#![allow(missing_docs)]

use std::io;
use std::io::{Read, Write};

/// Provides high level API for reading from a stream.
pub mod stream {
    pub use crate::read::stream::*;
}
/// Types for creating ZIP archives.
pub mod write {
    use crate::write::{FileOptionExtension, FileOptions};
    /// Unstable methods for [`FileOptions`].
    pub trait FileOptionsExt {
        /// Write the file with the given password using the deprecated ZipCrypto algorithm.
        ///
        /// This is not recommended for new archives, as ZipCrypto is not secure.
        fn with_deprecated_encryption(self, password: &[u8]) -> Self;
    }
    impl<T: FileOptionExtension> FileOptionsExt for FileOptions<T> {
        fn with_deprecated_encryption(self, password: &[u8]) -> Self {
            self.with_deprecated_encryption(password)
        }
    }
}

/// Helper methods for writing unsigned integers in little-endian form.
pub trait LittleEndianWriteExt: Write {
    fn write_u16_le(&mut self, input: u16) -> io::Result<()> {
        self.write_all(&input.to_le_bytes())
    }

    fn write_u32_le(&mut self, input: u32) -> io::Result<()> {
        self.write_all(&input.to_le_bytes())
    }

    fn write_u64_le(&mut self, input: u64) -> io::Result<()> {
        self.write_all(&input.to_le_bytes())
    }

    fn write_u128_le(&mut self, input: u128) -> io::Result<()> {
        self.write_all(&input.to_le_bytes())
    }
}

impl<W: Write> LittleEndianWriteExt for W {}

/// Helper methods for reading unsigned integers in little-endian form.
pub trait LittleEndianReadExt: Read {
    fn read_u16_le(&mut self) -> io::Result<u16> {
        let mut out = [0u8; 2];
        self.read_exact(&mut out)?;
        Ok(u16::from_le_bytes(out))
    }

    fn read_u32_le(&mut self) -> io::Result<u32> {
        let mut out = [0u8; 4];
        self.read_exact(&mut out)?;
        Ok(u32::from_le_bytes(out))
    }

    fn read_u64_le(&mut self) -> io::Result<u64> {
        let mut out = [0u8; 8];
        self.read_exact(&mut out)?;
        Ok(u64::from_le_bytes(out))
    }
}

impl<R: Read> LittleEndianReadExt for R {}
