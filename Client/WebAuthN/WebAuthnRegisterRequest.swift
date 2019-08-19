// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

struct PublicKeyCredentialDescriptor: Decodable {
    let type: String
    let id: String
}

struct WebAuthnRegisterRequest: Decodable {
    struct PublicKey: Decodable {
        struct PubKeyCredParams: Decodable {
            var alg: Int
            var type: String
        }
        
        struct User: Decodable {
            var displayName: String
            var name: String
            var id: String
        }
        
        struct Rp: Decodable {
            var id: String?
            var name: String
        }
        
        // This struct itself is optional
        // If present the two keys may or may not be present
        struct AuthenticatorSelection: Decodable {
            var requireResidentKey: Bool?
        }
        
        let authenticatorSelection: AuthenticatorSelection?
        let pubKeyCredParams: [PubKeyCredParams]
        let user: User
        let rp: Rp
        let challenge: String
        let excludeCredentials: [PublicKeyCredentialDescriptor]?
    }
    let publicKey: PublicKey
}
