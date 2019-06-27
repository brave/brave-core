// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Shared

enum  WebAuthnClientDataType: String {
    case create = "webauthn.create"
    case get = "webauthn.get"
}

struct WebAuthnClientData {
    var type: String
    var challenge: String
    var origin: String
    
    enum CodingKeys: String, CodingKey {
        case type
        case challenge
        case origin
    }
}

extension WebAuthnClientData: Encodable {
    func encode(to encoder: Encoder) throws {
        var container = encoder.container(keyedBy: CodingKeys.self)
        try container.encode(type, forKey: .type)
        try container.encode(origin, forKey: .origin)
        
        let challengeData = Data(base64Encoded: challenge)
        let websafeChallenge = challengeData?.websafeBase64String()
        try container.encode(websafeChallenge, forKey: .challenge)
    }
}

func clientDataHash (data: Data) -> Data? {
    var hash = [UInt8](repeating: 0, count: Int(CC_SHA256_DIGEST_LENGTH))
    data.withUnsafeBytes {
        _ = CC_SHA256($0.baseAddress, CC_LONG(data.count), &hash)
    }
    return Data(hash)
}
