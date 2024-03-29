// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// https://github.com/zcash/librustzcash/blob/zcash_primitives-0.15.0/zcash_primitives/src/merkle_tree.rs

//! Parsers and serializers for Zcash Merkle trees.

use byteorder::{LittleEndian, ReadBytesExt, WriteBytesExt};
use incrementalmerkletree::{
    frontier::{CommitmentTree, Frontier, NonEmptyFrontier},
    witness::IncrementalWitness,
    Address, /* Hashable, */ Level, MerklePath, Position,
};
use orchard::tree::MerkleHashOrchard;
use std::io::{self, Read, Write};
use crate::librustzcash::encoding::{Optional, Vector};

/// A hashable node within a Merkle tree.
pub trait HashSer {
    /// Parses a node from the given byte source.
    fn read<R: Read>(reader: R) -> io::Result<Self>
    where
        Self: Sized;

    /// Serializes this node.
    fn write<W: Write>(&self, writer: W) -> io::Result<()>;
}

impl HashSer for MerkleHashOrchard {
    fn read<R: Read>(mut reader: R) -> io::Result<Self>
    where
        Self: Sized,
    {
        let mut repr = [0u8; 32];
        reader.read_exact(&mut repr)?;
        <Option<_>>::from(Self::from_bytes(&repr)).ok_or_else(|| {
            io::Error::new(
                io::ErrorKind::InvalidData,
                "Non-canonical encoding of Pallas base field value.",
            )
        })
    }

    fn write<W: Write>(&self, mut writer: W) -> io::Result<()> {
        writer.write_all(&self.to_bytes())
    }
}

/// Writes a usize value encoded as a u64 in little-endian order. Since usize
/// is platform-dependent, we consistently represent it as u64 in serialized
/// formats.
pub fn write_usize_leu64<W: Write>(mut writer: W, value: usize) -> io::Result<()> {
    // Panic if we get a usize value that can't fit into a u64.
    writer.write_u64::<LittleEndian>(value.try_into().unwrap())
}

/// Reads a usize value encoded as a u64 in little-endian order. Since usize
/// is platform-dependent, we consistently represent it as u64 in serialized
/// formats.
pub fn read_leu64_usize<R: Read>(mut reader: R) -> io::Result<usize> {
    reader.read_u64::<LittleEndian>()?.try_into().map_err(|e| {
        io::Error::new(
            io::ErrorKind::InvalidData,
            format!(
                "usize could not be decoded from a 64-bit value on this platform: {:?}",
                e
            ),
        )
    })
}

pub fn write_position<W: Write>(mut writer: W, position: Position) -> io::Result<()> {
    writer.write_u64::<LittleEndian>(position.into())
}

pub fn read_position<R: Read>(mut reader: R) -> io::Result<Position> {
    reader.read_u64::<LittleEndian>().map(Position::from)
}

pub fn write_address<W: Write>(mut writer: W, addr: Address) -> io::Result<()> {
    writer.write_u8(addr.level().into())?;
    writer.write_u64::<LittleEndian>(addr.index())
}

pub fn read_address<R: Read>(mut reader: R) -> io::Result<Address> {
    let level = reader.read_u8().map(Level::from)?;
    let index = reader.read_u64::<LittleEndian>()?;
    Ok(Address::from_parts(level, index))
}

pub fn write_nonempty_frontier_v1<H: HashSer, W: Write>(
    mut writer: W,
    frontier: &NonEmptyFrontier<H>,
) -> io::Result<()> {
    write_position(&mut writer, frontier.position())?;
    if frontier.position().is_right_child() {
        // The v1 serialization wrote the sibling of a right-hand leaf as an optional value, rather
        // than as part of the ommers vector.
        frontier
            .ommers()
            .get(0)
            .expect("ommers vector cannot be empty for right-hand nodes")
            .write(&mut writer)?;
        Optional::write(&mut writer, Some(frontier.leaf()), |w, n: &H| n.write(w))?;
        Vector::write(&mut writer, &frontier.ommers()[1..], |w, e| e.write(w))?;
    } else {
        frontier.leaf().write(&mut writer)?;
        Optional::write(&mut writer, None, |w, n: &H| n.write(w))?;
        Vector::write(&mut writer, frontier.ommers(), |w, e| e.write(w))?;
    }

    Ok(())
}

#[allow(clippy::redundant_closure)]
pub fn read_nonempty_frontier_v1<H: HashSer + Clone, R: Read>(
    mut reader: R,
) -> io::Result<NonEmptyFrontier<H>> {
    let position = read_position(&mut reader)?;
    let left = H::read(&mut reader)?;
    let right = Optional::read(&mut reader, H::read)?;
    let mut ommers = Vector::read(&mut reader, |r| H::read(r))?;

    let leaf = if let Some(right) = right {
        // if the frontier has a right leaf, then the left leaf is the first ommer
        ommers.insert(0, left);
        right
    } else {
        left
    };

    NonEmptyFrontier::from_parts(position, leaf, ommers).map_err(|err| {
        io::Error::new(
            io::ErrorKind::InvalidData,
            format!("Parsing resulted in an invalid Merkle frontier: {:?}", err),
        )
    })
}

