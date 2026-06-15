//! Integration tests for `corez::io`.
//!
//! These tests exercise the public API and run under every feature
//! combination, ensuring that `std` re-exports behave identically to
//! the custom `no_std` implementations.

use corez::io::{Cursor, ErrorKind, Read, Write};

// ---------------------------------------------------------------------------
// Cursor — Read
// ---------------------------------------------------------------------------

#[test]
fn cursor_read_basic() {
    let data = [1u8, 2, 3, 4, 5];
    let mut cursor = Cursor::new(&data[..]);
    let mut buf = [0u8; 3];
    let n = cursor.read(&mut buf).unwrap();
    assert_eq!(n, 3);
    assert_eq!(buf, [1, 2, 3]);
    assert_eq!(cursor.position(), 3);
}

#[test]
fn cursor_read_exact_success() {
    let data = [10u8, 20, 30];
    let mut cursor = Cursor::new(&data[..]);
    let mut buf = [0u8; 3];
    cursor.read_exact(&mut buf).unwrap();
    assert_eq!(buf, [10, 20, 30]);
}

#[test]
fn cursor_read_exact_eof() {
    let data = [1u8, 2];
    let mut cursor = Cursor::new(&data[..]);
    let mut buf = [0u8; 3];
    let err = cursor.read_exact(&mut buf).unwrap_err();
    assert_eq!(err.kind(), ErrorKind::UnexpectedEof);
}

#[test]
fn cursor_read_past_end() {
    let data = [1u8, 2, 3];
    let mut cursor = Cursor::new(&data[..]);
    cursor.set_position(100);
    let mut buf = [0u8; 1];
    assert_eq!(cursor.read(&mut buf).unwrap(), 0); // EOF
}

#[test]
fn cursor_position_and_set_position() {
    let data = [10u8, 20, 30, 40];
    let mut cursor = Cursor::new(&data[..]);
    assert_eq!(cursor.position(), 0);
    let mut buf = [0u8; 2];
    cursor.read_exact(&mut buf).unwrap();
    assert_eq!(cursor.position(), 2);
    cursor.set_position(0);
    cursor.read_exact(&mut buf).unwrap();
    assert_eq!(buf, [10, 20]);
}

#[test]
#[cfg(feature = "alloc")]
fn cursor_into_inner() {
    let data = vec![1u8, 2, 3];
    let cursor = Cursor::new(data.clone());
    assert_eq!(cursor.into_inner(), data);
}

// ---------------------------------------------------------------------------
// Cursor — Write (Vec<u8>)
// ---------------------------------------------------------------------------

#[test]
#[cfg(feature = "alloc")]
fn cursor_write_vec() {
    let mut cursor = Cursor::new(Vec::new());
    cursor.write_all(&[1, 2, 3]).unwrap();
    cursor.write_all(&[4, 5]).unwrap();
    assert_eq!(cursor.into_inner(), vec![1, 2, 3, 4, 5]);
}

#[test]
#[cfg(feature = "alloc")]
fn cursor_write_vec_at_position() {
    let mut cursor = Cursor::new(vec![0u8; 5]);
    cursor.set_position(2);
    cursor.write_all(&[10, 20]).unwrap();
    assert_eq!(cursor.position(), 4);
    assert_eq!(cursor.get_ref(), &vec![0, 0, 10, 20, 0]);
}

#[test]
#[cfg(feature = "alloc")]
fn cursor_write_vec_extends() {
    let mut cursor = Cursor::new(vec![1u8, 2]);
    cursor.set_position(1);
    cursor.write_all(&[10, 20, 30]).unwrap();
    assert_eq!(cursor.into_inner(), vec![1, 10, 20, 30]);
}

// On platforms where usize < u64, a cursor position beyond usize::MAX
// must produce an InvalidInput error rather than silently truncating.
#[test]
#[cfg(feature = "alloc")]
#[cfg(not(target_pointer_width = "64"))]
fn cursor_write_vec_pos_overflow() {
    let mut cursor = Cursor::new(Vec::new());
    cursor.set_position(u64::from(u32::MAX) + 1);
    let err = cursor.write_all(&[1]).unwrap_err();
    assert_eq!(err.kind(), ErrorKind::InvalidInput);
}

