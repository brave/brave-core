//! CID Serde (de)serialization for the IPLD Data Model.
//!
//! CIDs cannot directly be represented in any of the native Serde Data model types. In order to
//! work around that limitation. a newtype struct is introduced, that is used as a marker for Serde
//! (de)serialization.
extern crate alloc;

use alloc::{format, vec::Vec};
use core::convert::TryFrom;
use core::fmt;

use serde::{de, ser};
use serde_bytes::ByteBuf;

use crate::CidGeneric;

/// An identifier that is used internally by Serde implementations that support [`Cid`]s.
pub const CID_SERDE_PRIVATE_IDENTIFIER: &str = "$__private__serde__identifier__for__cid";

/// Serialize a CID into the Serde data model as enum.
///
/// Custom types are not supported by Serde, hence we map a CID into an enum that can be identified
/// as a CID by implementations that support CIDs. The corresponding Rust type would be:
///
/// ```text
/// struct $__private__serde__identifier__for__cid(serde_bytes::BytesBuf);
/// ```
impl<const SIZE: usize> ser::Serialize for CidGeneric<SIZE> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: ser::Serializer,
    {
        let value = ByteBuf::from(self.to_bytes());
        serializer.serialize_newtype_struct(CID_SERDE_PRIVATE_IDENTIFIER, &value)
    }
}

/// Visitor to transform bytes into a CID.
pub struct BytesToCidVisitor<const SIZE: usize = 64>;

impl<'de, const SIZE: usize> de::Visitor<'de> for BytesToCidVisitor<SIZE> {
    type Value = CidGeneric<SIZE>;

    fn expecting(&self, fmt: &mut fmt::Formatter) -> fmt::Result {
        write!(fmt, "a valid CID in bytes")
    }

    fn visit_bytes<E>(self, value: &[u8]) -> Result<Self::Value, E>
    where
        E: de::Error,
    {
        CidGeneric::<SIZE>::try_from(value)
            .map_err(|err| de::Error::custom(format!("Failed to deserialize CID: {}", err)))
    }

    /// Some Serde data formats interpret a byte stream as a sequence of bytes (e.g. `serde_json`).
    fn visit_seq<A>(self, mut seq: A) -> Result<Self::Value, A::Error>
    where
        A: de::SeqAccess<'de>,
    {
        let mut bytes = Vec::new();
        while let Some(byte) = seq.next_element()? {
            bytes.push(byte);
        }
        CidGeneric::<SIZE>::try_from(bytes)
            .map_err(|err| de::Error::custom(format!("Failed to deserialize CID: {}", err)))
    }
}

/// Deserialize a CID into a newtype struct.
///
/// Deserialize a CID that was serialized as a newtype struct, so that can be identified as a CID.
/// Its corresponding Rust type would be:
///
/// ```text
/// struct $__private__serde__identifier__for__cid(serde_bytes::BytesBuf);
/// ```
impl<'de, const SIZE: usize> de::Deserialize<'de> for CidGeneric<SIZE> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: de::Deserializer<'de>,
    {
        /// Main visitor to deserialize a CID.
        ///
        /// This visitor has only a single entry point to deserialize CIDs, it's
        /// `visit_new_type_struct()`. This ensures that it isn't accidentally used to decode CIDs
        /// to bytes.
        struct MainEntryVisitor<const SIZE: usize>;

        impl<'de, const SIZE: usize> de::Visitor<'de> for MainEntryVisitor<SIZE> {
            type Value = CidGeneric<SIZE>;

            fn expecting(&self, fmt: &mut fmt::Formatter) -> fmt::Result {
                write!(fmt, "a valid CID in bytes, wrapped in an newtype struct")
            }

            fn visit_newtype_struct<D>(self, deserializer: D) -> Result<Self::Value, D::Error>
            where
                D: de::Deserializer<'de>,
            {
                deserializer.deserialize_bytes(BytesToCidVisitor)
            }
        }

        deserializer.deserialize_newtype_struct(CID_SERDE_PRIVATE_IDENTIFIER, MainEntryVisitor)
    }
}

#[cfg(test)]
mod tests {
    use std::convert::TryFrom;

    use crate::CidGeneric;

    #[test]
    fn test_cid_serde() {
        let cid = CidGeneric::<70>::try_from(
            "bafkreibme22gw2h7y2h7tg2fhqotaqjucnbc24deqo72b6mkl2egezxhvy",
        )
        .unwrap();
        let bytes = serde_json::to_string(&cid).unwrap();
        let cid2 = serde_json::from_str(&bytes).unwrap();
        assert_eq!(cid, cid2);
    }
}
