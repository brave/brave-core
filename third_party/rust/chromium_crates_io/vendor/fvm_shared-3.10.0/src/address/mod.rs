// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

mod errors;
mod network;
mod payload;
mod protocol;

use std::borrow::Cow;
use std::fmt;
use std::hash::Hash;
use std::str::FromStr;

use data_encoding::Encoding;
use data_encoding_macro::new_encoding;
use fvm_ipld_encoding::strict_bytes;
use serde::{de, Deserialize, Deserializer, Serialize, Serializer};

pub use self::errors::Error;
pub use self::network::{current_network, set_current_network, Network};
pub use self::payload::{DelegatedAddress, Payload};
pub use self::protocol::Protocol;
use crate::ActorID;

/// defines the encoder for base32 encoding with the provided string with no padding
const ADDRESS_ENCODER: Encoding = new_encoding! {
    symbols: "abcdefghijklmnopqrstuvwxyz234567",
    padding: None,
};

/// Hash length of payload for Secp and Actor addresses.
pub const PAYLOAD_HASH_LEN: usize = 20;

/// Uncompressed secp public key used for validation of Secp addresses.
pub const SECP_PUB_LEN: usize = 65;

/// BLS public key length used for validation of BLS addresses.
pub const BLS_PUB_LEN: usize = 48;

/// Max length of f4 sub addresses.
pub const MAX_SUBADDRESS_LEN: usize = 54;

/// Defines first available ID address after builtin actors
pub const FIRST_NON_SINGLETON_ADDR: ActorID = 100;

lazy_static::lazy_static! {
    static ref BLS_ZERO_ADDR_BYTES: [u8; BLS_PUB_LEN] = {
        let bz_addr = Network::Mainnet.parse_address("f3yaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaby2smx7a");
        if let Ok(Address {payload: Payload::BLS(pubkey), ..}) = bz_addr {
            pubkey
        } else {
            panic!("failed to parse BLS address from provided BLS_ZERO_ADDR string")
        }
    };
}

/// Length of the checksum hash for string encodings.
pub const CHECKSUM_HASH_LEN: usize = 4;

/// The max encoded length of an address.
pub const MAX_ADDRESS_LEN: usize = 65;

const MAX_ADDRRESS_TEXT_LEN: usize = 138;
const MAINNET_PREFIX: &str = "f";
const TESTNET_PREFIX: &str = "t";

/// Address is the struct that defines the protocol and data payload conversion from either
/// a public key or value
#[derive(Copy, Clone, Debug, Hash, PartialEq, Eq, PartialOrd, Ord)]
#[cfg_attr(feature = "testing", derive(Default))]
#[cfg_attr(feature = "arb", derive(arbitrary::Arbitrary))]
pub struct Address {
    payload: Payload,
}

impl Address {
    /// Construct a new address with the specified network.
    fn new(protocol: Protocol, bz: &[u8]) -> Result<Self, Error> {
        Ok(Self {
            payload: Payload::new(protocol, bz)?,
        })
    }

    /// Creates address from encoded bytes.
    pub fn from_bytes(bz: &[u8]) -> Result<Self, Error> {
        if bz.len() < 2 {
            Err(Error::InvalidLength)
        } else {
            let protocol = Protocol::from_byte(bz[0]).ok_or(Error::UnknownProtocol)?;
            Self::new(protocol, &bz[1..])
        }
    }

    /// Generates new address using ID protocol.
    pub const fn new_id(id: u64) -> Self {
        Self {
            payload: Payload::ID(id),
        }
    }

    /// Generates new address using Secp256k1 pubkey.
    pub fn new_secp256k1(pubkey: &[u8]) -> Result<Self, Error> {
        if pubkey.len() != SECP_PUB_LEN {
            return Err(Error::InvalidSECPLength(pubkey.len()));
        }
        Ok(Self {
            payload: Payload::Secp256k1(address_hash(pubkey)),
        })
    }

    /// Generates new address using the Actor protocol.
    pub fn new_actor(data: &[u8]) -> Self {
        Self {
            payload: Payload::Actor(address_hash(data)),
        }
    }

    /// Generates a new delegated address from a namespace and a subaddress.
    pub fn new_delegated(ns: ActorID, subaddress: &[u8]) -> Result<Self, Error> {
        Ok(Self {
            payload: Payload::Delegated(DelegatedAddress::new(ns, subaddress)?),
        })
    }

    /// Generates new address using BLS pubkey.
    pub fn new_bls(pubkey: &[u8]) -> Result<Self, Error> {
        if pubkey.len() != BLS_PUB_LEN {
            return Err(Error::InvalidBLSLength(pubkey.len()));
        }
        let mut key = [0u8; BLS_PUB_LEN];
        key.copy_from_slice(pubkey);
        Ok(Self {
            payload: Payload::BLS(key),
        })
    }

