//! `Read` and `Write` trait definitions for `no_std` environments.

use super::{Error, ErrorKind, Result};
use core::fmt;

/// A trait for reading bytes from a source.
///
/// This is a minimal subset of [`std::io::Read`] for `no_std` environments.
pub trait Read {
    /// Pull some bytes from this source into the specified buffer, returning
    /// how many bytes were read.
    ///
    /// # Implementor contract
    ///
    /// The return value and the default [`read_exact`](Read::read_exact) impl
    /// interact as follows — implementors **must** uphold these invariants for
    /// the default methods to behave correctly:
    ///
    /// * **`Ok(0)` signals EOF.** When `buf` is non-empty, returning `Ok(0)`
    ///   tells callers (including `read_exact`) that no more bytes will ever be
    ///   produced. If your source can temporarily have no data (e.g. a
    ///   non-blocking channel), return
    ///   `Err(ErrorKind::Interrupted)` or a custom error instead.
    ///   If the source has an end, it should eventually return `Ok(0)`.
    /// * **`Ok(0)` with an empty `buf` is always valid** and should not block.
    /// * **Short reads are not errors.** `Ok(n)` with `0 < n < buf.len()` is
    ///   expected; callers loop when they need more.
    /// * **`Err(ErrorKind::Interrupted)` should be surfaced, not retried
    ///   internally.** `read_exact` already retries on `Interrupted`; if the
    ///   impl also retries, callers lose the ability to observe or cancel on
    ///   interruption.
    /// * **`buf` contents beyond `[..n]` are unspecified.** Callers must not
    ///   read past the returned count.
    /// * **On `Err`, the number of bytes consumed is unspecified.**
    fn read(&mut self, buf: &mut [u8]) -> Result<usize>;

    /// Read the exact number of bytes required to fill `buf`.
    fn read_exact(&mut self, mut buf: &mut [u8]) -> Result<()> {
        while !buf.is_empty() {
            match self.read(buf) {
                Ok(0) => return Err(Error::from(ErrorKind::UnexpectedEof)),
                Ok(n) => buf = &mut buf[n..],
                Err(ref e) if e.kind() == ErrorKind::Interrupted => {
                    // Errors of kind [`ErrorKind::Interrupted`] are automatically retried.
                    // This matches the behavior of [`std::io::Read::read_exact`]: since this
                    // method promises to fill `buf` completely, a transient interruption
                    // (e.g. EINTR) is not a reason to fail — the read is simply retried.
                }
                Err(e) => return Err(e),
            }
        }
        Ok(())
    }
}

/// A trait for writing bytes to a destination.
///
/// This is a minimal subset of [`std::io::Write`] for `no_std` environments.
pub trait Write {
    /// Write a buffer into this writer, returning how many bytes were written.
    ///
    /// # Implementor contract
    ///
    /// The return value and the default [`write_all`](Write::write_all) impl
    /// interact as follows — implementors **must** uphold these invariants for
    /// the default methods to behave correctly:
    ///
    /// * **`Ok(0)` means the writer cannot accept more bytes.** `write_all`
    ///   converts this into `ErrorKind::WriteZero`. This is the write-side
    ///   equivalent of EOF.
    /// * **`Ok(0)` with an empty `buf` is always valid.**
    /// * **Short writes are not errors.** `Ok(n)` with `0 < n < buf.len()` is
    ///   expected; callers loop when they need more.
    /// * **`Err(ErrorKind::Interrupted)` should be surfaced, not retried
    ///   internally.** `write_all` already retries on `Interrupted`; if the
    ///   impl also retries, callers lose the ability to observe or cancel on
    ///   interruption.
    /// * **On `Err`, the number of bytes written is unspecified.**
    fn write(&mut self, buf: &[u8]) -> Result<usize>;

    /// Flush this output stream, ensuring that all intermediately buffered
    /// contents reach their destination.
    ///
    /// Unbuffered writers may implement this as a no-op returning `Ok(())`.
    /// Callers should not assume previously written bytes are durable until
    /// `flush` returns `Ok(())`.
    fn flush(&mut self) -> Result<()>;

    /// Attempts to write an entire buffer into this writer.
    fn write_all(&mut self, mut buf: &[u8]) -> Result<()> {
        while !buf.is_empty() {
            match self.write(buf) {
                Ok(0) => return Err(Error::from(ErrorKind::WriteZero)),
                Ok(n) => buf = &buf[n..],
                Err(ref e) if e.kind() == ErrorKind::Interrupted => {
                    // Errors of kind [`ErrorKind::Interrupted`] are automatically retried.
                    // This matches the behavior of [`std::io::Write::write_all`]: since this
                    // method promises to write `buf` completely, a transient interruption
                    // (e.g. EINTR) is not a reason to fail — the write is simply retried.
                }
                Err(e) => return Err(e),
            }
        }
        Ok(())
    }

    /// Writes a formatted string into this writer.
    fn write_fmt(&mut self, fmt: fmt::Arguments<'_>) -> Result<()> {
        struct Adapter<'a, T: ?Sized> {
            inner: &'a mut T,
            error: Result<()>,
        }

        impl<T: Write + ?Sized> fmt::Write for Adapter<'_, T> {
            fn write_str(&mut self, s: &str) -> fmt::Result {
                match self.inner.write_all(s.as_bytes()) {
                    Ok(()) => Ok(()),
                    Err(e) => {
                        self.error = Err(e);
                        Err(fmt::Error)
                    }
                }
            }
        }

        let mut adapter = Adapter {
            inner: self,
            error: Ok(()),
        };
        match fmt::write(&mut adapter, fmt) {
            Ok(()) => Ok(()),
            Err(..) => {
                if adapter.error.is_err() {
                    adapter.error
                } else {
                    Err(Error::new_static(ErrorKind::Other, "formatter error"))
                }
            }
        }
    }
}
