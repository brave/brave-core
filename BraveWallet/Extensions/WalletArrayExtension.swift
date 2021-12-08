// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

extension Array where Element == NSNumber {
  init?(hexString: String) {
    var value = hexString.removingHexPrefix
    if value.count % 2 != 0 {
      value.insert("0", at: value.startIndex)
    }
    if !value.allSatisfy(\.isHexDigit) {
      return nil
    }
    self.init(
      value.chunks(ofCount: 2)
        .compactMap { UInt8($0, radix: 16) }
        .map(NSNumber.init(value:))
    )
  }
}
