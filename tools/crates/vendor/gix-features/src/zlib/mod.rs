use std::ffi::c_int;

/// A type to hold all state needed for decompressing a ZLIB encoded stream.
pub struct Decompress(libz_rs_sys::z_stream);

unsafe impl Sync for Decompress {}
unsafe impl Send for Decompress {}

impl Default for Decompress {
    fn default() -> Self {
        Self::new()
    }
}

impl Decompress {
    /// The amount of bytes consumed from the input so far.
    pub fn total_in(&self) -> u64 {
        self.0.total_in as _
    }

    /// The amount of decompressed bytes that have been written to the output thus far.
    pub fn total_out(&self) -> u64 {
        self.0.total_out as _
    }

    /// Create a new instance. Note that it allocates in various ways and thus should be re-used.
    pub fn new() -> Self {
        let mut this = libz_rs_sys::z_stream::default();

        unsafe {
            libz_rs_sys::inflateInit_(
                &mut this,
                libz_rs_sys::zlibVersion(),
                core::mem::size_of::<libz_rs_sys::z_stream>() as core::ffi::c_int,
            );
        }

        Self(this)
    }

    /// Reset the state to allow handling a new stream.
    pub fn reset(&mut self) {
        unsafe { libz_rs_sys::inflateReset(&mut self.0) };
    }

    /// Decompress `input` and write all decompressed bytes into `output`, with `flush` defining some details about this.
    pub fn decompress(
        &mut self,
        input: &[u8],
        output: &mut [u8],
        flush: FlushDecompress,
    ) -> Result<Status, DecompressError> {
        self.0.avail_in = input.len() as _;
        self.0.avail_out = output.len() as _;

        self.0.next_in = input.as_ptr();
        self.0.next_out = output.as_mut_ptr();

        match unsafe { libz_rs_sys::inflate(&mut self.0, flush as _) } {
            libz_rs_sys::Z_OK => Ok(Status::Ok),
            libz_rs_sys::Z_BUF_ERROR => Ok(Status::BufError),
            libz_rs_sys::Z_STREAM_END => Ok(Status::StreamEnd),

            libz_rs_sys::Z_STREAM_ERROR => Err(DecompressError::StreamError),
            libz_rs_sys::Z_DATA_ERROR => Err(DecompressError::DataError),
            libz_rs_sys::Z_MEM_ERROR => Err(DecompressError::InsufficientMemory),
            err => Err(DecompressError::Unknown { err }),
        }
    }
}

impl Drop for Decompress {
    fn drop(&mut self) {
        unsafe { libz_rs_sys::inflateEnd(&mut self.0) };
    }
}

/// The error produced by [`Decompress::decompress()`].
#[derive(Debug, thiserror::Error)]
#[allow(missing_docs)]
pub enum DecompressError {
    #[error("stream error")]
    StreamError,
    #[error("Not enough memory")]
    InsufficientMemory,
    #[error("Invalid input data")]
    DataError,
    #[error("An unknown error occurred: {err}")]
    Unknown { err: c_int },
}

/// The status returned by [`Decompress::decompress()`].
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Status {
    /// The decompress operation went well. Not to be confused with `StreamEnd`, so one can continue
    /// the decompression.
    Ok,
    /// An error occurred when decompression.
    BufError,
    /// The stream was fully decompressed.
    StreamEnd,
}

/// Values which indicate the form of flushing to be used when
/// decompressing in-memory data.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
#[non_exhaustive]
#[allow(clippy::unnecessary_cast)]
pub enum FlushDecompress {
    /// A typical parameter for passing to compression/decompression functions,
    /// this indicates that the underlying stream to decide how much data to
    /// accumulate before producing output in order to maximize compression.
    None = libz_rs_sys::Z_NO_FLUSH as isize,

    /// All pending output is flushed to the output buffer and the output is
    /// aligned on a byte boundary so that the decompressor can get all input
    /// data available so far.
    ///
    /// Flushing may degrade compression for some compression algorithms and so
    /// it should only be used when necessary. This will complete the current
    /// deflate block and follow it with an empty stored block.
    Sync = libz_rs_sys::Z_SYNC_FLUSH as isize,

    /// Pending input is processed and pending output is flushed.
    ///
    /// The return value may indicate that the stream is not yet done and more
    /// data has yet to be processed.
    Finish = libz_rs_sys::Z_FINISH as isize,
}

/// non-streaming interfaces for decompression
pub mod inflate {
    /// The error returned by various [Inflate methods][super::Inflate]
    #[derive(Debug, thiserror::Error)]
    #[allow(missing_docs)]
    pub enum Error {
        #[error("Could not write all bytes when decompressing content")]
        WriteInflated(#[from] std::io::Error),
        #[error("Could not decode zip stream, status was '{0}'")]
        Inflate(#[from] super::DecompressError),
        #[error("The zlib status indicated an error, status was '{0:?}'")]
        Status(super::Status),
    }
}

/// Decompress a few bytes of a zlib stream without allocation
#[derive(Default)]
pub struct Inflate {
    /// The actual decompressor doing all the work.
    pub state: Decompress,
}

impl Inflate {
    /// Run the decompressor exactly once. Cannot be run multiple times
    pub fn once(&mut self, input: &[u8], out: &mut [u8]) -> Result<(Status, usize, usize), inflate::Error> {
        let before_in = self.state.total_in();
        let before_out = self.state.total_out();
        let status = self.state.decompress(input, out, FlushDecompress::None)?;
        Ok((
            status,
            (self.state.total_in() - before_in) as usize,
            (self.state.total_out() - before_out) as usize,
        ))
    }

    /// Ready this instance for decoding another data stream.
    pub fn reset(&mut self) {
        self.state.reset();
    }
}

///
pub mod stream;
