// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import struct Shared.Strings
import BraveUI

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
        Button(action: { onDismiss() }) {
          Text(Strings.cancelButtonTitle)
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
      ToolbarItemGroup(placement: .primaryAction) {
        Button(action: {
          isPresentingAddAccount = true
        }) {
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
      keyringStore: {
        let store = KeyringStore.previewStoreWithWalletCreated
        store.addPrimaryAccount("Account 2", coin: .eth, chainId: BraveWallet.MainnetChainId, completion: nil)
        store.addPrimaryAccount("Account 3", coin: .eth, chainId: BraveWallet.MainnetChainId, completion: nil)
        return store
      }(),
      networkStore: .previewStore,
      onDismiss: {}
    )
  }
}
#endif
