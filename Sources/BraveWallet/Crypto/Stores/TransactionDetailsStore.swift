// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore

class TransactionDetailsStore: ObservableObject {
  
  let transaction: BraveWallet.TransactionInfo
  @Published private(set) var parsedTransaction: ParsedTransaction?
  @Published private(set) var network: BraveWallet.NetworkInfo?
  @Published private(set) var title: String?
  @Published private(set) var value: String?
  @Published private(set) var fiat: String?
  @Published private(set) var gasFee: String?
  @Published private(set) var marketPrice: String?
  
  @Published private(set) var currencyCode: String = CurrencyCode.usd.code {
    didSet {
      currencyFormatter.currencyCode = currencyCode
      update()
    }
  }
  let currencyFormatter: NumberFormatter = .usdCurrencyFormatter
    .then {
      $0.minimumFractionDigits = 2
      $0.maximumFractionDigits = 6
    }
  
  private let keyringService: BraveWalletKeyringService
  private let walletService: BraveWalletBraveWalletService
  private let rpcService: BraveWalletJsonRpcService
  private let assetRatioService: BraveWalletAssetRatioService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let solanaTxManagerProxy: BraveWalletSolanaTxManagerProxy
  
  init(
    transaction: BraveWallet.TransactionInfo,
    keyringService: BraveWalletKeyringService,
    walletService: BraveWalletBraveWalletService,
    rpcService: BraveWalletJsonRpcService,
    assetRatioService: BraveWalletAssetRatioService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    solanaTxManagerProxy: BraveWalletSolanaTxManagerProxy
  ) {
    self.transaction = transaction
    self.keyringService = keyringService
    self.walletService = walletService
    self.rpcService = rpcService
    self.assetRatioService = assetRatioService
    self.blockchainRegistry = blockchainRegistry
    self.solanaTxManagerProxy = solanaTxManagerProxy
    
    walletService.defaultBaseCurrency { [self] currencyCode in
      self.currencyCode = currencyCode
    }
  }
  
  func update() {
    Task { @MainActor in
      let coin = transaction.coin
      let network = await rpcService.network(coin)
      self.network = network
      let keyring = await keyringService.keyringInfo(coin.keyringId)
      let allTokens: [BraveWallet.BlockchainToken] = await blockchainRegistry.allTokens(network.chainId, coin: network.coin)
      let userVisibleTokens: [BraveWallet.BlockchainToken] = await walletService.userAssets(network.chainId, coin: network.coin)
      let priceResult = await assetRatioService.priceWithIndividualRetry(
        userVisibleTokens.map { $0.assetRatioId.lowercased() },
        toAssets: [currencyFormatter.currencyCode],
        timeframe: .oneDay
      )
      let assetRatios = priceResult.assetPrices.reduce(into: [String: Double]()) {
        $0[$1.fromAsset] = Double($1.price)
      }
      var solEstimatedTxFee: UInt64?
      if transaction.coin == .sol {
        (solEstimatedTxFee, _, _) = await solanaTxManagerProxy.estimatedTxFee(transaction.id)
      }
      guard let parsedTransaction = transaction.parsedTransaction(
        network: network,
        accountInfos: keyring.accountInfos,
        visibleTokens: userVisibleTokens,
        allTokens: allTokens,
        assetRatios: assetRatios,
        solEstimatedTxFee: solEstimatedTxFee,
        currencyFormatter: currencyFormatter
      ) else {
        return
      }
      self.parsedTransaction = parsedTransaction
      self.currencyFormatter.maximumFractionDigits = 2 // use max. 2 digits for market price calculation
      
      switch parsedTransaction.details {
      case let .ethSend(details),
        let .erc20Transfer(details),
        let .solSystemTransfer(details),
        let .solSplTokenTransfer(details):
        self.title = Strings.Wallet.sent
        self.value = String(format: "%@ %@", details.fromAmount, details.fromToken.symbol)
        self.fiat = details.fromFiat
        if let tokenPrice = assetRatios[details.fromToken.assetRatioId.lowercased()] {
          self.marketPrice = currencyFormatter.string(from: NSNumber(value: tokenPrice)) ?? "$0.00"
        }
      case let .ethSwap(details):
        self.title = Strings.Wallet.swap
        if let fromToken = details.fromToken {
          self.value = String(format: "%@ %@", details.fromAmount, fromToken.symbol)
          if let tokenPrice = assetRatios[fromToken.assetRatioId.lowercased()] {
            self.marketPrice = currencyFormatter.string(from: NSNumber(value: tokenPrice)) ?? "$0.00"
          }
        } else {
          self.value = details.fromAmount
        }
        self.fiat = details.fromFiat
      case let .ethErc20Approve(details):
        self.title = Strings.Wallet.approveNetworkButtonTitle
        self.value = String(format: "%@ %@", details.approvalAmount, details.token.symbol)
        if let tokenPrice = assetRatios[details.token.assetRatioId.lowercased()] {
          self.marketPrice = currencyFormatter.string(from: NSNumber(value: tokenPrice)) ?? "$0.00"
        }
      case let .erc721Transfer(details):
        self.title = Strings.Wallet.swap
        if let fromToken = details.fromToken {
          self.value = String(format: "%@ %@", details.fromAmount, fromToken.symbol)
          if let tokenPrice = assetRatios[fromToken.assetRatioId.lowercased()] {
            self.marketPrice = currencyFormatter.string(from: NSNumber(value: tokenPrice)) ?? "$0.00"
          }
        } else {
          self.value = details.fromAmount
        }
      case .other:
        break
      }
      if let gasFee = parsedTransaction.gasFee {
        self.gasFee = String(format: "%@ %@\n%@", gasFee.fee, parsedTransaction.networkSymbol, gasFee.fiat)
      }
    }
  }
}
