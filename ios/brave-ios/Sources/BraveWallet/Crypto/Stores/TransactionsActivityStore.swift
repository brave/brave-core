// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Preferences
import SwiftUI

class TransactionsActivityStore: ObservableObject, WalletObserverStore {
  /// Sections of transactions for display. Each section represents one date.
  @Published var transactionSections: [TransactionSection] = []
  /// Filter query to filter the transactions by.
  @Published var query: String = ""
  @Published var errorMessage: String?
  /// Selected networks to show transactions for.
  @Published var networkFilters: [Selectable<BraveWallet.NetworkInfo>] = [] {
    didSet {
      guard !oldValue.isEmpty else { return }  // initial assignment to `networkFilters`
      update()
    }
  }

  @Published private(set) var currencyCode: String = CurrencyCode.usd.code {
    didSet {
      currencyFormatter.currencyCode = currencyCode
      guard oldValue != currencyCode else { return }
      update()
    }
  }

  let currencyFormatter: NumberFormatter = .usdCurrencyFormatter

  private var solEstimatedTxFeesCache: [String: UInt64] = [:]
  private var assetPricesCache: [String: Double] = [:]
  /// Cache of metadata for NFTs. The key is the token's `id`.
  private var metadataCache: [String: NFTMetadata] = [:]
  /// Cache for storing `BlockchainToken`s that are not in user assets or our token registry.
  /// This could occur with a dapp creating a transaction.
  private var tokenInfoCache: [BraveWallet.BlockchainToken] = []

  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let assetRatioService: BraveWalletAssetRatioService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let txService: BraveWalletTxService
  private let solTxManagerProxy: BraveWalletSolanaTxManagerProxy
  private let ipfsApi: IpfsAPI
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
    ipfsApi: IpfsAPI,
    userAssetManager: WalletUserAssetManagerType
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.assetRatioService = assetRatioService
    self.blockchainRegistry = blockchainRegistry
    self.txService = txService
    self.solTxManagerProxy = solTxManagerProxy
    self.ipfsApi = ipfsApi
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
    transactionDetailsStore?.tearDown()
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
            let existingSelectionValue = self?.networkFilters.first(where: {
              $0.model.chainId == network.chainId
            })?.isSelected
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
      let allAccounts = await keyringService.allAccounts()
      let allAccountInfos = allAccounts.accounts
      // setup network filters if not currently setup
      let allNetworks = await self.rpcService.allNetworksForSupportedCoins()
      if self.networkFilters.isEmpty {
        self.networkFilters = allNetworks.map {
          .init(isSelected: true, model: $0)
        }
      }
      let networks = networkFilters.filter(\.isSelected).map(\.model)
      let networksForCoin: [BraveWallet.CoinType: [BraveWallet.NetworkInfo]] = Dictionary(
        grouping: networks,
        by: \.coin
      )
      let allNetworksAllCoins = networksForCoin.values.flatMap { $0 }

      let allTransactions = await txService.allTransactions(
        networksForCoin: networksForCoin,
        for: allAccountInfos
      ).filter { $0.txStatus != .rejected }
      let userAssets = await assetManager.getAllUserAssetsInNetworkAssets(
        networks: allNetworksAllCoins,
        includingUserDeleted: true
      ).flatMap(\.tokens)
      let allTokens = await blockchainRegistry.allTokens(
        in: allNetworksAllCoins
      ).flatMap(\.tokens)
      let ethTransactions = allTransactions.filter { $0.coin == .eth }
      if !ethTransactions.isEmpty {  // we can only fetch unknown Ethereum tokens
        let unknownTokenInfo = ethTransactions.unknownTokenContractAddressChainIdPairs(
          knownTokens: userAssets + allTokens + tokenInfoCache
        )
        updateUnknownTokens(for: unknownTokenInfo)
      }
      guard !Task.isCancelled else { return }
      // display transactions prior to network request to fetch
      // estimated solana tx fees & asset prices
      self.transactionSections = buildTransactionSections(
        transactions: allTransactions,
        networksForCoin: networksForCoin,
        allNetworks: allNetworks,
        accountInfos: allAccountInfos,
        userAssets: userAssets,
        allTokens: allTokens + tokenInfoCache,
        assetRatios: assetPricesCache,
        nftMetadata: metadataCache,
        solEstimatedTxFees: solEstimatedTxFeesCache
      )
      guard !self.transactionSections.isEmpty else { return }

