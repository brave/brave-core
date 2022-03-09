// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import BigNumber

public class TransactionConfirmationStore: ObservableObject {
  struct State {
    var value: String = ""
    var symbol: String = ""
    var fiat: String = ""
    var gasValue: String = ""
    var gasSymbol: String = ""
    var gasFiat: String = ""
    var gasAssetRatio: Double = 0.0
    var totalFiat: String = ""
    var isBalanceSufficient: Bool = true
  }
  @Published var state: State = .init()
  @Published var isLoading: Bool = false
  @Published var gasEstimation1559: BraveWallet.GasEstimation1559?
  /// This is a list of all unpproved transactions iterated through all the accounts for the current keyring
  @Published private(set) var transactions: [BraveWallet.TransactionInfo] = []
  /// This is an id for the unppproved transaction that is currently displayed on screen
  @Published var activeTransactionId: BraveWallet.TransactionInfo.ID = "" {
    didSet {
      if let tx = transactions.first(where: { $0.id == activeTransactionId }) {
        fetchDetails(for: tx)
      } else if let firstTx = transactions.first {
        fetchDetails(for: firstTx)
      }
    }
  }
  
  private var assetRatios: [String: Double] = [:]
  
  let numberFormatter = NumberFormatter().then {
    $0.numberStyle = .currency
    $0.currencyCode = "USD"
  }
  
  private let assetRatioService: BraveWalletAssetRatioService
  private let rpcService: BraveWalletJsonRpcService
  private let txService: BraveWalletTxService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let walletService: BraveWalletBraveWalletService
  private let ethTxManagerProxy: BraveWalletEthTxManagerProxy
  private let keyringService: BraveWalletKeyringService
  private var selectedChain: BraveWallet.EthereumChain = .init()
  
  init(
    assetRatioService: BraveWalletAssetRatioService,
    rpcService: BraveWalletJsonRpcService,
    txService: BraveWalletTxService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    walletService: BraveWalletBraveWalletService,
    ethTxManagerProxy: BraveWalletEthTxManagerProxy,
    keyringService: BraveWalletKeyringService
  ) {
    self.assetRatioService = assetRatioService
    self.rpcService = rpcService
    self.txService = txService
    self.blockchainRegistry = blockchainRegistry
    self.walletService = walletService
    self.ethTxManagerProxy = ethTxManagerProxy
    self.keyringService = keyringService
    
    self.txService.add(self)
  }
  
  func updateGasValue(for transaction: BraveWallet.TransactionInfo) {
    let gasLimit = transaction.ethTxGasLimit
    let gasFormatter = WeiFormatter(decimalFormatStyle: .gasFee(limit: gasLimit.removingHexPrefix, radix: .hex))
    let gasFee: String
    if transaction.isEIP1559Transaction {
      gasFee = transaction.txDataUnion.ethTxData1559?.maxFeePerGas ?? ""
    } else {
      gasFee = transaction.ethTxGasPrice
    }
    self.state.gasValue = gasFormatter.decimalString(
      for: gasFee.removingHexPrefix,
         radix: .hex,
         decimals: Int(selectedChain.decimals)
    ) ?? ""
    if !self.state.gasValue.isEmpty {
      rpcService.balance(transaction.fromAddress, coin: .eth) { [weak self] weiBalance, status, _ in
        guard let self = self, status == .success else { return }
        let formatter = WeiFormatter(decimalFormatStyle: .balance)
        guard let decimalString = formatter.decimalString(
                for: weiBalance.removingHexPrefix, radix: .hex, decimals: Int(self.selectedChain.decimals)
              ),
              let value = BDouble(decimalString), let gasValue = BDouble(self.state.gasValue),
              value > gasValue else {
                self.state.isBalanceSufficient = false
                return
              }
        self.state.isBalanceSufficient = true
      }
    }
  }
  
