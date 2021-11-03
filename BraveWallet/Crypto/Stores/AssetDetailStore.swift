// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import SwiftUI

struct AccountAssetViewModel: Identifiable {
  var account: BraveWallet.AccountInfo
  fileprivate var decimalBalance: Double
  var balance: String
  var fiatBalance: String
  
  var id: String {
    account.id
  }
}

class AssetDetailStore: ObservableObject {
  @Published private(set) var isInitialState: Bool = true
  @Published private(set) var isLoadingPrice: Bool = false
  @Published private(set) var isLoadingChart: Bool = false
  @Published private(set) var price: String = "$0.0000"
  @Published private(set) var priceDelta: String = "0.00%"
  @Published private(set) var priceIsDown: Bool = false
  @Published private(set) var btcRatio: String = "0.0000 BTC"
  @Published private(set) var priceHistory: [BraveWallet.AssetTimePrice] = []
  @Published var timeframe: BraveWallet.AssetPriceTimeframe = .oneDay {
    didSet {
      if timeframe != oldValue {
        update()
      }
    }
  }
  @Published private(set) var isLoadingAccountBalances: Bool = false
  @Published private(set) var accounts: [AccountAssetViewModel] = []
  
  private var assetPriceValue: Double = 0.0
  
  private let assetRatioController: BraveWalletAssetRatioController
  private let keyringController: BraveWalletKeyringController
  private let rpcController: BraveWalletEthJsonRpcController
  
  let token: BraveWallet.ERCToken
  
  init(
    assetRatioController: BraveWalletAssetRatioController,
    keyringController: BraveWalletKeyringController,
    rpcController: BraveWalletEthJsonRpcController,
    token: BraveWallet.ERCToken
  ) {
    self.assetRatioController = assetRatioController
    self.keyringController = keyringController
    self.rpcController = rpcController
    self.token = token
    
    self.keyringController.add(self)
    self.rpcController.add(self)
  }
  
  static let priceFormatter = NumberFormatter().then {
    $0.numberStyle = .currency
    $0.currencyCode = "USD"
    $0.maximumFractionDigits = 4
  }
  
  private let percentFormatter = NumberFormatter().then {
    $0.numberStyle = .percent
    $0.maximumFractionDigits = 2
  }
  
  func update() {
    isLoadingPrice = true
    isLoadingChart = true
    
    assetRatioController.price(
      [token.symbol],
      toAssets: ["usd", "btc"],
      timeframe: timeframe) { [weak self] success, prices in
        guard let self = self else { return }
        self.isLoadingPrice = false
        self.isInitialState = false
        if let assetPrice = prices.first(where: { $0.toAsset == "usd" }),
           let value = Double(assetPrice.price) {
          self.assetPriceValue = value
          self.price = Self.priceFormatter.string(from: NSNumber(value: value)) ?? ""
          if let deltaVaue = Double(assetPrice.assetTimeframeChange) {
            self.priceIsDown = deltaVaue.isZero || deltaVaue > 0 ? false : true
            self.priceDelta = self.percentFormatter.string(from: NSNumber(value: deltaVaue / 100.0)) ?? ""
          }
          for index in 0..<self.accounts.count {
            self.accounts[index].fiatBalance = Self.priceFormatter.string(from: NSNumber(value: self.accounts[index].decimalBalance * self.assetPriceValue)) ?? ""
          }
        }
        if let assetPrice = prices.first(where: { $0.toAsset == "btc" }) {
          self.btcRatio = "\(assetPrice.price) BTC"
        }
      }
    assetRatioController.priceHistory(
      token.symbol,
      timeframe: timeframe
    ) { [weak self] success, history in
      guard let self = self else { return }
      self.isLoadingChart = false
      self.priceHistory = history
    }
    fetchAccountBalances()
  }
  
  func fetchAccountBalances() {
    isLoadingAccountBalances = true
    keyringController.defaultKeyringInfo { [self] keyring in
      accounts = keyring.accountInfos.map {
        .init(account: $0, decimalBalance: 0.0, balance: "", fiatBalance: "")
      }
      let group = DispatchGroup()
      for account in keyring.accountInfos {
        group.enter()
        rpcController.balance(for: token, in: account) { value in
          defer { group.leave() }
          if let index = accounts.firstIndex(where: { $0.account.address == account.address }) {
            accounts[index].decimalBalance = value ?? 0.0
            accounts[index].balance = String(format: "%.4f", value ?? 0.0)
            accounts[index].fiatBalance = Self.priceFormatter.string(from: NSNumber(value: self.accounts[index].decimalBalance * assetPriceValue)) ?? ""
          }
        }
      }
      group.notify(queue: .main) {
        self.isLoadingAccountBalances = false
      }
    }
  }
}

extension AssetDetailStore: BraveWalletKeyringControllerObserver {
  func accountsChanged() {
    fetchAccountBalances()
  }
  
  func keyringCreated() {
  }
  
  func keyringRestored() {
  }
  
  func locked() {
  }
  
  func unlocked() {
  }
  
  func backedUp() {
  }
  
  func autoLockMinutesChanged() {
  }
  
  func selectedAccountChanged() {
  }
}

extension AssetDetailStore: BraveWalletEthJsonRpcControllerObserver {
  func chainChangedEvent(_ chainId: String) {
    fetchAccountBalances()
  }
  func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }
  func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
}
