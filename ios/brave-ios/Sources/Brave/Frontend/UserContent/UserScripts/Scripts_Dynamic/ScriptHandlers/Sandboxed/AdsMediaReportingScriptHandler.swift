// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit
import BraveCore
import os.log

class AdsMediaReportingScriptHandler: TabContentScript {
  let rewards: BraveRewards
  weak var tab: Tab?

  init(rewards: BraveRewards, tab: Tab) {
    self.rewards = rewards
    self.tab = tab
  }

  static let scriptName = "AdsMediaReportingScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "adsMediaReporting"
  static let scriptSandbox: WKContentWorld = .defaultClient
  static let userScript: WKUserScript? = nil

  func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage, replyHandler: (Any?, String?) -> Void) {
    defer { replyHandler(nil, nil) }
    
    if !verifyMessage(message: message, securityToken: UserScriptManager.securityToken) {
      assertionFailure("Missing required security token.")
      return
    }
    
    guard let body = message.body as? [String: AnyObject] else {
      return
    }

    if let isPlaying = body["data"] as? Bool, rewards.isEnabled {
      guard let tab = tab else { return }
      if isPlaying {
        rewards.reportMediaStarted(tabId: Int(tab.rewardsId))
      } else {
        rewards.reportMediaStopped(tabId: Int(tab.rewardsId))
      }
    }
  }
}
