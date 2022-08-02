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

extension BraveWallet.NetworkInfo: Identifiable {
  public var id: String {
    chainId
  }

  public var nativeToken: BraveWallet.BlockchainToken {
    .init(
      contractAddress: "",
      name: symbolName,
      logo: iconUrls.first ?? "",
      isErc20: false,
      isErc721: false,
      symbol: symbol,
      decimals: decimals,
      visible: false,
      tokenId: "",
      coingeckoId: "",
      chainId: "",
      coin: coin
    )
  }
  
  // Only Eth Mainnet or EVM has eip 1559
  var isEip1559: Bool {
    if coin == .eth, let ethData = data?.ethData {
      return ethData.isEip1559
    }
    return false
  }
}

extension BraveWallet.SignMessageRequest {
  static var previewRequest: BraveWallet.SignMessageRequest {
    .init(
      originInfo: .init(origin: .init(url: URL(string: "https://app.uniswap.org")!), originSpec: "", eTldPlusOne: "uniswap.org"),
      id: 1,
      address: "",
      message: "To avoid digital cat burglars, sign below to authenticate with CryptoKitties.",
      isEip712: false,
      domainHash: "",
      primaryHash: "",
      messageBytes: [],
      coin: .eth
    )
  }
}

extension BraveWallet.BlockchainToken: Identifiable {
  public var id: String {
    symbol.lowercased()
  }

  public func contractAddress(in network: BraveWallet.NetworkInfo) -> String {
    // ETH special swap address
    // Only checking token.symbol with selected network.symbol is sufficient
    // since there is no swap support for custom networks.
    return symbol == network.symbol ? BraveWallet.ethSwapAddress : contractAddress
  }
}

extension BraveWallet {
  /// The address that is expected when you are swapping ETH via SwapService APIs
  public static let ethSwapAddress: String = "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
}

extension BraveWallet.CoinType: Identifiable {
  public var id: Int {
    rawValue
  }
}
