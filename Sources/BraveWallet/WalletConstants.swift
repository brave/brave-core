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
  
  /// The list of token contract addresses that are supported to bridge to Aurora app
  static let supportedAuroraBridgeTokensContractAddresses: [String] = [
    "0x7fc66500c84a76ad7e9c93437bfc5ac33e2ddae9", // AAVE
    "0xaaaaaa20d9e0e2461697782ef11675f668207961", // AURORA
    "0xba100000625a3754423978a60c9317c58a424e3d", // BAL
    "0x0d8775f648430679a709e98d2b0cb6250d2887ef", // BAT
    "0xc00e94cb662c3520282e6f5717214004a7f26888", // COMP
    "0x2ba592f78db6436527729929aaf6c908497cb200", // CREAM
    "0x6b175474e89094c44da98b954eedeac495271d0f", // DAI
    "0x43dfc4159d86f3a37a5a4b3d4580b888ad7d4ddd", // DODO
    "0x3ea8ea4237344c9931214796d9417af1a1180770", // FLX
    "0x853d955acef822db058eb8505911ed77f175b99e", // FRAX
    "0x3432b6a60d23ca0dfca7761b7ab56459d9c964d0", // FXS
    "0xd9c2d319cd7e6177336b0a9c93c21cb48d84fb54", // HAPI
    "0x514910771af9ca656af840dff83e8264ecf986ca", // LINK
    "0x9f8f72aa9304c8b593d555f12ef6589cc3a579a2", // MKR
    "0x1117ac6ad6cdf1a3bc543bad3b133724620522d5", // MODA
    "0xf5cfbc74057c610c8ef151a439252680ac68c6dc", // OCT
    "0x9aeb50f542050172359a0e1a25a9933bc8c01259", // OIN
    "0xea7cc765ebc94c4805e3bff28d7e4ae48d06468a", // PAD
    "0x429881672b9ae42b8eba0e26cd9c73711b891ca5", // PICKLE
    "0x408e41876cccdc0f92210600ef50372656052a38", // REN
    "0xc011a73ee8576fb46f5e1c5751ca3b9fe0af2a6f", // SNX
    "0x6b3595068778dd592e39a122f4f5a5cf09c90fe2", // SUSHI
    "0x1f9840a85d5af5bf1d1762f925bdaddc4201f984", // UNI
    "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48", // USDC
    "0xdac17f958d2ee523a2206206994597c13d831ec7", // USDT
    "0x2260fac5e5542a773aa44fbcfedf7c193bc2c599", // WBTC
    "0x4691937a7508860f876c9c0a2a617e7d9e945d4b", // WOO
    "0x0bc529c00c6401aef6d220be8c6ea1667f6ad93e"  // YFI
  ]
  
  /// The link for users to learn the overview of aurora bridge
  static let auroraBridgeOverviewLink: URL? = URL(string: "https://doc.aurora.dev/bridge/bridge-overview/")
  /// The link for users to learn about the risk of using aurora bridge
  static let auroraBridgeRiskLink: URL? = URL(string: "https://rainbowbridge.app/approvals")
  
  /// The link for users to open Aurora site
  static let auroraBridgeLink: URL? = URL(string: "https://rainbowbridge.app")
}

struct WalletDebugFlags {
  static var isSolanaDappsEnabled: Bool = false
}
