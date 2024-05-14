// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

struct TransactionsActivityView: View {

  @ObservedObject var store: TransactionsActivityStore
  @ObservedObject var networkStore: NetworkStore

  @State private var isPresentingNetworkFilter = false
  @State private var transactionDetails: TransactionDetailsStore?

  var body: some View {
    TransactionsListView(
      transactionSections: store.transactionSections,
      query: $store.query,
      errorMessage: $store.errorMessage,
      filtersButtonTapped: {
        isPresentingNetworkFilter = true
      },
      transactionTapped: {
        transactionDetails = store.transactionDetailsStore(for: $0)
      },
      transactionFollowUpActionTapped: store.handleTransactionFollowUpAction
    )
    .onAppear {
      store.update()
    }
    .sheet(isPresented: $isPresentingNetworkFilter) {
      NavigationView {
        NetworkFilterView(
          networks: store.networkFilters,
          networkStore: networkStore,
          saveAction: { selectedNetworks in
            store.networkFilters = selectedNetworks
          }
        )
      }
      .navigationViewStyle(.stack)
      .onDisappear {
        networkStore.closeNetworkSelectionStore()
      }
    }
    .sheet(
      isPresented: Binding(
        get: { self.transactionDetails != nil },
        set: {
          if !$0 {
            self.transactionDetails = nil
            self.store.closeTransactionDetailsStore()
          }
        }
      )
    ) {
      if let transactionDetailsStore = self.transactionDetails {
        TransactionDetailsView(
          transactionDetailsStore: transactionDetailsStore,
          networkStore: networkStore
        )
      }
    }
  }
}

#if DEBUG
struct TransactionsActivityView_Previews: PreviewProvider {
  static var previews: some View {
    TransactionsActivityView(
      store: .preview,
      networkStore: .previewStore
    )
  }
}
#endif
