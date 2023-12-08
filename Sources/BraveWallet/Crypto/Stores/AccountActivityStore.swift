// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

class AccountActivityStore: ObservableObject, WalletObserverStore {
  /// If we want to observe selected account changes (ex. in `WalletPanelView`).
  /// In some cases, we do not want to update the account displayed when the
  /// selected account changes (ex. when removing an account).
  let observeAccountUpdates: Bool
  private(set) var account: BraveWallet.AccountInfo
  @Published private(set) var userAssets: [AssetViewModel] = []
  @Published private(set) var userNFTs: [NFTAssetViewModel] = []
  @Published var transactionSummaries: [TransactionSummary] = []
  @Published private(set) var currencyCode: String = CurrencyCode.usd.code {
    didSet {
      currencyFormatter.currencyCode = currencyCode
      guard oldValue != currencyCode else { return }
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
  private let ipfsApi: IpfsAPI
  private let assetManager: WalletUserAssetManagerType
  /// Cache for storing `BlockchainToken`s that are not in user assets or our token registry.
  /// This could occur with a dapp creating a transaction.
  private var tokenInfoCache: [BraveWallet.BlockchainToken] = []
  
  private var keyringServiceObserver: KeyringServiceObserver?
  private var rpcServiceObserver: JsonRpcServiceObserver?
  private var txServiceObserver: TxServiceObserver?
  private var walletServiceObserver: WalletServiceObserver?
  
  var isObserving: Bool {
    keyringServiceObserver != nil && rpcServiceObserver != nil && txServiceObserver != nil && walletServiceObserver != nil
  }
  
  init(
    account: BraveWallet.AccountInfo,
    observeAccountUpdates: Bool,
    keyringService: BraveWalletKeyringService,
    walletService: BraveWalletBraveWalletService,
    rpcService: BraveWalletJsonRpcService,
    assetRatioService: BraveWalletAssetRatioService,
    txService: BraveWalletTxService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    solTxManagerProxy: BraveWalletSolanaTxManagerProxy,
    ipfsApi: IpfsAPI,
    userAssetManager: WalletUserAssetManagerType
  ) {
    self.account = account
    self.observeAccountUpdates = observeAccountUpdates
    self.keyringService = keyringService
    self.walletService = walletService
    self.rpcService = rpcService
    self.assetRatioService = assetRatioService
    self.txService = txService
    self.blockchainRegistry = blockchainRegistry
    self.solTxManagerProxy = solTxManagerProxy
    self.ipfsApi = ipfsApi
    self.assetManager = userAssetManager
    
    self.setupObservers()
    
    walletService.defaultBaseCurrency { [self] currencyCode in
      self.currencyCode = currencyCode
    }
  }
  
  func tearDown() {
    keyringServiceObserver = nil
    rpcServiceObserver = nil
    txServiceObserver = nil
    walletServiceObserver = nil
    transactionDetailsStore?.tearDown()
  }
  
  func setupObservers() {
    guard !isObserving else { return }
    self.keyringServiceObserver = KeyringServiceObserver(
      keyringService: keyringService,
      _selectedWalletAccountChanged: { [weak self] account in
        guard let self, self.observeAccountUpdates else { return }
        self.account = account
        self.update()
      },
      _selectedDappAccountChanged: { [weak self] _, account in
        guard let self, self.observeAccountUpdates, let account else { return }
        self.account = account
        self.update()
      }
    )
    self.rpcServiceObserver = JsonRpcServiceObserver(
      rpcService: rpcService,
      _chainChangedEvent: { [weak self] _, _, _ in
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
          // Handle small gap between chain changing and txController having the correct chain Id
          self?.update()
        }
      }
    )
    self.txServiceObserver = TxServiceObserver(
      txService: txService,
      _onNewUnapprovedTx: { [weak self] _ in
        self?.update()
      },
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

  func update() {
    Task { @MainActor in
      let coin = account.coin
      let networksForAccountCoin = await rpcService.allNetworks(coin)
        .filter { $0.chainId != BraveWallet.LocalhostChainId } // localhost not supported
      let networksForAccount = networksForAccountCoin.filter { // .fil coin type has two different keyring ids
        $0.supportedKeyrings.contains(account.keyringId.rawValue as NSNumber)
      }
      
      struct NetworkAssets: Equatable {
        let network: BraveWallet.NetworkInfo
        let tokens: [BraveWallet.BlockchainToken]
        let sortOrder: Int
      }
      let allUserAssets = assetManager.getAllUserAssetsInNetworkAssets(networks: networksForAccount, includingUserDeleted: true)
      let allTokens = await blockchainRegistry.allTokens(in: networksForAccountCoin).flatMap(\.tokens)
      var updatedUserAssets: [AssetViewModel] = []
      var updatedUserNFTs: [NFTAssetViewModel] = []
      for networkAssets in allUserAssets {
        for token in networkAssets.tokens {
          if token.isErc721 || token.isNft {
            updatedUserNFTs.append(
              NFTAssetViewModel(
                groupType: .none,
                token: token,
                network: networkAssets.network,
                balanceForAccounts: [:]
              )
            )
          } else {
            updatedUserAssets.append(
              AssetViewModel(
                groupType: .none,
                token: token,
                network: networkAssets.network,
                price: "",
                history: [],
                balanceForAccounts: [:]
              )
            )
          }
        }
      }
      self.userAssets = updatedUserAssets
      self.userNFTs = updatedUserNFTs
      
      typealias TokenNetworkAccounts = (token: BraveWallet.BlockchainToken, network: BraveWallet.NetworkInfo, accounts: [BraveWallet.AccountInfo])
      let allTokenNetworkAccounts = allUserAssets.flatMap { networkAssets in
        networkAssets.tokens.map { token in
          TokenNetworkAccounts(
            token: token,
            network: networkAssets.network,
            accounts: [account]
          )
        }
      }
      let totalBalances: [String: Double] = await withTaskGroup(of: [String: Double].self, body: { @MainActor group in
        for tokenNetworkAccounts in allTokenNetworkAccounts {
          group.addTask { @MainActor in
            let totalBalance = await self.rpcService.fetchTotalBalance(
              token: tokenNetworkAccounts.token,
              network: tokenNetworkAccounts.network,
              accounts: tokenNetworkAccounts.accounts
            )
            return [tokenNetworkAccounts.token.assetBalanceId: totalBalance]
          }
        }
        return await group.reduce(into: [String: Double](), { partialResult, new in
          for key in new.keys {
            partialResult[key] = new[key]
          }
        })
      })
      
      // fetch price for every user asset
      let allUserAssetsInToken = allUserAssets.flatMap(\.tokens)
      let allUserAssetsAssetRatioIds = allUserAssetsInToken.map(\.assetRatioId)
      let prices: [String: String] = await assetRatioService.fetchPrices(
        for: allUserAssetsAssetRatioIds,
        toAssets: [currencyFormatter.currencyCode],
        timeframe: .oneDay
      )
      
      // fetch NFTs metadata
      let allNFTMetadata = await rpcService.fetchNFTMetadata(
        tokens: userNFTs
          .map(\.token)
          .filter({ $0.isErc721 || $0.isNft }),
        ipfsApi: ipfsApi
      )
      
      guard !Task.isCancelled else { return }
      updatedUserAssets.removeAll()
      updatedUserNFTs.removeAll()
      for networkAssets in allUserAssets {
        for token in networkAssets.tokens {
          if token.isErc721 || token.isNft {
            updatedUserNFTs.append(
              NFTAssetViewModel(
                groupType: .none,
                token: token,
                network: networkAssets.network,
                balanceForAccounts: [account.address: Int(totalBalances[token.assetBalanceId] ?? 0)],
                nftMetadata: allNFTMetadata[token.id]
              )
            )
          } else {
            updatedUserAssets.append(
              AssetViewModel(
                groupType: .none,
                token: token,
                network: networkAssets.network,
                price: prices[token.assetRatioId.lowercased()] ?? "",
                history: [],
                balanceForAccounts: [account.address: totalBalances[token.assetBalanceId] ?? 0]
              )
            )
          }
        }
      }
      self.userAssets = updatedUserAssets
      self.userNFTs = updatedUserNFTs
      
      let assetRatios = self.userAssets.reduce(into: [String: Double](), {
        $0[$1.token.assetRatioId.lowercased()] = Double($1.price)
      })
      
      let allAccountsForCoin = await keyringService.allAccounts().accounts.filter { $0.coin == account.coin }
      self.transactionSummaries = await fetchTransactionSummarys(
        networksForAccountCoin: networksForAccountCoin,
        accountInfos: allAccountsForCoin,
        userAssets: userAssets.map(\.token),
        allTokens: allTokens,
        assetRatios: assetRatios
      )
    }
  }
  
  @MainActor private func fetchTransactionSummarys(
    networksForAccountCoin: [BraveWallet.NetworkInfo],
    accountInfos: [BraveWallet.AccountInfo],
    userAssets: [BraveWallet.BlockchainToken],
    allTokens: [BraveWallet.BlockchainToken],
    assetRatios: [String: Double]
  ) async -> [TransactionSummary] {
    let transactions = await txService.allTransactions(networks: networksForAccountCoin, for: account)
    var solEstimatedTxFees: [String: UInt64] = [:]
    if account.coin == .sol {
      solEstimatedTxFees = await solTxManagerProxy.estimatedTxFees(for: transactions)
    }
    let ethTransactions = transactions.filter { $0.coin == .eth }
    if !ethTransactions.isEmpty {
      // Gather known information about the transaction(s) tokens
      let unknownTokenInfo = ethTransactions.unknownTokenContractAddressChainIdPairs(
        knownTokens: userAssets + allTokens + tokenInfoCache
      )
      if !unknownTokenInfo.isEmpty {
        let unknownTokens: [BraveWallet.BlockchainToken] = await rpcService.fetchEthTokens(for: unknownTokenInfo)
        tokenInfoCache.append(contentsOf: unknownTokens)
      }
    }
    return transactions
      .compactMap { transaction in
        guard let network = networksForAccountCoin.first(where: { $0.chainId == transaction.chainId }) else {
          return nil
        }
        return TransactionParser.transactionSummary(
          from: transaction,
          network: network,
          accountInfos: accountInfos,
          userAssets: userAssets,
          allTokens: allTokens + tokenInfoCache,
          assetRatios: assetRatios,
          nftMetadata: [:],
          solEstimatedTxFee: solEstimatedTxFees[transaction.id],
          currencyFormatter: currencyFormatter
        )
      }.sorted(by: { $0.createdTime > $1.createdTime })
  }
  
  private var transactionDetailsStore: TransactionDetailsStore?
  func transactionDetailsStore(for transaction: BraveWallet.TransactionInfo) -> TransactionDetailsStore {
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
  
  #if DEBUG
  func previewTransactions() {
    transactionSummaries = [.previewConfirmedSwap, .previewConfirmedSend, .previewConfirmedERC20Approve]
  }
  #endif
}

extension AccountActivityStore: BraveWalletKeyringServiceObserver {
  func walletCreated() {
  }

  func walletRestored() {
  }
  
  func walletReset() {
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
  
  func selectedWalletAccountChanged(_ account: BraveWallet.AccountInfo) {
    guard observeAccountUpdates else { return }
    self.account = account
    update()
  }
  
  func selectedDappAccountChanged(_ coin: BraveWallet.CoinType, account: BraveWallet.AccountInfo?) {
    guard observeAccountUpdates, let account else { return }
    self.account = account
    update()
  }
  
  func accountsAdded(_ addedAccounts: [BraveWallet.AccountInfo]) {
  }
}

extension AccountActivityStore: BraveWalletJsonRpcServiceObserver {
  func chainChangedEvent(_ chainId: String, coin: BraveWallet.CoinType, origin: URLOrigin?) {
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
  func onTxServiceReset() {
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
  
  public func onDiscoverAssetsStarted() {
  }
  
  func onDiscoverAssetsCompleted(_ discoveredAssets: [BraveWallet.BlockchainToken]) {
  }
  
  func onResetWallet() {
  }
}
