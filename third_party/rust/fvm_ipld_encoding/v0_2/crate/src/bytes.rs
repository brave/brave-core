// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use serde::{de, Deserialize, Deserializer, Serialize, Serializer};
use serde_bytes::ByteBuf;

/// Wrapper for serializing slice of bytes.
#[derive(Serialize)]
#[serde(transparent)]
pub struct BytesSer<'a>(#[serde(with = "serde_bytes")] pub &'a [u8]);

/// Wrapper for deserializing dynamic sized Bytes.
#[derive(Deserialize, Serialize, Debug, PartialEq, Clone)]
#[serde(transparent)]
pub struct BytesDe(#[serde(with = "serde_bytes")] pub Vec<u8>);

/// Wrapper for deserializing array of 32 Bytes.
pub struct Byte32De(pub [u8; 32]);

impl Serialize for Byte32De {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        <[u8] as serde_bytes::Serialize>::serialize(&self.0, serializer)
    }
}

impl<'de> Deserialize<'de> for Byte32De {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let bz_buf: ByteBuf = Deserialize::deserialize(deserializer)?;
        if bz_buf.len() != 32 {
            return Err(de::Error::custom("Array of bytes not length 32"));
        }
        let mut array = [0; 32];
        array.copy_from_slice(bz_buf.as_ref());
        Ok(Byte32De(array))
    }
}

pub fn bytes_32(buf: &[u8]) -> [u8; 32] {
    let mut array = [0; 32];
    array.copy_from_slice(buf.as_ref());
    array
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{from_slice, to_vec};

    #[test]
    fn array_symmetric_serialization() {
        let vec: Vec<u8> = (0..32).collect::<Vec<u8>>();
        let slice_bz = to_vec(&BytesSer(&vec)).unwrap();
        let Byte32De(arr) = from_slice(&slice_bz).unwrap();
        // Check decoded array against slice
        assert_eq!(arr.as_ref(), vec.as_slice());
        // Check re-encoded array is equal to the slice encoded
        assert_eq!(to_vec(&BytesSer(&arr)).unwrap(), slice_bz);
    }
}
