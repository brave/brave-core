//! Pure-Rust codecs for LZMA, LZMA2, and XZ.
#![cfg_attr(docsrs, feature(doc_cfg, doc_cfg_hide))]
#![deny(missing_docs)]
#![deny(missing_debug_implementations)]
#![forbid(unsafe_code)]

#[macro_use]
mod macros;

mod decode;
mod encode;

pub mod error;

mod util;
mod xz;

use std::io;

/// Compression helpers.
pub mod compress {
    pub use crate::encode::options::*;
}

/// Decompression helpers.
pub mod decompress {
    pub use crate::decode::options::*;

    #[cfg(feature = "raw_decoder")]
    #[cfg_attr(docsrs, doc(cfg(raw_decoder)))]
    pub mod raw {
        //! Raw decoding primitives for LZMA/LZMA2 streams.
        pub use crate::decode::lzma::{LzmaDecoder, LzmaParams, LzmaProperties};
        pub use crate::decode::lzma2::Lzma2Decoder;
    }

    #[cfg(feature = "stream")]
    #[cfg_attr(docsrs, doc(cfg(stream)))]
    pub use crate::decode::stream::Stream;
}

/// Decompress LZMA data with default [`Options`](decompress/struct.Options.html).
pub fn lzma_decompress<R: io::BufRead, W: io::Write>(
    input: &mut R,
    output: &mut W,
) -> error::Result<()> {
    lzma_decompress_with_options(input, output, &decompress::Options::default())
}

/// Decompress LZMA data with the provided options.
pub fn lzma_decompress_with_options<R: io::BufRead, W: io::Write>(
    input: &mut R,
    output: &mut W,
    options: &decompress::Options,
) -> error::Result<()> {
    let params = decode::lzma::LzmaParams::read_header(input, options)?;
    let mut decoder = decode::lzma::LzmaDecoder::new(params, options.memlimit)?;
    decoder.decompress(input, output)
}

/// Compresses data with LZMA and default [`Options`](compress/struct.Options.html).
pub fn lzma_compress<R: io::BufRead, W: io::Write>(
    input: &mut R,
    output: &mut W,
) -> io::Result<()> {
    lzma_compress_with_options(input, output, &compress::Options::default())
}

/// Compress LZMA data with the provided options.
pub fn lzma_compress_with_options<R: io::BufRead, W: io::Write>(
    input: &mut R,
    output: &mut W,
    options: &compress::Options,
) -> io::Result<()> {
    let encoder = encode::dumbencoder::Encoder::from_stream(output, options)?;
    encoder.process(input)
}

/// Decompress LZMA2 data with default [`Options`](decompress/struct.Options.html).
pub fn lzma2_decompress<R: io::BufRead, W: io::Write>(
    input: &mut R,
    output: &mut W,
) -> error::Result<()> {
    decode::lzma2::Lzma2Decoder::new().decompress(input, output)
}

/// Compress data with LZMA2 and default [`Options`](compress/struct.Options.html).
pub fn lzma2_compress<R: io::BufRead, W: io::Write>(
    input: &mut R,
    output: &mut W,
) -> io::Result<()> {
    encode::lzma2::encode_stream(input, output)
}

/// Decompress XZ data with default [`Options`](decompress/struct.Options.html).
pub fn xz_decompress<R: io::BufRead, W: io::Write>(
    input: &mut R,
    output: &mut W,
) -> error::Result<()> {
    decode::xz::decode_stream(input, output)
}

/// Compress data with XZ and default [`Options`](compress/struct.Options.html).
pub fn xz_compress<R: io::BufRead, W: io::Write>(input: &mut R, output: &mut W) -> io::Result<()> {
    encode::xz::encode_stream(input, output)
}
