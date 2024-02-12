/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

public extension JSONSerialization {

  class func jsObject(withNative native: Any?, escaped: Bool = false) -> String? {

    guard let native = native, let data = try? JSONSerialization.data(withJSONObject: native, options: JSONSerialization.WritingOptions(rawValue: 0)) else {
      return nil
    }

    // Convert to string of JSON data, encode " for JSON to JS conversion
    var encoded = String(data: data, encoding: String.Encoding.utf8)

    if escaped {
      encoded = encoded?.replacingOccurrences(of: "\"", with: "\\\"")
    }

    encoded = encoded?.replacingOccurrences(of: "\"null\"", with: "null")

    return encoded
  }
}
