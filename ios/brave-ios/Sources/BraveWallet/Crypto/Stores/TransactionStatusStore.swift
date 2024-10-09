// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BigNumber
import BraveCore
import Combine
import Foundation
import Preferences
import Strings

public class TransactionStatusStore: ObservableObject, WalletObserverStore {
  /// The initial tx status should be `.submitted` otherwise, this store won't be created
  @Published var originalTxStatus: BraveWallet.TransactionStatus = .submitted {
    didSet {
      if oldValue == .submitted
        && originalTxStatus == .confirmed
        && pendingCancelTx != nil,
        let pendingTxStatus = pendingCancelTxStatus
      {
        // this tx is confirmed regardless there is a pending cancelling tx
        if pendingTxStatus == .submitted {
          // if the pending cancelling tx has been submitted
          // we should show some error
          print("Debug - original tx has been confirmed while cancelling tx has not")
        } else {
          print("Debug - original tx has been confirmed while cancelling tx has not but in other status: \(pendingTxStatus)")
        }
      }
    }
  }
  @Published var txProviderError: TransactionProviderError?
  @Published var pendingCancelTx: BraveWallet.TransactionInfo?
  @Published var pendingCancelParsedTx: ParsedTransaction?
  @Published var pendingCancelTxStatus: BraveWallet.TransactionStatus?
  @Published var cancelTxGasToken: BraveWallet.BlockchainToken?
  @Published var isConfirmingCancelTx: Bool = false {
    didSet {
      print("Debug - isConfirmingCancelTx: \(isConfirmingCancelTx)")
    }
  }
  @Published var gasRatio: Double = 0

  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let assetRatioService: BraveWalletAssetRatioService
  private let txService: BraveWalletTxService
  private let ethTxManagerProxy: BraveWalletEthTxManagerProxy
  private var txServiceObserver: TxServiceObserver?

  private var originalTxParsed: ParsedTransaction

  var isObserving: Bool {
    txServiceObserver != nil
  }

  var txValueInfo: String {
    switch originalTxParsed.details {
    case .erc20Transfer(let detail):
      return "\(detail.fromAmount) \(detail.fromToken?.symbol ?? "")"
    case .erc721Transfer(let detail):
      return "\(detail.fromAmount) \(detail.fromToken?.symbol ?? "")"
    case .ethSend(let detail):
      return "\(detail.fromAmount) \(detail.fromToken?.symbol ?? "ETH")"
    case .solSystemTransfer(let detail):
      return "\(detail.fromAmount) \(detail.fromToken?.symbol ?? "SOL")"
    case .solSplTokenTransfer(let detail):
      return "\(detail.fromAmount) \(detail.fromToken?.symbol ?? "")"
    case .solDappTransaction(let detail):
      return "\(detail.fromAmount) \(detail.symbol ?? "")"
    case .filSend(let detail):
      return "\(detail.sendAmount) \(detail.sendToken?.symbol ?? "FIL")"
    case .btcSend(let detail):
      return "\(detail.fromAmount) \(detail.fromToken?.symbol ?? "BTC")"
    default:
      return ""
    }
  }

  var isTxERC20Approve: Bool {
    activeParsedTx.transaction.txType == .erc20Approve
  }

  var txToAddress: String {
    activeParsedTx.toAddress
  }

  var isSpeedUpOrCancelAvailable: Bool {
    activeParsedTx.transaction.coin == .eth && !isConfirmingCancelTx
  }

  var activeParsedTx: ParsedTransaction {
    pendingCancelParsedTx ?? originalTxParsed
  }

  var activeTxStatus: BraveWallet.TransactionStatus {
    pendingCancelTxStatus ?? originalTxStatus
  }

  let currencyFormatter: NumberFormatter = .usdCurrencyFormatter
    .then {
      $0.minimumFractionDigits = 2
      $0.maximumFractionDigits = 6
    }

