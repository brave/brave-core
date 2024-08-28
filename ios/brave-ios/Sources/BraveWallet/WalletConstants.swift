// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import OrderedCollections

public struct WalletConstants {
  /// The Brave swap fee as a % value for 0x DEX aggregator
  ///
  /// This value will be formatted to a string such as 0.875%)
  static let braveSwapFee: Double = 0.00875
  /// The Brave swap fee as a % value for Jupiter DEX aggregator
  ///
  /// This value will be formatted to a string such as 0.85%)
  static let braveSwapJupiterFee: Double = 0.0085

  /// The wei value used for unlimited allowance in an ERC 20 transaction.
  static let maxUInt256 = "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"

  /// The `URLOrigin` used for transactions/requests from Brave Wallet.
  static let braveWalletOrigin: URLOrigin = .init(url: URL(string: "chrome://wallet")!)

  /// The `OriginInfo.originSpec` used for transactions/requests from Brave Wallet.
  static let braveWalletOriginSpec = "chrome://wallet"

  /// The url to Brave Help Center for Wallet.
  static let braveWalletSupportURL = URL(
    string: "https://support.brave.com/hc/en-us/categories/360001059151-Brave-Wallet"
  )!

  // TODO: update wiki link
  /// Brave Wiki page for Solana Name Service (SNS)
  public static let snsBraveWikiURL: URL = URL(
    string: "https://github.com/brave/brave-browser/wiki/Resolve-Methods-for-Solana-Name-Service"
  )!

  /// Terms of Use for Ethereum Name Service (ENS)
  public static let ensTermsOfUseURL: URL = URL(string: "https://consensys.net/terms-of-use/")!

  /// Privacy Policy for Ethereum Name Service (ENS)
  public static let ensPrivacyPolicyURL: URL = URL(string: "https://consensys.net/privacy-policy/")!

  /// The url to learn more about ENS off-chain lookups
  public static let braveWalletENSOffchainURL = URL(
    string: "https://github.com/brave/brave-browser/wiki/ENS-offchain-lookup"
  )!

  /// The url to learn more about Unstoppable Domains resolve methods.
  public static let braveWalletUnstoppableDomainsURL = URL(
    string: "https://github.com/brave/brave-browser/wiki/Resolve-Methods-for-Unstoppable-Domains"
  )!

  /// The url to the privacy policy for 0x swaps
  static let zeroXPrivacyPolicy = URL(string: "https://www.0x.org/privacy")!

  /// The url to the privacy policy for Jupiter swaps
  static let jupiterPrivacyPolicy = URL(string: "https://docs.jup.ag/legal/privacy-policy")!

  /// The url to learn more about NFT Discovery
  public static let nftDiscoveryURL = URL(
    string: "https://github.com/brave/brave-browser/wiki/NFT-Discovery"
  )!

  public static let braveWalletTermsOfUse = URL(string: "https://brave.com/terms-of-use/")!

  /// The currently supported test networks.
  static let supportedTestNetworkChainIds = [
    BraveWallet.SepoliaChainId,
    BraveWallet.LocalhostChainId,
    BraveWallet.SolanaDevnet,
    BraveWallet.SolanaTestnet,
    BraveWallet.FilecoinTestnet,
    BraveWallet.FilecoinEthereumTestnetChainId,
    BraveWallet.BitcoinTestnet,
  ]

  /// Primary network chain ids
  static let primaryNetworkChainIds: [String] = [
    BraveWallet.SolanaMainnet,
    BraveWallet.MainnetChainId,
    BraveWallet.FilecoinMainnet,
    BraveWallet.BitcoinMainnet,
  ]

  public enum SupportedCoinTypesMode {
    case general
    case dapps
  }

  #if DEBUG
  public static var isUnitTesting: Bool {
    ProcessInfo.processInfo.environment["XCTestConfigurationFilePath"] != nil
  }
  #endif

  /// The currently supported coin types in wallet
  public static func supportedCoinTypes(
    _ mode: SupportedCoinTypesMode = .general
  ) -> OrderedSet<BraveWallet.CoinType> {
    switch mode {
    case .general:
      #if DEBUG
      // Only enable .btc for unit tests.
      // Local Debug build need to
      // 1. Remove this check
      // 2. Enable bitcoin feature via build argument
      if isUnitTesting {
        return [.eth, .sol, .fil, .btc]
      }
      #endif
      // Any non-debug build will check bitcoin feature flag from core
      // TF public build can use BraveCore Switches in Browser Settings,
      // Debug section in order to enable Bitcoin.
      if FeatureList.kBraveWalletBitcoinFeature.enabled {
        return [.eth, .sol, .fil, .btc]
      } else {
        return [.eth, .sol, .fil]
      }
    case .dapps:
      return [.eth, .sol]
    }
  }

