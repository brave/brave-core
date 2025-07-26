//
//  TabWebUIHandler.swift
//  Brave
//
//  Created by Brandon T on 2025-07-25.
//

import BraveCore

public protocol TabWebUIDelegate: AnyObject {
  func unlockWallet(_ tab: some TabState)
}

class TabWebUIHandler: NSObject, BraveWebUIMessagingTabHelperDelegate {
  private weak var tab: ChromiumTabState?
  weak var delegate: TabWebUIDelegate?

  init(tab: ChromiumTabState) {
    self.tab = tab
    super.init()
  }

  func webUIUnlockWallet() {
    guard let tab = tab else { return }
    delegate?.unlockWallet(tab)
  }
}
