use super::{compress, decompress};

const TEXT: &str = include_str!("../../assets/example.txt");

#[test]
fn test_direct() {
    // Can we include_str!("assets/example.txt")?
    // It's excluded from the packaging step, so maybe not.
    crate::test_cycle_unwrap(
        TEXT.as_bytes(),
        |data| compress(data, 1),
        |data| decompress(data, TEXT.len()),
    );
}

#[test]
fn test_stream_compat() {
    // We can bulk-compress and stream-decode
    crate::test_cycle_unwrap(
        TEXT.as_bytes(),
        |data| compress(data, 1),
        |data| crate::decode_all(data),
    );

    // We can stream-encode and bulk-decompress
    crate::test_cycle_unwrap(
        TEXT.as_bytes(),
        |data| crate::encode_all(data, 1),
        |data| decompress(data, TEXT.len()),
    );
}

#[test]
fn has_content_size() {
    let compressed = compress(TEXT.as_bytes(), 1).unwrap();

    // Bulk functions by default include the content size.
    assert_eq!(
        zstd_safe::get_frame_content_size(&compressed).unwrap(),
        Some(TEXT.len() as u64)
    );
}

/// A job that fails must not leave the compressor refusing to be set up for
/// the next one.
#[test]
fn test_compressor_is_reusable_after_a_failed_job() {
    let dictionary = crate::dict::EncoderDictionary::copy(b"another", 3);
    let text = TEXT.as_bytes();

    let mut compressor = super::Compressor::new(1).unwrap();
    let fail = |compressor: &mut super::Compressor<'_>| {
        let mut small = Vec::with_capacity(16);
        compressor.compress_to_buffer(text, &mut small).unwrap_err();
    };

    fail(&mut compressor);
    compressor
        .set_pledged_src_size(Some(text.len() as u64))
        .unwrap();
    compressor.include_checksum(true).unwrap();
    compressor.set_compression_level(3).unwrap();

    fail(&mut compressor);
    compressor.set_dictionary(3, b"some dictionary").unwrap();

    fail(&mut compressor);
    compressor.set_prepared_dictionary(&dictionary).unwrap();

    fail(&mut compressor);
    compressor.set_dictionary(1, &[]).unwrap();

    let compressed = compressor.compress(text).unwrap();
    assert_eq!(decompress(&compressed, text.len()).unwrap(), text);
}

/// With worker threads, a failed job may leave them reading the dictionary:
/// replacing it right after waits for them.
#[cfg(feature = "zstdmt")]
#[test]
fn test_multithreaded_compressor_after_a_failed_job() {
    use crate::test_words as words;

    let dictionary = words(1 << 20, 1);
    let another = words(64 << 10, 2);
    let input = words(24 << 20, 3);

    for _ in 0..3 {
        let mut compressor =
            super::Compressor::with_dictionary(1, &dictionary).unwrap();
        compressor.multithread(2).unwrap();
        compressor
            .compress_to_buffer(&input, &mut Vec::with_capacity(0))
            .unwrap_err();
        compressor.set_dictionary(1, &another).unwrap();
        compressor.multithread(1).unwrap();

        let compressed = compressor.compress(&input).unwrap();
        let mut decompressor =
            super::Decompressor::with_dictionary(&another).unwrap();
        assert!(
            decompressor.decompress(&compressed, input.len()).unwrap()
                == input
        );

        // Dropping it in the middle of a failed job is fine too.
        compressor
            .compress_to_buffer(&input, &mut Vec::with_capacity(0))
            .unwrap_err();
    }
}
