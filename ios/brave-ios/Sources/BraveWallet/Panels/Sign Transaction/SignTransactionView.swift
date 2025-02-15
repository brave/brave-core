// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import SwiftUI

struct SignTransactionView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore

  var requests: [BraveWallet.SignSolTransactionsRequest]
  var cryptoStore: CryptoStore
  var onDismiss: () -> Void

  @State private var txIndex: Int = 0
  @State private var showWarning: Bool = true
  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.colorScheme) private var colorScheme
  @Environment(\.openURL) private var openWalletURL
  @ScaledMetric private var blockieSize = 54
  private let maxBlockieSize: CGFloat = 108

  init(
    keyringStore: KeyringStore,
    networkStore: NetworkStore,
    requests: [BraveWallet.SignSolTransactionsRequest],
    cryptoStore: CryptoStore,
    onDismiss: @escaping () -> Void
  ) {
    self.keyringStore = keyringStore
    self.networkStore = networkStore
    self.requests = requests
    self.cryptoStore = cryptoStore
    self.onDismiss = onDismiss
  }

  var navigationTitle: String {
    return currentRequest.txDatas.count > 1
      ? Strings.Wallet.signAllTransactionsTitle : Strings.Wallet.signTransactionTitle
  }

  private var currentRequest: BraveWallet.SignSolTransactionsRequest {
    requests[txIndex]
  }

  private var network: BraveWallet.NetworkInfo? {
    networkStore.allChains.first(where: { $0.chainId == currentRequest.chainId })
  }

  private func instructionsDisplayString() -> String {
    currentRequest.txDatas
      .map { $0.instructions }
      .map { instructionsForOneTx in
        instructionsForOneTx
          .map { TransactionParser.parseSolanaInstruction($0).toString }
          .joined(separator: "\n\n====\n\n")  // separator between each instruction
      }
      .joined(separator: "\n\n\n\n")  // separator between each transaction
  }

  private var account: BraveWallet.AccountInfo {
    keyringStore.allAccounts.first(where: { $0.accountId == currentRequest.fromAccountId })
      ?? keyringStore.selectedAccount
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
            if requests.count > 1 {
              HStack {
                Spacer()
                Text(
                  String.localizedStringWithFormat(
                    Strings.Wallet.transactionCount,
                    txIndex + 1,
                    requests.count
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
        cryptoStore.handleWebpageRequestResponse(
          .signSolTransactions(approved: true, id: currentRequest.id)
        )
        if requests.count == 1 {
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
      cryptoStore.handleWebpageRequestResponse(
        .signSolTransactions(approved: false, id: currentRequest.id)
      )
      if requests.count == 1 {
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
    if txIndex + 1 < requests.count {
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
      requests: [
        BraveWallet.SignSolTransactionsRequest(
          originInfo: .init(),
          id: 0,
          from: BraveWallet.AccountInfo.previewAccount.accountId,
          txDatas: [.init()],
          rawMessages: [.init()],
          chainId: BraveWallet.SolanaMainnet
        )
      ],
      cryptoStore: .previewStore,
      onDismiss: {}
    )
    .previewColorSchemes()
  }
}
#endif