    pub fn is_bls_zero_address(&self) -> bool {
        match self.payload {
            Payload::BLS(payload_bytes) => payload_bytes == *BLS_ZERO_ADDR_BYTES,
            _ => false,
        }
    }

    /// Returns protocol for Address
    pub fn protocol(&self) -> Protocol {
        Protocol::from(self.payload)
    }

    /// Returns the `Payload` object from the address, where the respective protocol data is kept
    /// in an enum separated by protocol
    pub fn payload(&self) -> &Payload {
        &self.payload
    }

    /// Converts Address into `Payload` object, where the respective protocol data is kept
    /// in an enum separated by protocol
    pub fn into_payload(self) -> Payload {
        self.payload
    }

    /// Returns the raw bytes data payload of the Address
    pub fn payload_bytes(&self) -> Vec<u8> {
        self.payload.to_raw_bytes()
    }

    /// Returns encoded bytes of Address
    pub fn to_bytes(self) -> Vec<u8> {
        self.payload.to_bytes()
    }

    /// Get ID of the address. ID protocol only.
    pub fn id(&self) -> Result<u64, Error> {
        match self.payload {
            Payload::ID(id) => Ok(id),
            _ => Err(Error::NonIDAddress),
        }
    }
}

impl fmt::Display for Address {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let protocol = self.protocol();

        // write `fP` where P is the protocol number.
        write!(f, "{}{}", current_network().to_prefix(), protocol)?;

        fn write_payload(
            f: &mut fmt::Formatter<'_>,
            protocol: Protocol,
            prefix: Option<&[u8]>,
            data: &[u8],
        ) -> fmt::Result {
            let mut hasher = blake2b_simd::Params::new()
                .hash_length(CHECKSUM_HASH_LEN)
                .to_state();
            hasher.update(&[protocol as u8]);
            if let Some(prefix) = prefix {
                hasher.update(prefix);
            }
            hasher.update(data);

            let mut buf = Vec::with_capacity(data.len() + CHECKSUM_HASH_LEN);
            buf.extend(data);
            buf.extend(hasher.finalize().as_bytes());

            f.write_str(&ADDRESS_ENCODER.encode(&buf))
        }

        match self.payload() {
            Payload::ID(id) => write!(f, "{}", id),
            Payload::Secp256k1(data) | Payload::Actor(data) => {
                write_payload(f, protocol, None, data)
            }
            Payload::BLS(data) => write_payload(f, protocol, None, data),
            Payload::Delegated(addr) => {
                write!(f, "{}f", addr.namespace())?;
                write_payload(
                    f,
                    protocol,
                    Some(unsigned_varint::encode::u64(
                        addr.namespace(),
                        &mut unsigned_varint::encode::u64_buffer(),
                    )),
                    addr.subaddress(),
                )
            }
        }
    }
}

#[cfg(feature = "arb")]
impl quickcheck::Arbitrary for Address {
    fn arbitrary(g: &mut quickcheck::Gen) -> Self {
        Self {
            payload: Payload::arbitrary(g),
        }
    }
}

