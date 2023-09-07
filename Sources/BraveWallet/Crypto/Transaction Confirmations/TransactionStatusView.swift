// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import BigNumber
import Strings
import DesignSystem

struct TransactionStatusView: View {
  @ObservedObject var confirmationStore: TransactionConfirmationStore
  let networkStore: NetworkStore
  @Binding var transactionDetails: TransactionDetailsStore?
 
  let onDismiss: () -> Void
  
  @Environment(\.openURL) private var openWalletURL
  
  @ViewBuilder private var signedOrSubmittedTxView: some View {
    GeometryReader { geometry in
      ScrollView(.vertical) {
        VStack(spacing: 10) {
          Image("tx-submitted", bundle: .module)
          Text(confirmationStore.activeTxStatus == .signed ? Strings.Wallet.signedTransactionTitle : Strings.Wallet.submittedTransactionTitle)
            .font(.title3.bold())
            .foregroundColor(Color(.braveLabel))
            .multilineTextAlignment(.center)
            .padding(.top, 10)
          Text(confirmationStore.activeTxStatus == .signed ? Strings.Wallet.signedTransactionDescription : Strings.Wallet.submittedTransactionDescription)
            .font(.subheadline)
            .foregroundColor(Color(.secondaryBraveLabel))
            .multilineTextAlignment(.center)
          Button(action: onDismiss) {
            Text(Strings.OKString)
              .padding(.horizontal, 8)
          }
          .padding(.top, 40)
          .buttonStyle(BraveFilledButtonStyle(size: .large))
          Button {
            if let tx = confirmationStore.allTxs.first(where: { $0.id == confirmationStore.activeTransactionId }),
               let txNetwork = networkStore.allChains.first(where: { $0.chainId == tx.chainId }),
               let url = txNetwork.txBlockExplorerLink(txHash: tx.txHash, for: txNetwork.coin) {
              openWalletURL(url)
            }
          } label: {
            HStack {
              Text(Strings.Wallet.viewOnBlockExplorer)
              Image(systemName: "arrow.up.forward.square")
            }
            .foregroundColor(Color(.braveBlurpleTint))
            .font(.subheadline.bold())
          }
          .padding(.top, 10)
        }
        .frame(maxWidth: .infinity, minHeight: geometry.size.height)
        .padding()
      }
      .background(
        Color(.braveGroupedBackground)
          .edgesIgnoringSafeArea(.all)
      )
    }
  }
  
  @ViewBuilder private var confirmedOrFailedTxView: some View {
    GeometryReader { geometry in
      ScrollView(.vertical) {
        VStack(spacing: 10) {
          Image(confirmationStore.activeTxStatus == .confirmed ? "tx-confirmed" : "tx-failed", bundle: .module)
          Text(confirmationStore.activeTxStatus == .confirmed ? Strings.Wallet.confirmedTransactionTitle : Strings.Wallet.failedTransactionTitle)
            .font(.title3.bold())
            .foregroundColor(confirmationStore.activeTxStatus == .confirmed ? Color(.braveLabel) : Color(.braveErrorLabel))
            .multilineTextAlignment(.center)
            .padding(.top, 10)
          Text(confirmationStore.activeTxStatus == .confirmed ? Strings.Wallet.confirmedTransactionDescription : Strings.Wallet.failedTransactionDescription)
            .font(.subheadline)
            .foregroundColor(Color(.secondaryBraveLabel))
            .multilineTextAlignment(.center)
          if confirmationStore.activeTxStatus == .error, let txProviderError = confirmationStore.transactionProviderErrorRegistry[confirmationStore.activeTransactionId] {
            StaticTextView(text: "\(txProviderError.code): \(txProviderError.message)")
              .frame(maxWidth: .infinity)
              .frame(height: 100)
              .background(Color(.tertiaryBraveGroupedBackground))
              .clipShape(RoundedRectangle(cornerRadius: 5, style: .continuous))
              .padding()
              .background(
                Color(.secondaryBraveGroupedBackground)
              )
              .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
              .padding(.top, 10)
          }
          HStack {
            if confirmationStore.activeTxStatus == .confirmed {
              Button {
                transactionDetails = confirmationStore.activeTxDetailsStore()
              } label: {
                Text(Strings.Wallet.confirmedTransactionReceiptButtonTitle)
              }
              .buttonStyle(BraveOutlineButtonStyle(size: .large))
            }
            Button(action: onDismiss) {
              Text(Strings.Wallet.confirmedTransactionCloseButtonTitle)
            }
            .buttonStyle(BraveFilledButtonStyle(size: .large))
          }
          .padding(.top, 40)
        }
        .frame(maxWidth: .infinity, minHeight: geometry.size.height)
        .padding()
      }
      .background(
        Color(.braveGroupedBackground)
          .edgesIgnoringSafeArea(.all)
      )
    }
  }
  
  var body: some View {
    switch confirmationStore.activeTxStatus {
    case .signed, .submitted:
      signedOrSubmittedTxView
    case .confirmed, .error:
      if confirmationStore.activeTxStatus == .error, confirmationStore.activeTransactionId.isEmpty {
        EmptyView()
      } else {
        confirmedOrFailedTxView
      }
    default:
      EmptyView()
    }
  }
}

#if DEBUG
struct TransactionStatusView_Previews: PreviewProvider {
  static var previews: some View {
    TransactionStatusView(
      confirmationStore: .previewStore,
      networkStore: .previewStore,
      transactionDetails: .constant(nil),
      onDismiss: { }
    )
  }
}
#endif
