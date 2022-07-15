// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveShared
import BraveCore
import Strings

struct CustomNetworkListView: View {
  @ObservedObject var networkStore: NetworkStore
  @State private var isPresentingNetworkDetails: CustomNetworkModel?
  @Environment(\.presentationMode) @Binding private var presentationMode
  @Environment(\.sizeCategory) private var sizeCategory
  @ObservedObject private var showTestNetworks = Preferences.Wallet.showTestNetworks

  private struct CustomNetworkDetails: Identifiable {
    var isEditMode: Bool
    var network: BraveWallet.NetworkInfo?
    var id: String {
      "\(isEditMode)"
    }
  }

  private func removeNetwork(_ network: BraveWallet.NetworkInfo) {
    networkStore.removeCustomNetwork(network) { _ in }
  }
  
  private var customNetworks: [BraveWallet.NetworkInfo] {
    networkStore.allChains.filter { $0.isCustom }
  }
  
  @ViewBuilder private var customNetworksList: some View {
    ForEach(customNetworks) { network in
      Button(action: {
        isPresentingNetworkDetails = .init(from: network)
      }) {
        HStack {
          VStack(alignment: .leading, spacing: 2) {
            Text(network.chainName)
              .foregroundColor(Color(.braveLabel))
              .font(.callout)
            Group {
              if sizeCategory.isAccessibilityCategory {
                VStack(alignment: .leading) {
                  Text(network.id)
                  Text(network.rpcUrls.first ?? "")
                }
              } else {
                HStack {
                  Text(network.id)
                  Text(network.rpcUrls.first ?? "")
                }
              }
            }
            .foregroundColor(Color(.secondaryBraveLabel))
            .font(.footnote)
          }
          Spacer()
          Image(systemName: "chevron.right")
            .font(.footnote.weight(.semibold))
            .foregroundColor(Color(.separator))
        }
        .padding(.vertical, 6)
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      .osAvailabilityModifiers { content in
        if #available(iOS 15.0, *) {
          content
            .swipeActions(edge: .trailing) {
              Button(role: .destructive, action: {
                removeNetwork(network)
              }) {
                Label(Strings.Wallet.delete, systemImage: "trash")
              }
            }
        } else {
          content
        }
      }
    }
    .onDelete { indexSet in
      let networksToRemove = indexSet.map({ customNetworks[$0] })
      withAnimation(.default) {
        for network in networksToRemove {
          removeNetwork(network)
        }
      }
    }
  }

  var body: some View {
    List {
      if !customNetworks.isEmpty {
        Section {
          customNetworksList
        }
      }
      
      Section {
        Toggle(Strings.Wallet.showTestNetworksTitle, isOn: $showTestNetworks.value)
          .foregroundColor(Color(.braveLabel))
          .toggleStyle(SwitchToggleStyle(tint: Color(.braveBlurpleTint)))
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
    }
    .listStyle(.insetGrouped)
    .overlay(
      Group {
        if customNetworks.isEmpty {
          Text(Strings.Wallet.noNetworks)
            .font(.headline.weight(.medium))
            .frame(maxWidth: .infinity)
            .multilineTextAlignment(.center)
            .foregroundColor(Color(.secondaryBraveLabel))
            .transition(.opacity)
        }
      }
    )
    .navigationTitle(Strings.Wallet.customNetworksTitle)
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      ToolbarItemGroup(placement: .confirmationAction) {
        Button(action: {
          isPresentingNetworkDetails = .init()
        }) {
          Label(Strings.Wallet.addCustomNetworkBarItemTitle, systemImage: "plus")
            .foregroundColor(Color(.braveOrange))
        }
      }
    }
    .sheet(item: $isPresentingNetworkDetails) { detailsModel in
      NavigationView {
        CustomNetworkDetailsView(
          networkStore: networkStore,
          model: detailsModel
        )
      }
      .navigationViewStyle(StackNavigationViewStyle())
    }
  }
}

#if DEBUG
struct CustomNetworkListView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      CustomNetworkListView(networkStore: .previewStore)
    }
    NavigationView {
      CustomNetworkListView(networkStore: .previewStoreWithCustomNetworkAdded)
    }
  }
}
#endif
