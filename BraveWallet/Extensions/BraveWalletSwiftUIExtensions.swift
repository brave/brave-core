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
  
  public var nativeToken: BraveWallet.BlockchainToken {
    .init(contractAddress: "",
          name: symbolName,
          logo: iconUrls.first ?? "",
          isErc20: false,
          isErc721: false,
          symbol: symbol,
          decimals: decimals,
          visible: false,
          tokenId: "",
          coingeckoId: ""
    )
  }
  
  public var isCustom: Bool {
    let ethNetworks = [BraveWallet.MainnetChainId,
                       BraveWallet.RinkebyChainId,
                       BraveWallet.RopstenChainId,
                       BraveWallet.GoerliChainId,
                       BraveWallet.KovanChainId,
                       BraveWallet.LocalhostChainId]
    return !ethNetworks.contains(id)
  }
}

extension BraveWallet.BlockchainToken: Identifiable {
  public var id: String {
    symbol.lowercased()
  }
  
  public func contractAddress(in network: BraveWallet.EthereumChain) -> String {
    if network.chainId == BraveWallet.RopstenChainId {
      switch symbol.uppercased() {
      case "ETH": return BraveWallet.ethSwapAddress
      case "DAI" : return BraveWallet.daiSwapAddress
      case "USDC": return BraveWallet.usdcSwapAddress
      default: return contractAddress
      }
    } else {
      // ETH special swap address in Ropsten network
      // Only checking token.symbol with selected network.symbol is sufficient
      // since there is no swap support for custom networks.
      return symbol == network.symbol ? BraveWallet.ethSwapAddress : contractAddress
    }
  }
}

extension BraveWallet {
  /// The address that is expected when you are swapping ETH via SwapService APIs
  public static let ethSwapAddress: String = "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
  
  ///  The address that is expected when you are swapping DAI via SwapService APIs
  ///  Also the contract address to fetch DAI balance on Ropsten
  public static let daiSwapAddress: String = "0xad6d458402f60fd3bd25163575031acdce07538d"
  
  ///  The address that is expected when you are swapping USDC via SwapService APIs
  ///  Also the contract address to fetch USDC balance on Ropsten
  public static let usdcSwapAddress: String = "0x07865c6e87b9f70255377e024ace6630c1eaa37f"
  
  /// A list of supported assets' symbols for swapping in `Ropsten` network
  public static let assetsSwapInRopsten: [String] = ["DAI", "USDC"]
}
