// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import BraveUI
import struct Shared.Strings
import BigNumber

struct TransactionConfirmationView: View {
  var transactions: [BraveWallet.TransactionInfo]
  
  @ObservedObject var confirmationStore: TransactionConfirmationStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var keyringStore: KeyringStore
  
  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  private enum ViewMode: Int {
    case transaction
    case details
  }
  
  @State private var viewMode: ViewMode = .transaction
  @State private var activeTransactionId: BraveWallet.TransactionInfo.ID = "" {
    didSet {
      confirmationStore.fetchDetails(for: activeTransaction)
    }
  }
  
  private func next() {
    if let index = transactions.firstIndex(where: { $0.id == activeTransactionId }) {
      var nextIndex = transactions.index(after: index)
      if nextIndex == transactions.endIndex {
        nextIndex = 0
      }
      activeTransactionId = transactions[nextIndex].id
    } else {
      activeTransactionId = transactions.first!.id
    }
  }
  
  private func rejectAll() {
    for transaction in transactions {
      confirmationStore.reject(transaction: transaction)
    }
  }
  
  private var activeTransaction: BraveWallet.TransactionInfo {
    transactions.first(where: { $0.id == activeTransactionId }) ?? transactions.first!
  }
  
  private var fromAccountName: String {
    NamedAddresses.name(for: activeTransaction.fromAddress, accounts: keyringStore.keyring.accountInfos)
  }
  
  private var toAccountName: String {
    NamedAddresses.name(for: activeTransaction.txData.baseData.to, accounts: keyringStore.keyring.accountInfos)
  }
  
  private var transactionType: String {
    if activeTransaction.txType == .erc20Approve {
      return Strings.Wallet.transactionTypeApprove
    }
    return activeTransaction.isSwap ? Strings.Wallet.swap : Strings.Wallet.send
  }
  
  private var transactionDetails: String {
    let data = activeTransaction.txData.baseData.data
      .map { byte in
        String(format: "%02X", byte.uint8Value)
      }
      .joined()
    if data.isEmpty {
      return Strings.Wallet.inputDataPlaceholder
    }
    return "0x\(data)"
  }
  
  @ViewBuilder private var editGasFeeButton: some View {
    let titleView = Text(Strings.Wallet.editGasFeeButtonTitle)
      .fontWeight(.semibold)
      .foregroundColor(Color(.braveBlurpleTint))
    Group {
      if activeTransaction.isEIP1559Transaction {
        if let gasEstimation = activeTransaction.txData.gasEstimation {
          NavigationLink(
            destination: EditPriorityFeeView(
              transaction: activeTransaction,
              gasEstimation: gasEstimation,
              confirmationStore: confirmationStore
            )
          ) {
            titleView
          }
        }
      } else {
        NavigationLink(
          destination: EditGasFeeView(transaction: activeTransaction, confirmationStore: confirmationStore)
        ) {
          titleView
        }
      }
    }
    .font(.footnote)
  }
  
