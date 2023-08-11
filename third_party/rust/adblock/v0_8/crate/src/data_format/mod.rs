//! Allows serialization of the adblock engine into a compact binary format, as well as subsequent
//! rapid deserialization back into an engine.
//!
//! In order to support multiple format versions simultaneously, this module wraps around different
//! serialization/deserialization implementations and can automatically dispatch to the appropriate
//! one.

mod v0;

pub(crate) mod utils;

use crate::blocker::Blocker;
use crate::cosmetic_filter_cache::CosmeticFilterCache;

/// Newer formats start with this magic byte sequence.
/// Calculated as the leading 4 bytes of `echo -n 'brave/adblock-rust' | sha512sum`.
const ADBLOCK_RUST_DAT_MAGIC: [u8; 4] = [0xd1, 0xd9, 0x3a, 0xaf];

/// Provides structural aggregration of referenced adblock engine data to allow for allocation-free
/// serialization.
///
/// Note that this does not implement `Serialize` directly, as it is composed of parts which must
/// be serialized independently. Instead, use the `serialize` method.
pub(crate) enum SerializeFormat<'a> {
    V0(v0::SerializeFormat<'a>),
}

#[derive(Debug)]
pub enum SerializationError {
    RmpSerdeError(rmp_serde::encode::Error),
}

impl From<rmp_serde::encode::Error> for SerializationError {
    fn from(e: rmp_serde::encode::Error) -> Self {
        Self::RmpSerdeError(e)
    }
}

impl<'a> SerializeFormat<'a> {
    pub(crate) fn build(blocker: &'a Blocker, cfc: &'a CosmeticFilterCache) -> Self {
        Self::V0(v0::SerializeFormat::from((blocker, cfc)))
    }

    pub(crate) fn serialize(&self) -> Result<Vec<u8>, SerializationError> {
        match self {
            Self::V0(v) => v.serialize(),
        }
    }
}

/// Structural representation of adblock engine data that can be built up from deserialization and
/// used directly to construct new `Engine` components without unnecessary allocation.
///
/// Note that this does not implement `Deserialize` directly, as it is composed of parts which must
/// be deserialized independently. Instead, use the `deserialize` method.
pub(crate) enum DeserializeFormat {
    V0(v0::DeserializeFormat),
}

#[derive(Debug)]
pub enum DeserializationError {
    RmpSerdeError(rmp_serde::decode::Error),
    UnsupportedFormatVersion(u8),
    NoHeaderFound,
    /// Support for the legacy gzip-compressed data format was removed in version 0.8.0 of this
    /// crate. If you still need it for some reason, you can convert it using 0.7.x by
    /// deserializing and then reserializing it into the newer V0 format.
    LegacyFormatNoLongerSupported,
}

impl From<rmp_serde::decode::Error> for DeserializationError {
    fn from(e: rmp_serde::decode::Error) -> Self {
        Self::RmpSerdeError(e)
    }
}

impl DeserializeFormat {
    pub(crate) fn build(self) -> (Blocker, CosmeticFilterCache) {
        match self {
            Self::V0(v) => v.into(),
        }
    }

    pub(crate) fn deserialize(serialized: &[u8]) -> Result<Self, DeserializationError> {
        /// adblock-rust's legacy DAT format has always used flate2 1.0.x, which has never changed
        /// the header sequence from these 10 bits when the GzEncoder is left uncustomized.
        const FLATE2_GZ_HEADER_BYTES: [u8; 10] = [31, 139, 8, 0, 0, 0, 0, 0, 0, 255];

        if serialized.starts_with(&ADBLOCK_RUST_DAT_MAGIC) {
            let version = serialized[ADBLOCK_RUST_DAT_MAGIC.len()];
            match version {
                0 => Ok(Self::V0(v0::DeserializeFormat::deserialize(serialized)?)),
                v => Err(DeserializationError::UnsupportedFormatVersion(v)),
            }
        } else if serialized.starts_with(&FLATE2_GZ_HEADER_BYTES) {
            Err(DeserializationError::LegacyFormatNoLongerSupported)
        } else {
            Err(DeserializationError::NoHeaderFound)
        }
    }
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
