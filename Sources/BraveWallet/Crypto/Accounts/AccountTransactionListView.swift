// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import struct Shared.Strings
import BraveUI

struct AccountTransactionListView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var activityStore: AccountActivityStore
  @ObservedObject var networkStore: NetworkStore
  
  @Environment(\.openURL) private var openWalletURL
  
  @State private var transactionDetails: TransactionDetailsStore?
  
  private func emptyTextView(_ message: String) -> some View {
    Text(message)
      .font(.footnote.weight(.medium))
      .frame(maxWidth: .infinity)
      .multilineTextAlignment(.center)
      .foregroundColor(Color(.secondaryBraveLabel))
  }
  
  var body: some View {
    List {
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.transactionsTitle))
      ) {
        Group {
          if activityStore.transactionSummaries.isEmpty {
            emptyTextView(Strings.Wallet.noTransactions)
          } else {
            ForEach(activityStore.transactionSummaries) { txSummary in
              Button(action: {
                self.transactionDetails = activityStore.transactionDetailsStore(for: txSummary.txInfo)
              }) {
                TransactionSummaryView(summary: txSummary)
              }
              .contextMenu {
                if !txSummary.txHash.isEmpty {
                  Button(action: {
                    if let txNetwork = self.networkStore.allChains.first(where: { $0.chainId == txSummary.txInfo.chainId }),
                       let url = txNetwork.txBlockExplorerLink(txHash: txSummary.txHash, for: txNetwork.coin) {
                      openWalletURL(url)
                    }
                  }) {
                    Label(Strings.Wallet.viewOnBlockExplorer, systemImage: "arrow.up.forward.square")
                  }
                }
              }
            }
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
    }
    .listStyle(InsetGroupedListStyle())
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationTitle(Strings.Wallet.transactionsTitle)
    .navigationBarTitleDisplayMode(.inline)
    .sheet(
      isPresented: Binding(
        get: { self.transactionDetails != nil },
        set: { if !$0 { self.transactionDetails = nil } }
      )
    ) {
      if let transactionDetailsStore = transactionDetails {
        TransactionDetailsView(
          transactionDetailsStore: transactionDetailsStore,
          networkStore: networkStore
        )
      }
    }
  }
}

#if DEBUG
struct AccountTransactionListView_Previews: PreviewProvider {
  static var previews: some View {
    AccountTransactionListView(
      keyringStore: .previewStore,
      activityStore: {
        let store: AccountActivityStore = .previewStore
        store.previewTransactions()
        return store
      }(),
      networkStore: .previewStore
    )
  }
}
#endif
