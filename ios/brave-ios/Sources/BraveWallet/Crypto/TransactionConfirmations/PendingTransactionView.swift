// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BigNumber
import BraveCore
import DesignSystem
import Strings
import SwiftUI

struct PendingTransactionView: View {
  @ObservedObject var confirmationStore: TransactionConfirmationStore
  let networkStore: NetworkStore
  let keyringStore: KeyringStore
  @Binding var isShowingGas: Bool
  @Binding var isShowingAdvancedSettings: Bool
  @Binding var isTxSubmitting: Bool

  var onDismiss: () -> Void

  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.openURL) private var openWalletURL

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
      return confirmationStore.activeParsedTransaction.transaction.isSwap
        ? Strings.Wallet.swap : Strings.Wallet.send
    }
  }

  /// The view for changing between available pending transactions. ex. '1 of 4 Next'
  @ViewBuilder private var transactionsButton: some View {
    if confirmationStore.unapprovedTxs.count > 1 {
      let index =
        confirmationStore.unapprovedTxs.firstIndex(
          of: confirmationStore.activeParsedTransaction.transaction
        ) ?? 0
      HStack {
        Text(
          String.localizedStringWithFormat(
            Strings.Wallet.transactionCount,
            index + 1,
            confirmationStore.unapprovedTxs.count
          )
        )
        .fontWeight(.semibold)
        Button(action: confirmationStore.nextTransaction) {
          Text(Strings.Wallet.next)
            .fontWeight(.semibold)
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
    }
  }

  /// View showing the currently selected account with a blockie. Used for `erc20Approve` txType only.
  @ViewBuilder private var erc20ApproveAccountView: some View {
    HStack {
      let address = confirmationStore.activeParsedTransaction.fromAccountInfo.blockieSeed
      AddressView(address: address) {
        Text(address.truncatedAddress)
          .fontWeight(.semibold)
      }
      Blockie(address: address)
        .frame(width: blockieSize, height: blockieSize)
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
      if let originInfo = confirmationStore.originInfo {
        Group {
          if originInfo.isBraveWalletOrigin {
            Image("wallet-brave-icon", bundle: .module)
              .resizable()
              .aspectRatio(contentMode: .fit)
              .padding(4)
              .frame(maxWidth: .infinity, maxHeight: .infinity)
              .background(Color(.braveDisabled))
          } else {
            if let url = URL(string: originInfo.originSpec) {
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

        Text(originInfo: originInfo)
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
          Text(
            String.localizedStringWithFormat(
              Strings.Wallet.confirmationViewAllowSpendTitle,
              confirmationStore.symbol
            )
          )
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
          Text(
            String.localizedStringWithFormat(
              Strings.Wallet.confirmationViewAllowSpendSubtitle,
              confirmationStore.symbol
            )
          )
          .font(.footnote)
        }
        .multilineTextAlignment(.center)
      }
      if confirmationStore.isUnlimitedApprovalRequested {
        Label(
          Strings.Wallet.confirmationViewUnlimitedWarning,
          systemImage: "exclamationmark.triangle"
        )
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
          proposedAllowance: confirmationStore.proposedAllowance,
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
    let titleView = Text(Strings.Wallet.editButtonTitle)
      .fontWeight(.semibold)
      .foregroundColor(Color(.braveBlurpleTint))
    Group {
      if let gasEstimation = confirmationStore.eip1559GasEstimation {
        NavigationLink(
          destination: EditPriorityFeeView(
            transaction: confirmationStore.activeParsedTransaction.transaction,
            gasEstimation: gasEstimation,
            confirmationStore: confirmationStore
          )
        ) {
          titleView
        }
      } else {
        NavigationLink(
          destination: EditGasFeeView(
            transaction: confirmationStore.activeParsedTransaction.transaction,
            confirmationStore: confirmationStore
          )
        ) {
          titleView
        }
      }
    }
    .font(.footnote)
  }

  @ViewBuilder private var editNonceRow: some View {
    NavigationLink(
      destination: EditNonceView(
        confirmationStore: confirmationStore,
        transaction: confirmationStore.activeParsedTransaction.transaction
      )
    ) {
      HStack {
        Image(braveSystemName: "leo.settings")
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

  private var currentTransactionView: some View {
    VStack {
      if confirmationStore.activeParsedTransaction.transaction.txType == .erc20Approve {
        erc20ApproveHeader
      } else {
        TransactionHeader(
          fromAccountInfo: confirmationStore.activeParsedTransaction.fromAccountInfo,
          fromAccountName: confirmationStore.activeParsedTransaction.namedFromAddress,
          toAccountAddress: confirmationStore.activeParsedTransaction.toAddress,
          toAccountName: confirmationStore.activeParsedTransaction.namedToAddress,
          isContractAddress: confirmationStore.isContractAddress,
          originInfo: confirmationStore.originInfo,
          transactionType: transactionType,
          value: "\(confirmationStore.value) \(confirmationStore.symbol)",
          fiat: confirmationStore.fiat,
          contractAddressTapped: { contractAddress in
            let network = confirmationStore.activeParsedTransaction.network
            guard let url = network.contractAddressURL(contractAddress) else {
              return
            }
            openWalletURL(url)
          }
        )
      }

      if confirmationStore.isSolTokenTransferWithAssociatedTokenAccountCreation {
        VStack(alignment: .leading, spacing: 8) {
          Text(Strings.Wallet.confirmationViewSolSplTokenAccountCreationWarning)
            .foregroundColor(Color(.braveErrorLabel))
            .font(.subheadline.weight(.medium))
          Button {
            openWalletURL(WalletConstants.splTokenAccountCreationLink)
          } label: {
            Text(Strings.Wallet.learnMoreButton)
              .foregroundColor(Color(.braveBlurpleTint))
              .font(.subheadline)
          }
        }
        .padding(.horizontal, 24)
        .padding(.vertical, 20)
        .background(
          Color(.braveErrorBackground)
            .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
        )
      }

      if confirmationStore.activeParsedTransaction.hasSystemProgramAssignInstruction {
        VStack(alignment: .leading, spacing: 8) {
          Label {
            Text(Strings.Wallet.confirmationViewSolAccountOwnershipChangeWarningTitle)
              .foregroundColor(Color(braveSystemName: .systemfeedbackWarningText))
          } icon: {
            Image(braveSystemName: "leo.warning.triangle-outline")
              .foregroundColor(Color(braveSystemName: .systemfeedbackWarningIcon))
          }
          .font(.subheadline.weight(.bold))
          Text(Strings.Wallet.confirmationViewSolAccountOwnershipChangeWarning)
            .foregroundColor(Color(braveSystemName: .systemfeedbackWarningText))
            .font(.subheadline.weight(.medium))
        }
        .padding(.horizontal, 24)
        .padding(.vertical, 20)
        .background(
          Color(braveSystemName: .systemfeedbackWarningBackground)
            .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
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
              if confirmationStore.activeParsedTransaction.coin == .fil {
                if let gasLimit = confirmationStore.filTxGasLimit {
                  HStack {
                    Text("Gas Limit")
                      .foregroundColor(Color(.bravePrimary))
                    Spacer()
                    Text("\(gasLimit) \(confirmationStore.gasSymbol)")
                      .foregroundColor(Color(.bravePrimary))
                      .multilineTextAlignment(.trailing)
                  }
                  .padding()
                  .accessibilityElement(children: .contain)
                  Divider()
                }
                if let gasPremium = confirmationStore.filTxGasPremium {
                  HStack {
                    Text("Gas Premium")
                      .foregroundColor(Color(.bravePrimary))
                    Spacer()
                    Text("\(gasPremium) \(confirmationStore.gasSymbol)")
                      .foregroundColor(Color(.bravePrimary))
                      .multilineTextAlignment(.trailing)
                  }
                  .padding()
                  .accessibilityElement(children: .contain)
                  Divider()
                }
                if let gasFeeCap = confirmationStore.filTxGasFeeCap {
                  HStack {
                    Text("Gas Fee Cap")
                      .foregroundColor(Color(.bravePrimary))
                    Spacer()
                    Text("\(gasFeeCap) \(confirmationStore.gasSymbol)")
                      .foregroundColor(Color(.bravePrimary))
                      .multilineTextAlignment(.trailing)
                  }
                  .padding()
                  .accessibilityElement(children: .contain)
                  Divider()
                }
              }
              HStack {
                VStack(alignment: .leading) {
                  Text(
                    confirmationStore.activeParsedTransaction.coin == .sol
                      || confirmationStore.activeParsedTransaction.coin == .btc
                      ? Strings.Wallet.transactionFee : Strings.Wallet.gasFee
                  )
                  .foregroundColor(Color(.bravePrimary))
                  if confirmationStore.activeParsedTransaction.coin == .eth {
                    editGasFeeButton
                  }
                }
                Spacer()
                VStack(alignment: .trailing) {
                  Text("\(confirmationStore.gasValue) \(confirmationStore.gasSymbol)")
                    .foregroundColor(Color(.bravePrimary))
                    .multilineTextAlignment(.trailing)
                  Text(confirmationStore.gasFiat)
                    .font(.footnote)
                    .multilineTextAlignment(.trailing)
                }
              }
              .font(.callout)
              .padding()
              .accessibilityElement(children: .contain)
              Divider()
                .padding(.leading)
              if confirmationStore.activeParsedTransaction.transaction.txType == .erc20Approve {
                Group {
                  HStack {
                    Text(Strings.Wallet.confirmationViewCurrentAllowance)
                    Spacer()
                    Text("\(confirmationStore.currentAllowance) \(confirmationStore.symbol)")
                      .multilineTextAlignment(.trailing)
                  }
                  .padding()
                  .accessibilityElement(children: .contain)
                  Divider()
                  HStack {
                    Text(Strings.Wallet.editPermissionsProposedAllowanceHeader)
                    Spacer()
                    Text("\(confirmationStore.value) \(confirmationStore.symbol)")
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
                    Text(
                      confirmationStore.activeParsedTransaction.coin == .sol
                        || confirmationStore.activeParsedTransaction.coin == .btc
                        ? Strings.Wallet.amountAndFee : Strings.Wallet.amountAndGas
                    )
                    .font(.footnote)
                    .foregroundColor(Color(.secondaryBraveLabel))
                    Text(
                      "\(confirmationStore.value) \(confirmationStore.symbol) + \(confirmationStore.gasValue) \(confirmationStore.gasSymbol)"
                    )
                    .foregroundColor(Color(.bravePrimary))
                    .multilineTextAlignment(.trailing)
                    HStack(spacing: 4) {
                      if !confirmationStore.isBalanceSufficient {
                        Text(Strings.Wallet.insufficientBalance)
                          .foregroundColor(Color(.braveErrorLabel))
                      }
                      Text(confirmationStore.totalFiat)
                        .foregroundColor(
                          confirmationStore.isBalanceSufficient
                            ? Color(.braveLabel) : Color(.braveErrorLabel)
                        )
                    }
                    .accessibilityElement(children: .contain)
                    .font(.footnote)
                  }
                }
                .padding()
                .accessibilityElement(children: .contain)
              }
              if confirmationStore.activeParsedTransaction.coin == .eth {
                Divider()
                  .padding(.leading)
                editNonceRow
              }
            }
          case .details:
            VStack(alignment: .leading) {
              StaticTextView(text: confirmationStore.transactionDetails)
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
    Button {
      Task {
        await confirmationStore.reject(
          transaction: confirmationStore.activeParsedTransaction.transaction
        )
      }
    } label: {
      Label(Strings.Wallet.rejectTransactionButtonTitle, systemImage: "xmark")
    }
    .buttonStyle(BraveOutlineButtonStyle(size: .large))
    .disabled(isTxSubmitting)
    WalletLoadingButton(
      isLoading: isTxSubmitting
    ) {
      Task {
        await confirmationStore.confirm(
          transaction: confirmationStore.activeParsedTransaction.transaction
        )
      }
    } label: {
      Label(Strings.Wallet.confirm, systemImage: "checkmark.circle.fill")
    }
    .buttonStyle(BraveFilledButtonStyle(size: .large))
    .disabled(
      !confirmationStore.isBalanceSufficient || confirmationStore.activeTxStatus == .approved
    )
    .transaction { transaction in
      transaction.animation = nil
      transaction.disablesAnimations = true
    }
  }

  var body: some View {
    ScrollView(.vertical) {
      VStack {
        // Current network, transaction buttons
        HStack(alignment: .top) {
          if confirmationStore.activeParsedTransaction.transaction.txType != .ethSwap {
            // network shown below each token for swap
            Text(confirmationStore.activeParsedTransaction.network.chainName)
          }
          Spacer()
          VStack(alignment: .trailing) {
            transactionsButton
            if confirmationStore.activeParsedTransaction.transaction.txType == .erc20Approve {
              // for other txTypes, account is shown in `TransactionHeader`
              erc20ApproveAccountView
            }
          }
        }
        .font(.callout)

        // Current Active Transaction info
        if confirmationStore.activeParsedTransaction.transaction.txType == .ethSwap {
          SaferSignTransactionContainerView(
            parsedTransaction: confirmationStore.activeParsedTransaction,
            editGasFeeTapped: {
              isShowingGas = true
            },
            advancedSettingsTapped: {
              isShowingAdvancedSettings = true
            }
          )
        } else {
          currentTransactionView
        }

        // Cancel / Confirm buttons
        if confirmationStore.unapprovedTxs.count > 1 {
          Button {
            Task {
              let success = await confirmationStore.rejectAllTransactions()
              if success {
                onDismiss()
              }
            }
          } label: {
            Text(
              String.localizedStringWithFormat(
                Strings.Wallet.rejectAllTransactions,
                confirmationStore.unapprovedTxs.count
              )
            )
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
  }
}

#if DEBUG
struct PendingTransactionView_Previews: PreviewProvider {
  static var previews: some View {
    PendingTransactionView(
      confirmationStore: .previewStore,
      networkStore: .previewStore,
      keyringStore: .previewStore,
      isShowingGas: .constant(false),
      isShowingAdvancedSettings: .constant(false),
      isTxSubmitting: .constant(false),
      onDismiss: {}
    )
  }
}
#endif
