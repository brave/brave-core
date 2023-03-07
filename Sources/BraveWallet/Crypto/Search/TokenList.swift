// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import Strings
import BraveUI

struct TokenList<Content: View>: View {
  var tokens: [BraveWallet.BlockchainToken]
  var content: (BraveWallet.BlockchainToken) -> Content

  @State private var query = ""

  private var filteredTokens: [BraveWallet.BlockchainToken] {
    let normalizedQuery = query.lowercased()
    if normalizedQuery.isEmpty {
      return tokens
    }
    return tokens.filter {
      $0.symbol.lowercased().contains(normalizedQuery) || $0.name.lowercased().contains(normalizedQuery)
    }
  }

  init(
    tokens: [BraveWallet.BlockchainToken],
    @ViewBuilder content: @escaping (BraveWallet.BlockchainToken) -> Content
  ) {
    self.tokens = tokens
    self.content = content
  }

  var body: some View {
    List {
      Section(
        header: WalletListHeaderView(
          title: Text(Strings.Wallet.assetsTitle)
        )
      ) {
        Group {
          if filteredTokens.isEmpty {
            Text(Strings.Wallet.assetSearchEmpty)
              .font(.footnote)
              .foregroundColor(Color(.secondaryBraveLabel))
              .multilineTextAlignment(.center)
              .frame(maxWidth: .infinity)
          } else {
            ForEach(filteredTokens) { token in
              content(token)
            }
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
    }
    .listStyle(InsetGroupedListStyle())
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .animation(nil, value: query)
    .searchable(
      text: $query,
      placement: .navigationBarDrawer(displayMode: .always)
    )
  }
}

#if DEBUG
struct TokenListView_Previews: PreviewProvider {
  static var previews: some View {
    TokenList(tokens: MockBlockchainRegistry.testTokens) { token in
      Text(token.name)
    }
  }
}
#endif
