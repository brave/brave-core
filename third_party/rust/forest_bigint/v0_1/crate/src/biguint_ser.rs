// Copyright 2020 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use num_bigint::BigUint;
use serde::{Deserialize, Serialize};

/// Wrapper for serializing big ints to match filecoin spec. Serializes as bytes.
#[derive(Serialize)]
#[serde(transparent)]
pub struct BigUintSer<'a>(#[serde(with = "self")] pub &'a BigUint);

/// Wrapper for deserializing as BigUint from bytes.
#[derive(Deserialize, Serialize, Clone)]
#[serde(transparent)]
pub struct BigUintDe(#[serde(with = "self")] pub BigUint);

pub fn serialize<S>(int: &BigUint, serializer: S) -> Result<S::Ok, S::Error>
where
    S: serde::Serializer,
{
    let mut bz = int.to_bytes_be();

    // Insert positive sign byte at start of encoded bytes if non-zero
    if bz == [0] {
        bz = Vec::new()
    } else {
        bz.insert(0, 0);
    }

    // Serialize as bytes
    serde_bytes::Serialize::serialize(&bz, serializer)
}

pub fn deserialize<'de, D>(deserializer: D) -> Result<BigUint, D::Error>
where
    D: serde::Deserializer<'de>,
{
    let bz: &[u8] = serde_bytes::Deserialize::deserialize(deserializer)?;
    if bz.is_empty() {
        return Ok(BigUint::default());
    }

    if bz.get(0) != Some(&0) {
        return Err(serde::de::Error::custom(
            "First byte must be 0 to decode as BigUint",
        ));
    }

    Ok(BigUint::from_bytes_be(&bz[1..]))
}
