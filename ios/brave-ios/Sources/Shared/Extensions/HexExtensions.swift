// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

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
