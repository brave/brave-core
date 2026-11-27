#![cfg(async_compression_unstable)]

use compression_codecs::mbrotli::params::EncoderParams;

#[macro_use]
mod utils;

test_cases!(mbrotli);

#[test]
pub fn mbrotli_params() {
    let _ = EncoderParams::default();
}

#[cfg(feature = "tokio")]
mod params {
    use async_compression::{mbrotli::EncoderParams, tokio::bufread::MbrotliEncoder, Level};
    use tokio::io::AsyncReadExt as _;

    use crate::utils::{algos::mbrotli::sync, block_on};

    fn input() -> Vec<u8> {
        b"the quick brown fox jumps over the lazy dog\n".repeat(1024)
    }

    fn compress(input: &[u8], params: EncoderParams) -> Vec<u8> {
        let mut encoder = MbrotliEncoder::with_params(input, params);
        let mut output = Vec::new();
        block_on(encoder.read_to_end(&mut output)).unwrap();
        output
    }

    #[test]
    #[ntest::timeout(10000)]
    fn every_quality_roundtrips() {
        let input = input();
        for quality in -1..=12 {
            let params = EncoderParams::default().quality(Level::Precise(quality));
            let compressed = compress(&input, params);
            assert_eq!(sync::decompress(&compressed), input, "quality {quality}");
        }
    }

    #[test]
    #[ntest::timeout(10000)]
    fn zero_window_size_selects_default_window() {
        let input = input();
        let default = compress(&input, EncoderParams::default());
        let zero = compress(&input, EncoderParams::default().window_size(0));
        let smallest = compress(&input, EncoderParams::default().window_size(1));

        assert_eq!(zero, default);
        assert_ne!(smallest, default);
    }

    #[test]
    #[ntest::timeout(10000)]
    fn tuned_params_roundtrip() {
        let input = input();
        let params = EncoderParams::default()
            .quality(Level::Default)
            .window_size(0)
            .block_size(100)
            .size_hint(input.len())
            .text_mode();
        let compressed = compress(&input, params);
        assert_eq!(sync::decompress(&compressed), input);

        let params = EncoderParams::default()
            .quality(Level::Fastest)
            .window_size(100)
            .font_mode();
        let compressed = compress(&input, params);
        assert_eq!(sync::decompress(&compressed), input);
    }
}

/// Explicit interoperability with the `brotli` crate in both directions: streams written by
/// `MbrotliEncoder` must decode with `brotli`, and streams written by `brotli` must decode with
/// `MbrotliDecoder`.
#[cfg(feature = "tokio")]
mod brotli_interop {
    use async_compression::{
        mbrotli::EncoderParams,
        tokio::bufread::{MbrotliDecoder, MbrotliEncoder},
        Level,
    };
    use brotli::{enc::backward_references::BrotliEncoderParams, CompressorReader, Decompressor};
    use std::io::Read as _;
    use tokio::io::AsyncReadExt as _;

    use crate::utils::block_on;

    fn inputs() -> Vec<(&'static str, Vec<u8>)> {
        // Deterministic pseudo-random bytes, so the stream contains poorly compressible data.
        let mut state = 0x2545_f491_4f6c_dd1d_u64;
        let noise = (0..256 * 1024)
            .map(|_| {
                state ^= state << 13;
                state ^= state >> 7;
                state ^= state << 17;
                state as u8
            })
            .collect();

        vec![
            ("empty", Vec::new()),
            ("single byte", vec![42]),
            (
                "text",
                b"the quick brown fox jumps over the lazy dog\n".repeat(1024),
            ),
            ("noise", noise),
        ]
    }

    fn mbrotli_compress(input: &[u8], params: EncoderParams) -> Vec<u8> {
        let mut output = Vec::new();
        block_on(MbrotliEncoder::with_params(input, params).read_to_end(&mut output)).unwrap();
        output
    }

    fn mbrotli_decompress(input: &[u8]) -> Vec<u8> {
        let mut output = Vec::new();
        block_on(MbrotliDecoder::new(input).read_to_end(&mut output)).unwrap();
        output
    }

    fn brotli_compress(input: &[u8], quality: i32, lgwin: i32) -> Vec<u8> {
        let params = BrotliEncoderParams {
            quality,
            lgwin,
            ..Default::default()
        };
        let mut output = Vec::new();
        CompressorReader::with_params(input, 4096, &params)
            .read_to_end(&mut output)
            .unwrap();
        output
    }

    fn brotli_decompress(input: &[u8]) -> Vec<u8> {
        let mut output = Vec::new();
        Decompressor::new(input, 4096)
            .read_to_end(&mut output)
            .unwrap();
        output
    }

    #[test]
    #[ntest::timeout(60000)]
    fn mbrotli_to_brotli() {
        for (name, input) in inputs() {
            for quality in 0..=11 {
                for window in [10, 16, 22, 24] {
                    let params = EncoderParams::default()
                        .quality(Level::Precise(quality))
                        .window_size(window);
                    let compressed = mbrotli_compress(&input, params);
                    assert_eq!(
                        brotli_decompress(&compressed),
                        input,
                        "{name}, quality {quality}, window {window}"
                    );
                }
            }
        }
    }

    #[test]
    #[ntest::timeout(60000)]
    fn brotli_to_mbrotli() {
        for (name, input) in inputs() {
            for quality in 0..=11 {
                for lgwin in [10, 16, 22, 24] {
                    let compressed = brotli_compress(&input, quality, lgwin);
                    assert_eq!(
                        mbrotli_decompress(&compressed),
                        input,
                        "{name}, quality {quality}, lgwin {lgwin}"
                    );
                }
            }
        }
    }
}
