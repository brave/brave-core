// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Preferences

import struct Shared.Strings

extension Preferences {
  public final class Wallet {
    public enum WalletType: Int, Identifiable, CaseIterable {
      case none
      case brave

      public var id: Int {
        rawValue
      }

      public var name: String {
        switch self {
        case .none:
          return Strings.Wallet.walletTypeNone
        case .brave:
          return Strings.Wallet.braveWallet
        }
      }
    }
    /// The default wallet to use for Ethereum to be communicate with web3
    public static let defaultEthWallet = Option<Int>(
      key: "wallet.default-wallet",
      default: WalletType.brave.rawValue
    )
    /// The default wallet to use for Solana to be communicate with web3
    public static let defaultSolWallet = Option<Int>(
      key: "wallet.default-sol-wallet",
      default: WalletType.brave.rawValue
    )
    /// Whether or not webpages can use the Ethereum Provider API to communicate with users Ethereum wallet
    public static let allowEthProviderAccess: Option<Bool> = .init(
      key: "wallet.allow-eth-provider-access",
      default: true
    )
    /// Whether or not webpages can use the Solana Provider API to communicate with users Solana wallet
    public static let allowSolProviderAccess: Option<Bool> = .init(
      key: "wallet.allow-sol-provider-access",
      default: true
    )
    /// The option to display web3 notification
    public static let displayWeb3Notifications = Option<Bool>(
      key: "wallet.display-web3-notifications",
      default: true
    )
    /// The option to determine if we show or hide test networks in network lists
    /// - warning: this preference should not be used other than the migration of itself.
    public static let showTestNetworks = Option<Bool>(
      key: "wallet.show-test-networks",
      default: false
    )
    /// The option for users to turn off aurora popup
    public static let showAuroraPopup = Option<Bool>(key: "wallet.show-aurora-popup", default: true)

    // MARK: Portfolio settings
    public static let isShowingGraph = Option<Bool>(key: "wallet.isShowingGraph", default: true)
    public static let isShowingBalances = Option<Bool>(
      key: "wallet.isShowingBalances",
      default: true
    )
    public static let isShowingNFTsTab = Option<Bool>(key: "wallet.isShowingNFTsTab", default: true)

    // MARK: Portfolio & NFT filters
    public static let groupByFilter = Option<Int>(
      key: "wallet.groupByFilter",
      default: GroupBy.none.rawValue
    )
    public static let sortOrderFilter = Option<Int>(
      key: "wallet.sortOrderFilter",
      default: SortOrder.valueDesc.rawValue
    )
    public static let isHidingSmallBalancesFilter = Option<Bool>(
      key: "wallet.isHidingSmallBalancesFilter",
      default: false
    )
    public static let isHidingUnownedNFTsFilter = Option<Bool>(
      key: "wallet.isHidingUnownedNFTsFilter",
      default: false
    )
    public static let isShowingNFTNetworkLogoFilter = Option<Bool>(
      key: "wallet.isShowingNFTNetworkLogoFilter",
      default: false
    )
    public static let nonSelectedAccountsFilter = Option<[String]>(
      key: "wallet.nonSelectedAccountsFilter",
      default: []
    )
    public static let nonSelectedNetworksFilter = Option<[String]>(
      key: "wallet.nonSelectedNetworksFilter",
      default: WalletConstants.supportedTestNetworkChainIds
    )

    /// Reset Wallet Preferences based on coin type
    public static func reset(for coin: BraveWallet.CoinType) {
      switch coin {
      case .eth:
        Preferences.Wallet.defaultEthWallet.reset()
        Preferences.Wallet.allowEthProviderAccess.reset()
      case .sol:
        Preferences.Wallet.defaultSolWallet.reset()
        Preferences.Wallet.allowSolProviderAccess.reset()
      case .fil, .btc, .zec:
        // not supported
        fallthrough
      @unknown default:
        return
      }
    }

    /// Used to track whether to prompt user to enable NFT discovery
    public static let shouldShowNFTDiscoveryPermissionCallout = Option<Bool>(
      key: "wallet.show-nft-discovery-permission-callout",
      default: true
    )

    /// Used to track whether to migrate user assets stored in BraveCore to CoreData
    static let migrateCoreToWalletUserAssetCompleted = Option<Bool>(
      key: "wallet.core-to-wallet-user-asset",
      default: false
    )

    /// Used to track whether to show user wallet onboarding completed screen after user has created or restore a wallet
    public static let isOnboardingCompleted = Option<Bool>(
      key: "wallet.show-wallet-is-onboarding-completed",
      default: false
    )

    /// Used for Debug section for anyone wants to test with Bitcoin Testnet network or account
    public static let isBitcoinTestnetEnabled = Option<Bool>(
      key: "wallet.is-bitcoin-testnet-enabled",
      default: false
    )

    /// Used to track whether to migrate `account.address` to `account.accountId.uniqueKey` (`account.id`)
    static let migrateCacheKeyCompleted = Option<Bool>(
      key: "wallet.migrate-cache-key-completed",
      default: false
    )

    /// Used to track whether `showTestNetworks` has been read for the last time
    public static let migrateShowTestNetworksCompleted = Option<Bool>(
      key: "wallet.migrate-show-test-networks-completed",
      default: false
    )

    /// Used to track whether to migrate user assets stored in CoreData to BraveCore
    static let migrateWalletUserAssetToCoreCompleted = Option<Bool>(
      key: "wallet.wallet-user-asset-to-core",
      default: false
    )
  }
}

extension BraveWallet.ResolveMethod: Identifiable, CaseIterable {
  public static var allCases: [BraveWallet.ResolveMethod] = [.ask, .enabled, .disabled]

  public var id: Int { rawValue }

  public var name: String {
    switch self {
    case .ask:
      return Strings.Wallet.web3DomainOptionAsk
    case .enabled:
      return Strings.Wallet.web3DomainOptionEnabled
    case .disabled:
      return Strings.Wallet.web3DomainOptionDisabled
    @unknown default:
      return Strings.Wallet.web3DomainOptionAsk
    }
  }
}
