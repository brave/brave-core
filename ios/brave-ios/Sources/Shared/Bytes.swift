/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

public typealias GUID = String

/**
 * Utilities for futzing with bytes and such.
 */
open class Bytes {
  open class func generateRandomBytes(_ len: UInt) -> Data {
    let len = Int(len)
    var data = Data(count: len)
    data.withUnsafeMutableBytes {
      if let baseAddress = $0.baseAddress,
        SecRandomCopyBytes(kSecRandomDefault, len, baseAddress) != errSecSuccess {
        fatalError("Random byte generation failed.")
      }
    }

    return data
  }

  open class func generateGUID() -> GUID {
    // Turns the standard NSData encoding into the URL-safe variant that Sync expects.
    return generateRandomBytes(9)
      .base64EncodedString(options: [])
      .replacingOccurrences(of: "/", with: "_")
      .replacingOccurrences(of: "+", with: "-")
  }

  open class func decodeBase64(_ b64: String) -> Data? {
    return Data(base64Encoded: b64, options: [])
  }

  /**
     * Turn a string of base64 characters into an NSData *without decoding*.
     * This is to allow HMAC to be computed of the raw base64 string.
     */
  open class func dataFromBase64(_ b64: String) -> Data? {
    return b64.data(using: .ascii, allowLossyConversion: false)
  }

  func fromHex(_ str: String) -> Data {
    // TODO
    return Data()
  }
}
