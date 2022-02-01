/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveCore
import struct Shared.Strings

struct AssetSearchView: View {
  var keyringStore: KeyringStore
  var cryptoStore: CryptoStore
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  @State private var allTokens: [BraveWallet.BlockchainToken] = []
  
  var body: some View {
    NavigationView {
      TokenList(tokens: allTokens.filter({ $0.isErc20 || $0.symbol == cryptoStore.networkStore.selectedChain.symbol })) { token in
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
          TokenView(token: token)
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
      cryptoStore.blockchainRegistry.allTokens(BraveWallet.MainnetChainId) { tokens in
        self.allTokens = tokens.sorted(by: { $0.symbol < $1.symbol })
      }
    }
  }
}
