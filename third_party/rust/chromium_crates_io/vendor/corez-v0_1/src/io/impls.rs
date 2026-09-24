//! `Read` and `Write` implementations for core types in `no_std` environments.

use super::{Error, ErrorKind, Read, Result, Write};

// ---------------------------------------------------------------------------
// Forwarding impls — &mut R / &mut W
// ---------------------------------------------------------------------------

impl<R: Read + ?Sized> Read for &mut R {
    fn read(&mut self, buf: &mut [u8]) -> Result<usize> {
        (**self).read(buf)
    }

    fn read_exact(&mut self, buf: &mut [u8]) -> Result<()> {
        (**self).read_exact(buf)
    }
}

impl<W: Write + ?Sized> Write for &mut W {
    fn write(&mut self, buf: &[u8]) -> Result<usize> {
        (**self).write(buf)
    }

    fn flush(&mut self) -> Result<()> {
        (**self).flush()
    }

    fn write_all(&mut self, buf: &[u8]) -> Result<()> {
        (**self).write_all(buf)
    }

    fn write_fmt(&mut self, fmt: core::fmt::Arguments<'_>) -> Result<()> {
        (**self).write_fmt(fmt)
    }
}

// ---------------------------------------------------------------------------
// Forwarding impls — Box<R> / Box<W>
// ---------------------------------------------------------------------------

#[cfg(feature = "alloc")]
impl<R: Read + ?Sized> Read for alloc::boxed::Box<R> {
    fn read(&mut self, buf: &mut [u8]) -> Result<usize> {
        (**self).read(buf)
    }

    fn read_exact(&mut self, buf: &mut [u8]) -> Result<()> {
        (**self).read_exact(buf)
    }
}

#[cfg(feature = "alloc")]
impl<W: Write + ?Sized> Write for alloc::boxed::Box<W> {
    fn write(&mut self, buf: &[u8]) -> Result<usize> {
        (**self).write(buf)
    }

    fn flush(&mut self) -> Result<()> {
        (**self).flush()
    }

    fn write_all(&mut self, buf: &[u8]) -> Result<()> {
        (**self).write_all(buf)
    }

    fn write_fmt(&mut self, fmt: core::fmt::Arguments<'_>) -> Result<()> {
        (**self).write_fmt(fmt)
    }
}

// ---------------------------------------------------------------------------
// In-memory buffer — Read for &[u8]
// ---------------------------------------------------------------------------

impl Read for &[u8] {
    fn read(&mut self, buf: &mut [u8]) -> Result<usize> {
        let amt = core::cmp::min(buf.len(), self.len());
        let (to_copy, rest) = self.split_at(amt);
        buf[..amt].copy_from_slice(to_copy);
        *self = rest;
        Ok(amt)
    }

    fn read_exact(&mut self, buf: &mut [u8]) -> Result<()> {
        if buf.len() > self.len() {
            return Err(Error::from(ErrorKind::UnexpectedEof));
        }
        let (to_copy, rest) = self.split_at(buf.len());
        buf.copy_from_slice(to_copy);
        *self = rest;
        Ok(())
    }
}

// ---------------------------------------------------------------------------
// In-memory buffer — Write for &mut [u8]
// ---------------------------------------------------------------------------

impl Write for &mut [u8] {
    fn write(&mut self, buf: &[u8]) -> Result<usize> {
        let amt = core::cmp::min(buf.len(), self.len());
        let (dest, rest) = core::mem::take(self).split_at_mut(amt);
        dest.copy_from_slice(&buf[..amt]);
        *self = rest;
        Ok(amt)
    }

    fn write_all(&mut self, buf: &[u8]) -> Result<()> {
        if buf.len() > self.len() {
            return Err(Error::from(ErrorKind::WriteZero));
        }
        let (dest, rest) = core::mem::take(self).split_at_mut(buf.len());
        dest.copy_from_slice(buf);
        *self = rest;
        Ok(())
    }

    fn flush(&mut self) -> Result<()> {
        Ok(())
    }
}

// ---------------------------------------------------------------------------
// In-memory buffer — Write for Vec<u8>
// ---------------------------------------------------------------------------

#[cfg(feature = "alloc")]
impl Write for alloc::vec::Vec<u8> {
    fn write(&mut self, buf: &[u8]) -> Result<usize> {
        self.extend_from_slice(buf);
        Ok(buf.len())
    }

    fn write_all(&mut self, buf: &[u8]) -> Result<()> {
        self.extend_from_slice(buf);
        Ok(())
    }

    fn flush(&mut self) -> Result<()> {
        Ok(())
    }
}
