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
  
  @State var allNFTMetadata: [String: NFTMetadata] = [:]
  
  var network: BraveWallet.NetworkInfo
  
  var body: some View {
    TokenList(tokens: sendTokenStore.userVisibleAssets) { token in
      Button(action: {
        sendTokenStore.selectedSendNFTMetadata = allNFTMetadata[token.id]
        sendTokenStore.selectedSendToken = token
        presentationMode.dismiss()
      }) {
        TokenView(
          token: token,
          network: network
        ) {
          if token.isErc721 || token.isNft {
            NFTIconView(
              token: token,
              network: network,
              url: allNFTMetadata[token.id]?.imageURL
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
        self.allNFTMetadata = await sendTokenStore.fetchNFTMetadata(tokens: sendTokenStore.userVisibleAssets.filter { $0.isErc721 || $0.isNft })
      }
    }
    .navigationTitle(Strings.Wallet.searchTitle)
  }
}
