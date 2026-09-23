// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

//! Partial implementation of Direct Anonymous Attestation (DAA) for the Web
//! Discovery Project. Only signer functions are available. Performs the same elliptic curve operations as the [original C library](https://github.com/whotracksme/anonymous-credentials).
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

#[cxx::bridge(namespace = "web_discovery")]
mod ffi {
    struct JoinInitialization {
        gsk: Vec<u8>,
        join_request: Vec<u8>,
    }

    extern "Rust" {
        type AnonymousCredentialsManager;
        type GroupPublicKey;
        type CredentialBIG;
        type JoinResponse;
        type UserCredentials;

        type VecU8Result;
        type GroupPublicKeyResult;
        type CredentialBIGResult;
        type JoinResponseResult;
        type UserCredentialsResult;

        fn error_message(self: &VecU8Result) -> String;
        fn is_ok(self: &VecU8Result) -> bool;
        fn unwrap(self: &mut VecU8Result) -> Vec<u8>;

        fn error_message(self: &GroupPublicKeyResult) -> String;
        fn is_ok(self: &GroupPublicKeyResult) -> bool;
        fn unwrap(self: &mut GroupPublicKeyResult) -> Box<GroupPublicKey>;

        fn error_message(self: &CredentialBIGResult) -> String;
        fn is_ok(self: &CredentialBIGResult) -> bool;
        fn unwrap(self: &mut CredentialBIGResult) -> Box<CredentialBIG>;

        fn error_message(self: &JoinResponseResult) -> String;
        fn is_ok(self: &JoinResponseResult) -> bool;
        fn unwrap(self: &mut JoinResponseResult) -> Box<JoinResponse>;

        fn error_message(self: &UserCredentialsResult) -> String;
        fn is_ok(self: &UserCredentialsResult) -> bool;
        fn unwrap(self: &mut UserCredentialsResult) -> Box<UserCredentials>;

        fn load_group_public_key(data: &[u8]) -> Box<GroupPublicKeyResult>;
        fn load_credential_big(data: &[u8]) -> Box<CredentialBIGResult>;
        fn load_join_response(data: &[u8]) -> Box<JoinResponseResult>;
        fn load_user_credentials(data: &[u8]) -> Box<UserCredentialsResult>;

        fn new_anonymous_credentials_manager() -> Box<AnonymousCredentialsManager>;
        fn new_anonymous_credentials_with_fixed_seed() -> Box<AnonymousCredentialsManager>;
        fn start_join(
            self: &mut AnonymousCredentialsManager,
            challenge: &[u8],
        ) -> JoinInitialization;
        fn finish_join(
            self: &mut AnonymousCredentialsManager,
            public_key: &GroupPublicKey,
            gsk: &CredentialBIG,
            join_resp: Box<JoinResponse>,
        ) -> Box<VecU8Result>;
        fn set_gsk_and_credentials(
            self: &mut AnonymousCredentialsManager,
            gsk: Box<CredentialBIG>,
            credentials: Box<UserCredentials>,
        );
        fn sign(
            self: &mut AnonymousCredentialsManager,
            msg: &[u8],
            basename: &[u8],
        ) -> Box<VecU8Result>;
    }
}

use ffi::*;

macro_rules! result_wrapper {
    ($inner_type:ident, $result_type:ident) => {
        struct $result_type(Option<Result<$inner_type>>);

        impl $result_type {
            fn error_message(self: &$result_type) -> String {
                self.0
                    .as_ref()
                    .and_then(|r| r.as_ref().err())
                    .map(|e| e.to_string())
                    .unwrap_or_default()
            }

            fn is_ok(self: &$result_type) -> bool {
                self.0.as_ref().map(|r| r.as_ref().is_ok()).unwrap_or_default()
            }
        }

        impl From<Result<$inner_type>> for $result_type {
            fn from(value: Result<$inner_type>) -> Self {
                Self(Some(value))
            }
        }
    };
}

macro_rules! result_wrapper_box_unwrap {
    ($inner_type:ident, $result_type:ident) => {
        impl $result_type {
            fn unwrap(self: &mut $result_type) -> Box<$inner_type> {
                Box::new(self.0.take().unwrap().unwrap())
            }
        }
    };
}

macro_rules! wrapper_with_loader {
    ($internal_type:ident, $wrapped_type:ident, $result_type:ident, $loader_func_name:ident) => {
        #[derive(Default)]
        struct $wrapped_type($internal_type);

        fn $loader_func_name(data: &[u8]) -> Box<$result_type> {
            Box::new($internal_type::try_from(data).map(|v| $wrapped_type(v)).into())
        }
    };
}

type VecU8 = Vec<u8>;

result_wrapper!(GroupPublicKey, GroupPublicKeyResult);
result_wrapper!(CredentialBIG, CredentialBIGResult);
result_wrapper!(JoinResponse, JoinResponseResult);
result_wrapper!(UserCredentials, UserCredentialsResult);
result_wrapper!(VecU8, VecU8Result);

result_wrapper_box_unwrap!(GroupPublicKey, GroupPublicKeyResult);
result_wrapper_box_unwrap!(CredentialBIG, CredentialBIGResult);
result_wrapper_box_unwrap!(JoinResponse, JoinResponseResult);
result_wrapper_box_unwrap!(UserCredentials, UserCredentialsResult);

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

impl VecU8Result {
    fn unwrap(&mut self) -> Vec<u8> {
        self.0.take().unwrap().unwrap()
    }
}

#[allow(dead_code)]
struct AnonymousCredentialsManager(InternalCredentialManager);

fn new_anonymous_credentials_manager() -> Box<AnonymousCredentialsManager> {
    Box::new(AnonymousCredentialsManager(InternalCredentialManager::new()))
}

fn new_anonymous_credentials_with_fixed_seed() -> Box<AnonymousCredentialsManager> {
    Box::new(AnonymousCredentialsManager(InternalCredentialManager::new_with_seed(&[0u8; 1])))
}

impl AnonymousCredentialsManager {
    fn start_join(&mut self, challenge: &[u8]) -> JoinInitialization {
        let result = self.0.start_join(challenge);
        JoinInitialization {
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
    ) -> Box<VecU8Result> {
        Box::new(
            || -> Result<Vec<u8>> {
                self.0
                    .finish_join(&public_key.0, &gsk.0, join_resp.0)
                    .map(|creds| creds.to_bytes().to_vec())
            }()
            .into(),
        )
    }

    fn set_gsk_and_credentials(
        &mut self,
        gsk: Box<CredentialBIG>,
        credentials: Box<UserCredentials>,
    ) {
        self.0.set_gsk_and_credentials(gsk.0, credentials.0);
    }

    fn sign(&mut self, msg: &[u8], basename: &[u8]) -> Box<VecU8Result> {
        Box::new(self.0.sign(msg, basename).map(|sig| sig.to_bytes().to_vec()).into())
    }
}
