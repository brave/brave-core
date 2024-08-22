// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

//! Partial implementation of Direct Anonymous Attestation (DAA) for the Web Discovery Project.
//! Only signer functions are available. Performs the same elliptic curve operations as the [original C library](https://github.com/whotracksme/anonymous-credentials).
//!
//! bn254 is the only supported curve for this library.

mod data;
mod join;
mod manager;
mod sign;
mod util;

use crate::data::{
    BIG_SIZE, ECP2_COMPAT_SIZE, ECP_PROOF_SIZE, ECP_SIZE, GROUP_PUBLIC_KEY_SIZE,
    JOIN_RESPONSE_SIZE, USER_CREDENTIALS_SIZE,
};
use thiserror::Error;

use crate::data::{
    CredentialBIG as InternalCredentialBIG, GroupPublicKey as InternalGroupPublicKey,
    JoinResponse as InternalJoinResponse, UserCredentials as InternalUserCredentials,
};
use crate::manager::CredentialManager as InternalCredentialManager;

#[derive(Error, Debug)]
pub enum CredentialError {
    #[error("ECP should be {0} bytes", ECP_SIZE)]
    BadECP,
    #[error("ECP2 should be {0} bytes", ECP2_COMPAT_SIZE)]
    BadECP2,
    #[error("BIG should be {0} bytes", BIG_SIZE)]
    BadBIG,
    #[error("ECP proof should be {0} bytes", ECP_PROOF_SIZE)]
    BadECPProof,
    #[error("User credentials should be {0} bytes", USER_CREDENTIALS_SIZE)]
    BadUserCredentials,
    #[error("Join response should be {0} bytes", JOIN_RESPONSE_SIZE)]
    BadJoinResponse,
    #[error("Group public key should be {0} bytes", GROUP_PUBLIC_KEY_SIZE)]
    GroupPublicKeyLength,
    #[error("Join response validation failed")]
    JoinResponseValidation,
    #[error("Private key and/or credentials not set")]
    CredentialsNotSet,
    #[error("Group public key verification failed")]
    BadGroupPublicKey,
}

pub type Result<T> = std::result::Result<T, CredentialError>;

#[cxx::bridge(namespace = "anonymous_credentials")]
mod ffi {
    struct StartJoinResult {
        gsk: Vec<u8>,
        join_request: Vec<u8>,
    }
    struct VecU8Result {
        data: Vec<u8>,
        error_message: String,
    }
    struct GroupPublicKeyResult {
        value: Box<GroupPublicKey>,
        error_message: String,
    }
    struct CredentialBIGResult {
        value: Box<CredentialBIG>,
        error_message: String,
    }
    struct JoinResponseResult {
        value: Box<JoinResponse>,
        error_message: String,
    }
    struct UserCredentialsResult {
        value: Box<UserCredentials>,
        error_message: String,
    }

    extern "Rust" {
        type CredentialManager;
        type GroupPublicKey;
        type CredentialBIG;
        type JoinResponse;
        type UserCredentials;

        fn load_group_public_key(data: &[u8]) -> GroupPublicKeyResult;
        fn load_credential_big(data: &[u8]) -> CredentialBIGResult;
        fn load_join_response(data: &[u8]) -> JoinResponseResult;
        fn load_user_credentials(data: &[u8]) -> UserCredentialsResult;

        fn new_credential_manager() -> Box<CredentialManager>;
        fn new_credential_manager_with_fixed_seed() -> Box<CredentialManager>;
        fn start_join(self: &mut CredentialManager, challenge: &[u8]) -> StartJoinResult;
        fn finish_join(
            self: &mut CredentialManager,
            public_key: &GroupPublicKey,
            gsk: &CredentialBIG,
            join_resp: Box<JoinResponse>,
        ) -> VecU8Result;
        fn set_gsk_and_credentials(
            self: &mut CredentialManager,
            gsk: Box<CredentialBIG>,
            credentials: Box<UserCredentials>,
        );
        fn sign(self: &mut CredentialManager, msg: &[u8], basename: &[u8]) -> VecU8Result;
    }
}

use ffi::*;

macro_rules! wrapper_with_loader {
    ($internal_type:ident, $wrapped_type:ident, $result_type:ident, $loader_func_name:ident) => {
        #[derive(Default)]
        struct $wrapped_type($internal_type);

        fn $loader_func_name(data: &[u8]) -> $result_type {
            match $internal_type::try_from(data) {
                Ok(value) => $result_type {
                    value: Box::new($wrapped_type(value)),
                    error_message: String::new(),
                },
                Err(e) => $result_type {
                    value: Box::new(Default::default()),
                    error_message: e.to_string(),
                },
            }
        }
    };
}

wrapper_with_loader!(
    InternalGroupPublicKey,
    GroupPublicKey,
    GroupPublicKeyResult,
    load_group_public_key
);
wrapper_with_loader!(
    InternalCredentialBIG,
    CredentialBIG,
    CredentialBIGResult,
    load_credential_big
);
wrapper_with_loader!(InternalJoinResponse, JoinResponse, JoinResponseResult, load_join_response);
wrapper_with_loader!(
    InternalUserCredentials,
    UserCredentials,
    UserCredentialsResult,
    load_user_credentials
);

#[allow(dead_code)]
struct CredentialManager(InternalCredentialManager);

fn new_credential_manager() -> Box<CredentialManager> {
    Box::new(CredentialManager(InternalCredentialManager::new()))
}

fn new_credential_manager_with_fixed_seed() -> Box<CredentialManager> {
    Box::new(CredentialManager(InternalCredentialManager::new_with_seed(&[0u8; 1])))
}

impl CredentialManager {
    fn start_join(&mut self, challenge: &[u8]) -> StartJoinResult {
        let result = self.0.start_join(challenge);
        StartJoinResult {
            gsk: result.gsk.to_bytes().to_vec(),
            join_request: result.join_msg.to_bytes().to_vec(),
        }
    }

    /// Processes response and returns user credentials
    fn finish_join(
        &mut self,
        public_key: &GroupPublicKey,
        gsk: &CredentialBIG,
        join_resp: Box<JoinResponse>,
    ) -> VecU8Result {
        || -> Result<Vec<u8>> {
            self.0
                .finish_join(&public_key.0, &gsk.0, join_resp.0)
                .map(|creds| creds.to_bytes().to_vec())
        }()
        .into()
    }

    fn set_gsk_and_credentials(
        &mut self,
        gsk: Box<CredentialBIG>,
        credentials: Box<UserCredentials>,
    ) {
        self.0.set_gsk_and_credentials(gsk.0, credentials.0);
    }

    fn sign(&mut self, msg: &[u8], basename: &[u8]) -> VecU8Result {
        self.0.sign(msg, basename).map(|sig| sig.to_bytes().to_vec()).into()
    }
}

impl From<Result<Vec<u8>>> for VecU8Result {
    fn from(value: Result<Vec<u8>>) -> Self {
        match value {
            Ok(data) => VecU8Result { data, error_message: String::new() },
            Err(e) => VecU8Result { data: Vec::new(), error_message: e.to_string() },
        }
    }
}
