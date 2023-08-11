// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use cid::multihash::Multihash;
use cid::Cid;

pub const FIL_COMMITMENT_SEALED: u64 = 0xf102;
pub const FIL_COMMITMENT_UNSEALED: u64 = 0xf101;

/// Multihash code for Poseidon BLS replica commitments.
pub const POSEIDON_BLS12_381_A1_FC1: u64 = 0xb401;

/// Multihash code for Sha2 256 trunc254 padded used in data commitments.
pub const SHA2_256_TRUNC254_PADDED: u64 = 0x1012;

pub type Commitment = [u8; 32];

/// CommitmentToCID converts a raw commitment hash to a CID
/// by adding:
/// - the given filecoin codec type
/// - the given filecoin e
pub fn commitment_to_cid(mc: u64, mh: u64, commitment: &Commitment) -> Result<Cid, &'static str> {
    validate_filecoin_cid_segments(mc, mh, commitment)?;

    let mh = Multihash::wrap(mh, commitment).map_err(|_| "failed to wrap commitment cid")?;

    Ok(Cid::new_v1(mc, mh))
}

/// CIDToCommitment extracts the raw commitment bytes, the FilMultiCodec and
/// FilMultiHash from a CID, after validating that the codec and hash type are
/// consistent
pub fn cid_to_commitment(c: &Cid) -> Result<(u64, u64, Commitment), &'static str> {
    validate_filecoin_cid_segments(c.codec(), c.hash().code(), c.hash().digest())?;

    let mut comm = Commitment::default();
    comm.copy_from_slice(c.hash().digest());

    Ok((c.codec(), c.hash().code(), comm))
}

/// DataCommitmentV1ToCID converts a raw data commitment to a CID
/// by adding:
/// - codec: cid.FilCommitmentUnsealed
/// - hash type: multihash.Sha2256Truncated256Padded
pub fn data_commitment_v1_to_cid(comm_d: &Commitment) -> Result<Cid, &'static str> {
    commitment_to_cid(FIL_COMMITMENT_UNSEALED, SHA2_256_TRUNC254_PADDED, comm_d)
}

/// cid_to_data_commitment_v1 extracts the raw data commitment from a CID
/// assuming that it has the correct hashing function and
/// serialization types
pub fn cid_to_data_commitment_v1(c: &Cid) -> Result<Commitment, &'static str> {
    let (codec, _, comm_d) = cid_to_commitment(c)?;

    if codec != FIL_COMMITMENT_UNSEALED {
        return Err("data commitment codec must be Unsealed");
    }

    Ok(comm_d)
}

/// ReplicaCommitmentV1ToCID converts a raw data commitment to a CID
/// by adding:
/// - codec: cid.FilCommitmentSealed
/// - hash type: multihash.PoseidonBls12381A1Fc1
pub fn replica_commitment_v1_to_cid(comm_r: &Commitment) -> Result<Cid, &'static str> {
    commitment_to_cid(FIL_COMMITMENT_SEALED, POSEIDON_BLS12_381_A1_FC1, comm_r)
}

/// cid_to_replica_commitment_v1 extracts the raw replica commitment from a CID
/// assuming that it has the correct hashing function and
/// serialization types
pub fn cid_to_replica_commitment_v1(c: &Cid) -> Result<Commitment, &'static str> {
    let (codec, _, comm_r) = cid_to_commitment(c)?;

    if codec != FIL_COMMITMENT_SEALED {
        return Err("data commitment codec must be Sealed");
    }

    Ok(comm_r)
}

/// ValidateFilecoinCidSegments returns an error if the provided CID parts
/// conflict with each other.
fn validate_filecoin_cid_segments(mc: u64, mh: u64, comm_x: &[u8]) -> Result<(), &'static str> {
    match mc {
        FIL_COMMITMENT_UNSEALED => {
            if mh != SHA2_256_TRUNC254_PADDED {
                return Err("Incorrect hash function for unsealed commitment");
            }
        }
        FIL_COMMITMENT_SEALED => {
            if mh != POSEIDON_BLS12_381_A1_FC1 {
                return Err("Incorrect hash function for sealed commitment");
            }
        }
        _ => return Err("Invalid Codec, expected sealed or unsealed commitment codec"),
    }

    if comm_x.len() != 32 {
        Err("commitments must be 32 bytes long")
    } else {
        Ok(())
    }
}

/// piece_commitment_v1_to_cid converts a comm_p to a CID
/// -- it is just a helper function that is equivalent to
/// data_commitment_v1_to_cid.
pub fn piece_commitment_v1_to_cid(comm_p: &Commitment) -> Result<Cid, &'static str> {
    data_commitment_v1_to_cid(comm_p)
}

/// cid_to_piece_commitment_v1 converts a CID to a comm_p
/// -- it is just a helper function that is equivalent to
/// cid_to_data_commitment_v1.
pub fn cid_to_piece_commitment_v1(c: &Cid) -> Result<Commitment, &'static str> {
    cid_to_data_commitment_v1(c)
}
