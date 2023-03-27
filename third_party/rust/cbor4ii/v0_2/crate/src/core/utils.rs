#[cfg(feature = "use_alloc")]
use crate::alloc::vec::Vec;
#[cfg(feature = "use_alloc")]
use crate::core::enc;
use crate::core::dec;

/// An in-memory writer.
#[cfg(feature = "use_alloc")]
pub struct BufWriter(Vec<u8>);

#[cfg(feature = "use_alloc")]
impl BufWriter {
    /// Creates a new writer.
    pub fn new(buf: Vec<u8>) -> Self {
       BufWriter(buf)
    }

    /// Returns a reference to the underlying data.
    pub fn buffer(&self) -> &[u8] {
        &self.0
    }

    /// Returns the underlying vector.
    pub fn into_inner(self) -> Vec<u8> {
        self.0
    }

    /// Discards the underlying data.
    pub fn clear(&mut self) {
        self.0.clear();
    }
}

#[cfg(feature = "use_alloc")]
impl enc::Write for BufWriter {
    type Error = crate::alloc::collections::TryReserveError;

    #[inline]
    fn push(&mut self, input: &[u8]) -> Result<(), Self::Error> {
        self.0.try_reserve(input.len())?;
        self.0.extend_from_slice(input);
        Ok(())
    }
}

/// An in-memory reader.
pub struct SliceReader<'a> {
    buf: &'a [u8],
    limit: usize
}

impl SliceReader<'_> {
    pub fn new(buf: &[u8]) -> SliceReader<'_> {
        SliceReader { buf, limit: 256 }
    }
}

impl<'de> dec::Read<'de> for SliceReader<'de> {
    type Error = core::convert::Infallible;

    #[inline]
    fn fill<'b>(&'b mut self, want: usize) -> Result<dec::Reference<'de, 'b>, Self::Error> {
        let len = core::cmp::min(self.buf.len(), want);
        Ok(dec::Reference::Long(&self.buf[..len]))
    }

    #[inline]
    fn advance(&mut self, n: usize) {
        let len = core::cmp::min(self.buf.len(), n);
        self.buf = &self.buf[len..];
    }

    #[inline]
    fn step_in(&mut self) -> bool {
        if let Some(limit) = self.limit.checked_sub(1) {
            self.limit = limit;
            true
        } else {
            false
        }
    }

    #[inline]
    fn step_out(&mut self) {
        self.limit += 1;
    }
}

/// A writer to work with [`std::io::Write`].
#[cfg(feature = "use_std")]
pub struct IoWriter<W>(W);

#[cfg(feature = "use_std")]
impl<W> IoWriter<W> {
    pub fn new(writer: W) -> Self {
        IoWriter(writer)
    }

    pub fn into_inner(self) -> W {
        self.0
    }
}

#[cfg(feature = "use_std")]
impl<W: std::io::Write> enc::Write for IoWriter<W> {
    type Error = std::io::Error;

    #[inline]
    fn push(&mut self, input: &[u8]) -> Result<(), Self::Error> {
        self.0.write_all(input)
    }
}


/// A reader to work with [`std::io::BufRead`].
///
/// It has a recursion limit.
#[cfg(feature = "use_std")]
pub struct IoReader<R> {
    reader: R,
    limit: usize,
}

#[cfg(feature = "use_std")]
impl<R> IoReader<R> {
    pub fn new(reader: R) -> Self {
        Self { reader, limit: 256 }
    }

    pub fn into_inner(self) -> R {
        self.reader
    }
}

#[cfg(feature = "use_std")]
impl<'de, R: std::io::BufRead> dec::Read<'de> for IoReader<R> {
    type Error = std::io::Error;

    #[inline]
    fn fill<'b>(&'b mut self, _want: usize) -> Result<dec::Reference<'de, 'b>, Self::Error> {
        let buf = self.reader.fill_buf()?;
        Ok(dec::Reference::Short(buf))
    }

    #[inline]
    fn advance(&mut self, n: usize) {
        self.reader.consume(n);
    }

    #[inline]
    fn step_in(&mut self) -> bool {
        if let Some(limit) = self.limit.checked_sub(1) {
            self.limit = limit;
            true
        } else {
            false
        }
    }

    #[inline]
    fn step_out(&mut self) {
        self.limit += 1;
    }
}
