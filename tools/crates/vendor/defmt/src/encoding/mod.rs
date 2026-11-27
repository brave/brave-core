#[cfg(all(feature = "encoding-raw", feature = "encoding-rzcobs"))]
compile_error!("Multiple `encoding-*` features are enabled. You may only enable one.");

#[cfg_attr(feature = "encoding-raw", path = "raw.rs")]
#[cfg_attr(not(feature = "encoding-raw"), path = "rzcobs.rs")]
mod inner;

// This wrapper struct is to avoid copypasting the public docs in all the impls.

/// Encode raw defmt frames for sending over the wire.
///
/// defmt emits "log frames", which are sequences of bytes. The raw log frame data
/// is then *encoded* prior to sending over the wire.
///
/// `Encoder` will encode the frames according to the currently selected
/// `encoding-*` Cargo feature. See `Cargo.toml` for the supported encodings
/// and their tradeoffs.
///
/// Encodings may perform two functions:
///
/// - Framing: Adds extra data to allow the encoder to know when each frame starts
///   and ends in the stream. Unframed log frames already contain enough information for
///   the decoder to know when they end, so framing is optional. However, without framing
///   the decoder must receive all bytes intact or it may "lose sync". With framing, it can
///   recover from missing/corrupted data, and can start decoding from the "middle" of an
///   already-running stream.
/// - Compression: The frame data has rather low entropy (for example, it contains many
///   zero bytes due to encoding all integers in fixed with, and will likely contain many
///   repetitions). Compression can decrease the on-the-wire required bandwidth.
///
/// defmt provides the `Encoder` separately instead of feeding already-encoded bytes
/// to the `Logger` because `Logger` implementations may decide to allow
/// concurrent logging from multiple "contexts" such as threads or interrupt
/// priority levels. In this case, the Logger implementation needs to create one
/// Encoder for each such context.
pub struct Encoder {
    inner: inner::Encoder,
}

impl Encoder {
    /// Create a new `Encoder`.
    #[allow(clippy::new_without_default)]
    pub const fn new() -> Self {
        Self {
            inner: inner::Encoder::new(),
        }
    }

    /// Start encoding a log frame.
    ///
    /// `Logger` impls will typically call this from `acquire()`.
    ///
    /// You may only call `start_frame` when no frame is currently being encoded.
    /// Failure to do so may result in corrupted data on the wire.
    ///
    /// The `write` closure will be called with the encoded data that must
    /// be sent on the wire. It may be called zero, one, or multiple times.
    pub fn start_frame(&mut self, write: impl FnMut(&[u8])) {
        self.inner.start_frame(write)
    }

    /// Finish encoding a log frame.
    ///
    /// `Logger` impls will typically call this from `release()`.
    ///
    /// You may only call `end_frame` when a frame is currently being encoded.
    /// Failure to do so may result in corrupted data on the wire.
    ///
    /// The `write` closure will be called with the encoded data that must
    /// be sent on the wire. It may be called zero, one, or multiple times.
    pub fn end_frame(&mut self, write: impl FnMut(&[u8])) {
        self.inner.end_frame(write)
    }

    /// Write part of data for a log frame.
    ///
    /// `Logger` impls will typically call this from `write()`.
    ///
    /// You may only call `write` when a frame is currently being encoded.
    /// Failure to do so may result in corrupted data on the wire.
    ///
    /// The `write` closure will be called with the encoded data that must
    /// be sent on the wire. It may be called zero, one, or multiple times.
    pub fn write(&mut self, data: &[u8], write: impl FnMut(&[u8])) {
        self.inner.write(data, write)
    }
}
