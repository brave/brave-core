// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import Strings

struct BuyTokenSearchView: View {
  @ObservedObject var buyTokenStore: BuyTokenStore
  var network: BraveWallet.NetworkInfo

  @Environment(\.presentationMode) @Binding private var presentationMode

  var body: some View {
    TokenList(tokens: buyTokenStore.allTokens) { token in
      Button(action: {
        buyTokenStore.selectedBuyToken = token
        presentationMode.dismiss()
      }) {
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
