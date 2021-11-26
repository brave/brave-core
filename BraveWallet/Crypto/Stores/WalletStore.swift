// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

/// The main wallet store
public class WalletStore: ObservableObject {
  
  public let keyringStore: KeyringStore
  public let networkStore: NetworkStore
  public let portfolioStore: PortfolioStore
  public let buyTokenStore: BuyTokenStore
  public let sendTokenStore: SendTokenStore
  public let swapTokenStore: SwapTokenStore
  public let confirmationStore: TransactionConfirmationStore
  
  @Published var buySendSwapDestination: BuySendSwapDestination?
  @Published var isPresentingTransactionConfirmations: Bool = false
  @Published private(set) var unapprovedTransactions: [BraveWallet.TransactionInfo] = []
  
  // MARK: -
  
  private let keyringController: BraveWalletKeyringController
  private let rpcController: BraveWalletEthJsonRpcController
  private let walletService: BraveWalletBraveWalletService
  private let assetRatioController: BraveWalletAssetRatioController
  private let swapController: BraveWalletSwapController
  let tokenRegistry: BraveWalletERCTokenRegistry
  private let transactionController: BraveWalletEthTxController
  
  public init(
    keyringController: BraveWalletKeyringController,
    rpcController: BraveWalletEthJsonRpcController,
    walletService: BraveWalletBraveWalletService,
    assetRatioController: BraveWalletAssetRatioController,
    swapController: BraveWalletSwapController,
    tokenRegistry: BraveWalletERCTokenRegistry,
    transactionController: BraveWalletEthTxController
  ) {
    self.keyringController = keyringController
    self.rpcController = rpcController
    self.walletService = walletService
    self.assetRatioController = assetRatioController
    self.swapController = swapController
    self.tokenRegistry = tokenRegistry
    self.transactionController = transactionController
    
    self.keyringStore = .init(keyringController: keyringController)
    self.networkStore = .init(rpcController: rpcController)
    self.portfolioStore = .init(
      keyringController: keyringController,
      rpcController: rpcController,
      walletService: walletService,
      assetRatioController: assetRatioController,
      tokenRegistry: tokenRegistry
    )
    self.buyTokenStore = .init(
      tokenRegistry: tokenRegistry,
      rpcController: rpcController
    )
    self.sendTokenStore = .init(
      keyringController: keyringController,
      rpcController: rpcController,
      walletService: walletService,
      transactionController: transactionController
    )
    self.swapTokenStore = .init(
      keyringController: keyringController,
      tokenRegistry: tokenRegistry,
      rpcController: rpcController,
      assetRatioController: assetRatioController,
      swapController: swapController,
      transactionController: transactionController
    )
    self.confirmationStore = .init(
      assetRatioController: assetRatioController,
      rpcController: rpcController,
      txController: transactionController,
      tokenRegistry: tokenRegistry
    )
    self.keyringController.add(self)
    self.transactionController.add(self)
  }
  
  func fetchUnapprovedTransactions() {
    keyringController.defaultKeyringInfo { [self] keyring in
      var pendingTransactions: [BraveWallet.TransactionInfo] = []
      let group = DispatchGroup()
      for info in keyring.accountInfos {
        group.enter()
        transactionController.allTransactionInfo(info.address) { tx in
          defer { group.leave() }
          pendingTransactions.append(contentsOf: tx.filter { $0.txStatus == .unapproved })
        }
      }
      group.notify(queue: .main) {
        if !pendingTransactions.isEmpty && buySendSwapDestination != nil {
          // Dismiss any buy send swap open to show the unapproved transactions
          self.buySendSwapDestination = nil
        }
        self.unapprovedTransactions = pendingTransactions
        self.isPresentingTransactionConfirmations = !pendingTransactions.isEmpty
      }
    }
  }
  
  private var assetDetailStore: AssetDetailStore?
  func assetDetailStore(for token: BraveWallet.ERCToken) -> AssetDetailStore {
    if let store = assetDetailStore, store.token.id == token.id {
      return store
    }
    let store = AssetDetailStore(
      assetRatioController: assetRatioController,
      keyringController: keyringController,
      rpcController: rpcController,
      txController: transactionController,
      token: token
    )
    assetDetailStore = store
    return store
  }
  
  private var accountActivityStore: AccountActivityStore?
  func accountActivityStore(for account: BraveWallet.AccountInfo) -> AccountActivityStore {
    if let store = accountActivityStore, store.account.address == account.address {
      return store
    }
    let store = AccountActivityStore(
      account: account,
      walletService: walletService,
      rpcController: rpcController,
      assetRatioController: assetRatioController,
      txController: transactionController
    )
    accountActivityStore = store
    return store
  }
}

extension WalletStore: BraveWalletEthTxControllerObserver {
  public func onNewUnapprovedTx(_ txInfo: BraveWallet.TransactionInfo) {
    fetchUnapprovedTransactions()
  }
  public func onUnapprovedTxUpdated(_ txInfo: BraveWallet.TransactionInfo) {
    fetchUnapprovedTransactions()
  }
  public func onTransactionStatusChanged(_ txInfo: BraveWallet.TransactionInfo) {
    fetchUnapprovedTransactions()
  }
}

extension WalletStore: BraveWalletKeyringControllerObserver {
  public func keyringCreated() {
  }
  public func keyringRestored() {
  }
  public func locked() {
    isPresentingTransactionConfirmations = false
  }
  public func unlocked() {
  }
  public func backedUp() {
  }
  public func accountsChanged() {
  }
  public func autoLockMinutesChanged() {
  }
  public func selectedAccountChanged() {
  }
}
