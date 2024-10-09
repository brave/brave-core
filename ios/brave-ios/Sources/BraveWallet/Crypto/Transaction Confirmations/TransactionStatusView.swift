// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BigNumber
import BraveCore
import DesignSystem
import Strings
import SwiftUI

struct TransactionStatusView: View {
  @ObservedObject var txStatusStore: TransactionStatusStore
  let networkStore: NetworkStore

  let onDismiss: () -> Void

  @Environment(\.openURL) private var openWalletURL
  @Environment(\.presentationMode) var presentationMode: Binding<PresentationMode>

  @State private var isSpinning: Bool = false
  @State private var isConfirmSpinning: Bool = false
  @State private var isConfirmSpinningFinished: Bool = false
  @State private var isShowingCancelConfirmation: Bool = false
  @State private var isShowingCancelError: Bool = false
  @State private var isShowingPendingCancelTx: Bool = false
  @State private var scrollViewContentSize: CGSize = .zero

  @ViewBuilder private var statusIcon: some View {
    switch txStatusStore.activeTxStatus {
    case .submitted:
      if txStatusStore.isConfirmingCancelTx {
        EmptyView()
      } else {
        Image(braveSystemName: txStatusStore.isTxERC20Approve ? "leo.lock.open" : "leo.send.filled")
      }
    case .signed:
      Image(braveSystemName: "leo.arrow.up")
    case .confirmed:
      Image(systemName: "checkmark")
    case .error:
      Image(systemName: "xmark")
    default:
      EmptyView()
    }
  }

  @ViewBuilder private var explorerButton: some View {
    Button {
      Task {
        if let url = await txStatusStore.explorerLink() {
          openWalletURL(url)
        }
      }
    } label: {
      if txStatusStore.activeTxStatus == .signed {
        HStack {
          Text(Strings.Wallet.viewOnBlockExplorer)
          Image(braveSystemName: "leo.launch")
        }
        .foregroundColor(Color(braveSystemName: .iconInteractive))
        .font(.footnote.bold())
      } else {
        Image(braveSystemName: "leo.arrow.diagonal-up-right")
          .font(.subheadline)
          .foregroundColor(Color(braveSystemName: .iconInteractive))
      }
    }
  }

  @ViewBuilder private var txStatusIconView: some View {
    switch txStatusStore.activeTxStatus {
    case .submitted:
      ZStack {
        Group {
          Circle()
            .stroke(
              Color(braveSystemName: .primary20),
              lineWidth: 6
            )
          Circle()
            .trim(from: 0, to: 0.25)
            .stroke(
              Color(braveSystemName: .primary40),
              style: StrokeStyle(lineWidth: 6, lineCap: .round)
            )
            .rotationEffect(isSpinning ? .degrees(360) : .degrees(0))
            .animation(
              .easeInOut(duration: 0.5)
                .repeatForever(autoreverses: false),
              value: isSpinning
            )
        }
        .frame(width: 100, height: 100)
        statusIcon
          .font(.title)
          .foregroundColor(Color(braveSystemName: .primary40))
      }
    case .signed:
      ZStack {
        Circle()
          .stroke(Color(braveSystemName: .primary40), lineWidth: 6)
          .frame(width: 100, height: 100)
        statusIcon
          .font(.title)
          .foregroundColor(Color(braveSystemName: .primary40))
      }
    case .error:
      statusIcon
        .font(.title.weight(.semibold))
        .foregroundColor(Color(braveSystemName: .systemfeedbackErrorIcon))
        .padding(24)
        .background(
          Color(braveSystemName: .systemfeedbackErrorBackground)
            .clipShape(Circle())
        )
    case .confirmed:
      ZStack {
        Group {
          Group {
            Circle()
              .stroke(Color(braveSystemName: .green20), lineWidth: 6)
            Circle()
              .trim(from: 0, to: 0.25)
              .stroke(
                Color(braveSystemName: .green40),
                style: StrokeStyle(lineWidth: 6, lineCap: .round)
              )
              .rotationEffect(isConfirmSpinning ? .degrees(360) : .degrees(0))
          }
          .frame(width: 100, height: 100)
        }
        .hidden(isHidden: isConfirmSpinningFinished)
        statusIcon
          .font(.title.weight(.semibold))
          .foregroundColor(Color(braveSystemName: .green30))
          .padding(24)
          .background(
            Color(braveSystemName: .green10)
              .clipShape(Circle())
              .frame(width: 100, height: 100)
          )
          .hidden(isHidden: !isConfirmSpinningFinished)
          .transition(.opacity)
      }
    default:
      EmptyView()
    }
  }

