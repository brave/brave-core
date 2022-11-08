// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import SwiftUI

struct NetworkPresentation: Equatable, Hashable, Identifiable {
  enum Network: Equatable, Hashable {
    case allNetworks
    case network(BraveWallet.NetworkInfo)
  }
  
  var id: String {
    switch network {
    case .allNetworks: return "allNetworks"
    case let .network(network): return network.id
    }
  }
  let network: Network
  let subNetworks: [BraveWallet.NetworkInfo]
  let isPrimaryNetwork: Bool
  
  init(
    network: Network,
    subNetworks: [BraveWallet.NetworkInfo],
    isPrimaryNetwork: Bool
  ) {
    self.network = network
    self.subNetworks = subNetworks
    self.isPrimaryNetwork = isPrimaryNetwork
  }
  
  static let allNetworks: Self = .init(
    network: .allNetworks,
    subNetworks: [],
    isPrimaryNetwork: true
  )
}

class NetworkSelectionStore: ObservableObject {
  
  enum Mode: Equatable {
    case select
    case filter
    case formSelection
    
    var isSelectMode: Bool { self == .select }
    var isFilterMode: Bool { self == .filter }
  }
  
  let mode: Mode
  var networkStore: NetworkStore
  
  @Published private(set) var primaryNetworks: [NetworkPresentation] = []
  @Published private(set) var secondaryNetworks: [NetworkPresentation] = []
  
  /// Network selected to show detail view (test networks)
  @Published var detailNetwork: NetworkPresentation?
  /// If we are prompting the user to add an account for the `nextNetwork.coin` type
  @Published var isPresentingNextNetworkAlert = false
  /// The network the user wishes to switch to, but does not (yet) have an account for `nextNetwork.coin` type
  @Published var nextNetwork: BraveWallet.NetworkInfo?
  /// If we are prompting the user to create a new account for the `nextNetwork.coin` type
  @Published var isPresentingAddAccount: Bool = false
  /// The network the user wishes to choose for adding a custom asset
  @Published var networkSelectionInForm: BraveWallet.NetworkInfo?
  
  init(
    mode: Mode = .select,
    networkStore: NetworkStore
  ) {
    self.mode = mode
    self.networkStore = networkStore
  }
  
  func update() {
    let subNetworks: (BraveWallet.NetworkInfo) -> [BraveWallet.NetworkInfo] = { [weak networkStore] network in
      guard let networkStore = networkStore,
              WalletConstants.primaryNetworkChainIds.contains(network.chainId),
              Preferences.Wallet.showTestNetworks.value else {
        return []
      }
      let isPrimaryOrTestnetChainId: (_ chainId: String) -> Bool = { chainId in
        WalletConstants.primaryNetworkChainIds.contains(chainId)
        || WalletConstants.supportedTestNetworkChainIds.contains(chainId)
      }
      return networkStore.allChains.filter {
        $0.coin == network.coin
        && $0.symbol == network.symbol
        && !networkStore.isCustomChain($0)
        && isPrimaryOrTestnetChainId($0.chainId)
      }
    }
    
    var primaryNetworks = networkStore.allChains
      .filter { WalletConstants.primaryNetworkChainIds.contains($0.chainId) }
      .map { network in
        let subNetworks = subNetworks(network)
        return NetworkPresentation(
          network: .network(network),
          subNetworks: subNetworks.count > 1 ? subNetworks : [],
          isPrimaryNetwork: true
        )
      }
    if mode == .filter {
      // users can filter by 'All Networks' but cannot select 'All Networks'
      primaryNetworks.insert(.allNetworks, at: 0)
    }
    self.primaryNetworks = primaryNetworks

    self.secondaryNetworks = networkStore.allChains
      .filter {
        !WalletConstants.primaryNetworkChainIds.contains($0.chainId)
        && !WalletConstants.supportedTestNetworkChainIds.contains($0.chainId)
      }
      .map { network in
        NetworkPresentation(
          network: .network(network),
          subNetworks: [],
          isPrimaryNetwork: false
        )
      }
  }
  
  @MainActor func selectNetwork(_ network: NetworkPresentation.Network) async -> Bool {
    switch mode {
    case .select:
      guard case let .network(network) = network else { return false }
      detailNetwork = nil

      let error = await networkStore.setSelectedChain(network)
      switch error {
      case .selectedChainHasNoAccounts:
        isPresentingNextNetworkAlert = true
        nextNetwork = network
        return false
      default:
        return true
      }
    case .filter:
      switch network {
      case .allNetworks:
        networkStore.networkFilter = .allNetworks
      case let .network(network):
        networkStore.networkFilter = .network(network)
      }
      return true
    case .formSelection:
      switch network {
      case let .network(network):
        networkSelectionInForm = network
        return true
      default:
        return false
      }
    }
  }
  
  func handleCreateAccountAlertResponse(shouldCreateAccount: Bool) {
    if shouldCreateAccount {
      // show create account for `nextNetwork.coin`
      self.isPresentingNextNetworkAlert = false
      self.isPresentingAddAccount = true
    } else {
      // not creating account, don't switch to nextNetwork
      self.isPresentingNextNetworkAlert = false
      self.nextNetwork = nil
    }
  }
  
  /// Should be called after dismissing create account. Returns true if an account was created and we switched networks.
  @MainActor func handleDismissAddAccount() async -> Bool {
    guard let nextNetwork = nextNetwork else { return false }
    // if it errors it's due to no accounts and we don't want to switch to nextNetwork
    let result = await networkStore.setSelectedChain(nextNetwork)
    self.nextNetwork = nil
    return result != .selectedChainHasNoAccounts
  }
}
