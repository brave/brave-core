/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveCore
import Swift
import struct Shared.Strings

/// Displays a summary of a given transaction
struct TransactionView: View {
  var info: BraveWallet.TransactionInfo
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  var visibleTokens: [BraveWallet.BlockchainToken]
  var allTokens: [BraveWallet.BlockchainToken]
  var displayAccountCreator: Bool
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

  private func token(for contractAddress: String) -> BraveWallet.BlockchainToken? {
    let findToken: (BraveWallet.BlockchainToken) -> Bool = {
      $0.contractAddress(in: networkStore.selectedChain).caseInsensitiveCompare(contractAddress) == .orderedSame
    }
    return visibleTokens.first(where: findToken) ?? allTokens.first(where: findToken)
  }

  private var gasFee: (String, fiat: String)? {
    let isEIP1559Transaction = info.isEIP1559Transaction
    let limit = info.ethTxGasLimit
    let formatter = WeiFormatter(decimalFormatStyle: .gasFee(limit: limit.removingHexPrefix, radix: .hex))
    let hexFee = isEIP1559Transaction ? (info.txDataUnion.ethTxData1559?.maxFeePerGas ?? "") : info.ethTxGasPrice
    if let value = formatter.decimalString(for: hexFee.removingHexPrefix, radix: .hex, decimals: Int(networkStore.selectedChain.decimals)) {
      return (
        value,
        {
          guard let doubleValue = Double(value), let assetRatio = assetRatios[networkStore.selectedChain.symbol.lowercased()] else {
            return "$0.00"
          }
          return numberFormatter.string(from: NSNumber(value: doubleValue * assetRatio)) ?? "$0.00"
        }()
      )
    }
    return nil
  }

  @ViewBuilder private var title: some View {
    let formatter = WeiFormatter(decimalFormatStyle: .balance)
    switch info.txType {
    case .erc20Approve:
      if let contractAddress = info.txDataUnion.ethTxData1559?.baseData.to,
          let value = info.txArgs[safe: 1],
          let token = token(for: contractAddress) {
        if value.caseInsensitiveCompare(WalletConstants.MAX_UINT256) == .orderedSame {
          Text(String.localizedStringWithFormat(Strings.Wallet.transactionApproveSymbolTitle, Strings.Wallet.editPermissionsApproveUnlimited, token.symbol))
        } else {
          Text(String.localizedStringWithFormat(Strings.Wallet.transactionApproveSymbolTitle, formatter.decimalString(for: value.removingHexPrefix, radix: .hex, decimals: Int(token.decimals)) ?? "", token.symbol))
        }
      } else {
        Text(Strings.Wallet.transactionUnknownApprovalTitle)
      }
    case .ethSend, .other:
      let amount = formatter.decimalString(for: info.ethTxValue.removingHexPrefix, radix: .hex, decimals: Int(networkStore.selectedChain.decimals)) ?? ""
      let fiat = numberFormatter.string(from: NSNumber(value: assetRatios[networkStore.selectedChain.symbol.lowercased(), default: 0] * (Double(amount) ?? 0))) ?? "$0.00"
      if info.isSwap {
        Text(String.localizedStringWithFormat(Strings.Wallet.transactionSwapTitle, amount, networkStore.selectedChain.symbol, fiat))
      } else {
        Text(String.localizedStringWithFormat(Strings.Wallet.transactionSendTitle, amount, networkStore.selectedChain.symbol, fiat))
      }
    case .erc20Transfer:
      if let value = info.txArgs[safe: 1], let token = token(for: info.ethTxToAddress) {
        let amount = formatter.decimalString(for: value.removingHexPrefix, radix: .hex, decimals: Int(token.decimals)) ?? ""
        let fiat = numberFormatter.string(from: NSNumber(value: assetRatios[token.symbol.lowercased(), default: 0] * (Double(amount) ?? 0))) ?? "$0.00"
        Text(String.localizedStringWithFormat(Strings.Wallet.transactionSendTitle, amount, token.symbol, fiat))
      } else {
        Text(Strings.Wallet.send)
      }
    case .erc721TransferFrom, .erc721SafeTransferFrom:
      if let token = token(for: info.ethTxToAddress) {
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
    let from = namedAddress(for: info.fromAddress)
    let to = namedAddress(for: info.ethTxToAddress)
    Text("\(from) \(Image(systemName: "arrow.right")) \(to)")
      .accessibilityLabel(
        String.localizedStringWithFormat(
          Strings.Wallet.transactionFromToAccessibilityLabel, from, to
        )
      )
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
        toAddress: info.ethTxToAddress,
        alignVisuallyCentered: false
      )
      .accessibilityHidden(true)
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
          .accessibilityElement(children: .combine)
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
        visibleTokens: [.previewToken],
        allTokens: [],
        displayAccountCreator: false,
        assetRatios: ["eth": 4576.36]
      )
      TransactionView(
        info: .previewConfirmedSwap,
        keyringStore: .previewStoreWithWalletCreated,
        networkStore: .previewStore,
        visibleTokens: [.previewToken],
        allTokens: [],
        displayAccountCreator: true,
        assetRatios: ["eth": 4576.36]
      )
      TransactionView(
        info: .previewConfirmedERC20Approve,
        keyringStore: .previewStoreWithWalletCreated,
        networkStore: .previewStore,
        visibleTokens: [.previewToken],
        allTokens: [],
        displayAccountCreator: false,
        assetRatios: ["eth": 4576.36]
      )
    }
    .padding(12)
    .previewLayout(.sizeThatFits)
  }
}
#endif