  @ViewBuilder private var txStatusTextView: some View {
    switch txStatusStore.activeTxStatus {
    case .submitted:
      VStack {
        Text(txStatusStore.isConfirmingCancelTx ? "Cancelling transaction" : Strings.Wallet.submittedTransactionTitle)
        .font(.title2.weight(.semibold))
        .padding(.bottom, 8)
        VStack {
          if !txStatusStore.isConfirmingCancelTx {
            Group {
              Text(txStatusStore.isTxERC20Approve ? "Approving " : "Sending ")
                + Text(txStatusStore.txValueInfo)
                .fontWeight(.semibold)
                + Text(" to ")
            }
            .font(.subheadline)
            .foregroundColor(Color(braveSystemName: .textSecondary))
          }
          HStack {
            Text(
              txStatusStore.isConfirmingCancelTx ? "Cancellation request submitted" : txStatusStore.txToAddress.truncatedAddress
            )
            .font(.subheadline.weight(.semibold))
            .foregroundColor(Color(braveSystemName: .textSecondary))
            explorerButton
          }
        }
      }
      .foregroundColor(Color(braveSystemName: .textPrimary))
      .multilineTextAlignment(.center)
    case .signed:
      VStack {
        Text(Strings.Wallet.signedTransactionTitle)
          .font(.title2.weight(.semibold))
          .padding(.bottom, 8)
        VStack(spacing: 8) {
          Text("Sending ")
            + Text(txStatusStore.txValueInfo)
            .fontWeight(.semibold)
          Text("Transaction has been signed and will be sent to network by dapps and await for confirmation.")
        }
        .font(.subheadline)
        .foregroundColor(Color(braveSystemName: .textSecondary))
      }
      .foregroundColor(Color(braveSystemName: .textPrimary))
      .multilineTextAlignment(.center)
    case .confirmed:
      VStack {
        Text(txStatusStore.isConfirmingCancelTx ? "Transaction canceled" : "Sent!")
          .font(.title2.weight(.semibold))
          .padding(.bottom, 8)
        VStack {
          Group {
            if txStatusStore.isConfirmingCancelTx {
              Text("Figure out the text")
            } else {
              Text(txStatusStore.txValueInfo)
                .fontWeight(.semibold)
                + Text(" has been sent to account")
            }
          }
          .font(.subheadline)
          HStack {
            Text(
              txStatusStore.txToAddress.truncatedAddress
            )
            .font(.subheadline.weight(.semibold))
            .foregroundColor(Color(braveSystemName: .textSecondary))
            explorerButton
          }
        }
      }
      .foregroundColor(Color(braveSystemName: .textPrimary))
      .multilineTextAlignment(.center)
    case .error:
      VStack {
        Text("Unable to send")
          .font(.title2.weight(.semibold))
          .padding(.bottom, 8)
        VStack {
          Group {
            Text("There was an error attempting to send ")
              + Text(txStatusStore.txValueInfo)
              .fontWeight(.semibold)
              + Text(" to ")
          }
          .font(.subheadline)
          .foregroundColor(Color(braveSystemName: .textSecondary))
          HStack {
            Text(
              txStatusStore.txToAddress.truncatedAddress
            )
            .font(.subheadline.weight(.semibold))
            .foregroundColor(Color(braveSystemName: .textSecondary))
            explorerButton
          }
        }
      }
      .foregroundColor(Color(braveSystemName: .textPrimary))
      .multilineTextAlignment(.center)
      .padding(.top, 10)
      if let txProviderError = txStatusStore.txProviderError {
        Text("\(txProviderError.code)")
          .font(.title3.weight(.semibold))
          .foregroundColor(Color(braveSystemName: .textSecondary))
          .multilineTextAlignment(.center)
      }
    default:
      EmptyView()
    }
  }

