// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

pub mod post;
mod registered_proof;
mod seal;

use std::fmt;

use fvm_ipld_encoding::repr::*;
use fvm_ipld_encoding::tuple::*;
use num_bigint::BigInt;
use num_derive::FromPrimitive;

pub use self::post::*;
pub use self::registered_proof::*;
pub use self::seal::*;
use crate::ActorID;

/// SectorNumber is a numeric identifier for a sector. It is usually relative to a miner.
pub type SectorNumber = u64;

/// The maximum assignable sector number.
/// Raising this would require modifying our AMT implementation.
pub const MAX_SECTOR_NUMBER: SectorNumber = i64::MAX as u64;

/// Unit of storage power (measured in bytes)
pub type StoragePower = BigInt;

/// The unit of spacetime committed to the network
pub type Spacetime = BigInt;

/// Unit of sector quality
pub type SectorQuality = BigInt;

/// SectorSize indicates one of a set of possible sizes in the network.
#[derive(Clone, Debug, PartialEq, Eq, Copy, FromPrimitive, Serialize_repr, Deserialize_repr)]
#[repr(u64)]
pub enum SectorSize {
    _2KiB = 2 << 10,
    _8MiB = 8 << 20,
    _512MiB = 512 << 20,
    _32GiB = 32 << 30,
    _64GiB = 2 * (32 << 30),
}

impl fmt::Display for SectorSize {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", *self as u64)
    }
}

/// Sector ID which contains the sector number and the actor ID for the miner.
#[derive(Clone, Debug, Default, PartialEq, Eq, Serialize_tuple, Deserialize_tuple)]
pub struct SectorID {
    pub miner: ActorID,
    pub number: SectorNumber,
}
