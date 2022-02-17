// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import Shared

// Modifier workaround for FB9812596 to avoid crashing on iOS 14 on Release builds
@available(iOS 15.0, *)
private struct SwipeActionsViewModifier_FB9812596: ViewModifier {
  var action: () -> Void
  
  func body(content: Content) -> some View {
    content
      .swipeActions(edge: .trailing) {
        Button(role: .destructive, action: action) {
          Label(Strings.Wallet.delete, systemImage: "trash")
        }
      }
  }
}

struct CustomNetworkListView: View {
  @ObservedObject var networkStore: NetworkStore
  @State private var isPresentingNetworkDetails: CustomNetworkModel?
  @Environment(\.presentationMode) @Binding private var presentationMode
  @Environment(\.sizeCategory) private var sizeCategory
  
  private struct CustomNetworkDetails: Identifiable {
    var isEditMode: Bool
    var network: BraveWallet.EthereumChain?
    var id: String {
      "\(isEditMode)"
    }
  }
  
  private func removeNetwork(_ network: BraveWallet.EthereumChain) {
    networkStore.removeCustomNetwork(network) { _ in }
  }
  
  var body: some View {
    List {
      Section {
        let customNetworks = networkStore.ethereumChains.filter({ $0.isCustom })
        ForEach(customNetworks) { network in
          Button(action: {
            isPresentingNetworkDetails = .init()
            isPresentingNetworkDetails?.populateDetails(from: network)
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
          .osAvailabilityModifiers { content in
            if #available(iOS 15.0, *) {
              content
                .modifier(SwipeActionsViewModifier_FB9812596 {
                  withAnimation(.default) {
                    removeNetwork(network)
                  }
                })
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
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .listStyle(.insetGrouped)
    .overlay(Group {
      if networkStore.ethereumChains.filter({ $0.isCustom }).isEmpty {
        Text(Strings.Wallet.noNetworks)
          .font(.headline.weight(.medium))
          .frame(maxWidth: .infinity)
          .multilineTextAlignment(.center)
          .foregroundColor(Color(.secondaryBraveLabel))
          .transition(.opacity)
      }
    })
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
