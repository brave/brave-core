// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use cid::Cid;
use fvm_shared::commcid::*;
use multihash_codetable::{Code, Multihash, MultihashDigest};
use rand::{thread_rng, Rng};

fn rand_comm() -> Commitment {
    let mut rng = thread_rng();

    let mut comm = Commitment::default();
    for b in comm.iter_mut() {
        *b = rng.gen();
    }
    comm
}

#[test]
fn comm_d_to_cid() {
    let comm = rand_comm();

    let cid = data_commitment_v1_to_cid(&comm).unwrap();

    assert_eq!(cid.codec(), FIL_COMMITMENT_UNSEALED);
    assert_eq!(cid.hash().code(), SHA2_256_TRUNC254_PADDED);
    assert_eq!(cid.hash().digest(), comm);
}

#[test]
fn cid_to_comm_d() {
    let comm = rand_comm();

    // Correct hash format
    let mh = Multihash::wrap(SHA2_256_TRUNC254_PADDED, &comm).unwrap();
    let c = Cid::new_v1(FIL_COMMITMENT_UNSEALED, mh);
    let decoded = cid_to_data_commitment_v1(&c).unwrap();
    assert_eq!(decoded, comm);

    // Should fail with incorrect codec
    let c = Cid::new_v1(FIL_COMMITMENT_SEALED, mh);
    assert!(cid_to_data_commitment_v1(&c).is_err());

    // Incorrect hash format
    let mh = Code::Blake2b256.digest(&comm);
    let c = Cid::new_v1(FIL_COMMITMENT_UNSEALED, mh);
    assert!(cid_to_data_commitment_v1(&c).is_err());
}

#[test]
fn comm_r_to_cid() {
    let comm = rand_comm();

    let cid = replica_commitment_v1_to_cid(&comm).unwrap();

    assert_eq!(cid.codec(), FIL_COMMITMENT_SEALED);
    assert_eq!(cid.hash().code(), POSEIDON_BLS12_381_A1_FC1);
    assert_eq!(cid.hash().digest(), comm);
}

#[test]
fn cid_to_comm_r() {
    let comm = rand_comm();

    // Correct hash format
    let mh = Multihash::wrap(POSEIDON_BLS12_381_A1_FC1, &comm).unwrap();
    let c = Cid::new_v1(FIL_COMMITMENT_SEALED, mh);
    let decoded = cid_to_replica_commitment_v1(&c).unwrap();
    assert_eq!(decoded, comm);

    // Should fail with incorrect codec
    let c = Cid::new_v1(FIL_COMMITMENT_UNSEALED, mh);
    assert!(cid_to_replica_commitment_v1(&c).is_err());

    // Incorrect hash format
    let mh = Code::Blake2b256.digest(&comm);
    let c = Cid::new_v1(FIL_COMMITMENT_SEALED, mh);
    assert!(cid_to_replica_commitment_v1(&c).is_err());
}

#[test]
fn symmetric_conversion() {
    let comm = rand_comm();

    // data
    let cid = data_commitment_v1_to_cid(&comm).unwrap();
    assert_eq!(
        cid_to_commitment(&cid).unwrap(),
        (FIL_COMMITMENT_UNSEALED, SHA2_256_TRUNC254_PADDED, comm)
    );

    // replica
    let cid = replica_commitment_v1_to_cid(&comm).unwrap();
    assert_eq!(
        cid_to_commitment(&cid).unwrap(),
        (FIL_COMMITMENT_SEALED, POSEIDON_BLS12_381_A1_FC1, comm)
    );

    // piece
    let cid = piece_commitment_v1_to_cid(&comm).unwrap();
    assert_eq!(
        cid_to_commitment(&cid).unwrap(),
        (FIL_COMMITMENT_UNSEALED, SHA2_256_TRUNC254_PADDED, comm)
    );
}
