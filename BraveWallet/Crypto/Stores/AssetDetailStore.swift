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
  @Published private(set) var isBuySupported: Bool = true
  
  private(set) var assetPriceValue: Double = 0.0
  
  private let assetRatioService: BraveWalletAssetRatioService
  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let txService: BraveWalletEthTxService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  
  let token: BraveWallet.BlockchainToken
  
  init(
    assetRatioService: BraveWalletAssetRatioService,
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    txService: BraveWalletEthTxService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    token: BraveWallet.BlockchainToken
  ) {
    self.assetRatioService = assetRatioService
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.txService = txService
    self.blockchainRegistry = blockchainRegistry
    self.token = token
    
    self.keyringService.add(self)
    self.rpcService.add(self)
    self.txService.add(self)
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
    
    blockchainRegistry.buyTokens(BraveWallet.MainnetChainId) { [self] tokens in
      isBuySupported = tokens.first(where: { $0.symbol.lowercased() == token.symbol.lowercased() }) != nil
    }
    
    keyringService.defaultKeyringInfo { [self] keyring in
      accounts = keyring.accountInfos.map {
        .init(account: $0, decimalBalance: 0.0, balance: "", fiatBalance: "")
      }
      assetRatioService.price(
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
            if let deltaValue = Double(assetPrice.assetTimeframeChange) {
              self.priceIsDown = deltaValue < 0
              self.priceDelta = self.percentFormatter.string(from: NSNumber(value: deltaValue / 100.0)) ?? ""
            }
            for index in 0..<self.accounts.count {
              self.accounts[index].fiatBalance = Self.priceFormatter.string(from: NSNumber(value: self.accounts[index].decimalBalance * self.assetPriceValue)) ?? ""
            }
          }
          if let assetPrice = prices.first(where: { $0.toAsset == "btc" }) {
            self.btcRatio = "\(assetPrice.price) BTC"
          }
        }
      assetRatioService.priceHistory(
        token.symbol,
        vsAsset: "usd",
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
    keyringService.defaultKeyringInfo { [self] keyring in
      let group = DispatchGroup()
      for account in keyring.accountInfos {
        group.enter()
        rpcService.balance(for: token, in: account) { value in
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
    rpcService.network { [weak self] network in
      guard let self = self else { return }
      self.keyringService.defaultKeyringInfo { keyring in
        var allTransactions: [BraveWallet.TransactionInfo] = []
        let group = DispatchGroup()
        for account in keyring.accountInfos {
          group.enter()
          self.txService.allTransactionInfo(account.address) { txs in
            defer { group.leave() }
            allTransactions.append(contentsOf: txs)
          }
        }
        group.notify(queue: .main) {
          self.transactions = allTransactions
            .filter { tx in
              switch tx.txType {
              case .erc20Approve, .erc20Transfer:
                let toAddress = tx.txData.baseData.to
                return toAddress == self.token.contractAddress
              case .ethSend, .other, .erc721TransferFrom, .erc721SafeTransferFrom:
                return network.symbol.caseInsensitiveCompare(self.token.symbol) == .orderedSame
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

extension AssetDetailStore: BraveWalletKeyringServiceObserver {
  func keyringReset() {
  }
  
  func accountsChanged() {
    keyringService.defaultKeyringInfo { [self] keyring in
      accounts = keyring.accountInfos.map {
        .init(account: $0, decimalBalance: 0.0, balance: "", fiatBalance: "")
      }
      fetchAccountBalances()
      fetchTransactions()
    }
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

extension AssetDetailStore: BraveWalletJsonRpcServiceObserver {
  func chainChangedEvent(_ chainId: String) {
    fetchAccountBalances()
    DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
      // There's some async gap between the chain changing and the EthTxService having the the correct
      // chain which results in fetching the _previous_ chains transactions
      self.fetchTransactions()
    }
  }
  func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }
  func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
}

extension AssetDetailStore: BraveWalletEthTxServiceObserver {
  func onNewUnapprovedTx(_ txInfo: BraveWallet.TransactionInfo) {
  }
  func onUnapprovedTxUpdated(_ txInfo: BraveWallet.TransactionInfo) {
  }
  func onTransactionStatusChanged(_ txInfo: BraveWallet.TransactionInfo) {
    fetchTransactions()
  }
}
