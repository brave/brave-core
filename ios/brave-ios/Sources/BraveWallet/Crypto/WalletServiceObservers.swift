// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

class KeyringServiceObserver: BraveWalletKeyringServiceObserver {

  var _walletReset: (() -> Void)?
  var _walletCreated: (() -> Void)?
  var _walletRestored: (() -> Void)?
  var _locked: (() -> Void)?
  var _unlocked: (() -> Void)?
  var _backedUp: (() -> Void)?
  var _accountsChanged: (() -> Void)?
  var _autoLockMinutesChanged: (() -> Void)?
  var _selectedWalletAccountChanged: ((_ account: BraveWallet.AccountInfo) -> Void)?
  var _selectedDappAccountChanged:
    ((_ coin: BraveWallet.CoinType, _ account: BraveWallet.AccountInfo?) -> Void)?
  var _accountsAdded: ((_ addedAccounts: [BraveWallet.AccountInfo]) -> Void)?

  init(
    keyringService: BraveWalletKeyringService,
    _walletReset: (() -> Void)? = nil,
    _walletCreated: (() -> Void)? = nil,
    _walletRestored: (() -> Void)? = nil,
    _locked: (() -> Void)? = nil,
    _unlocked: (() -> Void)? = nil,
    _backedUp: (() -> Void)? = nil,
    _accountsChanged: (() -> Void)? = nil,
    _autoLockMinutesChanged: (() -> Void)? = nil,
    _selectedWalletAccountChanged: ((BraveWallet.AccountInfo) -> Void)? = nil,
    _selectedDappAccountChanged: ((BraveWallet.CoinType, BraveWallet.AccountInfo?) -> Void)? = nil,
    _accountsAdded: (([BraveWallet.AccountInfo]) -> Void)? = nil
  ) {
    self._walletReset = _walletReset
    self._walletCreated = _walletCreated
    self._walletRestored = _walletRestored
    self._locked = _locked
    self._unlocked = _unlocked
    self._backedUp = _backedUp
    self._accountsChanged = _accountsChanged
    self._autoLockMinutesChanged = _autoLockMinutesChanged
    self._selectedWalletAccountChanged = _selectedWalletAccountChanged
    self._selectedDappAccountChanged = _selectedDappAccountChanged
    self._accountsAdded = _accountsAdded
    keyringService.addObserver(self)
  }

  func walletReset() {
    _walletReset?()
  }
  func walletCreated() {
    _walletCreated?()
  }
  public func walletRestored() {
    _walletRestored?()
  }
  public func locked() {
    _locked?()
  }
  public func unlocked() {
    _unlocked?()
  }
  public func backedUp() {
    _backedUp?()
  }
  public func accountsChanged() {
    _accountsChanged?()
  }
  public func autoLockMinutesChanged() {
    _autoLockMinutesChanged?()
  }
  public func selectedWalletAccountChanged(account: BraveWallet.AccountInfo) {
    _selectedWalletAccountChanged?(account)
  }
  public func selectedDappAccountChanged(
    coin: BraveWallet.CoinType,
    account: BraveWallet.AccountInfo?
  ) {
    _selectedDappAccountChanged?(coin, account)
  }
  public func accountsAdded(addedAccounts: [BraveWallet.AccountInfo]) {
    _accountsAdded?(addedAccounts)
  }
}

class WalletServiceObserver: BraveWalletBraveWalletServiceObserver {

  var _onActiveOriginChanged: ((_ originInfo: BraveWallet.OriginInfo) -> Void)?
  var _onDefaultEthereumWalletChanged: ((_ wallet: BraveWallet.DefaultWallet) -> Void)?
  var _onDefaultSolanaWalletChanged: ((_ wallet: BraveWallet.DefaultWallet) -> Void)?
  var _onDefaultBaseCurrencyChanged: ((_ currency: String) -> Void)?
  var _onDefaultBaseCryptocurrencyChanged: ((_ cryptocurrency: String) -> Void)?
  var _onNetworkListChanged: (() -> Void)?
  var _onDiscoverAssetsStarted: (() -> Void)?
  var _onDiscoverAssetsCompleted: ((_ discoveredAssets: [BraveWallet.BlockchainToken]) -> Void)?
  var _onResetWallet: (() -> Void)?