  @ViewBuilder private var buttonContainerView: some View {
    switch txStatusStore.activeTxStatus {
    case .submitted:
      HStack(spacing: 16) {
        if txStatusStore.isSpeedUpOrCancelAvailable {
          Button {
            isShowingCancelConfirmation = true
          } label: {
            Text("Cancel Transaction")
              .font(.footnote.weight(.semibold))
              .frame(maxWidth: .infinity)
          }
          .buttonStyle(
            BraveOutlineButtonStyle(
              size: .large,
              enabledTextColor: UIColor(braveSystemName: .textInteractive),
              enabledOutlineColor: UIColor(braveSystemName: .textInteractive)
            )
          )
        }
        Button {
        } label: {
          Text("View in Activity")
            .font(.footnote.weight(.semibold))
            .frame(maxWidth: .infinity)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
      }
    case .signed:
      explorerButton
    case .confirmed:
      Button(action: onDismiss) {
        Text("View in Activity")
          .frame(maxWidth: .infinity)
      }
      .buttonStyle(BraveFilledButtonStyle(size: .large))
    case .error:
      Button(action: onDismiss) {
        Text(Strings.Wallet.confirmedTransactionCloseButtonTitle)
          .frame(maxWidth: .infinity)
      }
      .buttonStyle(BraveFilledButtonStyle(size: .large))
    default:
      EmptyView()
    }
  }

  @ViewBuilder private var submittedTxView: some View {
    GeometryReader { geometry in
      ScrollView(.vertical) {
        VStack(spacing: 10) {
          if txStatusStore.isSpeedUpOrCancelAvailable {
            HStack {
              Text("Take longer than expected?")
                .font(.footnote)
                .foregroundColor(Color(braveSystemName: .systemfeedbackInfoText))
              Spacer()
              Button {
              } label: {
                Text("Speed up")
                  .font(.footnote.weight(.semibold))
              }
              .buttonStyle(
                BraveOutlineButtonStyle(
                  size: .small,
                  enabledTextColor: UIColor(braveSystemName: .textInteractive),
                  enabledOutlineColor: UIColor(braveSystemName: .dividerInteractive)
                )
              )
            }
            .padding(16)
            .background(
              Color(braveSystemName: .systemfeedbackInfoBackground)
                .cornerRadius(8)
            )
          }
          Spacer()
          txStatusIconView
            .onAppear {
              isSpinning = true
            }
            .padding(.bottom, 30)
          txStatusTextView
            .padding(.top, 10)
          Spacer()
          Text("You can safely dismiss this window.")
            .font(.caption)
            .foregroundColor(Color(braveSystemName: .textTertiary))
            .multilineTextAlignment(.center)
            .padding(.bottom, 8)
          buttonContainerView
        }
        .frame(maxWidth: .infinity, minHeight: geometry.size.height)
        .padding(.horizontal, 24)
      }
    }
  }

  @ViewBuilder private var confirmedTxView: some View {
    GeometryReader { geometry in
      ScrollView(.vertical) {
        VStack {
          Spacer()
          txStatusIconView
            .onAppear {
              withAnimation(.easeIn(duration: 0.5)) {
                isConfirmSpinning = true
              }
              withAnimation(.linear(duration: 0.75).delay(0.5)) {
                isConfirmSpinningFinished = true
              }
            }
          txStatusTextView
            .padding(.top, 10)
          Spacer()
          buttonContainerView
            .padding(.top, 40)
        }
        .frame(maxWidth: .infinity, minHeight: geometry.size.height)
        .padding(.horizontal, 24)
      }
    }
  }

  @ViewBuilder private var failedTxView: some View {
    GeometryReader { geometry in
      ScrollView(.vertical) {
        VStack {
          Spacer()
          txStatusIconView
            .onAppear {
              withAnimation(.easeIn(duration: 0.5)) {
                isConfirmSpinning = true
              }
              withAnimation(.linear(duration: 0.75).delay(0.5)) {
                isConfirmSpinningFinished = true
              }
            }
          txStatusTextView
            .padding(.top, 10)
          Spacer()
          buttonContainerView
            .padding(.top, 40)
        }
        .frame(maxWidth: .infinity, minHeight: geometry.size.height)
        .padding(.horizontal, 24)
      }
    }
  }

  @ViewBuilder private var signedTxView: some View {
    GeometryReader { geometry in
      ScrollView(.vertical) {
        VStack {
          Spacer()
          txStatusIconView
            .padding(.bottom, 32)
          txStatusTextView
          Spacer()
          Text("You can safely dismiss this window.")
            .font(.caption)
            .foregroundColor(Color(braveSystemName: .textTertiary))
            .multilineTextAlignment(.center)
          Spacer()
          buttonContainerView
            .padding(.bottom, 28)
        }
        .frame(maxWidth: .infinity, minHeight: geometry.size.height)
        .padding(.horizontal, 24)
      }
    }
  }

  @ViewBuilder private var cancelConfirmationView: some View {
    VStack {
      Spacer()
      VStack {
        Text("Cancel transaction?")
          .font(.title2.weight(.semibold))
        Text("Gas fees are needed to create a new transaction that cancels your pending crypto transaction.")
          .font(.subheadline)
      }
      .foregroundColor(Color(braveSystemName: .textPrimary))
      .multilineTextAlignment(.center)
      .padding(.horizontal, 16)
      Spacer()
      HStack(spacing: 8) {
        Button {
          isShowingCancelConfirmation = false
        } label: {
          Text("Back")
            .font(.footnote.weight(.semibold))
            .frame(maxWidth: .infinity)
        }
        .buttonStyle(
          BraveOutlineButtonStyle(
            size: .large,
            enabledTextColor: UIColor(braveSystemName: .textInteractive),
            enabledOutlineColor: UIColor(braveSystemName: .textInteractive)
          )
        )
        Button {
          Task { @MainActor in
            pauseAnimation()
            let success = await txStatusStore.cancelActiveTx()
            if success {
              isShowingCancelConfirmation = false
              isShowingPendingCancelTx = true
            } else {
              isShowingCancelConfirmation = false
              isShowingCancelError = true
            }
          }
        } label: {
          Text("Continue")
            .font(.footnote.weight(.semibold))
            .frame(maxWidth: .infinity)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
      }
    }
    .padding(16)
  }

  var body: some View {
    NavigationStack {
      Group {
        if isShowingCancelConfirmation {
          cancelConfirmationView
        } else if isShowingPendingCancelTx {
          PendingCancelTxContainer(
            txStatusStore: txStatusStore,
            onConfirmationCompletion: { success in
              if success {
                // pending cancel tx confirmed
                isShowingCancelConfirmation = false
                isShowingPendingCancelTx = false
              } else {
                print("Debug - failed to confirm cancel tx")
                onDismiss()
              }
            },
            onRejectionCompletion: { success in
              isShowingCancelConfirmation = false
              isShowingPendingCancelTx = false
            }
          )
        } else {
          switch txStatusStore.activeTxStatus {
          case .submitted:
            submittedTxView
          case .signed:
            signedTxView
          case .confirmed:
            confirmedTxView
          case .error:
            failedTxView
          default:
            EmptyView()
          }
        }
      }
      .interactiveDismissDisabled()
      .background(Color(braveSystemName: .containerBackground).ignoresSafeArea())
      .transparentNavigationBar()
      .toolbar {
        ToolbarItemGroup(placement: .destructiveAction) {
          Button {
            onDismiss()
          } label: {
            Image(braveSystemName: "leo.close")
              .foregroundColor(Color(braveSystemName: .iconDefault))
          }
          .hidden(isHidden: isShowingCancelConfirmation || isShowingPendingCancelTx)
        }
      }
      .toolbar {
        ToolbarItem(placement: .principal) {
          if isShowingCancelConfirmation {
            Group {
              Text(txStatusStore.isTxERC20Approve ? "Approving " : "Sending ")
              + Text(txStatusStore.txValueInfo)
                .fontWeight(.semibold)
              + Text(" to ")
              + Text(txStatusStore.txToAddress.truncatedAddress)
                .fontWeight(.semibold)
            }
            .font(.subheadline)
            .foregroundColor(Color(braveSystemName: .textSecondary))
          } else if isShowingPendingCancelTx {
            Text("Submit Cancellation")
          } else {
            EmptyView()
          }
        }
      }
      .navigationBarTitleDisplayMode(.inline)
      .alert(isPresented: $isShowingCancelError) {
        Alert(
          title: Text("Cancel failed"),
          message: nil,
          dismissButton: .default(Text(Strings.OKString))
        )
      }
    }
  }
  
  private func pauseAnimation() {
    isSpinning = false
    isConfirmSpinning = false
    isConfirmSpinningFinished = false
  }
}

struct PendingCancelTxContainer: View {
  @ObservedObject var txStatusStore: TransactionStatusStore
  @Environment(\.presentationMode) var presentationMode: Binding<PresentationMode>
  var onConfirmationCompletion: (_ success: Bool) -> Void
  var onRejectionCompletion: (_ success: Bool) -> Void

  @State private var isPresentingEditSpeedPriority: Bool = false
  @State private var isPresentingEditSpeedPriorityError: Bool = false

  var body: some View {
    Group {
      if let pendingCancelParsedTx = txStatusStore.pendingCancelParsedTx {
        VStack {
          PendingCancelTxView(
            fromAccount: pendingCancelParsedTx.fromAccountInfo,
            gasToken: pendingCancelParsedTx.network.nativeToken,
            gasFee: pendingCancelParsedTx.gasFee,
            isPresentingEditSpeedPriority: $isPresentingEditSpeedPriority
          )
          Spacer()
          HStack(spacing: 8) {
            Button {
              Task { @MainActor in
                let success = await txStatusStore.rejectCancellation()
                onRejectionCompletion(success)
              }
            } label: {
              Text("Reject")
                .font(.footnote.weight(.semibold))
                .frame(maxWidth: .infinity)
            }
            .buttonStyle(
              BraveOutlineButtonStyle(
                size: .large,
                enabledTextColor: UIColor(braveSystemName: .textInteractive),
                enabledOutlineColor: UIColor(braveSystemName: .dividerInteractive)
              )
            )
            Button {
              Task { @MainActor in
                if let errMsg = await txStatusStore.confirmCancellation() {
                  print("DEBUG - cancellation tx failed: \(errMsg)")
                  onConfirmationCompletion(false)
                } else {
                  print("DEBUG - cancellation tx confirmed")
                  onConfirmationCompletion(true)
                }
              }
            } label: {
              Text("Approve")
                .font(.footnote.weight(.semibold))
                .frame(maxWidth: .infinity)
            }
            .buttonStyle(BraveFilledButtonStyle(size: .large))
          }
          .padding(.horizontal, 16)
        }
      } else {
        ProgressView()
          .progressViewStyle(
            .braveCircular(
              size: .large,
              tint: UIColor(
                braveSystemName: .iconInteractive
              )
            )
          )
          .frame(maxWidth: .infinity, maxHeight: .infinity)
      }
    }
    .background(
      Color(braveSystemName: .neutral10)
        .ignoresSafeArea()
    )
    .background(
      Color.clear
        .sheet(isPresented: $isPresentingEditSpeedPriority) {
          if let pendingCancelTx = txStatusStore.pendingCancelTx,
            let gasEstimation = pendingCancelTx.txDataUnion.ethTxData1559?.gasEstimation
          {
            EditSpeedPriorityView(
              transaction: pendingCancelTx,
              gasEstimation: gasEstimation,
              gasAssetRatio: txStatusStore.gasRatio,
              currencyFormatter: txStatusStore.currencyFormatter,
              onUpdate: { priority, transaction, gasEstimation in
                Task { @MainActor in
                  let status =
                    await txStatusStore.updateSpeedPriority(
                      priority,
                      transaction: transaction,
                      gasEstimation: gasEstimation
                    )
                  if !status {
                    isPresentingEditSpeedPriorityError = true
                  }
                }
              }
            )
            .presentationDetents([
              .fraction(0.4),
              .large,
            ])
          }
        }
    )
    .alert(isPresented: $isPresentingEditSpeedPriority) {
      Alert(
        title: Text(Strings.Wallet.unknownError),
        message: Text(Strings.Wallet.editTransactionError),
        dismissButton: .default(Text(Strings.Wallet.editTransactionErrorCTA))
      )
    }
  }
}

struct PendingCancelTxView: View {
  let fromAccount: BraveWallet.AccountInfo
  let gasToken: BraveWallet.BlockchainToken
  let gasFee: GasFee?

  @Binding var isPresentingEditSpeedPriority: Bool

  @ScaledMetric private var blockieSize = 12
  @ScaledMetric private var copyIconSize = 10
  @ScaledMetric private var assetIconSize = 40

  var body: some View {
    List {
      Section {
        HStack {
          Spacer()
          VStack(spacing: 16) {
            Image(braveSystemName: "leo.brave.icon-monochrome")
              .imageScale(.large)
              .font(.system(size: 24))
              .foregroundColor(Color(.braveOrange))
              .padding(12)
              .background(
                Color(.white)
                  .clipShape(RoundedRectangle(cornerRadius: 16, style: .continuous))
              )
            Group {
              Text("Requested from ")
                .foregroundColor(Color(braveSystemName: .textTertiary))
                + Text("Brave Wallet")
                .foregroundColor(Color(braveSystemName: .textPrimary))
                .fontWeight(.semibold)
            }
            .font(.caption)
          }
          Spacer()
        }
      }
      .listRowInsets(.zero)
      .listRowBackground(Color(.clear))
      Section {
        HStack(spacing: 16) {
          AssetIcon(token: gasToken, network: nil)
            .frame(width: assetIconSize, height: assetIconSize)
          VStack(alignment: .leading) {
            HStack {
              Text("Estimated fee")
                .font(.caption.weight(.semibold))
                .foregroundColor(Color(braveSystemName: .textSecondary))
              Spacer()
              Button {
                UIPasteboard.general.string = fromAccount.address
              } label: {
                HStack(spacing: 2) {
                  Blockie(address: fromAccount.address)
                    .frame(width: blockieSize, height: blockieSize)
                  Text(fromAccount.name)
                    .font(.caption2)
                    .foregroundColor(Color(braveSystemName: .textSecondary))
                  Image(braveSystemName: "leo.copy")
                    .resizable()
                    .frame(width: copyIconSize, height: copyIconSize)
                    .foregroundColor(Color(braveSystemName: .iconDefault))
                    .padding(2)
                }
                .padding(4)
                .background(
                  Color(braveSystemName: .neutral10)
                    .clipShape(RoundedRectangle(cornerRadius: 4, style: .continuous))
                )
              }
            }
            if let gasFee {
              Text("- \(gasFee.fee) \(gasToken.symbol)")
                .font(.title2.weight(.semibold))
                .foregroundColor(Color(braveSystemName: .textPrimary))
              Text(gasFee.fiat)
                .font(.subheadline)
                .foregroundColor(Color(braveSystemName: .textPrimary))
            }
          }
        }
      }
      Section {
        HStack {
          Button {
            isPresentingEditSpeedPriority = true
          } label: {
            Text("Speed priority")
              .font(.subheadline.weight(.semibold))
              .foregroundColor(Color(braveSystemName: .textPrimary))
          }
          Spacer()
          Text("Standard")
            .font(.footnote.weight(.semibold))
            .foregroundColor(Color(braveSystemName: .textInteractive))
        }
      }
      Section {
        HStack(spacing: 8) {
          Spacer()
          Text("This gas fee will replace the original.")
            .font(.caption)
            .foregroundColor(Color(braveSystemName: .textTertiary))
          Button {
          } label: {
            Image(braveSystemName: "leo.info.outline")
              .foregroundColor(Color(braveSystemName: .iconDefault))
          }
          Spacer()
        }
      }
      .listRowInsets(.zero)
      .listRowBackground(Color(.clear))
    }
  }
}

#if DEBUG
struct TransactionStatusView_Previews: PreviewProvider {
  static var previews: some View {
    TransactionStatusView(
      txStatusStore: .previewStore,
      networkStore: .previewStore,
      onDismiss: {}
    )
  }
}
#endif
