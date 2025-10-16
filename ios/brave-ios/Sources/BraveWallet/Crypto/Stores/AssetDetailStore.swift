// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import SwiftUI

struct AccountAssetViewModel: Identifiable {
  var account: BraveWallet.AccountInfo
  fileprivate var decimalBalance: Double
  var balance: String
  var fiatBalance: String

  var id: String {
    account.id
  }
}

enum AssetDetailType: Identifiable {
  case blockchainToken(BraveWallet.BlockchainToken)
  case coinMarket(BraveWallet.CoinMarket)

  var id: String {
    switch self {
    case .blockchainToken(let token):
      return token.id
    case .coinMarket(let coinMarket):
      return coinMarket.id
    }
  }
}

class AssetDetailStore: ObservableObject, WalletObserverStore {
  @Published private(set) var isInitialState: Bool = true
  @Published private(set) var isLoadingPrice: Bool = false
  @Published private(set) var isLoadingChart: Bool = false
  @Published private(set) var price: Double = 0
  @Published private(set) var priceDelta: String = "0.00%"
  @Published private(set) var priceIsDown: Bool = false
  @Published private(set) var priceHistory: [BraveWallet.AssetTimePrice] = []
  @Published var timeframe: BraveWallet.AssetPriceTimeframe = .oneDay {
    didSet {
      if timeframe != oldValue {
        update()
      }
    }
  }
  @Published private(set) var isLoadingAccountBalances: Bool = false
  @Published private(set) var nonZeroBalanceAccounts: [AccountAssetViewModel] = []
  @Published private(set) var transactionSections: [TransactionSection] = []
  @Published private(set) var isSendSupported: Bool = false
  @Published private(set) var isSwapSupported: Bool = false
  @Published private(set) var currencyCode: String = CurrencyCode.usd.code {
    didSet {
      currencyFormatter.currencyCode = currencyCode
      guard oldValue != currencyCode,  // only if currency code changed
        !isInitialState  // only update if we're not in initial state
      else { return }
      update()
    }
  }
  @Published private(set) var network: BraveWallet.NetworkInfo?
  /// For buy option
  @Published private(set) var meldCryptoCurrency: BraveWallet.MeldCryptoCurrency?

  let currencyFormatter: NumberFormatter = .usdCurrencyFormatter

  var totalBalance: Double {
    nonZeroBalanceAccounts
      .compactMap { Double($0.balance) }
      .reduce(0, +)
  }

  var isBuySupported: Bool {
    meldCryptoCurrency != nil
  }

  private let assetRatioService: BraveWalletAssetRatioService
  private let keyringService: BraveWalletKeyringService
  private let walletService: BraveWalletBraveWalletService
  private let rpcService: BraveWalletJsonRpcService
  private let txService: BraveWalletTxService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let solTxManagerProxy: BraveWalletSolanaTxManagerProxy
  private let ipfsApi: IpfsAPI
  private let swapService: BraveWalletSwapService
  private let bitcoinWalletService: BraveWalletBitcoinWalletService
  private let zcashWalletService: BraveWalletZCashWalletService
  private let meldIntegrationService: BraveWalletMeldIntegrationService
  private let assetManager: WalletUserAssetManagerType
  /// Cache for storing `BlockchainToken`s that are not in user assets or our token registry.
  /// This could occur with a dapp creating a transaction.
  private var tokenInfoCache: [BraveWallet.BlockchainToken] = []
  private var keyringServiceObserver: KeyringServiceObserver?
  private var txServiceObserver: TxServiceObserver?
  private var walletServiceObserver: WalletServiceObserver?
  let assetDetailType: AssetDetailType
  var assetDetailToken: BraveWallet.BlockchainToken {
    switch assetDetailType {
    case .blockchainToken(let token):
      return token
    case .coinMarket(let coinMarket):
      return .init().then {
        if let matchedToken = depositableTokens.first(where: { token in
          token.symbol.caseInsensitiveCompare(coinMarket.symbol) == .orderedSame
        }) {
          $0.contractAddress = matchedToken.contractAddress
          $0.coin = matchedToken.coin
          $0.chainId = matchedToken.chainId
        }
        $0.coingeckoId = coinMarket.id
        $0.logo = coinMarket.image
        $0.symbol = coinMarket.symbol
        $0.name = coinMarket.name
      }
    }
  }

