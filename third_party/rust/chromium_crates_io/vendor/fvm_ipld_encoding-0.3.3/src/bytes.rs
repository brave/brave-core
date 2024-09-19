// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems, David Tolnay (serde)
// SPDX-License-Identifier: Apache-2.0, MIT

/// A much simplified version of serde_bytes that:
///
/// 1. Refuses to decode strings/arrays into "bytes", only accepting "bytes" (hence the "strict"
///    part).
/// 2. Can decode to/from byte arrays.
pub mod strict_bytes {
    use std::borrow::Cow;
    use std::fmt;

    use serde::de::{Error, Visitor};
    pub use serde::{Deserializer, Serializer};

    pub trait Deserialize<'de>: Sized {
        #[allow(missing_docs)]
        fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
        where
            D: Deserializer<'de>;
    }

    pub trait Serialize {
        #[allow(missing_docs)]
        fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
        where
            S: Serializer;
    }

    impl<T: ?Sized> Serialize for T
    where
        T: AsRef<[u8]>,
    {
        fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
        where
            S: Serializer,
        {
            serializer.serialize_bytes(self.as_ref())
        }
    }

    impl<'de> Deserialize<'de> for Vec<u8> {
        fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
        where
            D: Deserializer<'de>,
        {
            struct VecVisitor;

            impl<'de> Visitor<'de> for VecVisitor {
                type Value = Vec<u8>;

                fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                    formatter.write_str("byte array")
                }

                fn visit_bytes<E>(self, v: &[u8]) -> Result<Vec<u8>, E>
                where
                    E: Error,
                {
                    Ok(v.into())
                }

                fn visit_byte_buf<E>(self, v: Vec<u8>) -> Result<Vec<u8>, E>
                where
                    E: Error,
                {
                    Ok(v)
                }
            }
            deserializer.deserialize_byte_buf(VecVisitor)
        }
    }

    impl<'de: 'a, 'a> Deserialize<'de> for Cow<'a, [u8]> {
        fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
        where
            D: Deserializer<'de>,
        {
            struct CowVisitor;

            impl<'de> Visitor<'de> for CowVisitor {
                type Value = Cow<'de, [u8]>;

                fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                    formatter.write_str("a byte array")
                }

                fn visit_borrowed_bytes<E>(self, v: &'de [u8]) -> Result<Self::Value, E>
                where
                    E: Error,
                {
                    Ok(Cow::Borrowed(v))
                }

                fn visit_bytes<E>(self, v: &[u8]) -> Result<Self::Value, E>
                where
                    E: Error,
                {
                    Ok(Cow::Owned(v.to_vec()))
                }

                fn visit_byte_buf<E>(self, v: Vec<u8>) -> Result<Self::Value, E>
                where
                    E: Error,
                {
                    Ok(Cow::Owned(v))
                }
            }
            deserializer.deserialize_bytes(CowVisitor)
        }
    }

    impl<'de, const L: usize> Deserialize<'de> for [u8; L] {
        fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
        where
            D: Deserializer<'de>,
        {
            struct ArrVisitor<const S: usize>;
            impl<'de, const S: usize> Visitor<'de> for ArrVisitor<S> {
                type Value = [u8; S];

                fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                    write!(formatter, "a {}byte array", S)
                }

                fn visit_bytes<E>(self, v: &[u8]) -> Result<Self::Value, E>
                where
                    E: Error,
                {
                    if v.len() != S {
                        return Err(serde::de::Error::invalid_length(v.len(), &self));
                    }
                    let mut array = [0; S];
                    array.copy_from_slice(v);
                    Ok(array)
                }
            }
            deserializer.deserialize_bytes(ArrVisitor)
        }
    }

    /// Wrapper for serializing and deserializing dynamic sized Bytes.
    #[derive(serde::Deserialize, serde::Serialize, Debug, Eq, PartialEq, Clone)]
    #[serde(transparent)]
    pub struct ByteBuf(#[serde(with = "self")] pub Vec<u8>);

    impl ByteBuf {
        pub fn into_vec(self) -> Vec<u8> {
            self.0
        }
    }

    pub fn serialize<T, S>(bytes: &T, serializer: S) -> Result<S::Ok, S::Error>
    where
        T: ?Sized + AsRef<[u8]>,
        S: Serializer,
    {
        Serialize::serialize(bytes.as_ref(), serializer)
    }

    pub fn deserialize<'de, T, D>(deserializer: D) -> Result<T, D::Error>
    where
        T: Deserialize<'de>,
        D: Deserializer<'de>,
    {
        Deserialize::deserialize(deserializer)
    }
}

pub use strict_bytes::ByteBuf as BytesDe;

/// Wrapper for serializing slice of bytes.
#[derive(serde::Serialize)]
#[serde(transparent)]
pub struct BytesSer<'a>(#[serde(with = "strict_bytes")] pub &'a [u8]);

pub fn bytes_32(buf: &[u8]) -> [u8; 32] {
    let mut array = [0; 32];
    array.copy_from_slice(buf.as_ref());
    array
}

#[deprecated = "Use strict_bytes. serde_bytes is a deprecated alias."]
pub use strict_bytes as serde_bytes;

#[cfg(test)]
mod test {
    use serde::{Deserialize, Serialize};

    use crate::{from_slice, strict_bytes, to_vec, BytesDe, BytesSer};

    #[test]
    fn round_trip() {
        let buf = &[1u8, 2, 3, 4][..];
        let serialized = to_vec(&BytesSer(buf)).unwrap();
        let result: BytesDe = from_slice(&serialized).unwrap();
        assert_eq!(buf, &result.0);
    }

    #[test]
    fn wrapper() {
        #[derive(Serialize, Deserialize, Debug, Eq, PartialEq)]
        #[serde(transparent)]
        struct Wrapper(#[serde(with = "strict_bytes")] Vec<u8>);
        let input = Wrapper(vec![1, 2, 3, 4]);
        let serialized = to_vec(&input).unwrap();
        let result: Wrapper = from_slice(&serialized).unwrap();
        assert_eq!(input, result);
    }

    #[test]
    fn from_string_fails() {
        let serialized = to_vec(&"abcde").unwrap();
        from_slice::<BytesDe>(&serialized).expect_err("can't decode string into bytes");
    }

    #[test]
    fn from_list_fails() {
        let serialized = to_vec(&[1u8, 2, 3, 4]).unwrap();
        from_slice::<BytesDe>(&serialized).expect_err("can't decode list into bytes");
    }
}
