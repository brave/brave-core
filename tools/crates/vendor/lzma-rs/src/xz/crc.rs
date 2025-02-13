use crc::{Crc, CRC_32_ISO_HDLC, CRC_64_XZ};

pub const CRC32: Crc<u32> = Crc::<u32>::new(&CRC_32_ISO_HDLC);
pub const CRC64: Crc<u64> = Crc::<u64>::new(&CRC_64_XZ);
