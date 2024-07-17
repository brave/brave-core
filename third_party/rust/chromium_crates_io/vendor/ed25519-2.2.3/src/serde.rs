//! `serde` support.

use crate::{Signature, SignatureBytes};
use ::serde::{de, ser, Deserialize, Serialize};
use core::fmt;

impl Serialize for Signature {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        use ser::SerializeTuple;

        let mut seq = serializer.serialize_tuple(Signature::BYTE_SIZE)?;

        for byte in self.to_bytes() {
            seq.serialize_element(&byte)?;
        }

        seq.end()
    }
}

// serde lacks support for deserializing arrays larger than 32-bytes
// see: <https://github.com/serde-rs/serde/issues/631>
impl<'de> Deserialize<'de> for Signature {
    fn deserialize<D: de::Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        struct ByteArrayVisitor;

        impl<'de> de::Visitor<'de> for ByteArrayVisitor {
            type Value = [u8; Signature::BYTE_SIZE];

            fn expecting(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
                formatter.write_str("bytestring of length 64")
            }

            fn visit_seq<A>(self, mut seq: A) -> Result<[u8; Signature::BYTE_SIZE], A::Error>
            where
                A: de::SeqAccess<'de>,
            {
                use de::Error;
                let mut arr = [0u8; Signature::BYTE_SIZE];

                for (i, byte) in arr.iter_mut().enumerate() {
                    *byte = seq
                        .next_element()?
                        .ok_or_else(|| Error::invalid_length(i, &self))?;
                }

                Ok(arr)
            }
        }

        deserializer
            .deserialize_tuple(Signature::BYTE_SIZE, ByteArrayVisitor)
            .map(Into::into)
    }
}

#[cfg(feature = "serde_bytes")]
impl serde_bytes::Serialize for Signature {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        serializer.serialize_bytes(&self.to_bytes())
    }
}

#[cfg(feature = "serde_bytes")]
impl<'de> serde_bytes::Deserialize<'de> for Signature {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: de::Deserializer<'de>,
    {
        struct ByteArrayVisitor;

        impl<'de> de::Visitor<'de> for ByteArrayVisitor {
            type Value = SignatureBytes;

            fn expecting(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
                formatter.write_str("bytestring of length 64")
            }

            fn visit_bytes<E>(self, bytes: &[u8]) -> Result<Self::Value, E>
            where
                E: de::Error,
            {
                use de::Error;

                bytes
                    .try_into()
                    .map_err(|_| Error::invalid_length(bytes.len(), &self))
            }
        }

        deserializer
            .deserialize_bytes(ByteArrayVisitor)
            .map(Into::into)
    }
}

#[cfg(test)]
mod tests {
    use crate::{Signature, SignatureBytes};
    use hex_literal::hex;

    const SIGNATURE_BYTES: SignatureBytes = hex!(
        "
        e5564300c360ac729086e2cc806e828a
        84877f1eb8e5d974d873e06522490155
        5fb8821590a33bacc61e39701cf9b46b
        d25bf5f0595bbe24655141438e7a100b
        "
    );

    #[test]
    fn round_trip() {
        let signature = Signature::from_bytes(&SIGNATURE_BYTES);
        let serialized = bincode::serialize(&signature).unwrap();
        let deserialized = bincode::deserialize(&serialized).unwrap();
        assert_eq!(signature, deserialized);
    }
}