pub(self) fn parse_address(addr: &str) -> Result<(Address, Network), Error> {
    if addr.len() > MAX_ADDRRESS_TEXT_LEN || addr.len() < 3 {
        return Err(Error::InvalidLength);
    }
    let network = Network::from_prefix(addr.get(0..1).ok_or(Error::UnknownNetwork)?)?;

    // get protocol from second character
    let protocol: Protocol = match addr.get(1..2).ok_or(Error::UnknownProtocol)? {
        "0" => Protocol::ID,
        "1" => Protocol::Secp256k1,
        "2" => Protocol::Actor,
        "3" => Protocol::BLS,
        "4" => Protocol::Delegated,
        _ => {
            return Err(Error::UnknownProtocol);
        }
    };

    fn validate_and_split_checksum<'a>(
        protocol: Protocol,
        prefix: Option<&[u8]>,
        payload: &'a [u8],
    ) -> Result<&'a [u8], Error> {
        if payload.len() < CHECKSUM_HASH_LEN {
            return Err(Error::InvalidLength);
        }
        let (payload, csum) = payload.split_at(payload.len() - CHECKSUM_HASH_LEN);
        let mut hasher = blake2b_simd::Params::new()
            .hash_length(CHECKSUM_HASH_LEN)
            .to_state();
        hasher.update(&[protocol as u8]);
        if let Some(prefix) = prefix {
            hasher.update(prefix);
        }
        hasher.update(payload);
        if hasher.finalize().as_bytes() != csum {
            return Err(Error::InvalidChecksum);
        }
        Ok(payload)
    }

    // bytes after the protocol character is the data payload of the address
    let raw = addr.get(2..).ok_or(Error::InvalidPayload)?;
    let addr = match protocol {
        Protocol::ID => {
            if raw.len() > 20 {
                // 20 is max u64 as string
                return Err(Error::InvalidLength);
            }
            let id = raw.parse::<u64>()?;
            Address {
                payload: Payload::ID(id),
            }
        }
        Protocol::Delegated => {
            let (id, subaddr) = raw.split_once('f').ok_or(Error::InvalidPayload)?;
            if id.len() > 20 {
                // 20 is max u64 as string
                return Err(Error::InvalidLength);
            }
            let id = id.parse::<u64>()?;
            // decode subaddr
            let subaddr_csum = ADDRESS_ENCODER.decode(subaddr.as_bytes())?;
            // validate and split subaddr.
            let subaddr = validate_and_split_checksum(
                protocol,
                Some(unsigned_varint::encode::u64(
                    id,
                    &mut unsigned_varint::encode::u64_buffer(),
                )),
                &subaddr_csum,
            )?;

            Address {
                payload: Payload::Delegated(DelegatedAddress::new(id, subaddr)?),
            }
        }
        Protocol::Secp256k1 | Protocol::Actor | Protocol::BLS => {
            // decode using byte32 encoding
            let payload_csum = ADDRESS_ENCODER.decode(raw.as_bytes())?;
            // validate and split payload.
            let payload = validate_and_split_checksum(protocol, None, &payload_csum)?;

            // sanity check to make sure address hash values are correct length
            if match protocol {
                Protocol::Secp256k1 | Protocol::Actor => PAYLOAD_HASH_LEN,
                Protocol::BLS => BLS_PUB_LEN,
                _ => unreachable!(),
            } != payload.len()
            {
                return Err(Error::InvalidPayload);
            }

            Address::new(protocol, payload)?
        }
    };
    Ok((addr, network))
}

impl FromStr for Address {
    type Err = Error;
    fn from_str(addr: &str) -> Result<Self, Error> {
        current_network().parse_address(addr)
    }
}

impl Serialize for Address {
    fn serialize<S>(&self, s: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        let address_bytes = self.to_bytes();
        strict_bytes::Serialize::serialize(&address_bytes, s)
    }
}

impl<'de> Deserialize<'de> for Address {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let bz: Cow<'de, [u8]> = strict_bytes::Deserialize::deserialize(deserializer)?;

        // Create and return created address of unmarshalled bytes
        Address::from_bytes(&bz).map_err(de::Error::custom)
    }
}

pub(crate) fn to_leb_bytes(id: u64) -> Vec<u8> {
    // write id to buffer in leb128 format
    unsigned_varint::encode::u64(id, &mut unsigned_varint::encode::u64_buffer()).into()
}

pub(crate) fn from_leb_bytes(bz: &[u8]) -> Result<u64, Error> {
    // write id to buffer in leb128 format
    let (id, remaining) = unsigned_varint::decode::u64(bz)?;
    if !remaining.is_empty() {
        return Err(Error::InvalidPayload);
    }
    Ok(id)
}

#[cfg(test)]
mod tests {
    // Test cases for FOR-02: https://github.com/ChainSafe/forest/issues/1134
    use crate::address::errors::Error;
    use crate::address::{from_leb_bytes, to_leb_bytes};

    #[test]
    fn test_from_leb_bytes_passing() {
        let passing = vec![67];
        assert_eq!(to_leb_bytes(from_leb_bytes(&passing).unwrap()), vec![67]);
    }

    #[test]
    fn test_from_leb_bytes_extra_bytes() {
        let extra_bytes = vec![67, 0, 1, 2];

        match from_leb_bytes(&extra_bytes) {
            Ok(id) => {
                println!(
                    "Successfully decoded bytes when it was not supposed to. Result was: {:?}",
                    &to_leb_bytes(id)
                );
                panic!();
            }
            Err(e) => {
                assert_eq!(e, Error::InvalidPayload);
            }
        }
    }

    #[test]
    fn test_from_leb_bytes_minimal_encoding() {
        let minimal_encoding = vec![67, 0, 130, 0];

        match from_leb_bytes(&minimal_encoding) {
            Ok(id) => {
                println!(
                    "Successfully decoded bytes when it was not supposed to. Result was: {:?}",
                    &to_leb_bytes(id)
                );
                panic!();
            }
            Err(e) => {
                assert_eq!(e, Error::InvalidPayload);
            }
        }
    }
}

/// Returns an address hash for given data
fn address_hash(ingest: &[u8]) -> [u8; 20] {
    let digest = blake2b_simd::Params::new()
        .hash_length(PAYLOAD_HASH_LEN)
        .to_state()
        .update(ingest)
        .finalize();

    let mut hash = [0u8; 20];
    hash.copy_from_slice(digest.as_bytes());
    hash
}
