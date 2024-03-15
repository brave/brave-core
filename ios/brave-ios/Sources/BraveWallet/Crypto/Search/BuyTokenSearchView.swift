// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Strings
import SwiftUI

struct BuyTokenSearchView: View {
  @ObservedObject var buyTokenStore: BuyTokenStore
  var network: BraveWallet.NetworkInfo

  @Environment(\.presentationMode) @Binding private var presentationMode

  var body: some View {
    TokenList(
      tokens: buyTokenStore.allTokens
    ) { query, token in
      let symbolMatch = token.symbol.localizedCaseInsensitiveContains(query)
      let nameMatch = token.name.localizedCaseInsensitiveContains(query)
      return symbolMatch || nameMatch
    } header: {
      TokenListHeaderView(title: Strings.Wallet.assetsTitle)
    } content: { token in
      Button {
        buyTokenStore.selectedBuyToken = token
        presentationMode.dismiss()
      } label: {
        TokenView(
          token: token,
          network: network
        ) {
          AssetIconView(token: token, network: network)
        }
      }
    }
    .navigationTitle(Strings.Wallet.searchTitle.capitalized)
  }
}
