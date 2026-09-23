use std::io;

pub fn read_tag<R: io::BufRead>(input: &mut R, tag: &[u8]) -> io::Result<bool> {
    let mut buf = vec![0; tag.len()];
    input.read_exact(buf.as_mut_slice())?;
    Ok(buf.as_slice() == tag)
}

pub fn is_eof<R: io::BufRead>(input: &mut R) -> io::Result<bool> {
    let buf = input.fill_buf()?;
    Ok(buf.is_empty())
}

pub fn flush_zero_padding<R: io::BufRead>(input: &mut R) -> io::Result<bool> {
    loop {
        let len = {
            let buf = input.fill_buf()?;
            let len = buf.len();

            if len == 0 {
                return Ok(true);
            }

            for x in buf {
                if *x != 0u8 {
                    return Ok(false);
                }
            }
            len
        };

        input.consume(len);
    }
}

// A Read computing a digest on the bytes read.
pub struct CrcDigestRead<'a, 'b, R, S>
where
    R: 'a + io::Read,
    S: crc::Width,
{
    read: &'a mut R,                    // underlying reader
    digest: &'a mut crc::Digest<'b, S>, // hasher
}

impl<'a, 'b, R, S> CrcDigestRead<'a, 'b, R, S>
where
    R: io::Read,
    S: crc::Width,
{
    pub fn new(read: &'a mut R, digest: &'a mut crc::Digest<'b, S>) -> Self {
        Self { read, digest }
    }
}

impl<'a, 'b, R> io::Read for CrcDigestRead<'a, 'b, R, u32>
where
    R: io::Read,
{
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        let result = self.read.read(buf)?;
        self.digest.update(&buf[..result]);
        Ok(result)
    }
}

// A BufRead counting the bytes read.
pub struct CountBufRead<'a, R>
where
    R: 'a + io::BufRead,
{
    read: &'a mut R, // underlying reader
    count: usize,    // number of bytes read
}

impl<'a, R> CountBufRead<'a, R>
where
    R: io::BufRead,
{
    pub fn new(read: &'a mut R) -> Self {
        Self { read, count: 0 }
    }

    pub fn count(&self) -> usize {
        self.count
    }
}

impl<'a, R> io::Read for CountBufRead<'a, R>
where
    R: io::BufRead,
{
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        let result = self.read.read(buf)?;
        self.count += result;
        Ok(result)
    }
}

impl<'a, R> io::BufRead for CountBufRead<'a, R>
where
    R: io::BufRead,
{
    fn fill_buf(&mut self) -> io::Result<&[u8]> {
        self.read.fill_buf()
    }

    fn consume(&mut self, amt: usize) {
        self.read.consume(amt);
        self.count += amt;
    }
}
