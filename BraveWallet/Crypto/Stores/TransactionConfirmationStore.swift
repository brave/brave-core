// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import BigNumber
import Strings

public class TransactionConfirmationStore: ObservableObject {
  /// The value that are being sent/swapped/approved in this transaction
  @Published var value: String = ""
  /// The symbol of token that are being send/swapped/approved in this transaction
  @Published var symbol: String = ""
  /// The fiat value of `value`
  @Published var fiat: String = ""
  /// The network short name that this transaction is made on
  @Published var networkShortChainName: String = ""
  /// The gas value for this transaction
  @Published var gasValue: String = ""
  /// The symbol of the gas token for this transaction
  @Published var gasSymbol: String = ""
  /// The fiat value of `gasValue`
  @Published var gasFiat: String = ""
  /// The price for gas token
  @Published var gasAssetRatio: Double = 0.0
  /// The combine fiat value of `value` and `gasValue`
  @Published var totalFiat: String = ""
  /// If transaction fee payer account has enough gas token balance
  @Published var isBalanceSufficient: Bool = true
  /// If user requests unlimited approval for a erc20 token
  @Published var isUnlimitedApprovalRequested: Bool = false
  /// If this Solana transaction does not have an associated token account created
  @Published var isSolTokenTransferWithAssociatedTokenAccountCreation: Bool = false
  /// The current erc20 token allowance
  @Published var currentAllowance: String = ""
  /// The proposed arc20 token allowance
  @Published var proposedAllowance: String = ""
  /// The details of this transaction
  @Published var transactionDetails: String = ""
  /// The gas esitimation for this eip1559 transaction
  @Published var eip1559GasEstimation: BraveWallet.GasEstimation1559?
  /// The origin info of this transaction
  @Published var originInfo: BraveWallet.OriginInfo?
  /// This is an id for the unppproved transaction that is currently displayed on screen
  @Published var activeTransactionId: BraveWallet.TransactionInfo.ID = "" {
    didSet {
      if let tx = transactions.first(where: { $0.id == activeTransactionId }) {
        updateTransaction(with: tx)
      } else if let firstTx = transactions.first {
        updateTransaction(with: firstTx)
      }
    }
  }
  @Published private(set) var currencyCode: String = CurrencyCode.usd.code {
    didSet {
      currencyFormatter.currencyCode = currencyCode
      updateTransaction(with: activeTransaction)
    }
  }
  /// This is a list of all unpproved transactions iterated through all the accounts for the current keyring
  private(set) var transactions: [BraveWallet.TransactionInfo] = []

  let currencyFormatter: NumberFormatter = .usdCurrencyFormatter
    .then {
      $0.minimumFractionDigits = 2
      $0.maximumFractionDigits = 6
    }
  
  private var activeTransaction: BraveWallet.TransactionInfo {
    transactions.first(where: { $0.id == activeTransactionId }) ?? (transactions.first ?? .init())
  }
  
  private(set) var activeParsedTransaction: ParsedTransaction = .init()
  private var activeTransactionDetails: String {
    if activeParsedTransaction.transaction.coin == .sol {
      return String.localizedStringWithFormat(Strings.Wallet.inputDataPlaceholderSolana, activeParsedTransaction.transaction.txType.rawValue)
    } else {
      if activeParsedTransaction.transaction.txArgs.isEmpty {
        let data = activeParsedTransaction.transaction.ethTxData
          .map { byte in
            String(format: "%02X", byte.uint8Value)
          }
          .joined()
        if data.isEmpty {
          return Strings.Wallet.inputDataPlaceholder
        }
        return "0x\(data)"
      } else {
        return zip(activeParsedTransaction.transaction.txParams, activeParsedTransaction.transaction.txArgs)
          .map { (param, arg) in
            "\(param): \(arg)"
          }
          .joined(separator: "\n\n")
      }
    }
  }
  
  private let assetRatioService: BraveWalletAssetRatioService
  private let rpcService: BraveWalletJsonRpcService
  private let txService: BraveWalletTxService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let walletService: BraveWalletBraveWalletService
  private let ethTxManagerProxy: BraveWalletEthTxManagerProxy
  private let keyringService: BraveWalletKeyringService
  private let solTxManagerProxy: BraveWalletSolanaTxManagerProxy
  private var selectedChain: BraveWallet.NetworkInfo = .init()

