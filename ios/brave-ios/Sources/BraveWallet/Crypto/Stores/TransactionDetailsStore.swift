// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

class TransactionDetailsStore: ObservableObject, WalletObserverStore {

  @Published private(set) var transaction: BraveWallet.TransactionInfo
  @Published private(set) var parsedTransaction: ParsedTransaction?
  @Published private(set) var network: BraveWallet.NetworkInfo?
  @Published private(set) var isLoadingTransactionAction: Bool = false
  @Published private(set) var isCancelOrSpeedupAvailable: Bool = false

  @Published private(set) var currencyCode: String = CurrencyCode.usd.code {
    didSet {
      currencyFormatter.currencyCode = currencyCode
      guard currencyCode != oldValue else { return }
      // prices are fetched against currency code
      assetRatiosCache.removeAll()
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
  private var nftMetadataCache: [String: BraveWallet.NftMetadata] = [:]
  private var solEstimatedTxFeesCache: [String: UInt64] = [:]
  private var assetRatiosCache: [String: Double] = [:]

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
      _onNewUnapprovedTx: { [weak self] transaction in
        self?.updateTransaction(transaction)
      },
      _onUnapprovedTxUpdated: { [weak self] transaction in
        self?.updateTransaction(transaction)
      },
      _onTransactionStatusChanged: { [weak self] transaction in
        self?.updateTransaction(transaction)
      },
      _onTxServiceReset: { [weak self] in
        self?.update()
      }
    )
  }

  func tearDown() {
    txServiceObserver = nil
  }

  func updateTransaction(_ transaction: BraveWallet.TransactionInfo) {
    guard transaction.id == self.transaction.id else {
      // not the transaction currently open
      return
    }
    self.transaction = transaction
    self.update()
  }

  func update() {
    Task { @MainActor in
      let allNetworks = await rpcService.allNetworks()
      guard let network = allNetworks.first(where: { $0.chainId == transaction.chainId }) else {
        // Transactions should be removed if their network is removed
        // https://github.com/brave/brave-browser/issues/30234
        assertionFailure(
          "The NetworkInfo for the transaction's chainId (\(transaction.chainId)) is unavailable"
        )
        return
      }
      self.network = network
      let allTokens: [BraveWallet.BlockchainToken] = await blockchainRegistry.allTokens(
        chainId: network.chainId,
        coin: network.coin
      )
      let userAssets: [BraveWallet.BlockchainToken] =
        await assetManager.getAllUserAssetsInNetworkAssets(
          networks: [network],
          includingUserDeleted: true
        ).flatMap(\.tokens)
      if transaction.coin == .eth {
        // Gather known information about the transaction(s) tokens
        let unknownTokenInfo = [transaction].unknownTokenContractAddressChainIdPairs(
          knownTokens: userAssets + allTokens + tokenInfoCache
        )
        if !unknownTokenInfo.isEmpty {
          let unknownTokens: [BraveWallet.BlockchainToken] = await rpcService.fetchEthTokens(
            for: unknownTokenInfo
          )
          tokenInfoCache.append(contentsOf: unknownTokens)
        }
      }

      if transaction.isCancelOrSpeedUpTransactionSupported {
        self.isCancelOrSpeedupAvailable = true
      }

      let allAccounts = await keyringService.allAccounts().accounts
      guard
        let parsedTransaction = transaction.parsedTransaction(
          allNetworks: allNetworks,
          accountInfos: allAccounts,
          userAssets: userAssets,
          allTokens: allTokens + tokenInfoCache,
          assetRatios: assetRatiosCache,
          nftMetadata: nftMetadataCache,
          solEstimatedTxFee: solEstimatedTxFeesCache[transaction.id],
          currencyFormatter: currencyFormatter
        )
      else {
        return
      }
      self.parsedTransaction = parsedTransaction

      let txTokensWithoutPrice = (parsedTransaction.tokens + [network.nativeToken])
        .filter { self.assetRatiosCache[$0.assetRatioId] == nil }
      if !txTokensWithoutPrice.isEmpty {
        let priceResult = await assetRatioService.priceWithIndividualRetry(
          userAssets.map { $0.assetRatioId.lowercased() },
          toAssets: [currencyFormatter.currencyCode],
          timeframe: .oneDay
        )
        let assetRatios = priceResult.assetPrices.reduce(into: [String: Double]()) {
          $0[$1.fromAsset] = Double($1.price)
        }
        self.assetRatiosCache.merge(with: assetRatios)
      }

      if transaction.coin == .sol, solEstimatedTxFeesCache[transaction.id] == nil {
        let txFees = await solanaTxManagerProxy.solanaTxFeeEstimations(for: [transaction])
        solEstimatedTxFeesCache.merge(with: txFees)
      }
      guard
        let parsedTransaction = transaction.parsedTransaction(
          allNetworks: allNetworks,
          accountInfos: allAccounts,
          userAssets: userAssets,
          allTokens: allTokens + tokenInfoCache,
          assetRatios: assetRatiosCache,
          nftMetadata: nftMetadataCache,
          solEstimatedTxFee: solEstimatedTxFeesCache[transaction.id],
          currencyFormatter: currencyFormatter
        )
      else {
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
          details.fromTokenMetadata == nil
        {
          nftToken = fromToken
        } else {
          nftToken = nil
        }
      default:
        nftToken = nil
      }
      guard let nftToken,
        nftMetadataCache[nftToken.id] == nil
      else { return }
      self.nftMetadataCache[nftToken.id] = await rpcService.fetchNFTMetadata(
        for: nftToken,
        ipfsApi: ipfsApi
      )
      guard
        let parsedTransaction = transaction.parsedTransaction(
          allNetworks: allNetworks,
          accountInfos: allAccounts,
          userAssets: userAssets,
          allTokens: allTokens,
          assetRatios: assetRatiosCache,
          nftMetadata: nftMetadataCache,
          solEstimatedTxFee: solEstimatedTxFeesCache[transaction.id],
          currencyFormatter: currencyFormatter
        )
      else {
        return
      }
      self.parsedTransaction = parsedTransaction
    }
  }

  @MainActor func handleTransactionFollowUpAction(
    _ action: TransactionFollowUpAction
  ) async -> String? {
    guard !isLoadingTransactionAction else {
      return nil
    }
    self.isLoadingTransactionAction = true
    defer { self.isLoadingTransactionAction = false }
    guard
      let errorMessage = await txService.handleTransactionFollowUpAction(
        action,
        transaction: transaction
      )
    else {
      // success
      return nil
    }
    return errorMessage
  }
}
