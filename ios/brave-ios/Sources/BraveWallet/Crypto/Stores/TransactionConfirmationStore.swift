// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BigNumber
import BraveCore
import Combine
import Foundation
import Preferences
import Strings

public class TransactionConfirmationStore: ObservableObject, WalletObserverStore {
  /// The value that are being sent/swapped/approved in this transaction
  @Published var value: String = ""
  /// The symbol of token that are being send/swapped/approved in this transaction
  @Published var symbol: String = ""
  /// The fiat value of `value`
  @Published var fiat: String = ""
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
  /// The gas premium for filcoin transaction
  @Published var filTxGasPremium: String?
  /// The gas limit for filcoin transaction
  @Published var filTxGasLimit: String?
  /// The gas fee cap for filcoin transaction
  @Published var filTxGasFeeCap: String?
  /// The origin info of this transaction
  @Published var originInfo: BraveWallet.OriginInfo?
  /// This is an id for the unppproved transaction that is currently displayed on screen
  @Published var activeTransactionId: BraveWallet.TransactionInfo.ID = "" {
    didSet {
      if let tx = allTxs.first(where: { $0.id == activeTransactionId }) {
        updateTransaction(with: tx)
        activeTxStatus = tx.txStatus
      } else if let firstTx = unapprovedTxs.first {
        updateTransaction(with: firstTx)
        activeTxStatus = firstTx.txStatus
      } else {
        activeTxStatus = .error
      }
    }
  }
  @Published private(set) var currencyCode: String = CurrencyCode.usd.code {
    didSet {
      currencyFormatter.currencyCode = currencyCode
      guard currencyCode != oldValue else { return }
      updateTransaction(with: activeTransaction)
    }
  }
  @Published var activeTxStatus: BraveWallet.TransactionStatus = .unapproved {
    didSet {
      if isTxSubmitting == true, activeTxStatus != .approved {
        isTxSubmitting = false
      }
    }
  }
  /// Indicates Tx is being submitted. This value will be set to `true` after users click `Confirm` button
  @Published var isTxSubmitting: Bool = false
  /// Indicates if the to address is a contract address that should link to block explorer
  @Published var isContractAddress: Bool = false

  /// All transactions with any kind of status of all the accounts for all supported keyrings
  @Published var allTxs: [BraveWallet.TransactionInfo] = []
  /// This is a list of all unpproved transactions iterated through all the accounts for all supported keyrings
  var unapprovedTxs: [BraveWallet.TransactionInfo] {
    return allTxs.filter { $0.txStatus == .unapproved }
  }
  var isReadyToBeDismissed: Bool {
    return unapprovedTxs.isEmpty && activeTransactionId.isEmpty
  }
  /// This is a map between transaction id and its error happened during transaction submitting
  var transactionProviderErrorRegistry: [String: TransactionProviderError] = [:]

  let currencyFormatter: NumberFormatter = .usdCurrencyFormatter
    .then {
      $0.minimumFractionDigits = 2
      $0.maximumFractionDigits = 6
    }

  private var activeTransaction: BraveWallet.TransactionInfo {
    unapprovedTxs.first(where: { $0.id == activeTransactionId }) ?? (unapprovedTxs.first ?? .init())
  }

  private(set) var activeParsedTransaction: ParsedTransaction = .init()
  private var activeTransactionDetails: String {
    if activeParsedTransaction.transaction.coin == .sol {
      return String.localizedStringWithFormat(
        Strings.Wallet.inputDataPlaceholderTx,
        activeParsedTransaction.transaction.txType.localizedDescription
      )
    } else if activeParsedTransaction.transaction.coin == .btc,
      let btcTxData = activeParsedTransaction.transaction.txDataUnion.btcTxData
    {
      let inputs = btcTxData.inputs.enumerated().map { (index, input) in
        let inputLine = "\(Strings.Wallet.inputLabel): \(index)"
        let valueLine = "\(Strings.Wallet.valueLabel): \(input.value)"
        let addressLines = "\(Strings.Wallet.addressLabel):\n\(input.address)"
        return [inputLine, valueLine, addressLines].joined(separator: "\n")
      }.joined(separator: "\n\n")
      let outputs = btcTxData.outputs.enumerated().map { (index, output) in
        let outputLine = "\(Strings.Wallet.outputLabel): \(index)"
        let valueLine = "\(Strings.Wallet.valueLabel): \(output.value)"
        let addressLines = "\(Strings.Wallet.addressLabel):\n\(output.address)"
        return [outputLine, valueLine, addressLines].joined(separator: "\n")
      }.joined(separator: "\n\n")
      return inputs + "\n\n" + outputs
    } else {
      let functionType = String.localizedStringWithFormat(
        Strings.Wallet.inputDataPlaceholderTx,
        activeParsedTransaction.transaction.txType.localizedDescription
      )
      if activeParsedTransaction.transaction.txArgs.isEmpty {
        let data = activeParsedTransaction.transaction.ethTxData
          .map { byte in
            String(format: "%02X", byte.uint8Value)
          }
          .joined()
        if data.isEmpty {
          return functionType + "\n\n" + Strings.Wallet.inputDataPlaceholder
        }
        return functionType + "\n\n" + "0x\(data)"
      } else {
        return functionType
          + "\n\n"
          + zip(
            activeParsedTransaction.transaction.txParams,
            activeParsedTransaction.transaction.txArgs
          )
          .map { (param, arg) in
            "\(param): \(arg)"
          }
          .joined(separator: "\n\n")
      }
    }
  }
  /// A cache of networks for the supported coin types.
  private var allNetworks: [BraveWallet.NetworkInfo] = []

