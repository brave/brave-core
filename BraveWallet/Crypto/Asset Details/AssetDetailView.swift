/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit
import SwiftUI
import BraveCore
import BraveUI
import struct Shared.Strings

struct AssetDetailView: View {
  @ObservedObject var assetDetailStore: AssetDetailStore
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  
  @State private var tableInset: CGFloat = -16.0
  @State private var isShowingAddAccount: Bool = false
  
  @Environment(\.buySendSwapDestination)
  private var buySendSwapDestination: Binding<BuySendSwapDestination?>
  
  @Environment(\.openWalletURLAction) private var openWalletURL

  var body: some View {
    List {
      Section(
        header: AssetDetailHeaderView(
          assetDetailStore: assetDetailStore,
          keyringStore: keyringStore,
          networkStore: networkStore,
          buySendSwapDestination: buySendSwapDestination
        )
        .resetListHeaderStyle()
        .padding(.horizontal, tableInset) // inset grouped layout margins workaround
      ) {
      }
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.accountsPageTitle))
          .osAvailabilityModifiers { content in
            if #available(iOS 15.0, *) {
              content // padding already applied
            } else {
              content
            }
          },
        footer: Button(action: {
          isShowingAddAccount = true
        }) {
          Text(Strings.Wallet.addAccountTitle)
        }
        .listRowInsets(.zero)
        .buttonStyle(BraveOutlineButtonStyle(size: .small))
        .padding(.vertical, 8)
      ) {
        if assetDetailStore.accounts.isEmpty {
          Text(Strings.Wallet.noAccounts)
            .redacted(reason: assetDetailStore.isLoadingAccountBalances ? .placeholder : [])
            .shimmer(assetDetailStore.isLoadingAccountBalances)
            .font(.footnote)
        } else {
          ForEach(assetDetailStore.accounts) { viewModel in
            HStack {
              AccountView(address: viewModel.account.address, name: viewModel.account.name)
              let showFiatPlaceholder = viewModel.fiatBalance.isEmpty && assetDetailStore.isLoadingPrice
              let showBalancePlaceholder = viewModel.balance.isEmpty && assetDetailStore.isLoadingAccountBalances
              VStack(alignment: .trailing) {
                Text(showFiatPlaceholder ? "$0.00" : viewModel.fiatBalance)
                  .redacted(reason: showFiatPlaceholder ? .placeholder : [])
                  .shimmer(assetDetailStore.isLoadingPrice)
                Text(showBalancePlaceholder ? "0.0000 \(assetDetailStore.token.symbol)" : "\(viewModel.balance) \(assetDetailStore.token.symbol)")
                  .redacted(reason: showBalancePlaceholder ? .placeholder : [])
                  .shimmer(assetDetailStore.isLoadingAccountBalances)
              }
              .font(.footnote)
              .foregroundColor(Color(.secondaryBraveLabel))
            }
          }
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.transactionsTitle))
      ) {
        if assetDetailStore.transactions.isEmpty {
          Text(Strings.Wallet.noTransactions)
            .font(.footnote)
        } else {
          ForEach(assetDetailStore.transactions, id: \.id) { tx in
            TransactionView(
              info: tx,
              keyringStore: keyringStore,
              networkStore: networkStore,
              visibleTokens: [],
              displayAccountCreator: true,
              assetRatios: [assetDetailStore.token.symbol.lowercased(): assetDetailStore.assetPriceValue]
            )
            .contextMenu {
              Button(action: {
                if let baseURL = self.networkStore.selectedChain.blockExplorerUrls.first.map(URL.init(string:)), let url = baseURL?.appendingPathComponent("tx/\(tx.txHash)") {
                  openWalletURL?(url)
                }
              }) {
                Label(Strings.Wallet.viewOnBlockExplorer, systemImage: "arrow.up.forward.square")
              }
            }
          }
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .listStyle(InsetGroupedListStyle())
    .navigationTitle(assetDetailStore.token.name)
    .navigationBarTitleDisplayMode(.inline)
    .onAppear {
      assetDetailStore.update()
    }
    .introspectTableView { tableView in
      tableInset = -tableView.layoutMargins.left
    }
    .sheet(isPresented: $isShowingAddAccount) {
      NavigationView {
        AddAccountView(keyringStore: keyringStore)
      }
      .navigationViewStyle(StackNavigationViewStyle())
    }
  }
}

#if DEBUG
struct CurrencyDetailView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      AssetDetailView(
        assetDetailStore: .previewStore,
        keyringStore: .previewStore,
        networkStore: .previewStore
      )
        .navigationBarTitleDisplayMode(.inline)
    }
    .previewColorSchemes()
  }
}
#endif
