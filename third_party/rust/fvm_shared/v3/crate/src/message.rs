// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use anyhow::anyhow;
use fvm_ipld_encoding::de::{Deserialize, Deserializer};
use fvm_ipld_encoding::ser::{Serialize, Serializer};
use fvm_ipld_encoding::RawBytes;

use crate::address::Address;
use crate::econ::TokenAmount;
use crate::MethodNum;

/// Default Unsigned VM message type which includes all data needed for a state transition
#[cfg_attr(feature = "testing", derive(Default))]
#[derive(PartialEq, Clone, Debug, Hash, Eq)]
pub struct Message {
    pub version: u64,
    pub from: Address,
    pub to: Address,
    pub sequence: u64,
    pub value: TokenAmount,
    pub method_num: MethodNum,
    pub params: RawBytes,
    pub gas_limit: u64,
    pub gas_fee_cap: TokenAmount,
    pub gas_premium: TokenAmount,
}

impl Message {
    /// Does some basic checks on the Message to see if the fields are valid.
    pub fn check(self: &Message) -> anyhow::Result<()> {
        if self.gas_limit == 0 {
            return Err(anyhow!("Message has no gas limit set"));
        }
        // TODO: max block limit?
        if self.gas_limit > i64::MAX as u64 {
            return Err(anyhow!("Message gas exceeds i64 max"));
        }
        Ok(())
    }
}

impl Serialize for Message {
    fn serialize<S>(&self, s: S) -> std::result::Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        (
            &self.version,
            &self.to,
            &self.from,
            &self.sequence,
            &self.value,
            &self.gas_limit,
            &self.gas_fee_cap,
            &self.gas_premium,
            &self.method_num,
            &self.params,
        )
            .serialize(s)
    }
}

impl<'de> Deserialize<'de> for Message {
    fn deserialize<D>(deserializer: D) -> std::result::Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let (
            version,
            to,
            from,
            sequence,
            value,
            gas_limit,
            gas_fee_cap,
            gas_premium,
            method_num,
            params,
        ) = Deserialize::deserialize(deserializer)?;
        Ok(Self {
            version,
            from,
            to,
            sequence,
            value,
            method_num,
            params,
            gas_limit,
            gas_fee_cap,
            gas_premium,
        })
    }
}

#[cfg(feature = "arb")]
impl quickcheck::Arbitrary for Message {
    fn arbitrary(g: &mut quickcheck::Gen) -> Self {
        Self {
            to: Address::arbitrary(g),
            from: Address::arbitrary(g),
            version: u64::arbitrary(g),
            sequence: u64::arbitrary(g),
            value: TokenAmount::arbitrary(g),
            method_num: u64::arbitrary(g),
            params: fvm_ipld_encoding::RawBytes::new(Vec::arbitrary(g)),
            gas_limit: u64::arbitrary(g),
            gas_fee_cap: TokenAmount::arbitrary(g),
            gas_premium: TokenAmount::arbitrary(g),
        }
    }
}
