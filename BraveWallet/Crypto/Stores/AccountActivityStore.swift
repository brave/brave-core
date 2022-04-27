// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

class AccountActivityStore: ObservableObject {
  let account: BraveWallet.AccountInfo
  @Published private(set) var assets: [AssetViewModel] = []
  @Published private(set) var transactions: [BraveWallet.TransactionInfo] = []
  @Published private(set) var allTokens: [BraveWallet.BlockchainToken] = []

  private let walletService: BraveWalletBraveWalletService
  private let rpcService: BraveWalletJsonRpcService
  private let assetRatioService: BraveWalletAssetRatioService
  private let txService: BraveWalletTxService
  private let blockchainRegistry: BraveWalletBlockchainRegistry

  init(
    account: BraveWallet.AccountInfo,
    walletService: BraveWalletBraveWalletService,
    rpcService: BraveWalletJsonRpcService,
    assetRatioService: BraveWalletAssetRatioService,
    txService: BraveWalletTxService,
    blockchainRegistry: BraveWalletBlockchainRegistry
  ) {
    self.account = account
    self.walletService = walletService
    self.rpcService = rpcService
    self.assetRatioService = assetRatioService
    self.txService = txService
    self.blockchainRegistry = blockchainRegistry
    
    self.rpcService.add(self)
    self.txService.add(self)
  }

  func update() {
    fetchAssets()
    fetchTransactions()
  }

  private func fetchAssets() {
    rpcService.network { [self] network in
      let chainId = network.chainId
      blockchainRegistry.allTokens(chainId, coin: network.coin) { [self] allTokens in
        self.allTokens = allTokens
      }
      walletService.userAssets(chainId, coin: network.coin) { [self] tokens in
        var updatedAssets = tokens.map {
          AssetViewModel(token: $0, decimalBalance: 0, price: "", history: [])
        }
        let updatedTokens = updatedAssets.map(\.token)
        // fetch price & balance for each asset
        let dispatchGroup = DispatchGroup()
        dispatchGroup.enter()
        assetRatioService.price(
          updatedTokens.map { $0.symbol.lowercased() },
          toAssets: ["usd"],
          timeframe: .oneDay) { success, prices in
            defer { dispatchGroup.leave() }
            for price in prices {
              if let index = updatedAssets.firstIndex(where: {
                $0.token.symbol.caseInsensitiveCompare(price.fromAsset) == .orderedSame
              }) {
                updatedAssets[index].price = price.price
              }
            }
          }
        for token in updatedTokens {
          dispatchGroup.enter()
          rpcService.balance(for: token, in: account) { value in
            defer { dispatchGroup.leave() }
            if let value = value, let index = updatedAssets.firstIndex(where: {
              $0.token.symbol.caseInsensitiveCompare(token.symbol) == .orderedSame
            }) {
              updatedAssets[index].decimalBalance = value
            }
          }
        }
        dispatchGroup.notify(queue: .main) {
          self.assets = updatedAssets
        }
      }
    }
  }

  private func fetchTransactions() {
    txService.allTransactionInfo(.eth, from: account.address) { transactions in
      self.transactions =
        transactions
        .sorted(by: { $0.createdTime > $1.createdTime })
    }
  }
}

extension AccountActivityStore: BraveWalletJsonRpcServiceObserver {
  func chainChangedEvent(_ chainId: String, coin: BraveWallet.CoinType) {
    DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
      // Handle small gap between chain changing and txController having the correct chain Id
      self.update()
    }
  }
  func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }
  func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
}

extension AccountActivityStore: BraveWalletTxServiceObserver {
  func onNewUnapprovedTx(_ txInfo: BraveWallet.TransactionInfo) {
  }
  func onTransactionStatusChanged(_ txInfo: BraveWallet.TransactionInfo) {
    fetchTransactions()
  }
  func onUnapprovedTxUpdated(_ txInfo: BraveWallet.TransactionInfo) {
  }
}
