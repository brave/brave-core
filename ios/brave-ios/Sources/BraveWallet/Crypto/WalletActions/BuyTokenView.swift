// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import Strings
import SwiftUI

struct BuyTokenView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var buyTokenStore: BuyTokenStore

  @State private var isShowingProviderSelection = false

  var onDismiss: (() -> Void)

  var body: some View {
    NavigationView {
      Form {
        Section {
          AccountPicker(
            keyringStore: keyringStore,
            networkStore: networkStore
          )
          .listRowBackground(Color(UIColor.braveGroupedBackground))
          .resetListHeaderStyle()
        }
        if buyTokenStore.isSelectedNetworkSupported {
          Section(
            header: WalletListHeaderView(title: Text(Strings.Wallet.buy))
          ) {
            NavigationLink(
              destination: BuyTokenSearchView(
                buyTokenStore: buyTokenStore,
                network: networkStore.defaultSelectedChain
              )
            ) {
              HStack {
                if let token = buyTokenStore.selectedBuyToken {
                  AssetIconView(
                    token: token,
                    network: networkStore.defaultSelectedChain,
                    length: 26
                  )
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
              Menu {
                Picker(
                  selection: $buyTokenStore.selectedCurrency,
                  content: {
                    ForEach(buyTokenStore.supportedCurrencies) { currency in
                      Text(currency.currencyCode)
                        .foregroundColor(Color(.secondaryBraveLabel))
                        .tag(currency)
                    }
                  },
                  label: { EmptyView() }  // `Menu` label is used instead
                )
              } label: {
                HStack(spacing: 4) {
                  Text(buyTokenStore.selectedCurrency.symbol)
                    .font(.title2.weight(.bold))
                    .foregroundColor(Color(.braveLabel))
                  Image(systemName: "chevron.down")
                    .imageScale(.small)
                    .foregroundColor(Color(.secondaryBraveLabel))
                }
                .padding(.vertical, 4)
              }
              TextField("0", text: $buyTokenStore.buyAmount)
                .keyboardType(.decimalPad)
            }
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }
          Section(
            header: HStack {
              Button {
                isShowingProviderSelection = true
              } label: {
                Text(Strings.Wallet.purchaseMethodButtonTitle)
              }
              .buttonStyle(BraveFilledButtonStyle(size: .normal))
              .frame(maxWidth: .infinity)
            }
            .resetListHeaderStyle()
            .listRowBackground(Color(.clear))
          ) {
          }
        }
      }
      .environment(\.defaultMinListHeaderHeight, 0)
      .environment(\.defaultMinListRowHeight, 0)
      .listBackgroundColor(Color(UIColor.braveGroupedBackground))
      .navigationTitle(Strings.Wallet.buy)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button {
            onDismiss()
          } label: {
            Text(Strings.cancelButtonTitle)
              .foregroundColor(Color(.braveBlurpleTint))
          }
        }
      }
      .overlay(
        Group {
          if !buyTokenStore.isSelectedNetworkSupported {
            Text(Strings.Wallet.networkNotSupportedForBuyToken)
              .font(.headline.weight(.medium))
              .frame(maxWidth: .infinity)
              .multilineTextAlignment(.center)
              .foregroundColor(Color(.secondaryBraveLabel))
              .transition(.opacity)
          }
        }
      )
      .background(
        NavigationLink(
          isActive: $isShowingProviderSelection,
          destination: {
            BuyProviderSelectionView(
              buyTokenStore: buyTokenStore,
              keyringStore: keyringStore
            )
          },
          label: {
            EmptyView()
          }
        )
      )
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
