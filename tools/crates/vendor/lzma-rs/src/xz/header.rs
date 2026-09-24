//! XZ header.

use crate::decode::util;
use crate::error;
use crate::xz::crc::CRC32;
use crate::xz::StreamFlags;
use byteorder::{BigEndian, LittleEndian, ReadBytesExt};

/// File format magic header signature, see sect. 2.1.1.1.
pub(crate) const XZ_MAGIC: &[u8] = &[0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00];

/// Stream Header, see sect. 2.1.1.
#[derive(Clone, Copy, Debug)]
pub(crate) struct StreamHeader {
    pub(crate) stream_flags: StreamFlags,
}

impl StreamHeader {
    /// Parse a Stream Header from a buffered reader.
    pub(crate) fn parse<BR>(input: &mut BR) -> error::Result<Self>
    where
        BR: std::io::BufRead,
    {
        if !util::read_tag(input, XZ_MAGIC)? {
            return Err(error::Error::XzError(format!(
                "Invalid XZ magic, expected {:?}",
                XZ_MAGIC
            )));
        }

        let (flags, digested) = {
            let mut digest = CRC32.digest();
            let mut digest_rd = util::CrcDigestRead::new(input, &mut digest);
            let flags = digest_rd.read_u16::<BigEndian>()?;
            (flags, digest.finalize())
        };

        let crc32 = input.read_u32::<LittleEndian>()?;
        if crc32 != digested {
            return Err(error::Error::XzError(format!(
                "Invalid header CRC32: expected 0x{:08x} but got 0x{:08x}",
                crc32, digested
            )));
        }

        let stream_flags = StreamFlags::parse(flags)?;
        let header = Self { stream_flags };

        lzma_info!("XZ check method: {:?}", header.stream_flags.check_method);
        Ok(header)
    }
}
