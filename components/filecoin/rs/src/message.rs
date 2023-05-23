// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use fvm_ipld_encoding::RawBytes;
use fvm_shared::address::Address;
use fvm_shared::econ::TokenAmount;
use fvm_shared::message::Message;
use fvm_shared::MethodNum;
use serde::{Deserialize, Serialize};

#[cfg_attr(feature = "with-arbitrary", derive(arbitrary::Arbitrary))]
#[derive(Serialize, Deserialize)]
// https://github.com/Zondax/filecoin-signing-tools/blob/222362ae38a1ccd8523ea026d5298d4718b43e2b/extras/src/message.rs#L15
#[serde(remote = "Message", rename_all = "PascalCase")]
pub struct MessageAPI {
    #[serde(skip)]
    pub version: u64,
    #[serde(with = "address")]
    pub from: Address,
    #[serde(with = "address")]
    pub to: Address,
    #[serde(rename = "Nonce")]
    pub sequence: u64,
    #[serde(with = "tokenamount")]
    pub value: TokenAmount,
    #[serde(rename = "Method")]
    pub method_num: MethodNum,
    #[serde(with = "rawbytes")]
    pub params: RawBytes,
    pub gas_limit: u64,
    #[serde(with = "tokenamount")]
    pub gas_fee_cap: TokenAmount,
    #[serde(with = "tokenamount")]
    pub gas_premium: TokenAmount,
}

pub mod tokenamount {
    use fvm_shared::bigint::BigInt;
    use fvm_shared::econ::TokenAmount;
    use serde::{de, Deserialize, Deserializer, Serializer};
    use std::str::FromStr;

    pub fn serialize<S>(token_amount: &TokenAmount, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        let s = token_amount.to_string();
        serializer.serialize_str(&s)
    }

    pub fn deserialize<'de, D>(deserializer: D) -> Result<TokenAmount, D::Error>
    where
        D: Deserializer<'de>,
    {
        let s = String::deserialize(deserializer)?;
        let atto = BigInt::from_str(&s).map_err(de::Error::custom)?;
        Ok(TokenAmount::from_atto(atto))
    }
}

pub mod rawbytes {
    use base64::{decode, encode};
    use fvm_ipld_encoding::RawBytes;
    use serde::{de, Deserialize, Deserializer, Serializer};

    pub fn serialize<S>(raw: &RawBytes, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        let s = encode(raw.bytes());
        serializer.serialize_str(&s)
    }

    pub fn deserialize<'de, D>(deserializer: D) -> Result<RawBytes, D::Error>
    where
        D: Deserializer<'de>,
    {
        let s = String::deserialize(deserializer)?;
        let raw = decode(s).map_err(de::Error::custom)?;
        Ok(RawBytes::new(raw))
    }
}

pub mod address {
    use fvm_shared::address::Address;
    use serde::{de, Deserialize, Deserializer, Serializer};
    use std::str::FromStr;

    pub fn serialize<S>(address: &Address, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        let s = address.to_string();
        serializer.serialize_str(&s)
    }

    pub fn deserialize<'de, D>(deserializer: D) -> Result<Address, D::Error>
    where
        D: Deserializer<'de>,
    {
        let s = String::deserialize(deserializer)?;
        Address::from_str(&s).map_err(de::Error::custom)
    }
}