pub fn write_frontier_v1<H: HashSer, W: Write>(
    writer: W,
    frontier: &Frontier<H, 32>,
) -> io::Result<()> {
    Optional::write(writer, frontier.value(), write_nonempty_frontier_v1)
}

#[allow(clippy::redundant_closure)]
pub fn read_frontier_v1<H: HashSer + Clone, R: Read>(reader: R) -> io::Result<Frontier<H, 32>> {
    match Optional::read(reader, read_nonempty_frontier_v1)? {
        None => Ok(Frontier::empty()),
        Some(f) => Frontier::try_from(f).map_err(|err| {
            io::Error::new(
                io::ErrorKind::InvalidData,
                format!("Parsing resulted in an invalid Merkle frontier: {:?}", err),
            )
        }),
    }
}

/// Reads a legacy `CommitmentTree` from its serialized form.
pub fn read_commitment_tree<Node: HashSer, R: Read, const DEPTH: u8>(
    mut reader: R,
) -> io::Result<CommitmentTree<Node, DEPTH>> {
    let left = Optional::read(&mut reader, Node::read)?;
    let right = Optional::read(&mut reader, Node::read)?;
    let parents = Vector::read(&mut reader, |r| Optional::read(r, Node::read))?;

    CommitmentTree::from_parts(left, right, parents).map_err(|_| {
        io::Error::new(
            io::ErrorKind::InvalidData,
            "parents vector exceeded tree depth",
        )
    })
}

/// Serializes a legacy `CommitmentTree` as an array of bytes.
pub fn write_commitment_tree<Node: HashSer, W: Write, const DEPTH: u8>(
    tree: &CommitmentTree<Node, DEPTH>,
    mut writer: W,
) -> io::Result<()> {
    Optional::write(&mut writer, tree.left().as_ref(), |w, n| n.write(w))?;
    Optional::write(&mut writer, tree.right().as_ref(), |w, n| n.write(w))?;
    Vector::write(&mut writer, tree.parents(), |w, e| {
        Optional::write(w, e.as_ref(), |w, n| n.write(w))
    })
}

/// Reads an `IncrementalWitness` from its serialized form.
#[allow(clippy::redundant_closure)]
pub fn read_incremental_witness<Node: HashSer, R: Read, const DEPTH: u8>(
    mut reader: R,
) -> io::Result<IncrementalWitness<Node, DEPTH>> {
    let tree = read_commitment_tree(&mut reader)?;
    let filled = Vector::read(&mut reader, |r| Node::read(r))?;
    let cursor = Optional::read(&mut reader, read_commitment_tree)?;

    Ok(IncrementalWitness::from_parts(tree, filled, cursor))
}

/// Serializes an `IncrementalWitness` as an array of bytes.
pub fn write_incremental_witness<Node: HashSer, W: Write, const DEPTH: u8>(
    witness: &IncrementalWitness<Node, DEPTH>,
    mut writer: W,
) -> io::Result<()> {
    write_commitment_tree(witness.tree(), &mut writer)?;
    Vector::write(&mut writer, witness.filled(), |w, n| n.write(w))?;
    Optional::write(&mut writer, witness.cursor().as_ref(), |w, t| {
        write_commitment_tree(t, w)
    })
}

/// Reads a Merkle path from its serialized form.
pub fn merkle_path_from_slice<Node: HashSer, const DEPTH: u8>(
    mut witness: &[u8],
) -> io::Result<MerklePath<Node, DEPTH>> {
    // Skip the first byte, which should be DEPTH to signify the length of
    // the following vector of Pedersen hashes.
    if witness[0] != DEPTH {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            "depth is not as expected",
        ));
    }
    witness = &witness[1..];

    // Begin to construct the authentication path
    let iter = witness.chunks_exact(33);
    witness = iter.remainder();

    // The vector works in reverse
    let auth_path = iter
        .rev()
        .map(|bytes| {
            // Length of inner vector should be the length of a Pedersen hash
            if bytes[0] == 32 {
                // Sibling node should be an element of Fr
                Node::read(&bytes[1..])
            } else {
                Err(io::Error::new(
                    io::ErrorKind::InvalidData,
                    "length of auth path element is not the expected 32 bytes",
                ))
            }
        })
        .collect::<io::Result<Vec<_>>>()?;
    if auth_path.len() != usize::from(DEPTH) {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            format!("length of auth path is not the expected {} elements", DEPTH),
        ));
    }

    // Read the position from the witness
    let position = witness.read_u64::<LittleEndian>().and_then(|p| {
        Position::try_from(p).map_err(|_| {
            io::Error::new(
                io::ErrorKind::InvalidData,
                format!("decoded position {} exceeded the range of a `usize`", p),
            )
        })
    })?;

    // The witness should be empty now; if it wasn't, the caller would
    // have provided more information than they should have, indicating
    // a bug downstream
    if witness.is_empty() {
        let path_len = auth_path.len();
        MerklePath::from_parts(auth_path, position).map_err(|_| {
            io::Error::new(
                io::ErrorKind::InvalidData,
                format!(
                    "auth path expected to contain {} elements, got {}",
                    DEPTH, path_len
                ),
            )
        })
    } else {
        Err(io::Error::new(
            io::ErrorKind::InvalidData,
            "trailing data found in auth path decoding",
        ))
    }
}

