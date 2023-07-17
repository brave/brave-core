// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import SwiftUI
import Combine
import Data

public struct AssetViewModel: Identifiable, Equatable {
  var token: BraveWallet.BlockchainToken
  var network: BraveWallet.NetworkInfo
  var decimalBalance: Double
  var price: String
  var history: [BraveWallet.AssetTimePrice]

  public var id: String {
    token.id + network.chainId
  }
  
  /// Sort by the fiat/value of the asset (price x balance), otherwise by balance when price is unavailable.
  static func sortedByValue(lhs: AssetViewModel, rhs: AssetViewModel) -> Bool {
    if let lhsPrice = Double(lhs.price),
       let rhsPrice = Double(rhs.price) {
      return (lhsPrice * lhs.decimalBalance) > (rhsPrice * rhs.decimalBalance)
    } else if let lhsPrice = Double(lhs.price), (lhsPrice * lhs.decimalBalance) > 0 {
      // lhs has a non-zero value
      return true
    } else if let rhsPrice = Double(rhs.price), (rhsPrice * rhs.decimalBalance) > 0 {
      // rhs has a non-zero value
      return false
    }
    // price unavailable, sort by balance
    return lhs.decimalBalance > rhs.decimalBalance
  }
}

struct BalanceTimePrice: DataPoint, Equatable {
  var date: Date
  var price: Double
  var formattedPrice: String

  var value: CGFloat {
    price
  }
}

/// A store containing data around the users assets
public class PortfolioStore: ObservableObject {
  /// The dollar amount of your portfolio
  @Published private(set) var balance: String = "$0.00"
  /// The users visible fungible tokens. NFTs are separated into `userVisibleNFTs`.
  @Published private(set) var userVisibleAssets: [AssetViewModel] = []
  /// The timeframe of the portfolio
  @Published var timeframe: BraveWallet.AssetPriceTimeframe = .oneDay {
    didSet {
      if timeframe != oldValue {
        update()
      }
    }
  }
  /// A set of balances of your portfolio's visible assets based on `timeframe`
  @Published private(set) var historicalBalances: [BalanceTimePrice] = []
  /// Whether or not balances are still currently loading
  @Published private(set) var isLoadingBalances: Bool = false
  /// The current default base currency code
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
  @Published private(set) var isLoadingDiscoverAssets: Bool = false

  public private(set) lazy var userAssetsStore: UserAssetsStore = .init(
    blockchainRegistry: self.blockchainRegistry,
    rpcService: self.rpcService,
    keyringService: self.keyringService,
    assetRatioService: self.assetRatioService,
    ipfsApi: self.ipfsApi,
    userAssetManager: self.assetManager
  )
  
  let currencyFormatter: NumberFormatter = .usdCurrencyFormatter
  
  /// Cancellable for the last running `update()` Task.
  private var updateTask: Task<(), Never>?
  /// Cache of total balances. The key is the token's `assetBalanceId`.
  private var totalBalancesCache: [String: Double] = [:]
  /// Cache of prices for each token. The key is the token's `assetRatioId`.
  private var pricesCache: [String: String] = [:]
  /// Cache of priceHistories. The key is the token's `assetRatioId`.
  private var priceHistoriesCache: [String: [BraveWallet.AssetTimePrice]] = [:]

  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let assetRatioService: BraveWalletAssetRatioService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let ipfsApi: IpfsAPI
  private let assetManager: WalletUserAssetManagerType

  public init(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    assetRatioService: BraveWalletAssetRatioService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    ipfsApi: IpfsAPI,
    userAssetManager: WalletUserAssetManagerType
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.assetRatioService = assetRatioService
    self.blockchainRegistry = blockchainRegistry
    self.ipfsApi = ipfsApi
    self.assetManager = userAssetManager

    self.rpcService.add(self)
    self.keyringService.add(self)
    self.walletService.add(self)

    keyringService.isLocked { [self] isLocked in
      if !isLocked {
        update()
      }
    }
    walletService.defaultBaseCurrency { [self] currencyCode in
      self.currencyCode = currencyCode
    }
  }
  
