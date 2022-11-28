/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import BraveCore
import SwiftUI
import BraveShared

struct NetworkFilterView: View {
  
  @Binding var networkFilter: NetworkFilter
  @ObservedObject var networkStore: NetworkStore
  
  @State private(set) var primaryNetworks: [NetworkPresentation] = []
  @State private(set) var secondaryNetworks: [NetworkPresentation] = []
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  private var selectedNetwork: NetworkPresentation.Network {
    switch networkFilter {
    case .allNetworks:
      return .allNetworks
    case let .network(network):
      return .network(network)
    }
  }
  
  var body: some View {
    NetworkSelectionRootView(
      navigationTitle: Strings.Wallet.networkFilterTitle,
      selectedNetwork: selectedNetwork,
      primaryNetworks: primaryNetworks,
      secondaryNetworks: secondaryNetworks,
      selectNetwork: selectNetwork
    )
    .onAppear {
      fetchNetworks()
    }
  }
  
  private func fetchNetworks() {
    let primaryNetworks = networkStore.primaryNetworks
      .map { network in
        let subNetworks = networkStore.subNetworks(for: network)
        return NetworkPresentation(
          network: .network(network),
          subNetworks: subNetworks.count > 1 ? subNetworks : [],
          isPrimaryNetwork: true
        )
      }
    self.primaryNetworks = [.allNetworks] + primaryNetworks

    self.secondaryNetworks = networkStore.secondaryNetworks
      .map { network in
        NetworkPresentation(
          network: .network(network),
          subNetworks: [],
          isPrimaryNetwork: false
        )
      }
  }
  
  private func selectNetwork(_ network: NetworkPresentation.Network) {
    switch network {
    case .allNetworks:
      networkFilter = .allNetworks
    case let .network(network):
      networkFilter = .network(network)
    }
    presentationMode.dismiss()
  }
}
