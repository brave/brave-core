//! Allows serialization of the adblock engine into a compact binary format, as well as subsequent
//! rapid deserialization back into an engine.
//!
//! In order to support multiple format versions simultaneously, this module wraps around different
//! serialization/deserialization implementations and can automatically dispatch to the appropriate
//! one.

mod legacy;
mod v0;

pub mod utils;

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
    Legacy(legacy::SerializeFormat<'a>),
    V0(v0::SerializeFormat<'a>),
}

#[derive(Debug)]
pub enum SerializationError {
    RmpSerdeError(rmp_serde::encode::Error),
    GzError(std::io::Error),
}

/// Since two different versions of `rmp-serde` are being used, errors must be converted to a
/// single implementation.
impl From<rmp_serde_legacy::encode::Error> for SerializationError {
    fn from(e: rmp_serde_legacy::encode::Error) -> Self {
        use rmp_serde::encode::Error as EncodeError;
        use rmp_serde_legacy::encode::Error as LegacyEncodeError;

        let new_error = match e {
            LegacyEncodeError::InvalidValueWrite(e) => EncodeError::InvalidValueWrite(e),
            LegacyEncodeError::UnknownLength => EncodeError::UnknownLength,
            LegacyEncodeError::DepthLimitExceeded => EncodeError::DepthLimitExceeded,
            LegacyEncodeError::Syntax(e) => EncodeError::Syntax(e),
        };
        Self::RmpSerdeError(new_error)
    }
}

impl From<rmp_serde::encode::Error> for SerializationError {
    fn from(e: rmp_serde::encode::Error) -> Self {
        Self::RmpSerdeError(e)
    }
}

impl From<std::io::Error> for SerializationError {
    fn from(e: std::io::Error) -> Self {
        Self::GzError(e)
    }
}

impl<'a> SerializeFormat<'a> {
    pub(crate) fn build(blocker: &'a Blocker, cfc: &'a CosmeticFilterCache, legacy: bool) -> Self {
        if legacy {
            Self::Legacy(legacy::SerializeFormat::from((blocker, cfc)))
        } else {
            Self::V0(v0::SerializeFormat::from((blocker, cfc)))
        }
    }

    pub(crate) fn serialize(&self) -> Result<Vec<u8>, SerializationError> {
        match self {
            Self::Legacy(v) => v.serialize(),
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
    Legacy(legacy::DeserializeFormat),
    V0(v0::DeserializeFormat),
}

#[derive(Debug)]
pub enum DeserializationError {
    RmpSerdeError(rmp_serde::decode::Error),
    UnsupportedFormatVersion(u8),
    NoHeaderFound,
}

/// Since two different versions of `rmp-serde` are being used, errors must be converted to a
/// single implementation.
impl From<rmp_serde_legacy::decode::Error> for DeserializationError {
    fn from(e: rmp_serde_legacy::decode::Error) -> Self {
        use rmp_serde::decode::Error as DecodeError;
        use rmp_serde_legacy::decode::Error as LegacyDecodeError;

        let new_error = match e {
            LegacyDecodeError::InvalidMarkerRead(e) => DecodeError::InvalidMarkerRead(e),
            LegacyDecodeError::InvalidDataRead(e) => DecodeError::InvalidDataRead(e),
            LegacyDecodeError::TypeMismatch(m) => DecodeError::TypeMismatch(m),
            LegacyDecodeError::OutOfRange => DecodeError::OutOfRange,
            LegacyDecodeError::LengthMismatch(l) => DecodeError::LengthMismatch(l),
            LegacyDecodeError::Uncategorized(e) => DecodeError::Uncategorized(e),
            LegacyDecodeError::Syntax(e) => DecodeError::Syntax(e),
            LegacyDecodeError::Utf8Error(e) => DecodeError::Utf8Error(e),
            LegacyDecodeError::DepthLimitExceeded => DecodeError::DepthLimitExceeded,
        };
        Self::RmpSerdeError(new_error)
    }
}

impl From<rmp_serde::decode::Error> for DeserializationError {
    fn from(e: rmp_serde::decode::Error) -> Self {
        Self::RmpSerdeError(e)
    }
}

impl DeserializeFormat {
    pub(crate) fn build(self) -> (Blocker, CosmeticFilterCache) {
        match self {
            Self::Legacy(v) => v.into(),
            Self::V0(v) => v.into(),
        }
    }

    pub(crate) fn deserialize(serialized: &[u8]) -> Result<Self, DeserializationError> {
        /// adblock-rust has always used flate2 1.0.x for the legacy format, which has never
        /// changed the header sequence from these 10 bits when the GzEncoder is left uncustomized.
        const FLATE2_GZ_HEADER_BYTES: [u8; 10] = [31, 139, 8, 0, 0, 0, 0, 0, 0, 255];

        if serialized.starts_with(&FLATE2_GZ_HEADER_BYTES) {
            Ok(Self::Legacy(legacy::DeserializeFormat::deserialize(
                serialized,
            )?))
        } else if serialized.starts_with(&ADBLOCK_RUST_DAT_MAGIC) {
            let version = serialized[ADBLOCK_RUST_DAT_MAGIC.len()];
            match version {
                0 => Ok(Self::V0(v0::DeserializeFormat::deserialize(serialized)?)),
                v => Err(DeserializationError::UnsupportedFormatVersion(v)),
            }
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
