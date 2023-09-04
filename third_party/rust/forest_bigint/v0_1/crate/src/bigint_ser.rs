// Copyright 2020 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use num_bigint::{BigInt, Sign};
use serde::{Deserialize, Serialize};

/// Wrapper for serializing big ints to match filecoin spec. Serializes as bytes.
#[derive(Serialize)]
#[serde(transparent)]
pub struct BigIntSer<'a>(#[serde(with = "self")] pub &'a BigInt);

/// Wrapper for deserializing as BigInt from bytes.
#[derive(Deserialize, Serialize, Clone, Default)]
#[serde(transparent)]
pub struct BigIntDe(#[serde(with = "self")] pub BigInt);

/// Serializes big int as bytes following Filecoin spec.
pub fn serialize<S>(int: &BigInt, serializer: S) -> Result<S::Ok, S::Error>
where
    S: serde::Serializer,
{
    let (sign, mut bz) = int.to_bytes_be();

    // Insert sign byte at start of encoded bytes
    match sign {
        Sign::Minus => bz.insert(0, 1),
        Sign::Plus => bz.insert(0, 0),
        Sign::NoSign => bz = Vec::new(),
    }

    // Serialize as bytes
    serde_bytes::Serialize::serialize(&bz, serializer)
}

/// Deserializes bytes into big int.
pub fn deserialize<'de, D>(deserializer: D) -> Result<BigInt, D::Error>
where
    D: serde::Deserializer<'de>,
{
    let bz: &[u8] = serde_bytes::Deserialize::deserialize(deserializer)?;
    if bz.is_empty() {
        return Ok(BigInt::default());
    }
    let sign_byte = bz[0];
    let sign: Sign = match sign_byte {
        1 => Sign::Minus,
        0 => Sign::Plus,
        _ => {
            return Err(serde::de::Error::custom(
                "First byte must be valid sign (0, 1)",
            ));
        }
    };
    Ok(BigInt::from_bytes_be(sign, &bz[1..]))
}

#[cfg(feature = "json")]
pub mod json {
    use super::*;
    use std::str::FromStr;

    /// Serializes BigInt as String
    pub fn serialize<S>(int: &BigInt, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        String::serialize(&int.to_string(), serializer)
    }

    /// Deserializes String into BigInt.
    pub fn deserialize<'de, D>(deserializer: D) -> Result<BigInt, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let s = String::deserialize(deserializer)?;
        BigInt::from_str(&s).map_err(serde::de::Error::custom)
    }
}