      if allTransactions.contains(where: { $0.coin == .sol }) {
        let solTransactions = allTransactions.filter { $0.coin == .sol }
        await updateSolEstimatedTxFeesCache(solTransactions)
      }

      let allUserAssetsAssetRatioIds = userAssets.map(\.assetRatioId)
      await updateAssetPricesCache(assetRatioIds: allUserAssetsAssetRatioIds)

      guard !Task.isCancelled else { return }
      self.transactionSections = buildTransactionSections(
        transactions: allTransactions,
        networksForCoin: networksForCoin,
        allNetworks: allNetworks,
        accountInfos: allAccountInfos,
        userAssets: userAssets,
        allTokens: allTokens,
        assetRatios: assetPricesCache,
        nftMetadata: metadataCache,
        solEstimatedTxFees: solEstimatedTxFeesCache
      )

      let nftsWithoutMetadata = transactionSections.flatMap(\.transactions)
        .compactMap { parsedTx in
          switch parsedTx.details {
          case .erc721Transfer(let details):
            return details.fromToken
          case .solSplTokenTransfer(let details):
            if let fromToken = details.fromToken, fromToken.isNft {
              return fromToken
            }
            return nil
          default:
            return nil
          }
        }
        .filter { token in  // filter out already fetched metadata
          !metadataCache.keys.contains(where: {
            $0.caseInsensitiveCompare(token.contractAddress) == .orderedSame
          })
        }
      guard !Task.isCancelled, !nftsWithoutMetadata.isEmpty else { return }
      // fetch nft metadata for all NFTs
      let allMetadata = await rpcService.fetchNFTMetadata(
        tokens: nftsWithoutMetadata,
        ipfsApi: ipfsApi
      )
      for (key, value) in allMetadata {  // update cached values
        metadataCache[key] = value
      }
      guard !Task.isCancelled else { return }
      self.transactionSections = buildTransactionSections(
        transactions: allTransactions,
        networksForCoin: networksForCoin,
        allNetworks: allNetworks,
        accountInfos: allAccountInfos,
        userAssets: userAssets,
        allTokens: allTokens,
        assetRatios: assetPricesCache,
        nftMetadata: metadataCache,
        solEstimatedTxFees: solEstimatedTxFeesCache
      )
    }
  }

  func handleTransactionFollowUpAction(
    _ action: TransactionFollowUpAction,
    transaction: BraveWallet.TransactionInfo
  ) {
    Task { @MainActor in
      guard
        let errorMessage = await txService.handleTransactionFollowUpAction(
          action,
          transaction: transaction
        )
      else {
        return
      }
      self.errorMessage = errorMessage
    }
  }

  private func buildTransactionSections(
    transactions: [BraveWallet.TransactionInfo],
    networksForCoin: [BraveWallet.CoinType: [BraveWallet.NetworkInfo]],
    allNetworks: [BraveWallet.NetworkInfo],
    accountInfos: [BraveWallet.AccountInfo],
    userAssets: [BraveWallet.BlockchainToken],
    allTokens: [BraveWallet.BlockchainToken],
    assetRatios: [String: Double],
    nftMetadata: [String: NFTMetadata],
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

  @MainActor private func updateAssetPricesCache(assetRatioIds: [String]) async {
    let prices = await assetRatioService.fetchPrices(
      for: assetRatioIds,
      toAssets: [currencyFormatter.currencyCode],
      timeframe: .oneDay
    ).compactMapValues { Double($0) }
    for (key, value) in prices {  // update cached values
      self.assetPricesCache[key] = value
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

  private var transactionDetailsStore: TransactionDetailsStore?
  func transactionDetailsStore(
    for transaction: BraveWallet.TransactionInfo
  ) -> TransactionDetailsStore {
    let parsedTransaction =
      transactionSections
      .flatMap(\.transactions)
      .first(where: { $0.transaction.id == transaction.id })
    let transactionDetailsStore = TransactionDetailsStore(
      transaction: transaction,
      parsedTransaction: parsedTransaction,
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
}
