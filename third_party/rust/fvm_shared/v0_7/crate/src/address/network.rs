// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use super::{MAINNET_PREFIX, TESTNET_PREFIX};

/// Network defines the preconfigured networks to use with address encoding
#[derive(PartialEq, Eq, Copy, Clone, Debug, Hash)]
#[cfg_attr(feature = "arb", derive(arbitrary::Arbitrary))]
pub enum Network {
    Mainnet = 0,
    Testnet = 1,
}

impl Default for Network {
    fn default() -> Self {
        Network::Mainnet
    }
}

impl Network {
    /// to_prefix is used to convert the network into a string
    /// used when converting address to string
    pub(super) fn to_prefix(self) -> &'static str {
        match self {
            Network::Mainnet => MAINNET_PREFIX,
            Network::Testnet => TESTNET_PREFIX,
        }
    }
}
