/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

extension String {
  /// Truncates an address to only show the first 4 digits and last 4 digits
  var truncatedAddress: String {
    // All addresses should be at least 26 characters long but for the sake of this function, we will ensure
    // its at least the length of the string
    let prefixLength = hasPrefix("0x") ? 6 : 4
    return "\(prefix(prefixLength))…\(suffix(4))"
  }
  /// Removes the `0x` prefix that may exist on the string
  var removingHexPrefix: String {
    hasPrefix("0x") ? String(dropFirst(2)) : self
  }
  
  /// Check if the string is a valid address
  var isAddress: Bool {
    // An address has to start with `0x`
    guard starts(with: "0x") else { return false }
    // Check the rest of the char is a hex digit
    return dropFirst(2).allSatisfy(\.isHexDigit)
  }
}