  var body: some View {
    NavigationView {
      ScrollView(.vertical) {
        VStack {
          // Header
          HStack {
            Text(networkStore.selectedChain.shortChainName)
            Spacer()
            if transactions.count > 1 {
              let index = transactions.firstIndex(of: activeTransaction) ?? 0
              Text(String.localizedStringWithFormat(Strings.Wallet.transactionCount, index + 1, transactions.count))
                .fontWeight(.semibold)
              Button(action: next) {
                Text(Strings.Wallet.nextTransaction)
                  .fontWeight(.semibold)
                  .foregroundColor(Color(.braveBlurpleTint))
              }
            }
          }
          .font(.callout)
          // Summary
          VStack(spacing: 8) {
            VStack {
              BlockieGroup(
                fromAddress: activeTransaction.fromAddress,
                toAddress: activeTransaction.txData.baseData.to,
                size: 48
              )
              Group {
                if sizeCategory.isAccessibilityCategory {
                  VStack {
                    Text(fromAccountName)
                    Image(systemName: "arrow.down")
                    Text(toAccountName)
                  }
                } else {
                  HStack {
                    Text(fromAccountName)
                    Image(systemName: "arrow.right")
                    Text(toAccountName)
                  }
                }
              }
              .foregroundColor(Color(.bravePrimary))
              .font(.callout)
            }
            .accessibilityElement()
            .accessibility(addTraits: .isStaticText)
            .accessibility(
              label: Text(String.localizedStringWithFormat(
                Strings.Wallet.transactionFromToAccessibilityLabel, fromAccountName, toAccountName
              ))
            )
            VStack(spacing: 4) {
              Text(transactionType)
                .font(.footnote)
              Text("\(confirmationStore.state.value) \(confirmationStore.state.symbol)")
                .fontWeight(.semibold)
                .foregroundColor(Color(.bravePrimary))
              Text(confirmationStore.state.fiat) // Value in Fiat
                .font(.footnote)
            }
            .padding(.vertical, 8)
          }
          // View Mode
          VStack(spacing: 12) {
            Picker("", selection: $viewMode) {
              Text(Strings.Wallet.confirmationViewModeTransaction).tag(ViewMode.transaction)
              Text(Strings.Wallet.confirmationViewModeDetails).tag(ViewMode.details)
            }
            .pickerStyle(SegmentedPickerStyle())
            Group {
              switch viewMode {
              case .transaction:
                VStack(spacing: 0) {
                  HStack {
                    VStack(alignment: .leading) {
                      Text(Strings.Wallet.gasFee)
                        .foregroundColor(Color(.bravePrimary))
                      editGasFeeButton
                    }
                    Spacer()
                    VStack(alignment: .trailing) {
                      Text("\(confirmationStore.state.gasValue) \(confirmationStore.state.gasSymbol)")
                        .foregroundColor(Color(.bravePrimary))
                      Text(confirmationStore.state.gasFiat)
                        .font(.footnote)
                    }
                  }
                  .font(.callout)
                  .padding()
                  .accessibilityElement(children: .contain)
                  Divider()
                    .padding(.leading)
                  HStack {
                    Text(Strings.Wallet.total)
                      .foregroundColor(Color(.bravePrimary))
                      .font(.callout)
                      .accessibility(sortPriority: 1)
                    Spacer()
                    VStack(alignment: .trailing) {
                      Text(Strings.Wallet.amountAndGas)
                        .font(.footnote)
                        .foregroundColor(Color(.secondaryBraveLabel))
                      Text("\(confirmationStore.state.value) \(confirmationStore.state.symbol) + \(confirmationStore.state.gasValue) \(confirmationStore.state.gasSymbol)")
                        .foregroundColor(Color(.bravePrimary))
                      HStack(spacing: 4) {
                        if !confirmationStore.state.isBalanceSufficient {
                          Text(Strings.Wallet.insufficientBalance)
                            .foregroundColor(Color(.braveErrorLabel))
                        }
                        Text(confirmationStore.state.totalFiat)
                          .foregroundColor(
                            confirmationStore.state.isBalanceSufficient ? Color(.braveLabel) : Color(.braveErrorLabel)
                          )
                      }
                      .accessibilityElement(children: .contain)
                      .font(.footnote)
                    }
                  }
                  .padding()
                  .accessibilityElement(children: .contain)
                }
              case .details:
                VStack(alignment: .leading) {
                  DetailsTextView(text: transactionDetails)
                    .frame(maxWidth: .infinity)
                    .frame(height: 200)
                    .background(Color(.tertiaryBraveGroupedBackground))
                    .clipShape(RoundedRectangle(cornerRadius: 5, style: .continuous))
                }
                .padding()
              }
            }
            .frame(maxWidth: .infinity)
            .background(
              Color(.secondaryBraveGroupedBackground)
            )
            .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
          }
          if transactions.count > 1 {
            Button(action: rejectAll) {
              Text(String.localizedStringWithFormat(Strings.Wallet.rejectAllTransactions, transactions.count))
                .font(.subheadline.weight(.semibold))
                .foregroundColor(Color(.braveBlurpleTint))
            }
            .padding(.top, 8)
          }
          rejectConfirmContainer
            .padding(.top)
            .opacity(sizeCategory.isAccessibilityCategory ? 0 : 1)
            .accessibility(hidden: sizeCategory.isAccessibilityCategory)
        }
        .padding()
      }
      .overlay(
        Group {
          if sizeCategory.isAccessibilityCategory {
            rejectConfirmContainer
              .frame(maxWidth: .infinity)
              .padding(.top)
              .background(
                LinearGradient(
                  stops: [
                    .init(color: Color(.braveGroupedBackground).opacity(0), location: 0),
                    .init(color: Color(.braveGroupedBackground).opacity(1), location: 0.05),
                    .init(color: Color(.braveGroupedBackground).opacity(1), location: 1),
                  ],
                  startPoint: .top,
                  endPoint: .bottom
                )
                  .ignoresSafeArea()
                  .allowsHitTesting(false)
              )
          }
        },
        alignment: .bottom
      )
      .navigationBarTitle(transactions.count > 1 ? Strings.Wallet.confirmTransactionsTitle : Strings.Wallet.confirmTransactionTitle)
      .navigationBarTitleDisplayMode(.inline)
      .foregroundColor(Color(.braveLabel))
      .background(Color(.braveGroupedBackground).edgesIgnoringSafeArea(.all))
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button(action: { presentationMode.dismiss() }) {
            Text(Strings.CancelString)
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
    }
    .navigationViewStyle(StackNavigationViewStyle())
    .onAppear {
      assert(!transactions.isEmpty, "TransactionConfirmationView should not be displayed if there are no transactions to approve.")
      activeTransactionId = transactions[0].id
      confirmationStore.fetchDetails(for: activeTransaction)
    }
  }
  
  @ViewBuilder private var rejectConfirmContainer: some View {
    if sizeCategory.isAccessibilityCategory {
      VStack {
        rejectConfirmButtons
      }
    } else {
      HStack {
        rejectConfirmButtons
      }
    }
  }
  
  @ViewBuilder private var rejectConfirmButtons: some View {
    Button(action: {
      confirmationStore.reject(transaction: activeTransaction)
    }) {
      Label(Strings.Wallet.rejectTransactionButtonTitle, systemImage: "xmark")
    }
    .buttonStyle(BraveOutlineButtonStyle(size: .large))
    Button(action: {
      confirmationStore.confirm(transaction: activeTransaction)
    }) {
      Label(Strings.Wallet.confirmTransactionButtonTitle, systemImage: "checkmark.circle.fill")
    }
    .buttonStyle(BraveFilledButtonStyle(size: .large))
    .disabled(!confirmationStore.state.isBalanceSufficient)
  }
}

/// We needed a `TextEditor` that couldn't be edited and had a clear background color
/// so we have to fallback to UIKit for this
private struct DetailsTextView: UIViewRepresentable {
  var text: String
  