  func update() {
    self.updateTask?.cancel()
    self.updateTask = Task { @MainActor in
      self.isLoadingBalances = true
      // setup network filters if not currently setup
      if self.networkFilters.isEmpty {
        self.networkFilters = await self.rpcService.allNetworksForSupportedCoins().map {
          .init(isSelected: !WalletConstants.supportedTestNetworkChainIds.contains($0.chainId), model: $0)
        }
      }
      let networks: [BraveWallet.NetworkInfo] = self.networkFilters.filter(\.isSelected).map(\.model)
      struct NetworkAssets: Equatable {
        let network: BraveWallet.NetworkInfo
        let tokens: [BraveWallet.BlockchainToken]
        let sortOrder: Int
      }
      let allVisibleUserAssets = assetManager.getAllVisibleAssetsInNetworkAssets(networks: networks)
      var updatedUserVisibleAssets = buildAssetViewModels(allVisibleUserAssets: allVisibleUserAssets)
      // update userVisibleAssets on display immediately with empty values. Issue #5567
      self.userVisibleAssets = updatedUserVisibleAssets
        .sorted(by: AssetViewModel.sortedByValue(lhs:rhs:))
      let keyrings = await self.keyringService.keyrings(for: WalletConstants.supportedCoinTypes)
      guard !Task.isCancelled else { return }
      typealias TokenNetworkAccounts = (token: BraveWallet.BlockchainToken, network: BraveWallet.NetworkInfo, accounts: [BraveWallet.AccountInfo])
      let allTokenNetworkAccounts = allVisibleUserAssets.flatMap { networkAssets in
        networkAssets.tokens.map { token in
          TokenNetworkAccounts(
            token: token,
            network: networkAssets.network,
            accounts: keyrings.first(where: { $0.coin == token.coin })?.accountInfos ?? []
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
      for (key, value) in totalBalances {
        totalBalancesCache[key] = value
      }

      // fetch price for every token
      let allTokens = allVisibleUserAssets.flatMap(\.tokens)
      let allAssetRatioIds = allTokens.map(\.assetRatioId)
      let prices: [String: String] = await assetRatioService.fetchPrices(
        for: allAssetRatioIds,
        toAssets: [currencyFormatter.currencyCode],
        timeframe: timeframe
      )
      for (key, value) in prices { // update cached values
        self.pricesCache[key] = value
      }

      // fetch price history for every non-zero balance token
      let nonZeroBalanceAssetRatioIds: [String] = allTokens
        .filter { (totalBalancesCache[$0.assetBalanceId] ?? 0) > 0 }
        .map { $0.assetRatioId }
      let priceHistories: [String: [BraveWallet.AssetTimePrice]] = await fetchPriceHistory(for: nonZeroBalanceAssetRatioIds)
      for (key, value) in priceHistories { // update cached values
        self.priceHistoriesCache[key] = value
      }
      
      guard !Task.isCancelled else { return }
      updatedUserVisibleAssets = buildAssetViewModels(allVisibleUserAssets: allVisibleUserAssets)
      self.userVisibleAssets = updatedUserVisibleAssets
        .sorted(by: AssetViewModel.sortedByValue(lhs:rhs:))
      
      // Compute balance based on current prices
      let currentBalance = userVisibleAssets
        .compactMap {
          if let price = Double($0.price) {
            return $0.decimalBalance * price
          }
          return nil
        }
        .reduce(0.0, +)
      balance = currencyFormatter.string(from: NSNumber(value: currentBalance)) ?? "–"
      // Compute historical balances based on historical prices and current balances
      let assets = userVisibleAssets.filter { !$0.history.isEmpty }  // [[AssetTimePrice]]
      let minCount = assets.map(\.history.count).min() ?? 0  // Shortest array count
      historicalBalances = (0..<minCount).map { index in
        let value = assets.reduce(0.0, {
          $0 + ((Double($1.history[index].price) ?? 0.0) * $1.decimalBalance)
        })
        return .init(
          date: assets.map { $0.history[index].date }.max() ?? .init(),
          price: value,
          formattedPrice: currencyFormatter.string(from: NSNumber(value: value)) ?? "0.00"
        )
      }
      isLoadingBalances = false
    }
  }
  
  /// Builds the `AssetViewModel`s and `NFTAssetViewModel`s using the balances, price and metadata stored in their respective caches.
  private func buildAssetViewModels(allVisibleUserAssets: [NetworkAssets]) -> [AssetViewModel] {
    allVisibleUserAssets.flatMap { networkAssets in
      networkAssets.tokens.filter { (!$0.isErc721 && !$0.isNft) }.map { token in
        AssetViewModel(
          token: token,
          network: networkAssets.network,
          decimalBalance: totalBalancesCache[token.assetBalanceId] ?? 0,
          price: pricesCache[token.assetRatioId.lowercased()] ?? "",
          history: priceHistoriesCache[token.assetRatioId.lowercased()] ?? []
        )
      }
    }
  }
  
  /// Fetches the price history for the given `assetRatioId`, giving a dictionary with the price history for each symbol
  @MainActor func fetchPriceHistory(
    for priceIds: [String]
  ) async -> [String: [BraveWallet.AssetTimePrice]] {
    let uniquePriceIds = Set(priceIds)
    let priceHistories = await withTaskGroup(of: [String: [BraveWallet.AssetTimePrice]].self) { @MainActor group -> [String: [BraveWallet.AssetTimePrice]] in
      for priceId in uniquePriceIds {
        let priceId = priceId.lowercased()
        group.addTask { @MainActor in
          let (success, history) = await self.assetRatioService.priceHistory(
            priceId,
            vsAsset: self.currencyFormatter.currencyCode,
            timeframe: self.timeframe
          )
          if success {
            return [priceId: history.sorted(by: { $0.date < $1.date })]
          } else {
            return [:]
          }
        }
      }
      return await group.reduce(into: [String: [BraveWallet.AssetTimePrice]](), { partialResult, new in
        for key in new.keys {
          partialResult[key] = new[key]
        }
      })
    }
    return priceHistories
  }
}

extension PortfolioStore: BraveWalletJsonRpcServiceObserver {
  public func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }

  public func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }

  public func chainChangedEvent(_ chainId: String, coin: BraveWallet.CoinType, origin: URLOrigin?) {
    update()
  }
}

extension PortfolioStore: BraveWalletKeyringServiceObserver {
  public func keyringReset() {
  }

