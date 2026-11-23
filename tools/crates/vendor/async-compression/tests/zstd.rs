#[macro_use]
mod utils;

test_cases!(zstd);

#[cfg(any(feature = "tokio", feature = "futures-io"))]
mod pending {
    use std::{io, task::Poll};

    // Repeated polling while waiting for input must not repeatedly call zstd
    // with empty input: after 16 no-progress calls its context becomes unusable.
    fn chunks(
        input: &[u8],
        chunk_size: usize,
    ) -> impl ::futures::Stream<Item = io::Result<Vec<u8>>> + '_ {
        let mut chunks = input.chunks(chunk_size);
        let mut pending = 32;
        ::futures::stream::poll_fn(move |cx| {
            if pending > 0 {
                pending -= 1;
                cx.waker().wake_by_ref();
                return Poll::Pending;
            }
            pending = 32;
            Poll::Ready(chunks.next().map(|chunk| Ok(chunk.to_vec())))
        })
    }

    macro_rules! pending_tests {
        ($runtime:ident, $feature:literal, |$stream:ident| $reader:block) => {
            #[cfg(feature = $feature)]
            mod $runtime {
                use super::chunks;
                use ::$runtime::io::AsyncReadExt as _;
                use async_compression::$runtime::bufread::ZstdDecoder;
                use std::io;

                async fn decode(
                    input: &[u8],
                    chunk_size: usize,
                    output_size: usize,
                    multiple_members: bool,
                ) -> io::Result<Vec<u8>> {
                    let $stream = chunks(input, chunk_size);
                    let reader = $reader;
                    let mut decoder = ZstdDecoder::new(reader);
                    decoder.multiple_members(multiple_members);
                    let mut output = Vec::new();
                    let mut buffer = vec![0; output_size];
                    loop {
                        let len = decoder.read(&mut buffer).await?;
                        if len == 0 {
                            return Ok(output);
                        }
                        output.extend_from_slice(&buffer[..len]);
                    }
                }

                #[test]
                #[ntest::timeout(10000)]
                fn repeated_pending() {
                    ::futures::executor::block_on(async {
                        let data: Vec<u8> = (0u32..4096)
                            .flat_map(|i| i.wrapping_mul(2654435761).to_le_bytes())
                            .collect();
                        for data in [vec![], vec![b'x'; 65536], data] {
                            let compressed = libzstd::encode_all(data.as_slice(), 0).unwrap();
                            for chunk_size in [1, 1024] {
                                for output_size in [1, 8192] {
                                    for multiple_members in [false, true] {
                                        let output = decode(
                                            &compressed,
                                            chunk_size,
                                            output_size,
                                            multiple_members,
                                        )
                                        .await
                                        .unwrap();
                                        assert_eq!(output, data);
                                    }
                                }
                            }
                        }
                    });
                }

                #[test]
                #[ntest::timeout(10000)]
                fn repeated_pending_between_members() {
                    ::futures::executor::block_on(async {
                        let members = [vec![b'a'; 65536], vec![], vec![b'b'; 65536]];
                        let compressed: Vec<u8> = members
                            .iter()
                            .flat_map(|data| libzstd::encode_all(data.as_slice(), 0).unwrap())
                            .collect();
                        let output = decode(&compressed, 1, 7, true).await.unwrap();
                        assert_eq!(output, members.concat());
                    });
                }

                #[test]
                #[ntest::timeout(10000)]
                fn repeated_pending_truncated_input() {
                    ::futures::executor::block_on(async {
                        let compressed = libzstd::encode_all(&[b'a'; 65536][..], 0).unwrap();
                        for end in [1, compressed.len() / 2, compressed.len() - 1] {
                            let error =
                                decode(&compressed[..end], 1, 8192, true).await.unwrap_err();
                            assert_eq!(error.kind(), io::ErrorKind::UnexpectedEof);
                        }
                    });
                }

                #[test]
                #[ntest::timeout(10000)]
                fn repeated_pending_invalid_input() {
                    ::futures::executor::block_on(async {
                        assert!(decode(b"not a zstd frame", 1, 8192, true).await.is_err());
                    });
                }
            }
        };
    }

    pending_tests!(tokio, "tokio", |stream| {
        use ::futures::TryStreamExt as _;
        tokio_util::io::StreamReader::new(stream.map_ok(std::io::Cursor::new))
    });
    pending_tests!(futures, "futures-io", |stream| {
        use ::futures::TryStreamExt as _;
        stream.into_async_read()
    });
}
