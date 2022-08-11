// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

class AccountActivityStore: ObservableObject {
  private(set) var account: BraveWallet.AccountInfo
  @Published private(set) var assets: [AssetViewModel] = []
  @Published var transactionSummaries: [TransactionSummary] = []
  @Published private(set) var allTokens: [BraveWallet.BlockchainToken] = []
  @Published private(set) var currencyCode: String = CurrencyCode.usd.code {
    didSet {
      currencyFormatter.currencyCode = currencyCode
      update()
    }
  }

  let currencyFormatter: NumberFormatter = .usdCurrencyFormatter

  private let keyringService: BraveWalletKeyringService
  private let walletService: BraveWalletBraveWalletService
  private let rpcService: BraveWalletJsonRpcService
  private let assetRatioService: BraveWalletAssetRatioService
  private let txService: BraveWalletTxService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let solTxManagerProxy: BraveWalletSolanaTxManagerProxy

  init(
    account: BraveWallet.AccountInfo,
    keyringService: BraveWalletKeyringService,
    walletService: BraveWalletBraveWalletService,
    rpcService: BraveWalletJsonRpcService,
    assetRatioService: BraveWalletAssetRatioService,
    txService: BraveWalletTxService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    solTxManagerProxy: BraveWalletSolanaTxManagerProxy
  ) {
    self.account = account
    self.keyringService = keyringService
    self.walletService = walletService
    self.rpcService = rpcService
    self.assetRatioService = assetRatioService
    self.txService = txService
    self.blockchainRegistry = blockchainRegistry
    self.solTxManagerProxy = solTxManagerProxy
    
    self.keyringService.add(self)
    self.rpcService.add(self)
    self.txService.add(self)
    self.walletService.add(self)
    
    walletService.defaultBaseCurrency { [self] currencyCode in
      self.currencyCode = currencyCode
    }
  }

  func update() {
    Task { @MainActor in
      let coin = account.coin
      let network = await rpcService.network(coin)
      let keyring = await keyringService.keyringInfo(coin.keyringId)
      let allTokens: [BraveWallet.BlockchainToken] = await blockchainRegistry.allTokens(network.chainId, coin: network.coin)
      let userVisibleTokens: [BraveWallet.BlockchainToken] = await walletService.userAssets(network.chainId, coin: network.coin)
      self.assets = await fetchAssets(network: network, userVisibleTokens: userVisibleTokens)
      let assetRatios = self.assets.reduce(into: [String: Double](), {
        $0[$1.token.assetRatioId.lowercased()] = Double($1.price)
      })
      self.transactionSummaries = await fetchTransactionSummarys(
        network: network,
        accountInfos: keyring.accountInfos,
        userVisibleTokens: userVisibleTokens,
        allTokens: allTokens,
        assetRatios: assetRatios
      )
    }
  }

  @MainActor private func fetchAssets(
    network: BraveWallet.NetworkInfo,
    userVisibleTokens: [BraveWallet.BlockchainToken]
  ) async -> [AssetViewModel] {
    var updatedAssets = userVisibleTokens.map {
      AssetViewModel(token: $0, decimalBalance: 0, price: "", history: [])
    }
    // fetch price for each asset
    let priceResult = await assetRatioService.priceWithIndividualRetry(
      userVisibleTokens.map { $0.assetRatioId.lowercased() },
      toAssets: [currencyFormatter.currencyCode],
      timeframe: .oneDay
    )
    for price in priceResult.assetPrices {
      if let index = updatedAssets.firstIndex(where: {
        $0.token.assetRatioId.caseInsensitiveCompare(price.fromAsset) == .orderedSame
      }) {
        updatedAssets[index].price = price.price
      }
    }
    // fetch balance for each asset
    typealias TokenBalance = (token: BraveWallet.BlockchainToken, balance: Double?)
    let tokenBalances = await withTaskGroup(of: [TokenBalance].self) { @MainActor group -> [TokenBalance] in
      for token in userVisibleTokens {
        group.addTask { @MainActor in
          let balance = await self.rpcService.balance(for: token, in: self.account)
          return [TokenBalance(token, balance)]
        }
      }
      return await group.reduce([TokenBalance](), { $0 + $1 })
    }
    // update assets with balance
    for tokenBalance in tokenBalances {
      if let value = tokenBalance.balance, let index = updatedAssets.firstIndex(where: {
        $0.token.assetBalanceId.caseInsensitiveCompare(tokenBalance.token.assetBalanceId) == .orderedSame
      }) {
        updatedAssets[index].decimalBalance = value
      }
    }
    return updatedAssets
  }
  
