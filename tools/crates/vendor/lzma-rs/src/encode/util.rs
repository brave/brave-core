use std::io;

// A Write computing a digest on the bytes written.
pub struct CrcDigestWrite<'a, 'b, W, S>
where
    W: 'a + io::Write,
    S: crc::Width,
{
    write: &'a mut W,                   // underlying writer
    digest: &'a mut crc::Digest<'b, S>, // hasher
}

impl<'a, 'b, W, S> CrcDigestWrite<'a, 'b, W, S>
where
    W: io::Write,
    S: crc::Width,
{
    pub fn new(write: &'a mut W, digest: &'a mut crc::Digest<'b, S>) -> Self {
        Self { write, digest }
    }
}

impl<'a, 'b, W> io::Write for CrcDigestWrite<'a, 'b, W, u32>
where
    W: io::Write,
{
    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        let result = self.write.write(buf)?;
        self.digest.update(&buf[..result]);
        Ok(result)
    }
    fn flush(&mut self) -> io::Result<()> {
        self.write.flush()
    }
}

// A Write counting the bytes written.
pub struct CountWrite<'a, W>
where
    W: 'a + io::Write,
{
    write: &'a mut W, // underlying writer
    count: usize,     // number of bytes written
}

impl<'a, W> CountWrite<'a, W>
where
    W: io::Write,
{
    pub fn new(write: &'a mut W) -> Self {
        Self { write, count: 0 }
    }

    pub fn count(&self) -> usize {
        self.count
    }
}

impl<'a, W> io::Write for CountWrite<'a, W>
where
    W: io::Write,
{
    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        let result = self.write.write(buf)?;
        self.count += result;
        Ok(result)
    }

    fn flush(&mut self) -> io::Result<()> {
        self.write.flush()
    }
}
