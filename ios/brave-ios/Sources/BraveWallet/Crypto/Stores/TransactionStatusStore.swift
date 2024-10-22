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
  @Published var activeTxStatus: BraveWallet.TransactionStatus {
    didSet {
      originalTxStatus = activeTxStatus
    }
  }
  @Published var txProviderError: TransactionProviderError?

  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let txService: BraveWalletTxService
  private var txServiceObserver: TxServiceObserver?

  private var originalTxParsed: ParsedTransaction
  private var originalTxStatus: BraveWallet.TransactionStatus

  var isObserving: Bool {
    txServiceObserver != nil
  }

  var activeParsedTx: ParsedTransaction {
    originalTxParsed
  }

  init(
    activeTxStatus: BraveWallet.TransactionStatus,
    activeTxParsed: ParsedTransaction,
    txProviderError: TransactionProviderError?,
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    txService: BraveWalletTxService
  ) {
    self.activeTxStatus = activeTxStatus
    self.originalTxStatus = activeTxStatus
    self.originalTxParsed = activeTxParsed
    self.txProviderError = txProviderError
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.txService = txService

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
        // should only handle tx speed up and tx cancellation
      },
      _onUnapprovedTxUpdated: { _ in
        // should only handle tx speed up and tx cancellation
      },
      _onTransactionStatusChanged: { [weak self] txInfo in
        guard let self else { return }
        if self.activeParsedTx.transaction.id == txInfo.id {
          self.activeTxStatus = txInfo.txStatus
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
}