#[test]
#[cfg(feature = "alloc")]
fn cursor_write_vec_gap_fill() {
    let mut cursor = Cursor::new(vec![1u8, 2]);
    cursor.set_position(5);
    cursor.write_all(&[10]).unwrap();
    assert_eq!(cursor.into_inner(), vec![1, 2, 0, 0, 0, 10]);
}

// ---------------------------------------------------------------------------
// Cursor — Write (&mut [u8])
// ---------------------------------------------------------------------------

#[test]
fn cursor_write_mut_slice() {
    let mut buf = [0u8; 5];
    {
        let mut cursor = Cursor::new(&mut buf[..]);
        cursor.write_all(&[1, 2, 3]).unwrap();
        assert_eq!(cursor.position(), 3);
    }
    assert_eq!(buf, [1, 2, 3, 0, 0]);
}

#[test]
fn cursor_write_mut_slice_overflow() {
    let mut buf = [0u8; 2];
    let mut cursor = Cursor::new(&mut buf[..]);
    let err = cursor.write_all(&[1, 2, 3]).unwrap_err();
    assert_eq!(err.kind(), ErrorKind::WriteZero);
}

#[test]
fn cursor_write_mut_slice_short() {
    let mut buf = [0u8; 3];
    let mut cursor = Cursor::new(&mut buf[..]);
    cursor.set_position(2);
    assert_eq!(cursor.write(&[1, 2, 3]).unwrap(), 1); // only 1 byte fits
}

// ---------------------------------------------------------------------------
// Cursor — Write (&mut Vec<u8>)
// ---------------------------------------------------------------------------

#[test]
#[cfg(feature = "alloc")]
fn cursor_write_ref_vec() {
    let mut v = Vec::new();
    {
        let mut cursor = Cursor::new(&mut v);
        cursor.write_all(&[1, 2, 3]).unwrap();
    }
    assert_eq!(v, vec![1, 2, 3]);
}

// ---------------------------------------------------------------------------
// Slice impls — Read for &[u8]
// ---------------------------------------------------------------------------

#[test]
fn read_slice_basic() {
    let data = [1u8, 2, 3, 4, 5];
    let mut reader: &[u8] = &data;
    let mut buf = [0u8; 3];
    assert_eq!(reader.read(&mut buf).unwrap(), 3);
    assert_eq!(buf, [1, 2, 3]);
    assert_eq!(reader.read(&mut buf).unwrap(), 2);
    assert_eq!(buf[..2], [4, 5]);
}

#[test]
fn read_slice_exact() {
    let data = [10u8, 20, 30];
    let mut reader: &[u8] = &data;
    let mut buf = [0u8; 3];
    reader.read_exact(&mut buf).unwrap();
    assert_eq!(buf, [10, 20, 30]);
}

#[test]
fn read_slice_exact_eof() {
    let data = [1u8, 2];
    let mut reader: &[u8] = &data;
    let mut buf = [0u8; 3];
    let err = reader.read_exact(&mut buf).unwrap_err();
    assert_eq!(err.kind(), ErrorKind::UnexpectedEof);
}

#[test]
fn read_slice_empty() {
    let mut reader: &[u8] = &[];
    let mut buf = [0u8; 1];
    assert_eq!(reader.read(&mut buf).unwrap(), 0);
}

// ---------------------------------------------------------------------------
// Slice impls — Write for &mut [u8]
// ---------------------------------------------------------------------------

#[test]
fn write_mut_slice_basic() {
    let mut buf = [0u8; 5];
    let mut writer: &mut [u8] = &mut buf;
    assert_eq!(writer.write(&[1, 2, 3]).unwrap(), 3);
    assert_eq!(writer.write(&[4, 5, 6]).unwrap(), 2);
    assert_eq!(buf, [1, 2, 3, 4, 5]);
}

// ---------------------------------------------------------------------------
// Slice impls — Write for Vec<u8>
// ---------------------------------------------------------------------------

#[test]
#[cfg(feature = "alloc")]
fn write_vec_basic() {
    let mut buf = Vec::new();
    buf.write_all(&[1, 2, 3]).unwrap();
    buf.write_all(&[4, 5]).unwrap();
    assert_eq!(buf, vec![1, 2, 3, 4, 5]);
}

// ---------------------------------------------------------------------------
// Forwarding impls
// ---------------------------------------------------------------------------

