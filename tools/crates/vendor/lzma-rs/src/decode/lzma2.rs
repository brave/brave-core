use crate::decode::lzbuffer::LzBuffer;
use crate::decode::lzma::{DecoderState, LzmaProperties};
use crate::decode::{lzbuffer, rangecoder};
use crate::error;
use byteorder::{BigEndian, ReadBytesExt};
use std::io;
use std::io::Read;

#[derive(Debug)]
/// Raw decoder for LZMA2.
pub struct Lzma2Decoder {
    lzma_state: DecoderState,
}

impl Lzma2Decoder {
    /// Creates a new object ready for decompressing data that it's given.
    pub fn new() -> Lzma2Decoder {
        Lzma2Decoder {
            lzma_state: DecoderState::new(
                LzmaProperties {
                    lc: 0,
                    lp: 0,
                    pb: 0,
                },
                None,
            ),
        }
    }

    /// Performs the equivalent of replacing this decompression state with a freshly allocated copy.
    ///
    /// This function may not allocate memory and will attempt to reuse any previously allocated resources.
    #[cfg(feature = "raw_decoder")]
    pub fn reset(&mut self) {
        self.lzma_state.reset_state(LzmaProperties {
            lc: 0,
            lp: 0,
            pb: 0,
        });
    }

    /// Decompresses the input data into the output, consuming only as much input as needed and writing as much output as possible.
    pub fn decompress<W: io::Write, R: io::BufRead>(
        &mut self,
        input: &mut R,
        output: &mut W,
    ) -> error::Result<()> {
        let mut accum = lzbuffer::LzAccumBuffer::from_stream(output, usize::MAX);

        loop {
            let status = input.read_u8().map_err(|e| {
                error::Error::LzmaError(format!("LZMA2 expected new status: {}", e))
            })?;

            lzma_info!("LZMA2 status: {}", status);

            if status == 0 {
                lzma_info!("LZMA2 end of input");
                break;
            } else if status == 1 {
                // uncompressed reset dict
                Self::parse_uncompressed(&mut accum, input, true)?;
            } else if status == 2 {
                // uncompressed no reset
                Self::parse_uncompressed(&mut accum, input, false)?;
            } else {
                self.parse_lzma(&mut accum, input, status)?;
            }
        }

        accum.finish()?;
        Ok(())
    }

    fn parse_lzma<R, W>(
        &mut self,
        accum: &mut lzbuffer::LzAccumBuffer<W>,
        input: &mut R,
        status: u8,
    ) -> error::Result<()>
    where
        R: io::BufRead,
        W: io::Write,
    {
        if status & 0x80 == 0 {
            return Err(error::Error::LzmaError(format!(
                "LZMA2 invalid status {}, must be 0, 1, 2 or >= 128",
                status
            )));
        }

        let reset_dict: bool;
        let reset_state: bool;
        let reset_props: bool;
        match (status >> 5) & 0x3 {
            0 => {
                reset_dict = false;
                reset_state = false;
                reset_props = false;
            }
            1 => {
                reset_dict = false;
                reset_state = true;
                reset_props = false;
            }
            2 => {
                reset_dict = false;
                reset_state = true;
                reset_props = true;
            }
            3 => {
                reset_dict = true;
                reset_state = true;
                reset_props = true;
            }
            _ => unreachable!(),
        }

        let unpacked_size = input
            .read_u16::<BigEndian>()
            .map_err(|e| error::Error::LzmaError(format!("LZMA2 expected unpacked size: {}", e)))?;
        let unpacked_size = ((((status & 0x1F) as u64) << 16) | (unpacked_size as u64)) + 1;

        let packed_size = input
            .read_u16::<BigEndian>()
            .map_err(|e| error::Error::LzmaError(format!("LZMA2 expected packed size: {}", e)))?;
        let packed_size = (packed_size as u64) + 1;

        lzma_info!(
            "LZMA2 compressed block {{ unpacked_size: {}, packed_size: {}, reset_dict: {}, reset_state: {}, reset_props: {} }}",
            unpacked_size,
            packed_size,
            reset_dict,
            reset_state,
            reset_props
        );

        if reset_dict {
            accum.reset()?;
        }

        if reset_state {
            let new_props = if reset_props {
                let props = input.read_u8().map_err(|e| {
                    error::Error::LzmaError(format!("LZMA2 expected new properties: {}", e))
                })?;

                let mut pb = props as u32;
                if pb >= 225 {
                    return Err(error::Error::LzmaError(format!(
                        "LZMA2 invalid properties: {} must be < 225",
                        pb
                    )));
                }

                let lc = pb % 9;
                pb /= 9;
                let lp = pb % 5;
                pb /= 5;

                if lc + lp > 4 {
                    return Err(error::Error::LzmaError(format!(
                        "LZMA2 invalid properties: lc + lp ({} + {}) must be <= 4",
                        lc, lp
                    )));
                }

                lzma_info!("Properties {{ lc: {}, lp: {}, pb: {} }}", lc, lp, pb);
                LzmaProperties { lc, lp, pb }
            } else {
                self.lzma_state.lzma_props
            };

            self.lzma_state.reset_state(new_props);
        }

        self.lzma_state
            .set_unpacked_size(Some(unpacked_size + accum.len() as u64));

        let mut taken = input.take(packed_size);
        let mut rangecoder = rangecoder::RangeDecoder::new(&mut taken)
            .map_err(|e| error::Error::LzmaError(format!("LZMA input too short: {}", e)))?;
        self.lzma_state.process(accum, &mut rangecoder)
    }

    fn parse_uncompressed<R, W>(
        accum: &mut lzbuffer::LzAccumBuffer<W>,
        input: &mut R,
        reset_dict: bool,
    ) -> error::Result<()>
    where
        R: io::BufRead,
        W: io::Write,
    {
        let unpacked_size = input
            .read_u16::<BigEndian>()
            .map_err(|e| error::Error::LzmaError(format!("LZMA2 expected unpacked size: {}", e)))?;
        let unpacked_size = (unpacked_size as usize) + 1;

        lzma_info!(
            "LZMA2 uncompressed block {{ unpacked_size: {}, reset_dict: {} }}",
            unpacked_size,
            reset_dict
        );

        if reset_dict {
            accum.reset()?;
        }

        let mut buf = vec![0; unpacked_size];
        input.read_exact(buf.as_mut_slice()).map_err(|e| {
            error::Error::LzmaError(format!(
                "LZMA2 expected {} uncompressed bytes: {}",
                unpacked_size, e
            ))
        })?;
        accum.append_bytes(buf.as_slice());

        Ok(())
    }
}
