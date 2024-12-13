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
  @Published var activeTxStatus: BraveWallet.TransactionStatus
  @Published var txProviderError: TransactionProviderError?
  @Published var isOriginalTxConfirmed: Bool = false

  enum FollowUpAction: Equatable {
    case cancel(originalParsedTx: ParsedTransaction)
    case speedUp(originalParsedTx: ParsedTransaction)
    case none
  }

  private(set) var followUpAction: FollowUpAction

  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let txService: BraveWalletTxService
  private var txServiceObserver: TxServiceObserver?

  var isObserving: Bool {
    txServiceObserver != nil
  }

  var activeParsedTx: ParsedTransaction

  var isCancelAvailable: Bool {
    guard activeParsedTx.transaction.coin == .eth else { return false }
    if case .cancel(_) = followUpAction {
      return false
    } else {
      return true
    }
  }

  var isSpeedUpAvailable: Bool {
    return activeParsedTx.transaction.coin == .eth
  }

  init(
    activeTxStatus: BraveWallet.TransactionStatus,
    activeTxParsed: ParsedTransaction,
    txProviderError: TransactionProviderError?,
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    txService: BraveWalletTxService,
    followUpAction: FollowUpAction
  ) {
    self.activeTxStatus = activeTxStatus
    self.activeParsedTx = activeTxParsed
    self.txProviderError = txProviderError
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.txService = txService
    self.followUpAction = followUpAction

    self.setupObservers()
  }

  func tearDown() {
    txServiceObserver = nil
  }

  func setupObservers() {
    guard !isObserving else { return }
    self.txServiceObserver = TxServiceObserver(
      txService: txService,
      _onNewUnapprovedTx: { _ in
        // tx speed up and cancellation will be handled inside `TxConfirmationStore`
      },
      _onUnapprovedTxUpdated: { _ in
        // should only handle tx speed up and tx cancellation
      },
      _onTransactionStatusChanged: { [weak self] txInfo in
        guard let self else { return }
        if self.activeParsedTx.transaction.id == txInfo.id {
          self.activeTxStatus = txInfo.txStatus
        } else {
          if case .cancel(let originalParsedTx) = followUpAction,
            originalParsedTx.transaction.id == txInfo.id,
            txInfo.txStatus == .confirmed
          {
            // cancel tx is submitted
            // but the original tx is confirmed
            isOriginalTxConfirmed = true
          } else if case .speedUp(let originalParsedTx) = followUpAction,
            originalParsedTx.transaction.id == txInfo.id,
            txInfo.txStatus == .confirmed
          {
            // speed up tx is submitted
            // but the original tx is confirmed
            isOriginalTxConfirmed = true
          }
        }
      }
    )
  }

  @MainActor func explorerLink() async -> URL? {
    guard
      let txNetwork = await rpcService.allNetworks()
        .first(where: {
          $0.chainId == activeParsedTx.transaction.chainId
        })
    else { return nil }
    return txNetwork.txBlockExplorerLink(
      txHash: activeParsedTx.transaction.txHash,
      for: txNetwork.coin
    )
  }

  @MainActor func handleTransactionFollowUpAction(
    _ action: TransactionFollowUpAction
  ) async -> (String?, String?) {
    let (success, txId, error) = await txService.speedupOrCancelTransaction(
      coinType: activeParsedTx.transaction.coin,
      chainId: activeParsedTx.transaction.chainId,
      txMetaId: activeParsedTx.transaction.id,
      cancel: action == .cancel
    )
    if success {
      return (txId, nil)
    }
    return (nil, error)
  }
}