  func fetchDetails(for transaction: BraveWallet.TransactionInfo) {
    state = .init() // Reset state
    isLoading = true
    
    rpcService.chainId { [weak self] chainId in
      guard let self = self else { return }
      self.rpcService.network { selectedChain in
        self.selectedChain = selectedChain
        
        self.state.gasSymbol = selectedChain.symbol
        self.updateGasValue(for: transaction)
        
        let formatter = WeiFormatter(decimalFormatStyle: .balance)
        let txValue = transaction.ethTxValue.removingHexPrefix
        
        self.blockchainRegistry.allTokens(BraveWallet.MainnetChainId) { tokens in
          self.walletService.userAssets(chainId) { userAssets in
            let allTokens = tokens + userAssets.filter { asset in
              // Only get custom tokens
              !tokens.contains(where: { $0.contractAddress == asset.contractAddress })
            }
            
            switch transaction.txType {
            case .erc20Approve:
              // Find token in args
              if let token = allTokens.first(where: { $0.contractAddress.caseInsensitiveCompare(transaction.txArgs[0]) == .orderedSame
              }) {
                self.state.symbol = token.symbol
                let approvalValue = transaction.txArgs[1].removingHexPrefix
                self.state.value = formatter.decimalString(for: approvalValue, radix: .hex, decimals: Int(token.decimals)) ?? ""
              }
            case .erc20Transfer:
              if let token = allTokens.first(where: { $0.contractAddress.caseInsensitiveCompare(transaction.ethTxToAddress) == .orderedSame
              }) {
                self.state.symbol = token.symbol
                let value = transaction.txArgs[1].removingHexPrefix
                self.state.value = formatter.decimalString(for: value, radix: .hex, decimals: Int(token.decimals)) ?? ""
              }
            case .ethSend, .other, .erc721TransferFrom, .erc721SafeTransferFrom:
              self.state.symbol = selectedChain.symbol
              self.state.value = formatter.decimalString(for: txValue, radix: .hex, decimals: Int(selectedChain.decimals)) ?? ""
            @unknown default:
              break
            }
            self.fetchAssetRatios(for: self.state.symbol, gasSymbol: self.state.gasSymbol)
          }
        }
      }
    }
  }
  
  private func fetchAssetRatios(for symbol: String, gasSymbol: String) {
    @discardableResult func updateState() -> Bool {
      if let ratio = assetRatios[symbolKey], let gasRatio = assetRatios[gasKey] {
        let value = (Double(self.state.value) ?? 0.0) * ratio
        self.state.fiat = numberFormatter.string(from: NSNumber(value: value)) ?? ""
        let gasValue = (Double(self.state.gasValue) ?? 0.0) * gasRatio
        self.state.gasFiat = numberFormatter.string(from: NSNumber(value: gasValue)) ?? ""
        self.state.totalFiat = numberFormatter.string(from: NSNumber(value: value + gasValue)) ?? ""
        self.state.gasAssetRatio = gasRatio
        return true
      }
      return false
    }
    let symbolKey = symbol.lowercased()
    let gasKey = gasSymbol.lowercased()
    if updateState() {
      isLoading = false
    } else {
      let symbols = symbolKey == gasKey ? [symbolKey] : [symbolKey, gasKey]
      assetRatioService.price(
        symbols,
        toAssets: ["usd"],
        timeframe: .oneDay
      ) { [weak self] success, prices in
        // `success` only refers to finding _all_ prices and if even 1 of N prices
        // fail to fetch it will be false
        guard let self = self,
              self.state.symbol.caseInsensitiveCompare(symbol) == .orderedSame else {
                return
              }
        if let price = prices.first(where: { $0.fromAsset == symbolKey }),
           let ratio = Double(price.price) {
          self.assetRatios[symbolKey] = ratio
        }
        if let price = prices.first(where: { $0.fromAsset == gasKey }),
           let gasRatio = Double(price.price) {
          self.assetRatios[gasKey] = gasRatio
        }
        updateState()
        self.isLoading = false
      }
    }
  }
  
  private func fetchGasEstimation1559() {
    ethTxManagerProxy.gasEstimation1559() { [weak self] gasEstimation in
      self?.gasEstimation1559 = gasEstimation
    }
  }
  
