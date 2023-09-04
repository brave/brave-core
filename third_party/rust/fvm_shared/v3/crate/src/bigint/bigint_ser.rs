// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use std::borrow::Cow;

use fvm_ipld_encoding::strict_bytes;
use num_bigint::{BigInt, Sign};
use serde::{Deserialize, Serialize};

use super::MAX_BIGINT_SIZE;

/// Wrapper for serializing big ints to match filecoin spec. Serializes as bytes.
#[derive(Serialize)]
#[serde(transparent)]
pub struct BigIntSer<'a>(#[serde(with = "self")] pub &'a BigInt);

/// Wrapper for deserializing as BigInt from bytes.
#[derive(Deserialize, Serialize, Clone, Default, PartialEq, Eq, Debug)]
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

    if bz.len() > MAX_BIGINT_SIZE {
        return Err(<S::Error as serde::ser::Error>::custom("BigInt too large"));
    }

    // Serialize as bytes
    strict_bytes::Serialize::serialize(&bz, serializer)
}

/// Deserializes bytes into big int.
pub fn deserialize<'de, D>(deserializer: D) -> Result<BigInt, D::Error>
where
    D: serde::Deserializer<'de>,
{
    let bz: Cow<'de, [u8]> = strict_bytes::Deserialize::deserialize(deserializer)?;
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

    if bz.len() > MAX_BIGINT_SIZE {
        return Err(<D::Error as serde::de::Error>::custom("BigInt too large"));
    }

    Ok(BigInt::from_bytes_be(sign, &bz[1..]))
}

#[cfg(test)]
mod tests {
    use fvm_ipld_encoding::{from_slice, to_vec};

    use super::*;

    #[test]
    fn test_bigiint_max() {
        let max_limbs = MAX_BIGINT_SIZE / 4; // 32bit limbs to bytes
        let good = BigInt::new(Sign::Plus, vec![u32::MAX; max_limbs - 1]);
        let good_neg = BigInt::new(Sign::Minus, vec![u32::MAX; max_limbs - 1]);

        let good_bytes = to_vec(&BigIntSer(&good)).expect("should be good");
        let good_back: BigIntDe = from_slice(&good_bytes).unwrap();
        assert_eq!(good_back.0, good);

        let good_neg_bytes = to_vec(&BigIntSer(&good_neg)).expect("should be good");
        let good_neg_back: BigIntDe = from_slice(&good_neg_bytes).unwrap();
        assert_eq!(good_neg_back.0, good_neg);

        // max limbs will fail as the sign is prepended
        let bad1 = BigInt::new(Sign::Plus, vec![u32::MAX; max_limbs]);
        let bad1_neg = BigInt::new(Sign::Minus, vec![u32::MAX; max_limbs]);
        let bad2 = BigInt::new(Sign::Plus, vec![u32::MAX; max_limbs + 1]);
        let bad2_neg = BigInt::new(Sign::Minus, vec![u32::MAX; max_limbs + 1]);

        assert!(to_vec(&BigIntSer(&bad1)).is_err());
        assert!(to_vec(&BigIntSer(&bad1_neg)).is_err());
        assert!(to_vec(&BigIntSer(&bad2)).is_err());
        assert!(to_vec(&BigIntSer(&bad2_neg)).is_err());

        let bad_bytes = {
            let (sign, mut source) = bad1.to_bytes_be();
            match sign {
                Sign::Minus => source.insert(0, 0),
                _ => source.insert(0, 1),
            }
            to_vec(&strict_bytes::ByteBuf(source)).unwrap()
        };

        let res: Result<BigIntDe, _> = from_slice(&bad_bytes);
        assert!(res.is_err());
        assert!(res.unwrap_err().to_string().contains("BigInt too large"));
    }
}
