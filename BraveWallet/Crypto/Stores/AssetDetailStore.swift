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
  @Published private(set) var transactions: [BraveWallet.TransactionInfo] = []
  
  private(set) var assetPriceValue: Double = 0.0
  
  private let assetRatioController: BraveWalletAssetRatioController
  private let keyringController: BraveWalletKeyringController
  private let rpcController: BraveWalletEthJsonRpcController
  private let txController: BraveWalletEthTxController
  
  let token: BraveWallet.ERCToken
  
  init(
    assetRatioController: BraveWalletAssetRatioController,
    keyringController: BraveWalletKeyringController,
    rpcController: BraveWalletEthJsonRpcController,
    txController: BraveWalletEthTxController,
    token: BraveWallet.ERCToken
  ) {
    self.assetRatioController = assetRatioController
    self.keyringController = keyringController
    self.rpcController = rpcController
    self.txController = txController
    self.token = token
    
    self.keyringController.add(self)
    self.rpcController.add(self)
    self.txController.add(self)
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
    
    keyringController.defaultKeyringInfo { [self] keyring in
      accounts = keyring.accountInfos.map {
        .init(account: $0, decimalBalance: 0.0, balance: "", fiatBalance: "")
      }
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
      fetchTransactions()
    }
  }
  
  func fetchAccountBalances() {
    isLoadingAccountBalances = true
    keyringController.defaultKeyringInfo { [self] keyring in
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
  
  func fetchTransactions() {
    rpcController.chainId { [self] chainId in
      rpcController.allNetworks { networks in
        let selectedNetwork = networks.first(where: { $0.chainId == chainId }) ?? .init()
        keyringController.defaultKeyringInfo { keyring in
          var transactions: [BraveWallet.TransactionInfo] = []
          let group = DispatchGroup()
          for account in keyring.accountInfos {
            group.enter()
            txController.allTransactionInfo(account.address) { txs in
              defer { group.leave() }
              transactions.append(contentsOf: txs)
            }
          }
          group.notify(queue: .main) {
            self.transactions = transactions
              .filter { tx in
                switch tx.txType {
                case .erc20Approve, .erc20Transfer:
                  let toAddress = tx.txData.baseData.to
                  return toAddress == token.contractAddress
                case .ethSend, .other, .erc721TransferFrom, .erc721SafeTransferFrom:
                  return selectedNetwork.symbol.caseInsensitiveCompare(token.symbol) == .orderedSame
                @unknown default:
                  return false
                }
              }
              .sorted(by: { $0.createdTime > $1.createdTime })
          }
        }
      }
    }
  }
}

extension AssetDetailStore: BraveWalletKeyringControllerObserver {
  func accountsChanged() {
    fetchAccountBalances()
    fetchTransactions()
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
    DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
      // There's some async gap between the chain changing and the EthTxController having the the correct
      // chain which results in fetching the _previous_ chains transactions
      self.fetchTransactions()
    }
  }
  func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }
  func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
}

extension AssetDetailStore: BraveWalletEthTxControllerObserver {
  func onNewUnapprovedTx(_ txInfo: BraveWallet.TransactionInfo) {
  }
  func onUnapprovedTxUpdated(_ txInfo: BraveWallet.TransactionInfo) {
  }
  func onTransactionStatusChanged(_ txInfo: BraveWallet.TransactionInfo) {
    fetchTransactions()
  }
}
