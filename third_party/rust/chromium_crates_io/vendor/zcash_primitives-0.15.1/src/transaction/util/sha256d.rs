use sha2::{digest::Output, Digest, Sha256};
use std::io::{self, Read, Write};

/// Abstraction over a reader which SHA-256d-hashes the data being read.
pub struct HashReader<R: Read> {
    reader: R,
    hasher: Sha256,
}

impl<R: Read> HashReader<R> {
    /// Construct a new `HashReader` given an existing `reader` by value.
    pub fn new(reader: R) -> Self {
        HashReader {
            reader,
            hasher: Sha256::new(),
        }
    }

    pub fn into_base_reader(self) -> R {
        self.reader
    }

    /// Destroy this reader and return the hash of what was read.
    pub fn into_hash(self) -> Output<Sha256> {
        Sha256::digest(self.hasher.finalize())
    }
}

impl<R: Read> Read for HashReader<R> {
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        let bytes = self.reader.read(buf)?;

        if bytes > 0 {
            self.hasher.update(&buf[0..bytes]);
        }

        Ok(bytes)
    }
}

/// Abstraction over a writer which SHA-256d-hashes the data being read.
pub struct HashWriter {
    hasher: Sha256,
}

impl Default for HashWriter {
    fn default() -> Self {
        HashWriter {
            hasher: Sha256::new(),
        }
    }
}

impl HashWriter {
    /// Destroy this writer and return the hash of what was written.
    pub fn into_hash(self) -> Output<Sha256> {
        Sha256::digest(self.hasher.finalize())
    }
}

impl Write for HashWriter {
    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        self.hasher.update(buf);

        Ok(buf.len())
    }

    fn flush(&mut self) -> io::Result<()> {
        Ok(())
    }
}
