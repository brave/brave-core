// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore

class TransactionDetailsStore: ObservableObject, WalletObserverStore {
  
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
  private let assetManager: WalletUserAssetManagerType
  /// Cache for storing `BlockchainToken`s that are not in user assets or our token registry.
  /// This could occur with a dapp creating a transaction.
  private var tokenInfoCache: [String: BraveWallet.BlockchainToken] = [:]
  
  var isObserving: Bool = false
  
  init(
    transaction: BraveWallet.TransactionInfo,
    keyringService: BraveWalletKeyringService,
    walletService: BraveWalletBraveWalletService,
    rpcService: BraveWalletJsonRpcService,
    assetRatioService: BraveWalletAssetRatioService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    solanaTxManagerProxy: BraveWalletSolanaTxManagerProxy,
    userAssetManager: WalletUserAssetManagerType
  ) {
    self.transaction = transaction
    self.keyringService = keyringService
    self.walletService = walletService
    self.rpcService = rpcService
    self.assetRatioService = assetRatioService
    self.blockchainRegistry = blockchainRegistry
    self.solanaTxManagerProxy = solanaTxManagerProxy
    self.assetManager = userAssetManager
    
    walletService.defaultBaseCurrency { [self] currencyCode in
      self.currencyCode = currencyCode
    }
  }
  
  func update() {
    Task { @MainActor in
      let coin = transaction.coin
      let networksForCoin = await rpcService.allNetworks(coin)
      guard let network = networksForCoin.first(where: { $0.chainId == transaction.chainId }) else {
        // Transactions should be removed if their network is removed
        // https://github.com/brave/brave-browser/issues/30234
        assertionFailure("The NetworkInfo for the transaction's chainId (\(transaction.chainId)) is unavailable")
        return
      }
      self.network = network
      var allTokens: [BraveWallet.BlockchainToken] = await blockchainRegistry.allTokens(network.chainId, coin: network.coin) + tokenInfoCache.map(\.value)
      let userAssets: [BraveWallet.BlockchainToken] = assetManager.getAllUserAssetsInNetworkAssets(networks: [network], includingUserDeleted: true).flatMap { $0.tokens }
      let unknownTokenContractAddresses = transaction.tokenContractAddresses
        .filter { contractAddress in
          !userAssets.contains(where: { $0.contractAddress(in: network).caseInsensitiveCompare(contractAddress) == .orderedSame })
          && !allTokens.contains(where: { $0.contractAddress(in: network).caseInsensitiveCompare(contractAddress) == .orderedSame })
          && !tokenInfoCache.keys.contains(where: { $0.caseInsensitiveCompare(contractAddress) == .orderedSame })
        }
      if !unknownTokenContractAddresses.isEmpty {
        let unknownTokens = await assetRatioService.fetchTokens(for: unknownTokenContractAddresses)
        for unknownToken in unknownTokens {
          tokenInfoCache[unknownToken.contractAddress] = unknownToken
        }
        allTokens.append(contentsOf: unknownTokens)
      }
      
      let priceResult = await assetRatioService.priceWithIndividualRetry(
        userAssets.map { $0.assetRatioId.lowercased() },
        toAssets: [currencyFormatter.currencyCode],
        timeframe: .oneDay
      )
      let assetRatios = priceResult.assetPrices.reduce(into: [String: Double]()) {
        $0[$1.fromAsset] = Double($1.price)
      }
      var solEstimatedTxFee: UInt64?
      if transaction.coin == .sol {
        (solEstimatedTxFee, _, _) = await solanaTxManagerProxy.estimatedTxFee(network.chainId, txMetaId: transaction.id)
      }
      let allAccounts = await keyringService.allAccounts().accounts
      guard let parsedTransaction = transaction.parsedTransaction(
        network: network,
        accountInfos: allAccounts,
        userAssets: userAssets,
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
        self.value = String(format: "%@ %@", details.fromAmount, details.fromToken?.symbol ?? "")
        self.fiat = details.fromFiat
        if let fromToken = details.fromToken, let tokenPrice = assetRatios[fromToken.assetRatioId.lowercased()] {
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
        var token = details.token
        if token == nil {
          token = await self.fetchTokenInfo(for: details.tokenContractAddress)
        }
        
        self.title = Strings.Wallet.approveNetworkButtonTitle
        self.value = String(format: "%@ %@", details.approvalAmount, token?.symbol ?? "")
        if let token = token, let tokenPrice = assetRatios[token.assetRatioId.lowercased()] {
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
      case let .solDappTransaction(details):
        self.title = Strings.Wallet.solanaDappTransactionTitle
        self.value = details.fromAmount
      case let .solSwapTransaction(details):
        self.title = Strings.Wallet.solanaSwapTransactionTitle
        self.value = details.fromAmount
      case let .filSend(details):
        self.title = Strings.Wallet.sent
        self.value = String(format: "%@ %@", details.sendAmount, details.sendToken?.symbol ?? "")
        self.fiat = details.sendFiat
        if let sendToken = details.sendToken, let tokenPrice = assetRatios[sendToken.assetRatioId.lowercased()] {
          self.marketPrice = currencyFormatter.string(from: NSNumber(value: tokenPrice)) ?? "$0.00"
        }
      case .other:
        break
      }
      if let gasFee = parsedTransaction.gasFee {
        self.gasFee = String(format: "%@ %@\n%@", gasFee.fee, parsedTransaction.networkSymbol, gasFee.fiat)
      }
    }
  }
  
  @MainActor private func fetchTokenInfo(for contractAddress: String) async -> BraveWallet.BlockchainToken? {
    if let cachedToken = tokenInfoCache[contractAddress] {
      return cachedToken
    }
    let tokenInfo = await assetRatioService.tokenInfo(contractAddress)
    guard let tokenInfo = tokenInfo else { return nil }
    self.tokenInfoCache[contractAddress] = tokenInfo
    return tokenInfo
  }
}