  var isObserving: Bool {
    keyringServiceObserver != nil && txServiceObserver != nil && walletServiceObserver != nil
  }

  // All account info that has the same coin type as this asset's
  // This should only be used for asset from token registry
  // Token from market does not guarantee the correct coin type.
  var allAccountsForToken: [BraveWallet.AccountInfo] = []
  private var depositableTokens: [BraveWallet.BlockchainToken] = []

  init(
    assetRatioService: BraveWalletAssetRatioService,
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    txService: BraveWalletTxService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    solTxManagerProxy: BraveWalletSolanaTxManagerProxy,
    ipfsApi: IpfsAPI,
    swapService: BraveWalletSwapService,
    bitcoinWalletService: BraveWalletBitcoinWalletService,
    zcashWalletService: BraveWalletZCashWalletService,
    meldIntegrationService: BraveWalletMeldIntegrationService,
    userAssetManager: WalletUserAssetManagerType,
    assetDetailType: AssetDetailType
  ) {
    self.assetRatioService = assetRatioService
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.txService = txService
    self.blockchainRegistry = blockchainRegistry
    self.solTxManagerProxy = solTxManagerProxy
    self.ipfsApi = ipfsApi
    self.swapService = swapService
    self.bitcoinWalletService = bitcoinWalletService
    self.zcashWalletService = zcashWalletService
    self.meldIntegrationService = meldIntegrationService
    self.assetManager = userAssetManager
    self.assetDetailType = assetDetailType

    self.setupObservers()

    walletService.defaultBaseCurrency { [self] currencyCode in
      self.currencyCode = currencyCode
    }
  }

  func tearDown() {
    keyringServiceObserver = nil
    txServiceObserver = nil
    walletServiceObserver = nil
    transactionDetailsStore?.tearDown()
  }

  func setupObservers() {
    guard !isObserving else { return }
    self.assetManager.addUserAssetDataObserver(self)
    self.keyringServiceObserver = KeyringServiceObserver(
      keyringService: keyringService,
      _accountsChanged: { [weak self] in
        self?.update()
      }
    )
    self.txServiceObserver = TxServiceObserver(
      txService: txService,
      _onTransactionStatusChanged: { [weak self] _ in
        self?.update()
      }
    )
    self.walletServiceObserver = WalletServiceObserver(
      walletService: walletService,
      _onDefaultBaseCurrencyChanged: { [weak self] currency in
        self?.currencyCode = currency
      }
    )
  }

  private let percentFormatter = NumberFormatter().then {
    $0.numberStyle = .percent
    $0.maximumFractionDigits = 2
  }

