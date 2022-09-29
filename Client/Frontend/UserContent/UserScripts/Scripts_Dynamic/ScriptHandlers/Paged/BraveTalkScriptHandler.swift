// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Shared
import BraveShared
import BraveCore

private let log = Logger.browserLogger

class BraveTalkScriptHandler: TabContentScript {
  private weak var tab: Tab?
  private weak var rewards: BraveRewards?
  private var callback: ((Any?, String?) -> Void)?

  required init(tab: Tab, rewards: BraveRewards) {
    self.tab = tab
    self.rewards = rewards

    tab.rewardsEnabledCallback = { [weak self] success in
      self?.callback?(success, nil)
    }
  }

  static let scriptName = "BraveTalkScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .page
  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }
    return WKUserScript.create(source: secureScript(handlerName: messageHandlerName,
                                                    securityToken: scriptId,
                                                    script: script),
                               injectionTime: .atDocumentStart,
                               forMainFrameOnly: false,
                               in: scriptSandbox)
  }()

  func userContentController(
    _ userContentController: WKUserContentController,
    didReceiveScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      return
    }
    
    let allowedHosts = DomainUserScript.braveTalkHelper.associatedDomains

    guard let requestHost = message.frameInfo.request.url?.host,
      allowedHosts.contains(requestHost),
      message.frameInfo.isMainFrame
    else {
      log.error("Backup search request called from disallowed host")
      replyHandler(nil, nil)
      return
    }

    handleBraveRequestAdsEnabled(replyHandler: replyHandler)
  }

  private func handleBraveRequestAdsEnabled(replyHandler: @escaping (Any?, String?) -> Void) {

    guard let rewards = rewards, !PrivateBrowsingManager.shared.isPrivateBrowsing else {
      replyHandler(true, nil)
      return
    }

    if rewards.isEnabled {
      replyHandler(true, nil)
      return
    }

    // If rewards are disabled we show a Rewards panel,
    // The `callback` will be called from other place.
    if let tab = tab {
      callback = replyHandler
      tab.tabDelegate?.showRequestRewardsPanel(tab)
    }
  }
}
