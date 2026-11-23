use async_compression::tokio::{bufread, write};
use std::io::ErrorKind;
use tokio::io::{AsyncReadExt, AsyncWriteExt};

#[tokio::test]
async fn truncated_stream_returns_unexpected_eof_on_read_and_shutdown() {
    let input = tokio::io::BufReader::new(&[0x00][..]);
    let mut decoder = bufread::Deflate64Decoder::new(input);
    let mut output = Vec::new();

    let error = decoder.read_to_end(&mut output).await.unwrap_err();
    assert_eq!(error.kind(), ErrorKind::UnexpectedEof);

    let mut decoder = write::Deflate64Decoder::new(Vec::new());
    decoder.write_all(&[0x00]).await.unwrap();

    let error = decoder.shutdown().await.unwrap_err();
    assert_eq!(error.kind(), ErrorKind::UnexpectedEof);
}
