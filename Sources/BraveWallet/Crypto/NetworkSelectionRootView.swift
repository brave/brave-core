/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import BraveCore
import SwiftUI
import Preferences

struct NetworkSelectionRootView: View {
  
  var navigationTitle: String
  var selectedNetworks: [BraveWallet.NetworkInfo]
  var allNetworks: [BraveWallet.NetworkInfo]
  var selectNetwork: (BraveWallet.NetworkInfo) -> Void
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  var body: some View {
    List {
      Section {
        ForEach(allNetworks.primaryNetworks) { network in
          Button(action: { selectNetwork(network) }) {
            NetworkRowView(
              network: network,
              selectedNetworks: selectedNetworks
            )
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      }
      Section(content: {
        ForEach(allNetworks.secondaryNetworks) { network in
          Button(action: { selectNetwork(network) }) {
            NetworkRowView(
              network: network,
              selectedNetworks: selectedNetworks
            )
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      }, header: {
        WalletListHeaderView(title: Text(Strings.Wallet.networkSelectionSecondaryNetworks))
      })
      if Preferences.Wallet.showTestNetworks.value && !allNetworks.testNetworks.isEmpty {
        Section(content: {
          ForEach(allNetworks.testNetworks) { network in
            Button(action: { selectNetwork(network) }) {
              NetworkRowView(
                network: network,
                selectedNetworks: selectedNetworks
              )
            }
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }
        }, header: {
          WalletListHeaderView(title: Text("Test Networks"))
        })
      }
    }
    .listStyle(.insetGrouped)
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationTitle(navigationTitle)
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      ToolbarItemGroup(placement: .cancellationAction) {
        Button(action: { presentationMode.dismiss() }) {
          Text(Strings.cancelButtonTitle)
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
    }
  }
}

private struct NetworkRowView: View {

  var network: BraveWallet.NetworkInfo
  var selectedNetworks: [BraveWallet.NetworkInfo]

  @ScaledMetric private var length: CGFloat = 30
  
  init(
    network: BraveWallet.NetworkInfo,
    selectedNetworks: [BraveWallet.NetworkInfo]
  ) {
    self.network = network
    self.selectedNetworks = selectedNetworks
  }
  
  private var isSelected: Bool {
    selectedNetworks.contains(where: { $0.chainId == network.chainId })
  }

  private var checkmark: some View {
    Image(braveSystemName: "leo.check.normal")
      .resizable()
      .aspectRatio(contentMode: .fit)
      .opacity(isSelected ? 1 : 0)
      .foregroundColor(Color(.braveBlurpleTint))
      .frame(width: 14, height: 14)
  }

  var body: some View {
    HStack {
      checkmark
      NetworkIcon(network: network)
      VStack(alignment: .leading, spacing: 0) {
        Text(network.chainName)
          .font(.body)
      }
      .frame(minHeight: length) // maintain height for All Networks row w/o icon
      Spacer()
    }
    .accessibilityElement(children: .combine)
    .accessibilityAddTraits(isSelected ? [.isSelected] : [])
    .foregroundColor(Color(.braveLabel))
    .padding(.vertical, 4)
    .frame(maxWidth: .infinity, alignment: .leading)
    .contentShape(Rectangle())
  }
}

#if DEBUG
struct NetworkRowView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      NetworkRowView(
        network: .mockSolana,
        selectedNetworks: [.mockSolana]
      )
      NetworkRowView(
        network: .mockMainnet,
        selectedNetworks: [.mockMainnet]
      )
      NetworkRowView(
        network: .mockPolygon,
        selectedNetworks: [.mockMainnet]
      )
    }
    .previewLayout(.sizeThatFits)
  }
}
#endif
