// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use std::convert::TryInto;
use std::hash::Hash;
use std::u64;

use super::{from_leb_bytes, to_leb_bytes, Error, Protocol, BLS_PUB_LEN, PAYLOAD_HASH_LEN};

/// Payload is the data of the Address. Variants are the supported Address protocols.
#[derive(Copy, Clone, Debug, Hash, PartialEq, Eq)]
#[cfg_attr(feature = "arb", derive(arbitrary::Arbitrary))]
pub enum Payload {
    /// ID protocol address.
    ID(u64),
    /// SECP256K1 key address, 20 byte hash of PublicKey
    Secp256k1([u8; PAYLOAD_HASH_LEN]),
    /// Actor protocol address, 20 byte hash of actor data
    Actor([u8; PAYLOAD_HASH_LEN]),
    /// BLS key address, full 48 byte public key
    BLS([u8; BLS_PUB_LEN]),
}

impl Payload {
    /// Returns encoded bytes of Address without the protocol byte.
    pub fn to_raw_bytes(self) -> Vec<u8> {
        use Payload::*;
        match self {
            ID(i) => to_leb_bytes(i).unwrap(),
            Secp256k1(arr) => arr.to_vec(),
            Actor(arr) => arr.to_vec(),
            BLS(arr) => arr.to_vec(),
        }
    }

    /// Returns encoded bytes of Address including the protocol byte.
    pub fn to_bytes(self) -> Vec<u8> {
        use Payload::*;
        let mut bz = match self {
            ID(i) => to_leb_bytes(i).unwrap(),
            Secp256k1(arr) => arr.to_vec(),
            Actor(arr) => arr.to_vec(),
            BLS(arr) => arr.to_vec(),
        };

        bz.insert(0, Protocol::from(self) as u8);
        bz
    }

    /// Generates payload from raw bytes and protocol.
    pub fn new(protocol: Protocol, payload: &[u8]) -> Result<Self, Error> {
        let payload = match protocol {
            Protocol::ID => Self::ID(from_leb_bytes(payload)?),
            Protocol::Secp256k1 => Self::Secp256k1(
                payload
                    .try_into()
                    .map_err(|_| Error::InvalidPayloadLength(payload.len()))?,
            ),
            Protocol::Actor => Self::Actor(
                payload
                    .try_into()
                    .map_err(|_| Error::InvalidPayloadLength(payload.len()))?,
            ),
            Protocol::BLS => Self::BLS(
                payload
                    .try_into()
                    .map_err(|_| Error::InvalidPayloadLength(payload.len()))?,
            ),
        };
        Ok(payload)
    }
}

impl From<Payload> for Protocol {
    fn from(pl: Payload) -> Self {
        match pl {
            Payload::ID(_) => Self::ID,
            Payload::Secp256k1(_) => Self::Secp256k1,
            Payload::Actor(_) => Self::Actor,
            Payload::BLS(_) => Self::BLS,
        }
    }
}

impl From<&Payload> for Protocol {
    fn from(pl: &Payload) -> Self {
        match pl {
            Payload::ID(_) => Self::ID,
            Payload::Secp256k1(_) => Self::Secp256k1,
            Payload::Actor(_) => Self::Actor,
            Payload::BLS(_) => Self::BLS,
        }
    }
}

#[cfg(feature = "testing")]
impl Default for Payload {
    fn default() -> Self {
        Payload::ID(0)
    }
}
