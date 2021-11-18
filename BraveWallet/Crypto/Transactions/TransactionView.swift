/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveCore
import Swift
import struct Shared.Strings

extension BraveWallet.TransactionInfo {
  var isSwap: Bool {
    txData.baseData.to
      .caseInsensitiveCompare(NamedAddresses.swapExchangeProxyAddress) == .orderedSame
  }
}

/// Displays a summary of a given transaction
struct TransactionView: View {
  var info: BraveWallet.TransactionInfo
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  var visibleTokens: [BraveWallet.ERCToken]
  
  var displayAccountCreator: Bool // FIXME: Name sucks
  
  var assetRatios: [String: Double]
  
  private let timeFormatter = RelativeDateTimeFormatter().then {
    $0.unitsStyle = .full
    $0.dateTimeStyle = .numeric
  }
  
  private let numberFormatter = NumberFormatter().then {
    $0.numberStyle = .currency
    $0.currencyCode = "USD"
  }
  
  private func namedAddress(for address: String) -> String {
    NamedAddresses.name(for: address, accounts: keyringStore.keyring.accountInfos)
  }
  
  private var gasFee: (String, fiat: String)? {
    let isEIP1559Transaction = !info.txData.maxPriorityFeePerGas.isEmpty && !info.txData.maxFeePerGas.isEmpty
    let limit = info.txData.baseData.gasLimit
    let formatter = WeiFormatter(decimalFormatStyle: .gasFee(limit: limit.removingHexPrefix, radix: 16))
    let gasFee = isEIP1559Transaction ? info.txData.maxFeePerGas : info.txData.baseData.gasPrice
    if let value = formatter.decimalString(for: gasFee.removingHexPrefix, radix: .hex, decimals: Int(networkStore.selectedChain.decimals)) {
      return (value, {
        guard let doubleValue = Double(value), let assetRatio = assetRatios[networkStore.selectedChain.symbol.lowercased()] else {
          return "$0.00"
        }
        return numberFormatter.string(from: NSNumber(value: doubleValue * assetRatio)) ?? "$0.00"
      }())
    }
    return nil
  }
  
  @ViewBuilder private var title: some View {
    let formatter = WeiFormatter(decimalFormatStyle: .balance)
    switch info.txType {
    case .erc20Approve:
      if info.txArgs.count > 1, let token = visibleTokens.first(where: {
        $0.contractAddress == info.txArgs[0]
      }) {
        Text(String.localizedStringWithFormat(Strings.Wallet.transactionApproveSymbolTitle, formatter.decimalString(for: info.txArgs[1].removingHexPrefix, radix: .hex, decimals: Int(token.decimals)) ?? "", token.symbol))
      } else {
        Text(Strings.Wallet.transactionUnknownApprovalTitle)
      }
    case .ethSend, .other:
      let amount = formatter.decimalString(for: info.txData.baseData.value.removingHexPrefix, radix: .hex, decimals: Int(networkStore.selectedChain.decimals)) ?? ""
      let fiat = numberFormatter.string(from: NSNumber(value: assetRatios[networkStore.selectedChain.symbol.lowercased(), default: 0] * (Double(amount) ?? 0))) ?? "$0.00"
      if info.isSwap {
        Text(String.localizedStringWithFormat(Strings.Wallet.transactionSwapTitle, amount, networkStore.selectedChain.symbol, fiat))
      } else {
        Text(String.localizedStringWithFormat(Strings.Wallet.transactionSendTitle, amount, networkStore.selectedChain.symbol, fiat))
      }
    case .erc20Transfer:
      if let token = visibleTokens.first(where: {
        $0.contractAddress.caseInsensitiveCompare(info.txData.baseData.to) == .orderedSame
      }) {
        let amount = formatter.decimalString(for: info.txData.baseData.value.removingHexPrefix, radix: .hex, decimals: Int(token.decimals)) ?? ""
        let fiat = numberFormatter.string(from: NSNumber(value: assetRatios[token.symbol.lowercased(), default: 0] * (Double(amount) ?? 0))) ?? "$0.00"
        Text(String.localizedStringWithFormat(Strings.Wallet.transactionSendTitle, amount, token.symbol, fiat))
      } else {
        Text(Strings.Wallet.send)
      }
    case .erc721TransferFrom, .erc721SafeTransferFrom:
      if let token = visibleTokens.first(where: {
        $0.contractAddress.caseInsensitiveCompare(info.txData.baseData.to) == .orderedSame
      }) {
        Text(String.localizedStringWithFormat(Strings.Wallet.transactionUnknownSendTitle, token.symbol))
      } else {
        Text(Strings.Wallet.send)
      }
    @unknown default:
      EmptyView()
    }
  }
  