  func makeUIView(context: Context) -> UITextView {
    let textView = UITextView()
    textView.text = text
    textView.isEditable = false
    textView.backgroundColor = .tertiaryBraveGroupedBackground
    textView.font = {
      let metrics = UIFontMetrics(forTextStyle: .body)
      let desc = UIFontDescriptor.preferredFontDescriptor(withTextStyle: .body)
      let font = UIFont.monospacedSystemFont(ofSize: desc.pointSize, weight: .regular)
      return metrics.scaledFont(for: font)
    }()
    textView.adjustsFontForContentSizeCategory = true
    textView.textContainerInset = .init(top: 12, left: 8, bottom: 12, right: 8)
    return textView
  }
  func updateUIView(_ uiView: UITextView, context: Context) {
    uiView.text = text
  }
}

#if DEBUG
struct TransactionConfirmationView_Previews: PreviewProvider {
  static var previews: some View {
    TransactionConfirmationView(
      transactions: [
        BraveWallet.TransactionInfo.previewConfirmedERC20Approve,
        .previewConfirmedSend,
        .previewConfirmedSwap
      ].map {
        tx in
        tx.txStatus = .unapproved
        return tx
      },
      confirmationStore: .previewStore,
      networkStore: .previewStore,
      keyringStore: .previewStoreWithWalletCreated
    )
      .previewLayout(.sizeThatFits)
  }
}
#endif