  private let assetRatioService: BraveWalletAssetRatioService
  private let rpcService: BraveWalletJsonRpcService
  private let txService: BraveWalletTxService
  private let blockchainRegistry: BraveWalletBlockchainRegistry
  private let walletService: BraveWalletBraveWalletService
  private let ethTxManagerProxy: BraveWalletEthTxManagerProxy
  private let keyringService: BraveWalletKeyringService
  private let solTxManagerProxy: BraveWalletSolanaTxManagerProxy
  private let bitcoinWalletService: BraveWalletBitcoinWalletService
  private let ipfsApi: IpfsAPI
  private let assetManager: WalletUserAssetManagerType
  private var selectedChain: BraveWallet.NetworkInfo = .init()
  private var txServiceObserver: TxServiceObserver?
  private var walletServiceObserver: WalletServiceObserver?

  var isObserving: Bool {
    txServiceObserver != nil && walletServiceObserver != nil
  }

  init(
    assetRatioService: BraveWalletAssetRatioService,
    rpcService: BraveWalletJsonRpcService,
    txService: BraveWalletTxService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    walletService: BraveWalletBraveWalletService,
    ethTxManagerProxy: BraveWalletEthTxManagerProxy,
    keyringService: BraveWalletKeyringService,
    solTxManagerProxy: BraveWalletSolanaTxManagerProxy,
    bitcoinWalletService: BraveWalletBitcoinWalletService,
    ipfsApi: IpfsAPI,
    userAssetManager: WalletUserAssetManagerType
  ) {
    self.assetRatioService = assetRatioService
    self.rpcService = rpcService
    self.txService = txService
    self.blockchainRegistry = blockchainRegistry
    self.walletService = walletService
    self.ethTxManagerProxy = ethTxManagerProxy
    self.keyringService = keyringService
    self.solTxManagerProxy = solTxManagerProxy
    self.bitcoinWalletService = bitcoinWalletService
    self.ipfsApi = ipfsApi
    self.assetManager = userAssetManager

    self.setupObservers()

    walletService.defaultBaseCurrency { [self] currencyCode in
      self.currencyCode = currencyCode
    }
  }

  func tearDown() {
    txServiceObserver = nil
    walletServiceObserver = nil
    txDetailsStore?.tearDown()
  }

  func setupObservers() {
    guard !isObserving else { return }
    self.assetManager.addUserAssetDataObserver(self)
    self.txServiceObserver = TxServiceObserver(
      txService: txService,
      _onNewUnapprovedTx: { _ in
        // won't have any new unapproved tx being added if you on tx confirmation panel
      },
      _onUnapprovedTxUpdated: { [weak self] txInfo in
        self?.onUnapprovedTxUpdated(txInfo)
      },
      _onTransactionStatusChanged: { [weak self] txInfo in
        self?.onTransactionStatusChanged(txInfo)
      }
    )
    self.walletServiceObserver = WalletServiceObserver(
      walletService: walletService,
      _onDefaultBaseCurrencyChanged: { [weak self] currency in
        self?.currencyCode = currency
      }
    )
  }

  func nextTransaction() {
    if let index = unapprovedTxs.firstIndex(where: { $0.id == activeTransactionId }) {
      var nextIndex = unapprovedTxs.index(after: index)
      if nextIndex == unapprovedTxs.endIndex {
        nextIndex = 0
      }
      activeTransactionId = unapprovedTxs[nextIndex].id
    } else {
      activeTransactionId = unapprovedTxs.first!.id
    }
  }

