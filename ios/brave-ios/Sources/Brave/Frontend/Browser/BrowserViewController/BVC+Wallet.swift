// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import BraveWallet
import Data
import Foundation
import Growth
import Preferences
import Shared
import SwiftUI
import os.log

extension WalletStore {
  /// Creates a WalletStore based on whether or not the user is in Private Mode
  static func from(
    ipfsApi: IpfsAPI,
    walletP3A: BraveWalletBraveWalletP3A?,
    privateMode: Bool
  ) -> WalletStore? {
    guard
      let keyringService = BraveWallet.KeyringServiceFactory.get(privateMode: privateMode),
      let rpcService = BraveWallet.JsonRpcServiceFactory.get(privateMode: privateMode),
      let assetRatioService = BraveWallet.AssetRatioServiceFactory.get(privateMode: privateMode),
      let walletService = BraveWallet.ServiceFactory.get(privateMode: privateMode),
      let swapService = BraveWallet.SwapServiceFactory.get(privateMode: privateMode),
      let txService = BraveWallet.TxServiceFactory.get(privateMode: privateMode),
      let ethTxManagerProxy = BraveWallet.EthTxManagerProxyFactory.get(privateMode: privateMode),
      let solTxManagerProxy = BraveWallet.SolanaTxManagerProxyFactory.get(privateMode: privateMode),
      let walletP3A,
      let bitcoinWalletService = BraveWallet.BitcoinWalletServiceFactory.get(
        privateMode: privateMode
      )
    else {
      Logger.module.error("Failed to load wallet. One or more services were unavailable")
      return nil
    }
    return WalletStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      swapService: swapService,
      blockchainRegistry: BraveWalletAPI.blockchainRegistry,
      txService: txService,
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      ipfsApi: ipfsApi,
      walletP3A: walletP3A,
      bitcoinWalletService: bitcoinWalletService
    )
  }
}

extension CryptoStore {
  /// Creates a CryptoStore based on whether or not the user is in Private Mode
  static func from(
    ipfsApi: IpfsAPI,
    walletP3A: BraveWalletBraveWalletP3A?,
    privateMode: Bool
  ) -> CryptoStore? {
    guard
      let keyringService = BraveWallet.KeyringServiceFactory.get(privateMode: privateMode),
      let rpcService = BraveWallet.JsonRpcServiceFactory.get(privateMode: privateMode),
      let assetRatioService = BraveWallet.AssetRatioServiceFactory.get(privateMode: privateMode),
      let walletService = BraveWallet.ServiceFactory.get(privateMode: privateMode),
      let swapService = BraveWallet.SwapServiceFactory.get(privateMode: privateMode),
      let txService = BraveWallet.TxServiceFactory.get(privateMode: privateMode),
      let ethTxManagerProxy = BraveWallet.EthTxManagerProxyFactory.get(privateMode: privateMode),
      let solTxManagerProxy = BraveWallet.SolanaTxManagerProxyFactory.get(privateMode: privateMode),
      let walletP3A,
      let bitcoinWalletService = BraveWallet.BitcoinWalletServiceFactory.get(
        privateMode: privateMode
      )
    else {
      Logger.module.error("Failed to load wallet. One or more services were unavailable")
      return nil
    }
    return CryptoStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      swapService: swapService,
      blockchainRegistry: BraveWalletAPI.blockchainRegistry,
      txService: txService,
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      ipfsApi: ipfsApi,
      walletP3A: walletP3A,
      bitcoinWalletService: bitcoinWalletService
    )
  }
}

extension BrowserViewController {
  /// Initializes a new WalletStore for displaying the wallet, setting up an observer to notify
  /// when the pending request is updated so we can update the wallet url bar button.
  func newWalletStore() -> WalletStore? {
    let privateMode = privateBrowsingManager.isPrivateBrowsing
    guard
      let walletStore = WalletStore.from(
        ipfsApi: braveCore.ipfsAPI,
        walletP3A: braveCore.braveWalletAPI.walletP3A(),
        privateMode: privateMode
      )
    else {
      Logger.module.error("Failed to load wallet. One or more services were unavailable")
      return nil
    }
    self.walletStore = walletStore
    self.onPendingRequestUpdatedCancellable = walletStore.onPendingRequestUpdated
      .sink { [weak self] _ in
        self?.updateURLBarWalletButton()
      }
    return walletStore
  }

