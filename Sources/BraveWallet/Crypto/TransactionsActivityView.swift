/* Copyright 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import BraveCore
import SwiftUI

struct TransactionsActivityView: View {
  
  @ObservedObject var store: TransactionsActivityStore
  @ObservedObject var networkStore: NetworkStore
  
  @State private var isPresentingNetworkFilter = false
  @State private var transactionDetails: TransactionDetailsStore?
  
  private var networkFilterButton: some View {
    Button(action: {
      self.isPresentingNetworkFilter = true
    }) {
      Image(braveSystemName: "leo.tune")
        .font(.footnote.weight(.medium))
        .foregroundColor(Color(.braveBlurpleTint))
        .clipShape(Rectangle())
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
  }
  
  var body: some View {
    List {
      Section {
        if store.transactionSummaries.isEmpty {
          emptyState
            .listRowBackground(Color.clear)
        } else {
          Group {
            ForEach(store.transactionSummaries) { txSummary in
              Button(action: {
                self.transactionDetails = store.transactionDetailsStore(for: txSummary.txInfo)
              }) {
                TransactionSummaryView(summary: txSummary)
              }
            }
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        }
      } header: {
        HStack {
          Text(Strings.Wallet.assetsTitle)
          Spacer()
          networkFilterButton
        }
        .textCase(nil)
        .padding(.horizontal, -8)
        .frame(maxWidth: .infinity, alignment: .leading)
      }
    }
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .onAppear {
      store.update()
    }
    .sheet(
      isPresented: Binding(
        get: { self.transactionDetails != nil },
        set: { if !$0 { self.transactionDetails = nil } }
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
  
  private var emptyState: some View {
    VStack(alignment: .center, spacing: 10) {
      Text(Strings.Wallet.activityPageEmptyTitle)
        .font(.headline.weight(.semibold))
        .foregroundColor(Color(.braveLabel))
      Text(Strings.Wallet.activityPageEmptyDescription)
        .font(.subheadline.weight(.semibold))
        .foregroundColor(Color(.secondaryLabel))
    }
    .multilineTextAlignment(.center)
    .frame(maxWidth: .infinity)
    .padding(.vertical, 60)
    .padding(.horizontal, 32)
  }
}

#if DEBUG
struct TransactionsActivityViewView_Previews: PreviewProvider {
  static var previews: some View {
    TransactionsActivityView(
      store: .preview,
      networkStore: .previewStore
    )
  }
}
#endif