  init(
    activeTxStatus: BraveWallet.TransactionStatus,
    activeTxParsed: ParsedTransaction,
    txProviderError: TransactionProviderError?,
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    assetRatioService: BraveWalletAssetRatioService,
    txService: BraveWalletTxService,
    ethTxManagerProxy: BraveWalletEthTxManagerProxy
  ) {
    print("Debug - open TxStatusStore with tx(\(activeTxParsed.transaction.id)")
    self.originalTxStatus = activeTxStatus
    self.originalTxParsed = activeTxParsed
    self.txProviderError = txProviderError
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.assetRatioService = assetRatioService
    self.txService = txService
    self.ethTxManagerProxy = ethTxManagerProxy

    self.setupObservers()
  }

  func tearDown() {
    txServiceObserver = nil
  }

  func setupObservers() {
    guard !isObserving else { return }
    self.txServiceObserver = TxServiceObserver(
      txService: txService,
      _onNewUnapprovedTx: { [weak self] txInfo in
        guard let self else { return }
        if isTx(txInfo, cancelling: originalTxParsed.transaction) {
          Task { @MainActor [self] in
            print("DEBUG - new pending tx id: \(txInfo.id) nonce: \(txInfo.ethTxNonce)")
            self.pendingCancelTx = txInfo
            await self.prepareCancellingFlow()
          }
        }
      },
      _onUnapprovedTxUpdated: { [weak self] txInfo in
        // should only handle tx speed up and tx cancellation
        print("Debug - unapproved tx(\(txInfo.id)) udpated ")
      },
      _onTransactionStatusChanged: { [weak self] txInfo in
        guard let self else { return }
        if self.originalTxParsed.transaction.id == txInfo.id {
          self.originalTxStatus = txInfo.txStatus
        } else if pendingCancelParsedTx?.transaction.id == txInfo.id {
          self.pendingCancelTxStatus = txInfo.txStatus
        }
      }
    )
  }

  private func isTx(
    _ txA: BraveWallet.TransactionInfo,
    cancelling txB: BraveWallet.TransactionInfo
  ) -> Bool {
    guard txA.coin == .eth,
      txB.coin == .eth,
      txA.chainId == txB.chainId,
      txA.ethTxNonce == txB.ethTxNonce,
      txA.ethTxData.isEmpty,
      let txAValue = txA.txDataUnion.ethTxData1559?.baseData.value,
      let txAFrom = txA.fromAddress,
      let txBFrom = txB.fromAddress,
      let txAMaxFeePerGas =
        txA.txDataUnion.ethTxData1559?.maxFeePerGas,
      let txBMaxFeePerGas = txB.txDataUnion.ethTxData1559?.maxFeePerGas,
      let txAMaxFeePerGasFormatted =
        WalletAmountFormatter.weiToDecimalGwei(txAMaxFeePerGas.removingHexPrefix, radix: .hex),
      let txBMaxFeePerGasFormatted =
        WalletAmountFormatter.weiToDecimalGwei(txBMaxFeePerGas.removingHexPrefix, radix: .hex),
      let txAGasBDouble = BDouble(txAMaxFeePerGasFormatted),
      let txBGasIntBDouble = BDouble(txBMaxFeePerGasFormatted)
    else {
      return false
    }
    return txAValue == "0x0" && txAFrom.caseInsensitiveCompare(txBFrom) == .orderedSame && txAGasBDouble > txBGasIntBDouble
  }

