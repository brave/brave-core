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
  var onViewInActivity: () -> Void
  var onFollowUpTxCreated: (_ txId: String) -> Void

  @Environment(\.openURL) private var openWalletURL
  @Environment(\.pixelLength) private var pixelLength

  @State private var isSpinning: Bool = false
  @State private var isConfirmSpinning: Bool = false
  @State private var isConfirmSpinningFinished: Bool = false
  @State private var scrollViewContentSize: CGSize = .zero
  @State private var isShowingTxCancellationConfirmation: Bool = false
  @State private var speedUpTimer: Timer?
  @State private var isShowingSpeedUpBanner: Bool = false
  @State private var followUpActionError: String?

  @ViewBuilder private var statusIcon: some View {
    switch txStatusStore.activeTxStatus {
    case .submitted:
      if isTxERC20Approve {
        Image(braveSystemName: "leo.lock.open")
      } else if isTxEthSwap || isTxSolSwap {
        Image(braveSystemName: "leo.swap.horizontal")
      } else {
        Image(braveSystemName: "leo.send.filled")
      }
    case .signed:
      Image(braveSystemName: "leo.arrow.up")
    case .confirmed:
      Image(braveSystemName: "leo.check.normal")
    case .error:
      Image(braveSystemName: "leo.close")
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
      .onAppear {
        isSpinning = true
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
      ZStack {
        Circle()
          .fill(Color(braveSystemName: .systemfeedbackErrorBackground))
          .frame(width: 100, height: 100)
        statusIcon
          .font(.title.weight(.semibold))
          .foregroundColor(Color(braveSystemName: .systemfeedbackErrorIcon))
      }
    case .confirmed:
      ZStack {
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
        .hidden(isHidden: isConfirmSpinningFinished)
        statusIcon
          .font(.title)
          .foregroundColor(Color(braveSystemName: .green30))
          .background(
            Color(braveSystemName: .green10)
              .clipShape(Circle())
              .frame(width: 100, height: 100)
          )
          .hidden(isHidden: !isConfirmSpinningFinished)
          .transition(.opacity)
      }
      .onAppear {
        withAnimation(.easeIn(duration: 0.5)) {
          isConfirmSpinning = true
        }
        withAnimation(.linear(duration: 0.75).delay(0.5)) {
          isConfirmSpinningFinished = true
        }
      }
    default:
      EmptyView()
    }
  }

  private var isTxERC20Approve: Bool {
    txStatusStore.activeParsedTx.transaction.txType == .erc20Approve
  }

  private var isTxSolSwap: Bool {
    txStatusStore.activeParsedTx.transaction.txType == .solanaSwap
  }

  private var isTxEthSwap: Bool {
    txStatusStore.activeParsedTx.transaction.txType == .ethSwap
  }

  private var txFromValue: String {
    switch txStatusStore.activeParsedTx.details {
    case .erc20Transfer(let detail):
      return "\(detail.fromAmount) \(detail.fromToken?.symbol ?? "")"
    case .erc721Transfer(let detail):
      return "\(detail.fromAmount) \(detail.fromToken?.symbol ?? "")"
    case .ethSend(let detail):
      return "\(detail.fromAmount) \(detail.fromToken?.symbol ?? "ETH")"
    case .solSystemTransfer(let detail):
      return "\(detail.fromAmount) \(detail.fromToken?.symbol ?? "SOL")"
    case .solSplTokenTransfer(let detail):
      return "\(detail.fromAmount) \(detail.fromToken?.symbol ?? "")"
    case .solDappTransaction(let detail):
      return "\(detail.fromAmount) \(detail.symbol ?? "")"
    case .filSend(let detail):
      return "\(detail.sendAmount) \(detail.sendToken?.symbol ?? "FIL")"
    case .btcSend(let detail):
      return "\(detail.fromAmount) \(detail.fromToken?.symbol ?? "BTC")"
    case .ethErc20Approve(let detail):
      return "\(detail.approvalAmount) \(detail.token?.symbol ?? "")"
    case .ethSwap(let detail):
      return "\(detail.fromAmount) \(detail.fromToken?.symbol ?? "")"
    case .solSwapTransaction(let detail):
      return "\(detail.fromAmount) \(detail.symbol ?? "")"
    default:
      return ""
    }
  }

  private var txToValue: String {
    if isTxEthSwap {
      if case .ethSwap(let details) = txStatusStore.activeParsedTx.details {
        return "\(details.minBuyAmount) \(details.toToken?.symbol ?? "")"
      }
      return ""
    } else if isTxERC20Approve {
      return txStatusStore.activeParsedTx.fromAccountInfo.name
    } else {
      return txStatusStore.activeParsedTx.toAddress.truncatedAddress
    }
  }

  private var activeTxNetwork: BraveWallet.NetworkInfo? {
    networkStore.allChains.first(where: {
      $0.chainId.caseInsensitiveCompare(
        txStatusStore.activeParsedTx.network.chainId
      ) == .orderedSame
    })
  }

  private func revertFromValue(parsedTx: ParsedTransaction) -> String {
    switch parsedTx.details {
    case .erc20Transfer(let detail):
      return "\(detail.fromAmount) \(detail.fromToken?.symbol ?? "")"
    case .erc721Transfer(let detail):
      return "\(detail.fromAmount) \(detail.fromToken?.symbol ?? "")"
    case .ethSend(let detail):
      return "\(detail.fromAmount) \(detail.fromToken?.symbol ?? "ETH")"
    case .ethErc20Approve(let detail):
      return "\(detail.approvalAmount) \(detail.token?.symbol ?? "")"
    case .ethSwap(let detail):
      return "\(detail.fromAmount) \(detail.fromToken?.symbol ?? "")"
    default:
      return ""
    }
  }

  private func txCancelledMsg(parsedTx: ParsedTransaction) -> String {
    if case .ethErc20Approve(_) = parsedTx.details {
      return Strings.Wallet.cancelERC20ApprovalTxRevertMsg
    } else {
      return Strings.Wallet.cancelTxRevertMsg
    }
  }

  @ViewBuilder private var txStatusTextView: some View {
    switch txStatusStore.activeTxStatus {
    case .submitted:
      if case .cancel(_) = txStatusStore.followUpAction {
        // Cancelling transaction
        // Cancellation request submitted
        VStack {
          Text(Strings.Wallet.cancellingTransactionTitle)
            .font(.title2.weight(.semibold))
            .padding(.bottom, 8)
          HStack {
            Text(Strings.Wallet.cancellingTransactionDescription)
              .font(.subheadline)
            explorerButton
          }
        }
        .foregroundColor(Color(braveSystemName: .textPrimary))
        .multilineTextAlignment(.center)
      } else {
        // Sending (not cancelling)
        //
        // Transaction Submitted
        // Sending 0.1 ETH to Oxabcd
        //
        // Approving ERC20
        //
        // Transaction Submitted
        // Approving 1 DAI on Account 1
        //
        // Swapping on Eth
        //
        // Transaction Submitted
        // Swapping 0.1 ETH to 2.3 USDC
        // on Ethereum mainnet
        //
        // Swapping on Sol
        //
        // Transaction Submitted
        // Solana Swap
        // on Solana Mainnet
        VStack {
          Text(Strings.Wallet.submittedTransactionTitle)
            .font(.title2.weight(.semibold))
            .padding(.bottom, 8)
          Group {
            if isTxSolSwap {
              HStack {
                Text(Strings.Wallet.transactionSummarySolanaSwap)
                  .fontWeight(.semibold)
                explorerButton
              }
              Text("\(Strings.Wallet.txStatusOn) \(activeTxNetwork?.chainName ?? "")")
                .foregroundColor(Color(braveSystemName: .textSecondary))
            } else if isTxERC20Approve {
              Text("\(Strings.Wallet.txStatusApproving) ")
                .foregroundColor(Color(braveSystemName: .textSecondary))
                + Text(txFromValue)
                .fontWeight(.semibold)
                + Text(" \(Strings.Wallet.txStatusOn) ")
                .foregroundColor(Color(braveSystemName: .textSecondary))
              HStack {
                Text(txToValue)
                  .fontWeight(.semibold)
                explorerButton
              }
            } else if isTxEthSwap {
              Text("\(Strings.Wallet.txStatusSwapping) ")
                .foregroundColor(Color(braveSystemName: .textSecondary))
                + Text(txFromValue)
                .fontWeight(.semibold)
                + Text(" \(Strings.Wallet.txStatusTo) ")
                .foregroundColor(Color(braveSystemName: .textSecondary))
                + Text(txToValue)
                .fontWeight(.semibold)
            } else {
              Text("\(Strings.Wallet.txStatusSending) ")
                .foregroundColor(Color(braveSystemName: .textSecondary))
                + Text(txFromValue)
                .fontWeight(.semibold)
                + Text(" \(Strings.Wallet.txStatusTo) ")
                .foregroundColor(Color(braveSystemName: .textSecondary))
              HStack {
                Text(txToValue)
                  .fontWeight(.semibold)
                explorerButton
              }
            }
          }
          .font(.subheadline)
        }
        .foregroundColor(Color(braveSystemName: .textPrimary))
        .multilineTextAlignment(.center)
      }
    case .signed:
      // Signed send tx
      //
      // Transaction Signed
      // Sending 0.1 ETH
      // Transaction has been signed and will be sent to network by dapps and await for confirmation.
      VStack {
        Text(Strings.Wallet.signedTransactionTitle)
          .font(.title2.weight(.semibold))
          .padding(.bottom, 8)
        VStack(spacing: 8) {
          Text("\(Strings.Wallet.txStatusSending) ")
            .foregroundColor(Color(braveSystemName: .textSecondary))
            + Text(txFromValue)
            .fontWeight(.semibold)
          Text(Strings.Wallet.txStatusSignedConfirmedMsg)
            .foregroundColor(Color(braveSystemName: .textSecondary))
        }
        .font(.subheadline)
      }
      .foregroundColor(Color(braveSystemName: .textPrimary))
      .multilineTextAlignment(.center)
    case .confirmed:
      if case .cancel(let parsedTx) = txStatusStore.followUpAction {
        // Transaction cancellation
        //
        // Transaction cancelled!
        // 0.1 ETH has been reverted to account 0xf333...095F
        //
        VStack {
          Text(Strings.Wallet.confirmedTransactionCancellationTitle)
            .font(.title2.weight(.semibold))
            .padding(.bottom, 8)
          Text(revertFromValue(parsedTx: parsedTx))
            .fontWeight(.semibold)
            + Text(" \(txCancelledMsg(parsedTx: parsedTx)) ")
            .foregroundColor(Color(braveSystemName: .textSecondary))
          HStack {
            Text("\(Strings.Wallet.confirmedTransactionCancellationAccount) ")
              .foregroundColor(Color(braveSystemName: .textSecondary))
              + Text(parsedTx.fromAccountInfo.address.truncatedAddress)
              .fontWeight(.semibold)
            explorerButton
          }
        }
        .foregroundColor(Color(braveSystemName: .textPrimary))
        .multilineTextAlignment(.center)
      } else {
        // Sending (not cancelling)
        //
        // Sent!
        // 0.1 ETH has been sent to account Oxabcd
        //
        // Approving ERC20
        //
        // Approved!
        // 1 DAI has been approved on your Account 1
        //
        // Swapping on Eth
        //
        // Completed!
        // 2.3 USDC has been added to your Account 1
        // on Ethereum Mainnet
        //
        // Swapping on Sol
        //
        // Completed!
        // Solana Swap
        // on Solana Mainnet
        VStack {
          if isTxSolSwap {
            Text("\(Strings.Wallet.txStatusCompleted)!")
              .font(.title2.weight(.semibold))
              .padding(.bottom, 8)
            HStack {
              Text(Strings.Wallet.transactionSummarySolanaSwap)
                .fontWeight(.semibold)
              explorerButton
            }
            Text("\(Strings.Wallet.txStatusOn) \(activeTxNetwork?.chainName ?? "")")
              .foregroundColor(Color(braveSystemName: .textSecondary))
          } else if isTxEthSwap {
            Text("\(Strings.Wallet.txStatusCompleted)!")
              .font(.title2.weight(.semibold))
              .padding(.bottom, 8)
            Text(txToValue)
              .fontWeight(.semibold)
              + Text(" \(Strings.Wallet.txStatusSwappedMsg)")
              .foregroundColor(Color(braveSystemName: .textSecondary))
            HStack {
              Text(txStatusStore.activeParsedTx.fromAccountInfo.name)
                .fontWeight(.semibold)
              explorerButton
            }
          } else if isTxERC20Approve {
            Text("\(Strings.Wallet.txStatusCompleted)!")
              .font(.title2.weight(.semibold))
              .padding(.bottom, 8)
            Text(txFromValue)
              .fontWeight(.semibold)
              + Text(" \(Strings.Wallet.txStatusERC20ApprovalMsg)")
              .foregroundColor(Color(braveSystemName: .textSecondary))
            HStack {
              Text(txToValue)
                .fontWeight(.semibold)
              explorerButton
            }
          } else {
            Text("\(Strings.Wallet.sent)!")
              .font(.title2.weight(.semibold))
              .padding(.bottom, 8)
            Text(txFromValue)
              .fontWeight(.semibold)
              + Text(" \(Strings.Wallet.txStatusSentMsg)")
              .foregroundColor(Color(braveSystemName: .textSecondary))
            HStack {
              Text(txToValue)
                .fontWeight(.semibold)
              explorerButton
            }
          }
        }
        .foregroundColor(Color(braveSystemName: .textPrimary))
        .multilineTextAlignment(.center)
      }
    case .error:
      VStack {
        Group {
          if isTxEthSwap || isTxSolSwap {
            Text(Strings.Wallet.txStatusSwapTxErrorTitle)
          } else if isTxERC20Approve {
            Text(Strings.Wallet.txStatusERC20ApprovalTxErrorTitle)
          } else {
            Text(Strings.Wallet.txStatusSendTxErrorTitle)
          }
        }
        .font(.title2.weight(.semibold))
        .foregroundColor(Color(braveSystemName: .textPrimary))
        .padding(.bottom, 8)
        VStack {
          Group {
            if isTxEthSwap || isTxSolSwap {
              Text(Strings.Wallet.txStatusSwapTxErrorMsg)
            } else if isTxERC20Approve {
              Text(Strings.Wallet.txStatusERC20ApprovalTxErrorMsg)
            } else {
              Text(Strings.Wallet.txStatusSendTxErrorMsg)
            }
          }
          .font(.subheadline)
          .foregroundColor(Color(braveSystemName: .textSecondary))
        }
      }
      .multilineTextAlignment(.center)
      if let txProviderError = txStatusStore.txProviderError {
        Text(verbatim: "\(txProviderError.code)")
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
      HStack {
        if txStatusStore.isCancelAvailable {
          Button {
            isShowingTxCancellationConfirmation = true
          } label: {
            Text(Strings.Wallet.cancelTransactionStatusButtonTitle)
              .font(.footnote.weight(.semibold))
              .frame(maxWidth: .infinity)
          }
          .buttonStyle(BraveOutlineButtonStyle(size: .large))
        }
        Button {
          onViewInActivity()
        } label: {
          Text(Strings.Wallet.txStatusViewInActivity)
            .font(.footnote.weight(.semibold))
            .frame(maxWidth: .infinity)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
      }
    case .confirmed:
      Button {
        onViewInActivity()
      } label: {
        Text(Strings.Wallet.txStatusViewInActivity)
          .font(.footnote.weight(.semibold))
          .frame(maxWidth: .infinity)
      }
      .buttonStyle(BraveFilledButtonStyle(size: .large))
    case .signed:
      explorerButton
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

  private var disclosureView: some View {
    Text(Strings.Wallet.txStatusSubmittedDisclosure)
      .font(.caption)
      .foregroundColor(Color(braveSystemName: .textTertiary))
      .multilineTextAlignment(.center)
  }

  private var hideDisclosureView: Bool {
    txStatusStore.activeTxStatus == .error
      || txStatusStore.activeTxStatus == .dropped
      || txStatusStore.activeTxStatus == .confirmed
  }

  private var speedUpView: some View {
    HStack(spacing: 4) {
      Text(Strings.Wallet.speedUpBannerTitle)
        .foregroundColor(Color(braveSystemName: .systemfeedbackInfoText))
        .font(.subheadline)
        .multilineTextAlignment(.leading)
      Spacer()
      Button {
        Task { @MainActor in
          let (txId, error) = await txStatusStore.handleTransactionFollowUpAction(.speedUp)
          if let error {
            followUpActionError = error
          } else if let txId {
            onFollowUpTxCreated(txId)
          }
        }
      } label: {
        Text(Strings.Wallet.speedUpButtonTitle)
          .foregroundColor(Color(braveSystemName: .textInteractive))
          .font(.caption.weight(.semibold))
          .padding(6)
      }
      .overlay {
        RoundedRectangle(cornerRadius: 8)
          .stroke(Color(braveSystemName: .dividerInteractive), lineWidth: pixelLength)
      }
    }
    .padding(16)
    .background(
      Color(braveSystemName: .systemfeedbackInfoBackground)
        .clipShape(RoundedRectangle(cornerRadius: 8))
    )
  }

  var body: some View {
    NavigationStack {
      GeometryReader { geometry in
        ScrollView(.vertical) {
          VStack {
            speedUpView
              .hidden(isHidden: !isShowingSpeedUpBanner)
            Spacer()
            txStatusIconView
              .padding(.bottom, 30)
            txStatusTextView
              .padding(.top, 10)
            Spacer()
            disclosureView
              .padding(.bottom, 8)
              .hidden(isHidden: hideDisclosureView)
            buttonContainerView
          }
          .frame(maxWidth: .infinity, minHeight: geometry.size.height)
          .padding(.horizontal, 24)
        }
      }
      .interactiveDismissDisabled()
      .background(Color(braveSystemName: .containerBackground).ignoresSafeArea())
      .transparentNavigationBar(backButtonDisplayMode: .minimal)
      .toolbar {
        ToolbarItemGroup(placement: .destructiveAction) {
          Button {
            onDismiss()
          } label: {
            Image(braveSystemName: "leo.close")
              .foregroundColor(Color(braveSystemName: .iconDefault))
          }
        }
      }
      .navigationBarTitleDisplayMode(.inline)
      .navigationDestination(
        isPresented: $isShowingTxCancellationConfirmation,
        destination: {
          TxCancellationConfirmationView(
            txStatusStore: txStatusStore,
            onCancelCreationSucceeded: {
              isShowingTxCancellationConfirmation = false
              onFollowUpTxCreated($0)
            },
            cancelError: $followUpActionError
          )
          .padding(16)
        }
      )
    }
    .onChange(of: txStatusStore.activeTxStatus) { _ in
      if txStatusStore.activeTxStatus != .submitted {
        isShowingSpeedUpBanner = false
        speedUpTimer?.invalidate()
        speedUpTimer = nil
      }
    }
    .alert(
      isPresented: Binding(
        get: { followUpActionError != nil },
        set: {
          if !$0 {
            followUpActionError = nil
            isShowingTxCancellationConfirmation = false
          }
        }
      )
    ) {
      Alert(
        title: Text(Strings.genericErrorTitle),
        message: Text(followUpActionError ?? ""),
        dismissButton: .default(Text(Strings.OKString))
      )
    }
    .onAppear {
      if txStatusStore.activeTxStatus == .submitted && txStatusStore.isSpeedUpAvailable {
        speedUpTimer = Timer.scheduledTimer(
          withTimeInterval: 5,
          repeats: false,
          block: { _ in
            isShowingSpeedUpBanner = true
          }
        )
      }
    }
    .onDisappear {
      speedUpTimer?.invalidate()
      speedUpTimer = nil
    }
  }
}

struct TxCancellationConfirmationView: View {
  var txStatusStore: TransactionStatusStore
  var onCancelCreationSucceeded: (_ txId: String) -> Void
  @Binding var cancelError: String?

  var body: some View {
    VStack {
      Spacer()
      Text(Strings.Wallet.cancelTransactionStatusButtonTitle)
        .foregroundColor(Color(braveSystemName: .textPrimary))
        .font(.title2.weight(.semibold))
        .padding(.bottom, 8)
      Text(Strings.Wallet.cancelTransactionStatusConfirmationDescription)
        .foregroundColor(Color(braveSystemName: .textTertiary))
      Spacer()
      Button {
        Task { @MainActor in
          let (txId, error) = await txStatusStore.handleTransactionFollowUpAction(.cancel)
          if let error {
            cancelError = error
          } else if let txId {
            onCancelCreationSucceeded(txId)
          }
        }
      } label: {
        Text(Strings.Wallet.cancelTransactionStatusButtonTitle)
          .frame(maxWidth: .infinity)
      }
      .buttonStyle(BraveFilledButtonStyle(size: .large))
    }
    .multilineTextAlignment(.center)
  }
}

#if DEBUG
struct TransactionStatusView_Previews: PreviewProvider {
  static var previews: some View {
    TransactionStatusView(
      txStatusStore: .previewStore,
      networkStore: .previewStore,
      onDismiss: {},
      onViewInActivity: {},
      onFollowUpTxCreated: { _ in }
    )
  }
}
#endif
