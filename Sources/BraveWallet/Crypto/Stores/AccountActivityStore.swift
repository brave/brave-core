// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

class AccountActivityStore: ObservableObject {
  /// If we want to observe selected account changes (ex. in `WalletPanelView`).
  /// In some cases, we do not want to update the account displayed when the
  /// selected account changes (ex. when removing an account).
  let observeAccountUpdates: Bool
  private(set) var account: BraveWallet.AccountInfo
  @Published private(set) var userVisibleAssets: [AssetViewModel] = []
  @Published private(set) var userVisibleNFTs: [NFTAssetViewModel] = []
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
  /// Cache for storing `BlockchainToken`s that are not in user assets or our token registry.
  /// This could occur with a dapp creating a transaction.
  private var tokenInfoCache: [String: BraveWallet.BlockchainToken] = [:]

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
    ipfsApi: IpfsAPI
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
      let networksForAccountCoin = await rpcService.allNetworks(coin)
        .filter { $0.chainId != BraveWallet.LocalhostChainId } // localhost not supported
      
      struct NetworkAssets: Equatable {
        let network: BraveWallet.NetworkInfo
        let tokens: [BraveWallet.BlockchainToken]
        let sortOrder: Int
      }
      let allVisibleUserAssets = await walletService.allVisibleUserAssets(in: networksForAccountCoin)
      let allTokens = await blockchainRegistry.allTokens(in: networksForAccountCoin).flatMap(\.tokens)
      var updatedUserVisibleAssets: [AssetViewModel] = []
      var updatedUserVisibleNFTs: [NFTAssetViewModel] = []
      for networkAssets in allVisibleUserAssets {
        for token in networkAssets.tokens {
          if token.isErc721 || token.isNft {
            updatedUserVisibleNFTs.append(
              NFTAssetViewModel(
                token: token,
                network: networkAssets.network,
                balance: 0
              )
            )
          } else {
            updatedUserVisibleAssets.append(
              AssetViewModel(
                token: token,
                network: networkAssets.network,
                decimalBalance: 0,
                price: "",
                history: []
              )
            )
          }
        }
      }
      self.userVisibleAssets = updatedUserVisibleAssets
      self.userVisibleNFTs = updatedUserVisibleNFTs
      
      let keyringForAccount = await keyringService.keyringInfo(coin.keyringId)
      typealias TokenNetworkAccounts = (token: BraveWallet.BlockchainToken, network: BraveWallet.NetworkInfo, accounts: [BraveWallet.AccountInfo])
      let allTokenNetworkAccounts = allVisibleUserAssets.flatMap { networkAssets in
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
      
      // fetch price for every visible token
      let allVisibleTokens = allVisibleUserAssets.flatMap(\.tokens)
      let allVisibleTokenAssetRatioIds = allVisibleTokens.map(\.assetRatioId)
      let prices: [String: String] = await assetRatioService.fetchPrices(
        for: allVisibleTokenAssetRatioIds,
        toAssets: [currencyFormatter.currencyCode],
        timeframe: .oneDay
      )
      
      // fetch NFTs metadata
      let allNFTMetadata = await rpcService.fetchNFTMetadata(
        tokens: userVisibleNFTs
          .map(\.token)
          .filter({ $0.isErc721 || $0.isNft }),
        ipfsApi: ipfsApi
      )
      
      guard !Task.isCancelled else { return }
      updatedUserVisibleAssets.removeAll()
      updatedUserVisibleNFTs.removeAll()
      for networkAssets in allVisibleUserAssets {
        for token in networkAssets.tokens {
          if token.isErc721 || token.isNft {
            updatedUserVisibleNFTs.append(
              NFTAssetViewModel(
                token: token,
                network: networkAssets.network,
                balance: Int(totalBalances[token.assetBalanceId] ?? 0),
                nftMetadata: allNFTMetadata[token.id]
              )
            )
          } else {
            updatedUserVisibleAssets.append(
              AssetViewModel(
                token: token,
                network: networkAssets.network,
                decimalBalance: totalBalances[token.assetBalanceId] ?? 0,
                price: prices[token.assetRatioId.lowercased()] ?? "",
                history: []
              )
            )
          }
        }
      }
      self.userVisibleAssets = updatedUserVisibleAssets
      self.userVisibleNFTs = updatedUserVisibleNFTs
      
      let assetRatios = self.userVisibleAssets.reduce(into: [String: Double](), {
        $0[$1.token.assetRatioId.lowercased()] = Double($1.price)
      })
      
      self.transactionSummaries = await fetchTransactionSummarys(
        networksForAccountCoin: networksForAccountCoin,
        accountInfos: keyringForAccount.accountInfos,
        userVisibleTokens: userVisibleAssets.map(\.token),
        allTokens: allTokens,
        assetRatios: assetRatios
      )
    }
  }
  
  @MainActor private func fetchTransactionSummarys(
    networksForAccountCoin: [BraveWallet.NetworkInfo],
    accountInfos: [BraveWallet.AccountInfo],
    userVisibleTokens: [BraveWallet.BlockchainToken],
    allTokens: [BraveWallet.BlockchainToken],
    assetRatios: [String: Double]
  ) async -> [TransactionSummary] {
    let transactions = await txService.allTransactions(networks: networksForAccountCoin, for: account)
    var solEstimatedTxFees: [String: UInt64] = [:]
    if account.coin == .sol {
      solEstimatedTxFees = await solTxManagerProxy.estimatedTxFees(for: transactions)
    }
    let unknownTokenContractAddresses = transactions
      .flatMap { $0.tokenContractAddresses }
      .filter { contractAddress in
        !userVisibleTokens.contains(where: { $0.contractAddress.caseInsensitiveCompare(contractAddress) == .orderedSame })
        && !allTokens.contains(where: { $0.contractAddress.caseInsensitiveCompare(contractAddress) == .orderedSame })
        && !tokenInfoCache.keys.contains(where: { $0.caseInsensitiveCompare(contractAddress) == .orderedSame })
      }
    var allTokens = allTokens
    if !unknownTokenContractAddresses.isEmpty {
      let unknownTokens = await assetRatioService.fetchTokens(for: unknownTokenContractAddresses)
      for unknownToken in unknownTokens {
        tokenInfoCache[unknownToken.contractAddress] = unknownToken
      }
      allTokens.append(contentsOf: unknownTokens)
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
          visibleTokens: userVisibleTokens,
          allTokens: allTokens,
          assetRatios: assetRatios,
          solEstimatedTxFee: solEstimatedTxFees[transaction.id],
          currencyFormatter: currencyFormatter
        )
      }.sorted(by: { $0.createdTime > $1.createdTime })
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
    guard observeAccountUpdates else { return }
    keyringService.keyringInfo(coin.keyringId) { [self] keyringInfo in
      keyringService.selectedAccount(coin) { [self] accountAddress in
        account = keyringInfo.accountInfos.first(where: { $0.address == accountAddress }) ?? keyringInfo.accountInfos.first!
        update()
      }
    }
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
