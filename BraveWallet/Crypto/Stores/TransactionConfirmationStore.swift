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
  
  private var assetRatios: [String: Double] = [:]
  
  let numberFormatter = NumberFormatter().then {
    $0.numberStyle = .currency
    $0.currencyCode = "USD"
  }
  
  private let assetRatioService: BraveWalletAssetRatioService
  private let rpcService: BraveWalletJsonRpcService
  private let txService: BraveWalletEthTxService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let walletService: BraveWalletBraveWalletService
  private var selectedChain: BraveWallet.EthereumChain = .init()
  private var activeTransaction: BraveWallet.TransactionInfo?
  
  init(
    assetRatioService: BraveWalletAssetRatioService,
    rpcService: BraveWalletJsonRpcService,
    txService: BraveWalletEthTxService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    walletService: BraveWalletBraveWalletService
  ) {
    self.assetRatioService = assetRatioService
    self.rpcService = rpcService
    self.txService = txService
    self.blockchainRegistry = blockchainRegistry
    self.walletService = walletService
    
    self.txService.add(self)
  }
  
  func updateGasValue(for transaction: BraveWallet.TransactionInfo) {
    let gasLimit = transaction.txData.baseData.gasLimit
    let gasFormatter = WeiFormatter(decimalFormatStyle: .gasFee(limit: gasLimit.removingHexPrefix, radix: .hex))
    let gasFee: String
    if transaction.isEIP1559Transaction {
      gasFee = transaction.txData.maxFeePerGas
    } else {
      gasFee = transaction.txData.baseData.gasPrice
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
    activeTransaction = transaction
    
    rpcService.chainId { [weak self] chainId in
      guard let self = self else { return }
      self.rpcService.network { selectedChain in
        self.selectedChain = selectedChain
        
        self.state.gasSymbol = selectedChain.symbol
        self.updateGasValue(for: transaction)
        
        let formatter = WeiFormatter(decimalFormatStyle: .balance)
        let txValue = transaction.txData.baseData.value.removingHexPrefix
        
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
              if let token = allTokens.first(where: { $0.contractAddress.caseInsensitiveCompare(transaction.txData.baseData.to) == .orderedSame
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
  
  func confirm(transaction: BraveWallet.TransactionInfo) {
    txService.approveTransaction(transaction.id) { success in
    }
  }
  
  func reject(transaction: BraveWallet.TransactionInfo) {
    txService.rejectTransaction(transaction.id) { success in
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
    txService.setGasFeeAndLimitForUnapprovedTransaction(
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
    txService.setGasPriceAndLimitForUnapprovedTransaction(
      transaction.id,
      gasPrice: gasPrice,
      gasLimit: gasLimit
    ) { success in
      completion?(success)
    }
  }
}

extension TransactionConfirmationStore: BraveWalletEthTxServiceObserver {
  public func onNewUnapprovedTx(_ txInfo: BraveWallet.TransactionInfo) {
  }
  public func onTransactionStatusChanged(_ txInfo: BraveWallet.TransactionInfo) {
  }
  public func onUnapprovedTxUpdated(_ txInfo: BraveWallet.TransactionInfo) {
    if let tx = activeTransaction, txInfo.id == tx.id {
      fetchDetails(for: txInfo)
    }
  }
}
