extern crate std;
use crate as zstd_safe;

use self::std::vec::Vec;

const INPUT: &[u8] = b"Rust is a multi-paradigm system programming language focused on safety, especially safe concurrency. Rust is syntactically similar to C++, but is designed to provide better memory safety while maintaining high performance.";
const LONG_CONTENT: &str = include_str!("lib.rs");

#[cfg(feature = "std")]
#[test]
fn test_writebuf() {
    use zstd_safe::WriteBuf;

    let mut data = Vec::with_capacity(10);
    unsafe {
        data.write_from(|ptr, n| {
            assert!(n >= 4);
            let ptr = ptr as *mut u8;
            ptr.write(0);
            ptr.add(1).write(1);
            ptr.add(2).write(2);
            ptr.add(3).write(3);
            Ok(4)
        })
    }
    .unwrap();
    assert_eq!(data.as_slice(), &[0, 1, 2, 3]);

    let mut cursor = std::io::Cursor::new(&mut data);
    // Here we use a position larger than the actual data.
    // So expect the data to be zero-filled.
    cursor.set_position(6);
    unsafe {
        cursor.write_from(|ptr, n| {
            assert!(n >= 4);
            let ptr = ptr as *mut u8;
            ptr.write(4);
            ptr.add(1).write(5);
            ptr.add(2).write(6);
            ptr.add(3).write(7);
            Ok(4)
        })
    }
    .unwrap();

    assert_eq!(data.as_slice(), &[0, 1, 2, 3, 0, 0, 4, 5, 6, 7]);
}

#[cfg(feature = "std")]
#[test]
fn test_simple_cycle() {
    let mut buffer = std::vec![0u8; 256];
    let written = zstd_safe::compress(&mut buffer, INPUT, 3).unwrap();
    let compressed = &buffer[..written];

    let mut buffer = std::vec![0u8; 256];
    let written = zstd_safe::decompress(&mut buffer, compressed).unwrap();
    let decompressed = &buffer[..written];

    assert_eq!(INPUT, decompressed);
}

#[test]
fn test_cctx_cycle() {
    let mut buffer = std::vec![0u8; 256];
    let mut cctx = zstd_safe::CCtx::default();
    let written = cctx.compress(&mut buffer[..], INPUT, 1).unwrap();
    let compressed = &buffer[..written];

    let mut dctx = zstd_safe::DCtx::default();
    let mut buffer = std::vec![0u8; 256];
    let written = dctx.decompress(&mut buffer[..], compressed).unwrap();
    let decompressed = &buffer[..written];

    assert_eq!(INPUT, decompressed);
}

#[test]
fn test_dictionary() {
    // Prepare some content to train the dictionary.
    let bytes = LONG_CONTENT.as_bytes();
    let line_sizes: Vec<usize> =
        LONG_CONTENT.lines().map(|line| line.len() + 1).collect();

    // Train the dictionary
    let mut dict_buffer = std::vec![0u8; 100_000];
    let written =
        zstd_safe::train_from_buffer(&mut dict_buffer[..], bytes, &line_sizes)
            .unwrap();
    let dict_buffer = &dict_buffer[..written];

    // Create pre-hashed dictionaries for (de)compression
    let cdict = zstd_safe::create_cdict(dict_buffer, 3);
    let ddict = zstd_safe::create_ddict(dict_buffer);

    // Compress data
    let mut cctx = zstd_safe::CCtx::default();
    cctx.ref_cdict(&cdict).unwrap();

    let mut buffer = std::vec![0u8; 1024 * 1024];
    // First, try to compress without a dict
    let big_written = zstd_safe::compress(&mut buffer[..], bytes, 3).unwrap();

    let written = cctx
        .compress2(&mut buffer[..], bytes)
        .map_err(zstd_safe::get_error_name)
        .unwrap();

    assert!(big_written > written);
    let compressed = &buffer[..written];

    // Decompress data
    let mut dctx = zstd_safe::DCtx::default();
    dctx.ref_ddict(&ddict).unwrap();

    let mut buffer = std::vec![0u8; 1024 * 1024];
    let written = dctx
        .decompress(&mut buffer[..], compressed)
        .map_err(zstd_safe::get_error_name)
        .unwrap();
    let decompressed = &buffer[..written];

    // Profit!
    assert_eq!(bytes, decompressed);
}

#[test]
fn test_checksum() {
    let mut buffer = std::vec![0u8; 256];
    let mut cctx = zstd_safe::CCtx::default();
    cctx.set_parameter(zstd_safe::CParameter::ChecksumFlag(true))
        .unwrap();
    let written = cctx.compress2(&mut buffer[..], INPUT).unwrap();
    let compressed = &mut buffer[..written];

    let mut dctx = zstd_safe::DCtx::default();
    let mut buffer = std::vec![0u8; 1024*1024];
    let written = dctx
        .decompress(&mut buffer[..], compressed)
        .map_err(zstd_safe::get_error_name)
        .unwrap();
    let decompressed = &buffer[..written];

    assert_eq!(INPUT, decompressed);

    // Now try again with some corruption
    // TODO: Find a mutation that _wouldn't_ be detected without checksums.
    // (Most naive changes already trigger a "corrupt block" error.)
    if let Some(last) = compressed.last_mut() {
        *last = last.saturating_sub(1);
    }
    let err = dctx
        .decompress(&mut buffer[..], compressed)
        .map_err(zstd_safe::get_error_name)
        .err()
        .unwrap();
    // The error message will complain about the checksum.
    assert!(err.contains("checksum"));
}

#[cfg(all(feature = "experimental", feature = "std"))]
#[test]
fn test_upper_bound() {
    let mut buffer = std::vec![0u8; 256];

    assert!(zstd_safe::decompress_bound(&buffer).is_err());

    let written = zstd_safe::compress(&mut buffer, INPUT, 3).unwrap();
    let compressed = &buffer[..written];

    assert_eq!(
        zstd_safe::decompress_bound(&compressed),
        Ok(INPUT.len() as u64)
    );
}
