// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Strings
import SwiftUI

struct BuyTokenSearchView: View {
  @ObservedObject var buyTokenStore: BuyTokenStore
  let keyringStore: KeyringStore
  let networkStore: NetworkStore

  @State private var isPresentingAddAccount: Bool = false
  @State private var isPresentingAddAccountConfirmation: Bool = false
  @State private var savedSelectedBuyToken: BraveWallet.MeldCryptoCurrency?
  @Environment(\.presentationMode) @Binding private var presentationMode

  var body: some View {
    TokenList(
      tokens: buyTokenStore.supportedCryptoCurrencies
    ) { query, token in
      let symbolMatch = token.currencyCode.localizedCaseInsensitiveContains(query)
      let nameMatch = token.name?.localizedCaseInsensitiveContains(query) ?? false
      return symbolMatch || nameMatch
    } header: {
      TokenListHeaderView(title: Strings.Wallet.assetsTitle)
    } content: { token in
      Button {
        if !buyTokenStore.hasMatchedCoinTypeAccount(for: token) {
          onAccountCreationNeeded(token)
        } else {
          buyTokenStore.selectedBuyToken = token
          presentationMode.dismiss()
        }
      } label: {
        MeldCryptoView(token: token)
      }
    }
    .navigationTitle(Strings.Wallet.searchTitle.capitalized)
    .addAccount(
      keyringStore: keyringStore,
      networkStore: networkStore,
      preselectedAccountCoin: savedSelectedBuyToken?.coin,
      preselectedAccountNetwork: nil,
      isShowingConfirmation: $isPresentingAddAccountConfirmation,
      isShowingAddAccount: $isPresentingAddAccount,
      onConfirmAddAccount: { isPresentingAddAccount = true },
      onCancelAddAccount: { buyTokenStore.handleCancelAddAccount() },
      onAddAccountDismissed: {
        Task {
          if let savedSelectedBuyToken,
             await buyTokenStore.handleDismissAddAccount(
              selectingToken: savedSelectedBuyToken
             ) {
            self.savedSelectedBuyToken = nil
            presentationMode.dismiss()
          }
        }
      }
    )
  }

  private func onAccountCreationNeeded(_ asset: BraveWallet.MeldCryptoCurrency) {
    savedSelectedBuyToken = asset
    isPresentingAddAccountConfirmation = true
  }
}
