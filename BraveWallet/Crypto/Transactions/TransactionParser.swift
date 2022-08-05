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
    solEstimatedTxFee: UInt64? = nil,
    currencyFormatter: NumberFormatter
  ) -> GasFee? {
    var gasFee: GasFee?
    let existingMinimumFractionDigits = currencyFormatter.minimumFractionDigits
    let existingMaximumFractionDigits = currencyFormatter.maximumFractionDigits
    // Show additional decimal places for gas fee calculations (Solana has low tx fees).
    currencyFormatter.minimumFractionDigits = 2
    currencyFormatter.maximumFractionDigits = 10
    switch network.coin {
    case .eth:
      let isEIP1559Transaction = transaction.isEIP1559Transaction
      let limit = transaction.ethTxGasLimit
      let formatter = WeiFormatter(decimalFormatStyle: .gasFee(limit: limit.removingHexPrefix, radix: .hex))
      let hexFee = isEIP1559Transaction ? (transaction.txDataUnion.ethTxData1559?.maxFeePerGas ?? "") : transaction.ethTxGasPrice
      if let value = formatter.decimalString(for: hexFee.removingHexPrefix, radix: .hex, decimals: Int(network.decimals))?.trimmingTrailingZeros {
        if let doubleValue = Double(value),
            let assetRatio = assetRatios[network.symbol.lowercased()],
            let fiat = currencyFormatter.string(from: NSNumber(value: doubleValue * assetRatio)) {
          gasFee = .init(fee: value, fiat: fiat)
        } else {
          gasFee = .init(fee: value, fiat: "$0.00")
        }
      }
    case .sol:
      guard let solEstimatedTxFee = solEstimatedTxFee else { return nil }
      let estimatedTxFee = "\(solEstimatedTxFee)"
      let formatter = WeiFormatter(decimalFormatStyle: .decimals(precision: Int(network.decimals)))
      if let value = formatter.decimalString(for: estimatedTxFee, radix: .decimal, decimals: Int(network.decimals))?.trimmingTrailingZeros {
        if let doubleValue = Double(value),
            let assetRatio = assetRatios[network.symbol.lowercased()],
            let fiat = currencyFormatter.string(from: NSNumber(value: doubleValue * assetRatio)) {
          gasFee = .init(fee: value, fiat: fiat)
        } else {
          gasFee = .init(fee: value, fiat: "$0.00")
        }
      }
    case .fil:
      break
    @unknown default:
      break
    }
    // Restore previous fraction digits
    currencyFormatter.minimumFractionDigits = existingMinimumFractionDigits
    currencyFormatter.maximumFractionDigits = existingMaximumFractionDigits
    return gasFee
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
    solEstimatedTxFee: UInt64?,
    currencyFormatter: NumberFormatter,
    decimalFormatStyle: WeiFormatter.DecimalFormatStyle? = nil
  ) -> ParsedTransaction? {
    let formatter = WeiFormatter(decimalFormatStyle: decimalFormatStyle ?? .decimals(precision: Int(network.decimals)))
    switch transaction.txType {
    case .ethSend, .other:
      let fromValue = transaction.ethTxValue
      let fromValueFormatted = formatter.decimalString(for: fromValue.removingHexPrefix, radix: .hex, decimals: Int(network.decimals))?.trimmingTrailingZeros ?? ""
      let fromFiat = currencyFormatter.string(from: NSNumber(value: assetRatios[network.nativeToken.assetRatioId.lowercased(), default: 0] * (Double(fromValueFormatted) ?? 0))) ?? "$0.00"
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
            fromToken: network.nativeToken,
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
            let fromToken = token(for: tokenContractAddress, network: network, visibleTokens: visibleTokens, allTokens: allTokens) else {
        return nil
      }
      let fromAmount = formatter.decimalString(for: fromValue.removingHexPrefix, radix: .hex, decimals: Int(fromToken.decimals))?.trimmingTrailingZeros ?? ""
      let fromFiat = currencyFormatter.string(from: NSNumber(value: assetRatios[fromToken.assetRatioId.lowercased(), default: 0] * (Double(fromAmount) ?? 0))) ?? "$0.00"
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
            fromToken: fromToken,
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
      
      let fromFiat = currencyFormatter.string(from: NSNumber(value: assetRatios[fromToken?.assetRatioId.lowercased() ?? "", default: 0] * (Double(formattedSellAmount) ?? 0))) ?? "$0.00"
      let minBuyAmountFiat = currencyFormatter.string(from: NSNumber(value: assetRatios[toToken?.assetRatioId.lowercased() ?? "", default: 0] * (Double(formattedMinBuyAmount) ?? 0))) ?? "$0.00"
      /* Example:
       USDC -> DAI
       Sell Amount: 1.5
      
       fillPath = "0x07865c6e87b9f70255377e024ace6630c1eaa37fad6d458402f60fd3bd25163575031acdce07538d"
       fromTokenAddress = "0x07865c6e87b9f70255377e024ace6630c1eaa37f"
       fromToken.symbol = "USDC"
       sellAmountValue = "0x16e360"
       formattedSellAmount = "1.5"
       fromFiat = "$187.37"
       toTokenAddress = "0xad6d458402f60fd3bd25163575031acdce07538d"
       toToken.symbol = "DAI"
       minBuyAmountValue = "0x1bd02ca9a7c244e"
       formattedMinBuyAmount = "0.125259433834718286"
       minBuyAmountFiat = "$6.67"
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
            fromFiat: fromFiat,
            toToken: toToken,
            minBuyValue: minBuyAmountValue,
            minBuyAmount: formattedMinBuyAmount,
            minBuyAmountFiat: minBuyAmountFiat,
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
            let spenderAddress = transaction.txArgs[safe: 0],
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
            spenderAddress: spenderAddress,
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
      guard let lamports = transaction.txDataUnion.solanaTxData?.lamports,
            let toAddress = transaction.txDataUnion.solanaTxData?.toWalletAddress else {
        return nil
      }
      let fromValue = "\(lamports)"
      let fromValueFormatted = formatter.decimalString(for: fromValue, radix: .decimal, decimals: Int(network.decimals))?.trimmingTrailingZeros ?? ""
      let fromFiat = currencyFormatter.string(from: NSNumber(value: assetRatios[network.nativeToken.assetRatioId.lowercased(), default: 0] * (Double(fromValueFormatted) ?? 0))) ?? "$0.00"
      /* Example:
       Send 0.1234 SOL
       
       fromAddress="0x882F5a2c1C429e6592D801486566D0753BC1dD04"
       toAddress="0x4FC29eDF46859A67c5Bfa894C77a4E3C69353202"
       fromTokenSymbol="SOL"
       fromValue="0x1b667a56d488000"
       fromValueFormatted="0.1234"
       */
      return .init(
        transaction: transaction,
        namedFromAddress: NamedAddresses.name(for: transaction.fromAddress, accounts: accountInfos),
        fromAddress: transaction.fromAddress,
        namedToAddress: NamedAddresses.name(for: toAddress, accounts: accountInfos),
        toAddress: toAddress,
        networkSymbol: network.symbol,
        details: .solSystemTransfer(
          .init(
            fromToken: network.nativeToken,
            fromValue: fromValue,
            fromAmount: fromValueFormatted,
            fromFiat: fromFiat,
            gasFee: gasFee(
              from: transaction,
              network: network,
              assetRatios: assetRatios,
              solEstimatedTxFee: solEstimatedTxFee,
              currencyFormatter: currencyFormatter
            )
          )
        )
      )
    case .solanaSplTokenTransfer,
        .solanaSplTokenTransferWithAssociatedTokenAccountCreation:
      guard let amount = transaction.txDataUnion.solanaTxData?.amount,
            let toAddress = transaction.txDataUnion.solanaTxData?.toWalletAddress,
            let splTokenMintAddress = transaction.txDataUnion.solanaTxData?.splTokenMintAddress,
            let fromToken = token(for: splTokenMintAddress, network: network, visibleTokens: visibleTokens, allTokens: allTokens) else {
        return nil
      }
      let fromValue = "\(amount)"
      let fromValueFormatted = formatter.decimalString(for: fromValue, radix: .decimal, decimals: Int(fromToken.decimals))?.trimmingTrailingZeros ?? ""
      let fromFiat = currencyFormatter.string(from: NSNumber(value: assetRatios[fromToken.assetRatioId.lowercased(), default: 0] * (Double(fromValueFormatted) ?? 0))) ?? "$0.00"
      /* Example:
       Send 0.1234 SMB
       
       fromAddress="0x882F5a2c1C429e6592D801486566D0753BC1dD04"
       toAddress="0x4FC29eDF46859A67c5Bfa894C77a4E3C69353202"
       fromTokenSymbol="SMB"
       fromValue="0x1b667a56d488000"
       fromValueFormatted="0.1234"
       */
      return .init(
        transaction: transaction,
        namedFromAddress: NamedAddresses.name(for: transaction.fromAddress, accounts: accountInfos),
        fromAddress: transaction.fromAddress,
        namedToAddress: NamedAddresses.name(for: toAddress, accounts: accountInfos),
        toAddress: toAddress,
        networkSymbol: network.symbol,
        details: .solSplTokenTransfer(
          .init(
            fromToken: fromToken,
            fromValue: fromValue,
            fromAmount: fromValueFormatted,
            fromFiat: fromFiat,
            gasFee: gasFee(
              from: transaction,
              network: network,
              assetRatios: assetRatios,
              solEstimatedTxFee: solEstimatedTxFee,
              currencyFormatter: currencyFormatter
            )
          )
        )
      )
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
    case ethSend(SendDetails)
    case erc20Transfer(SendDetails)
    case ethSwap(EthSwapDetails)
    case ethErc20Approve(EthErc20ApproveDetails)
    case erc721Transfer(Eth721TransferDetails)
    case solSystemTransfer(SendDetails)
    case solSplTokenTransfer(SendDetails)
    case other
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
  
  /// Gas fee for the transaction if available
  var gasFee: GasFee? {
    switch details {
    case let .ethSend(details),
      let .erc20Transfer(details),
      let .solSystemTransfer(details),
      let .solSplTokenTransfer(details):
      return details.gasFee
    case let .ethSwap(details):
      return details.gasFee
    case let .ethErc20Approve(details):
      return details.gasFee
    case .erc721Transfer, .other:
      return nil
    }
  }
  
  init() {
    self.transaction = .init()
    self.namedFromAddress = ""
    self.fromAddress = ""
    self.namedToAddress = ""
    self.toAddress = ""
    self.networkSymbol = ""
    self.details = .other
  }
  
  init(
    transaction: BraveWallet.TransactionInfo,
    namedFromAddress: String,
    fromAddress: String,
    namedToAddress: String,
    toAddress: String,
    networkSymbol: String,
    details: Details
  ) {
    self.transaction = transaction
    self.namedFromAddress = namedFromAddress
    self.fromAddress = fromAddress
    self.namedToAddress = namedToAddress
    self.toAddress = toAddress
    self.networkSymbol = networkSymbol
    self.details = details
  }
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
  /// The spender address to get the current allowance
  let spenderAddress: String
  /// Gas fee for the transaction
  let gasFee: GasFee?
}

struct SendDetails: Equatable {
  /// Token being swapped from
  let fromToken: BraveWallet.BlockchainToken
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
  /// The amount formatted as currency
  let fromFiat: String?
  
  /// Token being swapped to
  let toToken: BraveWallet.BlockchainToken?
  /// Min. buy value prior to formatting
  let minBuyValue: String
  /// Min. buy amount formatted
  let minBuyAmount: String
  /// The amount formatted as currency
  let minBuyAmountFiat: String?
  
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
    solEstimatedTxFee: UInt64? = nil,
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
      solEstimatedTxFee: solEstimatedTxFee,
      currencyFormatter: currencyFormatter,
      decimalFormatStyle: decimalFormatStyle
    )
  }
}

extension ParsedTransaction {
  var coin: BraveWallet.CoinType {
    transaction.coin
  }
}
