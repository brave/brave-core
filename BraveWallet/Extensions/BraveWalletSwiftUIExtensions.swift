// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import BraveCore

extension BraveWallet.AccountInfo: Identifiable {
  public var id: String {
    address
  }
  public var isPrimary: Bool {
    !isImported
  }
}

extension BraveWallet.TransactionInfo: Identifiable {
  // Already has `id` property
}

extension BraveWallet.EthereumChain: Identifiable {
  public var id: String {
    chainId
  }
}

extension BraveWallet.ERCToken: Identifiable {
  public var id: String {
    isETH ? "eth" : contractAddress
  }
  /// Whether or not this ERCToken is actually ETH
  public var isETH: Bool {
    contractAddress.isEmpty && symbol.lowercased() == "eth"
  }
}

extension BraveWallet {
  /// The address that is expected when you are swapping ETH via SwapController APIs
  public static let ethSwapAddress: String = "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
}
