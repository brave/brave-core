/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveCore
import Strings

struct AssetSearchView: View {
  var keyringStore: KeyringStore
  var cryptoStore: CryptoStore
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  @State private var allTokens: [BraveWallet.BlockchainToken] = []
  
  var body: some View {
    NavigationView {
      TokenList(tokens: allTokens.filter { !$0.isErc721 || cryptoStore.networkStore.selectedChain.isNativeAsset($0) }) { token in
        NavigationLink(
          destination: AssetDetailView(
            assetDetailStore: cryptoStore.assetDetailStore(for: token),
            keyringStore: keyringStore,
            networkStore: cryptoStore.networkStore
          )
            .onDisappear {
              cryptoStore.closeAssetDetailStore(for: token)
            }
        ) {
          TokenView(token: token, network: cryptoStore.networkStore.selectedChain)
        }
      }
      .navigationTitle(Strings.Wallet.searchTitle.capitalized)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button(action: {
            presentationMode.dismiss()
          }) {
            Text(Strings.cancelButtonTitle)
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
    }
    .navigationViewStyle(StackNavigationViewStyle())
    .onAppear {
      cryptoStore.blockchainRegistry.allTokens(
        cryptoStore.networkStore.selectedChainId,
        coin: cryptoStore.networkStore.selectedChain.coin
      ) { tokens in
        self.allTokens = ([cryptoStore.networkStore.selectedChain.nativeToken] + tokens).sorted(by: { $0.symbol < $1.symbol })
      }
    }
  }
}
