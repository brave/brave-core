// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveUI
import BraveCore
import Shared

struct BuyTokenView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var buyTokenStore: BuyTokenStore
  @State private var amountInput = ""
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  @Environment(\.openWalletURLAction) private var openWalletURL
  
  var body: some View {
    NavigationView {
      Form {
        Section(
          header: AccountPicker(
            keyringStore: keyringStore,
            networkStore: networkStore
          )
            .listRowBackground(Color.clear)
            .resetListHeaderStyle()
            .padding(.top)
            .padding(.bottom, -16) // Get it a bit closer
        ) {
        }
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.buy))
        ) {
          NavigationLink(destination: BuyTokenSearchView(buyTokenStore: buyTokenStore)) {
            HStack {
              if let token = buyTokenStore.selectedBuyToken {
                AssetIconView(token: token, length: 26)
              }
              Text(buyTokenStore.selectedBuyToken?.symbol ?? "BAT")
                .font(.title3.weight(.semibold))
                .foregroundColor(Color(.braveLabel))
            }
            .padding(.vertical, 8)
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
        Section(
          header: WalletListHeaderView(title: Text(Strings.Wallet.enterAmount))
        ) {
          HStack {
            Text("$")
            TextField(Strings.Wallet.amountInCurrency, text: $amountInput)
              .keyboardType(.decimalPad)
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
        Section(
          header: HStack {
            Button(action: {
              buyTokenStore.fetchBuyUrl(account: keyringStore.selectedAccount, amount: amountInput) { urlString in
                guard let urlString = urlString, let url = URL(string: urlString) else {
                  return
                }
                openWalletURL?(url)
              }
            }) {
              Text(Strings.Wallet.buyButtonTitle)
            }
              .buttonStyle(BraveFilledButtonStyle(size: .normal))
              .frame(maxWidth: .infinity)
          }
            .resetListHeaderStyle()
            .listRowBackground(Color(.clear))
        ) {
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      .navigationTitle(Strings.Wallet.buy)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .navigationBarLeading) {
          Button(action: {
            presentationMode.dismiss()
          }) {
            Text(Strings.CancelString)
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
      .onAppear {
        buyTokenStore.fetchBuyTokens()
      }
    }
  }
}

#if DEBUG
struct BuyTokenView_Previews: PreviewProvider {
    static var previews: some View {
      BuyTokenView(
        keyringStore: .previewStoreWithWalletCreated,
        networkStore: .previewStore,
        buyTokenStore: .previewStore
      )
      .previewColorSchemes()
    }
}
#endif

