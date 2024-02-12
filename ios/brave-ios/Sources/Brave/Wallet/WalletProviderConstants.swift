// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

struct Web3ProviderEvent {
  var name: String
  var arguments: Any?
  
  init(_ name: String, arguments: Any? = nil) {
    self.name = name
    self.arguments = arguments
  }
}

extension Web3ProviderEvent {
  // MARK: - Common
  
  static let connect: Self = .init("connect")
  static let disconnect: Self = .init("disconnect")
  
  // MARK: - Ethereum

  static func ethereumChainChanged(chainId: String) -> Self {
    .init("chainChanged", arguments: chainId)
  }
  static func ethereumAccountsChanged(accounts: [String]) -> Self {
    .init("accountsChanged", arguments: [accounts.first].compactMap { $0 })
  }
}
