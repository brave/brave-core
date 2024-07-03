// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import Foundation
import Preferences
import Strings
import SwiftUI

/// An interface that helps you interact with a json-rpc service
///
/// This wraps a JsonRpcService that you would obtain through BraveCore and makes it observable
public class NetworkStore: ObservableObject, WalletObserverStore {

  enum SetSelectedChainError: Error {
    case selectedChainHasNoAccounts
    case chainAlreadySelected
    case unknown
  }

  @Published private(set) var allChains: [BraveWallet.NetworkInfo] = []

  @Published private(set) var defaultSelectedChainId: String = BraveWallet.MainnetChainId
  var defaultSelectedChain: BraveWallet.NetworkInfo {
    allChains.first(where: { $0.chainId == self.defaultSelectedChainId }) ?? .init()
  }

  /// Selected chain id for the current `origin`. If `origin` is nil, this will be the default chain id.
  @Published private(set) var selectedChainIdForOrigin: String = BraveWallet.MainnetChainId
  var selectedChainForOrigin: BraveWallet.NetworkInfo {
    allChains.first(where: { $0.chainId == self.selectedChainIdForOrigin }) ?? .init()
  }

  /// If Swap is supported for the current `defaultSelectedChain`.
  @Published private(set) var isSwapSupported: Bool = true

  /// The origin of the active tab (if applicable). Used for fetching/selecting network for the DApp origin.
  public var origin: URLOrigin? {
    didSet {
      guard origin != oldValue else { return }
      Task {
        await updateSelectedChain()
      }
    }
  }

  var visibleChains: [BraveWallet.NetworkInfo] {
    allChains.filter { chain in
      !chain.props.isHidden
    }
  }

  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let swapService: BraveWalletSwapService
  private let assetManager: WalletUserAssetManagerType
  private var rpcServiceObserver: JsonRpcServiceObserver?
  private var keyringServiceObserver: KeyringServiceObserver?

  private weak var networkSelectionStore: NetworkSelectionStore?

  var isObserving: Bool {
    rpcServiceObserver != nil && keyringServiceObserver != nil
  }

  public init(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    swapService: BraveWalletSwapService,
    userAssetManager: WalletUserAssetManagerType,
    origin: URLOrigin? = nil
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.swapService = swapService
    self.assetManager = userAssetManager
    self.origin = origin

    self.setupObservers()
    Task { @MainActor in
      if !Preferences.Wallet.migrateShowTestNetworksCompleted.value {
        // since test networks are default hidden in core,
        // if user previously set preference to show test networks
        // we will need to remove them from hidden list in core
        if Preferences.Wallet.showTestNetworks.value {
          await self.unhideAllSupportedTestnets()
          await self.updateChainList()
        }
        Preferences.Wallet.migrateShowTestNetworksCompleted.value = true
      }
    }
    Preferences.Wallet.isBitcoinTestnetEnabled.observe(from: self)
  }

  func tearDown() {
    rpcServiceObserver = nil
    keyringServiceObserver = nil

    networkSelectionStore?.tearDown()
  }

  func setupObservers() {
    guard !isObserving else { return }
    self.rpcServiceObserver = JsonRpcServiceObserver(
      rpcService: rpcService,
      _chainChangedEvent: { [weak self] chainId, coin, origin in
        Task { @MainActor [self] in
          // Verify correct account is selected for the new network.
          // This could occur from Eth Switch Chain request when Solana account selected.
          let accountId = await self?.walletService.ensureSelectedAccountForChain(
            coin: coin,
            chainId: chainId
          )
          // Sync our local properties with updated values
          if let origin, origin == self?.origin {
            self?.selectedChainIdForOrigin = chainId
          } else if origin == nil {
            self?.defaultSelectedChainId = chainId
            self?.isSwapSupported =
              await self?.swapService.isSwapSupported(chainId: chainId) ?? false
            if let origin = self?.origin {
              // The default network may be used for this origin if no
              // other network was assigned for this origin. If so, we
              // need to make sure the `selectedChainIdForOrigin` is updated
              // to reflect the correct network.
              if let network = await self?.rpcService.network(coin: coin, origin: origin) {
                self?.selectedChainIdForOrigin = network.chainId
              }
            }
          }
        }
      },
      _onAddEthereumChainRequestCompleted: { [weak self] _, _ in
        Task { [self] in
          await self?.updateChainList()
        }
      }
    )
    self.keyringServiceObserver = KeyringServiceObserver(
      keyringService: keyringService,
      _selectedWalletAccountChanged: { [weak self] account in
        Task { @MainActor [self] in
          if self?.defaultSelectedChain.coin != account.coin {
            if let selectedNetwork = await self?.rpcService.network(coin: account.coin, origin: nil)
            {
              self?.defaultSelectedChainId = selectedNetwork.chainId
            }
          }
          if let origin = self?.origin, self?.selectedChainForOrigin.coin != account.coin {
            // The default network may be used for this origin if no
            // other network was assigned for this origin. If so, we
            // need to make sure the `selectedChainIdForOrigin` is updated
            // to reflect the correct network.
            if let selectedNetwork = await self?.rpcService.network(
              coin: account.coin,
              origin: origin
            ) {
              self?.selectedChainIdForOrigin = selectedNetwork.chainId
            }
          }
        }
      }
    )

    self.networkSelectionStore?.setupObservers()
  }

