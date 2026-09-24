// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Preferences
import Shared
import Web
import WebKit
import os.log

class BraveSearchScriptHandler: TabContentScript {
  private let profile: LegacyBrowserProfile
  private weak var rewards: BraveRewards?

  /// Tracks how many in current browsing session the user has been prompted to set Brave Search as a default
  /// while on one of Brave Search websites.
  private static var canSetAsDefaultCounter = 0
  /// How many times user should be shown the default browser prompt on Brave Search websites.
  private let maxCountOfDefaultBrowserPromptsPerSession = 3
  /// How many times user is shown the default browser prompt in total, this does not reset between app launches.
  private let maxCountOfDefaultBrowserPromptsTotal = 10

  required init(profile: LegacyBrowserProfile, rewards: BraveRewards) {
    self.profile = profile
    self.rewards = rewards
  }

  static let scriptName = "BraveSearchScript"
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

  private enum Method: Int {
    case canSetBraveSearchAsDefault = 1
    case setBraveSearchDefault = 2
  }

  private struct MethodModel: Codable {
    enum CodingKeys: String, CodingKey {
      case methodId = "method_id"
    }

    let methodId: Int
  }

  func tab(
    _ tab: some TabState,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: (Any?, String?) -> Void
  ) {
    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      return
    }

    let allowedHosts = DomainUserScript.braveSearchHelper.associatedDomains

    guard let requestHost = message.frameInfo.request.url?.host,
      allowedHosts.contains(requestHost),
      message.frameInfo.isMainFrame
    else {
      Logger.module.error("Backup search request called from disallowed host")
      replyHandler(nil, nil)
      return
    }

    guard let data = try? JSONSerialization.data(withJSONObject: message.body, options: []),
      let method = try? JSONDecoder().decode(MethodModel.self, from: data).methodId,
      let helper = tab.braveSearch
    else {
      Logger.module.error("Failed to retrieve method id")
      replyHandler(nil, nil)
      return
    }

    switch method {
    case Method.canSetBraveSearchAsDefault.rawValue:
      let canSetBraveSearchAsDefault = helper.checkCanSetBraveSearchAsDefault()
      replyHandler(canSetBraveSearchAsDefault, nil)
    case Method.setBraveSearchDefault.rawValue:
      helper.setBraveSearchAsDefault()
      replyHandler(nil, nil)
    default:
      break
    }
  }
}
