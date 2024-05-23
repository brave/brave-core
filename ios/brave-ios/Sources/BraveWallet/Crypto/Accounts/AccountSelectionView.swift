// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import SwiftUI

import struct Shared.Strings

/// Displays all accounts and will update the selected account to the account tapped on.
struct AccountSelectionView: View {
  @ObservedObject var keyringStore: KeyringStore
  var networkStore: NetworkStore
  let onDismiss: () -> Void

  @State private var isPresentingAddAccount: Bool = false
  @Environment(\.presentationMode) @Binding private var presentationMode

  var body: some View {
    AccountSelectionRootView(
      navigationTitle: Strings.Wallet.selectAccountTitle,
      allAccounts: keyringStore.allAccounts,
      selectedAccounts: [keyringStore.selectedAccount],
      showsSelectAllButton: false,
      selectAccount: { selectedAccount in
        keyringStore.selectedAccount = selectedAccount
        onDismiss()
      }
    )
    .navigationTitle(Strings.Wallet.selectAccountTitle)
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      ToolbarItemGroup(placement: .cancellationAction) {
        Button {
          onDismiss()
        } label: {
          Text(Strings.cancelButtonTitle)
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
      ToolbarItemGroup(placement: .primaryAction) {
        Button {
          isPresentingAddAccount = true
        } label: {
          Label(Strings.Wallet.addAccountTitle, systemImage: "plus")
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
    }
    .sheet(isPresented: $isPresentingAddAccount) {
      NavigationView {
        AddAccountView(
          keyringStore: keyringStore,
          networkStore: networkStore
        )
      }
      .navigationViewStyle(.stack)
    }
  }
}

#if DEBUG
struct AccountSelectionView_Previews: PreviewProvider {
  static var previews: some View {
    AccountSelectionView(
      keyringStore: .previewStoreWithWalletCreated,
      networkStore: .previewStore,
      onDismiss: {}
    )
  }
}
#endif
