// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

#[cfg(feature = "arb")]
use std::array::from_fn;
use std::convert::TryInto;
use std::hash::Hash;
use std::u64;

use super::{
    from_leb_bytes, to_leb_bytes, Error, Protocol, BLS_PUB_LEN, MAX_SUBADDRESS_LEN,
    PAYLOAD_HASH_LEN,
};
use crate::ActorID;

/// A "delegated" (f4) address.
#[derive(Copy, Clone, Debug, Hash, PartialEq, Eq, PartialOrd, Ord)]
pub struct DelegatedAddress {
    namespace: ActorID,
    length: usize,
    buffer: [u8; MAX_SUBADDRESS_LEN],
}

#[cfg(feature = "arb")]
impl quickcheck::Arbitrary for DelegatedAddress {
    fn arbitrary(g: &mut quickcheck::Gen) -> Self {
        Self {
            namespace: ActorID::arbitrary(g),
            length: usize::arbitrary(g) % (MAX_SUBADDRESS_LEN + 1),
            buffer: from_fn(|_| u8::arbitrary(g)),
        }
    }
}

#[cfg(feature = "arb")]
impl<'a> arbitrary::Arbitrary<'a> for DelegatedAddress {
    fn arbitrary(u: &mut arbitrary::Unstructured<'a>) -> arbitrary::Result<Self> {
        Ok(DelegatedAddress {
            namespace: arbitrary::Arbitrary::arbitrary(u)?,
            length: u.int_in_range(0usize..=MAX_SUBADDRESS_LEN)?,
            buffer: arbitrary::Arbitrary::arbitrary(u)?,
        })
    }
}

impl DelegatedAddress {
    /// Construct a new delegated address from the namespace (actor id) and subaddress.
    pub fn new(namespace: ActorID, subaddress: &[u8]) -> Result<Self, Error> {
        let length = subaddress.len();
        if length > MAX_SUBADDRESS_LEN {
            return Err(Error::InvalidPayloadLength(length));
        }
        let mut addr = DelegatedAddress {
            namespace,
            length,
            buffer: [0u8; MAX_SUBADDRESS_LEN],
        };
        addr.buffer[..length].copy_from_slice(&subaddress[..length]);
        Ok(addr)
    }

    /// Returns the delegated address's namespace .
    #[inline]
    pub fn namespace(&self) -> ActorID {
        self.namespace
    }

    /// Returns the delegated address's subaddress .
    #[inline]
    pub fn subaddress(&self) -> &[u8] {
        &self.buffer[..self.length]
    }
}

/// Payload is the data of the Address. Variants are the supported Address protocols.
#[derive(Copy, Clone, Debug, Hash, PartialEq, Eq, PartialOrd, Ord)]
#[cfg_attr(feature = "arb", derive(arbitrary::Arbitrary))]
pub enum Payload {
    /// f0: ID protocol address.
    ID(u64),
    /// f1: SECP256K1 key address, 20 byte hash of PublicKey.
    Secp256k1([u8; PAYLOAD_HASH_LEN]),
    /// f2: Actor protocol address, 20 byte hash of actor data.
    Actor([u8; PAYLOAD_HASH_LEN]),
    /// f3: BLS key address, full 48 byte public key.
    BLS([u8; BLS_PUB_LEN]),
    /// f4: Delegated address, a namespace with an arbitrary subaddress.
    Delegated(DelegatedAddress),
}

#[cfg(feature = "arb")]
impl quickcheck::Arbitrary for Payload {
    fn arbitrary(g: &mut quickcheck::Gen) -> Self {
        match u8::arbitrary(g) % 5 {
            0 => Payload::ID(u64::arbitrary(g)),
            1 => Payload::Secp256k1(from_fn(|_| u8::arbitrary(g))),
            2 => Payload::Actor(from_fn(|_| u8::arbitrary(g))),
            3 => Payload::BLS(from_fn(|_| u8::arbitrary(g))),
            _ => Payload::Delegated(DelegatedAddress::arbitrary(g)),
        }
    }
}

impl Payload {
    /// Returns encoded bytes of Address without the protocol byte.
    pub fn to_raw_bytes(self) -> Vec<u8> {
        use Payload::*;
        match self {
            ID(i) => to_leb_bytes(i),
            Secp256k1(arr) => arr.to_vec(),
            Actor(arr) => arr.to_vec(),
            BLS(arr) => arr.to_vec(),
            Delegated(addr) => {
                let mut buf = to_leb_bytes(addr.namespace());
                buf.extend(addr.subaddress());
                buf
            }
        }
    }

    /// Returns encoded bytes of Address including the protocol byte.
    pub fn to_bytes(self) -> Vec<u8> {
        let mut bz = self.to_raw_bytes();
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
            Protocol::Delegated => {
                let (id, remaining) = unsigned_varint::decode::u64(payload)?;
                Self::Delegated(DelegatedAddress::new(id, remaining)?)
            }
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
            Payload::Delegated { .. } => Self::Delegated,
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
            Payload::Delegated { .. } => Self::Delegated,
        }
    }
}

impl TryFrom<&Payload> for DelegatedAddress {
    type Error = Error;

    fn try_from(value: &Payload) -> Result<Self, Self::Error> {
        match value {
            Payload::Delegated(d) => Ok(*d),
            _ => Err(Error::NonDelegatedAddress),
        }
    }
}

#[cfg(feature = "testing")]
impl Default for Payload {
    fn default() -> Self {
        Payload::ID(0)
    }
}
