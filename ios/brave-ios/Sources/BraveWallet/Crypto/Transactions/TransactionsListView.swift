// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import SwiftUI

struct TransactionSection: Equatable, Identifiable {
  var id: Date { date }
  let date: Date

  let transactions: [ParsedTransaction]
}

enum TransactionFollowUpAction {
  case retry
  case cancel
  case speedUp

  var buttonTitle: String {
    switch self {
    case .retry:
      return Strings.Wallet.retryTransactionButtonTitle
    case .cancel:
      return Strings.Wallet.cancelTransactionButtonTitle
    case .speedUp:
      return Strings.Wallet.speedUpTransactionButtonTitle
    }
  }

  var braveSystemImage: String {
    switch self {
    case .retry:
      return "leo.refresh"
    case .cancel:
      return "leo.close"
    case .speedUp:
      return "leo.network.speed-fast"
    }
  }
}

/// List of transactions separated by date with a search bar and filter button.
/// Used in Activity tab, Asset Details (#8137), etc.
struct TransactionsListView: View {

  /// All `TransactionSection` items, unfiltered.
  let transactionSections: [TransactionSection]
  /// Query displayed in the search bar above the transactions.
  @Binding var query: String
  /// Error message to display in an alert
  @Binding var errorMessage: String?
  /// Whether to display the filter button
  var showFilter: Bool = true
  /// Called when the filters button beside the search bar is tapped/
  let filtersButtonTapped: () -> Void
  /// Called when a transaction is tapped.
  let transactionTapped: (BraveWallet.TransactionInfo) -> Void
  /// Called when retry transaction is tapped from context menu.
  let transactionFollowUpActionTapped:
    (TransactionFollowUpAction, BraveWallet.TransactionInfo) -> Void

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
      LazyVStack(pinnedViews: [.sectionHeaders]) {  // pin search bar + filters
        Section(
          content: {
            LazyVStack {  // don't pin date headers
              if filteredTransactionSections.isEmpty {
                emptyState
                  .listRowBackground(Color.clear)
              } else {
                ForEach(filteredTransactionSections) { section in
                  Section(
                    content: {
                      ForEach(section.transactions, id: \.transaction.id) { parsedTransaction in
                        Button {
                          transactionTapped(parsedTransaction.transaction)
                        } label: {
                          TransactionSummaryViewContainer(
                            parsedTransaction: parsedTransaction
                          )
                        }
                        .contextMenu(menuItems: {
                          if parsedTransaction.transaction.isRetriable {
                            Button {
                              transactionFollowUpActionTapped(.retry, parsedTransaction.transaction)
                            } label: {
                              Label(
                                Strings.Wallet.retryTransactionButtonTitle,
                                braveSystemImage: "leo.refresh"
                              )
                            }
                          }
                          if parsedTransaction.transaction.isCancelOrSpeedUpTransactionSupported {
                            Button {
                              transactionFollowUpActionTapped(
                                .cancel,
                                parsedTransaction.transaction
                              )
                            } label: {
                              Label(
                                TransactionFollowUpAction.cancel.buttonTitle,
                                braveSystemImage: TransactionFollowUpAction.cancel.braveSystemImage
                              )
                            }
                            Button {
                              transactionFollowUpActionTapped(
                                .speedUp,
                                parsedTransaction.transaction
                              )
                            } label: {
                              Label(
                                TransactionFollowUpAction.speedUp.buttonTitle,
                                braveSystemImage: TransactionFollowUpAction.speedUp.braveSystemImage
                              )
                            }
                          }
                        })
                      }
                    },
                    header: {
                      Text(section.date, style: .date)
                        .font(.subheadline.weight(.semibold))
                        .foregroundColor(Color(braveSystemName: .textTertiary))
                        .frame(maxWidth: .infinity, alignment: .leading)
                    }
                  )
                }
              }
            }
            .padding(.horizontal)
          },
          header: {
            searchBarAndFiltersContainer
          }
        )
      }
    }
    .background(Color(braveSystemName: .containerBackground))
    .errorAlert(errorMessage: $errorMessage)
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
            .previewConfirmedERC20Approve,
          ].compactMap { $0 }
        )
      ],
      query: $query,
      errorMessage: .constant(nil),
      filtersButtonTapped: {},
      transactionTapped: { _ in },
      transactionFollowUpActionTapped: { _, _ in }
    )
  }
}
#endif
