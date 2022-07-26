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
  @Published private(set) var transactionSummaries: [TransactionSummary] = []
  @Published private(set) var isBuySupported: Bool = true
  @Published private(set) var currencyCode: String = CurrencyCode.usd.code {
    didSet {
      currencyFormatter.currencyCode = currencyCode
      update()
    }
  }

  let currencyFormatter: NumberFormatter = .usdCurrencyFormatter

  private(set) var assetPriceValue: Double = 0.0

  private let assetRatioService: BraveWalletAssetRatioService
  private let keyringService: BraveWalletKeyringService
  private let walletService: BraveWalletBraveWalletService
  private let rpcService: BraveWalletJsonRpcService
  private let txService: BraveWalletTxService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let solTxManagerProxy: BraveWalletSolanaTxManagerProxy

  let token: BraveWallet.BlockchainToken

  init(
    assetRatioService: BraveWalletAssetRatioService,
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    txService: BraveWalletTxService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    solTxManagerProxy: BraveWalletSolanaTxManagerProxy,
    token: BraveWallet.BlockchainToken
  ) {
    self.assetRatioService = assetRatioService
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.txService = txService
    self.blockchainRegistry = blockchainRegistry
    self.solTxManagerProxy = solTxManagerProxy
    self.token = token

    self.keyringService.add(self)
    self.rpcService.add(self)
    self.txService.add(self)
    self.walletService.add(self)

    walletService.defaultBaseCurrency { [self] currencyCode in
      self.currencyCode = currencyCode
    }
  }

  private let percentFormatter = NumberFormatter().then {
    $0.numberStyle = .percent
    $0.maximumFractionDigits = 2
  }
  
  public func update() {
    Task { @MainActor in
      self.isLoadingPrice = true
      self.isLoadingChart = true
      let coin = await walletService.selectedCoin()
      let network = await rpcService.network(coin)
      let buyTokens = await blockchainRegistry.buyTokens(.wyre, chainId: network.chainId)
      self.isBuySupported = buyTokens.first(where: { $0.symbol.lowercased() == token.symbol.lowercased() }) != nil
      // fetch accounts
      let keyring = await keyringService.keyringInfo(coin.keyringId)
      var updatedAccounts = keyring.accountInfos.map {
        AccountAssetViewModel(account: $0, decimalBalance: 0.0, balance: "", fiatBalance: "")
      }
      // fetch prices for the asset
      let (_, prices) = await assetRatioService.price([token.symbol], toAssets: [currencyFormatter.currencyCode, "btc"], timeframe: timeframe)
      self.isLoadingPrice = false
      self.isInitialState = false
      if let assetPrice = prices.first(where: { $0.toAsset.caseInsensitiveCompare(self.currencyFormatter.currencyCode) == .orderedSame }),
        let value = Double(assetPrice.price) {
        self.assetPriceValue = value
        self.price = self.currencyFormatter.string(from: NSNumber(value: value)) ?? ""
        if let deltaValue = Double(assetPrice.assetTimeframeChange) {
          self.priceIsDown = deltaValue < 0
          self.priceDelta = self.percentFormatter.string(from: NSNumber(value: deltaValue / 100.0)) ?? ""
        }
        for index in 0..<updatedAccounts.count {
          updatedAccounts[index].fiatBalance = self.currencyFormatter.string(from: NSNumber(value: updatedAccounts[index].decimalBalance * self.assetPriceValue)) ?? ""
        }
      }
      if let assetPrice = prices.first(where: { $0.toAsset == "btc" }) {
        self.btcRatio = "\(assetPrice.price) BTC"
      }
      // fetch price history for the asset
      let (_, priceHistory) = await assetRatioService.priceHistory(token.symbol, vsAsset: currencyFormatter.currencyCode, timeframe: timeframe)
      self.isLoadingChart = false
      self.priceHistory = priceHistory
      
      self.accounts = await fetchAccountBalances(updatedAccounts, keyring: keyring)
      let assetRatios = [token.symbol.lowercased(): assetPriceValue]
      self.transactionSummaries = await fetchTransactionSummarys(keyring: keyring, assetRatios: assetRatios)
    }
  }
  
  @MainActor private func fetchAccountBalances(
    _ accountAssetViewModels: [AccountAssetViewModel],
    keyring: BraveWallet.KeyringInfo
  ) async -> [AccountAssetViewModel] {
    var accountAssetViewModels = accountAssetViewModels
    isLoadingAccountBalances = true
    typealias AccountBalance = (account: BraveWallet.AccountInfo, balance: Double?)
    let tokenBalances = await withTaskGroup(of: [AccountBalance].self) { group -> [AccountBalance] in
      for account in keyring.accountInfos {
        group.addTask {
          let balance = await self.rpcService.balance(for: self.token, in: account)
          return [AccountBalance(account, balance)]
        }
      }
      return await group.reduce([AccountBalance](), { $0 + $1 })
    }
    for tokenBalance in tokenBalances {
      if let index = accounts.firstIndex(where: { $0.account.address == tokenBalance.account.address }) {
        accountAssetViewModels[index].decimalBalance = tokenBalance.balance ?? 0.0
        accountAssetViewModels[index].balance = String(format: "%.4f", tokenBalance.balance ?? 0.0)
        accountAssetViewModels[index].fiatBalance = self.currencyFormatter.string(from: NSNumber(value: accountAssetViewModels[index].decimalBalance * assetPriceValue)) ?? ""
      }
    }
    self.isLoadingAccountBalances = false
    return accountAssetViewModels
  }
  
  @MainActor private func fetchTransactionSummarys(
    keyring: BraveWallet.KeyringInfo,
    assetRatios: [String: Double]
  ) async -> [TransactionSummary] {
    let coin = token.coin
    let network = await rpcService.network(coin)
    let userVisibleAssets = await walletService.userAssets(network.chainId, coin: coin)
    let allTokens = await blockchainRegistry.allTokens(network.chainId, coin: coin)
    let allTransactions = await withTaskGroup(of: [BraveWallet.TransactionInfo].self) { group -> [BraveWallet.TransactionInfo] in
      for account in keyring.accountInfos {
        group.addTask {
          await self.txService.allTransactionInfo(coin, from: account.address)
        }
      }
      return await group.reduce([BraveWallet.TransactionInfo](), { partialResult, prior in
        return partialResult + prior
      })
    }
    var solEstimatedTxFees: [String: UInt64] = [:]
    if token.coin == .sol {
      solEstimatedTxFees = await solTxManagerProxy.estimatedTxFees(for: allTransactions.map(\.id))
    }
    return allTransactions
      .filter { tx in
        switch tx.txType {
        case .erc20Approve, .erc20Transfer:
          guard let tokenContractAddress = tx.txDataUnion.ethTxData1559?.baseData.to else {
            return false
          }
          return tokenContractAddress.caseInsensitiveCompare(self.token.contractAddress) == .orderedSame
        case .ethSend, .ethSwap, .other, .erc721TransferFrom, .erc721SafeTransferFrom:
          return network.symbol.caseInsensitiveCompare(self.token.symbol) == .orderedSame
        case .solanaSystemTransfer:
          return network.symbol.caseInsensitiveCompare(self.token.symbol) == .orderedSame
        case .solanaSplTokenTransfer, .solanaSplTokenTransferWithAssociatedTokenAccountCreation:
          guard let tokenContractAddress = tx.txDataUnion.solanaTxData?.splTokenMintAddress else {
            return false
          }
          return tokenContractAddress.caseInsensitiveCompare(self.token.contractAddress) == .orderedSame
        case .erc1155SafeTransferFrom, .solanaDappSignTransaction, .solanaDappSignAndSendTransaction:
          return false
        @unknown default:
          return false
        }
      }
      .sorted(by: { $0.createdTime > $1.createdTime })
      .map { transaction in
        TransactionParser.transactionSummary(
          from: transaction,
          network: network,
          accountInfos: keyring.accountInfos,
          visibleTokens: userVisibleAssets,
          allTokens: allTokens,
          assetRatios: assetRatios,
          solEstimatedTxFee: solEstimatedTxFees[transaction.id],
          currencyFormatter: self.currencyFormatter
        )
      }
  }
}

