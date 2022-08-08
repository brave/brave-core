// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

public struct AssetViewModel: Identifiable, Equatable {
  var token: BraveWallet.BlockchainToken
  var decimalBalance: Double
  var price: String
  var history: [BraveWallet.AssetTimePrice]

  public var id: String {
    token.id
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
  /// The users visible assets
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

  public private(set) lazy var userAssetsStore: UserAssetsStore = .init(
    walletService: self.walletService,
    blockchainRegistry: self.blockchainRegistry,
    rpcService: self.rpcService,
    assetRatioService: self.assetRatioService
  )
  
  let currencyFormatter: NumberFormatter = .usdCurrencyFormatter
  
  /// Cancellable for the last running `update()` Task.
  private var updateTask: Task<(), Never>?

  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let assetRatioService: BraveWalletAssetRatioService
  private let blockchainRegistry: BraveWalletBlockchainRegistry

  public init(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    assetRatioService: BraveWalletAssetRatioService,
    blockchainRegistry: BraveWalletBlockchainRegistry
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.assetRatioService = assetRatioService
    self.blockchainRegistry = blockchainRegistry

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
      let coin = await walletService.selectedCoin()
      let network = await rpcService.network(coin)
      let userAssets = await walletService.userAssets(network.chainId, coin: coin)
      // if the task was cancelled, don't update the UI
      guard !Task.isCancelled else { return }
      // update userVisibleAssets on display immediately with empty values. Issue #5567
      userVisibleAssets = userAssets.map { token in
        AssetViewModel(
          token: token,
          decimalBalance: 0.0,
          price: "",
          history: []
        )
      }
      
      let keyring = await keyringService.keyringInfo(coin.keyringId)
      guard !Task.isCancelled else { return } // limit network request(s) if cancelled
      let balances = await fetchBalances(for: userAssets, accounts: keyring.accountInfos)
      guard !Task.isCancelled else { return } // limit network request(s) if cancelled
      let nonZeroBalanceTokens = balances.filter { $1 > 0 }.map { $0.key }
      let nonZeroBalanceTokensPriceIds = userAssets.filter({ nonZeroBalanceTokens.contains($0.symbol.lowercased())}).map { $0.assetRatioId }
      let priceHistories = await fetchPriceHistory(for: nonZeroBalanceTokensPriceIds)
      guard !Task.isCancelled else { return } // limit network request(s) if cancelled
      let visibleTokens = userAssets.filter(\.visible).map { $0.assetRatioId.lowercased() }
      let prices = await fetchPrices(for: visibleTokens)
      
      // if the task was cancelled, don't update the UI
      guard !Task.isCancelled else { return }
      
      // build our userVisibleAssets
      userVisibleAssets = userAssets.map { token in
        let symbol = token.symbol.lowercased()
        let priceId = token.assetRatioId.lowercased()
        return AssetViewModel(
          token: token,
          decimalBalance: balances[symbol] ?? 0.0,
          price: prices[priceId] ?? "",
          history: priceHistories[priceId] ?? []
        )
      }
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
  
  /// Fetches the balances for a given list of tokens for each of the given accounts, giving a dictionary with a balance for each token symbol.
  @MainActor func fetchBalances(
    for tokens: [BraveWallet.BlockchainToken],
    accounts: [BraveWallet.AccountInfo]
  ) async -> [String: Double] {
    let balances = await withTaskGroup(of: [String: Double].self) { @MainActor group -> [String: Double] in
      for account in accounts {
        for token in tokens {
          group.addTask { @MainActor in
            guard let balance = await self.rpcService.balance(for: token, in: account) else {
              return [:]
            }
            let symbol = token.symbol.lowercased()
            return [symbol: balance]
          }
        }
      }
      return await group.reduce(into: [String: Double](), { partialResult, new in
        for key in new.keys {
          partialResult[key, default: 0] += new[key] ?? 0
        }
      })
    }
    return balances
  }
  
  /// Fetches the prices for a given list of `assetRatioId`, giving a dictionary with the price for each symbol
  @MainActor func fetchPrices(
    for priceIds: [String]
  ) async -> [String: String] {
    let priceResult = await assetRatioService.priceWithIndividualRetry(
      priceIds.map { $0.lowercased() },
      toAssets: [currencyFormatter.currencyCode],
      timeframe: timeframe
    )
    let prices = Dictionary(uniqueKeysWithValues: priceResult.assetPrices.map { ($0.fromAsset, $0.price) })
    return prices
  }
  
  /// Fetches the price history for the given `assetRatioId`, giving a dictionary with the price history for each symbol
  @MainActor func fetchPriceHistory(
    for priceIds: [String]
  ) async -> [String: [BraveWallet.AssetTimePrice]] {
    let priceHistories = await withTaskGroup(of: [String: [BraveWallet.AssetTimePrice]].self) { @MainActor group -> [String: [BraveWallet.AssetTimePrice]] in
      for priceId in priceIds {
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

  public func chainChangedEvent(_ chainId: String, coin: BraveWallet.CoinType) {
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
  public func keyringCreated(_ keyringId: String) {
  }
  public func keyringRestored(_ keyringId: String) {
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
  }
  
  public func onDefaultEthereumWalletChanged(_ wallet: BraveWallet.DefaultWallet) {
  }
  
  public func onDefaultSolanaWalletChanged(_ wallet: BraveWallet.DefaultWallet) {
  }
}
