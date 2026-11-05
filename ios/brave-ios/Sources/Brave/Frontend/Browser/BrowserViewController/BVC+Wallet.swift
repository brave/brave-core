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
import Web
import os.log

extension WalletStore {
  /// Creates a WalletStore based on the profile
  static func from(
    ipfsApi: IpfsAPI,
    profile: any Profile
  ) -> WalletStore? {
    guard
      let keyringService = BraveWallet.KeyringServiceFactory.get(profile: profile),
      let rpcService = BraveWallet.JsonRpcServiceFactory.get(profile: profile),
      let assetRatioService = BraveWallet.AssetRatioServiceFactory.get(profile: profile),
      let walletService = BraveWallet.ServiceFactory.get(profile: profile),
      let txService = BraveWallet.TxServiceFactory.get(profile: profile),
      let ethTxManagerProxy = BraveWallet.EthTxManagerProxyFactory.get(profile: profile),
      let solTxManagerProxy = BraveWallet.SolanaTxManagerProxyFactory.get(profile: profile),
      let bitcoinWalletService = BraveWallet.BitcoinWalletServiceFactory.get(profile: profile),
      let zcashWalletService = BraveWallet.ZCashWalletServiceFactory.get(profile: profile),
      let cardanoWalletService = BraveWallet.CardanoWalletServiceFactory.get(profile: profile)
    else {
      Logger.module.error("Failed to load wallet. One or more services were unavailable")
      return nil
    }
    return WalletStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWalletAPI.blockchainRegistry,
      txService: txService,
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      ipfsApi: ipfsApi,
      bitcoinWalletService: bitcoinWalletService,
      zcashWalletService: zcashWalletService,
      cardanoWalletService: cardanoWalletService
    )
  }
}

extension CryptoStore {
  /// Creates a CryptoStore based on the profile
  static func from(
    ipfsApi: IpfsAPI,
    profile: any Profile
  ) -> CryptoStore? {
    guard
      let keyringService = BraveWallet.KeyringServiceFactory.get(profile: profile),
      let rpcService = BraveWallet.JsonRpcServiceFactory.get(profile: profile),
      let assetRatioService = BraveWallet.AssetRatioServiceFactory.get(profile: profile),
      let walletService = BraveWallet.ServiceFactory.get(profile: profile),
      let txService = BraveWallet.TxServiceFactory.get(profile: profile),
      let ethTxManagerProxy = BraveWallet.EthTxManagerProxyFactory.get(profile: profile),
      let solTxManagerProxy = BraveWallet.SolanaTxManagerProxyFactory.get(profile: profile),
      let bitcoinWalletService = BraveWallet.BitcoinWalletServiceFactory.get(profile: profile),
      let zcashWalletService = BraveWallet.ZCashWalletServiceFactory.get(profile: profile),
      let cardanoWalletService = BraveWallet.CardanoWalletServiceFactory.get(profile: profile)
    else {
      Logger.module.error("Failed to load wallet. One or more services were unavailable")
      return nil
    }
    return CryptoStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWalletAPI.blockchainRegistry,
      txService: txService,
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      ipfsApi: ipfsApi,
      bitcoinWalletService: bitcoinWalletService,
      zcashWalletService: zcashWalletService,
      cardanoWalletService: cardanoWalletService
    )
  }
}

extension BrowserViewController {
  /// Initializes a new WalletStore for displaying the wallet, setting up an observer to notify
  /// when the pending request is updated so we can update the wallet url bar button.
  func newWalletStore() -> WalletStore? {
    if !profileController.braveWalletAPI.isAllowed {
      return nil
    }
    var profile = profileController.profile
    if privateBrowsingManager.isPrivateBrowsing {
      profile = profile.offTheRecordProfile
    }
    guard
      let walletStore = WalletStore.from(ipfsApi: profileController.ipfsAPI, profile: profile)
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
      webImageDownloader: profileController.webImageDownloader
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
    if let url = tabManager.selectedTab?.visibleURL {
      if InternalURL.isValid(url: url) {
        select(url: destinationURL, isUserDefinedURLNavigation: false)
      } else {
        tabManager.addTabAndSelect(
          URLRequest(url: destinationURL),
          isPrivate: privateBrowsingManager.isPrivateBrowsing
        )
      }
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
      webImageDownloader: profileController.webImageDownloader,
      presentingContext: presentWalletWithContext
    )
    walletHostingController.delegate = self

    switch presentWalletWithContext {
    case .settings:
      // Dismiss Wallet Panel first, then present Wallet
      self.dismiss(animated: true) { [weak self] in
        self?.present(walletHostingController, animated: true)
      }
    default:
      panel.present(walletHostingController, animated: true)
    }
  }
}

// MARK: Wallet WebUI action handlers
extension BrowserViewController {
  func showApprovePanelUI(tab: (any TabState)?) {
    guard let tab,
      let origin = tab.lastCommittedURL?.origin,
      let tabDappStore = tab.wallet?.tabDappStore
    else {
      return
    }
    presentWalletPanel(from: origin, with: tabDappStore)
  }

  func showWalletBackupUI() {
    presentNativeWallet(webUIAction: .backup)
  }

  func unlockWalletUI() {
    presentNativeWallet(webUIAction: .unlock)
  }

  func showOnboarding(_ isNewWallet: Bool) {
    presentNativeWallet(
      webUIAction: .onboarding(isNewAccount: isNewWallet)
    )
  }

  func openWalletHome() {
    openDestinationURL(.webUI.wallet.home)
  }

  func scanAddressQRCode(completion: @escaping (String) -> Void) {
    struct ScannerContainer: View {
      var onFound: (String) -> Void
      @State private var address: String = ""
      @State private var complete: Bool = false

      var body: some View {
        AddressQRCodeScannerView(coin: BraveWallet.CoinType.eth, address: $address)
          .onChange(of: address) { _, newValue in
            guard !complete, !newValue.isEmpty else { return }
            complete = true
            onFound(newValue)
          }
          .onDisappear {
            guard !complete else { return }
            complete = true
            onFound("")
          }
      }
    }
    let vc = UIHostingController(rootView: ScannerContainer(onFound: completion))
    vc.modalPresentationStyle = .fullScreen
    present(vc, animated: true)
  }
}
