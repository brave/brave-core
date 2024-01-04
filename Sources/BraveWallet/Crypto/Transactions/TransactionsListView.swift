/* Copyright 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import BraveCore
import SwiftUI

struct TransactionSection: Equatable, Identifiable {
  var id: Date { date }
  let date: Date
  
  let transactions: [ParsedTransaction]
}

/// List of transactions separated by date with a search bar and filter button.
/// Used in Activity tab, Asset Details (#8137), etc.
struct TransactionsListView: View {
  
  /// All `TransactionSection` items, unfiltered.
  let transactionSections: [TransactionSection]
  /// Query displayed in the search bar above the transactions.
  @Binding var query: String
  /// Whether to display the filter button
  var showFilter: Bool = true
  /// Called when the filters button beside the search bar is tapped/
  let filtersButtonTapped: () -> Void
  /// Called when a transaction is tapped.
  let transactionTapped: (BraveWallet.TransactionInfo) -> Void
  
  /// Returns `transactionSections` filtered using the `filter` value.
  var filteredTransactionSections: [TransactionSection] {
    if query.isEmpty {
      return transactionSections
    }
    return transactionSections.compactMap { transactionSection in
      // check for transactions matching `filter`
      let filteredTransactions = transactionSection.transactions.filter { parsedTransaction in
        parsedTransaction.matches(query)
      }
      if filteredTransactions.isEmpty {
        // don't return section if no transactions
        return nil
      }
      return TransactionSection(date: transactionSection.date, transactions: filteredTransactions)
    }
  }
  
  var body: some View {
    ScrollView {
      LazyVStack(pinnedViews: [.sectionHeaders]) { // pin search bar + filters
        Section(content: {
          LazyVStack { // don't pin date headers
            if filteredTransactionSections.isEmpty {
              emptyState
                .listRowBackground(Color.clear)
            } else {
              ForEach(filteredTransactionSections) { section in
                Section(content: {
                  ForEach(section.transactions, id: \.transaction.id) { parsedTransaction in
                    Button(action: {
                      transactionTapped(parsedTransaction.transaction)
                    }) {
                      TransactionSummaryViewContainer(
                        parsedTransaction: parsedTransaction
                      )
                    }
                    .listRowInsets(.zero)
                  }
                }, header: {
                  Text(section.date, style: .date)
                    .font(.subheadline.weight(.semibold))
                    .foregroundColor(Color(braveSystemName: .textTertiary))
                    .frame(maxWidth: .infinity, alignment: .leading)
                })
              }
            }
          }
          .padding(.horizontal)
        }, header: {
          searchBarAndFiltersContainer
        })
      }
    }
    .background(Color(braveSystemName: .containerBackground))
  }
  
  private var searchBarAndFiltersContainer: some View {
    VStack(spacing: 0) {
      HStack(spacing: 10) {
        SearchBar(text: $query, placeholder: Strings.Wallet.search)
        if showFilter {
          WalletIconButton(braveSystemName: "leo.filter.settings", action: filtersButtonTapped)
            .padding(.trailing, 8)
        }
      }
      .padding(.vertical, 8)
      Divider()
        .padding(.horizontal, 8)
    }
    .padding(.horizontal, 8)
    .frame(maxWidth: .infinity)
    .background(Color(braveSystemName: .containerBackground))
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

private struct SearchBar: UIViewRepresentable {
  
  @Binding var text: String
  var placeholder = ""
  
  func makeUIView(context: Context) -> UISearchBar {
    let searchBar = UISearchBar(frame: .zero)
    searchBar.text = text
    searchBar.placeholder = placeholder
    // remove black divider lines above/below field
    searchBar.searchBarStyle = .minimal
    // don't disable 'Search' when field empty
    searchBar.enablesReturnKeyAutomatically = false
    return searchBar
  }
  
  func updateUIView(_ uiView: UISearchBar, context: Context) {
    uiView.text = text
    uiView.placeholder = placeholder
    uiView.delegate = context.coordinator
  }
  
  func makeCoordinator() -> Coordinator {
    Coordinator(text: $text)
  }
  
  class Coordinator: NSObject, UISearchBarDelegate {
    @Binding var text: String
    
    init(text: Binding<String>) {
      _text = text
    }
    
    func searchBar(_ searchBar: UISearchBar, textDidChange searchText: String) {
      text = searchText
    }
    
    func searchBarSearchButtonClicked(_ searchBar: UISearchBar) {
      // dismiss keyboard when 'Search' / return key tapped
      searchBar.resignFirstResponder()
    }
  }
}

#if DEBUG
struct TransactionsListView_Previews: PreviewProvider {
  @State private static var query: String = ""
  static var previews: some View {
    TransactionsListView(
      transactionSections: [
        .init(
          date: Date(),
          transactions: [
            .previewConfirmedSend,
            .previewConfirmedSwap,
            .previewConfirmedERC20Approve
          ].compactMap { $0 }
        )
      ],
      query: $query,
      filtersButtonTapped: {},
      transactionTapped: { _ in }
    )
  }
}
#endif