  private var updateTask: Task<Void, Never>?
  private var solEstimatedTxFeesCache: [String: UInt64] = [:]
  private var assetPricesCache: [BraveWallet.AssetPrice] = []
  public func update() {
    updateTask?.cancel()
    updateTask = Task { @MainActor in
      self.isLoadingPrice = true
      self.isLoadingChart = true
      let allAccounts = await keyringService.allAccounts()

      switch assetDetailType {
      case .blockchainToken(let token):
        // not come from Market tab
        let allNetworks = await rpcService.allNetworks()
        let selectedNetwork = await rpcService.network(coin: token.coin, origin: nil)
        let network =
          allNetworks.first(where: { $0.coin == token.coin && $0.chainId == token.chainId })
          ?? selectedNetwork
        self.network = network
        let (matchedCryptoCurrency, _) =
          await meldIntegrationService.convertToMeldCryptoCurrency(for: assetDetailToken)
        self.meldCryptoCurrency = matchedCryptoCurrency
        self.isSendSupported = true
        self.isSwapSupported = await swapService.isSwapSupported(chainId: token.chainId)

        // fetch accounts
        self.allAccountsForToken = allAccounts.accounts.accountsFor(network: network)
        var updatedAccounts = allAccountsForToken.map {
          AccountAssetViewModel(account: $0, decimalBalance: 0.0, balance: "", fiatBalance: "")
        }

        // fetch prices for the asset
        let (prices, priceHistory) = await fetchPriceInfo(for: token)
        self.priceHistory = priceHistory
        self.isLoadingPrice = false
        self.isInitialState = false
        self.isLoadingChart = false

        if let assetPrice = prices.getTokenPrice(for: token),
          let value = Double(assetPrice.price)
        {
          self.price = value
          if let deltaValue = Double(assetPrice.percentageChange24h) {
            self.priceIsDown = deltaValue < 0
            self.priceDelta =
              self.percentFormatter.string(from: NSNumber(value: deltaValue / 100.0)) ?? ""
          }
          for index in 0..<updatedAccounts.count {
            updatedAccounts[index].fiatBalance =
              self.currencyFormatter.formatAsFiat(
                updatedAccounts[index].decimalBalance * self.price
              ) ?? ""
          }
        }

        // fetch accounts balance
        self.nonZeroBalanceAccounts = await fetchAccountBalances(updatedAccounts, network: network)

        // fetch transactions
        let userAssets = await assetManager.getAllUserAssetsInNetworkAssets(
          networks: [network],
          includingUserDeleted: true
        ).flatMap { $0.tokens }
        let allTokens = await blockchainRegistry.allTokens(
          chainId: network.chainId,
          coin: network.coin
        )
        let allTransactions = await txService.allTransactions(
          networksForCoin: [network.coin: [network]],
          for: allAccountsForToken
        )

        let ethTransactions = allTransactions.filter { $0.coin == .eth }
        if !ethTransactions.isEmpty {  // we can only fetch unknown Ethereum tokens
          let unknownTokenInfo = ethTransactions.unknownTokenContractAddressChainIdPairs(
            knownTokens: userAssets + allTokens + tokenInfoCache
          )
          updateUnknownTokens(for: unknownTokenInfo)
        }
        guard !Task.isCancelled else { return }
        // display transactions prior to network request to fetch prices and estimated solana tx fee
        // 1. build transaction sections
        self.transactionSections = buildTransactionSections(
          transactions: allTransactions,
          allNetworks: allNetworks,
          accountInfos: allAccountsForToken,
          userAssets: userAssets,
          allTokens: allTokens,
          assetRatios: assetPricesCache,
          nftMetadata: [:],  // NFT Detail is in another view
          solEstimatedTxFees: solEstimatedTxFeesCache
        )
        guard !self.transactionSections.isEmpty else { return }

        // 2. update estimated tx fee to build tx sections again
        if allTransactions.contains(where: { $0.coin == .sol }) {
          let solTransactions = allTransactions.filter { $0.coin == .sol }
          await updateSolEstimatedTxFeesCache(solTransactions)
        }

        guard !Task.isCancelled else { return }
        self.transactionSections = buildTransactionSections(
          transactions: allTransactions,
          allNetworks: allNetworks,
          accountInfos: allAccountsForToken,
          userAssets: userAssets,
          allTokens: allTokens,
          assetRatios: assetPricesCache,
          nftMetadata: [:],  // NFT Detail is in another view
          solEstimatedTxFees: solEstimatedTxFeesCache
        )

        // 3. update assets price t build tx section again
        await updateAssetPricesCache(for: userAssets)

        guard !Task.isCancelled else { return }
        self.transactionSections = buildTransactionSections(
          transactions: allTransactions,
          allNetworks: allNetworks,
          accountInfos: allAccountsForToken,
          userAssets: userAssets,
          allTokens: allTokens,
          assetRatios: assetPricesCache,
          nftMetadata: [:],  // NFT Detail is in another view
          solEstimatedTxFees: solEstimatedTxFeesCache
        )
      case .coinMarket(let coinMarket):
        // comes from Market tab
        self.price = coinMarket.currentPrice
        self.priceDelta =
          self.percentFormatter.string(
            from: NSNumber(value: coinMarket.priceChangePercentage24h / 100.0)
          ) ?? ""
        self.priceIsDown = coinMarket.priceChangePercentage24h < 0

        let priceHistory = await self.fetchPriceHistory(for: coinMarket.id)
        self.priceHistory = priceHistory
        self.isLoadingPrice = false
        self.isInitialState = false
        self.isLoadingChart = false

        let allNetworks = await rpcService.allNetworksForSupportedCoins()
        let allUserAssets = await assetManager.getAllUserAssetsInNetworkAssets(
          networks: allNetworks,
          includingUserDeleted: false
        )
        let allUserTokens = allUserAssets.flatMap(\.tokens)
        let allBlockchainTokens = await blockchainRegistry.allTokens(in: allNetworks)
          .flatMap(\.tokens)
        self.depositableTokens = allUserTokens + allBlockchainTokens

        let (matchedCryptoCurrency, _) =
          await meldIntegrationService.convertToMeldCryptoCurrency(for: assetDetailToken)
        self.meldCryptoCurrency = matchedCryptoCurrency

        // fetch accounts if this coinMarket is depositable
        if let depositableToken = convertCoinMarketToDepositableToken(symbol: coinMarket.symbol) {
          let depositableTokenKeyringId = BraveWallet.KeyringId.keyringId(
            for: depositableToken.coin,
            on: depositableToken.chainId
          )
          self.allAccountsForToken = allAccounts.accounts.filter {
            $0.keyringId == depositableTokenKeyringId
          }
        } else {
          self.allAccountsForToken = []
        }

        // below is all not supported from Market tab
        self.isSendSupported = false
        self.isSwapSupported = false
        self.nonZeroBalanceAccounts = []
        self.transactionSections = []
      }
    }
  }

