/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import BraveCore
import DesignSystem
import SwiftUI

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
  
  var body: some View {
    List {
      Section {
        ForEach(store.primaryNetworks) { presentation in
          Button(action: { selectNetwork(presentation.network) }) {
            NetworkRowView(
              presentation: presentation,
              selectedNetwork: networkStore.selectedChain,
              detailTappedHandler: {
                store.detailNetwork = presentation
              }
            )
          }
        }
      }
      Section(content: {
        ForEach(store.secondaryNetworks) { presentation in
          Button(action: { selectNetwork(presentation.network) }) {
            NetworkRowView(
              presentation: presentation,
              selectedNetwork: networkStore.selectedChain
            )
          }
        }
      }, header: {
        WalletListHeaderView(title: Text(Strings.Wallet.networkSelectionSecondaryNetworks))
      })
    }
    .listStyle(.insetGrouped)
    .navigationTitle(Strings.Wallet.networkSelectionTitle)
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      ToolbarItemGroup(placement: .cancellationAction) {
        Button(action: { presentationMode.dismiss() }) {
          Text(Strings.cancelButtonTitle)
            .foregroundColor(Color(.braveOrange))
        }
      }
    }
    .onAppear {
      store.update()
    }
    .background(
      NavigationLink(
        isActive: Binding(
          get: { store.detailNetwork != nil },
          set: { if !$0 { store.detailNetwork = nil } }
        ),
        destination: {
          if let detailNetwork = store.detailNetwork {
            NetworkSelectionDetailView(
              networks: detailNetwork.subNetworks,
              selectedNetwork: networkStore.selectedChain,
              selectedNetworkHandler: { network in
                selectNetwork(network)
              }
            )
          }
        },
        label: { EmptyView() }
      )
    )
    .background(
      Color.clear
        .alert(
          isPresented: $store.isPresentingNextNetworkAlert
        ) {
          Alert(
            title: Text(String.localizedStringWithFormat(Strings.Wallet.createAccountAlertTitle, store.nextNetwork?.shortChainName ?? "")),
            message: Text(Strings.Wallet.createAccountAlertMessage),
            primaryButton: .default(Text(Strings.yes), action: {
              store.handleCreateAccountAlertResponse(shouldCreateAccount: true)
            }),
            secondaryButton: .cancel(Text(Strings.no), action: {
              store.handleCreateAccountAlertResponse(shouldCreateAccount: false)
            })
          )
        }
    )
    .background(
      Color.clear
        .sheet(
          isPresented: $store.isPresentingAddAccount
        ) {
          NavigationView {
            AddAccountView(keyringStore: keyringStore, preSelectedCoin: store.nextNetwork?.coin)
          }
          .navigationViewStyle(.stack)
          .onDisappear {
            Task { @MainActor in
              await store.handleDismissAddAccount()
            }
          }
        }
    )
  }
  
  private func selectNetwork(_ network: BraveWallet.NetworkInfo) {
    Task { @MainActor in
      if await store.selectNetwork(network: network) {
        presentationMode.dismiss()
      }
    }
  }
}

private struct NetworkRowView: View {

  var presentation: NetworkSelectionStore.NetworkPresentation
  var selectedNetwork: BraveWallet.NetworkInfo
  var detailTappedHandler: (() -> Void)?

  @ScaledMetric private var length: CGFloat = 30
  
  init(
    presentation: NetworkSelectionStore.NetworkPresentation,
    selectedNetwork: BraveWallet.NetworkInfo,
    detailTappedHandler: (() -> Void)? = nil
  ) {
    self.presentation = presentation
    self.selectedNetwork = selectedNetwork
    self.detailTappedHandler = detailTappedHandler
  }
  
  private var isSelected: Bool {
    presentation.network == selectedNetwork || presentation.subNetworks.contains(selectedNetwork)
  }

  @ViewBuilder private var checkmark: some View {
    Image(systemName: "checkmark")
      .opacity(isSelected ? 1 : 0)
      .foregroundColor(Color(.braveLabel))
  }
  
  private var showShortChainName: Bool {
    presentation.isPrimaryNetwork && !presentation.subNetworks.isEmpty
  }

