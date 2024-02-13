/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import BraveCore
import DesignSystem
import SwiftUI
import BraveUI

struct NetworkSelectionView: View {
  
  var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var store: NetworkSelectionStore
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  init(
    keyringStore: KeyringStore,
    networkStore: NetworkStore,
    networkSelectionStore: NetworkSelectionStore
  ) {
    self.keyringStore = keyringStore
    self.networkStore = networkStore
    self.store = networkSelectionStore
  }
  
  private var selectedNetwork: BraveWallet.NetworkInfo {
    switch store.mode {
    case let .select(isForOrigin):
      if isForOrigin {
        return networkStore.selectedChainForOrigin
      }
      return networkStore.defaultSelectedChain
    case .formSelection:
      return store.networkSelectionInForm ?? .init()
    }
  }
  
  private var navigationTitle: String {
    switch store.mode {
    case .select: return Strings.Wallet.networkSelectionTitle
    case .formSelection: return Strings.Wallet.networkSelectionTitle
    }
  }
  
  var body: some View {
    NetworkSelectionRootView(
      navigationTitle: navigationTitle,
      selectedNetworks: [selectedNetwork],
      allNetworks: networkStore.allChains,
      selectNetwork: { network in
        selectNetwork(network)
      }
    )
    .addAccount(
      keyringStore: keyringStore,
      networkStore: networkStore,
      accountNetwork: store.nextNetwork,
      isShowingConfirmation: $store.isPresentingNextNetworkAlert,
      isShowingAddAccount: $store.isPresentingAddAccount,
      onConfirmAddAccount: {
        store.handleCreateAccountAlertResponse(shouldCreateAccount: true)
      },
      onCancelAddAccount: {
        store.handleCreateAccountAlertResponse(shouldCreateAccount: false)
      },
      onAddAccountDismissed: {
        Task { @MainActor in
          if await store.handleDismissAddAccount() {
            presentationMode.dismiss()
          }
        }
      }
    )
  }
  
  private func selectNetwork(_ network: BraveWallet.NetworkInfo) {
    Task { @MainActor in
      if await store.selectNetwork(network) {
        presentationMode.dismiss()
      }
    }
  }
}
