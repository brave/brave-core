use crate::{
    zrip::params::{max_window, DParameter},
    DecodeV2,
};
use compression_core::util::{PartialBuffer, WriteBuffer};
use std::io;
use zrip::push::{PushDecoder, Status};

#[derive(Debug)]
pub struct ZripDecoder {
    decoder: PushDecoder,
}

impl Default for ZripDecoder {
    fn default() -> Self {
        Self::new_with_params(&[])
    }
}

impl ZripDecoder {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn new_with_params(params: &[DParameter]) -> Self {
        let mut decoder = PushDecoder::new();
        if let Some(max) = max_window(params) {
            decoder = decoder.max_window(max);
        }
        Self { decoder }
    }

    pub fn new_with_dict(dictionary: &[u8]) -> io::Result<Self> {
        let dict = zrip::Dictionary::from_bytes(dictionary)
            .map_err(|err| io::Error::new(io::ErrorKind::InvalidInput, err))?;
        Ok(Self {
            decoder: PushDecoder::with_dict(dict),
        })
    }

    fn process(
        &mut self,
        input: &[u8],
        output: &mut WriteBuffer<'_>,
    ) -> io::Result<(usize, Status)> {
        // SAFETY: `decode_uninit` never de-initializes bytes of `output`.
        let progress = self
            .decoder
            .decode_uninit(input, unsafe { output.unwritten_mut() })
            .map_err(|err| io::Error::new(io::ErrorKind::InvalidData, err))?;
        // SAFETY: `decode_uninit` initializes exactly `produced` leading bytes of `output`.
        unsafe { output.assume_init_and_advance(progress.produced) };
        Ok((progress.consumed, progress.status))
    }
}

impl DecodeV2 for ZripDecoder {
    fn reinit(&mut self) -> io::Result<()> {
        self.decoder.reset();
        Ok(())
    }

    fn decode(
        &mut self,
        input: &mut PartialBuffer<&[u8]>,
        output: &mut WriteBuffer<'_>,
    ) -> io::Result<bool> {
        // zrip reports `Finished` once, and a frame may have ended while `flush` drained its
        // output; report it before reading on.
        if self.decoder.at_frame_end() {
            return Ok(true);
        }
        let (consumed, status) = self.process(input.unwritten(), output)?;
        input.advance(consumed);
        Ok(status == Status::Finished)
    }

    fn flush(&mut self, output: &mut WriteBuffer<'_>) -> io::Result<bool> {
        let (_, status) = self.process(&[], output)?;
        Ok(status != Status::NeedsOutput)
    }

    fn finish(&mut self, output: &mut WriteBuffer<'_>) -> io::Result<bool> {
        let (_, status) = self.process(&[], output)?;
        if status == Status::NeedsOutput {
            return Ok(false);
        }
        // Like the `zstd` backend: the stream must end with a complete frame.
        if self.decoder.at_frame_end() {
            Ok(true)
        } else {
            Err(io::Error::new(
                io::ErrorKind::UnexpectedEof,
                "zstd stream did not finish",
            ))
        }
    }
}
