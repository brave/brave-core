use crc32fast::Hasher;
use lzma_rs::decompress::raw::Lzma2Decoder;
use std::{
    collections::VecDeque,
    io::{BufRead, Error, Read, Result, Write},
};

#[derive(Debug)]
pub struct XzDecoder<R: BufRead> {
    compressed_reader: R,
    stream_size: usize,
    buf: VecDeque<u8>,
    check_size: usize,
    records: Vec<(usize, usize)>,
    flags: [u8; 2],
}

impl<R: BufRead> XzDecoder<R> {
    pub fn new(inner: R) -> Self {
        XzDecoder {
            compressed_reader: inner,
            stream_size: 0,
            buf: VecDeque::new(),
            check_size: 0,
            records: vec![],
            flags: [0, 0],
        }
    }
}

struct CountReader<'a, R: BufRead> {
    inner: &'a mut R,
    count: &'a mut usize,
}

impl<R: BufRead> Read for CountReader<'_, R> {
    fn read(&mut self, buf: &mut [u8]) -> Result<usize> {
        let count = self.inner.read(buf)?;
        *self.count += count;
        Ok(count)
    }
}

impl<R: BufRead> BufRead for CountReader<'_, R> {
    fn fill_buf(&mut self) -> Result<&[u8]> {
        self.inner.fill_buf()
    }

    fn consume(&mut self, amt: usize) {
        self.inner.consume(amt);
        *self.count += amt;
    }
}

struct BufWriter<'a> {
    inner: &'a mut [u8],
    written: &'a mut usize,
    total: &'a mut usize,
    rest: &'a mut VecDeque<u8>,
}

impl Write for BufWriter<'_> {
    fn write(&mut self, buf: &[u8]) -> Result<usize> {
        if self.inner.len() > *self.written {
            let len = std::cmp::min(buf.len(), self.inner.len() - *self.written);
            self.inner[*self.written..*self.written + len].copy_from_slice(&buf[..len]);
            *self.written += len;
            *self.total += len;
            Ok(len)
        } else {
            self.rest.extend(buf.iter());
            *self.total += buf.len();
            Ok(buf.len())
        }
    }

    fn flush(&mut self) -> Result<()> {
        Ok(())
    }
}

fn error<T>(s: &'static str) -> Result<T> {
    Err(Error::new(std::io::ErrorKind::InvalidData, s))
}

fn get_multibyte<R: BufRead>(input: &mut R, hasher: &mut Hasher) -> Result<u64> {
    let mut result = 0;
    for i in 0..9 {
        let mut b = [0u8; 1];
        input.read_exact(&mut b)?;
        hasher.update(&b);
        let b = b[0];
        result ^= ((b & 0x7F) as u64) << (i * 7);
        if (b & 0x80) == 0 {
            return Ok(result);
        }
    }
    error("Invalid multi-byte encoding")
}

