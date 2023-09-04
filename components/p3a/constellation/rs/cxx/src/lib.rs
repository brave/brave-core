/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use cxx::{CxxString, CxxVector};
use ffi::{
    ByteDataResult, LocalRandomnessDataResult, PPOPRFPublicKeyResult, RandomnessRequestStateResult,
    VecU8,
};
use star_constellation::api::ppoprf::ppoprf::ServerPublicKey;
use star_constellation::api::*;
use star_constellation::randomness::testing::{LocalFetcher, LocalFetcherResponse};
use star_constellation::randomness::RequestState as RandomnessRequestState;

#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = "constellation")]
mod ffi {
    struct RandomnessRequestStateResult {
        state: Box<RandomnessRequestStateWrapper>,
        error: String,
    }

    struct ByteDataResult {
        data: Vec<u8>,
        error: String,
    }

    struct VecU8 {
        data: Vec<u8>,
    }

    struct LocalRandomnessDataResult {
        points: Vec<VecU8>,
        proofs: Vec<VecU8>,
        error: String,
    }

    struct PPOPRFPublicKeyResult {
        key: Box<PPOPRFPublicKeyWrapper>,
        error: String,
    }

    extern "Rust" {
        type RandomnessRequestStateWrapper;
        type PPOPRFPublicKeyWrapper;

        // Loads a serialized randomness server/PPOPRF public key for
        // randomness response authenticity verification
        fn load_ppoprf_public_key(key_data: &[u8]) -> PPOPRFPublicKeyResult;

        // Creates an empty randomness server/PPOPRF public key to disable
        // randomness response verification
        fn get_ppoprf_null_public_key() -> Box<PPOPRFPublicKeyWrapper>;

        // Create Constellation points and proofs for a given set
        // of measurement attributes/layers and an epoch
        fn prepare_measurement(
            layers: &CxxVector<CxxString>,
            epoch: u8,
        ) -> RandomnessRequestStateResult;

        // Construct a randomness request for some given Constellation points (output of prepare_measurement)
        // for the caller to send to the randomness server
        fn construct_randomness_request(rrs: &RandomnessRequestStateWrapper) -> Vec<VecU8>;

        // Construct a final Constellation message for some given Constellation randomness and pre-randomness
        // points (output of prepare_measurement and construct_randomness_request) to send to the collector/aggregator.
        fn construct_message(
            rand_points: &Vec<VecU8>,
            rand_proofs: &Vec<VecU8>,
            rrs: &RandomnessRequestStateWrapper,
            verification_key: &PPOPRFPublicKeyWrapper,
            aux_bytes: &CxxVector<u8>,
            threshold: u32,
        ) -> ByteDataResult;

        // To be used for dev/testing only. Mocks the randomness server by producing a randomness response.
        fn generate_local_randomness(req: &Vec<VecU8>, epoch: u8) -> LocalRandomnessDataResult;
    }
}

pub struct RandomnessRequestStateWrapper(Option<RandomnessRequestState>);
pub struct PPOPRFPublicKeyWrapper(Option<ServerPublicKey>);

pub fn load_ppoprf_public_key(key_data: &[u8]) -> PPOPRFPublicKeyResult {
    match ServerPublicKey::load_from_bincode(key_data) {
        Ok(key) => PPOPRFPublicKeyResult {
            key: Box::new(PPOPRFPublicKeyWrapper(Some(key))),
            error: String::new(),
        },
        Err(e) => PPOPRFPublicKeyResult {
            key: Box::new(PPOPRFPublicKeyWrapper(None)),
            error: e.to_string(),
        },
    }
}

pub fn get_ppoprf_null_public_key() -> Box<PPOPRFPublicKeyWrapper> {
    Box::new(PPOPRFPublicKeyWrapper(None))
}

pub fn prepare_measurement(
    layers: &CxxVector<CxxString>,
    epoch: u8,
) -> RandomnessRequestStateResult {
    let layers: Vec<Vec<u8>> = layers.iter().map(|v| v.as_bytes().to_vec()).collect();
    match client::prepare_measurement(&layers, epoch) {
        Ok(state) => RandomnessRequestStateResult {
            state: Box::new(RandomnessRequestStateWrapper(Some(state))),
            error: String::new(),
        },
        Err(e) => RandomnessRequestStateResult {
            state: Box::new(RandomnessRequestStateWrapper(None)),
            error: e.to_string(),
        },
    }
}

pub fn construct_randomness_request(rrs: &RandomnessRequestStateWrapper) -> Vec<VecU8> {
    match rrs.0.as_ref() {
        Some(rrs) => {
            let res = client::construct_randomness_request(rrs);
            res.into_iter().map(|v| VecU8 { data: v }).collect()
        }
        None => Vec::new(),
    }
}

pub fn construct_message(
    rand_points: &Vec<VecU8>,
    rand_proofs: &Vec<VecU8>,
    rrs: &RandomnessRequestStateWrapper,
    verification_key: &PPOPRFPublicKeyWrapper,
    aux_bytes: &CxxVector<u8>,
    threshold: u32,
) -> ByteDataResult {
    match rrs.0.as_ref() {
        Some(rrs) => {
            let rand_points_vec: Vec<&[u8]> =
                rand_points.iter().map(|v| v.data.as_slice()).collect();
            let rand_proofs_vec: Vec<&[u8]> =
                rand_proofs.iter().map(|v| v.data.as_slice()).collect();
            let res = client::construct_message(
                &rand_points_vec,
                if !rand_proofs.is_empty() { Some(&rand_proofs_vec) } else { None },
                rrs,
                &verification_key.0,
                aux_bytes.as_slice(),
                threshold,
            );
            match res {
                Ok(data) => ByteDataResult { data, error: String::new() },
                Err(e) => ByteDataResult { data: Vec::new(), error: e.to_string() },
            }
        }
        None => ByteDataResult {
            data: Vec::new(),
            error: "Randomness request state is not present".to_string(),
        },
    }
}

pub fn generate_local_randomness(req: &Vec<VecU8>, epoch: u8) -> LocalRandomnessDataResult {
    let random_fetcher = LocalFetcher::new();
    let req_unwrapped: Vec<&[u8]> = req.iter().map(|v| v.data.as_slice()).collect();
    let res = random_fetcher.eval(req_unwrapped.as_slice(), epoch);
    match res {
        Ok(LocalFetcherResponse { serialized_points, serialized_proofs }) => {
            LocalRandomnessDataResult {
                points: serialized_points.into_iter().map(|v| VecU8 { data: v }).collect(),
                proofs: serialized_proofs.into_iter().map(|v| VecU8 { data: v }).collect(),
                error: String::new(),
            }
        }
        Err(e) => LocalRandomnessDataResult {
            points: Vec::new(),
            proofs: Vec::new(),
            error: e.to_string(),
        },
    }
}
