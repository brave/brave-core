/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use wasm_bindgen::prelude::*;

mod opaque_auth {
    use rand::CryptoRng;
    use rand::RngCore;

    use opaque_ke::ciphersuite::CipherSuite;
    use opaque_ke::errors::ProtocolError;
    use opaque_ke::{
        ClientLogin, ClientLoginFinishParameters, ClientLoginFinishResult, ClientLoginStartResult,
        ClientRegistration, ClientRegistrationFinishParameters, ClientRegistrationFinishResult,
        ClientRegistrationStartResult, CredentialResponse, RegistrationResponse,
    };
    pub struct DefaultCipherSuite;

    // The default suite to use
    impl CipherSuite for DefaultCipherSuite {
        type OprfCs = opaque_ke::Ristretto255;
        type KeGroup = opaque_ke::Ristretto255;
        type KeyExchange = opaque_ke::key_exchange::tripledh::TripleDh;
        type Ksf = opaque_ke::ksf::Identity;
    }

    pub fn client_registration_start<R: CryptoRng + RngCore>(
        client_rng: &mut R,
        password: &[u8],
    ) -> Result<ClientRegistrationStartResult<DefaultCipherSuite>, ProtocolError> {
        ClientRegistration::<DefaultCipherSuite>::start(client_rng, password)
    }

    pub fn client_registration_finish<R: CryptoRng + RngCore>(
        client_rng: &mut R,
        password: &[u8],
        registration: ClientRegistrationStartResult<DefaultCipherSuite>,
        response: RegistrationResponse<DefaultCipherSuite>,
        params: ClientRegistrationFinishParameters<DefaultCipherSuite>,
    ) -> Result<ClientRegistrationFinishResult<DefaultCipherSuite>, ProtocolError> {
        registration
            .state
            .finish(client_rng, password, response, params)
    }

    pub fn client_login_start<R: CryptoRng + RngCore>(
        client_rng: &mut R,
        password: &[u8],
    ) -> Result<ClientLoginStartResult<DefaultCipherSuite>, ProtocolError> {
        ClientLogin::<DefaultCipherSuite>::start(client_rng, password)
    }

    pub fn client_login_finish(
        password: &[u8],
        login: ClientLoginStartResult<DefaultCipherSuite>,
        response: CredentialResponse<DefaultCipherSuite>,
        params: ClientLoginFinishParameters<DefaultCipherSuite>,
    ) -> Result<ClientLoginFinishResult<DefaultCipherSuite>, ProtocolError> {
        login.state.finish(password, response, params)
    }
}

pub use opaque_auth::{
    client_login_finish, client_login_start, client_registration_finish, client_registration_start,
    DefaultCipherSuite,
};

#[wasm_bindgen]
pub fn return42() -> i32 {
    return 42;
}
