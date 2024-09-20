// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

mod bytes;
mod cbor;
mod cbor_store;
mod errors;
pub mod ipld_block;
mod raw;
mod vec;
use std::io;

pub use serde::{self, de, ser};

pub use self::bytes::*;
pub use self::cbor::*;
pub use self::cbor_store::CborStore;
pub use self::errors::*;
pub use self::vec::*;

/// CBOR should be used to pass CBOR data when internal links don't need to be
/// traversable/reachable. When a CBOR block is loaded, said links will not be added to the
/// reachable set.
pub const CBOR: u64 = 0x51;
/// DagCBOR should be used for all IPLD-CBOR data where CIDs need to be traversable.
pub const DAG_CBOR: u64 = 0x71;
/// RAW should be used for raw data.
pub const IPLD_RAW: u64 = 0x55;

// TODO: these really don't work all that well in a shared context like this as anyone importing
// them also need to _explicitly_ import the serde_tuple & serde_repr crates. These are _macros_,
// not normal items.

pub mod tuple {
    pub use serde_tuple::{self, Deserialize_tuple, Serialize_tuple};
}

pub mod repr {
    pub use serde_repr::{Deserialize_repr, Serialize_repr};
}

/// Serializes a value to a vector.
pub fn to_vec<T>(value: &T) -> Result<Vec<u8>, Error>
where
    T: ser::Serialize + ?Sized,
{
    serde_ipld_dagcbor::to_vec(value).map_err(Into::into)
}

/// Decode a value from CBOR from the given reader.
pub fn from_reader<T, R>(reader: R) -> Result<T, Error>
where
    T: de::DeserializeOwned,
    R: io::BufRead,
{
    serde_ipld_dagcbor::from_reader(reader).map_err(Into::into)
}

/// Decode a value from CBOR from the given slice.
pub fn from_slice<'a, T>(slice: &'a [u8]) -> Result<T, Error>
where
    T: de::Deserialize<'a>,
{
    serde_ipld_dagcbor::from_slice(slice).map_err(Into::into)
}

/// Encode a value as CBOR to the given writer.
pub fn to_writer<W, T>(mut writer: W, value: &T) -> Result<(), Error>
where
    W: io::Write,
    T: ser::Serialize,
{
    serde_ipld_dagcbor::to_writer(&mut writer, value).map_err(Into::into)
}