  @MainActor func setup() async {
    await updateChainList()
    await updateSelectedChain()
  }

  /// Updates the `selectedChainId` and `isSwapSupported` for the network for the current `origin`.
  @MainActor private func updateSelectedChain() async {
    // fetch current selected network
    let selectedCoin = await keyringService.allAccounts().selectedAccount?.coin ?? .eth
    let chain = await rpcService.network(coin: selectedCoin, origin: nil)
    // since we are fetch network from JsonRpcService,
    // we don't need to call `setNetwork` on JsonRpcService
    self.defaultSelectedChainId = chain.chainId
    let chainForOrigin = await rpcService.network(coin: selectedCoin, origin: origin)
    self.selectedChainIdForOrigin = chainForOrigin.chainId
    // update `isSwapSupported` for Buy/Send/Swap panel
    self.isSwapSupported = await swapService.isSwapSupported(chainId: chain.chainId)
  }

  private func unhideAllSupportedTestnets() async {
    var networks: [BraveWallet.CoinType: [String]] = [:]
    for chainId in WalletConstants.supportedTestNetworkChainIds {
      var coin: BraveWallet.CoinType = .eth
      switch chainId {
      case BraveWallet.SepoliaChainId,
        BraveWallet.FilecoinEthereumTestnetChainId:
        coin = .eth
      case BraveWallet.SolanaDevnet,
        BraveWallet.SolanaTestnet:
        coin = .sol
      case BraveWallet.FilecoinTestnet:
        coin = .fil
      case BraveWallet.BitcoinTestnet:
        coin = .btc
      default:
        break
      }
      var ids = networks[coin] ?? []
      ids.append(chainId)
      networks[coin] = ids
    }
    await rpcService.removeHiddenNetworks(for: networks)
  }

  @MainActor func updateChainList() async {
    // fetch all networks for all coin types
    self.allChains = await rpcService.allNetworksForSupportedCoins(
      respectHiddenNetworksPreference: false
    )
  }

  @MainActor @discardableResult func setSelectedChain(
    _ network: BraveWallet.NetworkInfo,
    isForOrigin: Bool
  ) async -> SetSelectedChainError? {
    let allAccounts = await keyringService.allAccounts()
    let allAccountsForNetwork = allAccounts.accounts.accountsFor(network: network)
    if allAccountsForNetwork.isEmpty {
      // Need to prompt user to create new account via alert
      return .selectedChainHasNoAccounts
    } else {
      if isForOrigin && selectedChainIdForOrigin != network.chainId {
        self.selectedChainIdForOrigin = network.chainId
      } else if !isForOrigin && defaultSelectedChainId != network.chainId {
        self.defaultSelectedChainId = network.chainId
      }

      let currentlySelectedCoin = allAccounts.selectedAccount?.coin ?? .eth
      let rpcServiceNetwork = await rpcService.network(
        coin: currentlySelectedCoin,
        origin: isForOrigin ? origin : nil
      )
      guard rpcServiceNetwork.chainId != network.chainId else {
        if !isForOrigin {  // `isSwapSupported` is for the `defaultSelectedChain`
          self.isSwapSupported = await swapService.isSwapSupported(chainId: network.chainId)
        }
        return .chainAlreadySelected
      }

      let success = await rpcService.setNetwork(
        chainId: network.chainId,
        coin: network.coin,
        origin: isForOrigin ? origin : nil
      )
      if success,
        !isForOrigin
      {  // `isSwapSupported` is for the `defaultSelectedChain`
        self.isSwapSupported = await swapService.isSwapSupported(chainId: network.chainId)
      }
      return success ? nil : .unknown
    }
  }

  func subNetworks(for network: BraveWallet.NetworkInfo) -> [BraveWallet.NetworkInfo] {
    guard WalletConstants.primaryNetworkChainIds.contains(network.chainId)
    else {
      return []
    }
    let isPrimaryOrTestnetChainId: (_ chainId: String) -> Bool = { chainId in
      WalletConstants.primaryNetworkChainIds.contains(chainId)
        || WalletConstants.supportedTestNetworkChainIds.contains(chainId)
    }
    return allChains.filter {
      $0.coin == network.coin
        && !$0.props.isCustom
        && isPrimaryOrTestnetChainId($0.chainId)
    }
  }

  func openNetworkSelectionStore(
    mode: NetworkSelectionStore.Mode = .select(isForOrigin: false)
  ) -> NetworkSelectionStore {
    if let store = networkSelectionStore {
      if store.mode == mode {
        return store
      } else {
        networkSelectionStore = nil
      }
    }
    let store = NetworkSelectionStore(mode: mode, networkStore: self)
    networkSelectionStore = store
    return store
  }

