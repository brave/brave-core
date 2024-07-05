// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Shared
import WebKit
import os.log

class RewardsReportingScriptHandler: TabContentScript {
  let rewards: BraveRewards

  init(rewards: BraveRewards) {
    self.rewards = rewards
  }

  static let scriptName = "RewardsReportingScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .page
  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }

    return WKUserScript(
      source: secureScript(
        handlerName: messageHandlerName,
        securityToken: scriptId,
        script: script
      ),
      injectionTime: .atDocumentStart,
      forMainFrameOnly: false,
      in: scriptSandbox
    )
  }()

  func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    defer { replyHandler(nil, nil) }
    struct Content: Decodable {
      var method: String
      var url: String
      var data: String?
      var referrerUrl: String?
    }

    if tab.isPrivate || !rewards.isEnabled {
      return
    }

    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      return
    }

    do {
      guard let body = message.body as? [String: AnyObject] else {
        return
      }

      if let body = body["data"] as? [String: AnyObject] {
        let json = try JSONSerialization.data(withJSONObject: body, options: [])
        var content = try JSONDecoder().decode(Content.self, from: json)

        guard let tabURL = tab.url else { return }

        if content.url.hasPrefix("//") {
          content.url = "\(tabURL.scheme ?? "http"):\(content.url)"
        }

        guard let url = URL(string: content.url) else { return }
        let refURL = URL(string: content.referrerUrl ?? "")
        rewards.reportXHRLoad(
          url: url,
          tabId: Int(tab.rewardsId),
          firstPartyURL: tabURL,
          referrerURL: refURL
        )
      }
    } catch {
      adsRewardsLog.error(
        "Failed to parse message from rewards reporting JS: \(error.localizedDescription)"
      )
    }
  }
}
