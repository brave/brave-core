use crate::{
    zrip::params::{options, CParameter},
    EncodeV2,
};
use compression_core::util::{PartialBuffer, WriteBuffer};
use std::io;
use zrip::push::{EndOp, PushEncoder, Status};

#[derive(Debug)]
pub struct ZripEncoder {
    encoder: PushEncoder,
}

impl ZripEncoder {
    /// # Panics
    ///
    /// Panics if `level` is outside -7..=4; [`CParameter::quality`] clamps to that range.
    pub fn new(level: i32) -> Self {
        Self::new_with_params(level, &[])
    }

    /// # Panics
    ///
    /// Panics if `level` is outside -7..=4; [`CParameter::quality`] clamps to that range.
    pub fn new_with_params(level: i32, params: &[CParameter]) -> Self {
        Self {
            encoder: PushEncoder::with_options(level, &options(params)).unwrap(),
        }
    }

    pub fn new_with_dict(level: i32, dictionary: &[u8]) -> io::Result<Self> {
        let dict = zrip::Dictionary::from_bytes(dictionary)
            .map_err(|err| io::Error::new(io::ErrorKind::InvalidInput, err))?;
        let encoder = PushEncoder::with_dict(level, dict)
            .map_err(|err| io::Error::new(io::ErrorKind::InvalidInput, err))?;
        Ok(Self { encoder })
    }

    fn process(
        &mut self,
        input: &[u8],
        output: &mut WriteBuffer<'_>,
        op: EndOp,
    ) -> io::Result<(usize, Status)> {
        // SAFETY: `encode_uninit` never de-initializes bytes of `output`.
        let progress = self
            .encoder
            .encode_uninit(input, unsafe { output.unwritten_mut() }, op)
            .map_err(io::Error::other)?;
        // SAFETY: `encode_uninit` initializes exactly `produced` leading bytes of `output`.
        unsafe { output.assume_init_and_advance(progress.produced) };
        Ok((progress.consumed, progress.status))
    }
}

impl EncodeV2 for ZripEncoder {
    fn encode(
        &mut self,
        input: &mut PartialBuffer<&[u8]>,
        output: &mut WriteBuffer<'_>,
    ) -> io::Result<()> {
        let (consumed, _) = self.process(input.unwritten(), output, EndOp::Continue)?;
        input.advance(consumed);
        Ok(())
    }

    fn flush(&mut self, output: &mut WriteBuffer<'_>) -> io::Result<bool> {
        let (_, status) = self.process(&[], output, EndOp::Flush)?;
        Ok(status != Status::NeedsOutput)
    }

    fn finish(&mut self, output: &mut WriteBuffer<'_>) -> io::Result<bool> {
        let (_, status) = self.process(&[], output, EndOp::End)?;
        Ok(status == Status::Finished)
    }
}
