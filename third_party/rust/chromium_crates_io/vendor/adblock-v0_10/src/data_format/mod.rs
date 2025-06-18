//! Allows serialization of the adblock engine into a compact binary format, as well as subsequent
//! rapid deserialization back into an engine.
//!
//! In order to support multiple format versions simultaneously, this module wraps around different
//! serialization/deserialization implementations and can automatically dispatch to the appropriate
//! one.

mod storage;

pub(crate) mod utils;

use crate::blocker::Blocker;
use crate::cosmetic_filter_cache::CosmeticFilterCache;
use crate::network_filter_list::NetworkFilterListParsingError;

/// Newer formats start with this magic byte sequence.
/// Calculated as the leading 4 bytes of `echo -n 'brave/adblock-rust' | sha512sum`.
const ADBLOCK_RUST_DAT_MAGIC: [u8; 4] = [0xd1, 0xd9, 0x3a, 0xaf];
const ADBLOCK_RUST_DAT_VERSION: u8 = 1;

#[derive(Debug)]
pub enum SerializationError {
    RmpSerdeError(rmp_serde::encode::Error),
}

impl From<rmp_serde::encode::Error> for SerializationError {
    fn from(e: rmp_serde::encode::Error) -> Self {
        Self::RmpSerdeError(e)
    }
}

#[derive(Debug)]
pub enum DeserializationError {
    RmpSerdeError(rmp_serde::decode::Error),
    UnsupportedFormatVersion(u8),
    NoHeaderFound,
    FlatBufferParsingError(flatbuffers::InvalidFlatbuffer),
    ValidationError,
}

impl From<std::convert::Infallible> for DeserializationError {
    fn from(x: std::convert::Infallible) -> Self {
        match x {}
    }
}

impl From<rmp_serde::decode::Error> for DeserializationError {
    fn from(e: rmp_serde::decode::Error) -> Self {
        Self::RmpSerdeError(e)
    }
}

impl From<NetworkFilterListParsingError> for DeserializationError {
    fn from(e: NetworkFilterListParsingError) -> Self {
        match e {
            NetworkFilterListParsingError::InvalidFlatbuffer(invalid_flatbuffer) => {
                Self::FlatBufferParsingError(invalid_flatbuffer)
            }
            NetworkFilterListParsingError::UniqueDomainsOutOfBounds(_) => Self::ValidationError,
        }
    }
}

pub(crate) fn serialize_engine(
    blocker: &Blocker,
    cfc: &CosmeticFilterCache,
) -> Result<Vec<u8>, SerializationError> {
    let serialize_format = storage::SerializeFormat::from((blocker, cfc));
    serialize_format.serialize()
}

pub(crate) fn deserialize_engine(
    serialized: &[u8],
) -> Result<(Blocker, CosmeticFilterCache), DeserializationError> {
    let deserialize_format = storage::DeserializeFormat::deserialize(serialized)?;
    deserialize_format.try_into()
}

// Verify the header (MAGIC + VERSION) and return the data after the header.
pub fn parse_dat_header(serialized: &[u8]) -> Result<&[u8], DeserializationError> {
    if !serialized.starts_with(&ADBLOCK_RUST_DAT_MAGIC) {
        return Err(DeserializationError::NoHeaderFound);
    }
    if serialized.len() < ADBLOCK_RUST_DAT_MAGIC.len() + 1 {
        return Err(DeserializationError::NoHeaderFound);
    }
    let version = serialized[ADBLOCK_RUST_DAT_MAGIC.len()];
    if version != ADBLOCK_RUST_DAT_VERSION {
        return Err(DeserializationError::UnsupportedFormatVersion(version));
    }

    Ok(&serialized[ADBLOCK_RUST_DAT_MAGIC.len() + 1..])
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn validate_magic_bytes() {
        use sha2::Digest;

        let mut hasher = sha2::Sha512::new();

        hasher.update("brave/adblock-rust");

        let result = hasher.finalize();

        assert!(result.starts_with(&ADBLOCK_RUST_DAT_MAGIC));
    }
}