#[test]
fn read_write_forwarding() {
    let data = [1u8, 2, 3];
    let mut reader: &[u8] = &data;
    let reader_ref: &mut &[u8] = &mut reader;
    let mut buf = [0u8; 3];
    reader_ref.read_exact(&mut buf).unwrap();
    assert_eq!(buf, [1, 2, 3]);
}

#[test]
#[cfg(feature = "alloc")]
fn box_read_forwarding() {
    let data: &[u8] = &[1, 2, 3];
    let mut reader: Box<dyn Read> = Box::new(data);
    let mut buf = [0u8; 3];
    reader.read_exact(&mut buf).unwrap();
    assert_eq!(buf, [1, 2, 3]);
}

#[test]
#[cfg(feature = "alloc")]
fn box_write_forwarding() {
    let mut v = Vec::new();
    {
        let mut writer: Box<dyn Write> = Box::new(&mut v);
        writer.write_all(&[1, 2, 3]).unwrap();
    }
    assert_eq!(v, vec![1, 2, 3]);
}

// ---------------------------------------------------------------------------
// Default impl coverage — read_exact (default) via one-byte reader
// ---------------------------------------------------------------------------

/// A reader that delivers data one byte at a time, exercising the
/// default `read_exact` retry loop.
struct OneByte<'a>(&'a [u8]);

impl Read for OneByte<'_> {
    fn read(&mut self, buf: &mut [u8]) -> corez::io::Result<usize> {
        if self.0.is_empty() || buf.is_empty() {
            return Ok(0);
        }
        buf[0] = self.0[0];
        self.0 = &self.0[1..];
        Ok(1)
    }
}

#[test]
fn read_exact_default_loops() {
    let data = [1u8, 2, 3, 4, 5];
    let mut reader = OneByte(&data);
    let mut buf = [0u8; 5];
    reader.read_exact(&mut buf).unwrap();
    assert_eq!(buf, [1, 2, 3, 4, 5]);
}

#[test]
fn read_exact_default_eof_midway() {
    let data = [1u8, 2];
    let mut reader = OneByte(&data);
    let mut buf = [0u8; 5];
    let err = reader.read_exact(&mut buf).unwrap_err();
    assert_eq!(err.kind(), ErrorKind::UnexpectedEof);
}

// ---------------------------------------------------------------------------
// Default impl coverage — read_exact retries Interrupted
// ---------------------------------------------------------------------------

/// A reader that returns `Interrupted` once before delivering data one
/// byte at a time, exercising the `read_exact` retry path.
struct InterruptOnce<'a> {
    inner: &'a [u8],
    interrupted: bool,
}

impl Read for InterruptOnce<'_> {
    fn read(&mut self, buf: &mut [u8]) -> corez::io::Result<usize> {
        if !self.interrupted {
            self.interrupted = true;
            return Err(corez::io::Error::from(ErrorKind::Interrupted));
        }
        if self.inner.is_empty() || buf.is_empty() {
            return Ok(0);
        }
        buf[0] = self.inner[0];
        self.inner = &self.inner[1..];
        Ok(1)
    }
}

#[test]
fn read_exact_retries_interrupted() {
    let data = [1u8, 2, 3];
    let mut reader = InterruptOnce {
        inner: &data,
        interrupted: false,
    };
    let mut buf = [0u8; 3];
    reader.read_exact(&mut buf).unwrap();
    assert_eq!(buf, [1, 2, 3]);
}

// ---------------------------------------------------------------------------
// Default impl coverage — write_all (default) via single-byte writer
// ---------------------------------------------------------------------------

/// A writer that accepts one byte at a time, exercising the default
/// `write_all` retry loop.
#[cfg(feature = "alloc")]
struct OneByteWriter(Vec<u8>);

#[cfg(feature = "alloc")]
impl Write for OneByteWriter {
    fn write(&mut self, buf: &[u8]) -> corez::io::Result<usize> {
        if buf.is_empty() {
            return Ok(0);
        }
        self.0.push(buf[0]);
        Ok(1)
    }

    fn flush(&mut self) -> corez::io::Result<()> {
        Ok(())
    }
}

#[test]
#[cfg(feature = "alloc")]
fn write_all_default_loops() {
    let mut writer = OneByteWriter(Vec::new());
    writer.write_all(&[1, 2, 3, 4, 5]).unwrap();
    assert_eq!(writer.0, vec![1, 2, 3, 4, 5]);
}

/// A writer backed by a fixed-size array, usable without alloc.
struct SliceWriter<'a> {
    buf: &'a mut [u8],
    pos: usize,
}

