/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

private let HexDigits: [String] = ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f"]

extension Data {
  public var hexEncodedString: String {
    self.map { String(format: "%02hhx", $0) }.joined()
  }
}

extension Data {
  public var base64EncodedString: String {
    return self.base64EncodedString(options: [])
  }
}
