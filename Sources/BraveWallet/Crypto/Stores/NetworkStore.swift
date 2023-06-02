/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveCore
import SwiftUI
import Strings
import Preferences
import Combine

/// An interface that helps you interact with a json-rpc service
///
/// This wraps a JsonRpcService that you would obtain through BraveCore and makes it observable
public class NetworkStore: ObservableObject {
  
  enum SetSelectedChainError: Error {
    case selectedChainHasNoAccounts
    case chainAlreadySelected
    case unknown
  }
  
  @Published private(set) var allChains: [BraveWallet.NetworkInfo] = []
  @Published private(set) var customChains: [BraveWallet.NetworkInfo] = []
  
  /// The primary networks from `allChains`
  var primaryNetworks: [BraveWallet.NetworkInfo] {
    allChains
      .filter { WalletConstants.primaryNetworkChainIds.contains($0.chainId) }
  }
  
  // The secondary networks from `allChains`
  var secondaryNetworks: [BraveWallet.NetworkInfo] {
    allChains
      .filter {
        !WalletConstants.primaryNetworkChainIds.contains($0.chainId)
        && !WalletConstants.supportedTestNetworkChainIds.contains($0.chainId)
      }
  }

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

  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let swapService: BraveWalletSwapService
  
  private weak var networkSelectionStore: NetworkSelectionStore?

  public init(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    swapService: BraveWalletSwapService,
    origin: URLOrigin? = nil
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.swapService = swapService
    self.origin = origin
    rpcService.add(self)
    keyringService.add(self)
  }
  
  @MainActor func setup() async {
    await updateChainList()
    await updateSelectedChain()
  }
  
  /// Updates the `selectedChainId` and `isSwapSupported` for the network for the current `origin`.
  @MainActor private func updateSelectedChain() async {
    // fetch current selected network
    let selectedCoin = await walletService.selectedCoin()
    let chain = await rpcService.network(selectedCoin, origin: nil)
    // since we are fetch network from JsonRpcService,
    // we don't need to call `setNetwork` on JsonRpcService
    self.defaultSelectedChainId = chain.chainId
    let chainForOrigin = await rpcService.network(selectedCoin, origin: origin)
    self.selectedChainIdForOrigin = chainForOrigin.chainId
    // update `isSwapSupported` for Buy/Send/Swap panel
    self.isSwapSupported = await swapService.isSwapSupported(chain.chainId)
  }

  @MainActor private func updateChainList() async {
    // fetch all networks for all coin types
    self.allChains = await rpcService.allNetworksForSupportedCoins()
    
    let customChainIds = await rpcService.customNetworks(.eth) // only support Ethereum custom chains
    self.customChains = allChains.filter { customChainIds.contains($0.id) }
  }
  
  func isCustomChain(_ network: BraveWallet.NetworkInfo) -> Bool {
    customChains.contains(where: { $0.coin == network.coin && $0.chainId == network.chainId })
  }
  
  @MainActor @discardableResult func setSelectedChain(
    _ network: BraveWallet.NetworkInfo,
    isForOrigin: Bool
  ) async -> SetSelectedChainError? {
    let keyringId = network.coin.keyringId
    let keyringInfo = await keyringService.keyringInfo(keyringId)
    if keyringInfo.accountInfos.isEmpty {
      // Need to prompt user to create new account via alert
      return .selectedChainHasNoAccounts
    } else {
      let selectedCoin = await walletService.selectedCoin()
      if isForOrigin {
        if self.selectedChainIdForOrigin != network.chainId {
          self.selectedChainIdForOrigin = network.chainId
        }
      } else {
        if self.defaultSelectedChainId != network.chainId {
          self.defaultSelectedChainId = network.chainId
        }
      }
      if isForOrigin && selectedChainIdForOrigin != network.chainId {
        self.selectedChainIdForOrigin = network.chainId
      } else if defaultSelectedChainId != network.chainId {
        self.defaultSelectedChainId = network.chainId
      }
      if selectedCoin != network.coin {
        let success = await rpcService.setNetwork(network.chainId, coin: network.coin, origin: isForOrigin ? origin : nil)
        return success ? nil : .unknown
      } else {
        let rpcServiceNetwork = await rpcService.network(network.coin, origin: isForOrigin ? origin : nil)
        guard rpcServiceNetwork.chainId != network.chainId else {
          if !isForOrigin { // `isSwapSupported` is for the `defaultSelectedChain`
            self.isSwapSupported = await swapService.isSwapSupported(network.chainId)
          }
          return .chainAlreadySelected
        }
        let success = await rpcService.setNetwork(network.chainId, coin: network.coin, origin: isForOrigin ? origin : nil)
        return success ? nil : .unknown
      }
    }
  }
  
