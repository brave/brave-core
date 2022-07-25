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

  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.presentationMode) @Binding private var presentationMode

  /// Blockie size for ERC 20 Approve transactions
  @ScaledMetric private var blockieSize = 24
  /// Favicon size for ERC 20 Approve transactions
  @ScaledMetric private var faviconSize = 48
  private let maxFaviconSize: CGFloat = 96

  private enum ViewMode: Int {
    case transaction
    case details
  }

  @State private var viewMode: ViewMode = .transaction

  private var fromAccountName: String {
    NamedAddresses.name(for: confirmationStore.activeTransaction.fromAddress, accounts: keyringStore.allAccounts)
  }

  private var toAccountName: String {
    return NamedAddresses.name(for: confirmationStore.activeTransaction.ethTxToAddress, accounts: keyringStore.allAccounts)
  }

  private var transactionType: String {
    if confirmationStore.activeTransaction.txType == .erc20Approve {
      return Strings.Wallet.transactionTypeApprove
    }
    return confirmationStore.activeTransaction.isSwap ? Strings.Wallet.swap : Strings.Wallet.send
  }

  private var transactionDetails: String {
    if confirmationStore.activeTransaction.txArgs.isEmpty {
      let data = confirmationStore.activeTransaction.ethTxData
        .map { byte in
          String(format: "%02X", byte.uint8Value)
        }
        .joined()
      if data.isEmpty {
        return Strings.Wallet.inputDataPlaceholder
      }
      return "0x\(data)"
    } else {
      return zip(confirmationStore.activeTransaction.txParams, confirmationStore.activeTransaction.txArgs)
        .map { (param, arg) in
          "\(param): \(arg)"
        }
        .joined(separator: "\n\n")
    }
  }
  
  private var isConfirmDisabled: Bool {
    confirmationStore.activeTransaction.coin != networkStore.selectedChain.coin
  }

  /// View showing the currently selected account with a blockie
  @ViewBuilder private var accountView: some View {
    HStack {
      AddressView(address: keyringStore.selectedAccount.address) {
        Text(keyringStore.selectedAccount.address.truncatedAddress)
          .fontWeight(.semibold)
      }
      Blockie(address: keyringStore.selectedAccount.address)
        .frame(width: blockieSize, height: blockieSize)
    }
  }

  /// The view for changing between available pending transactions. ex. '1 of 4 Next'
  @ViewBuilder private var transactionsButton: some View {
    if confirmationStore.transactions.count > 1 {
      let index = confirmationStore.transactions.firstIndex(of: confirmationStore.activeTransaction) ?? 0
      HStack {
        Text(String.localizedStringWithFormat(Strings.Wallet.transactionCount, index + 1, confirmationStore.transactions.count))
          .fontWeight(.semibold)
        Button(action: confirmationStore.nextTransaction) {
          Text(Strings.Wallet.next)
            .fontWeight(.semibold)
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
    } else {
      EmptyView()
    }
  }
  
  private var globeFavicon: some View {
    Image(systemName: "globe")
      .resizable()
      .aspectRatio(contentMode: .fit)
      .padding(8)
      .background(Color(.braveDisabled))
  }
  
  @ViewBuilder private var faviconAndOrigin: some View {
    VStack(spacing: 8) {
      if let originInfo = confirmationStore.state.originInfo {
        Group {
          if originInfo.isBraveWalletOrigin {
            Image("wallet-brave-icon", bundle: .current)
              .resizable()
              .aspectRatio(contentMode: .fit)
              .padding(4)
              .frame(maxWidth: .infinity, maxHeight: .infinity)
              .background(Color(.braveDisabled))
          } else {
            if let url = originInfo.origin.url {
              FaviconReader(url: url) { image in
                if let image = image {
                  Image(uiImage: image)
                    .resizable()
                } else {
                  globeFavicon
                }
              }
            } else {
              globeFavicon
            }
          }
        }
        .frame(width: min(faviconSize, maxFaviconSize), height: min(faviconSize, maxFaviconSize))
        .clipShape(RoundedRectangle(cornerRadius: 4, style: .continuous))
        Text(urlOrigin: originInfo.origin)
          .font(.subheadline)
          .foregroundColor(Color(.braveLabel))
          .multilineTextAlignment(.center)
      }
    }
    .accessibilityElement(children: .combine)
  }

  /// The header displayed for an `erc20Approve` txType transaction
  @ViewBuilder private var erc20ApproveHeader: some View {
    VStack(spacing: 20) {
      VStack(spacing: 8) {
        faviconAndOrigin
        VStack(spacing: 10) {
          Text(String.localizedStringWithFormat(Strings.Wallet.confirmationViewAllowSpendTitle, confirmationStore.state.symbol))
            .fontWeight(.semibold)
            .foregroundColor(Color(.bravePrimary))
          Text(String.localizedStringWithFormat(Strings.Wallet.confirmationViewAllowSpendSubtitle, confirmationStore.state.symbol))
            .font(.footnote)
        }
        .multilineTextAlignment(.center)
      }
      if confirmationStore.state.isUnlimitedApprovalRequested {
        Label(Strings.Wallet.confirmationViewUnlimitedWarning, systemImage: "exclamationmark.triangle")
          .padding(12)
          .foregroundColor(Color(.braveErrorLabel))
          .font(.subheadline)
          .background(
            Color(.braveErrorBackground)
              .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
          )
      }
      NavigationLink(
        destination: EditPermissionsView(
          proposedAllowance: confirmationStore.activeTransaction.txArgs[safe: 1] ?? "",
          confirmationStore: confirmationStore,
          keyringStore: keyringStore,
          networkStore: networkStore
        )
      ) {
        Text(Strings.Wallet.confirmationViewEditPermissions)
          .font(.subheadline.weight(.semibold))
          .foregroundColor(Color(.braveBlurpleTint))
      }
    }
    .padding(.horizontal)
    .padding(.bottom)
  }

  @ViewBuilder private var editGasFeeButton: some View {
    let titleView = Text(Strings.Wallet.editGasFeeButtonTitle)
      .fontWeight(.semibold)
      .foregroundColor(Color(.braveBlurpleTint))
    Group {
      if confirmationStore.activeTransaction.isEIP1559Transaction {
        if let gasEstimation = confirmationStore.activeTransaction.txDataUnion.ethTxData1559?.gasEstimation {
          NavigationLink(
            destination: EditPriorityFeeView(
              transaction: confirmationStore.activeTransaction,
              gasEstimation: gasEstimation,
              confirmationStore: confirmationStore
            )
          ) {
            titleView
          }
        }
      } else {
        NavigationLink(
          destination: EditGasFeeView(
            transaction: confirmationStore.activeTransaction,
            confirmationStore: confirmationStore
          )
        ) {
          titleView
        }
      }
    }
    .font(.footnote)
  }

  var body: some View {
      ScrollView(.vertical) {
        VStack {
          // Header
          HStack(alignment: .top) {
            Text(networkStore.selectedChain.shortChainName)
            Spacer()
            VStack(alignment: .trailing) {
              transactionsButton
              if confirmationStore.activeTransaction.txType == .erc20Approve {
                accountView // for other txTypes, account is shown in `TransactionHeader`
              }
            }
          }
          .font(.callout)
          // Summary
          if confirmationStore.activeTransaction.txType == .erc20Approve {
            erc20ApproveHeader
          } else {
            TransactionHeader(
              fromAccountAddress: confirmationStore.activeTransaction.fromAddress,
              fromAccountName: fromAccountName,
              toAccountAddress: confirmationStore.activeTransaction.ethTxToAddress,
              toAccountName: toAccountName,
              originInfo: confirmationStore.state.originInfo,
              transactionType: transactionType,
              value: "\(confirmationStore.state.value) \(confirmationStore.state.symbol)",
              fiat: confirmationStore.state.fiat
            )
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
                  if confirmationStore.activeTransaction.txType == .erc20Approve {
                    Group {
                      HStack {
                        Text(Strings.Wallet.confirmationViewCurrentAllowance)
                        Spacer()
                        Text("\(confirmationStore.state.currentAllowance) \(confirmationStore.state.symbol)")
                          .multilineTextAlignment(.trailing)
                      }
                      .padding()
                      .accessibilityElement(children: .contain)
                      Divider()
                      HStack {
                        Text(Strings.Wallet.editPermissionsProposedAllowanceHeader)
                        Spacer()
                        Text("\(confirmationStore.state.value) \(confirmationStore.state.symbol)")
                          .multilineTextAlignment(.trailing)
                      }
                      .padding()
                      .accessibilityElement(children: .contain)
                    }
                    .font(.callout)
                    .foregroundColor(Color(.bravePrimary))
                  } else {
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
                  Divider()
                    .padding(.leading)
                  NavigationLink(
                    destination: EditNonceView(
                      confirmationStore: confirmationStore,
                      transaction: confirmationStore.activeTransaction
                    )
                  ) {
                    HStack {
                      Image(braveSystemName: "brave.gear")
                        .foregroundColor(Color(.braveBlurpleTint))
                      Text(Strings.Wallet.advancedSettingsTransaction)
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .foregroundColor(Color(.braveBlurpleTint))
                      Spacer()
                      Image(systemName: "chevron.right")
                    }
                    .padding()
                    .font(.footnote.weight(.semibold))
                  }
                }
              case .details:
                VStack(alignment: .leading) {
                  StaticTextView(text: transactionDetails)
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
          if confirmationStore.transactions.count > 1 {
            Button(action: confirmationStore.rejectAllTransactions) {
              Text(String.localizedStringWithFormat(Strings.Wallet.rejectAllTransactions, confirmationStore.transactions.count))
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
      .navigationBarTitle(confirmationStore.transactions.count > 1 ? Strings.Wallet.confirmTransactionsTitle : Strings.Wallet.confirmTransactionTitle)
      .navigationBarTitleDisplayMode(.inline)
      .foregroundColor(Color(.braveLabel))
      .background(Color(.braveGroupedBackground).edgesIgnoringSafeArea(.all))
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button(action: { presentationMode.dismiss() }) {
            Text(Strings.cancelButtonTitle)
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
    .onAppear {
      confirmationStore.prepare()
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
      confirmationStore.reject(transaction: confirmationStore.activeTransaction) { success in
        if confirmationStore.transactions.count <= 1 {
          onDismiss()
        }
      }
    }) {
      Label(Strings.Wallet.rejectTransactionButtonTitle, systemImage: "xmark")
    }
    .buttonStyle(BraveOutlineButtonStyle(size: .large))
    Button(action: {
      confirmationStore.confirm(transaction: confirmationStore.activeTransaction) { error in
        if confirmationStore.transactions.count <= 1 {
          onDismiss()
        }
      }
    }) {
      Label(Strings.Wallet.confirm, systemImage: "checkmark.circle.fill")
    }
    .buttonStyle(BraveFilledButtonStyle(size: .large))
    .disabled(!confirmationStore.state.isBalanceSufficient || isConfirmDisabled)
  }
}

/// We needed a `TextEditor` that couldn't be edited and had a clear background color
/// so we have to fallback to UIKit for this
struct StaticTextView: UIViewRepresentable {
  var text: String
  var isMonospaced: Bool = true

  func makeUIView(context: Context) -> UITextView {
    let textView = UITextView()
    textView.text = text
    textView.isEditable = false
    textView.backgroundColor = .tertiaryBraveGroupedBackground
    textView.font = {
      let metrics = UIFontMetrics(forTextStyle: .body)
      let desc = UIFontDescriptor.preferredFontDescriptor(withTextStyle: .body)
      let font = isMonospaced ?
        UIFont.monospacedSystemFont(ofSize: desc.pointSize, weight: .regular) :
        UIFont.systemFont(ofSize: desc.pointSize, weight: .regular)
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
      confirmationStore: .previewStore,
      networkStore: .previewStore,
      keyringStore: .previewStoreWithWalletCreated,
      onDismiss: { }
    )
    .previewLayout(.sizeThatFits)
  }
}
#endif
