// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import SwiftUI

class SignTransactionRequestUnion {
  let id: Int32
  let chainId: String
  let originInfo: BraveWallet.OriginInfo
  let coin: BraveWallet.CoinType
  let fromAddress: String
  let txDatas: [BraveWallet.TxDataUnion]
  let rawMessage: [BraveWallet.ByteArrayStringUnion]
  
  init(
    id: Int32,
    chainId: String,
    originInfo: BraveWallet.OriginInfo,
    coin: BraveWallet.CoinType,
    fromAddress: String,
    txDatas: [BraveWallet.TxDataUnion],
    rawMessage: [BraveWallet.ByteArrayStringUnion]
  ) {
    self.id = id
    self.chainId = chainId
    self.originInfo = originInfo
    self.coin = coin
    self.fromAddress = fromAddress
    self.txDatas = txDatas
    self.rawMessage = rawMessage
  }
}

struct SignTransactionView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  
  enum Request {
    case signTransaction([BraveWallet.SignTransactionRequest])
    case signAllTransactions([BraveWallet.SignAllTransactionsRequest])
  }
  
  var request: Request
  var cryptoStore: CryptoStore
  var onDismiss: () -> Void
  
  @State private var txIndex: Int = 0
  @State private var showWarning: Bool = true
  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.colorScheme) private var colorScheme
  @Environment(\.openURL) private var openWalletURL
  @ScaledMetric private var blockieSize = 54
  private let maxBlockieSize: CGFloat = 108
  private let normalizedRequests: [SignTransactionRequestUnion]
  
  init(
    keyringStore: KeyringStore,
    networkStore: NetworkStore,
    request: Request,
    cryptoStore: CryptoStore,
    onDismiss: @escaping () -> Void
  ) {
    self.keyringStore = keyringStore
    self.networkStore = networkStore
    self.request = request
    self.cryptoStore = cryptoStore
    self.onDismiss = onDismiss
    switch self.request {
    case .signTransaction(let requests):
      self.normalizedRequests = requests.map {
        SignTransactionRequestUnion(
          id: $0.id,
          chainId: $0.chainId,
          originInfo: $0.originInfo,
          coin: $0.coin,
          fromAddress: $0.fromAddress,
          txDatas: [$0.txData],
          rawMessage: [$0.rawMessage]
        )
      }
    case .signAllTransactions(let requests):
      self.normalizedRequests = requests.map {
        SignTransactionRequestUnion(
          id: $0.id,
          chainId: $0.chainId,
          originInfo: $0.originInfo,
          coin: $0.coin,
          fromAddress: $0.fromAddress,
          txDatas: $0.txDatas,
          rawMessage: $0.rawMessages
        )
      }
    }
  }
  
  var navigationTitle: String {
    switch request {
    case .signTransaction:
      return Strings.Wallet.signTransactionTitle
    case .signAllTransactions:
      return Strings.Wallet.signAllTransactionsTitle
    }
  }
  
  private var currentRequest: SignTransactionRequestUnion {
    normalizedRequests[txIndex]
  }
  
  private var network: BraveWallet.NetworkInfo? {
    networkStore.allChains.first(where: { $0.chainId == currentRequest.chainId })
  }

  private func instructionsDisplayString() -> String {
    currentRequest.txDatas
      .map { $0.solanaTxData?.instructions ?? [] }
      .map { instructionsForOneTx in
        instructionsForOneTx
          .map { TransactionParser.parseSolanaInstruction($0).toString }
          .joined(separator: "\n\n====\n\n") // separator between each instruction
      }
      .joined(separator: "\n\n\n\n") // separator between each transaction
  }

  private var account: BraveWallet.AccountInfo {
    keyringStore.allAccounts.first(where: { $0.address == currentRequest.fromAddress }) ?? keyringStore.selectedAccount
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
            if normalizedRequests.count > 1 {
              HStack {
                Spacer()
                Text(String.localizedStringWithFormat(Strings.Wallet.transactionCount, txIndex + 1, normalizedRequests.count))
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
            Blockie(address: account.address)
              .frame(width: min(blockieSize, maxBlockieSize), height: min(blockieSize, maxBlockieSize))
            AddressView(address: account.address) {
              Text(account.name)
            }
            .foregroundColor(Color(.bravePrimary))
            .font(.callout)
            Text(originInfo: currentRequest.originInfo)
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
          divider
            .padding(.vertical, 8)
          VStack(alignment: .leading) {
            StaticTextView(text: instructionsDisplayString())
              .frame(maxWidth: .infinity)
              .frame(height: 200)
              .background(Color(.tertiaryBraveGroupedBackground))
              .clipShape(RoundedRectangle(cornerRadius: 5, style: .continuous))
              .padding()
          }
          .background(
            Color(.secondaryBraveGroupedBackground)
          )
          .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
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
      Button(action: { // Continue
        showWarning = false
      }) {
        Text(Strings.Wallet.continueButtonTitle)
          .imageScale(.large)
      }
      .buttonStyle(BraveFilledButtonStyle(size: .large))
      .disabled(txIndex != 0)
    } else {
      cancelButton
      Button(action: { // approve
        switch request {
        case .signTransaction(_):
          cryptoStore.handleWebpageRequestResponse(.signTransaction(approved: true, id: currentRequest.id))
          
        case .signAllTransactions(_):
          cryptoStore.handleWebpageRequestResponse(.signAllTransactions(approved: true, id: currentRequest.id))
        }
        if normalizedRequests.count == 1 {
          onDismiss()
        }
      }) {
        Label(Strings.Wallet.sign, braveSystemImage: "leo.key")
          .fixedSize(horizontal: true, vertical: false)
          .imageScale(.large)
      }
      .buttonStyle(BraveFilledButtonStyle(size: .large))
      .disabled(txIndex != 0)
    }
  }
  
  @ViewBuilder private var cancelButton: some View {
    Button(action: { // cancel
      switch request {
      case .signTransaction(_):
        cryptoStore.handleWebpageRequestResponse(.signTransaction(approved: false, id: currentRequest.id))
      case .signAllTransactions(_):
        cryptoStore.handleWebpageRequestResponse(.signAllTransactions(approved: false, id: currentRequest.id))
      }
      if normalizedRequests.count == 1 {
        onDismiss()
      }
    }) {
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
        Button(action: {
          openWalletURL(WalletConstants.signTransactionRiskLink)
        }) {
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
    if txIndex + 1 < normalizedRequests.count {
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
      request: .signTransaction([BraveWallet.SignTransactionRequest(
        originInfo: .init(),
        id: 0,
        from: BraveWallet.AccountInfo.previewAccount.accountId,
        fromAddress: BraveWallet.AccountInfo.previewAccount.address,
        txData: .init(),
        rawMessage: .init(),
        coin: .sol,
        chainId: BraveWallet.SolanaMainnet
      )]),
      cryptoStore: .previewStore,
      onDismiss: {}
    )
    .previewColorSchemes()
  }
}
#endif
