// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import SwiftUI

import struct Shared.Strings

/// Displays all accounts and allows multiple selection for filtering by accounts.
struct AccountFilterView: View {

  @Binding var accounts: [Selectable<BraveWallet.AccountInfo>]

  @Environment(\.presentationMode) @Binding private var presentationMode

  private var allSelected: Bool {
    accounts.allSatisfy(\.isSelected)
  }

  var body: some View {
    AccountSelectionRootView(
      navigationTitle: Strings.Wallet.selectAccountsTitle,
      allAccounts: accounts.map(\.model),
      selectedAccounts: accounts.filter(\.isSelected).map(\.model),
      showsSelectAllButton: true,
      selectAccount: selectAccount
    )
  }

  private func selectAccount(_ network: BraveWallet.AccountInfo) {
    DispatchQueue.main.async {
      if let index = accounts.firstIndex(
        where: { $0.model.id == network.id && $0.model.coin == network.coin }
      ) {
        accounts[index] = .init(
          isSelected: !accounts[index].isSelected,
          model: accounts[index].model
        )
      }
    }
  }
}
