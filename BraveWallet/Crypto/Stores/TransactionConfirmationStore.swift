// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import BigNumber
import Strings

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
    var isUnlimitedApprovalRequested: Bool = false
    var currentAllowance: String = ""
    var originInfo: BraveWallet.OriginInfo?
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
  @Published var allTokens: [BraveWallet.BlockchainToken] = []
  @Published private(set) var currencyCode: String = CurrencyCode.usd.code {
    didSet {
      currencyFormatter.currencyCode = currencyCode
      fetchDetails(for: activeTransaction)
    }
  }

  private var assetRatios: [String: Double] = [:]

  let currencyFormatter: NumberFormatter = .usdCurrencyFormatter
  
  var activeTransaction: BraveWallet.TransactionInfo {
    transactions.first(where: { $0.id == activeTransactionId }) ?? (transactions.first ?? .init())
  }

  private let assetRatioService: BraveWalletAssetRatioService
  private let rpcService: BraveWalletJsonRpcService
  private let txService: BraveWalletTxService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let walletService: BraveWalletBraveWalletService
  private let ethTxManagerProxy: BraveWalletEthTxManagerProxy
  private let keyringService: BraveWalletKeyringService
  private var selectedChain: BraveWallet.NetworkInfo = .init()

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
    self.walletService.add(self)
    
    walletService.defaultBaseCurrency { [self] currencyCode in
      self.currencyCode = currencyCode
    }
  }
  
  func nextTransaction() {
    if let index = transactions.firstIndex(where: { $0.id == activeTransactionId }) {
      var nextIndex = transactions.index(after: index)
      if nextIndex == transactions.endIndex {
        nextIndex = 0
      }
      activeTransactionId = transactions[nextIndex].id
    } else {
      activeTransactionId = transactions.first!.id
    }
  }

  func rejectAllTransactions() {
    for transaction in transactions {
      reject(transaction: transaction, completion: { _ in })
    }
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
    self.state.gasValue =
      gasFormatter.decimalString(
        for: gasFee.removingHexPrefix,
        radix: .hex,
        decimals: Int(selectedChain.decimals)
      ) ?? ""
    if !self.state.gasValue.isEmpty {
      rpcService.chainId(.eth) { [weak self] chainId in
        self?.rpcService.balance(transaction.fromAddress, coin: .eth, chainId: chainId) { [weak self] weiBalance, status, _ in
        guard let self = self, status == .success else { return }
        let formatter = WeiFormatter(decimalFormatStyle: .balance)
        guard
          let decimalString = formatter.decimalString(
            for: weiBalance.removingHexPrefix, radix: .hex, decimals: Int(self.selectedChain.decimals)
          ),
          let value = BDouble(decimalString), let gasValue = BDouble(self.state.gasValue),
          value > gasValue
        else {
          self.state.isBalanceSufficient = false
          return
        }
        self.state.isBalanceSufficient = true
      }
    }
  }
  }

  func fetchDetails(for transaction: BraveWallet.TransactionInfo) {
    state = .init()  // Reset state
    isLoading = true

    walletService.selectedCoin { [weak self] coin in
      guard let self = self else { return }
      self.rpcService.network(coin) { selectedChain in
        let chainId = selectedChain.chainId
        self.selectedChain = selectedChain
        
        self.state.gasSymbol = selectedChain.symbol
        self.updateGasValue(for: transaction)
        
        let formatter = WeiFormatter(decimalFormatStyle: .balance)
        let txValue = transaction.ethTxValue.removingHexPrefix
        
        self.blockchainRegistry.allTokens(chainId, coin: selectedChain.coin) { tokens in
          self.walletService.userAssets(chainId, coin: selectedChain.coin) { userAssets in
            let allTokens = tokens + userAssets.filter { asset in
              // Only get custom tokens
              !tokens.contains(where: { $0.contractAddress(in: selectedChain).caseInsensitiveCompare(asset.contractAddress) == .orderedSame })
            }
            
            switch transaction.txType {
            case .erc20Approve:
              // Find token in args
              let contractAddress = transaction.txDataUnion.ethTxData1559?.baseData.to ?? ""
              if let token = allTokens.first(where: {
                $0.contractAddress(in: selectedChain).caseInsensitiveCompare(contractAddress) == .orderedSame
              }) {
                self.state.symbol = token.symbol
                if let approvalValue = transaction.txArgs[safe: 1] {
                  if approvalValue.caseInsensitiveCompare(WalletConstants.MAX_UINT256) == .orderedSame {
                    self.state.value = Strings.Wallet.editPermissionsApproveUnlimited
                  } else {
                    self.state.value = formatter.decimalString(for: approvalValue.removingHexPrefix, radix: .hex, decimals: Int(token.decimals)) ?? ""
                  }
                }
                self.rpcService.erc20TokenAllowance(
                  token.contractAddress(in: selectedChain),
                  ownerAddress: transaction.fromAddress,
                  spenderAddress: transaction.txArgs[safe: 0] ?? "") { allowance, status, _ in
                    self.state.currentAllowance = formatter.decimalString(for: allowance.removingHexPrefix, radix: .hex, decimals: Int(token.decimals)) ?? ""
                  }
              }
              if let proposedAllowance = transaction.txArgs[safe: 1] {
                self.state.isUnlimitedApprovalRequested = proposedAllowance.caseInsensitiveCompare(WalletConstants.MAX_UINT256) == .orderedSame
              }
            case .erc20Transfer:
              if let token = allTokens.first(where: {
                $0.contractAddress(in: selectedChain).caseInsensitiveCompare(transaction.ethTxToAddress) == .orderedSame
              }) {
                self.state.symbol = token.symbol
                let value = transaction.txArgs[1].removingHexPrefix
                self.state.value = formatter.decimalString(for: value, radix: .hex, decimals: Int(token.decimals)) ?? ""
              }
            case .ethSend, .other, .erc721TransferFrom, .erc721SafeTransferFrom:
              self.state.symbol = selectedChain.symbol
              self.state.value = formatter.decimalString(for: txValue, radix: .hex, decimals: Int(selectedChain.decimals)) ?? ""
            case .solanaSystemTransfer,
                .solanaSplTokenTransfer,
                .solanaSplTokenTransferWithAssociatedTokenAccountCreation:
              break
            @unknown default:
              break
            }
            self.state.originInfo = transaction.originInfo
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
        self.state.fiat = currencyFormatter.string(from: NSNumber(value: value)) ?? ""
        let gasValue = (Double(self.state.gasValue) ?? 0.0) * gasRatio
        self.state.gasFiat = currencyFormatter.string(from: NSNumber(value: gasValue)) ?? ""
        self.state.totalFiat = currencyFormatter.string(from: NSNumber(value: value + gasValue)) ?? ""
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
        toAssets: [currencyFormatter.currencyCode],
        timeframe: .oneDay
      ) { [weak self] success, prices in
        // `success` only refers to finding _all_ prices and if even 1 of N prices
        // fail to fetch it will be false
        guard let self = self,
          self.state.symbol.caseInsensitiveCompare(symbol) == .orderedSame
        else {
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
    walletService.selectedCoin { [weak self] coin in
      guard let self = self else { return }
      self.keyringService.keyringInfo(coin.keyringId) { keyring in
        var pendingTransactions: [BraveWallet.TransactionInfo] = []
        let group = DispatchGroup()
        for info in keyring.accountInfos {
          group.enter()
          self.txService.allTransactionInfo(info.coin, from: info.address) { tx in
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
  }

  func confirm(transaction: BraveWallet.TransactionInfo, completion: @escaping (_ error: String?) -> Void) {
    txService.approveTransaction(transaction.coin, txMetaId: transaction.id) { success, error, message in
      completion(success ? nil : message)
    }
  }

  func reject(transaction: BraveWallet.TransactionInfo, completion: @escaping (Bool) -> Void) {
    txService.rejectTransaction(transaction.coin, txMetaId: transaction.id) { success in
      completion(success)
    }
  }

  func updateGasFeeAndLimits(
    for transaction: BraveWallet.TransactionInfo,
    maxPriorityFeePerGas: String,
    maxFeePerGas: String,
    gasLimit: String,
    completion: ((Bool) -> Void)? = nil
  ) {
    assert(
      transaction.isEIP1559Transaction,
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
    assert(
      !transaction.isEIP1559Transaction,
      "Use updateGasFeeAndLimits(for:maxPriorityFeePerGas:maxFeePerGas:gasLimit:) for EIP-1559 transactions")
    ethTxManagerProxy.setGasPriceAndLimitForUnapprovedTransaction(
      transaction.id,
      gasPrice: gasPrice,
      gasLimit: gasLimit
    ) { success in
      completion?(success)
    }
  }

  func prepare(completion: (() -> Void)? = nil) {
    let dispatchGroup = DispatchGroup()
    dispatchGroup.enter()
    fetchTransactions { [weak self] in
      defer { dispatchGroup.leave() }
      guard let self = self,
        let firstTx = self.transactions.first
      else { return }
      self.activeTransactionId = firstTx.id
      self.fetchGasEstimation1559()
    }
    dispatchGroup.enter()
    fetchTokens() { _ in
      dispatchGroup.leave()
    }
    dispatchGroup.notify(queue: .main) {
      completion?()
    }
  }

  func editNonce(
    for transaction: BraveWallet.TransactionInfo,
    nonce: String,
    completion: @escaping ((Bool) -> Void)
  ) {
    ethTxManagerProxy.setNonceForUnapprovedTransaction(transaction.id, nonce: nonce) { success in
      // not going to refresh unapproved transactions since the tx observer will be
      // notified `onTransactionStatusChanged` and `onUnapprovedTxUpdated`
      // `transactions` list will be refreshed there.
      completion(success)
    }
  }
  
  func editAllowance(
    txMetaId: String,
    spenderAddress: String,
    amount: String,
    completion: @escaping (Bool) -> Void
  ) {
    ethTxManagerProxy.makeErc20ApproveData(spenderAddress, amount: amount) { [weak self] success, data in
      guard let self = self else { return }
      if !success {
        completion(false)
        return
      }
      self.ethTxManagerProxy.setDataForUnapprovedTransaction(txMetaId, data: data) { success in
        // not going to refresh unapproved transactions since the tx observer will be
        // notified `onTransactionStatusChanged` and `onUnapprovedTxUpdated`
        // `transactions` list will be refreshed there.
        completion(success)
      }
    }
  }
  
  func fetchTokens(completion: (([BraveWallet.BlockchainToken]) -> Void)? = nil) {
    walletService.selectedCoin { [weak self] coin in
      guard let self = self else { return }
      self.rpcService.network(coin) { network in
        self.blockchainRegistry.allTokens(network.chainId, coin: network.coin) { tokens in
          self.allTokens = tokens
          completion?(tokens)
        }
      }
    }
  }

  func token(for contractAddress: String, in network: BraveWallet.NetworkInfo) -> BraveWallet.BlockchainToken? {
    allTokens.first(where: {
      $0.contractAddress(in: network).caseInsensitiveCompare(contractAddress) == .orderedSame
    })
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

extension TransactionConfirmationStore: BraveWalletBraveWalletServiceObserver {
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