  func convertCoinMarketToDepositableToken(symbol: String) -> BraveWallet.BlockchainToken? {
    let token = depositableTokens.first {
      $0.symbol.caseInsensitiveCompare(symbol) == .orderedSame
    }
    return token
  }

  // Return given token's price history
  @MainActor private func fetchPriceHistory(
    for assetRatioId: String
  ) async -> [BraveWallet.AssetTimePrice] {
    // fetch price history for the asset
    let (_, priceHistory) = await assetRatioService.priceHistory(
      asset: assetRatioId,
      vsAsset: currencyFormatter.currencyCode,
      timeframe: timeframe
    )

    return priceHistory
  }

  // Return given token's asset prices and price history
  @MainActor private func fetchPriceInfo(
    for token: BraveWallet.BlockchainToken
  ) async -> ([BraveWallet.AssetPrice], [BraveWallet.AssetTimePrice]) {
    // fetch prices for the asset
    let prices = await assetRatioService.fetchPrices(
      for: [token],
      vsCurrency: currencyFormatter.currencyCode
    )

    // fetch price history for the asset
    let priceHistory = await fetchPriceHistory(for: token.assetRatioId)

    return (prices, priceHistory)
  }

  @MainActor func handleTransactionFollowUpAction(
    _ action: TransactionFollowUpAction,
    transaction: BraveWallet.TransactionInfo
  ) async -> String? {
    guard
      let errorMessage = await txService.handleTransactionFollowUpAction(
        action,
        transaction: transaction
      )
    else {
      return nil
    }
    return errorMessage
  }

