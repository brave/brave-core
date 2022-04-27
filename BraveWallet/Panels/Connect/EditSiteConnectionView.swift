// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import struct Shared.Strings
import BraveShared
import BraveUI

struct EditSiteConnectionView: View {
  @ObservedObject var keyringStore: KeyringStore
  
  @ScaledMetric private var faviconSize = 48
  
  private func actionTitle(for account: BraveWallet.AccountInfo) -> String {
    // Disconnect - Connected and selected account
    // Connect - Not connected
    // Switch - Connected but not selected account
    if keyringStore.selectedAccount.id == account.id {
      return "Disconnect"
    }
    return "Switch"
  }
  
  var body: some View {
    NavigationView {
      Form {
        Section {
          ForEach(keyringStore.keyring.accountInfos) { account in
            HStack {
              AccountView(address: account.address, name: account.name)
              Spacer()
              Button { } label: {
                Text(actionTitle(for: account))
                  .foregroundColor(Color(.braveBlurpleTint))
                  .font(.footnote.weight(.semibold))
              }
            }
          }
        } header: {
          HStack(spacing: 12) {
            Image(systemName: "globe")
              .frame(width: faviconSize, height: faviconSize)
              .background(Color(.braveDisabled))
              .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
            VStack(alignment: .leading, spacing: 2) {
              Text(verbatim: "https://app.uniswap.org")
                .font(.subheadline.weight(.semibold))
                .foregroundColor(Color(.bravePrimary))
              Text("2 accounts connected")
                .font(.footnote)
                .foregroundColor(Color(.braveLabel))
            }
          }
          .frame(maxWidth: .infinity, alignment: .leading)
          .resetListHeaderStyle()
          .padding(.vertical)
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      .navigationTitle("Connections")
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .confirmationAction) {
          Button {
            
          } label: {
            Text(Strings.done)
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
    }
  }
}

#if DEBUG
struct EditSiteConnectionView_Previews: PreviewProvider {
  static var previews: some View {
    EditSiteConnectionView(
      keyringStore: {
        let store = KeyringStore.previewStoreWithWalletCreated
        store.addPrimaryAccount("Account 2", completion: nil)
        store.addPrimaryAccount("Account 3", completion: nil)
        return store
      }()
    )
  }
}
#endif
