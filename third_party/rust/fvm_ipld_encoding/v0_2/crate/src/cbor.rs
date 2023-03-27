// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use std::ops::Deref;
use std::rc::Rc;

use cid::{multihash, Cid};
use serde::{Deserialize, Serialize};

use super::errors::Error;
use crate::{de, from_slice, ser, to_vec, DAG_CBOR};

/// Cbor utility functions for serializable objects
pub trait Cbor: ser::Serialize + de::DeserializeOwned {
    /// Marshalls cbor encodable object into cbor bytes
    fn marshal_cbor(&self) -> Result<Vec<u8>, Error> {
        to_vec(&self)
    }

    /// Unmarshals cbor encoded bytes to object
    fn unmarshal_cbor(bz: &[u8]) -> Result<Self, Error> {
        from_slice(bz)
    }

    /// Returns the content identifier of the raw block of data
    /// Default is Blake2b256 hash
    fn cid(&self) -> Result<Cid, Error> {
        use multihash::MultihashDigest;
        const DIGEST_SIZE: u32 = 32; // TODO get from the multihash?
        let data = &self.marshal_cbor()?;
        let hash = multihash::Code::Blake2b256.digest(data);
        debug_assert_eq!(
            u32::from(hash.size()),
            DIGEST_SIZE,
            "expected 32byte digest"
        );
        Ok(Cid::new_v1(DAG_CBOR, hash))
    }
}

impl<T> Cbor for Vec<T> where T: Cbor {}
impl<T> Cbor for Option<T> where T: Cbor {}

/// Raw serialized cbor bytes.
/// This data is (de)serialized as a byte string.
#[derive(Clone, PartialEq, Debug, Serialize, Deserialize, Hash, Eq, Default)]
#[serde(transparent)]
pub struct RawBytes {
    #[serde(with = "serde_bytes")]
    bytes: Vec<u8>,
}

impl From<RawBytes> for Vec<u8> {
    fn from(b: RawBytes) -> Vec<u8> {
        b.bytes
    }
}

impl From<Vec<u8>> for RawBytes {
    fn from(v: Vec<u8>) -> RawBytes {
        RawBytes::new(v)
    }
}

impl From<RawBytes> for Rc<[u8]> {
    fn from(b: RawBytes) -> Rc<[u8]> {
        b.bytes.into()
    }
}

impl Cbor for RawBytes {}

impl Deref for RawBytes {
    type Target = Vec<u8>;
    fn deref(&self) -> &Self::Target {
        &self.bytes
    }
}

impl RawBytes {
    /// Constructor if data is encoded already
    pub fn new(bytes: Vec<u8>) -> Self {
        Self { bytes }
    }

    /// Contructor for encoding Cbor encodable structure.
    pub fn serialize<O: Serialize>(obj: O) -> Result<Self, Error> {
        Ok(Self {
            bytes: to_vec(&obj)?,
        })
    }

    /// Returns serialized bytes.
    pub fn bytes(&self) -> &[u8] {
        &self.bytes
    }

    /// Deserializes the serialized bytes into a defined type.
    pub fn deserialize<O: de::DeserializeOwned>(&self) -> Result<O, Error> {
        from_slice(&self.bytes)
    }
}
