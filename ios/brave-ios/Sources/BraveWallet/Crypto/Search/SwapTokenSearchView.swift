// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Strings
import SwiftUI

struct SwapTokenSearchView: View {
  @ObservedObject var swapTokenStore: SwapTokenStore

  @Environment(\.presentationMode) @Binding private var presentationMode

  enum SwapSearchType {
    case fromToken
    case toToken
  }

  var searchType: SwapSearchType
  var network: BraveWallet.NetworkInfo

  var body: some View {
    let excludedToken =
      searchType == .fromToken ? swapTokenStore.selectedToToken : swapTokenStore.selectedFromToken
    TokenList(
      tokens: swapTokenStore.allTokens
        .filter {
          let symbolNotMatch = $0.symbol != excludedToken?.symbol
          let otherMatch = !$0.isNft || $0.symbol == network.symbol
          return symbolNotMatch && otherMatch
        }
    ) { query, token in
      let symbolMatch = token.symbol.localizedCaseInsensitiveContains(query)
      let nameMatch = token.name.localizedCaseInsensitiveContains(query)
      return symbolMatch || nameMatch
    } header: {
      TokenListHeaderView(title: Strings.Wallet.assetsTitle)
    } content: { token in
      Button {
        if searchType == .fromToken {
          swapTokenStore.selectedFromToken = token
          swapTokenStore.fetchPriceQuote(base: .perSellAsset)
        } else {
          swapTokenStore.selectedToToken = token
          swapTokenStore.fetchPriceQuote(base: .perBuyAsset)
        }
        presentationMode.dismiss()
      } label: {
        TokenView(
          token: token,
          network: network
        ) {
          AssetIconView(
            token: token,
            network: network
          )
        }
      }
    }
    .navigationTitle(Strings.Wallet.searchTitle)
  }
}

#if DEBUG
struct SwapTokenSearchView_Previews: PreviewProvider {
  static var previews: some View {
    SwapTokenSearchView(
      swapTokenStore: .previewStore,
      searchType: .fromToken,
      network: .init()
    )
  }
}
#endif
