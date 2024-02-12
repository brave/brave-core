// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import struct Shared.Strings
import BraveUI

struct AccountTransactionListView: View {
  @ObservedObject var activityStore: AccountActivityStore
  @ObservedObject var networkStore: NetworkStore
  
  @State private var transactionDetails: TransactionDetailsStore?
  @State private var query: String = ""
  
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
      showFilter: false,
      filtersButtonTapped: { },
      transactionTapped: { transaction in
        self.transactionDetails = activityStore.transactionDetailsStore(for: transaction)
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