  public func accountsChanged() {
    update()
  }
  public func backedUp() {
  }
  public func keyringCreated(_ keyringId: BraveWallet.KeyringId) {
  }
  public func keyringRestored(_ keyringId: BraveWallet.KeyringId) {
  }
  public func locked() {
  }
  public func unlocked() {
    DispatchQueue.main.async { [self] in
      update()
    }
  }
  public func autoLockMinutesChanged() {
  }
  public func selectedAccountChanged(_ coinType: BraveWallet.CoinType) {
    DispatchQueue.main.async { [self] in
      update()
    }
  }
  
  public func accountsAdded(_ addedAccounts: [BraveWallet.AccountInfo]) {
  }
}

extension PortfolioStore: BraveWalletBraveWalletServiceObserver {
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
    Task { @MainActor in
      // A network was added or removed, update our network filters for the change.
      self.networkFilters = await self.rpcService.allNetworksForSupportedCoins().map { network in
        let defaultValue = !WalletConstants.supportedTestNetworkChainIds.contains(network.chainId)
        let existingSelectionValue = self.networkFilters.first(where: { $0.model.chainId == network.chainId})?.isSelected
        return .init(isSelected: existingSelectionValue ?? defaultValue, model: network)
      }
    }
  }
  
  public func onDefaultEthereumWalletChanged(_ wallet: BraveWallet.DefaultWallet) {
  }
  
  public func onDefaultSolanaWalletChanged(_ wallet: BraveWallet.DefaultWallet) {
  }
  
  public func onDiscoverAssetsStarted() {
    isLoadingDiscoverAssets = true
  }
  
  public func onDiscoverAssetsCompleted(_ discoveredAssets: [BraveWallet.BlockchainToken]) {
    isLoadingDiscoverAssets = false
    // assets update will be called via `CryptoStore`
  }
  
  public func onResetWallet() {
  }
}
