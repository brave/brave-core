// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import Strings

struct TransactionSummary: Equatable, Identifiable {
  struct GasFee: Equatable {
    let fee: String
    let fiat: String
  }

  /// The transaction
  let txInfo: BraveWallet.TransactionInfo
  /// From address of the transaction
  var fromAddress: String { txInfo.fromAddress }
  /// Account name for the from address of the transaction
  let namedFromAddress: String
  /// To address of the transaction
  var toAddress: String { txInfo.ethTxToAddress }
  /// Account name for the to address of the transaction
  let namedToAddress: String
  /// The title for the transaction summary.
  /// Ex. "Sent 1.0000 ETH ($1.00"',  "Swapped 2.0000 ETH ($2.00)" / "Approved 1.0000 DAI" /  "Sent ETH"
  let title: String
  /// The gas fee and fiat for the transaction
  let gasFee: GasFee?
  /// The network symbol for the transaction
  let networkSymbol: String
  
  /// Transaction id
  var id: String { txInfo.id }
  /// The hash of the transaction
  var txHash: String { txInfo.txHash }
  /// Current status of the transaction
  var txStatus: BraveWallet.TransactionStatus { txInfo.txStatus }
  /// The time the transaction was created
  var createdTime: Date { txInfo.createdTime }
}

class TransactionParser {
  
  private init() {}
  
  static func transactionSummary(
    from transaction: BraveWallet.TransactionInfo,
    network: BraveWallet.NetworkInfo,
    accountInfos: [BraveWallet.AccountInfo],
    visibleTokens: [BraveWallet.BlockchainToken],
    allTokens: [BraveWallet.BlockchainToken],
    assetRatios: [String: Double],
    currencyFormatter: NumberFormatter
  ) -> TransactionSummary {
    TransactionSummary(
      txInfo: transaction,
      namedFromAddress: NamedAddresses.name(for: transaction.fromAddress, accounts: accountInfos),
      namedToAddress: NamedAddresses.name(for: transaction.ethTxToAddress, accounts: accountInfos),
      title: title(
        from: transaction,
        network: network,
        visibleTokens: visibleTokens,
        allTokens: allTokens,
        assetRatios: assetRatios,
        currencyFormatter: currencyFormatter
      ),
      gasFee: gasFee(
        from: transaction,
        network: network,
        assetRatios: assetRatios,
        currencyFormatter: currencyFormatter
      ),
      networkSymbol: network.symbol
    )
  }
  
  private static func title(
    from transaction: BraveWallet.TransactionInfo,
    network: BraveWallet.NetworkInfo,
    visibleTokens: [BraveWallet.BlockchainToken],
    allTokens: [BraveWallet.BlockchainToken],
    assetRatios: [String: Double],
    currencyFormatter: NumberFormatter
  ) -> String {
    let formatter = WeiFormatter(decimalFormatStyle: .balance)
    switch transaction.txType {
    case .erc20Approve:
      if let contractAddress = transaction.txDataUnion.ethTxData1559?.baseData.to,
          let value = transaction.txArgs[safe: 1],
          let token = token(for: contractAddress, network: network, visibleTokens: visibleTokens, allTokens: allTokens) {
        if value.caseInsensitiveCompare(WalletConstants.MAX_UINT256) == .orderedSame {
          return String.localizedStringWithFormat(Strings.Wallet.transactionApproveSymbolTitle, Strings.Wallet.editPermissionsApproveUnlimited, token.symbol)
        } else {
          return String.localizedStringWithFormat(Strings.Wallet.transactionApproveSymbolTitle, formatter.decimalString(for: value.removingHexPrefix, radix: .hex, decimals: Int(token.decimals)) ?? "", token.symbol)
        }
      } else {
        return Strings.Wallet.transactionUnknownApprovalTitle
      }
    case .ethSend, .ethSwap, .other:
      let amount = formatter.decimalString(for: transaction.ethTxValue.removingHexPrefix, radix: .hex, decimals: Int(network.decimals)) ?? ""
      let fiat = currencyFormatter.string(from: NSNumber(value: assetRatios[network.symbol.lowercased(), default: 0] * (Double(amount) ?? 0))) ?? "$0.00"
      if transaction.isSwap {
        return String.localizedStringWithFormat(Strings.Wallet.transactionSwapTitle, amount, network.symbol, fiat)
      } else {
        return String.localizedStringWithFormat(Strings.Wallet.transactionSendTitle, amount, network.symbol, fiat)
      }
    case .erc20Transfer:
      if let value = transaction.txArgs[safe: 1], let token = token(for: transaction.ethTxToAddress, network: network, visibleTokens: visibleTokens, allTokens: allTokens) {
        let amount = formatter.decimalString(for: value.removingHexPrefix, radix: .hex, decimals: Int(token.decimals)) ?? ""
        let fiat = currencyFormatter.string(from: NSNumber(value: assetRatios[token.symbol.lowercased(), default: 0] * (Double(amount) ?? 0))) ?? "$0.00"
        return String.localizedStringWithFormat(Strings.Wallet.transactionSendTitle, amount, token.symbol, fiat)
      } else {
        return Strings.Wallet.send
      }
    case .erc721TransferFrom, .erc721SafeTransferFrom:
      if let token = token(for: transaction.ethTxToAddress, network: network, visibleTokens: visibleTokens, allTokens: allTokens) {
        return String.localizedStringWithFormat(Strings.Wallet.transactionUnknownSendTitle, token.symbol)
      } else {
        return Strings.Wallet.send
      }
    case .solanaSystemTransfer,
        .solanaSplTokenTransfer,
        .solanaSplTokenTransferWithAssociatedTokenAccountCreation,
        .erc1155SafeTransferFrom:
      return ""
    @unknown default:
      return ""
    }
  }
  
  private static func gasFee(
    from transaction: BraveWallet.TransactionInfo,
    network: BraveWallet.NetworkInfo,
    assetRatios: [String: Double],
    currencyFormatter: NumberFormatter
  ) -> TransactionSummary.GasFee? {
    let isEIP1559Transaction = transaction.isEIP1559Transaction
    let limit = transaction.ethTxGasLimit
    let formatter = WeiFormatter(decimalFormatStyle: .gasFee(limit: limit.removingHexPrefix, radix: .hex))
    let hexFee = isEIP1559Transaction ? (transaction.txDataUnion.ethTxData1559?.maxFeePerGas ?? "") : transaction.ethTxGasPrice
    if let value = formatter.decimalString(for: hexFee.removingHexPrefix, radix: .hex, decimals: Int(network.decimals)) {
      if let doubleValue = Double(value), let assetRatio = assetRatios[network.symbol.lowercased()] {
        return .init(fee: value, fiat: currencyFormatter.string(from: NSNumber(value: doubleValue * assetRatio)) ?? "$0.00")
      } else {
        return .init(fee: value, fiat: "$0.00")
      }
    }
    return nil
  }
  
  private static func token(
    for contractAddress: String,
    network: BraveWallet.NetworkInfo,
    visibleTokens: [BraveWallet.BlockchainToken],
    allTokens: [BraveWallet.BlockchainToken]
  ) -> BraveWallet.BlockchainToken? {
    let findToken: (BraveWallet.BlockchainToken) -> Bool = {
      $0.contractAddress(in: network).caseInsensitiveCompare(contractAddress) == .orderedSame
    }
    return visibleTokens.first(where: findToken) ?? allTokens.first(where: findToken)
  }
}

extension BraveWallet.TransactionStatus: Equatable { }
