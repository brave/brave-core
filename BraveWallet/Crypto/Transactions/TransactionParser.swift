// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import Strings

enum TransactionParser {
  
  static func gasFee(
    from transaction: BraveWallet.TransactionInfo,
    network: BraveWallet.NetworkInfo,
    assetRatios: [String: Double],
    currencyFormatter: NumberFormatter
  ) -> GasFee? {
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
  
  static func token(
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
  
  static func parseTransaction(
    transaction: BraveWallet.TransactionInfo,
    network: BraveWallet.NetworkInfo,
    accountInfos: [BraveWallet.AccountInfo],
    visibleTokens: [BraveWallet.BlockchainToken],
    allTokens: [BraveWallet.BlockchainToken],
    assetRatios: [String: Double],
    currencyFormatter: NumberFormatter,
    decimalFormatStyle: WeiFormatter.DecimalFormatStyle? = nil
  ) -> ParsedTransaction? {
    let formatter = WeiFormatter(decimalFormatStyle: decimalFormatStyle ?? .decimals(precision: Int(network.decimals)))
    switch transaction.txType {
    case .ethSend, .other:
      let fromTokenSymbol = network.symbol
      let fromValue = transaction.ethTxValue
      let fromValueFormatted = formatter.decimalString(for: fromValue.removingHexPrefix, radix: .hex, decimals: Int(network.decimals))?.trimmingTrailingZeros ?? ""
      let fromFiat = currencyFormatter.string(from: NSNumber(value: assetRatios[network.symbol.lowercased(), default: 0] * (Double(fromValueFormatted) ?? 0))) ?? "$0.00"
      /* Example:
       Send 0.1234 ETH
       
       fromAddress="0x882F5a2c1C429e6592D801486566D0753BC1dD04"
       toAddress="0x4FC29eDF46859A67c5Bfa894C77a4E3C69353202"
       fromTokenSymbol="ETH"
       fromValue="0x1b667a56d488000"
       fromValueFormatted="0.1234"
       */
      return .init(
        transaction: transaction,
        namedFromAddress: NamedAddresses.name(for: transaction.fromAddress, accounts: accountInfos),
        fromAddress: transaction.fromAddress,
        namedToAddress: NamedAddresses.name(for: transaction.ethTxToAddress, accounts: accountInfos),
        toAddress: transaction.ethTxToAddress,
        networkSymbol: network.symbol,
        details: .ethSend(
          .init(
            fromTokenSymbol: fromTokenSymbol,
            fromValue: fromValue,
            fromAmount: fromValueFormatted,
            fromFiat: fromFiat,
            gasFee: gasFee(
              from: transaction,
              network: network,
              assetRatios: assetRatios,
              currencyFormatter: currencyFormatter
            )
          )
        )
      )
    case .erc20Transfer:
      guard let toAddress = transaction.txArgs[safe: 0],
            let fromValue = transaction.txArgs[safe: 1],
            let tokenContractAddress = transaction.txDataUnion.ethTxData1559?.baseData.to,
            let token = token(for: tokenContractAddress, network: network, visibleTokens: visibleTokens, allTokens: allTokens) else {
        return nil
      }
      let fromAmount = formatter.decimalString(for: fromValue.removingHexPrefix, radix: .hex, decimals: Int(token.decimals))?.trimmingTrailingZeros ?? ""
      let fromFiat = currencyFormatter.string(from: NSNumber(value: assetRatios[token.symbol.lowercased(), default: 0] * (Double(fromAmount) ?? 0))) ?? "$0.00"
      /*
       fromAddress="0x882F5a2c1C429e6592D801486566D0753BC1dD04"
       toAddress="0x7c24aed73d82c9d98a1b86bc2c8d2452c40419f8"
       tokenContractAddress="0xaD6D458402F60fD3Bd25163575031ACDce07538D"
       fromValue="0x5ff20a91f724000"
       fromAmount="0.4321"
       fromFiat="$0.43"
       token.symbol="DAI"
       */
      return .init(
        transaction: transaction,
        namedFromAddress: NamedAddresses.name(for: transaction.fromAddress, accounts: accountInfos),
        fromAddress: transaction.fromAddress,
        namedToAddress: NamedAddresses.name(for: toAddress, accounts: accountInfos),
        toAddress: toAddress,
        networkSymbol: network.symbol,
        details: .erc20Transfer(
          .init(
            fromTokenSymbol: token.symbol,
            fromValue: fromValue,
            fromAmount: fromAmount,
            fromFiat: fromFiat,
            gasFee: gasFee(
              from: transaction,
              network: network,
              assetRatios: assetRatios,
              currencyFormatter: currencyFormatter
            )
          )
        )
      )
    case .ethSwap:
      guard let fillPath = transaction.txArgs[safe: 0],
            let sellAmountValue = transaction.txArgs[safe: 1],
            let minBuyAmountValue = transaction.txArgs[safe: 2] else {
        return nil
      }
      
      let fillPathNoHexPrefix = fillPath.removingHexPrefix
      let fillPathLength = fillPathNoHexPrefix.count / 2
      let splitIndex = fillPathNoHexPrefix.index(fillPathNoHexPrefix.startIndex, offsetBy: fillPathLength)
      let fromTokenAddress = String(fillPathNoHexPrefix[..<splitIndex]).addingHexPrefix
      let toTokenAddress = String(fillPathNoHexPrefix[splitIndex...]).addingHexPrefix
      
      let fromToken = token(for: fromTokenAddress, network: network, visibleTokens: visibleTokens, allTokens: allTokens)
      let fromTokenDecimals = Int(fromToken?.decimals ?? network.decimals)
      let toToken = token(for: toTokenAddress, network: network, visibleTokens: visibleTokens, allTokens: allTokens)
      let toTokenDecimals = Int(toToken?.decimals ?? network.decimals)
      
      let formattedSellAmount = formatter.decimalString(for: sellAmountValue.removingHexPrefix, radix: .hex, decimals: fromTokenDecimals)?.trimmingTrailingZeros ?? ""
      let formattedMinBuyAmount = formatter.decimalString(for: minBuyAmountValue.removingHexPrefix, radix: .hex, decimals: toTokenDecimals)?.trimmingTrailingZeros ?? ""
      /* Example:
       USDC -> DAI
       Sell Amount: 1.5
      
       fillPath="0x07865c6e87b9f70255377e024ace6630c1eaa37fad6d458402f60fd3bd25163575031acdce07538d"
       fromTokenAddress = "0x07865c6e87b9f70255377e024ace6630c1eaa37f"
       fromToken.symbol = "USDC"
       sellAmountValue="0x16e360"
       formattedSellAmount="1.5"
       toTokenAddress = "0xad6d458402f60fd3bd25163575031acdce07538d"
       toToken.symbol = "DAI"
       minBuyAmountValue="0x1bd02ca9a7c244e"
       formattedMinBuyAmount="0.125259433834718286"
       */
      return .init(
        transaction: transaction,
        namedFromAddress: NamedAddresses.name(for: transaction.fromAddress, accounts: accountInfos),
        fromAddress: transaction.fromAddress,
        namedToAddress: NamedAddresses.name(for: transaction.ethTxToAddress, accounts: accountInfos),
        toAddress: transaction.ethTxToAddress,
        networkSymbol: network.symbol,
        details: .ethSwap(
          .init(
            fromToken: fromToken,
            fromValue: sellAmountValue,
            fromAmount: formattedSellAmount,
            toToken: toToken,
            minBuyValue: minBuyAmountValue,
            minBuyAmount: formattedMinBuyAmount,
            gasFee: gasFee(
              from: transaction,
              network: network,
              assetRatios: assetRatios,
              currencyFormatter: currencyFormatter
            )
          )
        )
      )
    case .erc20Approve:
      guard let contractAddress = transaction.txDataUnion.ethTxData1559?.baseData.to,
            let value = transaction.txArgs[safe: 1],
            let token = token(for: contractAddress, network: network, visibleTokens: visibleTokens, allTokens: allTokens) else {
        return nil
      }
      let isUnlimited = value.caseInsensitiveCompare(WalletConstants.MAX_UINT256) == .orderedSame
      let approvalAmount: String
      if isUnlimited {
        approvalAmount = Strings.Wallet.editPermissionsApproveUnlimited
      } else {
        approvalAmount = formatter.decimalString(for: value.removingHexPrefix, radix: .hex, decimals: Int(token.decimals))?.trimmingTrailingZeros ?? ""
      }
      /* Example:
       Approve DAI
       Proposed 0.1 DAI approval limit
       
       isUnlimited=false
       fromAddress="0x7c24aED73D82c9D98a1B86Bc2C8d2452c40419F8"
       token.symbol="DAI"
       tokenContractAddress="0xaD6D458402F60fD3Bd25163575031ACDce07538D"
       approvalValue="0x2386f26fc10000"
       approvalAmount="0.01"
       */
      return .init(
        transaction: transaction,
        namedFromAddress: NamedAddresses.name(for: transaction.fromAddress, accounts: accountInfos),
        fromAddress: transaction.fromAddress,
        namedToAddress: NamedAddresses.name(for: transaction.ethTxToAddress, accounts: accountInfos),
        toAddress: transaction.ethTxToAddress,
        networkSymbol: network.symbol,
        details: .ethErc20Approve(
          .init(
            token: token,
            approvalValue: value,
            approvalAmount: approvalAmount,
            isUnlimited: isUnlimited,
            gasFee: gasFee(
              from: transaction,
              network: network,
              assetRatios: assetRatios,
              currencyFormatter: currencyFormatter
            )
          )
        )
      )
    case .erc721TransferFrom, .erc721SafeTransferFrom:
      guard let owner = transaction.txArgs[safe: 0],
            let toAddress = transaction.txArgs[safe: 1],
            let tokenId = transaction.txArgs[safe: 2],
            let tokenContractAddress = transaction.txDataUnion.ethTxData1559?.baseData.to,
            let token = token(for: tokenContractAddress, network: network, visibleTokens: visibleTokens, allTokens: allTokens) else {
        return nil
      }
      return .init(
        transaction: transaction,
        namedFromAddress: NamedAddresses.name(for: transaction.fromAddress, accounts: accountInfos),
        fromAddress: transaction.fromAddress, // The caller, which may not be the owner
        namedToAddress: NamedAddresses.name(for: toAddress, accounts: accountInfos),
        toAddress: toAddress,
        networkSymbol: network.symbol,
        details: .erc721Transfer(
          .init(
            fromToken: token,
            fromValue: "1", // Can only send 1 erc721 at a time
            fromAmount: "1",
            owner: owner,
            tokenId: tokenId
          )
        )
      )
    case .solanaSystemTransfer:
      return nil
    case .solanaSplTokenTransfer:
      return nil
    case .solanaSplTokenTransferWithAssociatedTokenAccountCreation:
      return nil
    case .erc1155SafeTransferFrom:
      return nil
    case .solanaDappSignAndSendTransaction:
      return nil
    case .solanaDappSignTransaction:
      return nil
    @unknown default:
      return nil
    }
  }
}

extension BraveWallet.TransactionStatus: Equatable { }

struct GasFee: Equatable {
  let fee: String
  let fiat: String
}

struct ParsedTransaction: Equatable {
  enum Details: Equatable {
    case ethSend(EthSendDetails)
    case erc20Transfer(EthSendDetails)
    case ethSwap(EthSwapDetails)
    case ethErc20Approve(EthErc20ApproveDetails)
    case erc721Transfer(Eth721TransferDetails)
  }
  
  /// The transaction
  let transaction: BraveWallet.TransactionInfo
  
  /// Account name for the from address of the transaction
  let namedFromAddress: String
  /// Address sending from
  let fromAddress: String
  
  /// Account name for the to address of the transaction
  let namedToAddress: String
  /// Address sending to
  let toAddress: String
  
  /// Network symbol of the transaction
  let networkSymbol: String
  
  /// Details of the transaction
  let details: Details
}

struct EthErc20ApproveDetails: Equatable {
  /// Token being approved
  let token: BraveWallet.BlockchainToken
  /// Value being approved prior to formatting
  let approvalValue: String
  /// Value being approved formatted
  let approvalAmount: String
  /// If the value being approved is unlimited
  let isUnlimited: Bool
  /// Gas fee for the transaction
  let gasFee: GasFee?
}

struct EthSendDetails: Equatable {
  /// From token symbol
  let fromTokenSymbol: String
  /// From value prior to formatting
  let fromValue: String
  /// From amount formatted
  let fromAmount: String
  /// The amount formatted as currency
  let fromFiat: String?
  
  /// Gas fee for the transaction
  let gasFee: GasFee?
}

struct EthSwapDetails: Equatable {
  /// Token being swapped from
  let fromToken: BraveWallet.BlockchainToken?
  /// From value prior to formatting
  let fromValue: String
  /// From amount formatted
  let fromAmount: String
  
  /// Token being swapped to
  let toToken: BraveWallet.BlockchainToken?
  /// Min. buy value prior to formatting
  let minBuyValue: String
  /// Min. buy amount formatted
  let minBuyAmount: String
  
  /// Gas fee for the transaction
  let gasFee: GasFee?
}

struct Eth721TransferDetails: Equatable {
  /// Token being swapped from
  let fromToken: BraveWallet.BlockchainToken?
  /// From value prior to formatting
  let fromValue: String
  /// From amount formatted
  let fromAmount: String
  
  /// Owner (must not be confused with the caller (fromAddress)
  let owner: String
  /// The token id
  let tokenId: String
}

extension BraveWallet.TransactionInfo {
  /// Use `TransactionParser` to build a `ParsedTransaction` model for this transaction.
  func parsedTransaction(
    network: BraveWallet.NetworkInfo,
    accountInfos: [BraveWallet.AccountInfo],
    visibleTokens: [BraveWallet.BlockchainToken],
    allTokens: [BraveWallet.BlockchainToken],
    assetRatios: [String: Double],
    currencyFormatter: NumberFormatter,
    decimalFormatStyle: WeiFormatter.DecimalFormatStyle? = nil
  ) -> ParsedTransaction? {
    TransactionParser.parseTransaction(
      transaction: self,
      network: network,
      accountInfos: accountInfos,
      visibleTokens: visibleTokens,
      allTokens: allTokens,
      assetRatios: assetRatios,
      currencyFormatter: currencyFormatter,
      decimalFormatStyle: decimalFormatStyle
    )
  }
}
