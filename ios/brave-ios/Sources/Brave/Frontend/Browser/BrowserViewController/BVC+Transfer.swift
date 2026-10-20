// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation
import Preferences
import UIKit

extension BrowserViewController: TransferAppRouting {
  /// The app-wide Brave Transfer coordinator, created on first use while
  /// the BraveTransfer feature is enabled. This window becomes the one that
  /// shows offers and toasts.
  @discardableResult
  func setUpBraveTransfer() -> TransferAppCoordinator? {
    let coordinator = TransferAppCoordinator.sharedIfEnabled(syncAPI: profileController.syncAPI)
    coordinator?.router = self
    return coordinator
  }

  func handleTransferHandoff(_ id: UUID) {
    setUpBraveTransfer()?.handoff.handle(id)
  }

  func openTransferPage() {
    popToBVC(isAnimated: false)
    if let tab = tabManager.getTabForURL(.webUI.transfer, isPrivate: false) {
      tabManager.selectTab(tab)
    } else {
      tabManager.addTabAndSelect(URLRequest(url: .webUI.transfer), isPrivate: false)
    }
  }

  func openTransferLink(_ url: URL) {
    openURLInNewTab(url, isPrivate: false, isPrivileged: false)
  }

  func openSyncSettings() {
    let controller: UIViewController
    if Preferences.Chromium.syncEnabled.value {
      controller = SyncSettingsTableViewController(
        isModallyPresented: true,
        braveCoreMain: profileController,
        windowProtection: windowProtection
      )
    } else {
      controller = SyncWelcomeViewController(
        braveCore: profileController,
        windowProtection: windowProtection,
        isModallyPresented: true
      )
    }
    var presenter: UIViewController = self
    while let presented = presenter.presentedViewController {
      presenter = presented
    }
    presenter.present(UINavigationController(rootViewController: controller), animated: true)
  }

  var transferPresentingViewController: UIViewController { self }
}
