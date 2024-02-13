// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import BigNumber
import Strings
import DesignSystem

struct TransactionConfirmationView: View {

  @ObservedObject var confirmationStore: TransactionConfirmationStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var keyringStore: KeyringStore

  var onDismiss: () -> Void
  
  @State private var isShowingGas: Bool = false
  @State private var isShowingAdvancedSettings: Bool = false
  @State private var transactionDetails: TransactionDetailsStore?
  
  private var transactionType: String {
    if confirmationStore.activeParsedTransaction.transaction.txType == .erc20Approve {
      return Strings.Wallet.transactionTypeApprove
    }
    switch confirmationStore.activeParsedTransaction.transaction.txType {
    case .erc20Approve:
      return Strings.Wallet.transactionTypeApprove
    case .solanaDappSignTransaction, .solanaDappSignAndSendTransaction:
      return Strings.Wallet.solanaDappTransactionTitle
    default:
      return confirmationStore.activeParsedTransaction.transaction.isSwap ? Strings.Wallet.swap : Strings.Wallet.send
    }
  }
  
  private var navigationTitle: String {
    if confirmationStore.isTxSubmitting || confirmationStore.activeTxStatus == .signed || confirmationStore.activeTxStatus == .submitted || confirmationStore.activeTxStatus == .confirmed || confirmationStore.activeTxStatus == .error {
      return "\(transactionType) \(confirmationStore.value) \(confirmationStore.symbol)"
    } else {
      if confirmationStore.activeParsedTransaction.transaction.txType == .ethSwap {
        return Strings.Wallet.swapConfirmationTitle
      }
      return confirmationStore.unapprovedTxs.count > 1 ? Strings.Wallet.confirmTransactionsTitle : Strings.Wallet.confirmTransactionTitle
    }
  }
  
  @ViewBuilder private var containerView: some View {
    PendingTransactionView(
      confirmationStore: confirmationStore,
      networkStore: networkStore,
      keyringStore: keyringStore,
      isShowingGas: $isShowingGas,
      isShowingAdvancedSettings: $isShowingAdvancedSettings,
      onDismiss: {
        if confirmationStore.unapprovedTxs.count == 0 {
          onDismiss()
        }
        // update activeTransactionId
        confirmationStore.updateActiveTxIdAfterSignedClosed()
      }
    )
    .overlay(
      Group {
        if confirmationStore.isTxSubmitting {
          ProgressView()
            .progressViewStyle(.braveCircular(size: .large, tint: .braveBlurpleTint))
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .background(Color(.braveGroupedBackground).ignoresSafeArea())
        } else {
          TransactionStatusView(
            confirmationStore: confirmationStore,
            networkStore: networkStore,
            transactionDetails: $transactionDetails) {
              if confirmationStore.unapprovedTxs.count == 0 {
                onDismiss()
              }
              // update activeTransactionId
              confirmationStore.updateActiveTxIdAfterSignedClosed()
            }
        }
      }
    )
  }

  var body: some View {
    ZStack {
      containerView
    }
    .navigationBarTitle(navigationTitle)
    .navigationBarTitleDisplayMode(.inline)
    .foregroundColor(Color(.braveLabel))
    .background(Color(.braveGroupedBackground).edgesIgnoringSafeArea(.all))
    .onChange(of: confirmationStore.activeTransactionId, perform: { newValue in
      // we are looking for `activeTransactionId` value
      // when the value is an empty string meaning there is no active transaction
      // aka there is no remaining pending transations
      if newValue == "" {
        onDismiss()
      }
    })
    .onAppear {
      Task {
        await confirmationStore.prepare()
      }
    }
    .background(
      NavigationLink(
        isActive: $isShowingGas,
        destination: {
          Group {
            if let gasEstimation = confirmationStore.eip1559GasEstimation {
              EditPriorityFeeView(
                transaction: confirmationStore.activeParsedTransaction.transaction,
                gasEstimation: gasEstimation,
                confirmationStore: confirmationStore
              )
            } else {
              EditGasFeeView(
                transaction: confirmationStore.activeParsedTransaction.transaction,
                confirmationStore: confirmationStore
              )
            }
          }
        },
        label: { EmptyView() }
      )
    )
    .background(
      NavigationLink(
        isActive: $isShowingAdvancedSettings,
        destination: {
          EditNonceView(
            confirmationStore: confirmationStore,
            transaction: confirmationStore.activeParsedTransaction.transaction
          )
        },
        label: { EmptyView() }
      )
    )
    .background(
      Color.clear
        .sheet(
          isPresented: Binding(
            get: { self.transactionDetails != nil },
            set: {
              if !$0 {
                self.transactionDetails = nil
                self.confirmationStore.closeTxDetailsStore()
              }
            }
          )
        ) {
          if let transactionDetailsStore = transactionDetails {
            TransactionDetailsView(
              transactionDetailsStore: transactionDetailsStore,
              networkStore: networkStore
            )
          }
        }
    )
  }
}

#if DEBUG
struct TransactionConfirmationView_Previews: PreviewProvider {
  static var previews: some View {
    TransactionConfirmationView(
      confirmationStore: .previewStore,
      networkStore: .previewStore,
      keyringStore: .previewStoreWithWalletCreated,
      onDismiss: { }
    )
    .previewLayout(.sizeThatFits)
  }
}
#endif
