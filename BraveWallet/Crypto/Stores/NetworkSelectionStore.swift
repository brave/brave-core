// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import SwiftUI

class NetworkSelectionStore: ObservableObject {
  
  struct NetworkPresentation: Equatable, Hashable, Identifiable {
    var id: String { network.id }
    let network: BraveWallet.NetworkInfo
    let subNetworks: [BraveWallet.NetworkInfo]
    let isPrimaryNetwork: Bool
    
    init(
      network: BraveWallet.NetworkInfo,
      subNetworks: [BraveWallet.NetworkInfo],
      isPrimaryNetwork: Bool
    ) {
      self.network = network
      self.subNetworks = subNetworks
      self.isPrimaryNetwork = isPrimaryNetwork
    }
  }
  
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
  
  init(
    networkStore: NetworkStore
  ) {
    self.networkStore = networkStore
  }
  
  func update() {
    let subNetworks: (BraveWallet.NetworkInfo) -> [BraveWallet.NetworkInfo] = { [weak networkStore] network in
      guard let networkStore = networkStore,
              WalletConstants.primaryNetworkChainIds.contains(network.chainId),
              Preferences.Wallet.showTestNetworks.value else {
        return []
      }
      return networkStore.allChains.filter {
        $0.coin == network.coin
        && $0.symbol == network.symbol
        && $0.chainId != BraveWallet.OptimismMainnetChainId
        && !$0.isCustom
      }
    }
    
    self.primaryNetworks = networkStore.allChains
      .filter { WalletConstants.primaryNetworkChainIds.contains($0.chainId) }
      .map { network in
        let subNetworks = subNetworks(network)
        return NetworkPresentation(
          network: network,
          subNetworks: subNetworks.count > 1 ? subNetworks : [],
          isPrimaryNetwork: true
        )
      }

    self.secondaryNetworks = networkStore.allChains
      .filter {
        !WalletConstants.primaryNetworkChainIds.contains($0.chainId)
        && !WalletConstants.supportedTestNetworkChainIds.contains($0.chainId)
      }
      .map { network in
        NetworkPresentation(
          network: network,
          subNetworks: [],
          isPrimaryNetwork: false
        )
      }
  }
  
  @MainActor func selectNetwork(network: BraveWallet.NetworkInfo) async -> Bool {
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
  
  @MainActor func handleDismissAddAccount() async {
    guard let nextNetwork = nextNetwork else { return }
    // if it errors it's due to no accounts and we don't want to switch to nextNetwork
    await networkStore.setSelectedChain(nextNetwork)
    self.nextNetwork = nil
  }
}
