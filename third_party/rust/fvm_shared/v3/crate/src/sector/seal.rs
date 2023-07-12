// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use cid::Cid;
use clock::ChainEpoch;
use fvm_ipld_encoding::strict_bytes;
use fvm_ipld_encoding::tuple::*;

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
#[derive(Clone, Debug, PartialEq, Eq, Serialize_tuple, Deserialize_tuple)]
pub struct SealVerifyInfo {
    pub registered_proof: RegisteredSealProof,
    pub sector_id: SectorID,
    pub deal_ids: Vec<deal::DealID>,
    pub randomness: SealRandomness,
    pub interactive_randomness: InteractiveSealRandomness,
    #[serde(with = "strict_bytes")]
    pub proof: Vec<u8>,
    pub sealed_cid: Cid,   // Commr
    pub unsealed_cid: Cid, // Commd
}

/// SealVerifyParams is the structure of information that must be sent with
/// a message to commit a sector. Most of this information is not needed in the
/// state tree but will be verified in sm.CommitSector. See SealCommitment for
/// data stored on the state tree for each sector.
#[derive(Clone, Debug, PartialEq, Eq, Serialize_tuple, Deserialize_tuple)]
pub struct SealVerifyParams {
    pub sealed_cid: Cid,
    pub interactive_epoch: ChainEpoch,
    pub registered_seal_proof: RegisteredSealProof,
    #[serde(with = "strict_bytes")]
    pub proof: Vec<u8>,
    pub deal_ids: Vec<deal::DealID>,
    pub sector_num: SectorNumber,
    pub seal_rand_epoch: ChainEpoch,
}

/// Information needed to verify an aggregated seal proof.
#[derive(Clone, Debug, PartialEq, Eq, Serialize_tuple, Deserialize_tuple)]
pub struct AggregateSealVerifyInfo {
    pub sector_number: SectorNumber,
    pub randomness: SealRandomness,
    pub interactive_randomness: InteractiveSealRandomness,

    pub sealed_cid: Cid,   // Commr
    pub unsealed_cid: Cid, // Commd
}

#[derive(Clone, Debug, PartialEq, Eq, Serialize_tuple, Deserialize_tuple)]
pub struct AggregateSealVerifyProofAndInfos {
    pub miner: ActorID,
    pub seal_proof: RegisteredSealProof,
    pub aggregate_proof: RegisteredAggregateProof,
    #[serde(with = "strict_bytes")]
    pub proof: Vec<u8>,
    pub infos: Vec<AggregateSealVerifyInfo>,
}

/// Information needed to verify a replica update
#[derive(Clone, Debug, PartialEq, Eq, Serialize_tuple, Deserialize_tuple)]
pub struct ReplicaUpdateInfo {
    pub update_proof_type: RegisteredUpdateProof,
    pub old_sealed_cid: Cid,
    pub new_sealed_cid: Cid,
    pub new_unsealed_cid: Cid,
    pub proof: Vec<u8>,
}
