//! This module contains traits to have a unified API across codecs.
//!
//! There are two traits defined, [`Codec`] and [`Links`]. Those are separate traits as the `Links`
//! trait is not generic over a certain type.

use cid::Cid;

use std::io::{BufRead, Write};

/// Each IPLD codec implementation should implement this Codec trait. This way codecs can be more
/// easily exchanged or combined.
pub trait Codec<T>: Links {
    /// The multicodec code of the IPLD codec.
    const CODE: u64;
    /// The error that is returned if encoding or decoding fails.
    type Error;

    /// Decode a reader into the desired type.
    fn decode<R: BufRead>(reader: R) -> Result<T, Self::Error>;
    /// Encode a type into a writer.
    fn encode<W: Write>(writer: W, data: &T) -> Result<(), Self::Error>;

    /// Decode a slice into the desired type.
    fn decode_from_slice(bytes: &[u8]) -> Result<T, Self::Error> {
        Self::decode(bytes)
    }

    /// Encode a type into bytes.
    fn encode_to_vec(data: &T) -> Result<Vec<u8>, Self::Error> {
        let mut output = Vec::new();
        Self::encode(&mut output, data)?;
        Ok(output)
    }
}

/// Trait for returning the links of a serialized IPLD data.
pub trait Links {
    /// The error that is returned if the link extraction fails.
    type LinksError;

    /// Return all links (CIDs) that the given encoded data contains.
    fn links(bytes: &[u8]) -> Result<impl Iterator<Item = Cid>, Self::LinksError>;
}
