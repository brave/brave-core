// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

extension String {
  /// Truncates an address to only show the first 4 digits and last 4 digits
  var truncatedAddress: String {
    // All addresses should be at least 26 characters long but for the sake of this function, we will ensure
    // its at least the length of the string
    guard !isEmpty else { return self }
    let prefixLength = hasPrefix("0x") ? 6 : 4
    return "\(prefix(prefixLength))…\(suffix(4))"
  }

  /// Truncates an hash to only show the first 6 digits and last 6 digits
  var truncatedHash: String {
    // All addresses should be at least 26 characters long but for the sake of this function, we will ensure
    // its at least the length of the string
    let prefixLength = hasPrefix("0x") ? 8 : 6
    return "\(prefix(prefixLength))…\(suffix(6))"
  }

  /// Removes the `0x` prefix that may exist on the string
  public var removingHexPrefix: String {
    (hasPrefix("0x") || hasPrefix("0X")) ? String(dropFirst(2)) : self
  }

  /// Adds the `0x` prefix that if it does not exist on the string
  var addingHexPrefix: String {
    hasPrefix("0x") ? self : "0x\(self)"
  }

  /// Check if the string is a valid ETH address
  var isETHAddress: Bool {
    // An address has to start with `0x`
    guard starts(with: "0x") else { return false }

    // removing `0x`
    let hex = removingHexPrefix
    // Check the length and the rest of the char is a hex digit
    return hex.count == 40 && hex.allSatisfy(\.isHexDigit)
  }

  /// Check if the string is a valid FIL address
  var isFILAddress: Bool {
    // FIL address has to start with `f` or `t`
    if starts(with: BraveWallet.FilecoinMainnet) || starts(with: BraveWallet.FilecoinTestnet) {
      // secp256k have 41 address length and BLS keys have 86 and FEVM f410 keys have 44
      if count == 41 || count == 86 || count == 44 {
        return true
      }
    }
    return false
  }

  func isBTCAddress(isMainnet: Bool) -> Bool {
    let regex: String
    if isMainnet {
      regex = "^(bc1|[13])[a-zA-HJ-NP-Z0-9]{25,59}"
    } else {
      regex = "^(tb1|[2nm])[a-zA-HJ-NP-Z0-9]{25,59}"
    }
    guard let regex = try? NSRegularExpression(pattern: regex) else {
      return false
    }
    return regex.firstMatch(
      in: self,
      options: [],
      range: NSRange(location: 0, length: self.utf16.count)
    ) != nil
  }

  /// Strip prefix if it exists, ex. 'ethereum:'
  var strippedETHAddress: String {
    guard !isETHAddress else { return self }
    if !starts(with: "0x"),
      contains("0x"),
      let range = range(of: "0x"),
      case let updatedAddressSubstring = self[range.lowerBound...],
      case let updatedAddress = String(updatedAddressSubstring),
      updatedAddress.isETHAddress
    {
      return updatedAddress
    }
    return self
  }

  /// Strip prefix if it exists, ex. 'solana:'
  var strippedSOLAddress: String {
    let prefixesToRemove = ["solana:", "Solana:"]
    for prefix in prefixesToRemove where self.hasPrefix(prefix) {
      return String(self.dropFirst(prefix.count))
    }
    return self
  }

  /// Insert zero-width space every character inside this string
  var zwspOutput: String {
    return map {
      String($0) + "\u{200b}"
    }.joined()
  }
}
