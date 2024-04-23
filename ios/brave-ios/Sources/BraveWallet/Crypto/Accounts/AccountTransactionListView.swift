// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import SwiftUI

import struct Shared.Strings

struct AccountTransactionListView: View {
  @ObservedObject var activityStore: AccountActivityStore
  @ObservedObject var networkStore: NetworkStore

  @State private var transactionDetails: TransactionDetailsStore?
  @State private var query: String = ""
  @State private var errorMessage: String?
  @Environment(\.dismiss) private var dismiss

  private func emptyTextView(_ message: String) -> some View {
    Text(message)
      .font(.footnote.weight(.medium))
      .frame(maxWidth: .infinity)
      .multilineTextAlignment(.center)
      .foregroundColor(Color(.secondaryBraveLabel))
  }

  var body: some View {
    TransactionsListView(
      transactionSections: activityStore.transactionSections,
      query: $query,
      errorMessage: $errorMessage,
      showFilter: false,
      filtersButtonTapped: {},
      transactionTapped: { transaction in
        self.transactionDetails = activityStore.transactionDetailsStore(for: transaction)
      },
      transactionFollowUpActionTapped: { action, transaction in
        Task { @MainActor in
          guard
            let errorMessage = await activityStore.handleTransactionFollowUpAction(
              action,
              transaction: transaction
            )
          else {
            // If we're presenting tx history from wallet panel we need
            // to dismiss tx history to present the new pending request
            if activityStore.isWalletPanel {
              dismiss()
            }
            return
          }
          self.errorMessage = errorMessage
        }
      }
    )
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
      activityStore: {
        let store: AccountActivityStore = .previewStore
        return store
      }(),
      networkStore: .previewStore
    )
  }
}
#endif