  @MainActor private func fetchAccountBalances(
    _ accountAssetViewModels: [AccountAssetViewModel],
    network: BraveWallet.NetworkInfo
  ) async -> [AccountAssetViewModel] {
    guard case .blockchainToken(let token) = assetDetailType
    else { return [] }

    var accountAssetViewModels = accountAssetViewModels
    isLoadingAccountBalances = true
    typealias AccountBalance = (account: BraveWallet.AccountInfo, balance: Double?)
    let tokenBalances = await withTaskGroup(of: [AccountBalance].self) {
      @MainActor group -> [AccountBalance] in
      for accountAssetViewModel in accountAssetViewModels {
        group.addTask { @MainActor in
          var tokenBalance: Double?
          if accountAssetViewModel.account.coin == .btc {
            tokenBalance = await self.bitcoinWalletService.fetchBTCBalance(
              accountId: accountAssetViewModel.account.accountId,
              type: .total
            )
          } else {
            if let assetBalancePerAccount = self.assetManager.getAssetBalances(
              for: token,
              account: accountAssetViewModel.account.id
            )?.first {
              tokenBalance = Double(assetBalancePerAccount.balance)
            } else {
              if accountAssetViewModel.account.coin == .zec {
                tokenBalance = await self.zcashWalletService.fetchZECTransparentBalances(
                  accountId: accountAssetViewModel.account.accountId
                )
              } else {
                tokenBalance = await self.rpcService.balance(
                  for: token,
                  in: accountAssetViewModel.account,
                  network: network
                )
              }
            }
          }
          return [AccountBalance(accountAssetViewModel.account, tokenBalance)]
        }
      }
      return await group.reduce([AccountBalance](), { $0 + $1 })
    }
    for tokenBalance in tokenBalances {
      if let index = accountAssetViewModels.firstIndex(where: {
        $0.account.id == tokenBalance.account.id
      }) {
        accountAssetViewModels[index].decimalBalance = tokenBalance.balance ?? 0.0
        accountAssetViewModels[index].balance = String(format: "%.4f", tokenBalance.balance ?? 0.0)
        accountAssetViewModels[index].fiatBalance =
          self.currencyFormatter.formatAsFiat(
            accountAssetViewModels[index].decimalBalance * price
          ) ?? ""
      }
    }
    self.isLoadingAccountBalances = false
    return accountAssetViewModels.filter { $0.decimalBalance > 0 }
  }

  private func buildTransactionSections(
    transactions: [BraveWallet.TransactionInfo],
    allNetworks: [BraveWallet.NetworkInfo],
    accountInfos: [BraveWallet.AccountInfo],
    userAssets: [BraveWallet.BlockchainToken],
    allTokens: [BraveWallet.BlockchainToken],
    assetRatios: [BraveWallet.AssetPrice],
    nftMetadata: [String: BraveWallet.NftMetadata],
    solEstimatedTxFees: [String: UInt64]
  ) -> [TransactionSection] {
    // Group transactions by day (only compare day/month/year)
    let transactionsGroupedByDate = Dictionary(grouping: transactions) { transaction in
      let dateComponents = Calendar.current.dateComponents(
        [.year, .month, .day],
        from: transaction.createdTime
      )
      return Calendar.current.date(from: dateComponents) ?? transaction.createdTime
    }
    // Map to 1 `TransactionSection` per date
    return transactionsGroupedByDate.keys.sorted(by: { $0 > $1 }).compactMap { date in
      let transactions = transactionsGroupedByDate[date] ?? []
      guard !transactions.isEmpty else { return nil }
      let parsedTransactions: [ParsedTransaction] =
        transactions
        .sorted(by: { $0.createdTime > $1.createdTime })
        .compactMap { transaction in
          return TransactionParser.parseTransaction(
            transaction: transaction,
            allNetworks: allNetworks,
            accountInfos: accountInfos,
            userAssets: userAssets,
            allTokens: allTokens + tokenInfoCache,
            assetRatios: assetRatios,
            nftMetadata: nftMetadata,
            solEstimatedTxFee: solEstimatedTxFees[transaction.id],
            currencyFormatter: currencyFormatter,
            decimalFormatStyle: .decimals(precision: 4)
          )
        }
      return TransactionSection(
        date: date,
        transactions: parsedTransactions
      )
    }
  }

  @MainActor private func updateSolEstimatedTxFeesCache(
    _ solTransactions: [BraveWallet.TransactionInfo]
  ) async {
    let fees = await solTxManagerProxy.solanaTxFeeEstimations(for: solTransactions)
    for (key, value) in fees {  // update cached values
      self.solEstimatedTxFeesCache[key] = value
    }
  }

  @MainActor private func updateAssetPricesCache(for tokens: [BraveWallet.BlockchainToken]) async {
    let prices = await assetRatioService.fetchPrices(
      for: tokens,
      vsCurrency: currencyFormatter.currencyCode
    )
    self.assetPricesCache.update(with: prices)
  }

