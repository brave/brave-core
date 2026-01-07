use core::ffi::c_uint;

use crate::deflate::DeflateConfig;
use crate::inflate::InflateConfig;
use crate::ReturnCode;
pub use crate::{DeflateFlush, InflateFlush};

/// Possible status results of compressing some data or successfully
/// decompressing a block of data.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum Status {
    /// Indicates success.
    ///
    /// Means that more input may be needed but isn't available
    /// and/or there's more output to be written but the output buffer is full.
    Ok,

    /// Indicates that forward progress is not possible due to input or output
    /// buffers being empty.
    ///
    /// For compression it means the input buffer needs some more data or the
    /// output buffer needs to be freed up before trying again.
    ///
    /// For decompression this means that more input is needed to continue or
    /// the output buffer isn't large enough to contain the result. The function
    /// can be called again after fixing both.
    BufError,

    /// Indicates that all input has been consumed and all output bytes have
    /// been written. Decompression/compression should not be called again.
    ///
    /// For decompression with zlib streams the adler-32 of the decompressed
    /// data has also been verified.
    StreamEnd,
}

/// Errors that can occur when decompressing.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
#[repr(i32)]
pub enum InflateError {
    /// Decompressing this input requires a dictionary.
    NeedDict { dict_id: u32 } = 2,
    /// The [`Inflate`] is in an inconsistent state, most likely
    /// due to an invalid configuration parameter.
    StreamError = -2,
    /// The input is not a valid deflate stream.
    DataError = -3,
    /// A memory allocation failed.
    MemError = -4,
}

impl From<InflateError> for ReturnCode {
    fn from(value: InflateError) -> Self {
        match value {
            InflateError::NeedDict { .. } => ReturnCode::NeedDict,
            InflateError::StreamError => ReturnCode::StreamError,
            InflateError::DataError => ReturnCode::DataError,
            InflateError::MemError => ReturnCode::MemError,
        }
    }
}

impl InflateError {
    pub fn as_str(self) -> &'static str {
        ReturnCode::from(self).error_message_str()
    }
}

/// The state that is used to decompress an input.
pub struct Inflate(crate::inflate::InflateStream<'static>);

impl Inflate {
    /// The amount of bytes consumed from the input so far.
    pub fn total_in(&self) -> u64 {
        #[allow(clippy::useless_conversion)]
        u64::from(self.0.total_in)
    }

    /// The amount of decompressed bytes that have been written to the output thus far.
    pub fn total_out(&self) -> u64 {
        #[allow(clippy::useless_conversion)]
        u64::from(self.0.total_out)
    }

    /// The error message if the previous operation failed.
    pub fn error_message(&self) -> Option<&'static str> {
        if self.0.msg.is_null() {
            None
        } else {
            unsafe { core::ffi::CStr::from_ptr(self.0.msg).to_str() }.ok()
        }
    }

    /// Create a new instance. Note that it allocates in various ways and thus should be re-used.
    ///
    /// The `window_bits` must be in the range `8..=15`, with `15` being most common.
    pub fn new(zlib_header: bool, window_bits: u8) -> Self {
        let config = InflateConfig {
            window_bits: if zlib_header {
                i32::from(window_bits)
            } else {
                -i32::from(window_bits)
            },
        };

        Self(crate::inflate::InflateStream::new(config))
    }

    /// Reset the state to allow handling a new stream.
    pub fn reset(&mut self, zlib_header: bool) {
        let mut config = InflateConfig::default();

        if !zlib_header {
            config.window_bits = -config.window_bits;
        }

        crate::inflate::reset_with_config(&mut self.0, config);
    }

    /// Decompress `input` and write all decompressed bytes into `output`, with `flush` defining some details about this.
    pub fn decompress(
        &mut self,
        input: &[u8],
        output: &mut [u8],
        flush: InflateFlush,
    ) -> Result<Status, InflateError> {
        // Limit the length of the input and output to the maximum value of a c_uint. For larger
        // inputs, this will either complete or signal that more input and output is needed. The
        // caller should be able to handle this regardless.
        self.0.avail_in = Ord::min(input.len(), c_uint::MAX as usize) as c_uint;
        self.0.avail_out = Ord::min(output.len(), c_uint::MAX as usize) as c_uint;

        // This cast_mut is unfortunate, that is just how the types are.
        self.0.next_in = input.as_ptr().cast_mut();
        self.0.next_out = output.as_mut_ptr();

        // SAFETY: the inflate state was properly initialized.
        match unsafe { crate::inflate::inflate(&mut self.0, flush) } {
            ReturnCode::Ok => Ok(Status::Ok),
            ReturnCode::StreamEnd => Ok(Status::StreamEnd),
            ReturnCode::NeedDict => Err(InflateError::NeedDict {
                dict_id: self.0.adler as u32,
            }),
            ReturnCode::ErrNo => unreachable!("the rust API does not use files"),
            ReturnCode::StreamError => Err(InflateError::StreamError),
            ReturnCode::DataError => Err(InflateError::DataError),
            ReturnCode::MemError => Err(InflateError::MemError),
            ReturnCode::BufError => Ok(Status::BufError),
            ReturnCode::VersionError => unreachable!("the rust API does not use the version"),
        }
    }

    pub fn set_dictionary(&mut self, dictionary: &[u8]) -> Result<u32, InflateError> {
        match crate::inflate::set_dictionary(&mut self.0, dictionary) {
            ReturnCode::Ok => Ok(self.0.adler as u32),
            ReturnCode::StreamError => Err(InflateError::StreamError),
            ReturnCode::DataError => Err(InflateError::DataError),
            other => unreachable!("set_dictionary does not return {other:?}"),
        }
    }
}

