/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use argon2::Argon2;
use opaque_ke::ciphersuite::CipherSuite;
use opaque_ke::{
    ClientLogin,
    ClientLoginFinishParameters,
    ClientRegistration,
    ClientRegistrationFinishParameters,
    CredentialResponse,
    Identifiers,
    RegistrationResponse
};
use rand::rngs::OsRng;
use wasm_bindgen::prelude::*;

pub struct DefaultCipherSuite;
impl CipherSuite for DefaultCipherSuite {
    type OprfCs = opaque_ke::Ristretto255;
    type KeGroup = opaque_ke::Ristretto255;
    type KeyExchange = opaque_ke::key_exchange::tripledh::TripleDh;
    type Ksf = Argon2<'static>;
}

#[wasm_bindgen]
pub struct Registration {
    state: Option<ClientRegistration<DefaultCipherSuite>>,
    rng: OsRng
}

#[wasm_bindgen]
impl Registration {
    #[wasm_bindgen(constructor)]
    pub fn new() -> Registration {
        Registration {
            state: None,
            rng: OsRng
        }
    }

    // result => POST /v2/accounts/password/init
    pub fn start(&mut self, password: &str) -> Result<String, JsValue> {
        match ClientRegistration::<DefaultCipherSuite>::start(
            &mut self.rng, &password.as_bytes()
        ) {
            Ok(result) => {
                self.state = Some(result.state);
                return Ok(hex::encode(result.message.serialize()));
            },
            Err(_error) => return Err(
                "ClientRegistration::<DefaultCipherSuite>::start() failed!".into()
            )
        };
    }

    // result => POST /v2/accounts/password/finalize
    pub fn finish(&mut self, hex_response: &str, password: &str, email: &str) -> Result<String, JsValue> {
        let serialized_response = match hex::decode(&hex_response) {
            Ok(serialized_response) => serialized_response,
            Err(_error) => return Err("hex::decode() failed!".into())
        };

        let response = match RegistrationResponse::deserialize(&serialized_response) {
            Ok(response) => response,
            Err(_error) => return Err("RegistrationResponse::deserialize() failed!".into())
        };

        match self.state.take().unwrap().finish(
            &mut self.rng,
            password.as_bytes(),
            response,
            ClientRegistrationFinishParameters {
                identifiers: Identifiers {
                    client: Some(email.as_bytes()),
                    ..Default::default()
                },
                ..Default::default()
            }
        ) {
            Ok(result) => return Ok(hex::encode(result.message.serialize())),
            Err(_error) => return Err("ClientRegistration::<DefaultCipherSuite>::finish() failed!".into())
        };
    }
}

#[wasm_bindgen]
pub struct Login {
    state: Option<ClientLogin<DefaultCipherSuite>>,
    rng: OsRng
}

#[wasm_bindgen]
impl Login {
    #[wasm_bindgen(constructor)]
    pub fn new() -> Login {
        Login {
            state: None,
            rng: OsRng
        }
    }

    // result => POST /v2/auth/login/init
    pub fn start(&mut self, password: &str) -> Result<String, JsValue> {
        match ClientLogin::<DefaultCipherSuite>::start(
            &mut self.rng, &password.as_bytes()
        ) {
            Ok(result) => {
                self.state = Some(result.state);
                return Ok(hex::encode(result.message.serialize()));
            },
            Err(_error) => return Err(
                "ClientLogin::<DefaultCipherSuite>::start() failed!".into()
            )
        };
    }

    // result => POST /v2/auth/login/finalize
    pub fn finish(&mut self, hex_response: &str, password: &str, email: &str) -> Result<String, JsValue> {
        let serialized_response = match hex::decode(&hex_response) {
            Ok(serialized_response) => serialized_response,
            Err(_error) => return Err("hex::decode() failed!".into())
        };

        let response = match CredentialResponse::deserialize(&serialized_response) {
            Ok(response) => response,
            Err(_error) => return Err("CredentialResponse::deserialize() failed!".into())
        };

        match self.state.take().unwrap().finish(
            password.as_bytes(),
            response,
            ClientLoginFinishParameters {
                identifiers: Identifiers {
                    client: Some(email.as_bytes()),
                    server: None,
                },
                ..Default::default()
            }
        ) {
            Ok(result) => return Ok(hex::encode(result.message.serialize())),
            Err(_error) => return Err("ClientLogin::<DefaultCipherSuite>::finish() failed!".into())
        };
    }
}