  func subNetworks(for network: BraveWallet.NetworkInfo) -> [BraveWallet.NetworkInfo] {
    guard WalletConstants.primaryNetworkChainIds.contains(network.chainId),
          Preferences.Wallet.showTestNetworks.value else {
      return []
    }
    let isPrimaryOrTestnetChainId: (_ chainId: String) -> Bool = { chainId in
      WalletConstants.primaryNetworkChainIds.contains(chainId)
      || WalletConstants.supportedTestNetworkChainIds.contains(chainId)
    }
    return allChains.filter {
      $0.coin == network.coin
      && $0.symbol == network.symbol
      && !isCustomChain($0)
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

  // MARK: - Custom Networks

  @Published var isAddingNewNetwork: Bool = false

  public func addCustomNetwork(
    _ network: BraveWallet.NetworkInfo,
    completion: @escaping (_ accepted: Bool, _ errMsg: String) -> Void
  ) {
    func add(network: BraveWallet.NetworkInfo, completion: @escaping (_ accepted: Bool, _ errMsg: String) -> Void) {
      rpcService.addChain(network) { [self] chainId, status, message in
        if status == .success {
          // Update `ethereumChains` by api calling
          Task {
            await updateChainList()
          }
          isAddingNewNetwork = false
          completion(true, "")
        } else {
          // meaning add custom network failed for some reason.
          // Also add the the old network back on rpc service
          if let oldNetwork = allChains.filter({ $0.coin == .eth }).first(where: { $0.id.lowercased() == network.id.lowercased() }) {
            rpcService.addChain(oldNetwork) { _, _, _ in
              Task {
                await self.updateChainList()
              }
              self.isAddingNewNetwork = false
              completion(false, message)
            }
          } else {
            isAddingNewNetwork = false
            completion(false, message)
          }
        }
      }
    }

    isAddingNewNetwork = true
    if allChains.filter({ $0.coin == .eth }).contains(where: { $0.id.lowercased() == network.id.lowercased() }) {
      removeNetworkForNewAddition(network) { [self] success in
        guard success else {
          isAddingNewNetwork = false
          completion(false, Strings.Wallet.failedToRemoveCustomNetworkErrorMessage)
          return
        }
        add(network: network, completion: completion)
      }
    } else {
      add(network: network, completion: completion)
    }
  }

  /// This method will not update `ethereumChains`
  private func removeNetworkForNewAddition(
    _ network: BraveWallet.NetworkInfo,
    completion: @escaping (_ success: Bool) -> Void
  ) {
    rpcService.removeChain(network.id, coin: network.coin, completion: completion)
  }

  public func removeCustomNetwork(
    _ network: BraveWallet.NetworkInfo,
    completion: @escaping (_ success: Bool) -> Void
  ) {
    guard network.coin == .eth else {
      completion(false)
      return
    }
    rpcService.removeChain(network.chainId, coin: network.coin) { [self] success in
      if success {
        // check if its the current network, set mainnet the active net
        if network.id.lowercased() == defaultSelectedChainId.lowercased() {
          rpcService.setNetwork(BraveWallet.MainnetChainId, coin: .eth, origin: nil, completion: { _ in })
        }
        Task {
          await updateChainList()
        }
      }
      completion(success)
    }
  }
  
  @MainActor func selectedNetwork(for coin: BraveWallet.CoinType) async -> BraveWallet.NetworkInfo {
    await rpcService.network(coin, origin: nil)
  }
}

extension NetworkStore: BraveWalletJsonRpcServiceObserver {
  public func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
  public func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
    Task {
      await updateChainList()
    }
  }
  public func chainChangedEvent(_ chainId: String, coin: BraveWallet.CoinType, origin: URLOrigin?) {
    Task { @MainActor in
      walletService.setSelectedCoin(coin)
      // since JsonRpcService notify us of change,
      // we don't need to call `setNetwork` on JsonRpcService
      if let origin {
        if origin == self.origin {
          selectedChainIdForOrigin = chainId
        }
      } else {
        defaultSelectedChainId = chainId
        isSwapSupported = await swapService.isSwapSupported(chainId)
        if self.origin != nil {
          // The default network may be used for this origin if no
          // other network was assigned for this origin. If so, we
          // need to make sure the `selectedChainIdForOrigin` is updated
          // to reflect the correct network.
          let network = await rpcService.network(coin, origin: origin)
          selectedChainIdForOrigin = network.chainId
        }
      }
    }
  }
}

extension NetworkStore: BraveWalletKeyringServiceObserver {
  public func selectedAccountChanged(_ coin: BraveWallet.CoinType) {
    Task { @MainActor in
      // coin type of selected account might have changed
      let chain = await rpcService.network(coin, origin: nil)
      await setSelectedChain(chain, isForOrigin: false)
      if let origin {
        let chainForOrigin = await rpcService.network(coin, origin: origin)
        await setSelectedChain(chainForOrigin, isForOrigin: true)
      }
    }
  }
  
  public func keyringCreated(_ keyringId: String) {
  }
  
  public func keyringRestored(_ keyringId: String) {
  }
  
  public func keyringReset() {
  }
  
  public func locked() {
  }
  
  public func unlocked() {
  }
  
  public func backedUp() {
  }
  
  public func accountsChanged() {
  }
  
  public func autoLockMinutesChanged() {
  }
  
  public func accountsAdded(_ addedAccounts: [BraveWallet.AccountInfo]) {
  }
}