  private var transactionDetailsStore: TransactionDetailsStore?
  func transactionDetailsStore(
    for transaction: BraveWallet.TransactionInfo
  ) -> TransactionDetailsStore {
    let transactionDetailsStore = TransactionDetailsStore(
      transaction: transaction,
      parsedTransaction: nil,
      keyringService: keyringService,
      walletService: walletService,
      rpcService: rpcService,
      assetRatioService: assetRatioService,
      blockchainRegistry: blockchainRegistry,
      txService: txService,
      solanaTxManagerProxy: solTxManagerProxy,
      ipfsApi: ipfsApi,
      userAssetManager: assetManager
    )
    self.transactionDetailsStore = transactionDetailsStore
    return transactionDetailsStore
  }

  func closeTransactionDetailsStore() {
    self.transactionDetailsStore?.tearDown()
    self.transactionDetailsStore = nil
  }

  @MainActor func accountCreationNeededForBuy() async -> Bool {
    guard let meldCryptoCurrency else {
      return false
    }
    return await !keyringService.isAccountAvailable(for: meldCryptoCurrency.coin)
  }

  /// Should be called after dismissing create account. Returns true if an account was created
  @MainActor func handleDismissAddAccount() async -> Bool {
    if await keyringService.isAccountAvailable(
      for: assetDetailToken.coin,
      chainId: assetDetailToken.chainId
    ) {
      self.update()
      return true
    } else {
      return false
    }
  }

  private func updateUnknownTokens(
    for contractAddressesChainIdPairs: [ContractAddressChainIdPair]
  ) {
    guard !contractAddressesChainIdPairs.isEmpty else { return }
    Task { @MainActor in
      // Gather known information about the transaction(s) tokens
      let unknownTokens: [BraveWallet.BlockchainToken] = await rpcService.fetchEthTokens(
        for: contractAddressesChainIdPairs
      )
      guard !unknownTokens.isEmpty else { return }
      tokenInfoCache.append(contentsOf: unknownTokens)
      update()
    }
  }
}

extension AssetDetailStore: BraveWalletKeyringServiceObserver {
  func walletReset() {
  }

  func accountsChanged() {
    update()
  }

  func walletCreated() {
  }

  func walletRestored() {
  }

  func locked() {
  }

  func unlocked() {
  }

  func backedUp() {
  }

  func autoLockMinutesChanged() {
  }

  func selectedWalletAccountChanged(account: BraveWallet.AccountInfo) {
  }

  func selectedDappAccountChanged(coin: BraveWallet.CoinType, account: BraveWallet.AccountInfo?) {
  }

  func accountsAdded(addedAccounts: [BraveWallet.AccountInfo]) {
  }
}

extension AssetDetailStore: BraveWalletTxServiceObserver {
  func onNewUnapprovedTx(txInfo: BraveWallet.TransactionInfo) {
  }
  func onUnapprovedTxUpdated(txInfo: BraveWallet.TransactionInfo) {
  }
  func onTransactionStatusChanged(txInfo: BraveWallet.TransactionInfo) {
    update()
  }
  func onTxServiceReset() {
  }
}

extension AssetDetailStore: BraveWalletBraveWalletServiceObserver {
  public func onActiveOriginChanged(originInfo: BraveWallet.OriginInfo) {
  }

  public func onDefaultWalletChanged(_ wallet: BraveWallet.DefaultWallet) {
  }

  public func onDefaultBaseCurrencyChanged(currency: String) {
    currencyCode = currency
  }

  public func onDefaultBaseCryptocurrencyChanged(cryptocurrency: String) {
  }

  public func onNetworkListChanged() {
  }

  func onDefaultEthereumWalletChanged(wallet: BraveWallet.DefaultWallet) {
  }

  func onDefaultSolanaWalletChanged(wallet: BraveWallet.DefaultWallet) {
  }

  func onDefaultCardanoWalletChanged(wallet: BraveWallet.DefaultWallet) {
  }

  func onDiscoverAssetsStarted() {
  }

  func onDiscoverAssetsCompleted(discoveredAssets: [BraveWallet.BlockchainToken]) {
  }

  func onResetWallet() {
  }
}

extension AssetDetailStore: WalletUserAssetDataObserver {
  func cachedBalanceRefreshed() {
    update()
  }

  func userAssetUpdated() {
  }
}