impl Write for SliceWriter<'_> {
    fn write(&mut self, data: &[u8]) -> corez::io::Result<usize> {
        if data.is_empty() || self.pos >= self.buf.len() {
            return Ok(0);
        }
        self.buf[self.pos] = data[0];
        self.pos += 1;
        Ok(1)
    }

    fn flush(&mut self) -> corez::io::Result<()> {
        Ok(())
    }
}

#[test]
fn write_all_default_loops_no_alloc() {
    let mut backing = [0u8; 5];
    let mut writer = SliceWriter {
        buf: &mut backing,
        pos: 0,
    };
    writer.write_all(&[1, 2, 3, 4, 5]).unwrap();
    assert_eq!(backing, [1, 2, 3, 4, 5]);
}

#[test]
fn write_all_default_write_zero() {
    let mut backing = [0u8; 2];
    let mut writer = SliceWriter {
        buf: &mut backing,
        pos: 0,
    };
    let err = writer.write_all(&[1, 2, 3]).unwrap_err();
    assert_eq!(err.kind(), ErrorKind::WriteZero);
}

// ---------------------------------------------------------------------------
// Default impl coverage — write_fmt
// ---------------------------------------------------------------------------

#[test]
#[cfg(feature = "alloc")]
fn write_fmt_basic() {
    let mut buf = Vec::new();
    write!(buf, "hello {}", 42).unwrap();
    assert_eq!(buf, b"hello 42");
}

#[test]
fn write_fmt_propagates_io_error() {
    struct FailWriter;
    impl Write for FailWriter {
        fn write(&mut self, _: &[u8]) -> corez::io::Result<usize> {
            Err(corez::io::Error::from(ErrorKind::Other))
        }
        fn flush(&mut self) -> corez::io::Result<()> {
            Ok(())
        }
    }
    let mut w = FailWriter;
    let err = write!(w, "hello").unwrap_err();
    assert_eq!(err.kind(), ErrorKind::Other);
}

// ---------------------------------------------------------------------------
// Error — source()
// ---------------------------------------------------------------------------

#[test]
fn simple_error_has_no_source() {
    use core::error::Error as _;
    let err = corez::io::Error::from(ErrorKind::Other);
    assert!(err.source().is_none());
}

#[test]
#[cfg(feature = "alloc")]
fn custom_error_wrapping_simple_has_no_source() {
    use core::error::Error as _;
    // Wrapping a simple io::Error (which itself has no source) produces
    // no source chain, because io::Error::source() delegates to the
    // wrapped error's source(), matching std::io::Error behaviour.
    let inner = corez::io::Error::from(ErrorKind::InvalidData);
    let outer = corez::io::Error::new(ErrorKind::Other, inner);
    assert!(outer.source().is_none());
}

/// An error type with a source, for testing source() delegation.
#[cfg(feature = "alloc")]
mod error_with_source {
    use core::fmt;

    #[derive(Debug)]
    pub struct Root;

    impl fmt::Display for Root {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            f.write_str("root cause")
        }
    }

    impl core::error::Error for Root {}

    #[derive(Debug)]
    pub struct Wrapper(pub Root);

    impl fmt::Display for Wrapper {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            f.write_str("wrapper")
        }
    }

    impl core::error::Error for Wrapper {
        fn source(&self) -> Option<&(dyn core::error::Error + 'static)> {
            Some(&self.0)
        }
    }
}

#[test]
#[cfg(feature = "alloc")]
fn custom_error_delegates_source() {
    use core::error::Error as _;
    use error_with_source::{Root, Wrapper};

    let err = corez::io::Error::new(ErrorKind::Other, Wrapper(Root));
    // source() should delegate to Wrapper::source(), returning the Root.
    let src = err.source().expect("expected a source");
    assert_eq!(src.to_string(), "root cause");
}

// ---------------------------------------------------------------------------
// ErrorKind — Display without alloc
// ---------------------------------------------------------------------------

#[test]
fn error_kind_display_no_alloc() {
    let mut buf = [0u8; 64];
    let mut cursor = Cursor::new(&mut buf[..]);
    // write! dispatches to io::Write::write_fmt when io::Write is in scope.
    write!(cursor, "{}", ErrorKind::NotFound).unwrap();
    let written = cursor.position() as usize;
    assert_eq!(&buf[..written], b"entity not found");
}