extension AssetDetailStore: BraveWalletKeyringServiceObserver {
  func keyringReset() {
  }

  func accountsChanged() {
    update()
  }

  func keyringCreated(_ keyringId: String) {
  }

  func keyringRestored(_ keyringId: String) {
  }

  func locked() {
  }

  func unlocked() {
  }

  func backedUp() {
  }

  func autoLockMinutesChanged() {
  }

  func selectedAccountChanged(_ coin: BraveWallet.CoinType) {
  }
}

extension AssetDetailStore: BraveWalletJsonRpcServiceObserver {
  func chainChangedEvent(_ chainId: String, coin: BraveWallet.CoinType) {
    DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
      // There's some async gap between the chain changing and the EthTxService having the the correct
      // chain which results in fetching the _previous_ chains transactions
      self.update()
    }
  }
  func onAddEthereumChainRequestCompleted(_ chainId: String, error: String) {
  }
  func onIsEip1559Changed(_ chainId: String, isEip1559: Bool) {
  }
}

extension AssetDetailStore: BraveWalletTxServiceObserver {
  func onNewUnapprovedTx(_ txInfo: BraveWallet.TransactionInfo) {
  }
  func onUnapprovedTxUpdated(_ txInfo: BraveWallet.TransactionInfo) {
  }
  func onTransactionStatusChanged(_ txInfo: BraveWallet.TransactionInfo) {
    update()
  }
}

extension AssetDetailStore: BraveWalletBraveWalletServiceObserver {
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
