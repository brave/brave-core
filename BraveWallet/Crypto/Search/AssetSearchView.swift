/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveCore
import struct Shared.Strings

private struct TokenView: View {
  var token: BraveWallet.ERCToken
  
  var body: some View {
    HStack(spacing: 8) {
      AssetIconView(token: token)
      VStack(alignment: .leading) {
        Text(token.name)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        Text(token.symbol.uppercased())
          .foregroundColor(Color(.secondaryBraveLabel))
      }
      .font(.footnote)
    }
    .padding(.vertical, 8)
  }
}

struct AssetSearchView: View {
  var walletStore: WalletStore
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  @State private var allTokens: [BraveWallet.ERCToken] = []
  
  var body: some View {
    NavigationView {
      TokenList(tokens: allTokens) { token in
        NavigationLink(
          destination: AssetDetailView(
            assetDetailStore: walletStore.assetDetailStore(for: token),
            keyringStore: walletStore.keyringStore,
            networkStore: walletStore.networkStore
          )
        ) {
          TokenView(token: token)
        }
      }
      .navigationTitle(Strings.Wallet.searchTitle.capitalized)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button(action: {
            presentationMode.dismiss()
          }) {
            Text(Strings.CancelString)
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
    }
    .navigationViewStyle(StackNavigationViewStyle())
    .onAppear {
      walletStore.tokenRegistry.allTokens { tokens in
        self.allTokens = tokens.sorted(by: { $0.symbol < $1.symbol })
      }
    }
  }
}
