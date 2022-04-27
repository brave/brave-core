// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveWallet
import struct Shared.InternalURL
import struct Shared.Logger
import BraveCore
import SwiftUI
import BraveUI
import Data
import BraveShared

private let log = Logger.browserLogger

extension WalletStore {
  /// Creates a WalletStore based on whether or not the user is in Private Mode
  static func from(privateMode: Bool) -> WalletStore? {
    guard
      let keyringService = BraveWallet.KeyringServiceFactory.get(privateMode: privateMode),
      let rpcService = BraveWallet.JsonRpcServiceFactory.get(privateMode: privateMode),
      let assetRatioService = BraveWallet.AssetRatioServiceFactory.get(privateMode: privateMode),
      let walletService = BraveWallet.ServiceFactory.get(privateMode: privateMode),
      let swapService = BraveWallet.SwapServiceFactory.get(privateMode: privateMode),
      let txService = BraveWallet.TxServiceFactory.get(privateMode: privateMode),
      let ethTxManagerProxy = BraveWallet.EthTxManagerProxyFactory.get(privateMode: privateMode)
    else {
      log.error("Failed to load wallet. One or more services were unavailable")
      return nil
    }
    return WalletStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      swapService: swapService,
      blockchainRegistry: BraveCoreMain.blockchainRegistry,
      txService: txService,
      ethTxManagerProxy: ethTxManagerProxy
    )
  }
}

extension BrowserViewController {
  func presentWalletPanel() {
    let privateMode = PrivateBrowsingManager.shared.isPrivateBrowsing
    guard let walletStore = WalletStore.from(privateMode: privateMode) else {
      return
    }
    let controller = WalletPanelHostingController(walletStore: walletStore, origin: getOrigin())
    controller.delegate = self
    let popover = PopoverController(contentController: controller, contentSizeBehavior: .autoLayout)
    popover.present(from: topToolbar.locationView.walletButton, on: self, completion: nil)
    scrollController.showToolbars(animated: true)
  }
}

extension WalletPanelHostingController: PopoverContentComponent {}

extension BrowserViewController: BraveWalletDelegate {
  func openWalletURL(_ destinationURL: URL) {
    if presentedViewController != nil {
      // dismiss to show the new tab
      self.dismiss(animated: true)
    }
    if let url = tabManager.selectedTab?.url, InternalURL.isValid(url: url) {
      select(url: destinationURL, visitType: .link)
    } else {
      _ = tabManager.addTabAndSelect(
        URLRequest(url: destinationURL),
        isPrivate: PrivateBrowsingManager.shared.isPrivateBrowsing
      )
    }
  }
}

extension BrowserViewController: BraveWalletProviderDelegate {
  func showPanel() {
    // TODO: Show ad-like notification prompt before calling `presentWalletPanel`
    presentWalletPanel()
  }

  func getOrigin() -> URLOrigin {
    guard let origin = tabManager.selectedTab?.url?.origin else {
      assert(false, "We should have a valid origin to get to this point")
      return .init()
    }
    return origin
  }

  func requestEthereumPermissions(_ completion: @escaping BraveWalletProviderResultsCallback) {
    Task { @MainActor in
      let permissionRequestManager = WalletProviderPermissionRequestsManager.shared
      let origin = getOrigin()
      
      if permissionRequestManager.hasPendingRequest(for: origin, coinType: .eth) {
        completion([], .userRejectedRequest, "A request is already in progress")
        return
      }
      
      let isPrivate = PrivateBrowsingManager.shared.isPrivateBrowsing
      
      // Check if eth permissions already exist for this origin and if they don't, ensure the user allows
      // ethereum provider access
      let ethPermissions = origin.url.map { Domain.ethereumPermissions(forUrl: $0) ?? [] } ?? []
      if ethPermissions.isEmpty, !Preferences.Wallet.allowEthereumProviderAccountRequests.value {
        completion([], .userRejectedRequest, "User rejected request")
        return
      }
      
      guard let walletStore = WalletStore.from(privateMode: isPrivate) else {
        completion([], .internalError, "")
        return
      }
      let (accounts, status, message) = await allowedAccounts(false)
      if status != .success {
        completion([], status, message)
        return
      }
      if status == .success && !accounts.isEmpty {
        completion(accounts, .success, "")
        return
      }
      
      let request = permissionRequestManager.beginRequest(for: origin, coinType: .eth, completion: { response in
        switch response {
        case .granted(let accounts):
          completion(accounts, .success, "")
        case .rejected:
          completion([], .userRejectedRequest, "User rejected request")
        }
      })

      let permissions = WalletHostingViewController(
        walletStore: walletStore,
        presentingContext: .requestEthererumPermissions(request),
        onUnlock: {
          Task { @MainActor in
            // If the user unlocks their wallet and we already have permissions setup they do not
            // go through the regular flow
            let (accounts, status, _) = await self.allowedAccounts(false)
            if status == .success, !accounts.isEmpty {
              permissionRequestManager.cancelRequest(request)
              completion(accounts, .success, "")
              self.dismiss(animated: true)
              return
            }
          }
        }
      )
      permissions.delegate = self
      present(permissions, animated: true)
    }
  }