  @MainActor func rejectAllTransactions() async -> Bool {
    var allRejectsSucceeded = true
    for transaction in unapprovedTxs {
      let success = await reject(transaction: transaction)
      if !success {
        allRejectsSucceeded = false
      }
    }
    return allRejectsSucceeded
  }

  @MainActor func prepare() async {
    allNetworks = await rpcService.allNetworksForSupportedCoins()
    allTxs = await fetchAllTransactions()
    if !unapprovedTxs.contains(where: { $0.id == activeTransactionId }) {
      self.activeTransactionId = unapprovedTxs.first?.id ?? ""
    }
    let transactionNetworks: [BraveWallet.NetworkInfo] = Set(allTxs.map(\.chainId))
      .compactMap { chainId in allNetworks.first(where: { $0.chainId == chainId }) }
    for network in transactionNetworks {
      let userAssets = await assetManager.getAllUserAssetsInNetworkAssets(
        networks: [network],
        includingUserDeleted: true
      ).flatMap { $0.tokens }
      await fetchAssetRatios(for: userAssets)
    }
    await fetchUnknownTokens(for: unapprovedTxs)
    await fetchSolEstimatedTxFees(for: unapprovedTxs)
  }

  func updateTransaction(
    with transaction: BraveWallet.TransactionInfo,
    shouldFetchCurrentAllowance: Bool = true,
    shouldFetchGasTokenBalance: Bool = true
  ) {
    Task { @MainActor in
      clearTrasactionInfoBeforeUpdate()

      let coin = transaction.coin
      let allAccountsForCoin = await keyringService.allAccounts().accounts.filter {
        $0.coin == coin
      }
      if !allNetworks.contains(where: { $0.chainId == transaction.chainId }) {
        allNetworks = await rpcService.allNetworksForSupportedCoins()
      }
      guard let network = allNetworks.first(where: { $0.chainId == transaction.chainId }) else {
        // default `BraveWallet.TransactionInfo()` has empty chainId
        if !transaction.chainId.isEmpty {
          // Transactions should be removed if their network is removed
          // https://github.com/brave/brave-browser/issues/30234
          assertionFailure(
            "The NetworkInfo for the transaction's chainId (\(transaction.chainId)) is unavailable"
          )
        }
        return
      }
      let allTokens =
        await blockchainRegistry.allTokens(chainId: network.chainId, coin: coin) + tokenInfoCache
      let userAssets = await assetManager.getAllUserAssetsInNetworkAssets(
        networks: [network],
        includingUserDeleted: true
      ).flatMap { $0.tokens }
      let solEstimatedTxFee: UInt64? = solEstimatedTxFeeCache[transaction.id]

      if transaction.isEIP1559Transaction {
        eip1559GasEstimation = transaction.txDataUnion.ethTxData1559?.gasEstimation
      }

      guard
        let parsedTransaction = transaction.parsedTransaction(
          allNetworks: allNetworks,
          accountInfos: allAccountsForCoin,
          userAssets: userAssets,
          allTokens: allTokens,
          assetRatios: assetRatios,
          nftMetadata: [:],
          solEstimatedTxFee: solEstimatedTxFee,
          currencyFormatter: currencyFormatter
        )
      else {
        return
      }
      activeParsedTransaction = parsedTransaction
      updateIsContractAddress()

      await fetchActiveTransactionDetails(
        accounts: allAccountsForCoin,
        network: network,
        allTokens: allTokens,
        shouldFetchCurrentAllowance: shouldFetchCurrentAllowance,
        shouldFetchGasTokenBalance: shouldFetchGasTokenBalance
      )
    }
  }

  private var txDetailsStore: TransactionDetailsStore?
  func activeTxDetailsStore() -> TransactionDetailsStore {
    let tx = allTxs.first { $0.id == activeTransactionId } ?? activeParsedTransaction.transaction
    let parsedTransaction: ParsedTransaction?
    if activeParsedTransaction.transaction.id == tx.id {
      parsedTransaction = activeParsedTransaction
    } else {
      parsedTransaction = nil
    }
    let txDetailsStore = TransactionDetailsStore(
      transaction: tx,
      parsedTransaction: parsedTransaction,
      keyringService: keyringService,
      walletService: walletService,
      rpcService: rpcService,
      assetRatioService: assetRatioService,
      blockchainRegistry: blockchainRegistry,
      txService: txService,
      solanaTxManagerProxy: solTxManagerProxy,
      ipfsApi: ipfsApi,
      userAssetManager: assetManager
    )
    self.txDetailsStore = txDetailsStore
    return txDetailsStore
  }

  func closeTxDetailsStore() {
    self.txDetailsStore?.tearDown()
    self.txDetailsStore = nil
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
    isContractAddress = false
    // Filecoin Tx
    filTxGasPremium = nil
    filTxGasLimit = nil
    filTxGasFeeCap = nil
  }

