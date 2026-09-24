use crate::error;
use std::io;

pub trait LzBuffer<W>
where
    W: io::Write,
{
    fn len(&self) -> usize;
    // Retrieve the last byte or return a default
    fn last_or(&self, lit: u8) -> u8;
    // Retrieve the n-th last byte
    fn last_n(&self, dist: usize) -> error::Result<u8>;
    // Append a literal
    fn append_literal(&mut self, lit: u8) -> error::Result<()>;
    // Fetch an LZ sequence (length, distance) from inside the buffer
    fn append_lz(&mut self, len: usize, dist: usize) -> error::Result<()>;
    // Get a reference to the output sink
    fn get_output(&self) -> &W;
    // Get a mutable reference to the output sink
    fn get_output_mut(&mut self) -> &mut W;
    // Consumes this buffer and flushes any data
    fn finish(self) -> io::Result<W>;
    // Consumes this buffer without flushing any data
    fn into_output(self) -> W;
}

// An accumulating buffer for LZ sequences
pub struct LzAccumBuffer<W>
where
    W: io::Write,
{
    stream: W,       // Output sink
    buf: Vec<u8>,    // Buffer
    memlimit: usize, // Buffer memory limit
    len: usize,      // Total number of bytes sent through the buffer
}

impl<W> LzAccumBuffer<W>
where
    W: io::Write,
{
    pub fn from_stream(stream: W, memlimit: usize) -> Self {
        Self {
            stream,
            buf: Vec::new(),
            memlimit,
            len: 0,
        }
    }

    // Append bytes
    pub fn append_bytes(&mut self, buf: &[u8]) {
        self.buf.extend_from_slice(buf);
        self.len += buf.len();
    }

    // Reset the internal dictionary
    pub fn reset(&mut self) -> io::Result<()> {
        self.stream.write_all(self.buf.as_slice())?;
        self.buf.clear();
        self.len = 0;
        Ok(())
    }
}

impl<W> LzBuffer<W> for LzAccumBuffer<W>
where
    W: io::Write,
{
    fn len(&self) -> usize {
        self.len
    }

    // Retrieve the last byte or return a default
    fn last_or(&self, lit: u8) -> u8 {
        let buf_len = self.buf.len();
        if buf_len == 0 {
            lit
        } else {
            self.buf[buf_len - 1]
        }
    }

    // Retrieve the n-th last byte
    fn last_n(&self, dist: usize) -> error::Result<u8> {
        let buf_len = self.buf.len();
        if dist > buf_len {
            return Err(error::Error::LzmaError(format!(
                "Match distance {} is beyond output size {}",
                dist, buf_len
            )));
        }

        Ok(self.buf[buf_len - dist])
    }

    // Append a literal
    fn append_literal(&mut self, lit: u8) -> error::Result<()> {
        let new_len = self.len + 1;

        if new_len > self.memlimit {
            Err(error::Error::LzmaError(format!(
                "exceeded memory limit of {}",
                self.memlimit
            )))
        } else {
            self.buf.push(lit);
            self.len = new_len;
            Ok(())
        }
    }

    // Fetch an LZ sequence (length, distance) from inside the buffer
    fn append_lz(&mut self, len: usize, dist: usize) -> error::Result<()> {
        lzma_debug!("LZ {{ len: {}, dist: {} }}", len, dist);
        let buf_len = self.buf.len();
        if dist > buf_len {
            return Err(error::Error::LzmaError(format!(
                "LZ distance {} is beyond output size {}",
                dist, buf_len
            )));
        }

        let mut offset = buf_len - dist;
        for _ in 0..len {
            let x = self.buf[offset];
            self.buf.push(x);
            offset += 1;
        }
        self.len += len;
        Ok(())
    }

    // Get a reference to the output sink
    fn get_output(&self) -> &W {
        &self.stream
    }

    // Get a mutable reference to the output sink
    fn get_output_mut(&mut self) -> &mut W {
        &mut self.stream
    }

    // Consumes this buffer and flushes any data
    fn finish(mut self) -> io::Result<W> {
        self.stream.write_all(self.buf.as_slice())?;
        self.stream.flush()?;
        Ok(self.stream)
    }

    // Consumes this buffer without flushing any data
    fn into_output(self) -> W {
        self.stream
    }
}

