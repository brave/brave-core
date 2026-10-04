// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import SwiftUI

class NetworkSelectionStore: ObservableObject, WalletObserverStore {

  enum Mode: Equatable {
    case select(isForOrigin: Bool)
    case formSelection
  }

  let mode: Mode
  var networkStore: NetworkStore

  /// If we are prompting the user to add an account for the `nextNetwork.coin` type
  @Published var isPresentingNextNetworkAlert = false
  /// The network the user wishes to switch to, but does not (yet) have an account for `nextNetwork.coin` type
  @Published var nextNetwork: BraveWallet.NetworkInfo?
  /// If we are prompting the user to create a new account for the `nextNetwork.coin` type
  @Published var isPresentingAddAccount: Bool = false
  /// The network the user wishes to choose for adding a custom asset
  @Published var networkSelectionInForm: BraveWallet.NetworkInfo?

  var isObserving: Bool = false

  init(
    mode: Mode = .select(isForOrigin: false),
    networkStore: NetworkStore
  ) {
    self.mode = mode
    self.networkStore = networkStore
  }

  @MainActor func selectNetwork(_ network: BraveWallet.NetworkInfo) async -> Bool {
    switch mode {
    case .select(let isForOrigin):
      let error = await networkStore.setSelectedChain(network, isForOrigin: isForOrigin)
      switch error {
      case .selectedChainHasNoAccounts:
        isPresentingNextNetworkAlert = true
        nextNetwork = network
        return false
      default:
        return true
      }
    case .formSelection:
      networkSelectionInForm = network
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

  /// Should be called after dismissing create account. Returns true if an account was created and we switched networks.
  @MainActor func handleDismissAddAccount() async -> Bool {
    guard let nextNetwork = nextNetwork else { return false }
    // if it errors it's due to no accounts and we don't want to switch to nextNetwork
    let result = await networkStore.setSelectedChain(nextNetwork, isForOrigin: false)
    self.nextNetwork = nil
    return result != .selectedChainHasNoAccounts
  }
}
