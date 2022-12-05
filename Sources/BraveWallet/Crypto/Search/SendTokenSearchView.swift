// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import Strings

struct SendTokenSearchView: View {
  @ObservedObject var sendTokenStore: SendTokenStore
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  @State var allERC721Metadata: [String: ERC721Metadata] = [:]
  
  var network: BraveWallet.NetworkInfo
  
  var body: some View {
    TokenList(tokens: sendTokenStore.userAssets) { token in
      Button(action: {
        sendTokenStore.selectedSendTokenERC721Metadata = allERC721Metadata[token.id]
        sendTokenStore.selectedSendToken = token
        presentationMode.dismiss()
      }) {
        TokenView(
          token: token,
          network: network
        ) {
          if token.isErc721 {
            NFTIconView(
              token: token,
              network: network,
              url: allERC721Metadata[token.id]?.imageURL
            )
          } else {
            AssetIconView(
              token: token,
              network: network
            )
          }
        }
      }
    }
    .onAppear {
      Task { @MainActor in
        self.allERC721Metadata = await sendTokenStore.fetchERC721Metadata(tokens: sendTokenStore.userAssets.filter { $0.isErc721 })
      }
    }
    .navigationTitle(Strings.Wallet.searchTitle)
  }
}
