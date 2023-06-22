/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import BraveCore
import SwiftUI
import BraveShared
import Preferences

struct Selectable<T: Identifiable & Equatable>: Equatable, Identifiable {
  let isSelected: Bool
  let model: T
  
  var id: T.ID { model.id }
}

struct NetworkFilterView: View {
  
  @State var networks: [Selectable<BraveWallet.NetworkInfo>]
  @ObservedObject var networkStore: NetworkStore
  let saveAction: ([Selectable<BraveWallet.NetworkInfo>]) -> Void
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  init(
    networks: [Selectable<BraveWallet.NetworkInfo>],
    networkStore: NetworkStore,
    saveAction: @escaping ([Selectable<BraveWallet.NetworkInfo>]) -> Void
  ) {
    self._networks = .init(initialValue: networks)
    self.networkStore = networkStore
    self.saveAction = saveAction
  }
  
  private var allSelected: Bool {
    networks
      .filter {
        if !Preferences.Wallet.showTestNetworks.value {
          return !WalletConstants.supportedTestNetworkChainIds.contains($0.model.chainId)
        }
        return true
      }
      .allSatisfy(\.isSelected)
  }
  
  var body: some View {
    NetworkSelectionRootView(
      navigationTitle: Strings.Wallet.networkFilterTitle,
      selectedNetworks: networks.filter(\.isSelected).map(\.model),
      allNetworks: networks.map(\.model),
      selectNetwork: selectNetwork
    )
    .toolbar {
      ToolbarItem(placement: .confirmationAction) {
        Button(action: {
          saveAction(networks)
          presentationMode.dismiss()
        }) {
          Text(Strings.Wallet.saveButtonTitle)
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
    }
    .toolbar {
      ToolbarItemGroup(placement: .bottomBar) {
        Spacer()
        Button(action: selectAll) {
          Text(allSelected ? Strings.Wallet.deselectAllButtonTitle : Strings.Wallet.selectAllButtonTitle)
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
    }
  }
  
  private func selectNetwork(_ network: BraveWallet.NetworkInfo) {
    DispatchQueue.main.async {
      if let index = networks.firstIndex(
        where: { $0.model.chainId == network.chainId && $0.model.coin == network.coin }
      ) {
        networks[index] = .init(isSelected: !networks[index].isSelected, model: networks[index].model)
      }
    }
  }
  
  private func selectAll() {
    DispatchQueue.main.async {
      networks = networks.map {
        // don't select test networks if they are hidden
        if !Preferences.Wallet.showTestNetworks.value {
          let isTestnet = WalletConstants.supportedTestNetworkChainIds.contains($0.model.chainId)
          return .init(isSelected: !isTestnet && !allSelected, model: $0.model)
        } else {
          return .init(isSelected: !allSelected, model: $0.model)
        }
      }
    }
  }
}
