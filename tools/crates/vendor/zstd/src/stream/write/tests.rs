use std::io::{Cursor, Write};
use std::iter;

use partial_io::{PartialOp, PartialWrite};

use crate::stream::decode_all;
use crate::stream::write::{Decoder, Encoder};

#[test]
fn test_cycle() {
    let input = b"Abcdefghabcdefgh";

    let buffer = Cursor::new(Vec::new());
    let mut encoder = Encoder::new(buffer, 1).unwrap();
    encoder.write_all(input).unwrap();
    let encoded = encoder.finish().unwrap().into_inner();

    // println!("Encoded: {:?}", encoded);

    let buffer = Cursor::new(Vec::new());
    let mut decoder = Decoder::new(buffer).unwrap();
    decoder.write_all(&encoded).unwrap();
    decoder.flush().unwrap();
    let decoded = decoder.into_inner().into_inner();

    assert_eq!(input, &decoded[..]);
}

/// Test that flush after a partial write works successfully without
/// corrupting the frame. This test is in this module because it checks
/// internal implementation details.
#[test]
fn test_partial_write_flush() {
    let input = vec![b'b'; 128 * 1024];
    let mut z = setup_partial_write(&input);

    // flush shouldn't corrupt the stream
    z.flush().unwrap();

    let buf = z.finish().unwrap().into_inner();
    assert_eq!(&decode_all(&buf[..]).unwrap(), &input);
}

/// Test that finish after a partial write works successfully without
/// corrupting the frame. This test is in this module because it checks
/// internal implementation details.
#[test]
fn test_partial_write_finish() {
    let input = vec![b'b'; 128 * 1024];
    let z = setup_partial_write(&input);

    // finish shouldn't corrupt the stream
    let buf = z.finish().unwrap().into_inner();
    assert_eq!(&decode_all(&buf[..]).unwrap(), &input);
}

fn setup_partial_write(input_data: &[u8]) -> Encoder<PartialWrite<Vec<u8>>> {
    let buf =
        PartialWrite::new(Vec::new(), iter::repeat(PartialOp::Limited(1)));
    let mut z = Encoder::new(buf, 1).unwrap();

    // Fill in enough data to make sure the buffer gets written out.
    z.write(input_data).unwrap();

    {
        let inner = &mut z.writer;
        // At this point, the internal buffer in z should have some data.
        assert_ne!(inner.offset(), inner.buffer().len());
    }

    z
}

/// Test that a panic from the writer can be caught: the wrappers must not hand
/// it the same data again on drop, or the second panic aborts the process.
#[test]
fn test_auto_wrappers_panicking_writer() {
    struct Panicking;

    impl Write for Panicking {
        fn write(&mut self, _: &[u8]) -> std::io::Result<usize> {
            panic!("this writer panics");
        }

        fn flush(&mut self) -> std::io::Result<()> {
            Ok(())
        }
    }

    let input = vec![b'b'; 128 * 1024];
    let encoded = crate::encode_all(&input[..], 1).unwrap();

    std::panic::catch_unwind(|| {
        let mut decoder = Decoder::new(Panicking).unwrap().auto_flush();
        decoder.write_all(&encoded)
    })
    .unwrap_err();

    std::panic::catch_unwind(|| {
        let mut encoder = Encoder::new(Panicking, 1).unwrap().auto_finish();
        encoder.write_all(&input).and_then(|()| encoder.flush())
    })
    .unwrap_err();
}

/// Test that an input which stops in the middle of a frame is reported by
/// `finish()`, like it is when reading.
#[test]
fn test_decoder_finish_incomplete_frame() {
    use std::io::ErrorKind::UnexpectedEof;

    let input = b"Abcdefghabcdefgh";
    let frame = crate::encode_all(&input[..], 1).unwrap();
    // Concatenated frames: only a cut at a frame boundary is complete.
    let encoded = [&frame[..], &frame[..]].concat();

    for cut in 0..=encoded.len() {
        let complete = cut != 0 && cut % frame.len() == 0;

        let mut decoder = Decoder::new(Vec::new()).unwrap();
        decoder.write_all(&encoded[..cut]).unwrap();
        match decoder.finish() {
            Ok(decoded) => {
                assert!(complete, "cut at {} accepted", cut);
                assert_eq!(decoded, input.repeat(cut / frame.len()));
            }
            Err(e) => {
                assert!(!complete, "cut at {} rejected: {}", cut, e);
                assert_eq!(e.kind(), UnexpectedEof);
            }
        }
    }
}

/// Test that what could be decoded is flushed, and that the stream can be
/// resumed, when `do_finish()` finds an incomplete frame.
#[test]
fn test_decoder_do_finish_resume() {
    // Larger than the 32KB output buffer.
    let input = vec![b'b'; 128 * 1024];
    let encoded = crate::encode_all(&input[..], 1).unwrap();
    let (head, tail) = encoded.split_at(encoded.len() - 1);

    let mut flushed = Decoder::new(Vec::new()).unwrap();
    flushed.write_all(head).unwrap();
    flushed.flush().unwrap();

    let mut decoder = Decoder::new(Vec::new()).unwrap();
    decoder.write_all(head).unwrap();
    decoder.do_finish().unwrap_err();
    assert_eq!(decoder.get_ref(), flushed.get_ref());

    decoder.write_all(tail).unwrap();
    // An empty write must not start a new frame.
    assert_eq!(decoder.write(&[]).unwrap(), 0);
    decoder.do_finish().unwrap();
    decoder.write_all(&encoded).unwrap_err();
    assert_eq!(decoder.finish().unwrap(), input);
}

/// Dropping a multithreaded encoder without finishing it waits for the
/// worker threads before freeing the dictionary they read.
#[cfg(feature = "zstdmt")]
#[test]
fn test_drop_unfinished_multithreaded_encoder() {
    use crate::test_words as words;

    let dictionary = words(1 << 20, 1);
    let input = words(24 << 20, 2);
    for _ in 0..3 {
        let mut encoder =
            Encoder::with_dictionary(Vec::new(), 3, &dictionary).unwrap();
        encoder.multithread(2).unwrap();
        encoder.write_all(&input).unwrap();
        drop(encoder);
    }
}
