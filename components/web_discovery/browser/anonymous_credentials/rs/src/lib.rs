/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use anonymous_credentials::{
    CredentialBIG, CredentialManager as InternalCredentialManager, GroupPublicKey, JoinResponse,
    Result, UserCredentials,
};

#[allow(unsafe_op_in_unsafe_fn)]
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
    struct EmptyResult {
        error_message: String,
    }
    extern "Rust" {
        type CredentialManager;
        fn new_credential_manager() -> Box<CredentialManager>;
        fn start_join(&mut self, challenge: &[u8]) -> StartJoinResult;
        fn finish_join(&mut self, public_key: &[u8], gsk: &[u8], join_resp: &[u8]) -> VecU8Result;
        fn set_gsk_and_credentials(&mut self, gsk: &[u8], credentials: &[u8]) -> EmptyResult;
        fn sign(&mut self, msg: &[u8], basename: &[u8]) -> VecU8Result;
    }
}

use ffi::*;

#[allow(dead_code)]
struct CredentialManager(InternalCredentialManager);

fn new_credential_manager() -> Box<CredentialManager> {
    Box::new(CredentialManager(InternalCredentialManager::new()))
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
    fn finish_join(&mut self, public_key: &[u8], gsk: &[u8], join_resp: &[u8]) -> VecU8Result {
        || -> Result<Vec<u8>> {
            let public_key = GroupPublicKey::try_from(public_key)?;
            let gsk = CredentialBIG::try_from(gsk)?;
            let join_resp = JoinResponse::try_from(join_resp)?;
            self.0
                .finish_join(&public_key, &gsk, join_resp)
                .map(|creds| creds.to_bytes().to_vec())
        }()
        .into()
    }

    fn set_gsk_and_credentials(&mut self, gsk: &[u8], credentials: &[u8]) -> EmptyResult {
        || -> Result<()> {
            let gsk = CredentialBIG::try_from(gsk)?;
            let credentials = UserCredentials::try_from(credentials)?;
            self.0.set_gsk_and_credentials(gsk, credentials);
            Ok(())
        }()
        .into()
    }

    fn sign(&mut self, msg: &[u8], basename: &[u8]) -> VecU8Result {
        self.0
            .sign(msg, basename)
            .map(|sig| sig.to_bytes().to_vec())
            .into()
    }
}

impl From<Result<Vec<u8>>> for VecU8Result {
    fn from(value: Result<Vec<u8>>) -> Self {
        match value {
            Ok(data) => VecU8Result {
                data,
                error_message: String::new(),
            },
            Err(e) => VecU8Result {
                data: Vec::new(),
                error_message: e.to_string(),
            },
        }
    }
}

impl From<Result<()>> for EmptyResult {
    fn from(value: Result<()>) -> Self {
        match value {
            Ok(()) => EmptyResult {
                error_message: String::new(),
            },
            Err(e) => EmptyResult {
                error_message: e.to_string(),
            },
        }
    }
}
