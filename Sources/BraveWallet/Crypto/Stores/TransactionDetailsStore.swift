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
  
  @Published private(set) var currencyCode: String = CurrencyCode.usd.code {
    didSet {
      currencyFormatter.currencyCode = currencyCode
      guard currencyCode != oldValue else { return }
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
  private let txService: BraveWalletTxService
  private let solanaTxManagerProxy: BraveWalletSolanaTxManagerProxy
  private let ipfsApi: IpfsAPI
  private let assetManager: WalletUserAssetManagerType
  /// Cache for storing `BlockchainToken`s that are not in user assets or our token registry.
  /// This could occur with a dapp creating a transaction.
  private var tokenInfoCache: [BraveWallet.BlockchainToken] = []
  private var nftMetadataCache: [String: NFTMetadata] = [:]
  
  var isObserving: Bool {
    txServiceObserver != nil
  }

  private var txServiceObserver: TxServiceObserver?
  
  init(
    transaction: BraveWallet.TransactionInfo,
    parsedTransaction: ParsedTransaction?,
    keyringService: BraveWalletKeyringService,
    walletService: BraveWalletBraveWalletService,
    rpcService: BraveWalletJsonRpcService,
    assetRatioService: BraveWalletAssetRatioService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    txService: BraveWalletTxService,
    solanaTxManagerProxy: BraveWalletSolanaTxManagerProxy,
    ipfsApi: IpfsAPI,
    userAssetManager: WalletUserAssetManagerType
  ) {
    self.transaction = transaction
    self.parsedTransaction = parsedTransaction
    self.keyringService = keyringService
    self.walletService = walletService
    self.rpcService = rpcService
    self.assetRatioService = assetRatioService
    self.blockchainRegistry = blockchainRegistry
    self.txService = txService
    self.solanaTxManagerProxy = solanaTxManagerProxy
    self.ipfsApi = ipfsApi
    self.assetManager = userAssetManager
    
    setupObservers()
    
    walletService.defaultBaseCurrency { [self] currencyCode in
      self.currencyCode = currencyCode
    }
  }
  
  func setupObservers() {
    guard !isObserving else { return }
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
  }
  
  func tearDown() {
    txServiceObserver = nil
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
      var allTokens: [BraveWallet.BlockchainToken] = await blockchainRegistry.allTokens(network.chainId, coin: network.coin)
      let userAssets: [BraveWallet.BlockchainToken] = assetManager.getAllUserAssetsInNetworkAssets(networks: [network], includingUserDeleted: true).flatMap(\.tokens)
      if coin == .eth {
        // Gather known information about the transaction(s) tokens
        let unknownTokenInfo = [transaction].unknownTokenContractAddressChainIdPairs(
          knownTokens: userAssets + allTokens + tokenInfoCache
        )
        if !unknownTokenInfo.isEmpty {
          let unknownTokens: [BraveWallet.BlockchainToken] = await rpcService.fetchEthTokens(for: unknownTokenInfo)
          tokenInfoCache.append(contentsOf: unknownTokens)
        }
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
        allTokens: allTokens + tokenInfoCache,
        assetRatios: assetRatios,
        nftMetadata: nftMetadataCache,
        solEstimatedTxFee: solEstimatedTxFee,
        currencyFormatter: currencyFormatter
      ) else {
        return
      }
      self.parsedTransaction = parsedTransaction

      // Fetch NFTMetadata if needed.
      let nftToken: BraveWallet.BlockchainToken?
      switch parsedTransaction.details {
      case .erc721Transfer(let details):
        if details.nftMetadata == nil {
          nftToken = details.fromToken
        } else {
          nftToken = nil
        }
      case .solSplTokenTransfer(let details):
        if let fromToken = details.fromToken,
           fromToken.isNft,
           details.fromTokenMetadata == nil {
          nftToken = fromToken
        } else {
          nftToken = nil
        }
      default:
        nftToken = nil
      }
      guard let nftToken else { return }
      self.nftMetadataCache[nftToken.id] = await rpcService.fetchNFTMetadata(for: nftToken, ipfsApi: ipfsApi)
      guard let parsedTransaction = transaction.parsedTransaction(
        network: network,
        accountInfos: allAccounts,
        userAssets: userAssets,
        allTokens: allTokens,
        assetRatios: assetRatios,
        nftMetadata: nftMetadataCache,
        solEstimatedTxFee: solEstimatedTxFee,
        currencyFormatter: currencyFormatter
      ) else {
        return
      }
      self.parsedTransaction = parsedTransaction
    }
  }
}