  private var assetRatios: [String: Double] = [:]
  private var currentAllowanceCache: [String: String] = [:]
  /// Cache of gas token balance for each chainId (in pending transactions) for each account.
  /// Outer key is the `NetworkInfo.chainId`, inner key is the `AccountInfo.id`.
  private var gasTokenBalanceCache: [String: [String: Double]] = [:]
  /// Cache for storing `BlockchainToken`s that are not in user assets or our token registry.
  /// This could occur with a dapp creating a transaction.
  private var tokenInfoCache: [BraveWallet.BlockchainToken] = []
  /// Cache for storing the estimated transaction fee for each Solana transaction. The key is the transaction id.
  private var solEstimatedTxFeeCache: [String: UInt64] = [:]
  /// Cache of byte code lookups. The outer key is address, inner key is chainId, ex. `[address: [chainId: byteCode]]`.
  private var byteCodeCache: [String: [String: String]] = [:]

  @MainActor private func fetchAssetRatios(
    for userVisibleTokens: [BraveWallet.BlockchainToken]
  ) async {
    let priceResult = await assetRatioService.priceWithIndividualRetry(
      userVisibleTokens.map { $0.assetRatioId.lowercased() },
      toAssets: [currencyFormatter.currencyCode],
      timeframe: .oneDay
    )
    let newAssetRatios = priceResult.assetPrices.reduce(into: [String: Double]()) {
      $0[$1.fromAsset] = Double($1.price)
    }
    assetRatios.merge(with: newAssetRatios)
    updateTransaction(
      with: activeTransaction,
      shouldFetchCurrentAllowance: false,
      shouldFetchGasTokenBalance: false
    )
  }

  @MainActor func fetchCurrentAllowance(
    for parsedTransaction: ParsedTransaction,
    allTokens: [BraveWallet.BlockchainToken]
  ) async {
    guard case .ethErc20Approve(let details) = parsedTransaction.details else {
      return
    }

    let formatter = WalletAmountFormatter(decimalFormatStyle: .balance)
    let ownerAddress = parsedTransaction.fromAccountInfo.address
    let (allowance, _, _) = await rpcService.erc20TokenAllowance(
      contract: details.token?.contractAddress(in: selectedChain) ?? "",
      ownerAddress: ownerAddress,
      spenderAddress: details.spenderAddress,
      chainId: parsedTransaction.transaction.chainId
    )
    let allowanceString =
      formatter.decimalString(
        for: allowance.removingHexPrefix,
        radix: .hex,
        decimals: Int(details.token?.decimals ?? selectedChain.decimals)
      ) ?? ""
    currentAllowanceCache[parsedTransaction.transaction.id] = allowanceString
    updateTransaction(
      with: activeTransaction,
      shouldFetchCurrentAllowance: false,
      shouldFetchGasTokenBalance: false
    )
  }

  @MainActor func fetchGasTokenBalance(
    token: BraveWallet.BlockchainToken,
    account: BraveWallet.AccountInfo,
    network: BraveWallet.NetworkInfo
  ) async {
    var gasBalancesForChain = gasTokenBalanceCache[network.chainId, default: [:]]
    if token.coin == .btc {
      if let availableBTCBalance = await bitcoinWalletService.fetchBTCBalance(
        accountId: account.accountId,
        type: .available
      ) {
        gasBalancesForChain[account.id] = availableBTCBalance
      }
    } else {
      if let assetBalance = assetManager.getAssetBalances(
        for: token,
        account: account.id
      )?.first(where: { $0.chainId == network.chainId }) {
        gasBalancesForChain[account.id] = Double(assetBalance.balance) ?? 0
      } else if let gasTokenBalance = await rpcService.balance(
        for: token,
        in: account,
        network: network
      ) {
        gasBalancesForChain[account.id] = gasTokenBalance
      }
    }
    gasTokenBalanceCache[network.chainId] = gasBalancesForChain
    updateTransaction(
      with: activeTransaction,
      shouldFetchCurrentAllowance: false,
      shouldFetchGasTokenBalance: false
    )
  }