  func closeNetworkSelectionStore() {
    networkSelectionStore = nil
  }

  @MainActor func setNetworkHidden(
    coin: BraveWallet.CoinType,
    chainId: String,
    hidden: Bool
  ) async {
    await rpcService.setNetworkHidden(
      coin: coin,
      chainId: chainId,
      hidden: hidden
    )
    await updateChainList()
  }

  @MainActor func updateDefaultNetwork(
    _ network: BraveWallet.NetworkInfo
  ) async {
    if network.props.isHidden {
      // network is currently hidden, need to unhide it first
      await setNetworkHidden(coin: network.coin, chainId: network.chainId, hidden: false)
    }
    let success = await rpcService.setNetwork(
      chainId: network.chainId,
      coin: network.coin,
      origin: nil
    )
    guard success else { return }
    await updateChainList()
  }

  // MARK: - Custom Networks

  @Published var isAddingNewNetwork: Bool = false

  @MainActor func addCustomNetwork(
    _ network: BraveWallet.NetworkInfo
  ) async -> (accepted: Bool, errMsg: String) {
    isAddingNewNetwork = true
    if allChains.contains(where: {
      $0.id.lowercased() == network.id.lowercased()
    }) {
      let removeStatus = await rpcService.removeChain(chainId: network.chainId, coin: network.coin)
      guard removeStatus else {
        return (false, "Not able to remove network chainId (\(network.chainId)")
      }
      // delete local stored user assets that in this custom network
      await assetManager.removeGroup(for: network.walletUserAssetGroupId)

      let (_, addStatus, errMsg) = await rpcService.addChain(network)
      guard addStatus == .success else {
        // if adding is not succeeded, we have to add back the old network info
        if let oldNetwork = allChains.first(where: {
          $0.id.lowercased() == network.id.lowercased()
        }) {
          let (_, addOldStatus, _) = await rpcService.addChain(oldNetwork)
          guard addOldStatus == .success else {
            isAddingNewNetwork = false
            return (false, errMsg)
          }
          await customNetworkNativeAssetMigration(network)
          isAddingNewNetwork = false
          await updateChainList()
          return (false, errMsg)
        } else {
          isAddingNewNetwork = false
          return (false, errMsg)
        }
      }
      await customNetworkNativeAssetMigration(network)
      isAddingNewNetwork = false
      await updateChainList()
      return (true, "")
    } else {
      let (_, addStatus, errMsg) = await rpcService.addChain(network)
      guard addStatus == .success else {
        isAddingNewNetwork = false
        return (false, errMsg)
      }
      await customNetworkNativeAssetMigration(network)
      isAddingNewNetwork = false
      await updateChainList()
      return (true, "")
    }
  }

  @MainActor public func removeCustomNetwork(
    _ network: BraveWallet.NetworkInfo
  ) async -> Bool {
    guard network.coin == .eth else {
      return false
    }

    let success = await rpcService.removeChain(chainId: network.chainId, coin: network.coin)
    if success {
      // check if its the current network, set mainnet the active net
      if network.id.lowercased() == defaultSelectedChainId.lowercased() {
        await rpcService.setNetwork(
          chainId: BraveWallet.MainnetChainId,
          coin: .eth,
          origin: nil
        )
      }
      // delete local stored user assets' balances that in this custom network
      await assetManager.removeUserAssetsAndBalances(for: network)
      await updateChainList()
    }
    return success
  }

  @MainActor func selectedNetwork(for coin: BraveWallet.CoinType) async -> BraveWallet.NetworkInfo {
    await rpcService.network(coin: coin, origin: nil)
  }

  @MainActor func customNetworkNativeAssetMigration(
    _ network: BraveWallet.NetworkInfo
  ) async {
    await assetManager.addUserAsset(
      network.nativeToken,
      isAutoDiscovery: false
    )
  }

  func network(for token: BraveWallet.BlockchainToken) -> BraveWallet.NetworkInfo? {
    return allChains.first { $0.chainId == token.chainId }
  }
}

extension Array where Element == BraveWallet.NetworkInfo {
  /// Returns the primary networks in Self.
  var primaryNetworks: [BraveWallet.NetworkInfo] {
    filter { WalletConstants.primaryNetworkChainIds.contains($0.chainId) }
  }

  /// Returns the secondary networks in Self.
  var secondaryNetworks: [BraveWallet.NetworkInfo] {
    filter {
      !WalletConstants.primaryNetworkChainIds.contains($0.chainId)
        && !WalletConstants.supportedTestNetworkChainIds.contains($0.chainId)
    }
  }

  /// Returns the known test networks in Self.
  var testNetworks: [BraveWallet.NetworkInfo] {
    filter {
      if !Preferences.Wallet.isBitcoinTestnetEnabled.value
        && $0.chainId == BraveWallet.BitcoinTestnet
      {
        return false
      }
      return WalletConstants.supportedTestNetworkChainIds.contains($0.chainId)
    }
  }
}
