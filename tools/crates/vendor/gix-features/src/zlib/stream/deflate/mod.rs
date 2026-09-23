use crate::zlib::Status;
use std::ffi::c_int;

const BUF_SIZE: usize = 4096 * 8;

/// A utility to zlib compress anything that is written via its [Write][std::io::Write] implementation.
///
/// Be sure to call `flush()` when done to finalize the deflate stream.
pub struct Write<W> {
    compressor: Compress,
    inner: W,
    buf: [u8; BUF_SIZE],
}

impl<W> Clone for Write<W>
where
    W: Clone,
{
    fn clone(&self) -> Self {
        Write {
            compressor: impls::new_compress(),
            inner: self.inner.clone(),
            buf: self.buf,
        }
    }
}

/// Hold all state needed for compressing data.
pub struct Compress(libz_rs_sys::z_stream);

unsafe impl Sync for Compress {}
unsafe impl Send for Compress {}

impl Default for Compress {
    fn default() -> Self {
        Self::new()
    }
}

impl Compress {
    /// The number of bytes that were read from the input.
    pub fn total_in(&self) -> u64 {
        self.0.total_in as _
    }

    /// The number of compressed bytes that were written to the output.
    pub fn total_out(&self) -> u64 {
        self.0.total_out as _
    }

    /// Create a new instance - this allocates so should be done with care.
    pub fn new() -> Self {
        let mut this = libz_rs_sys::z_stream::default();

        unsafe {
            libz_rs_sys::deflateInit_(
                &mut this,
                libz_rs_sys::Z_BEST_SPEED,
                libz_rs_sys::zlibVersion(),
                core::mem::size_of::<libz_rs_sys::z_stream>() as core::ffi::c_int,
            );
        }

        Self(this)
    }

    /// Prepare the instance for a new stream.
    pub fn reset(&mut self) {
        unsafe { libz_rs_sys::deflateReset(&mut self.0) };
    }

    /// Compress `input` and write compressed bytes to `output`, with `flush` controlling additional characteristics.
    pub fn compress(&mut self, input: &[u8], output: &mut [u8], flush: FlushCompress) -> Result<Status, CompressError> {
        self.0.avail_in = input.len() as _;
        self.0.avail_out = output.len() as _;

        self.0.next_in = input.as_ptr();
        self.0.next_out = output.as_mut_ptr();

        match unsafe { libz_rs_sys::deflate(&mut self.0, flush as _) } {
            libz_rs_sys::Z_OK => Ok(Status::Ok),
            libz_rs_sys::Z_BUF_ERROR => Ok(Status::BufError),
            libz_rs_sys::Z_STREAM_END => Ok(Status::StreamEnd),

            libz_rs_sys::Z_STREAM_ERROR => Err(CompressError::StreamError),
            libz_rs_sys::Z_MEM_ERROR => Err(CompressError::InsufficientMemory),
            err => Err(CompressError::Unknown { err }),
        }
    }
}

impl Drop for Compress {
    fn drop(&mut self) {
        unsafe { libz_rs_sys::deflateEnd(&mut self.0) };
    }
}

/// The error produced by [`Compress::compress()`].
#[derive(Debug, thiserror::Error)]
#[error("{msg}")]
#[allow(missing_docs)]
pub enum CompressError {
    #[error("stream error")]
    StreamError,
    #[error("Not enough memory")]
    InsufficientMemory,
    #[error("An unknown error occurred: {err}")]
    Unknown { err: c_int },
}

/// Values which indicate the form of flushing to be used when compressing
/// in-memory data.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
#[non_exhaustive]
#[allow(clippy::unnecessary_cast)]
pub enum FlushCompress {
    /// A typical parameter for passing to compression/decompression functions,
    /// this indicates that the underlying stream to decide how much data to
    /// accumulate before producing output in order to maximize compression.
    None = libz_rs_sys::Z_NO_FLUSH as isize,

    /// All pending output is flushed to the output buffer, but the output is
    /// not aligned to a byte boundary.
    ///
    /// All input data so far will be available to the decompressor (as with
    /// `Flush::Sync`). This completes the current deflate block and follows it
    /// with an empty fixed codes block that is 10 bits long, and it assures
    /// that enough bytes are output in order for the decompressor to finish the
    /// block before the empty fixed code block.
    Partial = libz_rs_sys::Z_PARTIAL_FLUSH as isize,

    /// All pending output is flushed to the output buffer and the output is
    /// aligned on a byte boundary so that the decompressor can get all input
    /// data available so far.
    ///
    /// Flushing may degrade compression for some compression algorithms and so
    /// it should only be used when necessary. This will complete the current
    /// deflate block and follow it with an empty stored block.
    Sync = libz_rs_sys::Z_SYNC_FLUSH as isize,

    /// All output is flushed as with `Flush::Sync` and the compression state is
    /// reset so decompression can restart from this point if previous
    /// compressed data has been damaged or if random access is desired.
    ///
    /// Using this option too often can seriously degrade compression.
    Full = libz_rs_sys::Z_FULL_FLUSH as isize,

    /// Pending input is processed and pending output is flushed.
    ///
    /// The return value may indicate that the stream is not yet done and more
    /// data has yet to be processed.
    Finish = libz_rs_sys::Z_FINISH as isize,
}

mod impls {
    use std::io;

    use crate::zlib::stream::deflate::{self, Compress, FlushCompress};
    use crate::zlib::Status;

    pub(crate) fn new_compress() -> Compress {
        Compress::new()
    }

    impl<W> deflate::Write<W>
    where
        W: io::Write,
    {
        /// Create a new instance writing compressed bytes to `inner`.
        pub fn new(inner: W) -> deflate::Write<W> {
            deflate::Write {
                compressor: new_compress(),
                inner,
                buf: [0; deflate::BUF_SIZE],
            }
        }

        /// Reset the compressor, starting a new compression stream.
        ///
        /// That way multiple streams can be written to the same inner writer.
        pub fn reset(&mut self) {
            self.compressor.reset();
        }

        /// Consume `self` and return the inner writer.
        pub fn into_inner(self) -> W {
            self.inner
        }

        fn write_inner(&mut self, mut buf: &[u8], flush: FlushCompress) -> io::Result<usize> {
            let total_in_when_start = self.compressor.total_in();
            loop {
                let last_total_in = self.compressor.total_in();
                let last_total_out = self.compressor.total_out();

                let status = self
                    .compressor
                    .compress(buf, &mut self.buf, flush)
                    .map_err(io::Error::other)?;

                let written = self.compressor.total_out() - last_total_out;
                if written > 0 {
                    self.inner.write_all(&self.buf[..written as usize])?;
                }

                match status {
                    Status::StreamEnd => return Ok((self.compressor.total_in() - total_in_when_start) as usize),
                    Status::Ok | Status::BufError => {
                        let consumed = self.compressor.total_in() - last_total_in;
                        buf = &buf[consumed as usize..];

                        // output buffer still makes progress
                        if self.compressor.total_out() > last_total_out {
                            continue;
                        }
                        // input still makes progress
                        if self.compressor.total_in() > last_total_in {
                            continue;
                        }
                        // input also makes no progress anymore, need more so leave with what we have
                        return Ok((self.compressor.total_in() - total_in_when_start) as usize);
                    }
                }
            }
        }
    }

    impl<W: io::Write> io::Write for deflate::Write<W> {
        fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
            self.write_inner(buf, FlushCompress::None)
        }

        fn flush(&mut self) -> io::Result<()> {
            self.write_inner(&[], FlushCompress::Finish).map(|_| ())
        }
    }
}

#[cfg(test)]
mod tests;
