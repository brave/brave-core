// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

enum NamedAddresses {
  /// Obtain a name for a given address and the users set of accounts. If no name exists for the
  /// specific address a truncated version of the address is returned.
  ///
  /// Certain addresses can also have names such as the Swap exchange proxy address.
  /// See ``defaultNamedAddresses`` for a list of named addresses
  static func name(
    for address: String,
    accounts: [BraveWallet.AccountInfo]
  ) -> String {
    var names = accounts.reduce(
      into: [String: String](),
      { $0[$1.address.lowercased()] = $1.name }
    )
    names.merge(Self.defaultNamedAddresses, uniquingKeysWith: { key, _ in key })
    return names[address.lowercased()] ?? address.truncatedAddress
  }
  /// The address for the 0x exchange proxy used for swapping currency
  static let swapExchangeProxyAddress = "0xdef1c0ded9bec7f1a1670819833240f027b25eff"

  static private let defaultNamedAddresses = [
    swapExchangeProxyAddress: "0x Exchange Proxy"
  ]
}
