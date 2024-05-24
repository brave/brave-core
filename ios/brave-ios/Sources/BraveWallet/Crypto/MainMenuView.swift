// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Preferences
import Strings
import SwiftUI

struct MainMenuView: View {

  let selectedTab: CryptoTab
  @Binding var isShowingSettings: Bool
  @Binding var isShowingBackup: Bool
  @Binding var isShowingAddAccount: Bool
  let keyringStore: KeyringStore

  @ObservedObject private var isShowingBalances = Preferences.Wallet.isShowingBalances
  @ObservedObject private var isShowingGraph = Preferences.Wallet.isShowingGraph
  @ObservedObject private var isShowingNFTs = Preferences.Wallet.isShowingNFTsTab

  @Environment(\.presentationMode) @Binding private var presentationMode
  @Environment(\.openURL) private var openWalletURL

  @ScaledMetric var rowHeight: CGFloat = 52
  @State private var viewHeight: CGFloat = 0

  var body: some View {
    ScrollView {
      LazyVStack(spacing: 0) {
        Button {
          presentationMode.dismiss()
          keyringStore.lock()
        } label: {
          MenuRowView(
            iconBraveSystemName: "leo.lock",
            title: Strings.Wallet.lockWallet
          )
        }
        .frame(height: rowHeight)

        Button {
          isShowingBackup = true
        } label: {
          MenuRowView(
            iconBraveSystemName: "leo.safe",
            title: Strings.Wallet.backUpWallet
          )
        }
        .frame(height: rowHeight)

        Button {
          isShowingSettings = true
          presentationMode.dismiss()
        } label: {
          MenuRowView(
            iconBraveSystemName: "leo.settings",
            title: Strings.Wallet.walletSettings
          )
        }
        .frame(height: rowHeight)

        if selectedTab == .portfolio {
          Divider()
          portfolioSettings
        } else if selectedTab == .accounts {
          Divider()
          accountsMenuItems
        }

        Divider()
        Button {
          openWalletURL(WalletConstants.braveWalletSupportURL)
        } label: {
          MenuRowView(
            iconBraveSystemName: "leo.help.outline",
            title: Strings.Wallet.helpCenter,
            accessoryContent: {
              Image(braveSystemName: "leo.launch")
                .imageScale(.large)
                .foregroundColor(Color(braveSystemName: .buttonBackground))
            }
          )
        }
        .frame(height: rowHeight)
      }
      .padding(.vertical, 8)
      .readSize { size in
        self.viewHeight = size.height
      }
    }
    .background(Color(braveSystemName: .containerBackground))
    .presentationDetents([
      .height(viewHeight)
    ])
  }

  @ViewBuilder private var portfolioSettings: some View {
    MenuRowView(
      iconBraveSystemName: "leo.eye.on",
      title: Strings.Wallet.balances,
      accessoryContent: {
        Toggle(isOn: $isShowingBalances.value) {
          EmptyView()
        }
        .tint(Color(braveSystemName: .buttonBackground))
      }
    )
    .frame(height: rowHeight)

    MenuRowView(
      iconBraveSystemName: "leo.graph",
      title: Strings.Wallet.graph,
      accessoryContent: {
        Toggle(isOn: $isShowingGraph.value) {
          EmptyView()
        }
        .tint(Color(braveSystemName: .buttonBackground))
      }
    )
    .frame(height: rowHeight)

    MenuRowView(
      iconBraveSystemName: "leo.nft",
      title: Strings.Wallet.nftsTab,
      accessoryContent: {
        Toggle(isOn: $isShowingNFTs.value) {
          EmptyView()
        }
        .tint(Color(braveSystemName: .buttonBackground))
      }
    )
    .frame(height: rowHeight)
  }

  @ViewBuilder private var accountsMenuItems: some View {
    Button {
      self.isShowingAddAccount = true
    } label: {
      MenuRowView(
        iconBraveSystemName: "leo.plus.add",
        title: Strings.Wallet.addAccountTitle
      )
    }
    .frame(height: rowHeight)
  }
}

#if DEBUG
struct MainMenuView_Previews: PreviewProvider {
  static var previews: some View {
    Color.white
      .sheet(
        isPresented: .constant(true),
        content: {
          MainMenuView(
            selectedTab: .portfolio,
            isShowingSettings: .constant(false),
            isShowingBackup: .constant(false),
            isShowingAddAccount: .constant(false),
            keyringStore: .previewStoreWithWalletCreated
          )
        }
      )
  }
}
#endif

private struct MenuRowView<AccessoryContent: View>: View {

  let iconBraveSystemName: String
  let title: String
  let accessoryContent: () -> AccessoryContent

  init(
    iconBraveSystemName: String,
    title: String,
    accessoryContent: @escaping () -> AccessoryContent = { EmptyView() }
  ) {
    self.iconBraveSystemName = iconBraveSystemName
    self.title = title
    self.accessoryContent = accessoryContent
  }

  var body: some View {
    HStack(spacing: 12) {
      Image(braveSystemName: iconBraveSystemName)
        .imageScale(.medium)
        .foregroundColor(Color(braveSystemName: .iconDefault))
      Text(title)
        .foregroundColor(Color(braveSystemName: .textPrimary))
      Spacer()
      accessoryContent()
    }
    .font(.callout.weight(.semibold))
    .padding(.horizontal)
  }
}
