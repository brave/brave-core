// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

extension Data {

  public func getBytes() -> [UInt8] {
    var bytes = [UInt8](repeating: 0, count: self.count)
    self.copyBytes(to: &bytes, count: self.count)
    return bytes
  }
}
