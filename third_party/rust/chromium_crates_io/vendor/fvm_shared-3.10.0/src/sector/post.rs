// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use cid::Cid;
use fvm_ipld_encoding::strict_bytes;
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
    #[serde(with = "strict_bytes")]
    pub proof_bytes: Vec<u8>,
}

#[cfg(feature = "arb")]
impl quickcheck::Arbitrary for PoStProof {
    fn arbitrary(g: &mut quickcheck::Gen) -> Self {
        let registered_postproof = g
            .choose(&[
                RegisteredPoStProof::StackedDRGWinning2KiBV1,
                RegisteredPoStProof::StackedDRGWinning8MiBV1,
                RegisteredPoStProof::StackedDRGWinning512MiBV1,
                RegisteredPoStProof::StackedDRGWinning32GiBV1,
                RegisteredPoStProof::StackedDRGWinning64GiBV1,
                RegisteredPoStProof::StackedDRGWindow2KiBV1,
                RegisteredPoStProof::StackedDRGWindow8MiBV1,
                RegisteredPoStProof::StackedDRGWindow512MiBV1,
                RegisteredPoStProof::StackedDRGWindow32GiBV1,
                RegisteredPoStProof::StackedDRGWindow64GiBV1,
                RegisteredPoStProof::StackedDRGWindow2KiBV1P1,
                RegisteredPoStProof::StackedDRGWindow8MiBV1P1,
                RegisteredPoStProof::StackedDRGWindow512MiBV1P1,
                RegisteredPoStProof::StackedDRGWindow32GiBV1P1,
                RegisteredPoStProof::StackedDRGWindow64GiBV1P1,
            ])
            .unwrap();
        PoStProof {
            post_proof: *registered_postproof,
            proof_bytes: Vec::arbitrary(g),
        }
    }
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
