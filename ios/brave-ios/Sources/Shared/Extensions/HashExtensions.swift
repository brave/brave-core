/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import CommonCrypto

extension Data {
  public var sha1: Data {
    let len = Int(CC_SHA1_DIGEST_LENGTH)
    let digest = UnsafeMutablePointer<UInt8>.allocate(capacity: len)
    CC_SHA1((self as NSData).bytes, CC_LONG(self.count), digest)
    return Data(bytes: UnsafePointer<UInt8>(digest), count: len)
  }

  public var sha256: Data {
    let len = Int(CC_SHA256_DIGEST_LENGTH)
    let digest = UnsafeMutablePointer<UInt8>.allocate(capacity: len)
    CC_SHA256((self as NSData).bytes, CC_LONG(self.count), digest)
    return Data(bytes: UnsafePointer<UInt8>(digest), count: len)
  }
}

public extension String {
  var fnv1a: Int {
    let basis: UInt64 = 14695981039346656037
    let prime: UInt64 = 1099511628211
    
    var hash: UInt64 = basis
    
    for c in self.utf8 {
      hash ^= UInt64(c)
      hash &*= prime
    }
    
    // Prevent overflow crash
    return Int(hash & UInt64(Int.max))
  }
}
