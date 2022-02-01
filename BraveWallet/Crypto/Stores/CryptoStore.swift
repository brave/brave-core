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
  @Published var isPresentingAssetSearch: Bool = false
  @Published var isPresentingTransactionConfirmations: Bool = false {
    didSet {
      if !isPresentingTransactionConfirmations {
        confirmationStore = nil
      }
    }
  }
  @Published private(set) var unapprovedTransactions: [BraveWallet.TransactionInfo] = []
  
  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let assetRatioService: BraveWalletAssetRatioService
  private let swapService: BraveWalletSwapService
  let blockchainRegistry: BraveWalletBlockchainRegistry
  private let txService: BraveWalletEthTxService
  
  public init(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    assetRatioService: BraveWalletAssetRatioService,
    swapService: BraveWalletSwapService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    txService: BraveWalletEthTxService
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.assetRatioService = assetRatioService
    self.swapService = swapService
    self.blockchainRegistry = blockchainRegistry
    self.txService = txService
    
    self.networkStore = .init(rpcService: rpcService)
    self.portfolioStore = .init(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: blockchainRegistry
    )
    
    self.keyringService.add(self)
    self.txService.add(self)
  }
  
  private var buyTokenStore: BuyTokenStore?
  func openBuyTokenStore(_ prefilledToken: BraveWallet.BlockchainToken?) -> BuyTokenStore {
    if let store = buyTokenStore {
      return store
    }
    let store = BuyTokenStore(
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      prefilledToken: prefilledToken
    )
    buyTokenStore = store
    return store
  }
  
  private var sendTokenStore: SendTokenStore?
  func openSendTokenStore(_ prefilledToken: BraveWallet.BlockchainToken?) -> SendTokenStore {
    if let store = sendTokenStore {
      return store
    }
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: txService,
      blockchainRegistry: blockchainRegistry,
      prefilledToken: prefilledToken
    )
    sendTokenStore = store
    return store
  }
  
  private var swapTokenStore: SwapTokenStore?
  func openSwapTokenStore(_ prefilledToken: BraveWallet.BlockchainToken?) -> SwapTokenStore {
    if let store = swapTokenStore {
      return store
    }
    let store = SwapTokenStore(
      keyringService: keyringService,
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      assetRatioService: assetRatioService,
      swapService: swapService,
      txService: txService,
      prefilledToken: prefilledToken
    )
    swapTokenStore = store
    return store
  }
  
  private var assetDetailStore: AssetDetailStore?
  func assetDetailStore(for token: BraveWallet.BlockchainToken) -> AssetDetailStore {
    if let store = assetDetailStore, store.token.id == token.id {
      return store
    }
    let store = AssetDetailStore(
      assetRatioService: assetRatioService,
      keyringService: keyringService,
      rpcService: rpcService,
      txService: txService,
      blockchainRegistry: blockchainRegistry,
      token: token
    )
    assetDetailStore = store
    return store
  }
  
  func closeAssetDetailStore(for token: BraveWallet.BlockchainToken) {
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
      rpcService: rpcService,
      assetRatioService: assetRatioService,
      txService: txService
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
      assetRatioService: assetRatioService,
      rpcService: rpcService,
      txService: txService,
      blockchainRegistry: blockchainRegistry,
      walletService: walletService
    )
    confirmationStore = store
    return store
  }
  
  func fetchUnapprovedTransactions() {
    keyringService.defaultKeyringInfo { [self] keyring in
      var pendingTransactions: [BraveWallet.TransactionInfo] = []
      let group = DispatchGroup()
      for info in keyring.accountInfos {
        group.enter()
        txService.allTransactionInfo(info.address) { tx in
          defer { group.leave() }
          pendingTransactions.append(contentsOf: tx.filter { $0.txStatus == .unapproved })
        }
      }
      group.notify(queue: .main) {
        if !pendingTransactions.isEmpty && buySendSwapDestination != nil {
          // Dismiss any buy send swap open to show the unapproved transactions
          self.buySendSwapDestination = nil
        }
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
          // On iPad if we set these before the send or swap screens disappear for some reason it crashes
          // within the SwiftUI runtime. Delaying it to give time for the animation to complete fixes it.
          self.unapprovedTransactions = pendingTransactions
          self.isPresentingTransactionConfirmations = !pendingTransactions.isEmpty
        }
      }
    }
  }
}

extension CryptoStore: BraveWalletEthTxServiceObserver {
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

extension CryptoStore: BraveWalletKeyringServiceObserver {
  public func keyringReset() {
  }
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
