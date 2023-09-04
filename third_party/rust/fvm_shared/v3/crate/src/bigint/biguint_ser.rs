// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use std::borrow::Cow;

use fvm_ipld_encoding::strict_bytes;
use num_bigint::BigUint;
use serde::{Deserialize, Serialize};

use super::MAX_BIGINT_SIZE;

/// Wrapper for serializing big ints to match filecoin spec. Serializes as bytes.
#[derive(Serialize)]
#[serde(transparent)]
pub struct BigUintSer<'a>(#[serde(with = "self")] pub &'a BigUint);

/// Wrapper for deserializing as BigUint from bytes.
#[derive(Deserialize, Serialize, Clone, Debug)]
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

    if bz.len() > MAX_BIGINT_SIZE {
        return Err(<S::Error as serde::ser::Error>::custom("BigInt too large"));
    }

    // Serialize as bytes
    strict_bytes::Serialize::serialize(&bz, serializer)
}

pub fn deserialize<'de, D>(deserializer: D) -> Result<BigUint, D::Error>
where
    D: serde::Deserializer<'de>,
{
    let bz: Cow<'de, [u8]> = strict_bytes::Deserialize::deserialize(deserializer)?;
    if bz.is_empty() {
        return Ok(BigUint::default());
    }

    if bz.first() != Some(&0) {
        return Err(serde::de::Error::custom(
            "First byte must be 0 to decode as BigUint",
        ));
    }

    if bz.len() > MAX_BIGINT_SIZE {
        return Err(<D::Error as serde::de::Error>::custom("BigInt too large"));
    }

    Ok(BigUint::from_bytes_be(&bz[1..]))
}

#[cfg(test)]
mod tests {
    use fvm_ipld_encoding::{from_slice, to_vec};

    use super::*;

    #[test]
    fn test_biguint_max() {
        let max_limbs = MAX_BIGINT_SIZE / 4; // 32bit limbs to bytes
        let good = BigUint::new(vec![u32::MAX; max_limbs - 1]);

        let good_bytes = to_vec(&BigUintSer(&good)).expect("should be good");
        let good_back: BigUintDe = from_slice(&good_bytes).unwrap();
        assert_eq!(good_back.0, good);

        // max limbs will fail as the sign is prepended
        let bad1 = BigUint::new(vec![u32::MAX; max_limbs]);
        let bad2 = BigUint::new(vec![u32::MAX; max_limbs + 1]);

        assert!(to_vec(&BigUintSer(&bad1)).is_err());
        assert!(to_vec(&BigUintSer(&bad2)).is_err());

        let bad_bytes = {
            let mut source = bad1.to_bytes_be();
            source.insert(0, 0);
            to_vec(&strict_bytes::ByteBuf(source)).unwrap()
        };

        let res: Result<BigUintDe, _> = from_slice(&bad_bytes);
        assert!(res.is_err());
        assert!(res.unwrap_err().to_string().contains("BigInt too large"));
    }
}