  /// Presents the Wallet panel for a given origin
  func presentWalletPanel(from origin: URLOrigin, with tabDappStore: TabDappStore) {
    guard let walletStore = self.walletStore ?? newWalletStore() else { return }
    walletStore.origin = origin
    let controller = WalletPanelHostingController(
      walletStore: walletStore,
      tabDappStore: tabDappStore,
      origin: origin,
      webImageDownloader: braveCore.webImageDownloader
    )
    controller.delegate = self
    let popover = PopoverController(contentController: controller)
    popover.present(from: topToolbar.locationView.walletButton, on: self, completion: nil)
    toolbarVisibilityViewModel.toolbarState = .expanded
  }
}

extension WalletPanelHostingController: PopoverContentComponent {}

extension BrowserViewController: BraveWalletDelegate {
  public func requestAppReview() {
    // wait for any dismissals, ex after adding an account/sending crypto/swapping crypto
    DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
      var topMostViewController: UIViewController = self
      while let presentedViewController = topMostViewController.presentedViewController {
        topMostViewController = presentedViewController
      }
      // Handle App Rating
      // User is either finished sending/swapping a crypto token or added an account to wallet.
      AppReviewManager.shared.handleAppReview(for: .revised, using: self)
    }
  }

  public func openDestinationURL(_ destinationURL: URL) {
    if presentedViewController != nil {
      // dismiss to show the new tab
      self.dismiss(animated: true)
    }
    if let url = tabManager.selectedTab?.url, InternalURL.isValid(url: url) {
      select(url: destinationURL, isUserDefinedURLNavigation: false)
    } else {
      _ = tabManager.addTabAndSelect(
        URLRequest(url: destinationURL),
        isPrivate: privateBrowsingManager.isPrivateBrowsing
      )
    }
  }

  public func walletPanel(
    _ panel: WalletPanelHostingController,
    presentWalletWithContext: PresentingContext,
    walletStore: WalletStore
  ) {
    let walletHostingController = WalletHostingViewController(
      walletStore: walletStore,
      webImageDownloader: braveCore.webImageDownloader,
      presentingContext: presentWalletWithContext
    )
    walletHostingController.delegate = self

    switch presentWalletWithContext {
    case .default, .settings:
      // Dismiss Wallet Panel first, then present Wallet
      self.dismiss(animated: true) { [weak self] in
        self?.present(walletHostingController, animated: true)
      }
    default:
      panel.present(walletHostingController, animated: true)
    }
  }
}

extension Tab: BraveWalletProviderDelegate {
  func showPanel() {
    guard let origin = url?.origin else {
      Logger.module.error("Failing to show Wallet panel due to unavailable tab url origin")
      return
    }
    tabDelegate?.showWalletNotification(self, origin: origin)
  }

  func getOrigin() -> URLOrigin {
    guard let origin = url?.origin else {
      // A nil url is possible if multiple tabs are restored but one or more
      // of the tabs is not opened yet (loaded the url). When a new chain is
      // assigned for a specific origin, the provider(s) will check origin
      // of all open Tab's to see if that provider needs(s) updated too.
      // We can get the url from the SessionTab, and return it's origin.
      if let sessionTabOrigin = SessionTab.from(tabId: id)?.url?.origin {
        return sessionTabOrigin
      }
      assert(false, "We should have a valid origin to get to this point")
      return .init()
    }
    return origin
  }

  public func requestPermissions(
    _ coinType: BraveWallet.CoinType,
    accounts: [String],
    completion: @escaping RequestPermissionsCallback
  ) {
    Task { @MainActor in
      let permissionRequestManager = WalletProviderPermissionRequestsManager.shared
      let origin = getOrigin()

      if permissionRequestManager.hasPendingRequest(for: origin, coinType: coinType) {
        completion(.requestInProgress, nil)
        return
      }

      let isPrivate = self.isPrivate

      // Check if eth permissions already exist for this origin and if they don't, ensure the user allows
      // ethereum/solana provider access
      let walletPermissions =
        origin.url.map { Domain.walletPermissions(forUrl: $0, coin: coinType) ?? [] } ?? []
      if walletPermissions.isEmpty {
        switch coinType {
        case .eth:
          if !Preferences.Wallet.allowEthProviderAccess.value {
            completion(.internal, nil)
            return
          }
        case .sol:
          if !Preferences.Wallet.allowSolProviderAccess.value {
            completion(.internal, nil)
            return
          }
        case .fil, .btc, .zec:
          // not supported
          fallthrough
        @unknown default:
          completion(.internal, nil)
          return
        }
      }

      guard !isPrivate else {
        completion(.internal, nil)
        return
      }
      switch coinType {
      case .eth, .sol:
        break  // only eth/sol supported for DApps.
      default:
        completion(.internal, nil)
        return
      }
      let allowedAccounts = getAllowedAccounts(coinType, accounts: accounts) ?? []
      if !allowedAccounts.isEmpty {
        // account(s) requested are allowed
        completion(.none, allowedAccounts)
        return
      }

      // add permission request to the queue
      let request = permissionRequestManager.beginRequest(
        for: origin,
        accounts: accounts,
        coinType: coinType,
        providerHandler: completion,
        completion: { response in
          switch response {
          case .granted(let accounts):
            AppReviewManager.shared.processSubCriteria(for: .walletConnectedDapp)
            completion(.none, accounts)
          case .rejected:
            completion(.none, [])
          }
          self.tabDelegate?.updateURLBarWalletButton()
        }
      )

      tabDappStore.latestPendingPermissionRequest = request
      self.tabDelegate?.showWalletNotification(self, origin: origin)
    }
  }

