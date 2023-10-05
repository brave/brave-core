/* Copyright 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import BraveCore
import SwiftUI

class TransactionsActivityStore: ObservableObject, WalletObserverStore {
  @Published var transactionSummaries: [TransactionSummary] = []
  
  @Published private(set) var currencyCode: String = CurrencyCode.usd.code {
    didSet {
      currencyFormatter.currencyCode = currencyCode
      guard oldValue != currencyCode else { return }
      update()
    }
  }
  @Published var networkFilters: [Selectable<BraveWallet.NetworkInfo>] = [] {
    didSet {
      guard !oldValue.isEmpty else { return } // initial assignment to `networkFilters`
      update()
    }
  }
  
  let currencyFormatter: NumberFormatter = .usdCurrencyFormatter
  
  private var solEstimatedTxFeesCache: [String: UInt64] = [:]
  private var assetPricesCache: [String: Double] = [:]
  
  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let assetRatioService: BraveWalletAssetRatioService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let txService: BraveWalletTxService
  private let solTxManagerProxy: BraveWalletSolanaTxManagerProxy
  private let assetManager: WalletUserAssetManagerType
  private var keyringServiceObserver: KeyringServiceObserver?
  private var txServiceObserver: TxServiceObserver?
  private var walletServiceObserver: WalletServiceObserver?
  
  var isObserving: Bool {
    keyringServiceObserver != nil && txServiceObserver != nil && walletServiceObserver != nil
  }
  
  init(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    assetRatioService: BraveWalletAssetRatioService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    txService: BraveWalletTxService,
    solTxManagerProxy: BraveWalletSolanaTxManagerProxy,
    userAssetManager: WalletUserAssetManagerType
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.assetRatioService = assetRatioService
    self.blockchainRegistry = blockchainRegistry
    self.txService = txService
    self.solTxManagerProxy = solTxManagerProxy
    self.assetManager = userAssetManager
    
    self.setupObservers()

    Task { @MainActor in
      self.currencyCode = await walletService.defaultBaseCurrency()
    }
  }
  
  func tearDown() {
    keyringServiceObserver = nil
    txServiceObserver = nil
    walletServiceObserver = nil
  }
  
  func setupObservers() {
    guard !isObserving else { return }
    self.keyringServiceObserver = KeyringServiceObserver(
      keyringService: keyringService,
      _accountsChanged: { [weak self] in
        self?.update()
      },
      _accountsAdded: { [weak self] _ in
        self?.update()
      }
    )
    self.txServiceObserver = TxServiceObserver(
      txService: txService,
      _onNewUnapprovedTx: { [weak self] _ in
        self?.update()
      },
      _onUnapprovedTxUpdated: { [weak self] _ in
        self?.update()
      },
      _onTransactionStatusChanged: { [weak self] _ in
        self?.update()
      },
      _onTxServiceReset: { [weak self] in
        self?.update()
      }
    )
    self.walletServiceObserver = WalletServiceObserver(
      walletService: walletService,
      _onNetworkListChanged: { [weak self] in
        Task { @MainActor [self] in
          // A network was added or removed, update our network filters for the change.
          guard let rpcService = self?.rpcService else { return }
          self?.networkFilters = await rpcService.allNetworksForSupportedCoins().map { network in
            let existingSelectionValue = self?.networkFilters.first(where: { $0.model.chainId == network.chainId})?.isSelected
            return .init(isSelected: existingSelectionValue ?? true, model: network)
          }
        }
      }
    )
  }
  
  private var updateTask: Task<Void, Never>?
  func update() {
    updateTask?.cancel()
    updateTask = Task { @MainActor in
      let allKeyrings = await self.keyringService.keyrings(
        for: WalletConstants.supportedCoinTypes()
      )
      let allAccountInfos = allKeyrings.flatMap(\.accountInfos)
      // setup network filters if not currently setup
      if self.networkFilters.isEmpty {
        self.networkFilters = await self.rpcService.allNetworksForSupportedCoins().map {
          .init(isSelected: true, model: $0)
        }
      }
      let networks = networkFilters.filter(\.isSelected).map(\.model)
      let networksForCoin: [BraveWallet.CoinType: [BraveWallet.NetworkInfo]] = Dictionary(grouping: networks, by: \.coin)
      let allNetworksAllCoins = networksForCoin.values.flatMap { $0 }
      
      let allTransactions = await txService.allTransactions(
        networksForCoin: networksForCoin, for: allKeyrings
      ).filter { $0.txStatus != .rejected }
      let userVisibleTokens = assetManager.getAllVisibleAssetsInNetworkAssets(networks: allNetworksAllCoins).flatMap(\.tokens)
      let allTokens = await blockchainRegistry.allTokens(
        in: allNetworksAllCoins
      ).flatMap(\.tokens)
      guard !Task.isCancelled else { return }
      // display transactions prior to network request to fetch
      // estimated solana tx fees & asset prices
      self.transactionSummaries = self.transactionSummaries(
        transactions: allTransactions,
        networksForCoin: networksForCoin,
        accountInfos: allAccountInfos,
        userVisibleTokens: userVisibleTokens,
        allTokens: allTokens,
        assetRatios: assetPricesCache,
        solEstimatedTxFees: solEstimatedTxFeesCache
      )
      guard !self.transactionSummaries.isEmpty else { return }

      if allTransactions.contains(where: { $0.coin == .sol }) {
        let solTransactions = allTransactions.filter { $0.coin == .sol }
        await updateSolEstimatedTxFeesCache(solTransactions)
      }

      let allVisibleTokenAssetRatioIds = userVisibleTokens.map(\.assetRatioId)
      await updateAssetPricesCache(assetRatioIds: allVisibleTokenAssetRatioIds)

      guard !Task.isCancelled else { return }
      self.transactionSummaries = self.transactionSummaries(
        transactions: allTransactions,
        networksForCoin: networksForCoin,
        accountInfos: allAccountInfos,
        userVisibleTokens: userVisibleTokens,
        allTokens: allTokens,
        assetRatios: assetPricesCache,
        solEstimatedTxFees: solEstimatedTxFeesCache
      )
    }
  }
  
  private func transactionSummaries(
    transactions: [BraveWallet.TransactionInfo],
    networksForCoin: [BraveWallet.CoinType: [BraveWallet.NetworkInfo]],
    accountInfos: [BraveWallet.AccountInfo],
    userVisibleTokens: [BraveWallet.BlockchainToken],
    allTokens: [BraveWallet.BlockchainToken],
    assetRatios: [String: Double],
    solEstimatedTxFees: [String: UInt64]
  ) -> [TransactionSummary] {
    transactions.compactMap { transaction in
      guard let networks = networksForCoin[transaction.coin], let network = networks.first(where: { $0.chainId == transaction.chainId }) else {
        return nil
      }
      return TransactionParser.transactionSummary(
        from: transaction,
        network: network,
        accountInfos: accountInfos,
        visibleTokens: userVisibleTokens,
        allTokens: allTokens,
        assetRatios: assetRatios,
        solEstimatedTxFee: solEstimatedTxFees[transaction.id],
        currencyFormatter: currencyFormatter
      )
    }.sorted(by: { $0.createdTime > $1.createdTime })
  }
  
  @MainActor private func updateSolEstimatedTxFeesCache(_ solTransactions: [BraveWallet.TransactionInfo]) async {
    let fees = await solTxManagerProxy.estimatedTxFees(for: solTransactions)
    for (key, value) in fees { // update cached values
      self.solEstimatedTxFeesCache[key] = value
    }
  }
  
  @MainActor private func updateAssetPricesCache(assetRatioIds: [String]) async {
    let prices = await assetRatioService.fetchPrices(
      for: assetRatioIds,
      toAssets: [currencyFormatter.currencyCode],
      timeframe: .oneDay
    ).compactMapValues { Double($0) }
    for (key, value) in prices { // update cached values
      self.assetPricesCache[key] = value
    }
  }
  
  func transactionDetailsStore(
    for transaction: BraveWallet.TransactionInfo
  ) -> TransactionDetailsStore {
    TransactionDetailsStore(
      transaction: transaction,
      keyringService: keyringService,
      walletService: walletService,
      rpcService: rpcService,
      assetRatioService: assetRatioService,
      blockchainRegistry: blockchainRegistry,
      solanaTxManagerProxy: solTxManagerProxy,
      userAssetManager: assetManager
    )
  }
}
