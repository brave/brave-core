use lzma_rs::decompress::{Options, Stream, UnpackedSize};
use std::collections::VecDeque;
use std::io::{copy, Error, Read, Result, Write};

const COMPRESSED_BYTES_TO_BUFFER: usize = 4096;

const OPTIONS: Options = Options {
    unpacked_size: UnpackedSize::ReadFromHeader,
    memlimit: None,
    allow_incomplete: true,
};

#[derive(Debug)]
pub struct LzmaDecoder<R> {
    compressed_reader: R,
    stream: Stream<VecDeque<u8>>,
}

impl<R: Read> LzmaDecoder<R> {
    pub fn new(inner: R) -> Self {
        LzmaDecoder {
            compressed_reader: inner,
            stream: Stream::new_with_options(&OPTIONS, VecDeque::new()),
        }
    }

    pub fn finish(mut self) -> Result<VecDeque<u8>> {
        copy(&mut self.compressed_reader, &mut self.stream)?;
        self.stream.finish().map_err(Error::from)
    }
}

impl<R: Read> Read for LzmaDecoder<R> {
    fn read(&mut self, buf: &mut [u8]) -> Result<usize> {
        let mut bytes_read = self.stream.get_output_mut().unwrap().read(buf)?;
        while bytes_read < buf.len() {
            let mut next_compressed = [0u8; COMPRESSED_BYTES_TO_BUFFER];
            let compressed_bytes_read = self.compressed_reader.read(&mut next_compressed)?;
            if compressed_bytes_read == 0 {
                break;
            }
            self.stream
                .write_all(&next_compressed[..compressed_bytes_read])?;
            bytes_read += self
                .stream
                .get_output_mut()
                .unwrap()
                .read(&mut buf[bytes_read..])?;
        }
        Ok(bytes_read)
    }
}
