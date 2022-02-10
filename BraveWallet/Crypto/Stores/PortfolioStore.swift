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
  
  public private(set) lazy var userAssetsStore: UserAssetsStore = .init(
    walletService: self.walletService,
    blockchainRegistry: self.blockchainRegistry,
    rpcService: self.rpcService,
    assetRatioService: self.assetRatioService
  )
  
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
    
    keyringService.isLocked { [self] isLocked in
      if !isLocked {
        update()
      }
    }
  }
  
  private let numberFormatter = NumberFormatter().then {
    $0.numberStyle = .currency
    $0.currencyCode = "USD"
  }
  
  /// Fills the `userVisibleAssets` models with balances in decimal format
  func fetchBalances(accounts: [BraveWallet.AccountInfo], completion: @escaping () -> Void) {
    let group = DispatchGroup()
    for account in accounts {
      for asset in userVisibleAssets {
        let token = asset.token
        group.enter()
        rpcService.balance(for: token, in: account) { [weak self] balance in
          defer { group.leave() }
          guard let self = self,
                let balance = balance,
                let index = self.userVisibleAssets.firstIndex(where: { $0.token.symbol == token.symbol }) else {
                  return
                }
          self.userVisibleAssets[index].decimalBalance += balance
        }
      }
    }
    group.notify(queue: .main, execute: completion)
  }
  
  func fetchPrices(_ completion: @escaping () -> Void) {
    assetRatioService.price(
      userVisibleAssets.map { $0.token.symbol.lowercased() },
      toAssets: ["usd"],
      timeframe: timeframe
    ) { [weak self] success, assetPrices in
      // `success` only refers to finding _all_ prices and if even 1 of N prices
      // fail to fetch it will be false
      guard let self = self else { return }
      for assetPrice in assetPrices {
        if let index = self.userVisibleAssets.firstIndex(where: {
          $0.token.symbol.caseInsensitiveCompare(assetPrice.fromAsset) == .orderedSame
        }) {
          self.userVisibleAssets[index].price = assetPrice.price
        }
      }
      completion()
    }
  }
  
  func fetchHistoryForNonZeroBalances(_ completion: @escaping () -> Void) {
    let assets = userVisibleAssets.filter { $0.decimalBalance > 0 }
    if assets.isEmpty {
      completion()
      return
    }
    let group = DispatchGroup()
    // Fill prices for each asset
    for asset in assets {
      group.enter()
      assetRatioService.priceHistory(
        asset.token.symbol,
        vsAsset: "usd",
        timeframe: timeframe
      ) { [weak self] success, history in
        defer { group.leave() }
        guard let self = self, success else { return }
        if let index = self.userVisibleAssets.firstIndex(where: {
          $0.token.symbol.caseInsensitiveCompare(asset.token.symbol) == .orderedSame
        }) {
          self.userVisibleAssets[index].history = history.sorted(by: { $0.date < $1.date })
        }
      }
    }
    group.notify(queue: .main, execute: completion)
  }
  
  func update() {
    isLoadingBalances = true
    rpcService.chainId { [self] chainId in
      // Get user assets for the selected chain
      walletService.userAssets(chainId) { [self] tokens in
        userVisibleAssets = tokens.filter(\.visible).map {
          .init(token: $0, decimalBalance: 0, price: "", history: [])
        }
        let group = DispatchGroup()
        group.enter()
        keyringService.defaultKeyringInfo { keyring in
          fetchBalances(accounts: keyring.accountInfos) {
            fetchHistoryForNonZeroBalances {
              group.leave()
            }
          }
        }
        group.enter()
        fetchPrices {
          group.leave()
        }
        group.notify(queue: .main) {
          // Compute balance based on current prices
          let currentBalance = userVisibleAssets
            .compactMap {
              if let price = Double($0.price) {
                return $0.decimalBalance * price
              }
              return nil
            }
            .reduce(0.0, +)
          balance = numberFormatter.string(from: NSNumber(value: currentBalance)) ?? "–"
          // Compute historical balances based on historical prices and current balances
          let assets = userVisibleAssets.filter { !$0.history.isEmpty } // [[AssetTimePrice]]
          let minCount = assets.map(\.history.count).min() ?? 0 // Shortest array count
          historicalBalances = (0..<minCount).map { index in
            let value = assets.reduce(0.0, {
              $0 + ((Double($1.history[index].price) ?? 0.0) * $1.decimalBalance)
            })
            return .init(
              date: assets.map { $0.history[index].date }.max() ?? .init(),
              price: value,
              formattedPrice: numberFormatter.string(from: NSNumber(value: value)) ?? "0.00"
            )
          }
          isLoadingBalances = false
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
  
  public func chainChangedEvent(_ chainId: String) {
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
  public func keyringCreated() {
  }
  public func keyringRestored() {
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
  public func selectedAccountChanged() {
  }
}
