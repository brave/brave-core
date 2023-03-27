// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use cid::Cid;
use clock::ChainEpoch;
use fvm_ipld_encoding::tuple::*;
use fvm_ipld_encoding::{serde_bytes, Cbor};

use crate::randomness::Randomness;
use crate::sector::{
    RegisteredAggregateProof, RegisteredSealProof, RegisteredUpdateProof, SectorID, SectorNumber,
};
use crate::{clock, deal, ActorID};

/// Randomness used for Seal proofs.
pub type SealRandomness = Randomness;

/// Randomness used when verifying a seal proof. This is just a seed value.
pub type InteractiveSealRandomness = Randomness;

/// Information needed to verify a seal proof.
#[derive(Clone, Debug, PartialEq, Serialize_tuple, Deserialize_tuple)]
pub struct SealVerifyInfo {
    pub registered_proof: RegisteredSealProof,
    pub sector_id: SectorID,
    pub deal_ids: Vec<deal::DealID>,
    pub randomness: SealRandomness,
    pub interactive_randomness: InteractiveSealRandomness,
    #[serde(with = "serde_bytes")]
    pub proof: Vec<u8>,
    pub sealed_cid: Cid,   // Commr
    pub unsealed_cid: Cid, // Commd
}

// For syscall marshalling.
impl Cbor for SealVerifyInfo {}

/// SealVerifyParams is the structure of information that must be sent with
/// a message to commit a sector. Most of this information is not needed in the
/// state tree but will be verified in sm.CommitSector. See SealCommitment for
/// data stored on the state tree for each sector.
#[derive(Clone, Debug, PartialEq, Serialize_tuple, Deserialize_tuple)]
pub struct SealVerifyParams {
    pub sealed_cid: Cid,
    pub interactive_epoch: ChainEpoch,
    pub registered_seal_proof: RegisteredSealProof,
    #[serde(with = "serde_bytes")]
    pub proof: Vec<u8>,
    pub deal_ids: Vec<deal::DealID>,
    pub sector_num: SectorNumber,
    pub seal_rand_epoch: ChainEpoch,
}

/// Information needed to verify an aggregated seal proof.
#[derive(Clone, Debug, PartialEq, Serialize_tuple, Deserialize_tuple)]
pub struct AggregateSealVerifyInfo {
    pub sector_number: SectorNumber,
    pub randomness: SealRandomness,
    pub interactive_randomness: InteractiveSealRandomness,

    pub sealed_cid: Cid,   // Commr
    pub unsealed_cid: Cid, // Commd
}

#[derive(Clone, Debug, PartialEq, Serialize_tuple, Deserialize_tuple)]
pub struct AggregateSealVerifyProofAndInfos {
    pub miner: ActorID,
    pub seal_proof: RegisteredSealProof,
    pub aggregate_proof: RegisteredAggregateProof,
    #[serde(with = "serde_bytes")]
    pub proof: Vec<u8>,
    pub infos: Vec<AggregateSealVerifyInfo>,
}

// For syscall marshalling.
impl Cbor for AggregateSealVerifyProofAndInfos {}

/// Information needed to verify a replica update
#[derive(Clone, Debug, PartialEq, Serialize_tuple, Deserialize_tuple)]
pub struct ReplicaUpdateInfo {
    pub update_proof_type: RegisteredUpdateProof,
    pub old_sealed_cid: Cid,
    pub new_sealed_cid: Cid,
    pub new_unsealed_cid: Cid,
    pub proof: Vec<u8>,
}

// For syscall marshalling.
impl Cbor for ReplicaUpdateInfo {}
