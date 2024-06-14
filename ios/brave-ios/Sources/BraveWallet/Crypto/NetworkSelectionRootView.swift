// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Preferences
import SwiftUI

struct NetworkSelectionRootView: View {

  var navigationTitle: String
  var selectedNetworks: [BraveWallet.NetworkInfo]
  var allNetworks: [BraveWallet.NetworkInfo]
  var showsCancelButton: Bool
  var showsSelectAllButton: Bool
  var selectNetwork: (BraveWallet.NetworkInfo) -> Void
  @Environment(\.presentationMode) @Binding private var presentationMode

  init(
    navigationTitle: String,
    selectedNetworks: [BraveWallet.NetworkInfo],
    allNetworks: [BraveWallet.NetworkInfo],
    showsCancelButton: Bool = true,
    showsSelectAllButton: Bool = false,
    selectNetwork: @escaping (BraveWallet.NetworkInfo) -> Void
  ) {
    self.navigationTitle = navigationTitle
    self.selectedNetworks = selectedNetworks
    self.allNetworks = allNetworks
    self.showsCancelButton = showsCancelButton
    self.showsSelectAllButton = showsSelectAllButton
    self.selectNetwork = selectNetwork
  }

  var body: some View {
    ScrollView {
      LazyVStack(spacing: 0) {
        SelectAllHeaderView(
          title: Strings.Wallet.networkSelectionPrimaryNetworks,
          showsSelectAllButton: showsSelectAllButton,
          allModels: allNetworks.primaryNetworks,
          selectedModels: selectedNetworks,
          select: selectNetwork
        )
        ForEach(allNetworks.primaryNetworks) { network in
          Button {
            selectNetwork(network)
          } label: {
            NetworkRowView(
              network: network,
              isSelected: selectedNetworks.contains(network)
            )
          }
          .buttonStyle(FadeButtonStyle())
        }

        DividerLine()
          .padding(.top, 12)

        SelectAllHeaderView(
          title: Strings.Wallet.networkSelectionSecondaryNetworks,
          showsSelectAllButton: showsSelectAllButton,
          allModels: allNetworks.secondaryNetworks,
          selectedModels: selectedNetworks,
          select: selectNetwork
        )
        ForEach(allNetworks.secondaryNetworks) { network in
          Button {
            selectNetwork(network)
          } label: {
            NetworkRowView(
              network: network,
              isSelected: selectedNetworks.contains(network)
            )
          }
          .buttonStyle(FadeButtonStyle())
        }

        if !allNetworks.testNetworks.isEmpty {
          DividerLine()
            .padding(.top, 12)

          SelectAllHeaderView(
            title: Strings.Wallet.networkSelectionTestNetworks,
            showsSelectAllButton: showsSelectAllButton,
            allModels: allNetworks.testNetworks,
            selectedModels: selectedNetworks,
            select: selectNetwork
          )
          ForEach(allNetworks.testNetworks) { network in
            Button {
              selectNetwork(network)
            } label: {
              NetworkRowView(
                network: network,
                isSelected: selectedNetworks.contains(network)
              )
            }
            .buttonStyle(FadeButtonStyle())
          }
        }
      }
    }
    .listBackgroundColor(Color(uiColor: WalletV2Design.containerBackground))
    .navigationTitle(navigationTitle)
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      ToolbarItemGroup(placement: .cancellationAction) {
        if showsCancelButton {
          Button {
            presentationMode.dismiss()
          } label: {
            Text(Strings.cancelButtonTitle)
              .foregroundColor(Color(.braveBlurpleTint))
          }
        }
      }
    }
  }
}

#if DEBUG
struct NetworkSelectionRootView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      NetworkSelectionRootView(
        navigationTitle: Strings.Wallet.selectNetworksTitle,
        selectedNetworks: [.mockMainnet, .mockSolana, .mockPolygon],
        allNetworks: [
          .mockMainnet, .mockSolana,
          .mockPolygon,
          .mockSepolia, .mockSolanaTestnet,
        ],
        selectNetwork: { _ in

        }
      )
    }
  }
}
#endif

private struct NetworkRowView: View {

  var network: BraveWallet.NetworkInfo
  var isSelected: Bool

  @ScaledMetric private var length: CGFloat = 30

  init(
    network: BraveWallet.NetworkInfo,
    isSelected: Bool
  ) {
    self.network = network
    self.isSelected = isSelected
  }

  private var checkmark: some View {
    Image(braveSystemName: "leo.check.normal")
      .resizable()
      .aspectRatio(contentMode: .fit)
      .hidden(isHidden: !isSelected)
      .foregroundColor(Color(.braveBlurpleTint))
      .frame(width: 14, height: 14)
      .transition(.identity)
      .animation(nil, value: isSelected)
  }

  var body: some View {
    HStack {
      NetworkIconView(network: network)
      VStack(alignment: .leading, spacing: 0) {
        Text(network.chainName)
          .font(.body)
      }
      .frame(minHeight: length)  // maintain height for All Networks row w/o icon
      Spacer()
      checkmark
    }
    .accessibilityElement(children: .combine)
    .accessibilityAddTraits(isSelected ? [.isSelected] : [])
    .foregroundColor(Color(.braveLabel))
    .padding(.horizontal)
    .padding(.vertical, 12)
    .contentShape(Rectangle())
  }
}

#if DEBUG
struct NetworkRowView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      NetworkRowView(
        network: .mockSolana,
        isSelected: true
      )
      NetworkRowView(
        network: .mockMainnet,
        isSelected: true
      )
      NetworkRowView(
        network: .mockPolygon,
        isSelected: false
      )
    }
    .previewLayout(.sizeThatFits)
  }
}
#endif
