// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import SwiftUI

class NetworkSelectionStore: ObservableObject {
  
  enum Mode: Equatable {
    case select
    case formSelection
  }
  
  let mode: Mode
  var networkStore: NetworkStore
  
  @Published private(set) var primaryNetworks: [NetworkPresentation] = []
  @Published private(set) var secondaryNetworks: [NetworkPresentation] = []
  
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
    self.primaryNetworks = networkStore.primaryNetworks
      .map { network in
        let subNetworks = networkStore.subNetworks(for: network)
        return NetworkPresentation(
          network: .network(network),
          subNetworks: subNetworks.count > 1 ? subNetworks : [],
          isPrimaryNetwork: true
        )
      }

    self.secondaryNetworks = networkStore.secondaryNetworks
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

      let error = await networkStore.setSelectedChain(network)
      switch error {
      case .selectedChainHasNoAccounts:
        isPresentingNextNetworkAlert = true
        nextNetwork = network
        return false
      default:
        return true
      }
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
