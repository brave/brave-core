//! A `Cursor` wraps an in-memory buffer and provides `Read`/`Write` access
//! with a tracked position, mirroring [`std::io::Cursor`] for `no_std`
//! environments.

use core::cmp;

use super::{Error, ErrorKind, Read, Result, Write};

/// A `Cursor` wraps an in-memory buffer and provides it with a [`Read`]
/// and/or [`Write`] implementation.
///
/// This is a `no_std` equivalent of [`std::io::Cursor`].
#[derive(Clone, Debug, Default, Eq, PartialEq)]
pub struct Cursor<T> {
    inner: T,
    pos: u64,
}

impl<T> Cursor<T> {
    /// Creates a new cursor wrapping the provided underlying in-memory buffer.
    ///
    /// The initial position is `0` — reads and writes will start at the
    /// beginning of the buffer.
    pub fn new(inner: T) -> Self {
        Cursor { inner, pos: 0 }
    }

    /// Consumes the cursor, returning the underlying value.
    pub fn into_inner(self) -> T {
        self.inner
    }

    /// Returns a reference to the underlying value.
    pub fn get_ref(&self) -> &T {
        &self.inner
    }

    /// Returns a mutable reference to the underlying value.
    pub fn get_mut(&mut self) -> &mut T {
        &mut self.inner
    }

    /// Returns the current position of the cursor.
    pub fn position(&self) -> u64 {
        self.pos
    }

    /// Sets the position of the cursor.
    pub fn set_position(&mut self, pos: u64) {
        self.pos = pos;
    }
}

// ---------------------------------------------------------------------------
// Read
// ---------------------------------------------------------------------------

impl<T: AsRef<[u8]>> Read for Cursor<T> {
    fn read(&mut self, buf: &mut [u8]) -> Result<usize> {
        let slice = self.inner.as_ref();
        let start = cmp::min(self.pos, slice.len() as u64) as usize;
        let remaining = &slice[start..];
        let amt = cmp::min(buf.len(), remaining.len());
        buf[..amt].copy_from_slice(&remaining[..amt]);
        self.pos += amt as u64;
        Ok(amt)
    }

    fn read_exact(&mut self, buf: &mut [u8]) -> Result<()> {
        let slice = self.inner.as_ref();
        let start = cmp::min(self.pos, slice.len() as u64) as usize;
        let remaining = &slice[start..];
        if buf.len() > remaining.len() {
            return Err(Error::from(ErrorKind::UnexpectedEof));
        }
        buf.copy_from_slice(&remaining[..buf.len()]);
        self.pos += buf.len() as u64;
        Ok(())
    }
}

// ---------------------------------------------------------------------------
// Write — &mut [u8]
// ---------------------------------------------------------------------------

fn slice_write(pos: &mut u64, slice: &mut [u8], buf: &[u8]) -> Result<usize> {
    let start = cmp::min(*pos, slice.len() as u64) as usize;
    let amt = cmp::min(buf.len(), slice.len() - start);
    slice[start..start + amt].copy_from_slice(&buf[..amt]);
    *pos += amt as u64;
    Ok(amt)
}

impl Write for Cursor<&mut [u8]> {
    fn write(&mut self, buf: &[u8]) -> Result<usize> {
        slice_write(&mut self.pos, self.inner, buf)
    }

    fn flush(&mut self) -> Result<()> {
        Ok(())
    }
}

// ---------------------------------------------------------------------------
// Write — Vec<u8> and &mut Vec<u8>
// ---------------------------------------------------------------------------

#[cfg(feature = "alloc")]
fn vec_write(pos: &mut u64, vec: &mut alloc::vec::Vec<u8>, buf: &[u8]) -> Result<usize> {
    let start: usize = (*pos).try_into().map_err(|_| {
        Error::new_static(
            ErrorKind::InvalidInput,
            "cursor position exceeds maximum possible vector length",
        )
    })?;
    // Fill any gap between the current length and the write position with
    // zeros.  Only the gap is resized here — the write payload is handled
    // below via copy_from_slice (overwrite) and extend_from_slice (append)
    // so that we avoid redundantly zero-initializing bytes that will be
    // immediately overwritten.
    if start > vec.len() {
        vec.resize(start, 0);
    }
    let overlap = cmp::min(buf.len(), vec.len() - start);
    vec[start..start + overlap].copy_from_slice(&buf[..overlap]);
    if overlap < buf.len() {
        vec.extend_from_slice(&buf[overlap..]);
    }
    *pos += buf.len() as u64;
    Ok(buf.len())
}

#[cfg(feature = "alloc")]
impl Write for Cursor<alloc::vec::Vec<u8>> {
    fn write(&mut self, buf: &[u8]) -> Result<usize> {
        vec_write(&mut self.pos, &mut self.inner, buf)
    }

    fn flush(&mut self) -> Result<()> {
        Ok(())
    }
}

#[cfg(feature = "alloc")]
impl Write for Cursor<&mut alloc::vec::Vec<u8>> {
    fn write(&mut self, buf: &[u8]) -> Result<usize> {
        vec_write(&mut self.pos, self.inner, buf)
    }

    fn flush(&mut self) -> Result<()> {
        Ok(())
    }
}
