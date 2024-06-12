// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import Preferences
import SwiftUI

struct Selectable<T: Identifiable & Equatable>: Equatable, Identifiable {
  let isSelected: Bool
  let model: T

  var id: T.ID { model.id }
}

struct NetworkFilterView: View {

  @State var networks: [Selectable<BraveWallet.NetworkInfo>]
  @ObservedObject var networkStore: NetworkStore
  let showsCancelButton: Bool
  let requiresSave: Bool
  let saveAction: ([Selectable<BraveWallet.NetworkInfo>]) -> Void

  @Environment(\.presentationMode) @Binding private var presentationMode

  init(
    networks: [Selectable<BraveWallet.NetworkInfo>],
    networkStore: NetworkStore,
    showsCancelButton: Bool = true,
    requiresSave: Bool = true,
    saveAction: @escaping ([Selectable<BraveWallet.NetworkInfo>]) -> Void
  ) {
    self._networks = .init(initialValue: networks)
    self.networkStore = networkStore
    self.showsCancelButton = showsCancelButton
    self.requiresSave = requiresSave
    self.saveAction = saveAction
  }

  private var allSelected: Bool {
    networks
      .filter { network in
        !networkStore.hiddenChains.contains { hiddenChain in
          hiddenChain.chainId == network.model.chainId
        }
      }
      .allSatisfy(\.isSelected)
  }

  var body: some View {
    NetworkSelectionRootView(
      navigationTitle: Strings.Wallet.networkFilterTitle,
      selectedNetworks: networks.filter(\.isSelected).map(\.model),
      allNetworks: networks.map(\.model),
      showsCancelButton: showsCancelButton,
      showsSelectAllButton: true,
      selectNetwork: selectNetwork
    )
    .toolbar {
      ToolbarItem(placement: .confirmationAction) {
        if requiresSave {
          Button {
            saveAction(networks)
            presentationMode.dismiss()
          } label: {
            Text(Strings.Wallet.saveButtonTitle)
              .foregroundColor(Color(.braveBlurpleTint))
          }
        }
      }
    }
    .onChange(of: networks) { networks in
      if !requiresSave {
        // No save button, so call saveAction when updating selections
        saveAction(networks)
      }
    }
  }

  private func selectNetwork(_ network: BraveWallet.NetworkInfo) {
    DispatchQueue.main.async {
      if let index = networks.firstIndex(
        where: { $0.model.chainId == network.chainId && $0.model.coin == network.coin }
      ) {
        networks[index] = .init(
          isSelected: !networks[index].isSelected,
          model: networks[index].model
        )
      }
    }
  }

  private func selectAll() {
    DispatchQueue.main.async {
      networks = networks.map {
        .init(isSelected: !allSelected, model: $0.model)
      }
    }
  }
}

#if DEBUG
struct NetworkFilterView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      NetworkFilterView(
        networks: [
          .init(isSelected: true, model: .mockMainnet),
          .init(isSelected: true, model: .mockSolana),
          .init(isSelected: true, model: .mockPolygon),
          .init(isSelected: true, model: .mockSepolia),
          .init(isSelected: true, model: .mockSolanaTestnet),
        ],
        networkStore: .previewStore,
        requiresSave: false,
        saveAction: { _ in

        }
      )
    }
  }
}
#endif
