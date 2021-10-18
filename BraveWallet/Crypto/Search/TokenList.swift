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
        let tokens = self.tokens
        if tokens.isEmpty {
          Text(Strings.Wallet.assetSearchEmpty)
            .font(.footnote)
            .foregroundColor(Color(.secondaryBraveLabel))
            .multilineTextAlignment(.center)
            .frame(maxWidth: .infinity)
        } else {
          ForEach(tokens) { token in
            content(token)
          }
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .listStyle(InsetGroupedListStyle())
  }
}

#if DEBUG
struct TokenListView_Previews: PreviewProvider {
  static let testTokens: [BraveWallet.ERCToken] = [
    .init(contractAddress: "0x0d8775f648430679a709e98d2b0cb6250d2887ef", name: "Basic Attention Token", logo: "", isErc20: true, isErc721: false, symbol: "BAT", decimals: 18, visible: true),
    .init(contractAddress: "0xB8c77482e45F1F44dE1745F52C74426C631bDD52", name: "BNB", logo: "", isErc20: true, isErc721: false, symbol: "BNB", decimals: 18, visible: true),
    .init(contractAddress: "0xdac17f958d2ee523a2206206994597c13d831ec7", name: "Tether USD", logo: "", isErc20: true, isErc721: false, symbol: "USDT", decimals: 6, visible: true),
    .init(contractAddress: "0x57f1887a8bf19b14fc0df6fd9b2acc9af147ea85", name: "Ethereum Name Service", logo: "", isErc20: false, isErc721: true, symbol: "ENS", decimals: 1, visible: true)
  ]
  
  static var previews: some View {
    TokenList(tokens: testTokens) { token in
      Text(token.name)
    }
  }
}
#endif

