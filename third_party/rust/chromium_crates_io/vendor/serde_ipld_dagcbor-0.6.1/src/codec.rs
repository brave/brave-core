//! Implementation of ipld-core's `Codec` trait.

use std::io::{BufRead, Write};

use ipld_core::{
    cid::Cid,
    codec::{Codec, Links},
    serde::ExtractLinks,
};
use serde::{de::Deserialize, ser::Serialize};

use crate::{de::Deserializer, error::CodecError};

/// DAG-CBOR implementation of ipld-core's `Codec` trait.
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub struct DagCborCodec;

impl<T> Codec<T> for DagCborCodec
where
    T: for<'a> Deserialize<'a> + Serialize,
{
    const CODE: u64 = 0x71;
    type Error = CodecError;

    fn decode<R: BufRead>(reader: R) -> Result<T, Self::Error> {
        Ok(crate::from_reader(reader)?)
    }

    fn encode<W: Write>(writer: W, data: &T) -> Result<(), Self::Error> {
        Ok(crate::to_writer(writer, data)?)
    }
}

impl Links for DagCborCodec {
    type LinksError = CodecError;

    fn links(data: &[u8]) -> Result<impl Iterator<Item = Cid>, Self::LinksError> {
        let mut deserializer = Deserializer::from_slice(data);
        Ok(ExtractLinks::deserialize(&mut deserializer)?
            .into_vec()
            .into_iter())
    }
}
