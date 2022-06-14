// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import struct Shared.Strings

struct AccountListView: View {
  @ObservedObject var keyringStore: KeyringStore
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  @State private var isPresentingAddAccount: Bool = false
  
  var onDismiss: () -> Void
  
  var body: some View {
    NavigationView {
      List {
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.accountsPageTitle))
        ) {
          ForEach(keyringStore.keyring.accountInfos) { account in
            AddressView(address: keyringStore.selectedAccount.address) {
              Button(action: {
                keyringStore.selectedAccount = account
                onDismiss()
              }) {
                AccountView(address: account.address, name: account.name)
              }
            }
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      .listStyle(InsetGroupedListStyle())
      .navigationTitle(Strings.Wallet.selectAccountTitle)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button(action: { onDismiss() }) {
            Text(Strings.cancelButtonTitle)
              .foregroundColor(Color(.braveOrange))
          }
        }
        ToolbarItemGroup(placement: .primaryAction) {
          Button(action: { isPresentingAddAccount = true }) {
            Label(Strings.Wallet.addAccountTitle, systemImage: "plus")
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
      .sheet(isPresented: $isPresentingAddAccount) {
        NavigationView {
          AddAccountView(keyringStore: keyringStore)
        }
        .navigationViewStyle(StackNavigationViewStyle())
      }
    }
    .navigationViewStyle(StackNavigationViewStyle())
  }
}

#if DEBUG
struct AccountListView_Previews: PreviewProvider {
  static var previews: some View {
    AccountListView(keyringStore: {
      let store = KeyringStore.previewStoreWithWalletCreated
      store.addPrimaryAccount("Account 2", completion: nil)
      store.addPrimaryAccount("Account 3", completion: nil)
      return store
    }(), onDismiss: {})
  }
}
#endif