  @MainActor private func fetchUnknownTokens(
    for transactions: [BraveWallet.TransactionInfo]
  ) async {
    let ethTransactions = transactions.filter { $0.coin == .eth }
    guard !ethTransactions.isEmpty else { return }  // we can only fetch unknown Ethereum tokens
    let coin: BraveWallet.CoinType = .eth
    let allNetworks = await rpcService.allNetworks().filter({ $0.coin == coin })
    let userAssets = await assetManager.getAllUserAssetsInNetworkAssets(
      networks: allNetworks,
      includingUserDeleted: true
    ).flatMap(\.tokens)
    let allTokens = await blockchainRegistry.allTokens(in: allNetworks).flatMap(\.tokens)
    // Gather known information about the transaction(s) tokens
    let unknownTokenInfo = ethTransactions.unknownTokenContractAddressChainIdPairs(
      knownTokens: userAssets + allTokens + tokenInfoCache
    )
    guard !unknownTokenInfo.isEmpty else { return }  // Only if we have unknown tokens
    let unknownTokens: [BraveWallet.BlockchainToken] = await rpcService.fetchEthTokens(
      for: unknownTokenInfo
    )
    tokenInfoCache.append(contentsOf: unknownTokens)
    updateTransaction(
      with: activeTransaction,
      shouldFetchCurrentAllowance: false,
      shouldFetchGasTokenBalance: false
    )
  }

  @MainActor private func fetchSolEstimatedTxFees(
    for transactions: [BraveWallet.TransactionInfo]
  ) async {
    let solTxs = transactions.filter { $0.coin == .sol }
    let txFees = await solTxManagerProxy.solanaTxFeeEstimations(for: solTxs)
    solEstimatedTxFeeCache.merge(with: txFees)
    updateTransaction(
      with: activeTransaction,
      shouldFetchCurrentAllowance: false,
      shouldFetchGasTokenBalance: false
    )
  }

  /// Updates `isContractAddress` for the `activeParsedTransaction` without blocking display updates.
  /// Will fetch the byte code for the transaction recipient only once per chain.
  func updateIsContractAddress() {
    Task { @MainActor in
      if activeParsedTransaction.coin == .eth {
        let toAddress = activeParsedTransaction.toAddress
        let chainId = activeParsedTransaction.network.chainId
        if byteCodeCache[toAddress]?[chainId] == nil {
          let (byteCode, providerError, _) = await rpcService.code(
            address: toAddress,
            coin: activeParsedTransaction.coin,
            chainId: chainId
          )
          if providerError == .success {
            var toAddressCache = self.byteCodeCache[toAddress, default: [:]]
            toAddressCache[chainId] = byteCode
            self.byteCodeCache[toAddress] = toAddressCache
          }
        }
        guard let byteCodeForAddress = byteCodeCache[toAddress]?[chainId] else { return }
        self.isContractAddress = byteCodeForAddress != "0x"
      } else {
        self.isContractAddress = false
      }
    }
  }