  @ViewBuilder private var subtitle: some View {
    // For the time being, use the same subtitle label until we have the ability to parse
    // Swap from/to addresses
    Text("\(namedAddress(for: info.fromAddress)) \(Image(systemName: "arrow.right")) \(namedAddress(for: info.txData.baseData.to))")
  }
  
  private var metadata: Text {
    let date = Text(info.createdTime, formatter: timeFormatter)
    if displayAccountCreator {
      return date + Text(" · \(namedAddress(for: info.fromAddress))")
    }
    return date
  }
  
  var body: some View {
    HStack(spacing: 12) {
      BlockieGroup(
        fromAddress: info.fromAddress,
        toAddress: info.txData.baseData.to,
        alignVisuallyCentered: false
      )
      VStack(alignment: .leading, spacing: 4) {
        title
          .font(.footnote.weight(.semibold))
          .foregroundColor(Color(.bravePrimary))
        if let (fee, fiat) = gasFee {
          HStack(spacing: 4) {
            Image("brave.coins.4")
            Text(
              String.localizedStringWithFormat(
                Strings.Wallet.transactionSummaryFee,
                fee, networkStore.selectedChain.symbol, fiat)
            )
          }
          .foregroundColor(Color(.braveLabel))
          .font(.caption)
        }
        subtitle
          .foregroundColor(Color(.braveLabel))
        HStack {
          metadata
            .foregroundColor(Color(.secondaryBraveLabel))
          Spacer()
          HStack(spacing: 4) {
            Image(systemName: "circle.fill")
              .foregroundColor(info.txStatus.color)
              .imageScale(.small)
              .accessibilityHidden(true)
            Text(info.txStatus.localizedDescription)
              .foregroundColor(Color(.braveLabel))
              .multilineTextAlignment(.trailing)
          }
          .accessibilityElement(children: .combine)
        }
        .font(.caption)
      }
      .font(.footnote)
    }
    .frame(maxWidth: .infinity, alignment: .leading)
    .padding(.vertical, 6)
  }
}

extension BraveWallet.TransactionStatus {
  var localizedDescription: String {
    switch self {
    case .confirmed:
      return Strings.Wallet.transactionStatusConfirmed
    case .approved:
      return Strings.Wallet.transactionStatusApproved
    case .rejected:
      return Strings.Wallet.transactionStatusRejected
    case .unapproved:
      return Strings.Wallet.transactionStatusUnapproved
    case .submitted:
      return Strings.Wallet.transactionStatusSubmitted
    case .error:
      return Strings.Wallet.transactionStatusError
    @unknown default:
      return Strings.Wallet.transactionStatusUnknown
    }
  }
  var color: Color {
    switch self {
    case .confirmed, .approved:
      return Color(.braveSuccessLabel)
    case .rejected, .error:
      return Color(.braveErrorLabel)
    case .submitted:
      return Color(.braveWarningLabel)
    case .unapproved:
      return Color(.secondaryButtonTint)
    @unknown default:
      return Color.clear
    }
  }
}

#if DEBUG
struct Transaction_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      TransactionView(
        info: .previewConfirmedSend,
        keyringStore: .previewStoreWithWalletCreated,
        networkStore: .previewStore,
        visibleTokens: [.eth],
        displayAccountCreator: false,
        assetRatios: ["eth": 4576.36]
      )
      TransactionView(
        info: .previewConfirmedSwap,
        keyringStore: .previewStoreWithWalletCreated,
        networkStore: .previewStore,
        visibleTokens: [.eth],
        displayAccountCreator: true,
        assetRatios: ["eth": 4576.36]
      )
      TransactionView(
        info: .previewConfirmedERC20Approve,
        keyringStore: .previewStoreWithWalletCreated,
        networkStore: .previewStore,
        visibleTokens: [.eth],
        displayAccountCreator: false,
        assetRatios: ["eth": 4576.36]
      )
    }
    .padding(12)
    .previewLayout(.sizeThatFits)
  }
}
#endif
