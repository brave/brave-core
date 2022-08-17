// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import DesignSystem
import BraveCore
import Strings

struct BuyTokenView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var buyTokenStore: BuyTokenStore

  @State private var amountInput = ""

  @Environment(\.openWalletURLAction) private var openWalletURL
  
  var onDismiss: (() -> Void)
  
  private var isBuySupported: Bool {
    WalletConstants.supportedBuyWithWyreNetworkChainIds.contains(networkStore.selectedChainId)
  }

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
          .padding(.bottom, -16)  // Get it a bit closer
        ) {
        }
        if isBuySupported {
          Section(
            header: WalletListHeaderView(title: Text(Strings.Wallet.buy))
          ) {
            NavigationLink(destination: BuyTokenSearchView(buyTokenStore: buyTokenStore, network: networkStore.selectedChain)) {
              HStack {
                if let token = buyTokenStore.selectedBuyToken {
                  AssetIconView(token: token, network: networkStore.selectedChain, length: 26)
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
              TextField(String.localizedStringWithFormat(Strings.Wallet.amountInCurrency, "USD"), text: $amountInput)
                .keyboardType(.decimalPad)
            }
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
          Section(
            header: HStack {
              Button(action: {
                buyTokenStore.fetchBuyUrl(
                  chainId: networkStore.selectedChainId,
                  account: keyringStore.selectedAccount,
                  amount: amountInput
                ) { urlString in
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
      }
      .navigationTitle(Strings.Wallet.buy)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button(action: { onDismiss() }) {
            Text(Strings.cancelButtonTitle)
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
      .overlay(
        Group {
          if !isBuySupported {
            Text(Strings.Wallet.networkNotSupportedForBuyToken)
              .font(.headline.weight(.medium))
              .frame(maxWidth: .infinity)
              .multilineTextAlignment(.center)
              .foregroundColor(Color(.secondaryBraveLabel))
              .transition(.opacity)
          }
        }
      )
      .onAppear {
        buyTokenStore.fetchBuyTokens(network: networkStore.selectedChain)
      }
    }
    .navigationViewStyle(.stack)
  }
}

#if DEBUG
struct BuyTokenView_Previews: PreviewProvider {
  static var previews: some View {
    BuyTokenView(
      keyringStore: .previewStore,
      networkStore: .previewStore,
      buyTokenStore: .previewStore,
      onDismiss: {}
    )
    .previewColorSchemes()
  }
}
#endif
