// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import SwiftUI

import struct Shared.Strings

struct AccountSelectionRootView: View {

  let navigationTitle: String
  let allAccounts: [BraveWallet.AccountInfo]
  let selectedAccounts: [BraveWallet.AccountInfo]
  var showsSelectAllButton: Bool
  var selectAccount: (BraveWallet.AccountInfo) -> Void

  init(
    navigationTitle: String,
    allAccounts: [BraveWallet.AccountInfo],
    selectedAccounts: [BraveWallet.AccountInfo],
    showsSelectAllButton: Bool,
    selectAccount: @escaping (BraveWallet.AccountInfo) -> Void
  ) {
    self.navigationTitle = navigationTitle
    self.allAccounts = allAccounts
    self.selectedAccounts = selectedAccounts
    self.showsSelectAllButton = showsSelectAllButton
    self.selectAccount = selectAccount
  }

  var body: some View {
    ScrollView {
      LazyVStack(spacing: 0) {
        SelectAllHeaderView(
          title: Strings.Wallet.accountsPageTitle,
          showsSelectAllButton: showsSelectAllButton,
          allModels: allAccounts,
          selectedModels: selectedAccounts,
          select: selectAccount
        )
        ForEach(allAccounts) { account in
          AccountListRowView(
            account: account,
            isSelected: selectedAccounts.contains(where: { selectedAccount in
              selectedAccount.accountId == account.accountId
            })
          ) {
            selectAccount(account)
          }
        }
      }
    }
    .listBackgroundColor(Color(uiColor: WalletV2Design.containerBackground))
    .navigationTitle(navigationTitle)
    .navigationBarTitleDisplayMode(.inline)
  }
}

private struct AccountListRowView: View {

  var account: BraveWallet.AccountInfo
  var isSelected: Bool
  let didSelect: () -> Void

  init(
    account: BraveWallet.AccountInfo,
    isSelected: Bool,
    didSelect: @escaping () -> Void
  ) {
    self.account = account
    self.isSelected = isSelected
    self.didSelect = didSelect
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
    AddressView(address: account.address) {
      Button(action: didSelect) {
        HStack {
          AccountView(account: account)
          checkmark
        }
        .contentShape(Rectangle())
      }
      .buttonStyle(FadeButtonStyle())
    }
    .accessibilityElement(children: .combine)
    .accessibilityAddTraits(isSelected ? [.isSelected] : [])
    .padding(.horizontal)
    .contentShape(Rectangle())
  }
}