  @MainActor private func prepareCancellingFlow() async {
    guard
      let pendingTx = pendingCancelTx,
      let network = await rpcService.allNetworks(
        for: [pendingTx.coin]
      ).first(where: { $0.chainId == pendingTx.chainId })
    else { return }

    cancelTxGasToken = network.nativeToken
    let allAccountsForCoin = await keyringService.allAccounts().accounts.filter {
      $0.coin == pendingTx.coin
    }
    let ratioFetch = await assetRatioService.priceWithIndividualRetry(
      [network.nativeToken.assetRatioId.lowercased()],
      toAssets: [currencyFormatter.currencyCode],
      timeframe: .oneDay
    )
    gasRatio = Double(ratioFetch.assetPrices.first?.price ?? "0") ?? 0
    if let parsedTransaction = pendingTx.parsedTransaction(
      allNetworks: [network],
      accountInfos: allAccountsForCoin,
      userAssets: [network.nativeToken],
      allTokens: [network.nativeToken],
      assetRatios: [network.nativeToken.assetRatioId.lowercased(): gasRatio],
      nftMetadata: [:],
      solEstimatedTxFee: nil,
      currencyFormatter: currencyFormatter
    ) {
      pendingCancelParsedTx = parsedTransaction
      pendingCancelTxStatus = pendingTx.txStatus
    }
  }

  @MainActor func explorerLink() async -> URL? {
    guard
      let txNetwork = await rpcService.allNetworks()
        .first(where: {
          $0.chainId == originalTxParsed.transaction.chainId
        })
    else { return nil }
    return txNetwork.txBlockExplorerLink(
      txHash: originalTxParsed.transaction.txHash,
      for: txNetwork.coin
    )
  }

  @MainActor func cancelActiveTx() async -> Bool {
//    pendingCancelTx = originalTxParsed.transaction
//    pendingCancelParsedTx = originalTxParsed
//    await prepareCancellingFlow()
//    return true
    print("Debug - cancelling tx(\(originalTxParsed.transaction.id)")
    let (success, metaId, errMsg) = await txService.speedupOrCancelTransaction(
      coinType: originalTxParsed.transaction.coin,
      chainId: originalTxParsed.transaction.chainId,
      txMetaId: originalTxParsed.transaction.id,
      cancel: true
    )
    print("DEBUG - cancel active tx(\(originalTxParsed.transaction.id)): \(success ? "successful" : "failed (\(errMsg))")")
    print("DEBUG - new meta id: \(metaId)")
    return success
  }

  @MainActor func confirmCancellation() async -> String? {
    guard let pendingCancelTx else { return nil }
    isConfirmingCancelTx = true
    let (success, error, message) = await txService.approveTransaction(
      coinType: pendingCancelTx.coin,
      chainId: pendingCancelTx.chainId,
      txMetaId: pendingCancelTx.id
    )

    return success ? nil : message
  }

  @MainActor func rejectCancellation() async -> Bool {
    guard let pendingCancelTx else { return false }
    return await txService.rejectTransaction(
      coinType: pendingCancelTx.coin,
      chainId: pendingCancelTx.chainId,
      txMetaId: pendingCancelTx.id
    )
  }
  
  @MainActor func updateSpeedPriority(
    _ priority: EditSpeedPriorityView.SpeedPriorityType,
    transaction: BraveWallet.TransactionInfo,
    gasEstimation: BraveWallet.GasEstimation1559
  ) async -> Bool {
    let hexGasLimit = transaction.ethTxGasLimit
    let hexGasFee: String
    let hexGasTip: String
    switch priority {
    case .slow:
      hexGasFee = gasEstimation.slowMaxFeePerGas
      hexGasTip = gasEstimation.slowMaxPriorityFeePerGas
    case .standard:
      hexGasFee = gasEstimation.avgMaxFeePerGas
      hexGasTip = gasEstimation.avgMaxPriorityFeePerGas
    case .fast:
      hexGasFee = gasEstimation.fastMaxFeePerGas
      hexGasTip = gasEstimation.fastMaxPriorityFeePerGas
    }

    return await ethTxManagerProxy.setGasFeeAndLimitForUnapprovedTransaction(
      chainId: transaction.chainId,
      txMetaId: transaction.id,
      maxPriorityFeePerGas: hexGasTip,
      maxFeePerGas: hexGasFee,
      gasLimit: hexGasLimit
    )
  }
}

extension TransactionStatusStore: WalletUserAssetDataObserver {
  public func cachedBalanceRefreshed() {
  }

  public func userAssetUpdated() {
  }
}
