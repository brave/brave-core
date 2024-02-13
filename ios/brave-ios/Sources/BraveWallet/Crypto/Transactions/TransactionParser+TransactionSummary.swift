// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import Strings

struct TransactionSummary: Equatable, Identifiable {
  /// The transaction
  let txInfo: BraveWallet.TransactionInfo
  /// From address of the transaction
  var fromAddress: String { txInfo.fromAccountId.address }
  /// Account name for the from address of the transaction
  let namedFromAddress: String
  /// To address of the transaction
  var toAddress: String
  /// Account name for the to address of the transaction
  let namedToAddress: String
  /// The title for the transaction summary.
  /// Ex. "Sent 1.0000 ETH ($1.00)"',  "Swapped 2.0000 ETH ($2.00)" / "Approved 1.0000 DAI" /  "Sent ETH" / "Sent 1 INVSBLE"
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

extension TransactionParser {
  
  static func transactionSummary(
    from transaction: BraveWallet.TransactionInfo,
    network: BraveWallet.NetworkInfo,
    accountInfos: [BraveWallet.AccountInfo],
    userAssets: [BraveWallet.BlockchainToken],
    allTokens: [BraveWallet.BlockchainToken],
    assetRatios: [String: Double],
    nftMetadata: [String: NFTMetadata],
    solEstimatedTxFee: UInt64?,
    currencyFormatter: NumberFormatter
  ) -> TransactionSummary {
    guard let parsedTransaction = parseTransaction(
      transaction: transaction,
      network: network,
      accountInfos: accountInfos,
      userAssets: userAssets,
      allTokens: allTokens,
      assetRatios: assetRatios,
      nftMetadata: nftMetadata,
      solEstimatedTxFee: solEstimatedTxFee,
      currencyFormatter: currencyFormatter,
      decimalFormatStyle: .balance // use 4 digit precision for summary
    ) else {
      let toAddress: String
      switch transaction.coin {
      case .eth:
        toAddress = transaction.ethTxToAddress
      case .sol:
        toAddress = transaction.txDataUnion.solanaTxData?.toWalletAddress ?? ""
      default:
        toAddress = ""
      }
      return .init(
        txInfo: transaction,
        namedFromAddress: NamedAddresses.name(for: transaction.fromAccountId.address, accounts: accountInfos),
        toAddress: toAddress,
        namedToAddress: NamedAddresses.name(for: transaction.ethTxToAddress, accounts: accountInfos),
        title: "",
        gasFee: gasFee(
          from: transaction,
          network: network,
          assetRatios: assetRatios,
          currencyFormatter: currencyFormatter
        ),
        networkSymbol: network.symbol
      )
    }
    switch parsedTransaction.details {
    case let .ethSend(details):
      let title = String.localizedStringWithFormat(Strings.Wallet.transactionSendTitle, details.fromAmount, details.fromToken?.symbol ?? "", details.fromFiat ?? "")
      return .init(
        txInfo: transaction,
        namedFromAddress: parsedTransaction.namedFromAddress,
        toAddress: parsedTransaction.toAddress,
        namedToAddress: parsedTransaction.namedToAddress,
        title: title,
        gasFee: details.gasFee,
        networkSymbol: parsedTransaction.networkSymbol
      )
    case let .erc20Transfer(details):
      let title = String.localizedStringWithFormat(Strings.Wallet.transactionSendTitle, details.fromAmount, details.fromToken?.symbol ?? "", details.fromFiat ?? "")
      return .init(
        txInfo: transaction,
        namedFromAddress: parsedTransaction.namedFromAddress,
        toAddress: parsedTransaction.toAddress,
        namedToAddress: parsedTransaction.namedToAddress,
        title: title,
        gasFee: details.gasFee,
        networkSymbol: parsedTransaction.networkSymbol
      )
    case let .ethSwap(details):
      let fromAmount = details.fromAmount
      let fromTokenSymbol = details.fromToken?.symbol ?? ""
      let toAmount = details.minBuyAmount
      let toTokenSymbol = details.toToken?.symbol ?? ""
      let title = String.localizedStringWithFormat(Strings.Wallet.transactionSwappedTitle, fromAmount, fromTokenSymbol, toAmount, toTokenSymbol)
      return .init(
        txInfo: transaction,
        namedFromAddress: parsedTransaction.namedFromAddress,
        toAddress: parsedTransaction.toAddress,
        namedToAddress: parsedTransaction.namedToAddress,
        title: title,
        gasFee: details.gasFee,
        networkSymbol: parsedTransaction.networkSymbol
      )
    case let .ethErc20Approve(details):
      let title: String
      if details.isUnlimited {
        title = String.localizedStringWithFormat(Strings.Wallet.transactionApproveSymbolTitle, Strings.Wallet.editPermissionsApproveUnlimited, details.token?.symbol ?? "")
      } else {
        title = String.localizedStringWithFormat(Strings.Wallet.transactionApproveSymbolTitle, details.approvalAmount, details.token?.symbol ?? "")
      }
      return .init(
        txInfo: transaction,
        namedFromAddress: parsedTransaction.namedFromAddress,
        toAddress: parsedTransaction.toAddress,
        namedToAddress: parsedTransaction.namedToAddress,
        title: title,
        gasFee: details.gasFee,
        networkSymbol: parsedTransaction.networkSymbol
      )
    case let .erc721Transfer(details):
      let title: String
      if let token = details.fromToken {
        title = "\(Strings.Wallet.send) 1 \(token.symbol)"
      } else {
        title = Strings.Wallet.send
      }
      return .init(
        txInfo: transaction,
        namedFromAddress: parsedTransaction.namedFromAddress,
        toAddress: parsedTransaction.toAddress,
        namedToAddress: parsedTransaction.namedToAddress,
        title: title,
        gasFee: nil,
        networkSymbol: parsedTransaction.networkSymbol
      )
    case let .solSystemTransfer(details), let .solSplTokenTransfer(details):
      let title = String.localizedStringWithFormat(Strings.Wallet.transactionSendTitle, details.fromAmount, details.fromToken?.symbol ?? "", details.fromFiat ?? "").replacingOccurrences(of: "()", with: "") // if fiat is empty string, remove `()`
      return .init(
        txInfo: transaction,
        namedFromAddress: parsedTransaction.namedFromAddress,
        toAddress: parsedTransaction.toAddress,
        namedToAddress: parsedTransaction.namedToAddress,
        title: title,
        gasFee: details.gasFee,
        networkSymbol: parsedTransaction.networkSymbol
      )
    case .solDappTransaction, .solSwapTransaction:
      let title: String
      if case .solDappTransaction = parsedTransaction.details {
        title = Strings.Wallet.solanaDappTransactionTitle
      } else {
        title = Strings.Wallet.solanaSwapTransactionTitle
      }
      return .init(
        txInfo: transaction,
        namedFromAddress: parsedTransaction.namedFromAddress,
        toAddress: parsedTransaction.toAddress,
        namedToAddress: parsedTransaction.namedToAddress,
        title: title,
        gasFee: parsedTransaction.gasFee,
        networkSymbol: parsedTransaction.networkSymbol
      )
    case let .filSend(details):
      let title = String.localizedStringWithFormat(Strings.Wallet.transactionSendTitle, details.sendAmount, details.sendToken?.symbol ?? "", details.sendFiat ?? "")
      return .init(
        txInfo: transaction,
        namedFromAddress: parsedTransaction.namedFromAddress,
        toAddress: parsedTransaction.toAddress,
        namedToAddress: parsedTransaction.namedToAddress,
        title: title,
        gasFee: details.gasFee,
        networkSymbol: parsedTransaction.networkSymbol
      )
    case .other:
      return .init(txInfo: .init(), namedFromAddress: "", toAddress: "", namedToAddress: "", title: "", gasFee: nil, networkSymbol: "")
    }
  }
}
