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
  
  private var selectedNetwork: NetworkPresentation.Network {
    switch store.mode {
    case .select:
      return .network(networkStore.selectedChain)
    case .filter:
      switch networkStore.networkFilter {
      case .allNetworks:
        return .allNetworks
      case let .network(network):
        return .network(network)
      }
    case .formSelection:
      return .network(store.networkSelectionInForm ?? .init())
    }
  }
  
  private var navigationTitle: String {
    switch store.mode {
    case .select: return Strings.Wallet.networkSelectionTitle
    case .filter: return Strings.Wallet.networkFilterTitle
    case .formSelection: return Strings.Wallet.networkSelectionTitle
    }
  }
  
  var body: some View {
    List {
      Section {
        ForEach(store.primaryNetworks) { presentation in
          Button(action: { selectNetwork(presentation.network) }) {
            NetworkRowView(
              presentation: presentation,
              selectedNetwork: selectedNetwork,
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
              selectedNetwork: selectedNetwork
            )
          }
        }
      }, header: {
        WalletListHeaderView(title: Text(Strings.Wallet.networkSelectionSecondaryNetworks))
      })
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
              selectedNetwork: selectedNetwork,
              navigationTitle: navigationTitle,
              selectedNetworkHandler: { network in
                selectNetwork(.network(network))
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
              if await store.handleDismissAddAccount() {
                presentationMode.dismiss()
              }
            }
          }
        }
    )
  }
  
  private func selectNetwork(_ presentation: NetworkPresentation.Network) {
    Task { @MainActor in
      if await store.selectNetwork(presentation) {
        presentationMode.dismiss()
      }
    }
  }
}

private struct NetworkRowView: View {

  var presentation: NetworkPresentation
  var selectedNetwork: NetworkPresentation.Network
  var detailTappedHandler: (() -> Void)?

  @ScaledMetric private var length: CGFloat = 30
  
  init(
    presentation: NetworkPresentation,
    selectedNetwork: NetworkPresentation.Network,
    detailTappedHandler: (() -> Void)? = nil
  ) {
    self.presentation = presentation
    self.selectedNetwork = selectedNetwork
    self.detailTappedHandler = detailTappedHandler
  }
  
  private var isSelected: Bool {
    if presentation.network == selectedNetwork {
      return true
    } else if case .network(let selectedNetwork) = self.selectedNetwork {
      return presentation.subNetworks.contains(selectedNetwork)
    }
    return false
  }

  @ViewBuilder private var checkmark: some View {
    Image(systemName: "checkmark")
      .opacity(isSelected ? 1 : 0)
      .foregroundColor(Color(.braveBlurpleTint))
  }
  
  private var showShortChainName: Bool {
    presentation.isPrimaryNetwork && !presentation.subNetworks.isEmpty
  }
  
  private var networkName: String {
    switch presentation.network {
    case .allNetworks:
      return Strings.Wallet.allNetworksTitle
    case let .network(network):
      return showShortChainName ? network.shortChainName : network.chainName
    }
  }

  var body: some View {
    HStack {
      HStack {
        checkmark
        if case let .network(network) = presentation.network {
          NetworkIcon(network: network)
        }
        VStack(alignment: .leading, spacing: 0) {
          Text(networkName)
          if case .network(let selectedNetwork) = self.selectedNetwork,
             presentation.subNetworks.contains(selectedNetwork) {
            Text(selectedNetwork.chainName)
              .foregroundColor(Color(.secondaryBraveLabel))
              .font(.footnote)
          }
        }
        .frame(minHeight: length) // maintain height for All Networks row w/o icon
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
            networkName)
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
          network: .network(.mockSolana),
          subNetworks: [.mockSolana],
          isPrimaryNetwork: true
        ),
        selectedNetwork: .network(.mockSolana)
      )
      NetworkRowView(
        presentation: .init(
          network: .network(.mockMainnet),
          subNetworks: [.mockMainnet, .mockGoerli, .mockSepolia],
          isPrimaryNetwork: true
        ),
        selectedNetwork: .network(.mockMainnet)
      )
      NetworkRowView(
        presentation: .init(
          network: .network(.mockPolygon),
          subNetworks: [],
          isPrimaryNetwork: false
        ),
        selectedNetwork: .network(.mockMainnet)
      )
    }
    .previewLayout(.sizeThatFits)
  }
}
#endif

// MARK: Detail View

private struct NetworkSelectionDetailView: View {
  
  var networks: [BraveWallet.NetworkInfo]
  var selectedNetwork: NetworkPresentation.Network
  let navigationTitle: String
  var selectedNetworkHandler: (BraveWallet.NetworkInfo) -> Void
  
  var body: some View {
    List {
      ForEach(networks) { network in
        Button(action: { selectedNetworkHandler(network) }) {
          NetworkSelectionDetailRow(
            isSelected: isSelected(network),
            network: network
          )
          .contentShape(Rectangle())
        }
      }
    }
    .listStyle(.insetGrouped)
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationTitle(networks.first?.shortChainName ?? navigationTitle)
    .navigationBarTitleDisplayMode(.inline)
  }
  
  private func isSelected(_ network: BraveWallet.NetworkInfo) -> Bool {
    if case let .network(selectedNetwork) = self.selectedNetwork {
      return network == selectedNetwork
    }
    return false
  }
}

#if DEBUG
struct NetworkSelectionDetailView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      NetworkSelectionDetailView(
        networks: [.mockMainnet, .mockGoerli, .mockSepolia],
        selectedNetwork: .network(.mockMainnet),
        navigationTitle: Strings.Wallet.networkFilterTitle,
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
        network: .mockGoerli
      )
    }
    .previewLayout(.sizeThatFits)
  }
}
#endif
