// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use std::sync::atomic::{AtomicU8, Ordering};

use num_derive::{FromPrimitive, ToPrimitive};
use num_traits::{FromPrimitive, ToPrimitive};

use super::{Address, Error, MAINNET_PREFIX, TESTNET_PREFIX};

static ATOMIC_NETWORK: AtomicU8 = AtomicU8::new(0);

/// Network defines the preconfigured networks to use with address encoding
#[derive(Copy, Clone, Debug, Hash, PartialEq, Eq, PartialOrd, Ord, FromPrimitive, ToPrimitive)]
#[repr(u8)]
#[cfg_attr(feature = "arb", derive(arbitrary::Arbitrary))]
#[derive(Default)]
pub enum Network {
    #[default]
    Mainnet = 0,
    Testnet = 1,
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

    /// from_prefix is used to convert the network from a string
    /// used when parsing
    pub(super) fn from_prefix(s: &str) -> Result<Self, Error> {
        match s {
            MAINNET_PREFIX => Ok(Network::Mainnet),
            TESTNET_PREFIX => Ok(Network::Testnet),
            _ => Err(Error::UnknownNetwork),
        }
    }

    /// Parse an address belonging to this network.
    pub fn parse_address(self, addr: &str) -> Result<Address, Error> {
        let (addr, network) = super::parse_address(addr)?;
        if network != self {
            return Err(Error::UnknownNetwork);
        }
        Ok(addr)
    }
}

/// Gets the current network.
pub fn current_network() -> Network {
    Network::from_u8(ATOMIC_NETWORK.load(Ordering::Relaxed)).unwrap_or_default()
}

/// Sets the default network.
///
/// The network is used to differentiate between different filecoin networks _in text_ but isn't
/// actually encoded in the binary representation of addresses. Changing the current network will:
///
/// 1. Change the prefix used when formatting an address as a string.
/// 2. Change the prefix _accepted_ when parsing an address.
pub fn set_current_network(network: Network) {
    ATOMIC_NETWORK.store(network.to_u8().unwrap_or_default(), Ordering::Relaxed)
}

#[cfg(test)]
mod tests {
    use std::str::FromStr;

    use super::*;
    use crate::address::Address;

    // We fork this test into a new process because it messes with global state.
    use rusty_fork::rusty_fork_test;
    rusty_fork_test! {
        #[test]
        fn set_network() {
            assert_eq!(current_network(), Network::default());
            assert_eq!(Network::default(), Network::Mainnet);

            // We're in mainnet mode.
            let addr1 = Address::from_str("f01");
            Address::from_str("t01").expect_err("should have failed to parse testnet address");
            assert_eq!(
                addr1,
                Network::Testnet.parse_address("t01"),
                "parsing an explicit address should still work"
            );

            // Switch to testnet mode.
            set_current_network(Network::Testnet);

            // Now we're in testnet mode.
            let addr2 = Address::from_str("t01");
            Address::from_str("f01").expect_err("should have failed to parse testnet address");

            // Networks are relevent for parsing only.
            assert_eq!(addr1, addr2)
        }
    }
}