// A circular buffer for LZ sequences
pub struct LzCircularBuffer<W>
where
    W: io::Write,
{
    stream: W,        // Output sink
    buf: Vec<u8>,     // Circular buffer
    dict_size: usize, // Length of the buffer
    memlimit: usize,  // Buffer memory limit
    cursor: usize,    // Current position
    len: usize,       // Total number of bytes sent through the buffer
}

impl<W> LzCircularBuffer<W>
where
    W: io::Write,
{
    pub fn from_stream(stream: W, dict_size: usize, memlimit: usize) -> Self {
        lzma_info!("Dict size in LZ buffer: {}", dict_size);
        Self {
            stream,
            buf: Vec::new(),
            dict_size,
            memlimit,
            cursor: 0,
            len: 0,
        }
    }

    fn get(&self, index: usize) -> u8 {
        *self.buf.get(index).unwrap_or(&0)
    }

    fn set(&mut self, index: usize, value: u8) -> error::Result<()> {
        let new_len = index + 1;

        if self.buf.len() < new_len {
            if new_len <= self.memlimit {
                self.buf.resize(new_len, 0);
            } else {
                return Err(error::Error::LzmaError(format!(
                    "exceeded memory limit of {}",
                    self.memlimit
                )));
            }
        }
        self.buf[index] = value;
        Ok(())
    }
}

impl<W> LzBuffer<W> for LzCircularBuffer<W>
where
    W: io::Write,
{
    fn len(&self) -> usize {
        self.len
    }

    // Retrieve the last byte or return a default
    fn last_or(&self, lit: u8) -> u8 {
        if self.len == 0 {
            lit
        } else {
            self.get((self.dict_size + self.cursor - 1) % self.dict_size)
        }
    }

    // Retrieve the n-th last byte
    fn last_n(&self, dist: usize) -> error::Result<u8> {
        if dist > self.dict_size {
            return Err(error::Error::LzmaError(format!(
                "Match distance {} is beyond dictionary size {}",
                dist, self.dict_size
            )));
        }
        if dist > self.len {
            return Err(error::Error::LzmaError(format!(
                "Match distance {} is beyond output size {}",
                dist, self.len
            )));
        }

        let offset = (self.dict_size + self.cursor - dist) % self.dict_size;
        Ok(self.get(offset))
    }

    // Append a literal
    fn append_literal(&mut self, lit: u8) -> error::Result<()> {
        self.set(self.cursor, lit)?;
        self.cursor += 1;
        self.len += 1;

        // Flush the circular buffer to the output
        if self.cursor == self.dict_size {
            self.stream.write_all(self.buf.as_slice())?;
            self.cursor = 0;
        }

        Ok(())
    }

    // Fetch an LZ sequence (length, distance) from inside the buffer
    fn append_lz(&mut self, len: usize, dist: usize) -> error::Result<()> {
        lzma_debug!("LZ {{ len: {}, dist: {} }}", len, dist);
        if dist > self.dict_size {
            return Err(error::Error::LzmaError(format!(
                "LZ distance {} is beyond dictionary size {}",
                dist, self.dict_size
            )));
        }
        if dist > self.len {
            return Err(error::Error::LzmaError(format!(
                "LZ distance {} is beyond output size {}",
                dist, self.len
            )));
        }

        let mut offset = (self.dict_size + self.cursor - dist) % self.dict_size;
        for _ in 0..len {
            let x = self.get(offset);
            self.append_literal(x)?;
            offset += 1;
            if offset == self.dict_size {
                offset = 0
            }
        }
        Ok(())
    }

    // Get a reference to the output sink
    fn get_output(&self) -> &W {
        &self.stream
    }

    // Get a mutable reference to the output sink
    fn get_output_mut(&mut self) -> &mut W {
        &mut self.stream
    }

    // Consumes this buffer and flushes any data
    fn finish(mut self) -> io::Result<W> {
        if self.cursor > 0 {
            self.stream.write_all(&self.buf[0..self.cursor])?;
            self.stream.flush()?;
        }
        Ok(self.stream)
    }

    // Consumes this buffer without flushing any data
    fn into_output(self) -> W {
        self.stream
    }
}