  var body: some View {
    HStack {
      HStack {
        checkmark
        NetworkIcon(network: presentation.network)
        VStack(alignment: .leading, spacing: 0) {
          Text(showShortChainName ? presentation.network.shortChainName : presentation.network.chainName)
          if presentation.subNetworks.contains(selectedNetwork) {
            Text(selectedNetwork.chainName)
              .foregroundColor(Color(.secondaryBraveLabel))
              .font(.footnote)
          }
        }
        Spacer()
      }
      .accessibilityElement(children: .combine)
      .accessibilityAddTraits(isSelected ? [.isSelected] : [])
      if !presentation.subNetworks.isEmpty {
        Button(action: { detailTappedHandler?() }) {
          Image(systemName: "chevron.right.circle")
            .foregroundColor(Color(.braveBlurpleTint))
            .contentShape(Rectangle())
        }
        .buttonStyle(.plain)
        .accessibilityLabel(
          Text(String.localizedStringWithFormat(
            Strings.Wallet.networkSelectionTestnetAccessibilityLabel,
            presentation.network.shortChainName)
          )
        )
      }
    }
    .foregroundColor(Color(.braveLabel))
    .padding(.vertical, 4)
    .frame(maxWidth: .infinity, alignment: .leading)
    .contentShape(Rectangle())
    .listRowBackground(Color(.secondaryBraveGroupedBackground))
  }
}

#if DEBUG
struct NetworkRowView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      NetworkRowView(
        presentation: .init(
          network: .mockSolana,
          subNetworks: [.mockSolana],
          isPrimaryNetwork: true
        ),
        selectedNetwork: .mockSolana
      )
      NetworkRowView(
        presentation: .init(
          network: .mockMainnet,
          subNetworks: [.mockMainnet, .mockRinkeby, .mockRopsten],
          isPrimaryNetwork: true
        ),
        selectedNetwork: .mockMainnet
      )
      NetworkRowView(
        presentation: .init(
          network: .mockPolygon,
          subNetworks: [],
          isPrimaryNetwork: false
        ),
        selectedNetwork: .mockMainnet
      )
    }
    .previewLayout(.sizeThatFits)
  }
}
#endif

// MARK: Detail View

private struct NetworkSelectionDetailView: View {
  
  var networks: [BraveWallet.NetworkInfo]
  var selectedNetwork: BraveWallet.NetworkInfo
  var selectedNetworkHandler: (BraveWallet.NetworkInfo) -> Void
  
  var body: some View {
    List {
      ForEach(networks) { network in
        Button(action: { selectedNetworkHandler(network) }) {
          NetworkSelectionDetailRow(
            isSelected: selectedNetwork == network,
            network: network
          )
          .contentShape(Rectangle())
        }
      }
    }
    .listStyle(.insetGrouped)
    .navigationTitle(networks.first?.shortChainName ?? Strings.Wallet.networkSelectionTitle)
    .navigationBarTitleDisplayMode(.inline)
  }
}

#if DEBUG
struct NetworkSelectionDetailView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      NetworkSelectionDetailView(
        networks: [.mockMainnet, .mockRinkeby, .mockRopsten],
        selectedNetwork: .mockMainnet,
        selectedNetworkHandler: { _ in }
      )
    }
  }
}
#endif

private struct NetworkSelectionDetailRow: View {
  
  var isSelected: Bool
  var network: BraveWallet.NetworkInfo
  
  @ScaledMetric private var length: CGFloat = 30
  
  var body: some View {
    HStack {
      NetworkIcon(network: network)
      Text(network.chainName)
      Spacer()
      if isSelected {
        Image(systemName: "checkmark")
      }
    }
    .foregroundColor(Color(.braveLabel))
    .listRowBackground(Color(.secondaryBraveGroupedBackground))
    .accessibilityAddTraits(isSelected ? [.isSelected] : [])
    .accessibilityElement(children: .combine)
  }
}

#if DEBUG
struct NetworkSelectionDetailRow_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      NetworkSelectionDetailRow(
        isSelected: true,
        network: .mockMainnet
      )
      NetworkSelectionDetailRow(
        isSelected: false,
        network: .mockRopsten
      )
    }
    .previewLayout(.sizeThatFits)
  }
}
#endif