  init(
    assetRatioService: BraveWalletAssetRatioService,
    rpcService: BraveWalletJsonRpcService,
    txService: BraveWalletTxService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    walletService: BraveWalletBraveWalletService,
    ethTxManagerProxy: BraveWalletEthTxManagerProxy,
    keyringService: BraveWalletKeyringService,
    solTxManagerProxy: BraveWalletSolanaTxManagerProxy
  ) {
    self.assetRatioService = assetRatioService
    self.rpcService = rpcService
    self.txService = txService
    self.blockchainRegistry = blockchainRegistry
    self.walletService = walletService
    self.ethTxManagerProxy = ethTxManagerProxy
    self.keyringService = keyringService
    self.solTxManagerProxy = solTxManagerProxy

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
  
  @MainActor func prepare() async {
    transactions = await fetchTransactions()
    if !transactions.contains(where: { $0.id == activeTransactionId }) {
      self.activeTransactionId = transactions.first?.id ?? ""
    }
    let coinsForTransactions: Set<BraveWallet.CoinType> = .init(transactions.map(\.coin))
    for coin in coinsForTransactions {
      let network = await rpcService.network(coin)
      let userVisibleTokens = await walletService.userAssets(network.chainId, coin: coin)
      await fetchAssetRatios(for: userVisibleTokens)
    }
  }
  
  func updateTransaction(
    with transaction: BraveWallet.TransactionInfo,
    shouldFetchCurrentAllowance: Bool = true,
    shouldFetchGasTokenBalance: Bool = true
  ) {
    Task { @MainActor in
      clearTrasactionInfoBeforeUpdate()
      
      let coin = transaction.coin
      let keyring = await keyringService.keyringInfo(coin.keyringId)
      let network = await rpcService.network(coin)
      let allTokens = await blockchainRegistry.allTokens(network.chainId, coin: coin)
      let userVisibleTokens = await walletService.userAssets(network.chainId, coin: coin)
      
      var solEstimatedTxFee: UInt64?
      if transaction.coin == .sol {
        (solEstimatedTxFee, _, _) = await solTxManagerProxy.estimatedTxFee(transaction.id)
      }
      
      if transaction.isEIP1559Transaction {
        eip1559GasEstimation = transaction.txDataUnion.ethTxData1559?.gasEstimation
      }
      
      guard let parsedTransaction = transaction.parsedTransaction(
        network: network,
        accountInfos: keyring.accountInfos,
        visibleTokens: userVisibleTokens,
        allTokens: allTokens,
        assetRatios: assetRatios,
        solEstimatedTxFee: solEstimatedTxFee,
        currencyFormatter: currencyFormatter
      ) else {
        return
      }
      activeParsedTransaction = parsedTransaction
      
      await fetchActiveTransactionDetails(
        keyring: keyring,
        network: network,
        allTokens: allTokens,
        shouldFetchCurrentAllowance: shouldFetchCurrentAllowance,
        shouldFetchGasTokenBalance: shouldFetchGasTokenBalance
      )
    }
  }
  
  private func clearTrasactionInfoBeforeUpdate() {
    // clear fields that could have dynamic async changes
    fiat = ""
    gasFiat = ""
    gasAssetRatio = 0.0
    totalFiat = ""
    currentAllowance = ""
    isBalanceSufficient = true
    isSolTokenTransferWithAssociatedTokenAccountCreation = false
    isUnlimitedApprovalRequested = false
  }
  
  private var assetRatios: [String: Double] = [:]
  private var currentAllowanceCache: [String: String] = [:]
  private var gasTokenBalanceCache: [String: Double] = [:]
  
  @MainActor private func fetchAssetRatios(for userVisibleTokens: [BraveWallet.BlockchainToken]) async {
    let priceResult = await assetRatioService.priceWithIndividualRetry(
      userVisibleTokens.map { $0.symbol.lowercased() },
      toAssets: [currencyFormatter.currencyCode],
      timeframe: .oneDay
    )
    let newAssetRatios = priceResult.assetPrices.reduce(into: [String: Double]()) {
      $0[$1.fromAsset] = Double($1.price)
    }
    assetRatios.merge(with: newAssetRatios)
    updateTransaction(with: activeTransaction, shouldFetchCurrentAllowance: false, shouldFetchGasTokenBalance: false)
  }
  
  @MainActor func fetchCurrentAllowance(
    for parsedTransaction: ParsedTransaction,
    allTokens: [BraveWallet.BlockchainToken]
  ) async {
    guard case let .ethErc20Approve(details) = parsedTransaction.details else {
      return
    }
    
    let formatter = WeiFormatter(decimalFormatStyle: .balance)
    let (allowance, _, _) = await rpcService.erc20TokenAllowance(details.token.contractAddress(in: selectedChain), ownerAddress: parsedTransaction.fromAddress, spenderAddress: details.spenderAddress)
    let allowanceString = formatter.decimalString(for: allowance.removingHexPrefix, radix: .hex, decimals: Int(details.token.decimals)) ?? ""
    currentAllowanceCache[parsedTransaction.transaction.id] = allowanceString
    updateTransaction(with: activeTransaction, shouldFetchCurrentAllowance: false, shouldFetchGasTokenBalance: false)
  }
  
  @MainActor func fetchGasTokenBalance(
    token: BraveWallet.BlockchainToken,
    account: BraveWallet.AccountInfo
  ) async {
    if let gasTokenBalance = await rpcService.balance(for: token, in: account) {
      gasTokenBalanceCache["\(token.symbol)\(account.address)"] = gasTokenBalance
      updateTransaction(with: activeTransaction, shouldFetchCurrentAllowance: false, shouldFetchGasTokenBalance: false)
    }
  }
  
  @MainActor func fetchActiveTransactionDetails(
    keyring: BraveWallet.KeyringInfo,
    network: BraveWallet.NetworkInfo,
    allTokens: [BraveWallet.BlockchainToken],
    shouldFetchCurrentAllowance: Bool,
    shouldFetchGasTokenBalance: Bool
  ) async {
    originInfo = activeParsedTransaction.transaction.originInfo
    transactionDetails = activeTransactionDetails
    networkShortChainName = network.shortChainName
    
    switch activeParsedTransaction.details {
    case let .ethSend(details),
      let .erc20Transfer(details),
      let .solSystemTransfer(details),
      let .solSplTokenTransfer(details):
      symbol = details.fromToken.symbol
      value = details.fromAmount
      fiat = details.fromFiat ?? ""
      
      isSolTokenTransferWithAssociatedTokenAccountCreation = activeParsedTransaction.transaction.txType == .solanaSplTokenTransferWithAssociatedTokenAccountCreation
      
      if let gasFee = details.gasFee {
        gasValue = gasFee.fee
        gasFiat = gasFee.fiat
        gasSymbol = activeParsedTransaction.networkSymbol
        gasAssetRatio = assetRatios[activeParsedTransaction.networkSymbol.lowercased(), default: 0]
        
        if let gasBalance = gasTokenBalanceCache["\(network.nativeToken.symbol)\(activeParsedTransaction.fromAddress)"] {
          if let gasValue = BDouble(gasFee.fee),
             BDouble(gasBalance) > gasValue {
            isBalanceSufficient = true
          } else {
            isBalanceSufficient = false
          }
        } else if shouldFetchGasTokenBalance {
          if let account = keyring.accountInfos.first(where: { $0.address == activeParsedTransaction.fromAddress }) {
            await fetchGasTokenBalance(token: network.nativeToken, account: account)
          }
        }
      }
      
      totalFiat = totalFiat(value: value, tokenSymbol: symbol, gasValue: gasValue, gasSymbol: gasSymbol, assetRatios: assetRatios, currencyFormatter: currencyFormatter)
      
    case let .ethErc20Approve(details):
      value = details.approvalAmount
      symbol = details.token.symbol
      proposedAllowance = details.approvalValue
      isUnlimitedApprovalRequested = details.isUnlimited
      
      if let gasFee = details.gasFee {
        gasValue = gasFee.fee
        gasFiat = gasFee.fiat
        gasSymbol = activeParsedTransaction.networkSymbol
        gasAssetRatio = assetRatios[activeParsedTransaction.networkSymbol.lowercased(), default: 0]
        
        if let gasBalance = gasTokenBalanceCache["\(network.nativeToken.symbol)\(activeParsedTransaction.fromAddress)"] {
          if let gasValue = BDouble(gasFee.fee),
             BDouble(gasBalance) > gasValue {
            isBalanceSufficient = true
          } else {
            isBalanceSufficient = false
          }
        } else if shouldFetchGasTokenBalance {
          if let account = keyring.accountInfos.first(where: { $0.address == activeParsedTransaction.fromAddress }) {
            await fetchGasTokenBalance(token: network.nativeToken, account: account)
          }
        }
      }
      
      totalFiat = gasFiat
      
      if let cachedAllowance = self.currentAllowanceCache[activeParsedTransaction.transaction.id] {
        currentAllowance = cachedAllowance
      } else if shouldFetchCurrentAllowance {
        // async fetch current allowance to not delay updating transaction details (will re-parse after fetching current allowance)
        await fetchCurrentAllowance(for: activeParsedTransaction, allTokens: allTokens)
      }
    case let .ethSwap(details):
      symbol = details.fromToken?.symbol ?? ""
      value = details.fromAmount
      fiat = details.fromFiat ?? ""
      
      if let gasFee = details.gasFee {
        gasValue = gasFee.fee
        gasFiat = gasFee.fiat
        gasSymbol = activeParsedTransaction.networkSymbol
        gasAssetRatio = assetRatios[activeParsedTransaction.networkSymbol.lowercased(), default: 0]
        
        if let gasBalance = gasTokenBalanceCache["\(network.nativeToken.symbol)\(activeParsedTransaction.fromAddress)"] {
          if let gasValue = BDouble(gasFee.fee),
             BDouble(gasBalance) > gasValue {
            isBalanceSufficient = true
          } else {
            isBalanceSufficient = false
          }
        } else if shouldFetchGasTokenBalance {
          if let account = keyring.accountInfos.first(where: { $0.address == activeParsedTransaction.fromAddress }) {
            await fetchGasTokenBalance(token: network.nativeToken, account: account)
          }
        }
        
        totalFiat = totalFiat(value: value, tokenSymbol: symbol, gasValue: gasValue, gasSymbol: gasSymbol, assetRatios: assetRatios, currencyFormatter: currencyFormatter)
      }
    case let .erc721Transfer(details):
      symbol = details.fromToken?.symbol ?? ""
      value = details.fromAmount
    case .other:
      break
    }
  }
  
  private func totalFiat(
    value: String,
    tokenSymbol: String,
    gasValue: String,
    gasSymbol: String,
    assetRatios: [String: Double],
    currencyFormatter: NumberFormatter
  ) -> String {
    let ratio = assetRatios[tokenSymbol.lowercased(), default: 0]
    let gasRatio = assetRatios[gasSymbol.lowercased(), default: 0]
    let amount = (Double(value) ?? 0.0) * ratio
    let gasAmount = (Double(gasValue) ?? 0.0) * gasRatio
    let totalFiat = currencyFormatter.string(from: NSNumber(value: amount + gasAmount)) ?? "$0.00"
    return totalFiat
  }

  @MainActor private func fetchTransactions() async -> [BraveWallet.TransactionInfo] {
    let allKeyrings = await keyringService.keyrings(for: WalletConstants.supportedCoinTypes)
    
    return await txService.pendingTransactions(for: allKeyrings)
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
}

extension TransactionConfirmationStore: BraveWalletTxServiceObserver {
  public func onNewUnapprovedTx(_ txInfo: BraveWallet.TransactionInfo) {
    // won't have any new unapproved tx being added if you on tx confirmation panel
  }
  public func onTransactionStatusChanged(_ txInfo: BraveWallet.TransactionInfo) {
    Task { @MainActor in
      // once we come here. it means user either rejects or confirms a transaction
      // we will need to refresh the transaction list as well as
      // update the `activeTransactionId` to update the UI
      let indexOfChangedTx = transactions.firstIndex(where: { $0.id == txInfo.id }) ?? 0
      let newIndex = indexOfChangedTx > 0 ? indexOfChangedTx - 1 : 0
      
      transactions = await fetchTransactions()
      activeTransactionId = transactions[safe: newIndex]?.id ?? transactions.first?.id ?? ""
    }
  }
  public func onUnapprovedTxUpdated(_ txInfo: BraveWallet.TransactionInfo) {
    Task { @MainActor in
      // refresh the unapproved transaction list, as well as tx details UI
      transactions = await fetchTransactions()
      
      if !transactions.contains(where: { $0.id == activeTransactionId }) {
        activeTransactionId = transactions.first?.id ?? ""
      }
      if activeTransactionId == txInfo.id {
        updateTransaction(with: txInfo)
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
