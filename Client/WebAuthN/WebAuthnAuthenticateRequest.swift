// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
import Shared

struct WebAuthnAuthenticateRequest {
    var rpID: String?
    var challenge: String
    var allowCredentials: [String] = []
    var userPresence: Bool

    enum RequestKeys: String, CodingKey {
        case publicKey
    }
    
    enum PublicKeyDictionaryKeys: String, CodingKey {
        case rpId
        case challenge
        case allowCredentials
        case authenticatorSelection
        case userVerification
    }
}

private struct AllowCredentials: Codable {
    var id: String
    var type: String
}

extension WebAuthnAuthenticateRequest: Decodable {
    init(from decoder: Decoder) throws {
        let request = try decoder.container(keyedBy: RequestKeys.self)
        let publicKeyDictionary = try request.nestedContainer(keyedBy: PublicKeyDictionaryKeys.self, forKey: .publicKey)
        
        rpID = try publicKeyDictionary.decodeIfPresent(String.self, forKey: .rpId)
        challenge = try publicKeyDictionary.decode(String.self, forKey: .challenge)
        
        // userPresence is the inverse of userVerification, UP by default is true
        let userVerifcationString = try publicKeyDictionary.decodeIfPresent(String.self, forKey: .userVerification) ?? "discouraged"
        userPresence = userVerifcationString == "discouraged"
        
        let allowCredentialsArray = try publicKeyDictionary.decode([AllowCredentials].self, forKey: .allowCredentials)
    
        for credential in allowCredentialsArray {
            let publicKey = credential.id
            allowCredentials.append(publicKey)
        }
    }
}
