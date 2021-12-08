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
    symbol.lowercased()
  }
  /// Whether or not this ERCToken is actually ETH
  public var isETH: Bool {
    contractAddress.isEmpty && symbol.lowercased() == "eth"
  }
  
  public func swapAddress(in chainId: String) -> String {
    if chainId == BraveWallet.RopstenChainId {
      switch symbol.uppercased() {
      case "ETH": return BraveWallet.ethSwapAddress
      case "DAI" : return BraveWallet.daiSwapAddress
      case "USDC": return BraveWallet.usdcSwapAddress
      default: return contractAddress
      }
    } else {
      return isETH ? BraveWallet.ethSwapAddress : contractAddress
    }
  }
}

extension BraveWallet {
  /// The address that is expected when you are swapping ETH via SwapController APIs
  public static let ethSwapAddress: String = "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
  
  ///  The address that is expected when you are swapping DAI via SwapController APIs
  public static let daiSwapAddress: String = "0xad6d458402f60fd3bd25163575031acdce07538d"
  
  ///  The address that is expected when you are swapping USDC via SwapController APIs
  public static let usdcSwapAddress: String = "0x07865c6e87b9f70255377e024ace6630c1eaa37f"
  
  /// A list of supported assets' symbols for swapping in `Ropsten` network
  public static let assetsSwapInRopsten: [String] = ["DAI", "USDC"]
}