  @MainActor func fetchActiveTransactionDetails(
    accounts: [BraveWallet.AccountInfo],
    network: BraveWallet.NetworkInfo,
    allTokens: [BraveWallet.BlockchainToken],
    shouldFetchCurrentAllowance: Bool,
    shouldFetchGasTokenBalance: Bool
  ) async {
    originInfo = activeParsedTransaction.transaction.originInfo
    transactionDetails = activeTransactionDetails

    switch activeParsedTransaction.details {
    case .ethSend(let details),
      .erc20Transfer(let details),
      .solSystemTransfer(let details),
      .solSplTokenTransfer(let details),
      .btcSend(let details):
      symbol = details.fromToken?.symbol ?? ""
      value = details.fromAmount
      fiat = details.fromFiat ?? ""

      if activeParsedTransaction.transaction.txType == .solanaCompressedNftTransfer {
        symbol = "SOL"
      }

      isSolTokenTransferWithAssociatedTokenAccountCreation =
        activeParsedTransaction.transaction.txType
        == .solanaSplTokenTransferWithAssociatedTokenAccountCreation

      if let gasFee = details.gasFee {
        gasValue = gasFee.fee
        gasFiat = gasFee.fiat
        gasSymbol = activeParsedTransaction.networkSymbol
        gasAssetRatio = assetRatios[activeParsedTransaction.networkSymbol.lowercased(), default: 0]

        let gasBalance = gasTokenBalanceCache[network.chainId]?[
          activeParsedTransaction.fromAccountInfo.id
        ]
        if let gasBalance,
          let gasValue = BDouble(gasFee.fee),
          let fromToken = details.fromToken,
          let fromValue = BDouble(details.fromAmount)
        {
          if network.isNativeAsset(fromToken) {
            isBalanceSufficient = BDouble(gasBalance) > gasValue + fromValue
          } else {
            isBalanceSufficient = BDouble(gasBalance) > gasValue
          }
        } else if shouldFetchGasTokenBalance || gasBalance == nil {
          isBalanceSufficient = false
          if let account = accounts.first(where: {
            $0.id == activeParsedTransaction.fromAccountInfo.id
          }) {
            await fetchGasTokenBalance(
              token: network.nativeToken,
              account: account,
              network: network
            )
          }
        } else {
          isBalanceSufficient = true
        }
      }
      if let fromToken = details.fromToken {
        totalFiat = totalFiat(
          value: value,
          tokenAssetRatioId: fromToken.assetRatioId,
          gasValue: gasValue,
          gasSymbol: gasSymbol,
          assetRatios: assetRatios,
          currencyFormatter: currencyFormatter
        )
      }

    case .ethErc20Approve(let details):
      value = details.approvalAmount
      symbol = details.token?.symbol ?? ""
      proposedAllowance = details.approvalValue
      isUnlimitedApprovalRequested = details.isUnlimited

      if let gasFee = details.gasFee {
        gasValue = gasFee.fee
        gasFiat = gasFee.fiat
        gasSymbol = activeParsedTransaction.networkSymbol
        gasAssetRatio = assetRatios[activeParsedTransaction.networkSymbol.lowercased(), default: 0]

        if let gasBalance = gasTokenBalanceCache[network.chainId]?[
          activeParsedTransaction.fromAccountInfo.id
        ] {
          if let gasValue = BDouble(gasFee.fee),
            BDouble(gasBalance) > gasValue
          {
            isBalanceSufficient = true
          } else {
            isBalanceSufficient = false
          }
        } else if shouldFetchGasTokenBalance {
          if let account = accounts.first(where: {
            $0.id == activeParsedTransaction.fromAccountInfo.id
          }) {
            await fetchGasTokenBalance(
              token: network.nativeToken,
              account: account,
              network: network
            )
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
    case .ethSwap(let details):
      symbol = details.fromToken?.symbol ?? ""
      value = details.fromAmount
      fiat = details.fromFiat ?? ""

      if let gasFee = details.gasFee {
        gasValue = gasFee.fee
        gasFiat = gasFee.fiat
        gasSymbol = activeParsedTransaction.networkSymbol
        gasAssetRatio = assetRatios[activeParsedTransaction.networkSymbol.lowercased(), default: 0]

        if let gasBalance = gasTokenBalanceCache[network.chainId]?[
          activeParsedTransaction.fromAccountInfo.id
        ] {
          if let gasValue = BDouble(gasFee.fee),
            BDouble(gasBalance) > gasValue
          {
            isBalanceSufficient = true
          } else {
            isBalanceSufficient = false
          }
        } else if shouldFetchGasTokenBalance {
          if let account = accounts.first(where: {
            $0.id == activeParsedTransaction.fromAccountInfo.id
          }) {
            await fetchGasTokenBalance(
              token: network.nativeToken,
              account: account,
              network: network
            )
          }
        }

        totalFiat = totalFiat(
          value: value,
          tokenAssetRatioId: details.fromToken?.assetRatioId ?? "",
          gasValue: gasValue,
          gasSymbol: gasSymbol,
          assetRatios: assetRatios,
          currencyFormatter: currencyFormatter
        )
      }
    case .erc721Transfer(let details):
      symbol = details.fromToken?.symbol ?? ""
      value = details.fromAmount

      if let gasFee = details.gasFee {
        gasValue = gasFee.fee
        gasFiat = gasFee.fiat
        gasSymbol = activeParsedTransaction.networkSymbol
        gasAssetRatio = assetRatios[activeParsedTransaction.networkSymbol.lowercased(), default: 0]

        if let gasBalance = gasTokenBalanceCache[network.chainId]?[
          activeParsedTransaction.fromAccountInfo.id
        ] {
          if let gasValue = BDouble(gasFee.fee),
            BDouble(gasBalance) > gasValue
          {
            isBalanceSufficient = true
          } else {
            isBalanceSufficient = false
          }
        } else if shouldFetchGasTokenBalance {
          if let account = accounts.first(where: {
            $0.id == activeParsedTransaction.fromAccountInfo.id
          }) {
            await fetchGasTokenBalance(
              token: network.nativeToken,
              account: account,
              network: network
            )
          }
        }

        totalFiat = totalFiat(
          value: value,
          tokenAssetRatioId: details.fromToken?.assetRatioId ?? "",
          gasValue: gasValue,
          gasSymbol: gasSymbol,
          assetRatios: assetRatios,
          currencyFormatter: currencyFormatter
        )
      }
    case .solDappTransaction(let details), .solSwapTransaction(let details):
      symbol = details.symbol ?? ""
      value = details.fromAmount
      transactionDetails =
        transactionDetails
        + "\n\n"
        + details.instructions
        .map(\.toString)
        .joined(separator: "\n\n====\n\n")  // separator between each instruction

      if let gasFee = details.gasFee {
        gasValue = gasFee.fee
        gasFiat = gasFee.fiat
        gasSymbol = activeParsedTransaction.networkSymbol
        gasAssetRatio = assetRatios[activeParsedTransaction.networkSymbol.lowercased(), default: 0]

        if let gasBalance = gasTokenBalanceCache[network.chainId]?[
          activeParsedTransaction.fromAccountInfo.id
        ] {
          if let gasValue = BDouble(gasFee.fee),
            BDouble(gasBalance) > gasValue
          {
            isBalanceSufficient = true
          } else {
            isBalanceSufficient = false
          }
        } else if shouldFetchGasTokenBalance {
          if let account = accounts.first(where: {
            $0.id == activeParsedTransaction.fromAccountInfo.id
          }) {
            await fetchGasTokenBalance(
              token: network.nativeToken,
              account: account,
              network: network
            )
          }
        }
      }
    case .filSend(let details):
      symbol = details.sendToken?.symbol ?? ""
      value = details.sendAmount
      fiat = details.sendFiat ?? ""

      filTxGasPremium = details.gasPremium
      filTxGasLimit = details.gasLimit
      filTxGasFeeCap = details.gasFeeCap

      if let gasFee = details.gasFee {
        gasValue = gasFee.fee
        gasFiat = gasFee.fiat
        gasSymbol = activeParsedTransaction.networkSymbol
        gasAssetRatio = assetRatios[activeParsedTransaction.networkSymbol.lowercased(), default: 0]

        if let gasBalance = gasTokenBalanceCache[network.chainId]?[
          activeParsedTransaction.fromAccountInfo.id
        ] {
          if let gasValue = BDouble(gasFee.fee),
            let sendValue = BDouble(details.sendAmount)
          {
            isBalanceSufficient = BDouble(gasBalance) > gasValue + sendValue
          } else {
            isBalanceSufficient = false
          }
        } else if shouldFetchGasTokenBalance {
          if let account = accounts.first(where: {
            $0.id == activeParsedTransaction.fromAccountInfo.id
          }) {
            await fetchGasTokenBalance(
              token: network.nativeToken,
              account: account,
              network: network
            )
          }
        }
      }
      if let token = details.sendToken {
        totalFiat = totalFiat(
          value: value,
          tokenAssetRatioId: token.assetRatioId,
          gasValue: gasValue,
          gasSymbol: gasSymbol,
          assetRatios: assetRatios,
          currencyFormatter: currencyFormatter
        )
      }
    case .other:
      break
    }
  }

  private func totalFiat(
    value: String,
    tokenAssetRatioId: String,
    gasValue: String,
    gasSymbol: String,
    assetRatios: [String: Double],
    currencyFormatter: NumberFormatter
  ) -> String {
    let ratio = assetRatios[tokenAssetRatioId.lowercased(), default: 0]
    let gasRatio = assetRatios[gasSymbol.lowercased(), default: 0]
    let amount = (Double(value) ?? 0.0) * ratio
    let gasAmount = (Double(gasValue) ?? 0.0) * gasRatio
    let totalFiat = currencyFormatter.formatAsFiat(amount + gasAmount) ?? "$0.00"
    return totalFiat
  }

  @MainActor private func fetchAllTransactions() async -> [BraveWallet.TransactionInfo] {
    let allAccounts = await keyringService.allAccounts().accounts
    var allNetworksForCoin: [BraveWallet.CoinType: [BraveWallet.NetworkInfo]] = [:]
    for coin in WalletConstants.supportedCoinTypes() {
      let allNetworks = await rpcService.allNetworks().filter({ $0.coin == coin })
      allNetworksForCoin[coin] = allNetworks
    }
    return await txService.pendingTransactions(
      networksForCoin: allNetworksForCoin,
      for: allAccounts
    )
    .sorted(by: { $0.createdTime > $1.createdTime })
  }

  @MainActor func confirm(
    transaction: BraveWallet.TransactionInfo
  ) async -> String? {
    isTxSubmitting = true
    let (success, error, message) = await txService.approveTransaction(
      coinType: transaction.coin,
      chainId: transaction.chainId,
      txMetaId: transaction.id
    )
    if error.tag == .providerError {
      transactionProviderErrorRegistry[transaction.id] = TransactionProviderError(
        code: error.providerError.rawValue,
        message: message
      )
    } else if error.tag == .solanaProviderError {
      transactionProviderErrorRegistry[transaction.id] = TransactionProviderError(
        code: error.solanaProviderError.rawValue,
        message: message
      )
    }
    return success ? nil : message
  }

  @MainActor func reject(transaction: BraveWallet.TransactionInfo) async -> Bool {
    await txService.rejectTransaction(
      coinType: transaction.coin,
      chainId: transaction.chainId,
      txMetaId: transaction.id
    )
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
      "Use updateGasFeeAndLimits(for:gasPrice:gasLimit:) for standard transactions"
    )
    ethTxManagerProxy.setGasFeeAndLimitForUnapprovedTransaction(
      chainId: transaction.chainId,
      txMetaId: transaction.id,
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
      "Use updateGasFeeAndLimits(for:maxPriorityFeePerGas:maxFeePerGas:gasLimit:) for EIP-1559 transactions"
    )
    ethTxManagerProxy.setGasPriceAndLimitForUnapprovedTransaction(
      chainId: transaction.chainId,
      txMetaId: transaction.id,
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
    ethTxManagerProxy.setNonceForUnapprovedTransaction(
      chainId: transaction.chainId,
      txMetaId: transaction.id,
      nonce: nonce
    ) { success in
      // not going to refresh unapproved transactions since the tx observer will be
      // notified `onTransactionStatusChanged` and `onUnapprovedTxUpdated`
      // `transactions` list will be refreshed there.
      completion(success)
    }
  }

  func editAllowance(
    transaction: BraveWallet.TransactionInfo,
    spenderAddress: String,
    amount: String,
    completion: @escaping (Bool) -> Void
  ) {
    ethTxManagerProxy.makeErc20ApproveData(spenderAddress: spenderAddress, amount: amount) {
      [weak self] success, data in
      guard let self = self else { return }
      if !success {
        completion(false)
        return
      }
      self.ethTxManagerProxy.setDataForUnapprovedTransaction(
        chainId: transaction.chainId,
        txMetaId: transaction.id,
        data: data
      ) { success in
        // not going to refresh unapproved transactions since the tx observer will be
        // notified `onTransactionStatusChanged` and `onUnapprovedTxUpdated`
        // `transactions` list will be refreshed there.
        completion(success)
      }
    }
  }

  func updateActiveTxIdAfterSignedClosed() {
    let indexOfChangedTx = unapprovedTxs.firstIndex(where: { $0.id == activeTransactionId }) ?? 0
    let newIndex = indexOfChangedTx > 0 ? indexOfChangedTx - 1 : 0
    activeTransactionId = unapprovedTxs[safe: newIndex]?.id ?? unapprovedTxs.first?.id ?? ""
  }

  func onUnapprovedTxUpdated(_ txInfo: BraveWallet.TransactionInfo) {
    // refresh the unapproved transaction list, as well as tx details UI
    // first update `allTxs` with the new updated txInfo(txStatus)
    if let index = allTxs.firstIndex(where: { $0.id == txInfo.id }) {
      allTxs[index] = txInfo
    }

    // update details UI if the current active tx is updated
    if activeTransactionId == txInfo.id {
      updateTransaction(with: txInfo)
      activeTxStatus = txInfo.txStatus
    }

    // if somehow the current active transaction no longer exists
    // set the first `.unapproved` tx as the new `activeTransactionId`
    if !unapprovedTxs.contains(where: { $0.id == activeTransactionId }) {
      activeTransactionId = unapprovedTxs.first?.id ?? ""
    }
  }

  func onTransactionStatusChanged(_ txInfo: BraveWallet.TransactionInfo) {
    // first update `allTxs` with the new updated txInfo(txStatus)
    if let index = allTxs.firstIndex(where: { $0.id == txInfo.id }) {
      allTxs[index] = txInfo
    }

    // only update the `activeTransactionId` if the current active transaction status
    // becomes `.rejected`/`.dropped`
    if activeTransactionId == txInfo.id,
      txInfo.txStatus == .rejected || txInfo.txStatus == .dropped
    {
      let indexOfChangedTx = unapprovedTxs.firstIndex(where: { $0.id == txInfo.id }) ?? 0
      let newIndex = indexOfChangedTx > 0 ? indexOfChangedTx - 1 : 0
      activeTransactionId =
        unapprovedTxs[safe: newIndex]?.id ?? unapprovedTxs.first?.id ?? ""
    } else {
      if activeTransactionId == txInfo.id {
        activeTxStatus = txInfo.txStatus
      }
    }
  }
}

struct TransactionProviderError {
  let code: Int
  let message: String
}

extension TransactionConfirmationStore: WalletUserAssetDataObserver {
  public func cachedBalanceRefreshed() {
    updateTransaction(
      with: activeTransaction,
      shouldFetchCurrentAllowance: false,
      shouldFetchGasTokenBalance: true
    )
  }

  public func userAssetUpdated() {
  }
}