  @MainActor private func fetchTransactionSummarys(
    network: BraveWallet.NetworkInfo,
    accountInfos: [BraveWallet.AccountInfo],
    userVisibleTokens: [BraveWallet.BlockchainToken],
    allTokens: [BraveWallet.BlockchainToken],
    assetRatios: [String: Double]
  ) async -> [TransactionSummary] {
    let transactions = await txService.allTransactionInfo(account.coin, from: account.address)
    var solEstimatedTxFees: [String: UInt64] = [:]
    if account.coin == .sol {
      solEstimatedTxFees = await solTxManagerProxy.estimatedTxFees(for: transactions.map(\.id))
    }
    return transactions
      .sorted(by: { $0.createdTime > $1.createdTime })
      .map { transaction in
        TransactionParser.transactionSummary(
          from: transaction,
          network: network,
          accountInfos: accountInfos,
          visibleTokens: userVisibleTokens,
          allTokens: allTokens,
          assetRatios: assetRatios,
          solEstimatedTxFee: solEstimatedTxFees[transaction.id],
          currencyFormatter: currencyFormatter
        )
      }
  }
  
  func transactionDetailsStore(for transaction: BraveWallet.TransactionInfo) -> TransactionDetailsStore {
    TransactionDetailsStore(
      transaction: transaction,
      keyringService: keyringService,
      walletService: walletService,
      rpcService: rpcService,
      assetRatioService: assetRatioService,
      blockchainRegistry: blockchainRegistry,
      solanaTxManagerProxy: solTxManagerProxy
    )
  }
  
  #if DEBUG
  func previewTransactions() {
    transactionSummaries = [.previewConfirmedSwap, .previewConfirmedSend, .previewConfirmedERC20Approve]
  }
  #endif
}

extension AccountActivityStore: BraveWalletKeyringServiceObserver {
  func keyringCreated(_ keyringId: String) {
  }
  
  func keyringRestored(_ keyringId: String) {
  }
  
  func keyringReset() {
  }
  
  func locked() {
  }
  
  func unlocked() {
  }
  
  func backedUp() {
  }
  
  func accountsChanged() {
  }
  
  func autoLockMinutesChanged() {
  }
  
  func selectedAccountChanged(_ coin: BraveWallet.CoinType) {
    keyringService.keyringInfo(coin.keyringId) { [self] keyringInfo in
      keyringService.selectedAccount(coin) { [self] accountAddress in
        account = keyringInfo.accountInfos.first(where: { $0.address == accountAddress }) ?? keyringInfo.accountInfos.first!
        update()
      }
    }
  }
}

extension AccountActivityStore: BraveWalletJsonRpcServiceObserver {
  func chainChangedEvent(_ chainId: String, coin: BraveWallet.CoinType) {
    DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
      // Handle small gap between chain changing and txController having the correct chain Id
      self.update()
    }
  }
  func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }
  func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
}

extension AccountActivityStore: BraveWalletTxServiceObserver {
  func onNewUnapprovedTx(_ txInfo: BraveWallet.TransactionInfo) {
    update()
  }
  func onTransactionStatusChanged(_ txInfo: BraveWallet.TransactionInfo) {
    update()
  }
  func onUnapprovedTxUpdated(_ txInfo: BraveWallet.TransactionInfo) {
  }
}

extension AccountActivityStore: BraveWalletBraveWalletServiceObserver {
  public func onActiveOriginChanged(_ originInfo: BraveWallet.OriginInfo) {
  }

  public func onDefaultWalletChanged(_ wallet: BraveWallet.DefaultWallet) {
  }

  public func onDefaultBaseCurrencyChanged(_ currency: String) {
    currencyCode = currency
  }

  public func onDefaultBaseCryptocurrencyChanged(_ cryptocurrency: String) {
  }

  public func onNetworkListChanged() {
  }
  
  func onDefaultEthereumWalletChanged(_ wallet: BraveWallet.DefaultWallet) {
  }
  
  func onDefaultSolanaWalletChanged(_ wallet: BraveWallet.DefaultWallet) {
  }
}