impl<R: BufRead> Read for XzDecoder<R> {
    fn read(&mut self, buf: &mut [u8]) -> Result<usize> {
        if !self.buf.is_empty() {
            let len = std::cmp::min(buf.len(), self.buf.len());
            buf[..len].copy_from_slice(&self.buf.as_slices().0[..len]);
            self.buf.drain(..len);
            return Ok(len);
        }
        let mut reader = CountReader {
            inner: &mut self.compressed_reader,
            count: &mut self.stream_size,
        };
        if *reader.count == 0 {
            let mut b = [0u8; 12];
            match reader.read(&mut b) {
                Ok(0) => return Ok(0),
                Err(e) => return Err(e),
                _ => (),
            }
            if b[..6] != b"\xFD7zXZ\0"[..] {
                return error("Invalid XZ header");
            }
            self.flags = [b[6], b[7]];
            if self.flags[0] != 0 || self.flags[1] & 0xF0 != 0 {
                return error("Invalid XZ stream flags");
            }
            match self.flags[1] & 0x0F {
                0 => self.check_size = 0,
                1 => self.check_size = 4,
                _ => return error("Unsupported XZ stream flags"),
            }
            let mut digest = Hasher::new();
            digest.update(&self.flags);
            if digest.finalize().to_le_bytes() != b[8..] {
                return error("Invalid XZ stream flags CRC32");
            }
        }

        let block_begin = *reader.count;
        let mut b = [0u8; 1];
        reader.read_exact(&mut b)?;

        let mut digest = Hasher::new();
        digest.update(&b);
        if b[0] == 0 {
            // index
            let num_records = get_multibyte(&mut reader, &mut digest)?;
            if num_records != self.records.len() as u64 {
                return error("Invalid XZ index record count");
            }
            for (unpadded_size, total) in &self.records {
                if get_multibyte(&mut reader, &mut digest)? != *unpadded_size as u64 {
                    return error("Invalid XZ unpadded size");
                }
                if get_multibyte(&mut reader, &mut digest)? != *total as u64 {
                    return error("Invalid XZ uncompressed size");
                }
            }
            let mut size = *reader.count - block_begin;
            let mut b = vec![0u8; (4 - (size & 0x3)) & 0x3];
            reader.read_exact(b.as_mut_slice())?;
            if !b.iter().all(|&b| b == 0) {
                return error("Invalid XZ index padding");
            }
            digest.update(b.as_slice());
            size += b.len();
            let mut b = [0u8; 16];
            reader.read_exact(&mut b)?;
            if digest.finalize().to_le_bytes() != b[..4] {
                return error("Invalid XZ index CRC32");
            }
            let mut digest = Hasher::new();
            digest.update(&b[8..14]);
            if digest.finalize().to_le_bytes() != b[4..8] {
                return error("Invalid XZ footer CRC32");
            }
            if b[8..12] != ((size >> 2) as u32).to_le_bytes() {
                return error("Invalid XZ footer size");
            }
            if self.flags != b[12..14] {
                return error("Invalid XZ footer flags");
            }
            if &b[14..16] != b"YZ" {
                return error("Invalid XZ footer magic");
            }
            let mut b = vec![0u8; (4 - (*reader.count & 0x3)) & 0x3];
            reader.read_exact(b.as_mut_slice())?;
            if !b.iter().all(|&b| b == 0) {
                return error("Invalid XZ footer padding");
            }
            *reader.count = 0;
            return self.read(buf);
        }

        // block
        let header_end = ((b[0] as usize) << 2) - 1 + *reader.count;
        let mut b = [0u8; 1];
        reader.read_exact(&mut b)?;
        digest.update(&b);
        let flags = b[0];
        let num_filters = (flags & 0x03) + 1;

        if flags & 0x3C != 0 {
            return error("Invalid XZ block flags");
        }
        if flags & 0x40 != 0 {
            get_multibyte(&mut reader, &mut digest)?;
        }
        if flags & 0x80 != 0 {
            get_multibyte(&mut reader, &mut digest)?;
        }
        for _ in 0..num_filters {
            let filter_id = get_multibyte(&mut reader, &mut digest)?;
            if filter_id != 0x21 {
                return error("Unsupported XZ filter ID");
            }
            let properties_size = get_multibyte(&mut reader, &mut digest)?;
            if properties_size != 1 {
                return error("Unsupported XZ filter properties size");
            }
            reader.read_exact(&mut b)?;
            if b[0] & 0xC0 != 0 {
                return error("Unsupported XZ filter properties");
            }
            digest.update(&b);
        }
        let Some(padding_bytes) = header_end.checked_sub(*reader.count) else {
            return error("Invalid XZ block header (too short)");
        };
        let mut b = vec![0u8; padding_bytes];
        reader.read_exact(b.as_mut_slice())?;
        if !b.iter().all(|&b| b == 0) {
            return error("Invalid XZ block header padding");
        }
        digest.update(b.as_slice());

        let mut b = [0u8; 4];
        reader.read_exact(&mut b)?;
        if digest.finalize().to_le_bytes() != b {
            return error("Invalid XZ block header CRC32");
        }
        let mut written = 0;
        let mut total = 0;
        Lzma2Decoder::new().decompress(
            &mut reader,
            &mut BufWriter {
                inner: buf,
                written: &mut written,
                rest: &mut self.buf,
                total: &mut total,
            },
        )?;

        let unpadded_size = *reader.count - block_begin;
        self.records.push((unpadded_size, total));
        // ignore check here since zip itself will check it
        let mut b = vec![0u8; ((4 - (unpadded_size & 0x3)) & 0x3) + self.check_size];
        reader.read_exact(b.as_mut_slice())?;
        if !b.as_slice()[..self.check_size].iter().all(|&b| b == 0) {
            return error("Invalid XZ block padding");
        }
        Ok(written)
    }
}

impl<R: BufRead> XzDecoder<R> {
    pub fn into_inner(self) -> R {
        self.compressed_reader
    }
}
