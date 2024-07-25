// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import SwiftUI

struct NetworkSelectionView: View {

  enum NetworkSelectionType {
    case addCustomAsset
    case other
  }

  var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var store: NetworkSelectionStore
  @Environment(\.presentationMode) @Binding private var presentationMode

  init(
    keyringStore: KeyringStore,
    networkStore: NetworkStore,
    networkSelectionStore: NetworkSelectionStore,
    networkSelectionType: NetworkSelectionType = .other
  ) {
    self.keyringStore = keyringStore
    self.networkStore = networkStore
    self.store = networkSelectionStore
    self.networkSelectionType = networkSelectionType
  }

  private var selectedNetwork: BraveWallet.NetworkInfo {
    switch store.mode {
    case .select(let isForOrigin):
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

  private var networkSelectionType: NetworkSelectionType

  private var allSelectableNetworks: [BraveWallet.NetworkInfo] {
    switch networkSelectionType {
    case .addCustomAsset:
      return networkStore.visibleChains.filter {
        $0.coin == .eth || $0.coin == .sol
      }
    case .other:
      return networkStore.visibleChains
    }
  }

  var body: some View {
    NetworkSelectionRootView(
      navigationTitle: navigationTitle,
      selectedNetworks: [selectedNetwork],
      allNetworks: allSelectableNetworks,
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