  /// All of currently supported `OnRampProvider`s.
  /// Use `OnRampProvider.allSupportedOnRampProviders` to get providers available for current device locale.
  static let supportedOnRampProviders: OrderedSet<BraveWallet.OnRampProvider> = [
    .ramp, .sardine, .transak, .stripe, .coinbase,
  ]

  /// The supported Ethereum Name Service (ENS) extensions
  static let supportedENSExtensions = [".eth"]
  /// The supported Solana Name Service (SNS) extensions
  static let supportedSNSExtensions = [".sol"]
  /// The supported Unstoppable Domain (UD) extensions
  public static let supportedUDExtensions = [
    ".crypto", ".x", ".nft", ".dao", ".wallet",
    ".blockchain", ".bitcoin", ".zil", ".altimist", ".anime",
    ".klever", ".manga", ".polygon", ".unstoppable", ".pudgy",
    ".tball", ".stepn", ".secret", ".raiin", ".pog", ".clay",
    ".metropolis", ".witg", ".ubu", ".kryptic", ".farms", ".dfz",
  ]

  /// The supported IPFS schemes
  static let supportedIPFSSchemes = ["ipfs", "ipns"]

  /// The supported send transaction types, used for P3A reporting.
  static let sendTransactionTypes: [BraveWallet.TransactionType] = [
    .ethSend, .erc20Transfer,
    .solanaSystemTransfer, .solanaSplTokenTransfer,
    .solanaSplTokenTransferWithAssociatedTokenAccountCreation,
  ]

  /// The link for users to learn more about Solana SPL token account creation in transaction confirmation screen
  static let splTokenAccountCreationLink = URL(
    string: "https://support.brave.com/hc/en-us/articles/5546517853325"
  )!

  /// The list of token contract addresses that are supported to bridge to Aurora app
  static let supportedAuroraBridgeTokensContractAddresses: [String] = [
    "0x7fc66500c84a76ad7e9c93437bfc5ac33e2ddae9",  // AAVE
    "0xaaaaaa20d9e0e2461697782ef11675f668207961",  // AURORA
    "0xba100000625a3754423978a60c9317c58a424e3d",  // BAL
    "0x0d8775f648430679a709e98d2b0cb6250d2887ef",  // BAT
    "0xc00e94cb662c3520282e6f5717214004a7f26888",  // COMP
    "0x2ba592f78db6436527729929aaf6c908497cb200",  // CREAM
    "0x6b175474e89094c44da98b954eedeac495271d0f",  // DAI
    "0x43dfc4159d86f3a37a5a4b3d4580b888ad7d4ddd",  // DODO
    "0x3ea8ea4237344c9931214796d9417af1a1180770",  // FLX
    "0x853d955acef822db058eb8505911ed77f175b99e",  // FRAX
    "0x3432b6a60d23ca0dfca7761b7ab56459d9c964d0",  // FXS
    "0xd9c2d319cd7e6177336b0a9c93c21cb48d84fb54",  // HAPI
    "0x514910771af9ca656af840dff83e8264ecf986ca",  // LINK
    "0x9f8f72aa9304c8b593d555f12ef6589cc3a579a2",  // MKR
    "0x1117ac6ad6cdf1a3bc543bad3b133724620522d5",  // MODA
    "0xf5cfbc74057c610c8ef151a439252680ac68c6dc",  // OCT
    "0x9aeb50f542050172359a0e1a25a9933bc8c01259",  // OIN
    "0xea7cc765ebc94c4805e3bff28d7e4ae48d06468a",  // PAD
    "0x429881672b9ae42b8eba0e26cd9c73711b891ca5",  // PICKLE
    "0x408e41876cccdc0f92210600ef50372656052a38",  // REN
    "0xc011a73ee8576fb46f5e1c5751ca3b9fe0af2a6f",  // SNX
    "0x6b3595068778dd592e39a122f4f5a5cf09c90fe2",  // SUSHI
    "0x1f9840a85d5af5bf1d1762f925bdaddc4201f984",  // UNI
    "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",  // USDC
    "0xdac17f958d2ee523a2206206994597c13d831ec7",  // USDT
    "0x2260fac5e5542a773aa44fbcfedf7c193bc2c599",  // WBTC
    "0x4691937a7508860f876c9c0a2a617e7d9e945d4b",  // WOO
    "0x0bc529c00c6401aef6d220be8c6ea1667f6ad93e",  // YFI
  ]

  /// The link for users to learn the overview of aurora bridge
  static let auroraBridgeOverviewLink: URL? = URL(
    string: "https://doc.aurora.dev/bridge/bridge-overview/"
  )
  /// The link for users to learn about the risk of using aurora bridge
  static let auroraBridgeRiskLink: URL? = URL(string: "https://rainbowbridge.app/approvals")

  /// The link for users to open Aurora site
  static let auroraBridgeLink: URL? = URL(string: "https://rainbowbridge.app")

  /// The link for for users to learn more about sign transactions
  static let signTransactionRiskLink: URL = URL(
    string: "https://support.brave.com/hc/en-us/articles/4409513799693"
  )!
}