  init(
    walletService: BraveWalletBraveWalletService,
    _onActiveOriginChanged: ((_ originInfo: BraveWallet.OriginInfo) -> Void)? = nil,
    _onDefaultEthereumWalletChanged: ((_ wallet: BraveWallet.DefaultWallet) -> Void)? = nil,
    _onDefaultSolanaWalletChanged: ((_ wallet: BraveWallet.DefaultWallet) -> Void)? = nil,
    _onDefaultBaseCurrencyChanged: ((_ currency: String) -> Void)? = nil,
    _onDefaultBaseCryptocurrencyChanged: ((_ cryptocurrency: String) -> Void)? = nil,
    _onNetworkListChanged: (() -> Void)? = nil,
    _onDiscoverAssetsStarted: (() -> Void)? = nil,
    _onDiscoverAssetsCompleted: ((_ discoveredAssets: [BraveWallet.BlockchainToken]) -> Void)? =
      nil,
    _onResetWallet: (() -> Void)? = nil
  ) {
    self._onActiveOriginChanged = _onActiveOriginChanged
    self._onDefaultEthereumWalletChanged = _onDefaultEthereumWalletChanged
    self._onDefaultSolanaWalletChanged = _onDefaultSolanaWalletChanged
    self._onDefaultBaseCurrencyChanged = _onDefaultBaseCurrencyChanged
    self._onDefaultBaseCryptocurrencyChanged = _onDefaultBaseCryptocurrencyChanged
    self._onNetworkListChanged = _onNetworkListChanged
    self._onDiscoverAssetsStarted = _onDiscoverAssetsStarted
    self._onDiscoverAssetsCompleted = _onDiscoverAssetsCompleted
    self._onResetWallet = _onResetWallet
    walletService.addObserver(self)
  }

  func onActiveOriginChanged(originInfo: BraveWallet.OriginInfo) {
    _onActiveOriginChanged?(originInfo)
  }

  func onDefaultEthereumWalletChanged(wallet: BraveWallet.DefaultWallet) {
    _onDefaultEthereumWalletChanged?(wallet)
  }

  func onDefaultSolanaWalletChanged(wallet: BraveWallet.DefaultWallet) {
    _onDefaultSolanaWalletChanged?(wallet)
  }

  func onDefaultBaseCurrencyChanged(currency: String) {
    _onDefaultBaseCurrencyChanged?(currency)
  }

  func onDefaultBaseCryptocurrencyChanged(cryptocurrency: String) {
    _onDefaultBaseCryptocurrencyChanged?(cryptocurrency)
  }

  func onNetworkListChanged() {
    _onNetworkListChanged?()
  }

  func onDiscoverAssetsStarted() {
    _onDiscoverAssetsStarted?()
  }

  func onDiscoverAssetsCompleted(discoveredAssets: [BraveWallet.BlockchainToken]) {
    _onDiscoverAssetsCompleted?(discoveredAssets)
  }

  func onResetWallet() {
    _onResetWallet?()
  }
}

class TxServiceObserver: BraveWalletTxServiceObserver {
  var _onNewUnapprovedTx: ((_ txInfo: BraveWallet.TransactionInfo) -> Void)?
  var _onUnapprovedTxUpdated: ((_ txInfo: BraveWallet.TransactionInfo) -> Void)?
  var _onTransactionStatusChanged: ((_ txInfo: BraveWallet.TransactionInfo) -> Void)?
  var _onTxServiceReset: (() -> Void)?

  init(
    txService: BraveWalletTxService,
    _onNewUnapprovedTx: ((_: BraveWallet.TransactionInfo) -> Void)? = nil,
    _onUnapprovedTxUpdated: ((_: BraveWallet.TransactionInfo) -> Void)? = nil,
    _onTransactionStatusChanged: ((_: BraveWallet.TransactionInfo) -> Void)? = nil,
    _onTxServiceReset: (() -> Void)? = nil
  ) {
    self._onNewUnapprovedTx = _onNewUnapprovedTx
    self._onUnapprovedTxUpdated = _onUnapprovedTxUpdated
    self._onTransactionStatusChanged = _onTransactionStatusChanged
    self._onTxServiceReset = _onTxServiceReset
    txService.addObserver(self)
  }

  func onNewUnapprovedTx(txInfo: BraveWallet.TransactionInfo) {
    _onNewUnapprovedTx?(txInfo)
  }

  func onUnapprovedTxUpdated(txInfo: BraveWallet.TransactionInfo) {
    _onUnapprovedTxUpdated?(txInfo)
  }

  func onTransactionStatusChanged(txInfo: BraveWallet.TransactionInfo) {
    _onTransactionStatusChanged?(txInfo)
  }

  func onTxServiceReset() {
    _onTxServiceReset?()
  }
}

class JsonRpcServiceObserver: BraveWalletJsonRpcServiceObserver {
  var _chainChangedEvent:
    ((_ chainId: String, _ coin: BraveWallet.CoinType, _ origin: URLOrigin?) -> Void)?
  var _onAddEthereumChainRequestCompleted: ((_ chainId: String, _ error: String) -> Void)?

  init(
    rpcService: BraveWalletJsonRpcService,
    _chainChangedEvent: (
      (_ chainId: String, _ coin: BraveWallet.CoinType, _ origin: URLOrigin?) -> Void
    )? = nil,
    _onAddEthereumChainRequestCompleted: ((_ chainId: String, _ error: String) -> Void)? = nil
  ) {
    self._chainChangedEvent = _chainChangedEvent
    self._onAddEthereumChainRequestCompleted = _onAddEthereumChainRequestCompleted
    rpcService.addObserver(self)
  }

  func chainChangedEvent(chainId: String, coin: BraveWallet.CoinType, origin: URLOrigin?) {
    _chainChangedEvent?(chainId, coin, origin)
  }

  func onAddEthereumChainRequestCompleted(chainId: String, error: String) {
    _onAddEthereumChainRequestCompleted?(chainId, error)
  }

}