  private func fetchTransactions(completion: @escaping (() -> Void)) {
    keyringService.defaultKeyringInfo { [weak self] keyring in
      guard let self = self else { return }
      var pendingTransactions: [BraveWallet.TransactionInfo] = []
      let group = DispatchGroup()
      for info in keyring.accountInfos {
        group.enter()
        self.txService.allTransactionInfo(.eth, from: info.address) { tx in
          defer { group.leave() }
          pendingTransactions.append(contentsOf: tx.filter { $0.txStatus == .unapproved })
        }
      }
      group.notify(queue: .main) {
        self.transactions = pendingTransactions
        completion()
      }
    }
  }
  
  func confirm(transaction: BraveWallet.TransactionInfo) {
    txService.approveTransaction(.eth, txMetaId: transaction.id) { success, error, message  in
    }
  }
  
  func reject(transaction: BraveWallet.TransactionInfo) {
    txService.rejectTransaction(.eth, txMetaId: transaction.id) { success in
    }
  }
  
  func updateGasFeeAndLimits(
    for transaction: BraveWallet.TransactionInfo,
    maxPriorityFeePerGas: String,
    maxFeePerGas: String,
    gasLimit: String,
    completion: ((Bool) -> Void)? = nil
  ) {
    assert(transaction.isEIP1559Transaction,
           "Use updateGasFeeAndLimits(for:gasPrice:gasLimit:) for standard transactions")
    ethTxManagerProxy.setGasFeeAndLimitForUnapprovedTransaction(
      transaction.id,
      maxPriorityFeePerGas: maxPriorityFeePerGas,
      maxFeePerGas: maxFeePerGas,
      gasLimit: gasLimit
    ) { success in
      completion?(success)
    }
  }
  
  func updateGasFeeAndLimits(
    for transaction: BraveWallet.TransactionInfo,
    gasPrice: String,
    gasLimit: String,
    completion: ((Bool) -> Void)? = nil
  ) {
    assert(!transaction.isEIP1559Transaction,
           "Use updateGasFeeAndLimits(for:maxPriorityFeePerGas:maxFeePerGas:gasLimit:) for EIP-1559 transactions")
    ethTxManagerProxy.setGasPriceAndLimitForUnapprovedTransaction(
      transaction.id,
      gasPrice: gasPrice,
      gasLimit: gasLimit
    ) { success in
      completion?(success)
    }
  }
  
  func prepare() {
    fetchTransactions { [weak self] in
      guard let self = self,
            let firstTx = self.transactions.first
      else { return }
      self.activeTransactionId = firstTx.id
      self.fetchGasEstimation1559()
    }
  }
  
  func editNonce(
    for transaction: BraveWallet.TransactionInfo,
    nonce: String,
    completion: @escaping ((Bool) -> Void)
  ) {
    ethTxManagerProxy.setNonceForUnapprovedTransaction(transaction.id, nonce: nonce) { success in
      // not going to refresh unapproved transactions since the tx observer will be
      // notified `onTransactionStatusChanged` and `ononUnapprovedTxUpdated`
      // `transactions` list will be refreshed there.
      completion(success)
    }
  }
}

extension TransactionConfirmationStore: BraveWalletTxServiceObserver {
  public func onNewUnapprovedTx(_ txInfo: BraveWallet.TransactionInfo) {
    // won't have any new unapproved tx being added if you on tx confirmation panel
  }
  public func onTransactionStatusChanged(_ txInfo: BraveWallet.TransactionInfo) {
    refreshTransactions(txInfo)
  }
  public func onUnapprovedTxUpdated(_ txInfo: BraveWallet.TransactionInfo) {
    // refresh the unapproved transaction list, as well as tx details UI
    refreshTransactions(txInfo)
  }
  
  private func refreshTransactions(_ txInfo: BraveWallet.TransactionInfo) {
    fetchTransactions { [weak self] in
      guard let self = self else { return }
      if self.activeTransactionId == txInfo.id {
        self.fetchDetails(for: txInfo)
      }
    }
  }
}
