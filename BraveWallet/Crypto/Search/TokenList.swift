// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import struct Shared.Strings

struct TokenList<Content: View>: View {
  var tokens: [BraveWallet.ERCToken]
  var content: (BraveWallet.ERCToken) -> Content
  
  @State private var query = ""
  
  private var filteredTokens: [BraveWallet.ERCToken] {
    let query = query.lowercased()
    if query.isEmpty {
      return tokens
    }
    return tokens.filter {
      $0.symbol.lowercased().contains(query) ||
      $0.name.lowercased().contains(query)
    }
  }
  
  init(
    tokens: [BraveWallet.ERCToken],
    @ViewBuilder content: @escaping (BraveWallet.ERCToken) -> Content
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
        .osAvailabilityModifiers { content in
          if #available(iOS 15.0, *) {
            content // Padding already applied
          } else {
            content
              .padding(.top)
          }
        }
      ) {
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
    .listStyle(InsetGroupedListStyle())
    .animation(nil, value: query)
    .filterable(text: $query)
  }
}

#if DEBUG
struct TokenListView_Previews: PreviewProvider {
  static var previews: some View {
    TokenList(tokens: TestTokenRegistry.testTokens) { token in
      Text(token.name)
    }
  }
}
#endif