  /// Returns the selected account if present in `allowedAccounts`, otherwise returns `allowedAccounts`
  func filterAllowedAccounts(
    _ allowedAccounts: [String],
    selectedAccount: String?
  ) -> [String] {
    if let selectedAccount = selectedAccount, allowedAccounts.contains(selectedAccount) {
      return [selectedAccount]
    }
    return allowedAccounts
  }

  /// Fetches all allowed accounts for the current origin.
  func getAllowedAccounts(_ type: BraveWallet.CoinType, accounts: [String]) -> [String]? {
    // For Ethereum, locked status is checked in `EthereumProviderImpl::GetAllowedAccounts`
    // For Solana, locked status is checked in `SolanaProviderImpl::Connect` before
    // calling `IsAccountAllowed` delegate method.
    guard let originURL = url?.origin.url,
      let permittedAccountAddresses = Domain.walletPermissions(forUrl: originURL, coin: type)
    else {
      return []
    }
    // Filter `accounts` requested to return only those permitted
    return accounts.filter { address in
      permittedAccountAddresses.contains(where: { permittedAddress in
        permittedAddress.caseInsensitiveCompare(address) == .orderedSame
      })
    }
  }

  func isAccountAllowed(_ type: BraveWallet.CoinType, account: String) -> Bool {
    guard let allowedAccounts = getAllowedAccounts(type, accounts: [account]) else {
      return false
    }
    return allowedAccounts.contains(account)
  }

  func walletInteractionDetected() {
    // No usage for iOS
  }

  func showWalletBackup() {
    // No usage for iOS
  }

  func unlockWallet() {
    // No usage for iOS
  }

  func showWalletOnboarding() {
    showPanel()
  }

  func isTabVisible() -> Bool {
    tabDelegate?.isTabVisible(self) ?? false
  }

  func isPermissionDenied(_ type: BraveWallet.CoinType) -> Bool {
    switch type {
    case .eth:
      return !Preferences.Wallet.allowEthProviderAccess.value
    case .sol:
      return !Preferences.Wallet.allowSolProviderAccess.value
    case .fil, .btc, .zec:
      return true
    @unknown default:
      return true
    }
  }

  func showAccountCreation(_ coin: BraveWallet.CoinType) {
    let privateMode = self.isPrivate
    guard let keyringService = BraveWallet.KeyringServiceFactory.get(privateMode: privateMode)
    else {
      return
    }
    Task { @MainActor in
      let origin = getOrigin()

      // check if we receive account creation request without a wallet setup
      let isWalletCreated = await keyringService.isWalletCreated()
      if !isWalletCreated {
        // Wallet is not setup. User must onboard / setup wallet first.
        self.tabDelegate?.showWalletNotification(self, origin: origin)
        return
      }

      let accountCreationRequestManager = WalletProviderAccountCreationRequestManager.shared

      // check if same account creation request exists
      guard !accountCreationRequestManager.hasPendingRequest(for: origin, coinType: coin)
      else { return }

      // store the account creation request
      accountCreationRequestManager.beginRequest(for: origin, coinType: coin) { [weak self] in
        self?.tabDelegate?.updateURLBarWalletButton()
      }
      // show wallet notification
      self.tabDelegate?.showWalletNotification(self, origin: origin)
    }
  }

  func isSolanaAccountConnected(_ account: String) -> Bool {
    tabDappStore.solConnectedAddresses.contains(account)
  }

  func addSolanaConnectedAccount(_ account: String) {
    Task { @MainActor in
      tabDappStore.solConnectedAddresses.insert(account)
    }
  }

  func removeSolanaConnectedAccount(_ account: String) {
    Task { @MainActor in
      tabDappStore.solConnectedAddresses.remove(account)
    }
  }

