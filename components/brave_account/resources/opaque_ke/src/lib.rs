/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use argon2::Argon2;
use opaque_ke::ciphersuite::CipherSuite;
use opaque_ke::rand::rngs::OsRng;
use opaque_ke::{
    ClientLogin, ClientLoginFinishParameters, ClientRegistration,
    ClientRegistrationFinishParameters, CredentialResponse, Identifiers, RegistrationResponse,
};
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
    rng: OsRng,
}

#[wasm_bindgen]
impl Registration {
    #[wasm_bindgen(constructor)]
    pub fn new() -> Self {
        Self { state: None, rng: OsRng }
    }

    // result => POST /v2/accounts/password/init
    pub fn start(&mut self, password: &str) -> Result<String, JsValue> {
        let result =
            ClientRegistration::<DefaultCipherSuite>::start(&mut self.rng, &password.as_bytes())
                .map_err(|_| "ClientRegistration::<DefaultCipherSuite>::start() failed!")?;

        self.state = Some(result.state);
        Ok(hex::encode(result.message.serialize()))
    }

    // result => POST /v2/accounts/password/finalize
    pub fn finish(
        &mut self,
        hex_response: &str,
        password: &str,
        email: &str,
    ) -> Result<String, JsValue> {
        let serialized_response =
            hex::decode(&hex_response).map_err(|_| "hex::decode() failed!")?;
        let response = RegistrationResponse::deserialize(&serialized_response)
            .map_err(|_| "RegistrationResponse::deserialize() failed!")?;
        let result = self
            .state
            .take()
            .ok_or("Calling Registration::finish() without first calling Registration::start()!")?
            .finish(
                &mut self.rng,
                password.as_bytes(),
                response,
                ClientRegistrationFinishParameters {
                    identifiers: Identifiers {
                        client: Some(email.as_bytes()),
                        ..Default::default()
                    },
                    ..Default::default()
                },
            )
            .map_err(|_| "ClientRegistration::<DefaultCipherSuite>::finish() failed!")?;

        Ok(hex::encode(result.message.serialize()))
    }
}

#[wasm_bindgen]
pub struct Login {
    state: Option<ClientLogin<DefaultCipherSuite>>,
    rng: OsRng,
}

#[wasm_bindgen]
impl Login {
    #[wasm_bindgen(constructor)]
    pub fn new() -> Self {
        Self { state: None, rng: OsRng }
    }

    // result => POST /v2/auth/login/init
    pub fn start(&mut self, password: &str) -> Result<String, JsValue> {
        let result = ClientLogin::<DefaultCipherSuite>::start(&mut self.rng, &password.as_bytes())
            .map_err(|_| "ClientLogin::<DefaultCipherSuite>::start() failed!")?;

        self.state = Some(result.state);
        Ok(hex::encode(result.message.serialize()))
    }

    // result => POST /v2/auth/login/finalize
    pub fn finish(
        &mut self,
        hex_response: &str,
        password: &str,
        email: &str,
    ) -> Result<String, JsValue> {
        let serialized_response =
            hex::decode(&hex_response).map_err(|_| "hex::decode() failed!")?;
        let response = CredentialResponse::deserialize(&serialized_response)
            .map_err(|_| "CredentialResponse::deserialize() failed!")?;
        let result = self
            .state
            .take()
            .ok_or("Calling Login::finish() without first calling Login::start()!")?
            .finish(
                password.as_bytes(),
                response,
                ClientLoginFinishParameters {
                    identifiers: Identifiers { client: Some(email.as_bytes()), server: None },
                    ..Default::default()
                },
            )
            .map_err(|_| "ClientLogin::<DefaultCipherSuite>::finish() failed!")?;

        Ok(hex::encode(result.message.serialize()))
    }
}
