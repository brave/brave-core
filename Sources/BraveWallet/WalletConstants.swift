// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import OrderedCollections

struct WalletConstants {
  /// The Brave swap fee as a % value
  ///
  /// This value will be formatted to a string such as 0.875%)
  static let braveSwapFee: Double = 0.00875

  /// The wei value used for unlimited allowance in an ERC 20 transaction.
  static let MAX_UINT256 = "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
  
  /// The origin used for transactions/requests from Brave Wallet.
  static let braveWalletOrigin: URLOrigin = .init(url: URL(string: "chrome://wallet")!)

  /// The currently supported test networks.
  static let supportedTestNetworkChainIds = [
    BraveWallet.RinkebyChainId,
    BraveWallet.RopstenChainId,
    BraveWallet.GoerliChainId,
    BraveWallet.KovanChainId,
    BraveWallet.LocalhostChainId,
    BraveWallet.SolanaDevnet,
    BraveWallet.SolanaTestnet,
    BraveWallet.FilecoinTestnet
  ]
  
  /// Primary network chain ids
  static let primaryNetworkChainIds: [String] = [
    BraveWallet.SolanaMainnet,
    BraveWallet.MainnetChainId,
    BraveWallet.FilecoinMainnet
  ]
  
  /// The currently supported coin types.
  static var supportedCoinTypes: OrderedSet<BraveWallet.CoinType> {
    return [.eth, .sol]
  }
  
  /// The link for users to learn more about Solana SPL token account creation in transaction confirmation screen
  static let splTokenAccountCreationLink = URL(string: "https://support.brave.com/hc/en-us/articles/5546517853325")!
  
  /// Supported networks for buying via Wyre
  // Not include Polygon Mainnet and Avalanche Mainnet due to core-side issue
  // https://github.com/brave/brave-browser/issues/24444
  // Will bring back via https://github.com/brave/brave-ios/issues/5781
  static let supportedBuyWithWyreNetworkChainIds: [String] = [
    BraveWallet.MainnetChainId
  ]
}

struct WalletDebugFlags {
  static var isSolanaDappsEnabled: Bool = false
}
