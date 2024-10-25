// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BigNumber
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
    switch network.coin {
    case .eth:
      let isEIP1559Transaction = transaction.isEIP1559Transaction
      let limit = transaction.ethTxGasLimit
      let formatter = WalletAmountFormatter(
        decimalFormatStyle: .gasFee(limit: limit.removingHexPrefix, radix: .hex)
      )
      let hexFee =
        isEIP1559Transaction
        ? (transaction.txDataUnion.ethTxData1559?.maxFeePerGas ?? "") : transaction.ethTxGasPrice
      if let value = formatter.decimalString(
        for: hexFee.removingHexPrefix,
        radix: .hex,
        decimals: Int(network.decimals)
      )?.trimmingTrailingZeros {
        if let doubleValue = Double(value),
          let assetRatio = assetRatios[network.symbol.lowercased()],
          let fiat = currencyFormatter.formatAsFiat(doubleValue * assetRatio)
        {
          gasFee = .init(fee: value, fiat: fiat)
        } else {
          gasFee = .init(fee: value, fiat: currencyFormatter.formatAsFiat(0) ?? "$0.00")
        }
      }
    case .sol:
      guard let solEstimatedTxFee = solEstimatedTxFee else { return nil }
      let estimatedTxFee = "\(solEstimatedTxFee)"
      let formatter = WalletAmountFormatter(
        decimalFormatStyle: .decimals(precision: Int(network.decimals))
      )
      if let value = formatter.decimalString(
        for: estimatedTxFee,
        radix: .decimal,
        decimals: Int(network.decimals)
      )?.trimmingTrailingZeros {
        if let doubleValue = Double(value),
          let assetRatio = assetRatios[network.symbol.lowercased()],
          let fiat = currencyFormatter.formatAsFiat(doubleValue * assetRatio)
        {
          gasFee = .init(fee: value, fiat: fiat)
        } else {
          gasFee = .init(fee: value, fiat: currencyFormatter.formatAsFiat(0) ?? "$0.00")
        }
      }
    case .fil:
      guard let filTxData = transaction.txDataUnion.filTxData else { return nil }
      if let gasLimit = BDouble(filTxData.gasLimit),
        let gasFeeCapValue = BDouble(filTxData.gasFeeCap),
        let gasPremiumValue = BDouble(filTxData.gasPremium)
      {
        let decimals = Int(network.decimals)
        // baseFee = gasFeeCap - gasPremium
        let baseFeeValue = gasFeeCapValue - gasPremiumValue
        let gasFeeValue =
          (transaction.isEIP1559Transaction ? gasFeeCapValue : baseFeeValue) * gasLimit
        let gasFeeValueDecimals = gasFeeValue / (BDouble(10) ** decimals)

        let gasFeeString = gasFeeValueDecimals.decimalExpansion(
          precisionAfterDecimalPoint: decimals,
          rounded: true
        ).trimmingTrailingZeros
        if let doubleValue = Double(gasFeeString),
          let assetRatio = assetRatios[network.symbol.lowercased()],
          let fiat = currencyFormatter.formatAsFiat(doubleValue * assetRatio)
        {
          gasFee = .init(fee: gasFeeString, fiat: fiat)
        } else {
          gasFee = .init(fee: gasFeeString, fiat: currencyFormatter.formatAsFiat(0) ?? "$0.00")
        }
      }
    case .btc:
      guard let btcTxData = transaction.txDataUnion.btcTxData,
        case let formatter = WalletAmountFormatter(decimalFormatStyle: .decimals(precision: 8)),
        let gasFeeString = formatter.decimalString(
          for: "\(btcTxData.fee)",
          radix: .decimal,
          decimals: 8
        )
      else { return nil }
      if let doubleValue = Double(gasFeeString),
        let assetRatio = assetRatios[network.symbol.lowercased()],
        let fiat = currencyFormatter.string(from: NSNumber(value: doubleValue * assetRatio))
      {
        gasFee = .init(fee: gasFeeString, fiat: fiat)
      } else {
        gasFee = .init(fee: gasFeeString, fiat: currencyFormatter.formatAsFiat(0) ?? "$0.00")
      }
    case .zec:
      break
    @unknown default:
      break
    }
    return gasFee
  }

  static func token(
    for contractAddress: String,
    network: BraveWallet.NetworkInfo,
    userAssets: [BraveWallet.BlockchainToken],
    allTokens: [BraveWallet.BlockchainToken]
  ) -> BraveWallet.BlockchainToken? {
    let findToken: (BraveWallet.BlockchainToken) -> Bool = {
      $0.contractAddress(in: network).caseInsensitiveCompare(contractAddress) == .orderedSame
    }
    return userAssets.first(where: findToken) ?? allTokens.first(where: findToken)
  }

  static func parseTransaction(
    transaction: BraveWallet.TransactionInfo,
    allNetworks: [BraveWallet.NetworkInfo],
    accountInfos: [BraveWallet.AccountInfo],
    userAssets: [BraveWallet.BlockchainToken],
    allTokens: [BraveWallet.BlockchainToken],
    assetRatios: [String: Double],
    nftMetadata: [String: BraveWallet.NftMetadata],
    solEstimatedTxFee: UInt64?,
    currencyFormatter: NumberFormatter,
    decimalFormatStyle: WalletAmountFormatter.DecimalFormatStyle? = nil
  ) -> ParsedTransaction? {
    guard
      let txNetwork = allNetworks.first(where: {
        $0.chainId.caseInsensitiveCompare(transaction.chainId) == .orderedSame
          && $0.coin == transaction.coin
      })
    else {
      return nil
    }
    let formatter = WalletAmountFormatter(
      decimalFormatStyle: decimalFormatStyle ?? .decimals(precision: Int(txNetwork.decimals))
    )
    let fromAccountInfo =
      accountInfos.first(where: { $0.accountId == transaction.fromAccountId }) ?? .init()
    switch transaction.txType {
    case .ethSend, .other:
      if let filTxData = transaction.txDataUnion.filTxData {  // FIL send tx
        let sendValue = filTxData.value
        let sendValueFormatted =
          formatter.decimalString(
            for: sendValue,
            radix: .decimal,
            decimals: Int(txNetwork.nativeToken.decimals)
          )?.trimmingTrailingZeros ?? ""
        let sendFiat =
          currencyFormatter.formatAsFiat(
            assetRatios[txNetwork.nativeToken.assetRatioId.lowercased(), default: 0]
              * (Double(sendValueFormatted) ?? 0)
          ) ?? "$0.00"
        let gasLimitValueFormatted =
          formatter.decimalString(
            for: filTxData.gasLimit,
            radix: .decimal,
            decimals: Int(txNetwork.nativeToken.decimals)
          )?.trimmingTrailingZeros ?? ""
        let gasPremiumValueFormatted =
          formatter.decimalString(
            for: filTxData.gasPremium,
            radix: .decimal,
            decimals: Int(txNetwork.nativeToken.decimals)
          )?.trimmingTrailingZeros ?? ""
        let gasFeeCapValueFormatted =
          formatter.decimalString(
            for: filTxData.gasFeeCap,
            radix: .decimal,
            decimals: Int(txNetwork.nativeToken.decimals)
          )?.trimmingTrailingZeros ?? ""
        // Example
        // Send 1 FIL
        //
        // fromAddress="t1xqhfiydm2yq6augugonr4zpdllh77iw53aesdes"
        // toAddress="t895quq7gkjh6ebshr7qi2ud7vycel4m7x6dvsekf"
        // sendTokenSymbol="FL"
        // sendValue="1000000000000000000"
        // sendValueFormatted="1"
        // gasPremiumValue="100911"
        // gasLimitValue="1527953"
        // gasFeeCapValue="101965"
        // gasPremiumValueFormatted="0.000000000000100911"
        // gasLimitValueFormatted="0.000000000001527953"
        // gasFeeCapValueFormatted="0.000000000000101965"
        return .init(
          transaction: transaction,
          namedFromAddress: NamedAddresses.name(
            for: fromAccountInfo.address,
            accounts: accountInfos
          ),
          fromAccountInfo: fromAccountInfo,
          namedToAddress: NamedAddresses.name(for: filTxData.to, accounts: accountInfos),
          toAddress: filTxData.to,
          network: txNetwork,
          details: .filSend(
            .init(
              sendToken: txNetwork.nativeToken,
              sendValue: filTxData.value,
              sendAmount: sendValueFormatted,
              sendFiat: sendFiat,
              gasPremium: gasPremiumValueFormatted,
              gasLimit: gasLimitValueFormatted,
              gasFeeCap: gasFeeCapValueFormatted,
              gasFee: gasFee(
                from: transaction,
                network: txNetwork,
                assetRatios: assetRatios,
                currencyFormatter: currencyFormatter
              )
            )
          )
        )
      } else if let btcTxData = transaction.txDataUnion.btcTxData {  // BTC send tx
        // Require 8 decimals precision for BTC parsing
        let namedFromAccount = fromAccountInfo.name
        let fromValue = "\(btcTxData.amount)"
        let fromValueFormatted =
          formatter.decimalString(
            for: fromValue,
            radix: .decimal,
            decimals: Int(txNetwork.decimals)
          )?.trimmingTrailingZeros ?? ""
        let fromFiat =
          currencyFormatter.string(
            from: NSNumber(
              value: assetRatios[txNetwork.nativeToken.assetRatioId.lowercased(), default: 0]
                * (Double(fromValueFormatted) ?? 0)
            )
          ) ?? "$0.00"
        let fromToken = (allTokens + userAssets).first(where: {
          $0.coin == transaction.coin && $0.chainId == transaction.chainId
        })
        // Example:
        // Send 0.00001 BTC
        //
        // fromValue="1000"
        // fromValueFormatted="0.00001"
        return .init(
          transaction: transaction,
          namedFromAddress: namedFromAccount,
          fromAccountInfo: fromAccountInfo,
          namedToAddress: "",
          toAddress: btcTxData.to,
          network: txNetwork,
          details: .btcSend(
            .init(
              fromToken: fromToken,
              fromValue: fromValue,
              fromAmount: fromValueFormatted,
              fromFiat: fromFiat,
              fromTokenMetadata: nil,
              gasFee: gasFee(
                from: transaction,
                network: txNetwork,
                assetRatios: assetRatios,
                currencyFormatter: currencyFormatter
              )
            )
          )
        )
      } else {
        let fromValue = transaction.ethTxValue
        let fromValueFormatted =
          formatter.decimalString(
            for: fromValue.removingHexPrefix,
            radix: .hex,
            decimals: Int(txNetwork.decimals)
          )?.trimmingTrailingZeros ?? ""
        let fromFiat =
          currencyFormatter.formatAsFiat(
            assetRatios[txNetwork.nativeToken.assetRatioId.lowercased(), default: 0]
              * (Double(fromValueFormatted) ?? 0)
          ) ?? "$0.00"
        // Example:
        // Send 0.1234 ETH
        //
        // fromAddress="0x882F5a2c1C429e6592D801486566D0753BC1dD04"
        // toAddress="0x4FC29eDF46859A67c5Bfa894C77a4E3C69353202"
        // fromTokenSymbol="ETH"
        // fromValue="0x1b667a56d488000"
        // fromValueFormatted="0.1234"
        return .init(
          transaction: transaction,
          namedFromAddress: NamedAddresses.name(
            for: fromAccountInfo.address,
            accounts: accountInfos
          ),
          fromAccountInfo: fromAccountInfo,
          namedToAddress: NamedAddresses.name(
            for: transaction.ethTxToAddress,
            accounts: accountInfos
          ),
          toAddress: transaction.ethTxToAddress,
          network: txNetwork,
          details: .ethSend(
            .init(
              fromToken: txNetwork.nativeToken,
              fromValue: fromValue,
              fromAmount: fromValueFormatted,
              fromFiat: fromFiat,
              fromTokenMetadata: nil,
              gasFee: gasFee(
                from: transaction,
                network: txNetwork,
                assetRatios: assetRatios,
                currencyFormatter: currencyFormatter
              )
            )
          )
        )
      }
    case .erc20Transfer:
      guard let toAddress = transaction.txArgs[safe: 0],
        let fromValue = transaction.txArgs[safe: 1],
        let tokenContractAddress = transaction.erc20TransferTokenContractAddress
      else {
        return nil
      }
      let fromToken = token(
        for: tokenContractAddress,
        network: txNetwork,
        userAssets: userAssets,
        allTokens: allTokens
      )
      var fromAmount = ""
      var fromFiat = currencyFormatter.formatAsFiat(0) ?? "$0.00"
      if let token = fromToken {
        fromAmount =
          formatter.decimalString(
            for: fromValue.removingHexPrefix,
            radix: .hex,
            decimals: Int(token.decimals)
          )?.trimmingTrailingZeros ?? ""
        fromFiat =
          currencyFormatter.formatAsFiat(
            assetRatios[token.assetRatioId.lowercased(), default: 0]
              * (Double(fromAmount) ?? 0)
          ) ?? "$0.00"
      }
      // fromAddress="0x882F5a2c1C429e6592D801486566D0753BC1dD04"
      // toAddress="0x7c24aed73d82c9d98a1b86bc2c8d2452c40419f8"
      // tokenContractAddress="0xaD6D458402F60fD3Bd25163575031ACDce07538D"
      // fromValue="0x5ff20a91f724000"
      // fromAmount="0.4321"
      // fromFiat="$0.43"
      // token.symbol="DAI"
      return .init(
        transaction: transaction,
        namedFromAddress: NamedAddresses.name(
          for: fromAccountInfo.address,
          accounts: accountInfos
        ),
        fromAccountInfo: fromAccountInfo,
        namedToAddress: NamedAddresses.name(for: toAddress, accounts: accountInfos),
        toAddress: toAddress,
        network: txNetwork,
        details: .erc20Transfer(
          .init(
            fromToken: fromToken,
            fromValue: fromValue,
            fromAmount: fromAmount,
            fromFiat: fromFiat,
            fromTokenMetadata: nil,
            gasFee: gasFee(
              from: transaction,
              network: txNetwork,
              assetRatios: assetRatios,
              currencyFormatter: currencyFormatter
            )
          )
        )
      )
    case .ethSwap:
      guard let swapInfo = transaction.swapInfo
      else {
        return nil
      }
      let sellAmountValue = swapInfo.fromAmount
      let minBuyAmountValue = swapInfo.toAmount

      let (fromTokenAddress, toTokenAddress) = transaction.ethSwapTokenContractAddresses ?? ("", "")

      let fromNetwork =
        allNetworks.first {
          $0.coin == swapInfo.fromCoin && $0.chainId == swapInfo.fromChainId
        } ?? txNetwork

      let fromToken = token(
        for: fromTokenAddress,
        network: fromNetwork,
        userAssets: userAssets,
        allTokens: allTokens
      )

      let fromTokenDecimals = Int(fromToken?.decimals ?? fromNetwork.decimals)

      let toNetwork =
        allNetworks.first {
          $0.chainId.caseInsensitiveCompare(swapInfo.toChainId) == .orderedSame
            && $0.coin == swapInfo.toCoin
        } ?? txNetwork

      let toToken = token(
        for: toTokenAddress,
        network: toNetwork,
        userAssets: userAssets,
        allTokens: allTokens
      )

      let toTokenDecimals = Int(toToken?.decimals ?? toNetwork.decimals)

      let formattedSellAmount =
        formatter.decimalString(
          for: sellAmountValue.removingHexPrefix,
          radix: .hex,
          decimals: fromTokenDecimals
        )?.trimmingTrailingZeros ?? ""
      let formattedMinBuyAmount =
        formatter.decimalString(
          for: minBuyAmountValue.removingHexPrefix,
          radix: .hex,
          decimals: toTokenDecimals
        )?.trimmingTrailingZeros ?? ""

      let fromFiat =
        currencyFormatter.formatAsFiat(
          assetRatios[fromToken?.assetRatioId.lowercased() ?? "", default: 0]
            * (Double(formattedSellAmount) ?? 0)
        ) ?? "$0.00"
      let minBuyAmountFiat =
        currencyFormatter.formatAsFiat(
          assetRatios[toToken?.assetRatioId.lowercased() ?? "", default: 0]
            * (Double(formattedMinBuyAmount) ?? 0)
        ) ?? "$0.00"
      // Example:
      // USDC -> DAI
      // Sell Amount: 1.5
      //
      // fromTokenAddress = "0x07865c6e87b9f70255377e024ace6630c1eaa37f"
      // fromToken.symbol = "USDC"
      // sellAmountValue = "0x16e360"
      // formattedSellAmount = "1.5"
      // fromFiat = "$187.37"
      // toTokenAddress = "0xad6d458402f60fd3bd25163575031acdce07538d"
      // toToken.symbol = "DAI"
      // minBuyAmountValue = "0x1bd02ca9a7c244e"
      // formattedMinBuyAmount = "0.125259433834718286"
      // minBuyAmountFiat = "$6.67"
      return .init(
        transaction: transaction,
        namedFromAddress: NamedAddresses.name(
          for: fromAccountInfo.address,
          accounts: accountInfos
        ),
        fromAccountInfo: fromAccountInfo,
        namedToAddress: NamedAddresses.name(
          for: transaction.ethTxToAddress,
          accounts: accountInfos
        ),
        toAddress: transaction.ethTxToAddress,
        network: txNetwork,
        details: .ethSwap(
          .init(
            fromToken: fromToken,
            fromNetwork: fromNetwork,
            fromValue: sellAmountValue,
            fromAmount: formattedSellAmount,
            fromFiat: fromFiat,
            toToken: toToken,
            toNetwork: toNetwork,
            minBuyValue: minBuyAmountValue,
            minBuyAmount: formattedMinBuyAmount,
            minBuyAmountFiat: minBuyAmountFiat,
            gasFee: gasFee(
              from: transaction,
              network: txNetwork,
              assetRatios: assetRatios,
              currencyFormatter: currencyFormatter
            )
          )
        )
      )
    case .erc20Approve:
      guard let contractAddress = transaction.erc20ApproveTokenContractAddress,
        let spenderAddress = transaction.txArgs[safe: 0],
        let value = transaction.txArgs[safe: 1]
      else {
        return nil
      }
      let token = token(
        for: contractAddress,
        network: txNetwork,
        userAssets: userAssets,
        allTokens: allTokens
      )
      let isUnlimited = value.caseInsensitiveCompare(WalletConstants.maxUInt256) == .orderedSame
      let approvalAmount: String
      let approvalFiat: String
      if isUnlimited {
        approvalAmount = Strings.Wallet.editPermissionsApproveUnlimited
        approvalFiat = Strings.Wallet.editPermissionsApproveUnlimited
      } else {
        approvalAmount =
          formatter.decimalString(
            for: value.removingHexPrefix,
            radix: .hex,
            decimals: Int(token?.decimals ?? txNetwork.decimals)
          )?.trimmingTrailingZeros ?? ""
        approvalFiat =
          currencyFormatter.formatAsFiat(
            assetRatios[token?.assetRatioId.lowercased() ?? "", default: 0]
              * (Double(approvalAmount) ?? 0)
          ) ?? "$0.00"
      }
      // Example:
      // Approve DAI
      // Proposed 0.1 DAI approval limit
      //
      // isUnlimited=false
      // fromAddress="0x7c24aED73D82c9D98a1B86Bc2C8d2452c40419F8"
      // token.symbol="DAI"
      // tokenContractAddress="0xaD6D458402F60fD3Bd25163575031ACDce07538D"
      // approvalValue="0x2386f26fc10000"
      // approvalAmount="0.01"
      return .init(
        transaction: transaction,
        namedFromAddress: NamedAddresses.name(
          for: fromAccountInfo.address,
          accounts: accountInfos
        ),
        fromAccountInfo: fromAccountInfo,
        namedToAddress: NamedAddresses.name(
          for: transaction.ethTxToAddress,
          accounts: accountInfos
        ),
        toAddress: transaction.ethTxToAddress,
        network: txNetwork,
        details: .ethErc20Approve(
          .init(
            token: token,
            tokenContractAddress: contractAddress,
            approvalValue: value,
            approvalAmount: approvalAmount,
            approvalFiat: approvalFiat,
            isUnlimited: isUnlimited,
            spenderAddress: spenderAddress,
            gasFee: gasFee(
              from: transaction,
              network: txNetwork,
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
        let tokenContractAddress = transaction.erc721ContractAddress
      else {
        return nil
      }
      let token = token(
        for: tokenContractAddress,
        network: txNetwork,
        userAssets: userAssets,
        allTokens: allTokens
      )
      let tokenNFTMetadata: BraveWallet.NftMetadata?
      if let token {
        tokenNFTMetadata = nftMetadata[token.id]
      } else {
        tokenNFTMetadata = nil
      }

      return .init(
        transaction: transaction,
        namedFromAddress: NamedAddresses.name(
          for: fromAccountInfo.address,
          accounts: accountInfos
        ),
        fromAccountInfo: fromAccountInfo,  // The caller, which may not be the owner
        namedToAddress: NamedAddresses.name(for: toAddress, accounts: accountInfos),
        toAddress: toAddress,
        network: txNetwork,
        details: .erc721Transfer(
          .init(
            fromToken: token,
            fromValue: "1",  // Can only send 1 erc721 at a time
            fromAmount: "1",
            nftMetadata: tokenNFTMetadata,
            owner: owner,
            tokenId: tokenId,
            gasFee: gasFee(
              from: transaction,
              network: txNetwork,
              assetRatios: assetRatios,
              currencyFormatter: currencyFormatter
            )
          )
        )
      )
    case .solanaSystemTransfer:
      guard let lamports = transaction.txDataUnion.solanaTxData?.lamports,
        let toAddress = transaction.txDataUnion.solanaTxData?.toWalletAddress
      else {
        return nil
      }
      let fromValue = "\(lamports)"
      let fromValueFormatted =
        formatter.decimalString(
          for: fromValue,
          radix: .decimal,
          decimals: Int(txNetwork.decimals)
        )?.trimmingTrailingZeros ?? ""
      let fromFiat =
        currencyFormatter.formatAsFiat(
          assetRatios[txNetwork.nativeToken.assetRatioId.lowercased(), default: 0]
            * (Double(fromValueFormatted) ?? 0)
        ) ?? "$0.00"
      // Example:
      // Send 0.1234 SOL
      //
      // fromAddress="0x882F5a2c1C429e6592D801486566D0753BC1dD04"
      // toAddress="0x4FC29eDF46859A67c5Bfa894C77a4E3C69353202"
      // fromTokenSymbol="SOL"
      // fromValue="0x1b667a56d488000"
      // fromValueFormatted="0.1234"
      return .init(
        transaction: transaction,
        namedFromAddress: NamedAddresses.name(
          for: fromAccountInfo.address,
          accounts: accountInfos
        ),
        fromAccountInfo: fromAccountInfo,
        namedToAddress: NamedAddresses.name(for: toAddress, accounts: accountInfos),
        toAddress: toAddress,
        network: txNetwork,
        details: .solSystemTransfer(
          .init(
            fromToken: txNetwork.nativeToken,
            fromValue: fromValue,
            fromAmount: fromValueFormatted,
            fromFiat: fromFiat,
            fromTokenMetadata: nil,
            gasFee: gasFee(
              from: transaction,
              network: txNetwork,
              assetRatios: assetRatios,
              solEstimatedTxFee: transaction.txStatus == .unapproved
                ? solEstimatedTxFee : transaction.solEstimatedTxFeeFromTxData,
              currencyFormatter: currencyFormatter
            )
          )
        )
      )
    case .solanaSplTokenTransfer,
      .solanaSplTokenTransferWithAssociatedTokenAccountCreation,
      .solanaCompressedNftTransfer:
      guard let amount = transaction.txDataUnion.solanaTxData?.amount,
        let toAddress = transaction.txDataUnion.solanaTxData?.toWalletAddress,
        let splTokenMintAddress = transaction.txDataUnion.solanaTxData?.tokenAddress
      else {
        return nil
      }
      let fromToken: BraveWallet.BlockchainToken? = token(
        for: splTokenMintAddress,
        network: txNetwork,
        userAssets: userAssets,
        allTokens: allTokens
      )
      let tokenNFTMetadata: BraveWallet.NftMetadata?
      if let fromToken {
        tokenNFTMetadata = nftMetadata[fromToken.id]
      } else {
        tokenNFTMetadata = nil
      }
      let fromValue = "\(amount)"
      var fromValueFormatted = ""
      var fromFiat = "$0.00"
      if let token = fromToken {
        fromValueFormatted =
          formatter.decimalString(for: fromValue, radix: .decimal, decimals: Int(token.decimals))?
          .trimmingTrailingZeros ?? ""
        if token.isNft {
          fromFiat = ""  // don't show fiat for NFTs
        } else {
          fromFiat =
            currencyFormatter.formatAsFiat(
              assetRatios[token.assetRatioId.lowercased(), default: 0]
                * (Double(fromValueFormatted) ?? 0)
            ) ?? "$0.00"
        }
      } else {
        fromValueFormatted = "0"
        fromFiat = ""
      }
      // Example:
      // Send 0.1234 SMB
      //
      // fromAddress="0x882F5a2c1C429e6592D801486566D0753BC1dD04"
      // toAddress="0x4FC29eDF46859A67c5Bfa894C77a4E3C69353202"
      // fromTokenSymbol="SMB"
      // fromValue="0x1b667a56d488000"
      // fromValueFormatted="0.1234"
      return .init(
        transaction: transaction,
        namedFromAddress: NamedAddresses.name(
          for: fromAccountInfo.address,
          accounts: accountInfos
        ),
        fromAccountInfo: fromAccountInfo,
        namedToAddress: NamedAddresses.name(for: toAddress, accounts: accountInfos),
        toAddress: toAddress,
        network: txNetwork,
        details: .solSplTokenTransfer(
          .init(
            fromToken: fromToken,
            fromValue: fromValue,
            fromAmount: fromValueFormatted,
            fromFiat: fromFiat,
            fromTokenMetadata: tokenNFTMetadata,
            gasFee: gasFee(
              from: transaction,
              network: txNetwork,
              assetRatios: assetRatios,
              solEstimatedTxFee: transaction.txStatus == .unapproved
                ? solEstimatedTxFee : transaction.solEstimatedTxFeeFromTxData,
              currencyFormatter: currencyFormatter
            )
          )
        )
      )
    case .solanaDappSignAndSendTransaction, .solanaDappSignTransaction, .solanaSwap:
      let transactionLamports = transaction.txDataUnion.solanaTxData?.lamports
      let fromAddress = transaction.fromAddress
      var toAddress = transaction.txDataUnion.solanaTxData?.toWalletAddress
      var fromValue = ""
      var fromAmount = ""
      var symbol: String?
      let instructions = transaction.txDataUnion.solanaTxData?.instructions ?? []
      let parsedInstructions = instructions.map {
        TransactionParser.parseSolanaInstruction($0, decimalFormatStyle: decimalFormatStyle)
      }

      // Calculate lamports from the transaction instructions
      var valueFromInstructions = BDouble(0)
      instructions.forEach { instruction in
        guard let instructionTypeValue = instruction.decodedData?.instructionType else {
          return
        }
        if instruction.isSystemProgram,
          let instructionType = BraveWallet.SolanaSystemInstruction(
            rawValue: Int(instructionTypeValue)
          )
        {
          symbol = "SOL"
          switch instructionType {
          case .transfer, .transferWithSeed:
            if let instructionLamports = instruction.decodedData?.paramFor(BraveWallet.Lamports)?
              .value,
              let instructionLamportsValue = BDouble(instructionLamports),
              let fromPubkey = instruction.accountMetas[safe: 0]?.pubkey,
              let toPubkey = instruction.accountMetas[safe: 1]?.pubkey,
              fromPubkey != toPubkey
            {  // only show lamports as transfered if the amount is going to a different pubKey
              valueFromInstructions += instructionLamportsValue
            }
          case .withdrawNonceAccount:
            if let instructionLamports = instruction.decodedData?.paramFor(BraveWallet.Lamports)?
              .value,
              let instructionLamportsValue = BDouble(instructionLamports)
            {
              if let nonceAccount = instruction.accountMetas[safe: 0]?.pubkey,
                nonceAccount == fromAddress
              {
                valueFromInstructions += instructionLamportsValue
              } else if let toPubkey = instruction.accountMetas[safe: 1]?.pubkey,
                toPubkey == fromAddress
              {
                valueFromInstructions -= instructionLamportsValue
              }
            }
          case .createAccount, .createAccountWithSeed:
            if let instructionLamports = instruction.decodedData?.paramFor(BraveWallet.Lamports)?
              .value,
              let instructionLamportsValue = BDouble(instructionLamports)
            {
              if let fromPubkey = instruction.accountMetas[safe: 0]?.pubkey,
                fromPubkey == fromAddress
              {
                valueFromInstructions += instructionLamportsValue
              }
            }
          default:
            if let instructionLamports = instruction.decodedData?.paramFor(BraveWallet.Lamports)?
              .value,
              let instructionLamportsValue = BDouble(instructionLamports)
            {
              valueFromInstructions += instructionLamportsValue
            }
          }
          if let transactionLamports = transactionLamports,
            let transactionLamportsValue = BDouble(exactly: transactionLamports)
          {
            let transactionTotal = transactionLamportsValue + valueFromInstructions
            fromValue =
              transactionTotal
              .decimalExpansion(
                precisionAfterDecimalPoint:
                  Int(txNetwork.decimals)
              ).trimmingTrailingZeros
            if let transactionTotalFormatted = formatter.decimalString(
              for: "\(transactionTotal)",
              decimals: 9
            )?.trimmingTrailingZeros {
              fromAmount = transactionTotalFormatted
            }
          }
        }
        if toAddress == nil || toAddress?.isEmpty == true,
          let toPubkey = instruction.toPubkey
        {
          toAddress = toPubkey
        }
      }

      let solanaTxDetails: SolanaTxDetails = .init(
        fromValue: fromValue,
        fromAmount: fromAmount,
        symbol: symbol,
        gasFee: gasFee(
          from: transaction,
          network: txNetwork,
          assetRatios: assetRatios,
          solEstimatedTxFee: transaction.txStatus == .unapproved
            ? solEstimatedTxFee : transaction.solEstimatedTxFeeFromTxData,
          currencyFormatter: currencyFormatter
        ),
        instructions: parsedInstructions
      )
      let details: ParsedTransaction.Details
      if transaction.txType == .solanaSwap {
        details = .solSwapTransaction(solanaTxDetails)
      } else {  // .solanaDappSignTransaction, .solanaDappSignAndSendTransaction
        details = .solDappTransaction(solanaTxDetails)
      }
      return .init(
        transaction: transaction,
        namedFromAddress: NamedAddresses.name(
          for: fromAccountInfo.address,
          accounts: accountInfos
        ),
        fromAccountInfo: fromAccountInfo,
        namedToAddress: NamedAddresses.name(for: toAddress ?? "", accounts: accountInfos),
        toAddress: toAddress ?? "",
        network: txNetwork,
        details: details
      )
    case .erc1155SafeTransferFrom:
      return nil
    case .ethFilForwarderTransfer:
      return nil
    @unknown default:
      return nil
    }
  }

  static func parseSolanaInstruction(
    _ instruction: BraveWallet.SolanaInstruction,
    decimalFormatStyle: WalletAmountFormatter.DecimalFormatStyle? = nil
  ) -> SolanaTxDetails.ParsedSolanaInstruction {
    guard let decodedData = instruction.decodedData else {
      let title = Strings.Wallet.solanaUnknownInstructionName
      let programId = SolanaTxDetails.ParsedSolanaInstruction.KeyValue(
        key: Strings.Wallet.solanaInstructionProgramId,
        value: instruction.programId
      )
      let accounts = instruction.accountKeyValues
      let data = SolanaTxDetails.ParsedSolanaInstruction.KeyValue(
        key: Strings.Wallet.solanaInstructionData,
        value: "\(instruction.data)"
      )
      return .init(name: title, details: [programId] + accounts + [data], instruction: instruction)
    }
    let formatter = WalletAmountFormatter(
      decimalFormatStyle: decimalFormatStyle ?? .decimals(precision: 9)
    )
    var details: [SolanaTxDetails.ParsedSolanaInstruction.KeyValue] = []
    if instruction.isSystemProgram {
      let accounts = instruction.accountKeyValues
      details.append(contentsOf: accounts)

      if let lamportsParam = decodedData.paramFor(BraveWallet.Lamports),
        let lamportsValue = formatter.decimalString(
          for: lamportsParam.value,
          radix: .decimal,
          decimals: 9
        )?.trimmingTrailingZeros
      {
        details.append(.init(key: Strings.Wallet.solanaAmount, value: "\(lamportsValue) SOL"))
      }

      let params = decodedData.params
      if !params.isEmpty {
        let params = params.map { param in
          SolanaTxDetails.ParsedSolanaInstruction.KeyValue(
            key: param.localizedName,
            value: param.value
          )
        }
        details.append(contentsOf: params)
      }

    } else if instruction.isTokenProgram {
      let accounts = instruction.accountKeyValues
      details.append(contentsOf: accounts)

      if let amountParam = decodedData.paramFor(BraveWallet.Amount),
        let decimalsParam = decodedData.paramFor(BraveWallet.Decimals),
        let decimals = Int(decimalsParam.value),
        let amountValue = formatter.decimalString(
          for: amountParam.value,
          radix: .decimal,
          decimals: decimals
        )?.trimmingTrailingZeros
      {
        details.append(.init(key: Strings.Wallet.solanaAmount, value: amountValue))
      }

      let params = decodedData.params
      if !params.isEmpty {
        let params = params.map { param in
          SolanaTxDetails.ParsedSolanaInstruction.KeyValue(
            key: param.localizedName,
            value: param.value
          )
        }
        details.append(contentsOf: params)
      }
    }
    return .init(
      name: instruction.instructionName,
      details: details,
      instruction: instruction
    )
  }
}

extension BraveWallet.TransactionStatus: Equatable {}

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
    case solDappTransaction(SolanaTxDetails)
    case solSwapTransaction(SolanaTxDetails)
    case filSend(FilSendDetails)
    case btcSend(SendDetails)
    case other
  }

  /// The transaction
  let transaction: BraveWallet.TransactionInfo

  /// Account name for the from address of the transaction
  let namedFromAddress: String
  /// `AccountInfo` sending from
  let fromAccountInfo: BraveWallet.AccountInfo

  /// Account name for the to address of the transaction
  let namedToAddress: String
  /// Address sending to
  let toAddress: String

  /// Network of the transaction
  let network: BraveWallet.NetworkInfo
  /// Network symbol of the transaction
  var networkSymbol: String { network.symbol }

  /// Details of the transaction
  let details: Details

  /// Gas fee for the transaction if available
  var gasFee: GasFee? {
    switch details {
    case .ethSend(let details),
      .erc20Transfer(let details),
      .solSystemTransfer(let details),
      .solSplTokenTransfer(let details):
      return details.gasFee
    case .ethSwap(let details):
      return details.gasFee
    case .ethErc20Approve(let details):
      return details.gasFee
    case .solDappTransaction(let details),
      .solSwapTransaction(let details):
      return details.gasFee
    case .erc721Transfer(let details):
      return details.gasFee
    case .filSend(let details):
      return details.gasFee
    case .btcSend(let details):
      return details.gasFee
    case .other:
      return nil
    }
  }

  var hasSystemProgramAssignInstruction: Bool {
    if case .solDappTransaction(let details) = details {
      for parsedInstruction in details.instructions {
        let isAssign =
          parsedInstruction.instruction.instructionTypeName
          == WalletConstants.solanaTxInstructionTypeNameAssign
        let isAssignWithSeed =
          parsedInstruction.instruction.instructionTypeName
          == WalletConstants.solanaTxInstructionTypeNameAssignWithSeed
        if isAssign || isAssignWithSeed {
          return true
        }
      }
    }
    return false
  }

  init() {
    self.transaction = .init()
    self.namedFromAddress = ""
    self.fromAccountInfo = .init()
    self.namedToAddress = ""
    self.toAddress = ""
    self.network = .init()
    self.details = .other
  }

  init(
    transaction: BraveWallet.TransactionInfo,
    namedFromAddress: String,
    fromAccountInfo: BraveWallet.AccountInfo,
    namedToAddress: String,
    toAddress: String,
    network: BraveWallet.NetworkInfo,
    details: Details
  ) {
    self.transaction = transaction
    self.namedFromAddress = namedFromAddress
    self.fromAccountInfo = fromAccountInfo
    self.namedToAddress = namedToAddress
    self.toAddress = toAddress
    self.network = network
    self.details = details
  }

  /// Determines if the given query matches the `ParsedTransaction`.
  func matches(_ query: String) -> Bool {
    if namedFromAddress.localizedCaseInsensitiveContains(query)
      || namedToAddress.localizedCaseInsensitiveContains(query)
      || toAddress.localizedCaseInsensitiveContains(query)
      || transaction.txHash.localizedCaseInsensitiveContains(query)
      || (!fromAccountInfo.address.isEmpty
        && fromAccountInfo.address.localizedCaseInsensitiveContains(query))
    {
      return true
    }
    switch details {
    case .ethSend(let sendDetails),
      .erc20Transfer(let sendDetails),
      .solSystemTransfer(let sendDetails),
      .solSplTokenTransfer(let sendDetails),
      .btcSend(let sendDetails):
      return sendDetails.fromToken?.matches(query) == true
    case .ethSwap(let ethSwapDetails):
      return ethSwapDetails.fromToken?.matches(query) == true
        || ethSwapDetails.toToken?.matches(query) == true
    case .ethErc20Approve(let ethErc20ApproveDetails):
      return ethErc20ApproveDetails.token?.matches(query) == true
    case .erc721Transfer(let eth721TransferDetails):
      return eth721TransferDetails.fromToken?.matches(query) == true
    case .solDappTransaction(let solanaTxDetails),
      .solSwapTransaction(let solanaTxDetails):
      return solanaTxDetails.symbol?.localizedCaseInsensitiveContains(query) == true
    case .filSend(let filSendDetails):
      return filSendDetails.sendToken?.matches(query) == true
    case .other:
      return false
    }
  }
}

extension BraveWallet.BlockchainToken {
  /// Determines if the given query matches the `BlockchainToken`.
  fileprivate func matches(_ query: String) -> Bool {
    name.localizedCaseInsensitiveContains(query) == true
      || symbol.localizedCaseInsensitiveContains(query) == true
      || contractAddress.localizedCaseInsensitiveContains(query) == true
  }
}

struct EthErc20ApproveDetails: Equatable {
  /// Token being approved
  let token: BraveWallet.BlockchainToken?
  /// The contract address of the token being approved
  let tokenContractAddress: String
  /// Value being approved prior to formatting
  let approvalValue: String
  /// Value being approved formatted
  let approvalAmount: String
  /// The amount approved formatted as currency
  let approvalFiat: String
  /// If the value being approved is unlimited
  let isUnlimited: Bool
  /// The spender address to get the current allowance
  let spenderAddress: String
  /// Gas fee for the transaction
  let gasFee: GasFee?
}

struct SendDetails: Equatable {
  /// Token being swapped from
  let fromToken: BraveWallet.BlockchainToken?
  /// From value prior to formatting
  let fromValue: String
  /// From amount formatted
  let fromAmount: String
  /// The amount formatted as currency
  let fromFiat: String?
  /// Metadata if `fromToken` is an NFT
  let fromTokenMetadata: BraveWallet.NftMetadata?

  /// Gas fee for the transaction
  let gasFee: GasFee?
}

struct EthSwapDetails: Equatable {
  /// Token being swapped from
  let fromToken: BraveWallet.BlockchainToken?
  /// From token network
  let fromNetwork: BraveWallet.NetworkInfo
  /// From value prior to formatting
  let fromValue: String
  /// From amount formatted
  let fromAmount: String
  /// The amount formatted as currency
  let fromFiat: String?

  /// Token being swapped to
  let toToken: BraveWallet.BlockchainToken?
  /// To token network
  let toNetwork: BraveWallet.NetworkInfo
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
  /// Metadata for the NFT being sent
  let nftMetadata: BraveWallet.NftMetadata?

  /// Owner (must not be confused with the caller (fromAddress)
  let owner: String
  /// The token id
  let tokenId: String
  /// Gas fee for the transaction
  let gasFee: GasFee?
}

struct SolanaTxDetails: Equatable {
  struct ParsedSolanaInstruction: Equatable {
    struct KeyValue: Equatable {
      let key: String
      let value: String
    }
    /// Name of the instruction
    let name: String
    /// Key/value pairs of the details of the instruction (accounts, lamports/amount, params).
    let details: [KeyValue]
    /// The Solana instruction that was parsed
    let instruction: BraveWallet.SolanaInstruction

    var toString: String {
      let detailsString = details.map { keyValue in
        let valueIndentedNewLines = keyValue.value.replacingOccurrences(of: "\n", with: "\n")
        return "\(keyValue.key):\n\(valueIndentedNewLines)"
      }.joined(separator: "\n--\n")
      return "\(name)\n--\n\(detailsString)"
    }
  }
  /// From value prior to formatting
  let fromValue: String
  /// From amount formatted
  let fromAmount: String
  /// The token symbol (if available). Token/symbol may be unavailable for Token Program instruction types.
  let symbol: String?
  /// Gas fee for the transaction
  let gasFee: GasFee?
  /// Instructions for the transaction
  let instructions: [ParsedSolanaInstruction]
}

struct FilSendDetails: Equatable {
  /// Token being sent
  let sendToken: BraveWallet.BlockchainToken?
  /// send value prior to formatting
  let sendValue: String
  /// send amount formatted
  let sendAmount: String
  /// The amount formatted as currency
  let sendFiat: String?

  /// Gas premium for the transaction
  let gasPremium: String?
  /// Gas limit for the transaction
  let gasLimit: String?
  /// Gas fee cap for the transaction
  let gasFeeCap: String?
  /// Gas fee for the transaction
  let gasFee: GasFee?
}

extension BraveWallet.TransactionInfo {
  /// Use `TransactionParser` to build a `ParsedTransaction` model for this transaction.
  func parsedTransaction(
    allNetworks: [BraveWallet.NetworkInfo],
    accountInfos: [BraveWallet.AccountInfo],
    userAssets: [BraveWallet.BlockchainToken],
    allTokens: [BraveWallet.BlockchainToken],
    assetRatios: [String: Double],
    nftMetadata: [String: BraveWallet.NftMetadata],
    solEstimatedTxFee: UInt64? = nil,
    currencyFormatter: NumberFormatter,
    decimalFormatStyle: WalletAmountFormatter.DecimalFormatStyle? = nil
  ) -> ParsedTransaction? {
    TransactionParser.parseTransaction(
      transaction: self,
      allNetworks: allNetworks,
      accountInfos: accountInfos,
      userAssets: userAssets,
      allTokens: allTokens,
      assetRatios: assetRatios,
      nftMetadata: nftMetadata,
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

  var ethSwap: EthSwapDetails? {
    guard case .ethSwap(let ethSwapDetails) = details else { return nil }
    return ethSwapDetails
  }

  var tokens: [BraveWallet.BlockchainToken] {
    switch details {
    case .ethSend(let details),
      .erc20Transfer(let details),
      .solSystemTransfer(let details),
      .solSplTokenTransfer(let details):
      if let token = details.fromToken {
        return [token]
      }
    case .ethSwap(let details):
      return [details.fromToken, details.toToken].compactMap { $0 }
    case .ethErc20Approve(let details):
      if let token = details.token {
        return [token]
      }
    case .erc721Transfer(let details):
      if let token = details.fromToken {
        return [token]
      }
    case .filSend(let details):
      if let token = details.sendToken {
        return [token]
      }
    case .btcSend(let details):
      if let token = details.fromToken {
        return [token]
      }
    case .solDappTransaction, .solSwapTransaction, .other:
      break
    }
    return []
  }
}

extension BraveWallet.TransactionInfo {
  /// Contract address for the ERC20 token being approved
  var erc20ApproveTokenContractAddress: String? {
    guard txType == .erc20Approve else { return nil }
    return txDataUnion.ethTxData1559?.baseData.to
  }

  /// Contract address for the ERC20 token being transferred
  var erc20TransferTokenContractAddress: String? {
    guard txType == .erc20Transfer else { return nil }
    return txDataUnion.ethTxData1559?.baseData.to
  }

  /// Contract address for the from and to token being swapped
  var ethSwapTokenContractAddresses: (from: String, to: String)? {
    guard txType == .ethSwap, let swapInfo = swapInfo
    else { return nil }
    return (swapInfo.fromAsset, swapInfo.toAsset)
  }

  /// Contract address for the ERC721 token
  var erc721ContractAddress: String? {
    guard txType == .erc721TransferFrom || txType == .erc721SafeTransferFrom else { return nil }
    return txDataUnion.ethTxData1559?.baseData.to
  }

  /// The contract addresses of the tokens in the transaction
  var tokenContractAddresses: [String] {
    switch txType {
    case .erc20Approve:
      if let erc20ApproveTokenContractAddress {
        return [erc20ApproveTokenContractAddress]
      }
    case .ethSwap:
      if let (fromTokenContractAddress, toTokenContractAddress) = ethSwapTokenContractAddresses {
        return [fromTokenContractAddress, toTokenContractAddress]
      }
    case .erc20Transfer:
      if let erc20TransferTokenContractAddress {
        return [erc20TransferTokenContractAddress]
      }
    case .erc721TransferFrom, .erc721SafeTransferFrom:
      if let erc721ContractAddress {
        return [erc721ContractAddress]
      }
    case .solanaSplTokenTransfer, .solanaSplTokenTransferWithAssociatedTokenAccountCreation:
      if let splTokenMintAddress = txDataUnion.solanaTxData?.tokenAddress {
        return [splTokenMintAddress]
      }
    case .ethSend, .erc1155SafeTransferFrom, .other:
      break
    case .solanaSystemTransfer,
      .solanaCompressedNftTransfer,
      .solanaDappSignTransaction,
      .solanaDappSignAndSendTransaction,
      .solanaSwap:
      break
    case .ethFilForwarderTransfer:
      break
    @unknown default:
      break
    }
    return []
  }
}
