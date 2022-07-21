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

  /// Fetches the balances for a given list of tokens for each of the given accounts, giving a dictionary with a balance for each token symbol.
  private func fetchBalances(for tokens: [BraveWallet.BlockchainToken], accounts: [BraveWallet.AccountInfo], completion: @escaping ([String: Double]) -> Void) {
    var balances: [String: Double] = [:]
    let group = DispatchGroup()
    for account in accounts {
      for token in tokens {
        group.enter()
        rpcService.balance(for: token, in: account) { balance in
          defer { group.leave() }
          guard let balance = balance else {
            return
          }
          let symbol = token.symbol.lowercased()
          balances[symbol, default: 0] += balance
        }
      }
    }
    group.notify(
      queue: .main,
      execute: {
        completion(balances)
      })
  }

  /// Fetches the prices for a given list of symbols, giving a dictionary with the price for each symbol
  private func fetchPrices(for symbols: [String], completion: @escaping ([String: String]) -> Void) {
    assetRatioService.priceWithIndividualRetry(
      symbols.map { $0.lowercased() },
      toAssets: [currencyFormatter.currencyCode],
      timeframe: timeframe
    ) { priceResult in
      let prices = Dictionary(uniqueKeysWithValues: priceResult.assetPrices.map { ($0.fromAsset, $0.price) })
      completion(prices)
    }
  }

  /// Fetches the price history for the given symbols, giving a dictionary with the price history for each symbol
  private func fetchPriceHistory(for symbols: [String], completion: @escaping ([String: [BraveWallet.AssetTimePrice]]) -> Void) {
    var priceHistories: [String: [BraveWallet.AssetTimePrice]] = [:]
    let group = DispatchGroup()
    for symbol in symbols {
      let symbol = symbol.lowercased()
      group.enter()
      assetRatioService.priceHistory(
        symbol,
        vsAsset: currencyFormatter.currencyCode,
        timeframe: timeframe
      ) { success, history in
        defer { group.leave() }
        guard success else { return }
        priceHistories[symbol] = history.sorted(by: { $0.date < $1.date })
      }
    }
    group.notify(
      queue: .main,
      execute: {
        completion(priceHistories)
      })
  }

  func update() {
    isLoadingBalances = true
    walletService.selectedCoin { [self] coin in
      rpcService.network(coin) { [self] network in
        // Get user assets for the selected chain
        self.walletService.userAssets(network.chainId, coin: network.coin) { [self] tokens in
          let visibleTokens = tokens.filter(\.visible)
          let dispatchGroup = DispatchGroup()
          // fetch user balances, then fetch price history for tokens with non-zero balance
          var balances: [String: Double] = [:]
          var priceHistories: [String: [BraveWallet.AssetTimePrice]] = [:]
          dispatchGroup.enter()
          keyringService.keyringInfo(network.coin.keyringId) { [self] keyring in
            fetchBalances(for: visibleTokens, accounts: keyring.accountInfos) { [self] fetchedBalances in
              balances = fetchedBalances
              let nonZeroBalanceTokens = balances.filter { $1 > 0 }.map { $0.key }
              fetchPriceHistory(for: nonZeroBalanceTokens) { fetchedPriceHistories in
                defer { dispatchGroup.leave() }
                priceHistories = fetchedPriceHistories
              }
            }
          }
          // fetch prices
          let visibleTokenSymbols = visibleTokens.map { $0.symbol.lowercased() }
          var prices: [String: String] = [:]
          dispatchGroup.enter()
          fetchPrices(for: visibleTokenSymbols) { fetchedPrices in
            defer { dispatchGroup.leave() }
            prices = fetchedPrices
          }
          dispatchGroup.notify(queue: .main) { [self] in
            // build our userVisibleAssets
            userVisibleAssets = visibleTokens.map { token in
              let symbol = token.symbol.lowercased()
              return AssetViewModel(
                token: token,
                decimalBalance: balances[symbol] ?? 0.0,
                price: prices[symbol] ?? "",
                history: priceHistories[symbol] ?? []
              )
            }
            // Compute balance based on current prices
            let currentBalance =
            userVisibleAssets
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
      }
    }
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
}
