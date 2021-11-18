// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

public struct AssetViewModel: Identifiable, Equatable {
  var token: BraveWallet.ERCToken
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
    tokenRegistry: self.tokenRegistry,
    rpcController: self.rpcController
  )
  
  private let keyringController: BraveWalletKeyringController
  private let rpcController: BraveWalletEthJsonRpcController
  private let walletService: BraveWalletBraveWalletService
  private let assetRatioController: BraveWalletAssetRatioController
  private let tokenRegistry: BraveWalletERCTokenRegistry
  
  public init(
    keyringController: BraveWalletKeyringController,
    rpcController: BraveWalletEthJsonRpcController,
    walletService: BraveWalletBraveWalletService,
    assetRatioController: BraveWalletAssetRatioController,
    tokenRegistry: BraveWalletERCTokenRegistry
  ) {
    self.keyringController = keyringController
    self.rpcController = rpcController
    self.walletService = walletService
    self.assetRatioController = assetRatioController
    self.tokenRegistry = tokenRegistry
    
    self.rpcController.add(self)
    self.keyringController.add(self)
    
    keyringController.isLocked { [self] isLocked in
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
    let balanceFormatter = WeiFormatter(decimalFormatStyle: .balance)
    let group = DispatchGroup()
    for account in accounts {
      for index in 0..<userVisibleAssets.count {
        let token = userVisibleAssets[index].token
        func updateBalance(_ success: Bool, _ balance: String) {
          guard success, let decimalString = balanceFormatter.decimalString(
            for: balance.removingHexPrefix,
               radix: .hex,
               decimals: Int(token.decimals)
          ), !decimalString.isEmpty, let decimal = Double(decimalString) else {
            return
          }
          userVisibleAssets[index].decimalBalance += decimal
        }
        if token.isETH {
          group.enter()
          rpcController.balance(account.address) { success, balance in
            defer { group.leave() }
            updateBalance(success, balance)
          }
        } else if token.isErc20 {
          group.enter()
          rpcController.erc20TokenBalance(token.contractAddress, address: account.address) { success, balance in
            defer { group.leave() }
            updateBalance(success, balance)
          }
        }
      }
    }
    group.notify(queue: .main, execute: completion)
  }
  
  func fetchPricesAndHistory(_ completion: @escaping () -> Void) {
    let group = DispatchGroup()
    // Fill prices for each asset
    group.enter()
    assetRatioController.price(
      userVisibleAssets.map { $0.token.symbol.lowercased() },
      toAssets: ["usd"], // TODO: Switch to users preferred currency
      timeframe: timeframe
    ) { [weak self] success, assetPrices in
      defer { group.leave() }
      guard let self = self, success else { return }
      for assetPrice in assetPrices {
        if let index = self.userVisibleAssets.firstIndex(where: {
          $0.token.symbol.caseInsensitiveCompare(assetPrice.fromAsset) == .orderedSame
        }) {
          self.userVisibleAssets[index].price = assetPrice.price
        }
      }
    }
    for asset in userVisibleAssets {
      group.enter()
      assetRatioController.priceHistory(
        asset.token.symbol,
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
    rpcController.chainId { [self] chainId in
      // Get user assets for the selected chain
      walletService.userAssets(chainId) { [self] tokens in
        userVisibleAssets = tokens.map {
          .init(token: $0, decimalBalance: 0, price: "", history: [])
        }
        let group = DispatchGroup()
        group.enter()
        keyringController.defaultKeyringInfo { keyring in
          fetchBalances(accounts: keyring.accountInfos) {
            group.leave()
          }
        }
        group.enter()
        fetchPricesAndHistory {
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

extension PortfolioStore: BraveWalletEthJsonRpcControllerObserver {
  public func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
  
  public func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }
  
  public func chainChangedEvent(_ chainId: String) {
    update()
  }
}

extension PortfolioStore: BraveWalletKeyringControllerObserver {
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
