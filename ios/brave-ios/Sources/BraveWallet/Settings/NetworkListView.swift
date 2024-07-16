// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import Preferences
import Strings
import SwiftUI

struct NetworkListView: View {
  @ObservedObject var networkStore: NetworkStore
  @State private var isPresentingNetworkDetails: NetworkModel?
  @Environment(\.presentationMode) @Binding private var presentationMode
  @Environment(\.sizeCategory) private var sizeCategory

  private struct CustomNetworkDetails: Identifiable {
    var isEditMode: Bool
    var network: BraveWallet.NetworkInfo?
    var id: String {
      "\(isEditMode)"
    }
  }

  private func removeNetwork(_ network: BraveWallet.NetworkInfo) {
    Task { @MainActor in
      await networkStore.removeCustomNetwork(network)
    }
  }

  private var customNetworks: [BraveWallet.NetworkInfo] {
    networkStore.customChains
  }

  private var allNetworks: [BraveWallet.NetworkInfo] {
    networkStore.allChains.filter {
      if $0.chainId == BraveWallet.BitcoinTestnet {
        return Preferences.Wallet.isBitcoinTestnetEnabled.value
      }
      return true
    }
  }

  @ViewBuilder func networkRow(_ network: BraveWallet.NetworkInfo) -> some View {
    Button {
      isPresentingNetworkDetails = .init(
        mode: .edit(network)
      )
    } label: {
      HStack {
        VStack(alignment: .leading, spacing: 4) {
          Text(network.chainName)
            .foregroundColor(Color(.braveLabel))
            .font(.callout.weight(isDefaultNetwork(network) ? .bold : .regular))
          Group {
            Text(network.chainId)
            if let rpcEndpoint = network.rpcEndpoints[
              safe: Int(network.activeRpcEndpointIndex)
            ]?.absoluteString {
              Text(rpcEndpoint)
            }
          }
          .foregroundColor(Color(.secondaryBraveLabel))
          .font(.footnote)
        }
        Spacer()
        HStack {
          Button {
            if networkStore.isHiddenChain(network) {
              Task { @MainActor in
                await networkStore.removeHiddenNetwork(
                  coin: network.coin,
                  for: network.chainId
                )
              }
            } else {
              Task { @MainActor in
                await networkStore.addHiddenNetwork(
                  coin: network.coin,
                  for: network.chainId
                )
              }
            }
          } label: {
            Image(
              braveSystemName: networkStore.isHiddenChain(network)
                ? "leo.eye.off" : "leo.eye.on"
            )
            .font(.callout.weight(.semibold))
            .foregroundColor(
              isDefaultNetwork(network)
                ? Color(.braveDisabled) : Color(.braveLabel)
            )
          }
          .disabled(isDefaultNetwork(network))
          Rectangle()
            .fill(
              Color(uiColor: WalletV2Design.dividerSubtle)
            )
            .frame(width: 1)
            .padding(.vertical, 16)
          Menu {
            VStack {
              Button {
                isPresentingNetworkDetails = .init(
                  mode: .edit(network)
                )
              } label: {
                Text(Strings.Wallet.editButtonTitle)
              }
              if networkStore.isCustomChain(network)
                && !isDefaultNetwork(network)
              {
                Button {
                  removeNetwork(network)
                } label: {
                  Text(Strings.Wallet.delete)
                }
              }
              if !isDefaultNetwork(network) {
                Button {
                  Task { @MainActor in
                    await networkStore.updateDefaultNetwork(network)
                  }
                } label: {
                  Text(Strings.Wallet.setDefaultNetwork)
                }
              }
            }
            .foregroundColor(Color(.braveLabel))
          } label: {
            Image(
              braveSystemName: "leo.more.vertical"
            )
            .font(.callout.weight(.semibold))
            .foregroundColor(Color(.braveLabel))
          }
        }
      }
    }
  }

  var body: some View {
    List {
      ForEach(WalletConstants.supportedCoinTypes()) { coin in
        Section {
          let networks = allNetworks.filter { $0.coin == coin }
          ForEach(networks) { network in
            networkRow(network)
              .padding(.vertical, 6)
              .listRowBackground(
                Color(.secondaryBraveGroupedBackground)
              )
          }
        } header: {
          Text(coin.localizedTitle)
        }
      }
    }
    .listStyle(.insetGrouped)
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationTitle(Strings.Wallet.transactionDetailsNetworkTitle)
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      ToolbarItemGroup(placement: .confirmationAction) {
        Button {
          isPresentingNetworkDetails = .init()
        } label: {
          Label(Strings.Wallet.addCustomNetworkBarItemTitle, systemImage: "plus")
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
    }
    .sheet(item: $isPresentingNetworkDetails) { detailsModel in
      NavigationView {
        NetworkDetailsView(
          networkStore: networkStore,
          model: detailsModel
        )
      }
      .navigationViewStyle(StackNavigationViewStyle())
    }
    .task {
      await networkStore.updateChainList()
    }
  }

  private func isDefaultNetwork(
    _ network: BraveWallet.NetworkInfo
  ) -> Bool {
    guard let defaultNetwork = networkStore.defaultNetworks[network.coin]
    else {
      return false
    }
    return defaultNetwork.chainId == network.chainId
  }
}

#if DEBUG
struct CustomNetworkListView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      NetworkListView(networkStore: .previewStore)
    }
    NavigationView {
      NetworkListView(networkStore: .previewStoreWithCustomNetworkAdded)
    }
  }
}
#endif
