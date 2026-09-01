// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import OrderedCollections

public struct WalletConstants {
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
    string: "https://support.brave.app/hc/categories/360001062531-Wallet"
  )!

  // TODO: update wiki link
  /// Brave Wiki page for Solana Name Service (SNS)
  public static let snsBraveWikiURL: URL = URL(
    string: "https://github.com/brave/brave-browser/wiki/Resolve-Methods-for-Solana-Name-Service"
  )!

  /// Terms of Use for Ethereum Name Service (ENS)
  public static let ensTermsOfUseURL: URL = URL(string: "https://chainstack.com/tos/")!

  /// Privacy Policy for Ethereum Name Service (ENS)
  public static let ensPrivacyPolicyURL: URL = URL(string: "https://chainstack.com/privacy/")!

  /// The url to learn more about ENS off-chain lookups
  public static let braveWalletENSOffchainURL = URL(
    string: "https://github.com/brave/brave-browser/wiki/ENS-offchain-lookup"
  )!

  /// The url to learn more about Unstoppable Domains resolve methods.
  public static let braveWalletUnstoppableDomainsURL = URL(
    string: "https://github.com/brave/brave-browser/wiki/Web3-Top-Level-Domains"
  )!

  /// The url to learn more about NFT Discovery
  public static let nftDiscoveryURL = URL(
    string: "https://github.com/brave/brave-browser/wiki/NFT-Discovery"
  )!

  public static let braveWalletTermsOfUse = URL(string: "https://brave.com/terms-of-use/")!

  /// The currently supported test networks.
  static let supportedTestNetworkChainIds = [
    BraveWallet.SepoliaChainId,
    BraveWallet.SolanaDevnet,
    BraveWallet.SolanaTestnet,
    BraveWallet.FilecoinTestnet,
    BraveWallet.FilecoinEthereumTestnetChainId,
    BraveWallet.BitcoinTestnet,
    BraveWallet.ZCashTestnet,
    BraveWallet.CardanoTestnet,
  ]

  /// Primary network chain ids
  static let primaryNetworkChainIds: [String] = [
    BraveWallet.SolanaMainnet,
    BraveWallet.MainnetChainId,
    BraveWallet.FilecoinMainnet,
    BraveWallet.BitcoinMainnet,
    BraveWallet.ZCashMainnet,
    BraveWallet.CardanoMainnet,
  ]

  /// Chain ids that are required and cannot be de-selected during onboarding.
  static let mandatoryNetworkChainIds: [String] = [
    BraveWallet.SolanaMainnet,
    BraveWallet.MainnetChainId,
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

  /// Returns true if all three Cardano dApp support feature flags are enabled:
  /// - kBraveWalletCardanoEnabled (Cardano support)
  /// - kBraveWalletCardanoDAppSupportIOS (Cardano dApp support)
  public static var isCardanoDAppSupportEnabled: Bool {
    FeatureList.kBraveWalletCardanoEnabled?.enabled == true
      && FeatureList.kBraveWalletCardanoDAppSupportIOS?.enabled == true
  }

  /// The currently supported coin types in wallet
  public static func supportedCoinTypes(
    _ mode: SupportedCoinTypesMode = .general
  ) -> OrderedSet<BraveWallet.CoinType> {
    var result = OrderedSet<BraveWallet.CoinType>()
    switch mode {
    case .general:
      #if DEBUG
      // Only enable .btc and .zec for unit tests.
      // Local Debug build need to
      // 1. Remove this check
      // 2. Enable bitcoin feature via build argument
      if isUnitTesting {
        return [.eth, .sol, .fil, .btc, .zec]
      }
      #endif
      // Any non-debug build will check bitcoin & zcash feature flag from core
      // TF public build can use BraveCore Switches in Browser Settings,
      // Debug section in order to enable Bitcoin.
      result = [.eth, .sol, .fil]
      if FeatureList.kBraveWalletBitcoinFeature?.enabled == true {
        result.append(.btc)
      }
      if FeatureList.kBraveWalletZCashFeature?.enabled == true {
        result.append(.zec)
      }
      if FeatureList.kBraveWalletCardanoEnabled?.enabled == true {
        result.append(.ada)
      }
    case .dapps:
      return isCardanoDAppSupportEnabled ? [.eth, .sol, .ada] : [.eth, .sol]
    }
    return result
  }

  /// The supported Ethereum Name Service (ENS) extensions
  static let supportedENSExtensions = [".eth"]
  /// The supported Solana Name Service (SNS) extensions
  static let supportedSNSExtensions = [".sol"]
  /// The supported Unstoppable Domain (UD) extensions
  public static let supportedUDExtensions = [
    ".agent",
    ".ai4",
    ".altimist",
    ".amped",
    ".anime",
    ".anyone",
    ".arculus",
    ".ask",
    ".ath",
    ".austin",
    ".bald",
    ".basenji",
    ".bay",
    ".bch",
    ".benji",
    ".binanceus",
    ".bitcoin",
    ".bitget",
    ".bitscrunch",
    ".blockchain",
    ".boomer",
    ".brave",
    ".bunni",
    ".calicoin",
    ".carbon",
    ".caw",
    ".cgai",
    ".chip",
    ".chomp",
    ".clay",
    ".collect",
    ".crypto",
    ".dao",
    ".dejay",
    ".depin",
    ".derad",
    ".dfz",
    ".digibyte",
    ".doga",
    ".donut",
    ".dream",
    ".dsci",
    ".emir",
    ".ethermail",
    ".farms",
    ".goblin",
    ".gotchi",
    ".grow",
    ".her",
    ".hub",
    ".imtoken",
    ".kingdom",
    ".klever",
    ".kresus",
    ".kryptic",
    ".learn",
    ".lfg",
    ".ltc",
    ".lunar",
    ".manga",
    ".marketer",
    ".metropolis",
    ".miku",
    ".ministry",
    ".mobix",
    ".moon",
    ".mooncat",
    ".mumu",
    ".mycircle",
    ".nft",
    ".nibi",
    ".npc",
    ".ohm",
    ".onchain",
    ".pack",
    ".pastor",
    ".pbdx",
    ".pendle",
    ".pilot",
    ".podcast",
    ".pog",
    ".pokt",
    ".polygon",
    ".presearch",
    ".privacy",
    ".propykeys",
    ".pudgy",
    ".pundi",
    ".quantum",
    ".rad",
    ".raiin",
    ".secret",
    ".smobler",
    ".south",
    ".spend",
    ".stepn",
    ".supernova",
    ".tball",
    ".tea",
    ".tribe",
    ".twin",
    ".u",
    ".ubu",
    ".unstoppable",
    ".wallet",
    ".web3",
    ".wifi",
    ".witg",
    ".wrkx",
    ".x",
    ".xec",
    ".xmr",
    ".xyo",
    ".zano",
    ".zil",
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
    string: "https://support.brave.app/hc/en-us/articles/5546517853325"
  )!

  /// The link for for users to learn more about sign transactions
  static let signTransactionRiskLink: URL = URL(
    string: "https://support.brave.app/hc/en-us/articles/4409513799693"
  )!

  /// Solana Transacation Instruction Type Name
  static let solanaTxInstructionTypeNameAssign: String = "Assign"

  /// Solana Transacation Instruction Type Name
  static let solanaTxInstructionTypeNameAssignWithSeed: String = "AssignWithSeed"
}
