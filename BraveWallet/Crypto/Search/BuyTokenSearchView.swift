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
  
  @State private var allTokens: [BraveWallet.ERCToken] = []
  @State private var query = ""
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  private var tokens: [BraveWallet.ERCToken] {
    let query = query.lowercased()
    if query.isEmpty {
      return buyTokenStore.buyTokens
    }
    return buyTokenStore.buyTokens.filter {
      $0.symbol.lowercased().contains(query) ||
      $0.name.lowercased().contains(query)
    }
  }
  
  var body: some View {
    TokenList(tokens: tokens) { token in
      Button(action: {
        buyTokenStore.selectedBuyToken = token
        presentationMode.dismiss()
      }) {
        TokenView(token: token)
      }
    }
    .navigationTitle(Strings.Wallet.searchTitle)
    .animation(nil, value: query)
    .filterable(text: $query)
  }
}

