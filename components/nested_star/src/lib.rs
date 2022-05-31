/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

use cxx::{CxxString, CxxVector};
use ffi::{ByteDataResult, PPOPRFPublicKeyResult, RandomnessRequestStateResult};
use nested_sta_rs::api::ppoprf::ppoprf::ServerPublicKey;
use nested_sta_rs::api::*;
use nested_sta_rs::errors::NestedSTARError;
use nested_sta_rs::randomness::{testing::LocalFetcher, RequestState as RandomnessRequestState};

#[cxx::bridge(namespace = "nested_star")]
mod ffi {
    struct RandomnessRequestStateResult {
        state: Box<RandomnessRequestStateWrapper>,
        error: String,
    }

    struct ByteDataResult {
        data: Vec<u8>,
        error: String,
    }

    struct PPOPRFPublicKeyResult {
        key: Box<PPOPRFPublicKeyWrapper>,
        error: String,
    }

    extern "Rust" {
        type RandomnessRequestStateWrapper;
        type PPOPRFPublicKeyWrapper;

        fn load_ppoprf_public_key(key_data: &[u8]) -> PPOPRFPublicKeyResult;

        fn get_ppoprf_null_public_key() -> Box<PPOPRFPublicKeyWrapper>;

        fn prepare_measurement(
            layers: &CxxVector<CxxString>,
            epoch: u8,
        ) -> RandomnessRequestStateResult;
        fn construct_randomness_request(rrs: &RandomnessRequestStateWrapper) -> ByteDataResult;

        fn construct_message(
            randomness_response: &CxxVector<u8>,
            rrs: &RandomnessRequestStateWrapper,
            verification_key: &PPOPRFPublicKeyWrapper,
            aux_bytes: &CxxVector<u8>,
            threshold: u32,
        ) -> ByteDataResult;

        // To be used for dev/testing only
        fn generate_local_randomness(req: &Vec<u8>) -> ByteDataResult;
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

fn create_byte_data_result(res: Result<Vec<u8>, NestedSTARError>) -> ByteDataResult {
    match res {
        Ok(data) => ByteDataResult { data, error: String::new() },
        Err(e) => ByteDataResult { data: Vec::new(), error: e.to_string() },
    }
}

pub fn construct_randomness_request(rrs: &RandomnessRequestStateWrapper) -> ByteDataResult {
    let res = client::construct_randomness_request(&rrs.0.as_ref().unwrap());
    create_byte_data_result(res)
}

pub fn construct_message(
    randomness_response: &CxxVector<u8>,
    rrs: &RandomnessRequestStateWrapper,
    verification_key: &PPOPRFPublicKeyWrapper,
    aux_bytes: &CxxVector<u8>,
    threshold: u32,
) -> ByteDataResult {
    let res = client::construct_message(
        randomness_response.as_slice(),
        rrs.0.as_ref().unwrap(),
        &verification_key.0,
        aux_bytes.as_slice(),
        threshold,
    );
    create_byte_data_result(res)
}

pub fn generate_local_randomness(req: &Vec<u8>) -> ByteDataResult {
    let random_fetcher = LocalFetcher::new();
    let res = random_fetcher.eval(req);
    create_byte_data_result(res)
}
