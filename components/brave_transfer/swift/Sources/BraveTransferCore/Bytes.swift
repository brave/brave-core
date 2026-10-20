// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

public typealias Bytes = [UInt8]

extension Array where Element == UInt8 {
  public init?(hexString: String) {
    let digits = Array(hexString.utf8)
    guard digits.count % 2 == 0 else { return nil }
    var out = Bytes()
    out.reserveCapacity(digits.count / 2)
    var i = 0
    while i < digits.count {
      guard let high = hexValue(digits[i]), let low = hexValue(digits[i + 1]) else {
        return nil
      }
      out.append(high << 4 | low)
      i += 2
    }
    self = out
  }

  public var hexString: String {
    let digits = Array("0123456789abcdef".utf8)
    var out = Bytes()
    out.reserveCapacity(count * 2)
    for byte in self {
      out.append(digits[Int(byte >> 4)])
      out.append(digits[Int(byte & 0x0f)])
    }
    return String(decoding: out, as: UTF8.self)
  }
}

private func hexValue(_ c: UInt8) -> UInt8? {
  switch c {
  case UInt8(ascii: "0")...UInt8(ascii: "9"): return c - UInt8(ascii: "0")
  case UInt8(ascii: "a")...UInt8(ascii: "f"): return c - UInt8(ascii: "a") + 10
  case UInt8(ascii: "A")...UInt8(ascii: "F"): return c - UInt8(ascii: "A") + 10
  default: return nil
  }
}

/// Decodes UTF-8, returning nil for anything that is not well-formed
/// (including encoded surrogates and overlong forms).
func decodeUTF8Strictly(_ bytes: Bytes) -> String? {
  let string = String(decoding: bytes, as: UTF8.self)
  return string.utf8.elementsEqual(bytes) ? string : nil
}
