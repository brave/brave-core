// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use cid::Cid;
use fvm_ipld_encoding::Cbor;
use serde_tuple::*;

use super::*;
use crate::randomness::Randomness;
use crate::ActorID;

/// Randomness type used for generating PoSt proof randomness.
pub type PoStRandomness = Randomness;

/// Information about a sector necessary for PoSt verification
#[derive(Debug, PartialEq, Clone, Eq, Serialize_tuple, Deserialize_tuple)]
pub struct SectorInfo {
    /// Used when sealing - needs to be mapped to PoSt registered proof when used to verify a PoSt
    pub proof: RegisteredSealProof,
    pub sector_number: SectorNumber,
    pub sealed_cid: Cid,
}

/// Proof of spacetime data stored on chain.
#[derive(Debug, PartialEq, Clone, Eq, Serialize_tuple, Deserialize_tuple)]
pub struct PoStProof {
    pub post_proof: RegisteredPoStProof,
    #[serde(with = "serde_bytes")]
    pub proof_bytes: Vec<u8>,
}

/// Information needed to verify a Winning PoSt attached to a block header.
/// Note: this is not used within the state machine, but by the consensus/election mechanisms.
#[derive(Debug, PartialEq, Default, Clone, Eq, Serialize_tuple, Deserialize_tuple)]
pub struct WinningPoStVerifyInfo {
    pub randomness: PoStRandomness,
    pub proofs: Vec<PoStProof>,
    pub challenge_sectors: Vec<SectorInfo>,
    /// Used to derive 32-byte prover ID
    pub prover: ActorID,
}

/// Information needed to verify a Window PoSt submitted directly to a miner actor.
#[derive(Debug, PartialEq, Default, Clone, Eq, Serialize_tuple, Deserialize_tuple)]
pub struct WindowPoStVerifyInfo {
    pub randomness: PoStRandomness,
    pub proofs: Vec<PoStProof>,
    pub challenged_sectors: Vec<SectorInfo>,
    pub prover: ActorID,
}

/// Information submitted by a miner to provide a Window PoSt.
#[derive(Debug, PartialEq, Default, Clone, Eq, Serialize_tuple, Deserialize_tuple)]
pub struct OnChainWindowPoStVerifyInfo {
    pub proofs: Vec<PoStProof>,
}

impl Cbor for WindowPoStVerifyInfo {}

impl Cbor for PoStProof {}

impl Cbor for SectorInfo {}
