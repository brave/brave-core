// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
@_spi(ChromiumWebViewAccess) import Web

public class WalletWebUIHelper: NSObject, TabObserver, WalletPageHandler {
  private(set) weak var tab: (any TabState)?
  private var showWalletBackUpHandler: (() -> Void)?
  private var unlockWalletHandler: (() -> Void)?
  private var showOnboardingHandler: ((Bool) -> Void)?

  public init?(
    tab: some TabState,
    showWalletBackUpHandler: (() -> Void)?,
    unlockWalletHandler: (() -> Void)?,
    showOnboardingHandler: ((Bool) -> Void)?
  ) {
    if !tab.isChromiumTab || !FeatureList.kBraveWalletWebUIIOS.enabled {
      return nil
    }
    self.tab = tab
    self.showWalletBackUpHandler = showWalletBackUpHandler
    self.unlockWalletHandler = unlockWalletHandler
    self.showOnboardingHandler = showOnboardingHandler
    super.init()
    tab.addObserver(self)
  }

  deinit {
    tab?.removeObserver(self)
  }

  // MARK: - TabObserver

  public func tabDidCreateWebView(_ tab: some TabState) {
    BraveWebView.from(tab: tab)?.walletPageHandler = self
  }

  public func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }

  // MARK: - WalletPageHandler
  public func showWalletBackupUI() {
    showWalletBackUpHandler?()
  }

  public func unlockWalletUI() {
    unlockWalletHandler?()
  }

  public func showOnboarding(_ isNewWallet: Bool) {
    showOnboardingHandler?(isNewWallet)
  }
}

extension TabDataValues {
  private struct WalletWebUIHelperKey: TabDataKey {
    static var defaultValue: WalletWebUIHelper? = nil
  }

  public var walletWebUIHelper: WalletWebUIHelper? {
    get { self[WalletWebUIHelperKey.self] }
    set { self[WalletWebUIHelperKey.self] = newValue }
  }
}