  func clearSolanaConnectedAccounts() {
    Task { @MainActor in
      tabDappStore.solConnectedAddresses = .init()
      await updateSolanaProperties()
    }
  }
}

extension Tab: BraveWalletEventsListener {
  func emitEthereumEvent(_ event: Web3ProviderEvent) {
    guard !isPrivate,
      Preferences.Wallet.defaultEthWallet.value == Preferences.Wallet.WalletType.brave.rawValue,
      let webView = self.webView
    else {
      return
    }
    var arguments: [Any] = [event.name]
    if let eventArgs = event.arguments {
      arguments.append(eventArgs)
    }
    webView.evaluateSafeJavaScript(
      functionName: "window.ethereum.emit",
      args: arguments,
      contentWorld: EthereumProviderScriptHandler.scriptSandbox,
      completion: nil
    )
  }

  func chainChangedEvent(chainId: String) {
    guard !isPrivate else { return }

    Task { @MainActor in
      // chain change might not apply to this origin when assigning
      // new default network / nil origin: brave-browser #30344
      guard let provider = walletEthProvider,
        case let providerChainId = await provider.chainId(),
        providerChainId == chainId
      else { return }

      // Temporary fix for #5404
      // Ethereum properties have been updated correctly, however, dapp is not updated unless there is a reload
      // We keep the same as Metamask, that, we will reload tab on chain changes.
      emitEthereumEvent(.ethereumChainChanged(chainId: chainId))
      await updateEthereumProperties()
      reload()
    }
  }

  func accountsChangedEvent(accounts: [String]) {
    guard !isPrivate else { return }

    emitEthereumEvent(.ethereumAccountsChanged(accounts: accounts))

    Task {
      await updateEthereumProperties()
    }
  }

  @MainActor
  func updateEthereumProperties() async {
    guard !isPrivate,
      let keyringService = BraveWallet.KeyringServiceFactory.get(privateMode: false),
      Preferences.Wallet.defaultEthWallet.value == Preferences.Wallet.WalletType.brave.rawValue
    else {
      return
    }

    // Turn an optional value into a string (or quoted string in case of the value being a string) or
    // return `undefined`
    func valueOrUndefined<T>(_ value: T?) -> String {
      switch value {
      case .some(let string as String):
        return "\"\(string)\""
      case .some(let value):
        return "\(value)"
      case .none:
        return "undefined"
      }
    }
    guard let webView = webView, let provider = walletEthProvider else {
      return
    }

    let chainId = await provider.chainId()
    await webView.evaluateSafeJavaScript(
      functionName: "window.ethereum.chainId = \"\(chainId)\"",
      contentWorld: EthereumProviderScriptHandler.scriptSandbox,
      asFunction: false
    )

    let networkVersion = valueOrUndefined(Int(chainId.removingHexPrefix, radix: 16))
    await webView.evaluateSafeJavaScript(
      functionName: "window.ethereum.networkVersion = \"\(networkVersion)\"",
      contentWorld: EthereumProviderScriptHandler.scriptSandbox,
      asFunction: false
    )

    let isKeyringLocked = await keyringService.isLocked()
    let selectedAccount: String
    if isKeyringLocked {
      // Check for locked status before assigning account address.
      // `getAllowedAccounts` is not async, can't check locked status.
      selectedAccount = valueOrUndefined(Optional<String>.none)
    } else {
      let allAccounts = await keyringService.allAccounts()
      let allEthAccounts = allAccounts.accounts.filter { $0.coin == .eth }
      if let allowedAccounts = getAllowedAccounts(.eth, accounts: allEthAccounts.map(\.address)) {
        let selectedAccountForCoin = allAccounts.ethDappSelectedAccount
        let filteredAllowedAccounts = filterAllowedAccounts(
          allowedAccounts,
          selectedAccount: selectedAccountForCoin?.address
        )
        selectedAccount = valueOrUndefined(filteredAllowedAccounts.first)
      } else {
        selectedAccount = valueOrUndefined(Optional<String>.none)
      }
    }
    await webView.evaluateSafeJavaScript(
      functionName: "window.ethereum.selectedAddress = \(selectedAccount)",
      contentWorld: EthereumProviderScriptHandler.scriptSandbox,
      asFunction: false
    )
  }

  func messageEvent(subscriptionId: String, result: MojoBase.Value) {
    let eventArgs = MojoBase.Value(dictionaryValue: [
      "type": MojoBase.Value(stringValue: "eth_subscription"),
      "data": MojoBase.Value(dictionaryValue: [
        "subscription": MojoBase.Value(stringValue: subscriptionId),
        "result": result,
      ]),
    ])
    emitEthereumEvent(.init("message", arguments: eventArgs.jsonObject))
  }
}