  func allowedAccounts(_ includeAccountsWhenLocked: Bool) async -> ([String], BraveWallet.ProviderError, String) {
    guard let selectedTab = tabManager.selectedTab else {
      return ([], .internalError, "Internal error")
    }
    updateURLBarWalletButton()
    return await selectedTab.allowedAccounts(includeAccountsWhenLocked)
  }

  func updateURLBarWalletButton() {
    topToolbar.locationView.walletButton.buttonState =
    tabManager.selectedTab?.isWalletIconVisible == true ? .active : .inactive
  }
}

extension Tab: BraveWalletEventsListener {
  func emitEthereumEvent(_ event: Web3ProviderEvent) {
    var arguments: [Any] = [event.name]
    if let eventArgs = event.arguments {
      arguments.append(eventArgs)
    }
    webView?.evaluateSafeJavaScript(
      functionName: "window.ethereum.emit",
      args: arguments,
      contentWorld: .page,
      completion: nil
    )
  }
  
  func chainChangedEvent(_ chainId: String) {
    Task { @MainActor in
      guard let provider = walletProvider,
            case let currentChainId = await provider.chainId(),
            chainId != currentChainId else { return }
      emitEthereumEvent(.ethereumChainChanged(chainId: chainId))
      updateEthereumProperties()
    }
  }
  
  func accountsChangedEvent(_ accounts: [String]) {
    emitEthereumEvent(.ethereumAccountsChanged(accounts: accounts))
    updateEthereumProperties()
  }

  @MainActor func allowedAccounts(_ includeAccountsWhenLocked: Bool) async -> ([String], BraveWallet.ProviderError, String) {
    func filterAccounts(
      _ accounts: [String],
      selectedAccount: String?
    ) -> [String] {
      if let selectedAccount = selectedAccount, accounts.contains(selectedAccount) {
        return [selectedAccount]
      }
      return accounts
    }
    // This method is called immediately upon creation of the wallet provider, which happens at tab
    // configuration, which means it may not be selected or ready yet.
    guard let keyringService = BraveWallet.KeyringServiceFactory.get(privateMode: false),
          let originURL = url?.origin.url else {
      return ([], .internalError, "Internal error")
    }
    let isLocked = await keyringService.isLocked()
    if !includeAccountsWhenLocked && isLocked {
      return ([], .success, "")
    }
    let selectedAccount = await keyringService.selectedAccount(.eth)
    let permissions = Domain.ethereumPermissions(forUrl: originURL)
    return (
      filterAccounts(permissions ?? [], selectedAccount: selectedAccount),
      .success,
      ""
    )
  }
  
  func updateEthereumProperties() {
    Task { @MainActor in
      /// Turn an optional value into a string (or quoted string in case of the value being a string) or
      /// return `undefined`
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
      guard let webView = webView, let provider = walletProvider else {
        return
      }
      let chainId = await provider.chainId()
      webView.evaluateSafeJavaScript(
        functionName: "window.ethereum.chainId = \"\(chainId)\"",
        contentWorld: .page,
        asFunction: false,
        completion: nil
      )
      let networkVersion = valueOrUndefined(Int(chainId.removingHexPrefix, radix: 16))
      webView.evaluateSafeJavaScript(
        functionName: "window.ethereum.networkVersion = \(networkVersion)",
        contentWorld: .page,
        asFunction: false,
        completion: nil
      )
      let selectedAccount = valueOrUndefined(await allowedAccounts(false).0.first)
      webView.evaluateSafeJavaScript(
        functionName: "window.ethereum.selectedAddress = \(selectedAccount)",
        contentWorld: .page,
        asFunction: false,
        completion: nil
      )
    }
  }
}
