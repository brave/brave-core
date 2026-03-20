// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import SwiftUI

enum SignTransactionRequestItem {
  case solana(BraveWallet.SignSolTransactionsRequest)
  case cardano(BraveWallet.SignCardanoTransactionRequest)
}

struct SignTransactionRequests {
  let items: [SignTransactionRequestItem]
}

struct SignTransactionView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore

  var requests: SignTransactionRequests
  var cryptoStore: CryptoStore
  var onDismiss: () -> Void

  @State private var txIndex: Int = 0
  @State private var showWarning: Bool = true
  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.colorScheme) private var colorScheme
  @Environment(\.openURL) private var openWalletURL
  @ScaledMetric private var blockieSize = 54
  private let maxBlockieSize: CGFloat = 108

  private enum ViewMode: Int {
    case transaction
    case details
  }

  @State private var viewMode: ViewMode = .details

  init(
    keyringStore: KeyringStore,
    networkStore: NetworkStore,
    solRequests: [BraveWallet.SignSolTransactionsRequest]?,
    cardanoRequests: [BraveWallet.SignCardanoTransactionRequest]?,
    cryptoStore: CryptoStore,
    onDismiss: @escaping () -> Void
  ) {
    self.keyringStore = keyringStore
    self.networkStore = networkStore
    self.cryptoStore = cryptoStore
    self.onDismiss = onDismiss

    let items: [SignTransactionRequestItem] =
      (solRequests ?? []).map { .solana($0) } + (cardanoRequests ?? []).map { .cardano($0) }
    self.requests = SignTransactionRequests(items: items)
  }

  var navigationTitle: String {
    switch currentRequest {
    case .solana(let signSolTransactionsRequest):
      return signSolTransactionsRequest.txDatas.count > 1
        ? Strings.Wallet.signAllTransactionsTitle : Strings.Wallet.signTransactionTitle
    case .cardano(_):
      return Strings.Wallet.signTransactionTitle
    }
  }

  private var currentRequest: SignTransactionRequestItem {
    requests.items[txIndex]
  }

  private var network: BraveWallet.NetworkInfo? {
    switch currentRequest {
    case .solana(let signSolTransactionsRequest):
      return networkStore.allChains.first(
        where: { $0.chainId == signSolTransactionsRequest.chainId.chainId }
      )
    case .cardano(let signCardanoTransactionRequest):
      return networkStore.allChains.first(
        where: { $0.chainId == signCardanoTransactionRequest.chainId.chainId }
      )
    }
  }

  private func instructionsDisplayString() -> String {
    switch currentRequest {
    case .solana(let signSolTransactionsRequest):
      return signSolTransactionsRequest.txDatas
        .map { $0.instructions }
        .map { instructionsForOneTx in
          instructionsForOneTx
            .map { TransactionParser.parseSolanaInstruction($0).toString }
            .joined(separator: "\n\n====\n\n")  // separator between each instruction
        }
        .joined(separator: "\n\n\n\n")  // separator between each transaction
    case .cardano(let signCardanoTransactionRequest):
      let inputDetails = signCardanoTransactionRequest.inputs
        .map { input in
          var details: [String] = []
          details.append(
            "\(Strings.Wallet.inputLabel):\n\(input.outpointTxid):\(input.outpointIndex)"
          )
          details.append("\(Strings.Wallet.valueLabel):\n\(input.value)")

          // Add tokens if present
          for token in input.tokens {
            details.append(
              "\(Strings.Wallet.signCardanoTxRequestDetailsTokenLabel):\n\(token.tokenIdHex):\(token.value)"
            )
          }

          details.append(
            "\(Strings.Wallet.signCardanoTxRequestDetailsAddressLabel):\n\(input.address)"
          )

          return details.joined(separator: "\n\n")
        }
        .joined(separator: "\n\n====\n\n")  // separator between each input

      let outputDetails = signCardanoTransactionRequest.outputs
        .map { output in
          var details: [String] = []
          details.append(
            "\(Strings.Wallet.signCardanoTxRequestDetailsAddressLabel):\n\(output.address)"
          )
          details.append("\(Strings.Wallet.valueLabel):\n\(output.value)")

          for token in output.tokens {
            details.append(
              "\(Strings.Wallet.signCardanoTxRequestDetailsTokenLabel):\n\(token.tokenIdHex):\(token.value)"
            )
          }

          return details.joined(separator: "\n\n")
        }
        .joined(separator: "\n\n====\n\n")  // separator between each output

      var sections: [String] = []
      if !inputDetails.isEmpty {
        sections.append("==\(Strings.Wallet.inputLabel)==\n\n\(inputDetails)")
      }
      if !outputDetails.isEmpty {
        sections.append("==\(Strings.Wallet.outputLabel)==\n\n\(outputDetails)")
      }
      return sections.joined(separator: "\n\n\n\n")
    }
  }

  private var account: BraveWallet.AccountInfo {
    switch currentRequest {
    case .solana(let signSolTransactionsRequest):
      return keyringStore.allAccounts.first(
        where: { $0.accountId == signSolTransactionsRequest.fromAccountId }
      ) ?? keyringStore.selectedAccount
    case .cardano(let signCardanoTransactionRequest):
      return keyringStore.allAccounts.first(
        where: { $0.accountId == signCardanoTransactionRequest.accountId }
      ) ?? keyringStore.selectedAccount
    }
  }

  private var currentRequestOriginInfo: BraveWallet.OriginInfo {
    switch currentRequest {
    case .solana(let signSolTransactionsRequest):
      return signSolTransactionsRequest.originInfo
    case .cardano(let signCardanoTransactionRequest):
      return signCardanoTransactionRequest.originInfo
    }
  }

  private var currentRequestId: Int32 {
    switch currentRequest {
    case .solana(let signSolTransactionsRequest):
      return signSolTransactionsRequest.id
    case .cardano(let signCardanoTransactionRequest):
      return signCardanoTransactionRequest.id
    }
  }

  @ViewBuilder
  func signTxRequestStaticTextView(text: String) -> some View {
    VStack(alignment: .leading) {
      StaticTextView(text: text)
        .frame(maxWidth: .infinity)
        .frame(height: 200)
        .background(Color(.tertiaryBraveGroupedBackground))
        .clipShape(RoundedRectangle(cornerRadius: 5, style: .continuous))
    }
    .padding()
  }

  var body: some View {
    ScrollView(.vertical) {
      VStack {
        VStack(spacing: 12) {
          HStack {
            if let network = self.network {
              Text(network.chainName)
                .font(.callout)
                .foregroundColor(Color(.braveLabel))
            }
            Spacer()
            if requests.items.count > 1 {
              HStack {
                Spacer()
                Text(
                  String.localizedStringWithFormat(
                    Strings.Wallet.transactionCount,
                    txIndex + 1,
                    requests.items.count
                  )
                )
                .fontWeight(.semibold)
                Button(action: next) {
                  Text(Strings.Wallet.next)
                    .fontWeight(.semibold)
                    .foregroundColor(Color(.braveBlurpleTint))
                }
              }
            }
          }
          VStack(spacing: 12) {
            Blockie(address: account.blockieSeed)
              .frame(
                width: min(blockieSize, maxBlockieSize),
                height: min(blockieSize, maxBlockieSize)
              )
            AddressView(address: account.address) {
              Text(account.name)
            }
            .foregroundColor(Color(.bravePrimary))
            .font(.callout)
            Text(originInfo: currentRequestOriginInfo)
              .foregroundColor(Color(.braveLabel))
              .font(.subheadline)
              .multilineTextAlignment(.center)
          }
          .accessibilityElement(children: .combine)
          Text(Strings.Wallet.signatureRequestSubtitle)
            .font(.title3.weight(.semibold))
            .foregroundColor(Color(.bravePrimary))
        }
        .padding(.horizontal, 8)
        if showWarning {
          warningView
            .padding(.vertical, 12)
            .padding(.horizontal, 20)
        } else {
          switch currentRequest {
          case .solana(_):
            divider
              .padding(.vertical, 8)
            signTxRequestStaticTextView(text: instructionsDisplayString())
              .background(
                Color(.secondaryBraveGroupedBackground)
              )
              .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
          case .cardano(let signCardanoTransactionRequest):
            // View Mode
            VStack(spacing: 12) {
              Picker("", selection: $viewMode) {
                Text(Strings.Wallet.confirmationViewModeDetails).tag(ViewMode.details)
                Text(Strings.Wallet.confirmationViewModeTransaction).tag(ViewMode.transaction)
              }
              .pickerStyle(SegmentedPickerStyle())
              Group {
                switch viewMode {
                case .transaction:
                  signTxRequestStaticTextView(
                    text: signCardanoTransactionRequest.rawTxData
                  )
                case .details:
                  signTxRequestStaticTextView(
                    text: instructionsDisplayString()
                  )
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
        buttonsContainer
          .padding(.top)
          .opacity(sizeCategory.isAccessibilityCategory ? 0 : 1)
          .accessibility(hidden: sizeCategory.isAccessibilityCategory)
      }
      .padding()
    }
    .navigationBarTitleDisplayMode(.inline)
    .navigationTitle(Text(navigationTitle))
    .background(Color(.braveGroupedBackground).edgesIgnoringSafeArea(.all))
  }

  @ViewBuilder private var buttonsContainer: some View {
    if sizeCategory.isAccessibilityCategory {
      VStack {
        buttons
      }
    } else {
      HStack {
        buttons
      }
    }
  }

  @ViewBuilder private var buttons: some View {
    if showWarning {
      cancelButton
      Button {  // Continue
        showWarning = false
      } label: {
        Text(Strings.Wallet.continueButtonTitle)
          .imageScale(.large)
      }
      .buttonStyle(BraveFilledButtonStyle(size: .large))
      .disabled(txIndex != 0)
    } else {
      cancelButton
      Button {  // approve
        switch currentRequest {
        case .solana(_):
          cryptoStore.handleWebpageRequestResponse(
            .signSolTransactions(approved: true, id: currentRequestId)
          )
        case .cardano(_):
          cryptoStore.handleWebpageRequestResponse(
            .signCardanoTransactions(approved: true, id: currentRequestId)
          )
        }
        if requests.items.count == 1 {
          onDismiss()
        }
      } label: {
        Label(Strings.Wallet.sign, braveSystemImage: "leo.key")
          .fixedSize(horizontal: true, vertical: false)
          .imageScale(.large)
      }
      .buttonStyle(BraveFilledButtonStyle(size: .large))
      .disabled(txIndex != 0)
    }
  }

  @ViewBuilder private var cancelButton: some View {
    Button {  // cancel
      switch currentRequest {
      case .solana(_):
        cryptoStore.handleWebpageRequestResponse(
          .signSolTransactions(approved: false, id: currentRequestId)
        )
      case .cardano(_):
        cryptoStore.handleWebpageRequestResponse(
          .signCardanoTransactions(approved: false, id: currentRequestId)
        )
      }
      if requests.items.count == 1 {
        onDismiss()
      }
    } label: {
      Label(Strings.cancelButtonTitle, systemImage: "xmark")
        .fixedSize(horizontal: true, vertical: false)
        .imageScale(.large)
    }
    .buttonStyle(BraveOutlineButtonStyle(size: .large))
  }

  @ViewBuilder private var divider: some View {
    VStack {
      Text(Strings.Wallet.solanaSignTransactionDetails)
        .font(.subheadline.weight(.semibold))
        .foregroundColor(Color(.bravePrimary))
      HStack {
        LinearGradient(braveGradient: colorScheme == .dark ? .darkGradient02 : .lightGradient02)
      }
      .frame(height: 4)
    }
  }

  @ViewBuilder private var warningView: some View {
    VStack(alignment: .leading, spacing: 8) {
      Group {
        Label(Strings.Wallet.signTransactionSignRisk, systemImage: "exclamationmark.triangle")
          .font(.subheadline.weight(.semibold))
          .foregroundColor(Color(.braveErrorLabel))
          .padding(.top, 12)
        Text(Strings.Wallet.solanaSignTransactionWarning)
          .font(.subheadline)
          .foregroundColor(Color(.braveErrorLabel))
        Button {
          openWalletURL(WalletConstants.signTransactionRiskLink)
        } label: {
          Text(Strings.Wallet.learnMoreButton)
            .font(.subheadline)
            .foregroundColor(Color(.braveBlurpleTint))
        }
        .padding(.bottom, 12)
      }
      .padding(.horizontal, 12)
    }
    .background(
      Color(.braveErrorBackground)
        .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
    )
  }

  private func next() {
    if txIndex + 1 < requests.items.count {
      txIndex += 1
    } else {
      txIndex = 0
    }
  }
}

#if DEBUG
struct SignTransaction_Previews: PreviewProvider {
  static var previews: some View {
    SignTransactionView(
      keyringStore: .previewStore,
      networkStore: .previewStore,
      solRequests: [
        BraveWallet.SignSolTransactionsRequest(
          originInfo: .init(),
          id: 0,
          from: BraveWallet.AccountInfo.previewAccount.accountId,
          txDatas: [.init()],
          rawMessages: [.init()],
          chainId: BraveWallet.ChainId(coin: .sol, chainId: BraveWallet.SolanaMainnet)
        )
      ],
      cardanoRequests: nil,
      cryptoStore: .previewStore,
      onDismiss: {}
    )
    .previewColorSchemes()
  }
}
#endif
