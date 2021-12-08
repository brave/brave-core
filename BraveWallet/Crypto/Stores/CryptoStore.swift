// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore

public class CryptoStore: ObservableObject {
  public let networkStore: NetworkStore
  public let portfolioStore: PortfolioStore
  
  @Published var buySendSwapDestination: BuySendSwapDestination? {
    didSet {
      if buySendSwapDestination == nil {
        buyTokenStore = nil
        sendTokenStore = nil
        swapTokenStore = nil
      }
    }
  }
  @Published var isPresentingTransactionConfirmations: Bool = false {
    didSet {
      if !isPresentingTransactionConfirmations {
        confirmationStore = nil
      }
    }
  }
  @Published private(set) var unapprovedTransactions: [BraveWallet.TransactionInfo] = []
  
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
    
    self.networkStore = .init(rpcController: rpcController)
    self.portfolioStore = .init(
      keyringController: keyringController,
      rpcController: rpcController,
      walletService: walletService,
      assetRatioController: assetRatioController,
      tokenRegistry: tokenRegistry
    )
    
    self.keyringController.add(self)
    self.transactionController.add(self)
  }
  
  private var buyTokenStore: BuyTokenStore?
  func openBuyTokenStore() -> BuyTokenStore {
    if let store = buyTokenStore {
      return store
    }
    let store = BuyTokenStore(
      tokenRegistry: tokenRegistry,
      rpcController: rpcController
    )
    buyTokenStore = store
    return store
  }
  
  private var sendTokenStore: SendTokenStore?
  func openSendTokenStore() -> SendTokenStore {
    if let store = sendTokenStore {
      return store
    }
    let store = SendTokenStore(
      keyringController: keyringController,
      rpcController: rpcController,
      walletService: walletService,
      transactionController: transactionController,
      tokenRegistery: tokenRegistry
    )
    sendTokenStore = store
    return store
  }
  
  private var swapTokenStore: SwapTokenStore?
  func openSwapTokenStore() -> SwapTokenStore {
    if let store = swapTokenStore {
      return store
    }
    let store = SwapTokenStore(
      keyringController: keyringController,
      tokenRegistry: tokenRegistry,
      rpcController: rpcController,
      assetRatioController: assetRatioController,
      swapController: swapController,
      transactionController: transactionController
    )
    swapTokenStore = store
    return store
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
  
  func closeAssetDetailStore(for token: BraveWallet.ERCToken) {
    if let store = assetDetailStore, store.token.id == token.id {
      assetDetailStore = nil
    }
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
  
  func closeAccountActivityStore(for account: BraveWallet.AccountInfo) {
    if let store = accountActivityStore, store.account.address == account.address {
      accountActivityStore = nil
    }
  }
  
  private var confirmationStore: TransactionConfirmationStore?
  func openConfirmationStore() -> TransactionConfirmationStore {
    if let store = confirmationStore {
      return store
    }
    let store = TransactionConfirmationStore(
      assetRatioController: assetRatioController,
      rpcController: rpcController,
      txController: transactionController,
      tokenRegistry: tokenRegistry,
      walletService: walletService
    )
    confirmationStore = store
    return store
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
}

extension CryptoStore: BraveWalletEthTxControllerObserver {
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

extension CryptoStore: BraveWalletKeyringControllerObserver {
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
