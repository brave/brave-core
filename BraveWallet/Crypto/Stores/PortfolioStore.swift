// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

public struct AssetViewModel: Identifiable, Equatable {
  var token: BraveWallet.ERCToken
  var balance: String
  var price: String
  
  public var id: String {
    token.id
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
      update()
    }
  }
  /// A set of balances of your portfolio's visible assets based on `timeframe`
  @Published var historicalBalances: [BraveWallet.AssetTimePrice] = []
  
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
  }
  
  /// Balances for ETH accounts in Wei
  private var balances: [String: String] = [:] // TODO: Convert to Fiat?
  
  func update() {
    // Update ETH balances
    keyringController.defaultKeyringInfo { [self] keyring in
      for account in keyring.accountInfos {
        rpcController.balance(account.address) { success, balance in
          if success {
            balances[account.address] = balance
          } else {
            balances[account.address] = nil
          }
        }
      }
    }
    rpcController.chainId { [self] chainId in
      // Get user assets for the selected chain
      walletService.userAssets(chainId) { [self] tokens in
        userVisibleAssets = tokens.map {
          .init(token: $0, balance: "", price: "")
        }
        // Fill prices for each asset
        assetRatioController.price(
          tokens.map { $0.symbol.lowercased() },
          toAssets: ["usd"], // TODO: Switch to users preferred currency
          timeframe: timeframe
        ) { success, assetPrices in
          guard success else { return }
          for assetPrice in assetPrices {
            if let index = userVisibleAssets.firstIndex(where: {
              $0.token.symbol.caseInsensitiveCompare(assetPrice.fromAsset) == .orderedSame
            }) {
              userVisibleAssets[index].price = assetPrice.price
            }
          }
          // Get price history for each asset for consolidated portfolio history
          struct AssetCandle {
            var token: BraveWallet.ERCToken
            var price: String
          }
          var assetHistoricalBalances: [Date: [AssetCandle]] = [:]
          let group = DispatchGroup()
          for asset in userVisibleAssets {
            group.enter()
            assetRatioController.priceHistory(asset.token.symbol, timeframe: timeframe) { success, history in
              defer { group.leave() }
              guard success else { return }
              for candle in history {
                assetHistoricalBalances[candle.date, default: []].append(.init(token: asset.token, price: candle.price))
              }
            }
          }
          group.notify(queue: .main) {
            // Consolidate all balances per day into Fiat…
          }
        }
        // Fill balances for erc20 tokens
        keyringController.defaultKeyringInfo { keyring in
          for account in keyring.accountInfos {
            for token in tokens where token.isErc20 {
              rpcController.erc20TokenBalance(token.contractAddress, address: account.address) { success, balance in
                guard success else { return }
                if let index = userVisibleAssets.firstIndex(where: {
                  $0.token.symbol.caseInsensitiveCompare(token.symbol) == .orderedSame
                }) {
                  userVisibleAssets[index].balance = balance // TODO: Convert from Wei to Fiat
                }
              }
            }
          }
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
    update()
  }
  public func autoLockMinutesChanged() {
  }
  public func selectedAccountChanged() {
  }
}
