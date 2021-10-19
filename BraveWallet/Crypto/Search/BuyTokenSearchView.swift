// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

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

struct BuyTokenSearchView: View {
  @ObservedObject var buyTokenStore: BuyTokenStore
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  var body: some View {
    TokenList(tokens: buyTokenStore.buyTokens) { token in
      Button(action: {
        buyTokenStore.selectedBuyToken = token
        presentationMode.dismiss()
      }) {
        TokenView(token: token)
      }
    }
    .navigationTitle(Strings.Wallet.searchTitle)
  }
}