extension Tab: BraveWalletSolanaEventsListener {
  func accountChangedEvent(account: String?) {
    Task {
      if let webView = webView {
        let script: String
        if let account = account {
          script =
            "if (\(UserScriptManager.walletSolanaNameSpace).solanaWeb3) { window.solana.emit('accountChanged', new \(UserScriptManager.walletSolanaNameSpace).solanaWeb3.PublicKey('\(account.htmlEntityEncodedString)')) }"
        } else {
          script = "window.solana.emit('accountChanged')"
        }
        await webView.evaluateSafeJavaScript(
          functionName: script,
          contentWorld: .page,
          asFunction: false
        )
      }
      await updateSolanaProperties()
    }
  }

  func disconnectEvent() {
    emitSolanaEvent(.disconnect)
  }

  func emitSolanaEvent(_ event: Web3ProviderEvent) {
    guard Preferences.Wallet.defaultSolWallet.value == Preferences.Wallet.WalletType.brave.rawValue,
      let webView = webView
    else {
      return
    }
    Task {
      var arguments: [Any] = [event.name]
      if let eventArgs = event.arguments {
        arguments.append(eventArgs)
      }
      await webView.evaluateSafeJavaScript(
        functionName: "window.solana.emit",
        args: arguments,
        contentWorld: .page
      )
    }
  }

  @MainActor func updateSolanaProperties() async {
    guard Preferences.Wallet.defaultSolWallet.value == Preferences.Wallet.WalletType.brave.rawValue,
      let webView = webView,
      let provider = walletSolProvider
    else {
      return
    }
    let isConnected = await provider.isConnected()
    await webView.evaluateSafeJavaScript(
      functionName: "window.solana.isConnected = \(isConnected)",
      contentWorld: .page,
      asFunction: false
    )
    // publicKey
    if let keyringService = walletKeyringService,
      let publicKey = await keyringService.allAccounts().solDappSelectedAccount?.address,
      self.isSolanaAccountConnected(publicKey)
    {
      await webView.evaluateSafeJavaScript(
        functionName: """
          if (\(UserScriptManager.walletSolanaNameSpace).solanaWeb3) {
            window.__firefox__.execute(function($) {
              window.solana.publicKey = $.deepFreeze(
                new \(UserScriptManager.walletSolanaNameSpace).solanaWeb3.PublicKey('\(publicKey.htmlEntityEncodedString)')
              );
            });
          }
          """,
        contentWorld: .page,
        asFunction: false
      )
    }
  }
}

extension Tab: BraveWalletKeyringServiceObserver {
  func walletCreated() {
  }

  func walletRestored() {
  }

  func walletReset() {
    reload()
    tabDelegate?.updateURLBarWalletButton()
  }

  func locked() {
    Task {
      await updateEthereumProperties()
      await updateSolanaProperties()
    }
  }

  func unlocked() {
    guard let origin = url?.origin,
      let keyringService = walletKeyringService
    else { return }
    Task { @MainActor in
      // check domain already has some permitted accounts for this Tab's URLOrigin
      let permissionRequestManager = WalletProviderPermissionRequestsManager.shared
      let allAccounts = await keyringService.allAccounts().accounts
      for coin in WalletConstants.supportedCoinTypes(.dapps) {
        let allAccountsForCoin = allAccounts.filter { $0.coin == coin }
        if permissionRequestManager.hasPendingRequest(for: origin, coinType: coin) {
          let pendingRequests = permissionRequestManager.pendingRequests(
            for: origin,
            coinType: coin
          )
          let accounts = getAllowedAccounts(coin, accounts: allAccountsForCoin.map(\.address)) ?? []
          if !accounts.isEmpty {
            for request in pendingRequests {
              // cancel the requests if `allowedAccounts` is not empty for this domain
              permissionRequestManager.cancelRequest(request)
              // let wallet provider know we have allowed accounts for this domain
              request.providerHandler?(.none, accounts)
            }
          }
        }
      }
    }
  }

  func backedUp() {
  }

  func accountsChanged() {
  }

  func autoLockMinutesChanged() {
  }

  func selectedWalletAccountChanged(account: BraveWallet.AccountInfo) {
  }

  func selectedDappAccountChanged(coin: BraveWallet.CoinType, account: BraveWallet.AccountInfo?) {
  }

  func accountsAdded(addedAccounts: [BraveWallet.AccountInfo]) {
  }
}
