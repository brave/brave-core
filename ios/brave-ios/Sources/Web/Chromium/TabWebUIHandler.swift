// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

public protocol TabWebUIDelegate: AnyObject {
  func showWalletApprovePanelUI(_ tab: some TabState)
  func showWalletBackupUI(_ tab: some TabState)
  func unlockWallet(_ tab: some TabState)
}

class TabWebUIHandler: NSObject, BraveWalletCommunicationProtocol {
  private weak var tab: ChromiumTabState?
  weak var delegate: TabWebUIDelegate?

  init(tab: ChromiumTabState) {
    self.tab = tab
    super.init()
  }

  func webUIShowWalletApprovePanelUI() {
    guard let tab = tab else { return }
    delegate?.showWalletApprovePanelUI(tab)
  }

  func webUIShowWalletBackupUI() {
    guard let tab = tab else { return }
    delegate?.showWalletBackupUI(tab)
  }

  func webUIUnlockWallet() {
    guard let tab = tab else { return }
    delegate?.unlockWallet(tab)
  }
}
