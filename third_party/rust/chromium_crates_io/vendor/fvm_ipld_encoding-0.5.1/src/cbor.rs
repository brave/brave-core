// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use std::fmt::{Debug, Formatter};
use std::ops::Deref;
use std::rc::Rc;

use serde::{Deserialize, Serialize};

use super::errors::Error;
use crate::{de, from_slice, ser, strict_bytes, to_vec};

/// Cbor utility functions for serializable objects
#[deprecated(note = "use to_vec or from_slice directly")]
pub trait Cbor: ser::Serialize + de::DeserializeOwned {
    /// Marshalls cbor encodable object into cbor bytes
    fn marshal_cbor(&self) -> Result<Vec<u8>, Error> {
        to_vec(&self)
    }

    /// Unmarshals cbor encoded bytes to object
    fn unmarshal_cbor(bz: &[u8]) -> Result<Self, Error> {
        from_slice(bz)
    }
}

#[allow(deprecated)]
impl<T> Cbor for Vec<T> where T: Cbor {}
#[allow(deprecated)]
impl<T> Cbor for Option<T> where T: Cbor {}

/// Raw serialized cbor bytes.
/// This data is (de)serialized as a byte string.
#[derive(Clone, PartialEq, Serialize, Deserialize, Hash, Eq, Default)]
#[serde(transparent)]
pub struct RawBytes {
    #[serde(with = "strict_bytes")]
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

#[allow(deprecated)]
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

impl Debug for RawBytes {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "RawBytes {{ ")?;
        for byte in &self.bytes {
            write!(f, "{:02x}", byte)?;
        }
        write!(f, " }}")
    }
}

#[cfg(test)]
mod test {
    use crate::RawBytes;

    #[test]
    fn debug_hex() {
        assert_eq!("RawBytes {  }", format!("{:?}", RawBytes::from(vec![])));
        assert_eq!("RawBytes { 00 }", format!("{:?}", RawBytes::from(vec![0])));
        assert_eq!("RawBytes { 0f }", format!("{:?}", RawBytes::from(vec![15])));
        assert_eq!(
            "RawBytes { 00010a10ff }",
            format!("{:?}", RawBytes::from(vec![0, 1, 10, 16, 255]))
        );
    }
}