impl Drop for Inflate {
    fn drop(&mut self) {
        let _ = crate::inflate::end(&mut self.0);
    }
}

/// Errors that can occur when compressing.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum DeflateError {
    /// The [`Deflate`] is in an inconsistent state, most likely
    /// due to an invalid configuration parameter.
    StreamError = -2,
    /// The input is not a valid deflate stream.
    DataError = -3,
    /// A memory allocation failed.
    MemError = -4,
}

impl From<DeflateError> for ReturnCode {
    fn from(value: DeflateError) -> Self {
        match value {
            DeflateError::StreamError => ReturnCode::StreamError,
            DeflateError::DataError => ReturnCode::DataError,
            DeflateError::MemError => ReturnCode::MemError,
        }
    }
}

impl DeflateError {
    pub fn as_str(self) -> &'static str {
        ReturnCode::from(self).error_message_str()
    }
}

impl From<ReturnCode> for Result<Status, DeflateError> {
    fn from(value: ReturnCode) -> Self {
        match value {
            ReturnCode::Ok => Ok(Status::Ok),
            ReturnCode::StreamEnd => Ok(Status::StreamEnd),
            ReturnCode::NeedDict => unreachable!("compression does not use dictionary"),
            ReturnCode::ErrNo => unreachable!("the rust API does not use files"),
            ReturnCode::StreamError => Err(DeflateError::StreamError),
            ReturnCode::DataError => Err(DeflateError::DataError),
            ReturnCode::MemError => Err(DeflateError::MemError),
            ReturnCode::BufError => Ok(Status::BufError),
            ReturnCode::VersionError => unreachable!("the rust API does not use the version"),
        }
    }
}

/// The state that is used to compress an input.
pub struct Deflate(crate::deflate::DeflateStream<'static>);

impl Deflate {
    /// The number of bytes that were read from the input.
    pub fn total_in(&self) -> u64 {
        #[allow(clippy::useless_conversion)]
        u64::from(self.0.total_in)
    }

    /// The number of compressed bytes that were written to the output.
    pub fn total_out(&self) -> u64 {
        #[allow(clippy::useless_conversion)]
        u64::from(self.0.total_out)
    }

    /// The error message if the previous operation failed.
    pub fn error_message(&self) -> Option<&'static str> {
        if self.0.msg.is_null() {
            None
        } else {
            unsafe { core::ffi::CStr::from_ptr(self.0.msg).to_str() }.ok()
        }
    }

    /// Create a new instance - this allocates so should be done with care.
    ///
    /// The `window_bits` must be in the range `8..=15`, with `15` being most common.
    pub fn new(level: i32, zlib_header: bool, window_bits: u8) -> Self {
        let config = DeflateConfig {
            window_bits: if zlib_header {
                i32::from(window_bits)
            } else {
                -i32::from(window_bits)
            },
            level,
            ..DeflateConfig::default()
        };

        Self(crate::deflate::DeflateStream::new(config))
    }

    /// Prepare the instance for a new stream.
    pub fn reset(&mut self) {
        crate::deflate::reset(&mut self.0);
    }

    /// Compress `input` and write compressed bytes to `output`, with `flush` controlling additional characteristics.
    pub fn compress(
        &mut self,
        input: &[u8],
        output: &mut [u8],
        flush: DeflateFlush,
    ) -> Result<Status, DeflateError> {
        // Limit the length of the input and output to the maximum value of a c_uint. For larger
        // inputs, this will either complete or signal that more input and output is needed. The
        // caller should be able to handle this regardless.
        self.0.avail_in = Ord::min(input.len(), c_uint::MAX as usize) as c_uint;
        self.0.avail_out = Ord::min(output.len(), c_uint::MAX as usize) as c_uint;

        // This cast_mut is unfortunate, that is just how the types are.
        self.0.next_in = input.as_ptr().cast_mut();
        self.0.next_out = output.as_mut_ptr();

        crate::deflate::deflate(&mut self.0, flush).into()
    }

    /// Specifies the compression dictionary to use.
    ///
    /// Returns the Adler-32 checksum of the dictionary.
    pub fn set_dictionary(&mut self, dictionary: &[u8]) -> Result<u32, DeflateError> {
        match crate::deflate::set_dictionary(&mut self.0, dictionary) {
            ReturnCode::Ok => Ok(self.0.adler as u32),
            ReturnCode::StreamError => Err(DeflateError::StreamError),
            other => unreachable!("set_dictionary does not return {other:?}"),
        }
    }

    /// Dynamically updates the compression level.
    ///
    /// This can be used to switch between compression levels for different
    /// kinds of data, or it can be used in conjunction with a call to [`Deflate::reset`]
    /// to reuse the compressor.
    ///
    /// This may return an error if there wasn't enough output space to complete
    /// the compression of the available input data before changing the
    /// compression level. Flushing the stream before calling this method
    /// ensures that the function will succeed on the first call.
    pub fn set_level(&mut self, level: i32) -> Result<Status, DeflateError> {
        match crate::deflate::params(&mut self.0, level, Default::default()) {
            ReturnCode::Ok => Ok(Status::Ok),
            ReturnCode::StreamError => Err(DeflateError::StreamError),
            ReturnCode::BufError => Ok(Status::BufError),
            other => unreachable!("set_level does not return {other:?}"),
        }
    }
}

impl Drop for Deflate {
    fn drop(&mut self) {
        let _ = crate::deflate::end(&mut self.0);
    }
}
