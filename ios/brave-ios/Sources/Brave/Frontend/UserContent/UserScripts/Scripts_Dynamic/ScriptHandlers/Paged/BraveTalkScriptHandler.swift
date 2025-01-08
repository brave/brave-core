// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#if canImport(BraveTalk)

import Foundation
import WebKit
import Shared
import BraveShared
import BraveCore
import os.log
import BraveTalk

class BraveTalkScriptHandler: TabContentScript {
  private weak var rewards: BraveRewards?
  private var rewardsEnabledReplyHandler: ((Any?, String?) -> Void)?
  private let launchNativeBraveTalk: (_ tab: Tab?, _ room: String, _ token: String) -> Void

  required init(
    rewards: BraveRewards,
    launchNativeBraveTalk: @escaping (_ tab: Tab?, _ room: String, _ token: String) -> Void
  ) {
    self.rewards = rewards
    self.launchNativeBraveTalk = launchNativeBraveTalk
  }

  static let scriptName = "BraveTalkScript"
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

  private struct Payload: Decodable {
    enum Kind: Decodable {
      case braveRequestAdsEnabled
      case launchNativeBraveTalk(String)
    }
    var kind: Kind
    var securityToken: String

    enum CodingKeys: String, CodingKey {
      case kind
      case url
      case securityToken = "securityToken"
    }

    init(from decoder: Decoder) throws {
      enum RawKindKey: String, Decodable {
        case braveRequestAdsEnabled
        case launchNativeBraveTalk
      }
      let container = try decoder.container(keyedBy: CodingKeys.self)
      let kind = try container.decode(RawKindKey.self, forKey: .kind)
      self.securityToken = try container.decode(String.self, forKey: .securityToken)
      switch kind {
      case .launchNativeBraveTalk:
        let url = try container.decode(String.self, forKey: .url)
        self.kind = .launchNativeBraveTalk(url)
      case .braveRequestAdsEnabled:
        self.kind = .braveRequestAdsEnabled
      }
    }
  }

  func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
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
      Logger.module.error("Backup search request called from disallowed host")
      replyHandler(nil, nil)
      return
    }

    guard let json = try? JSONSerialization.data(withJSONObject: message.body, options: []),
      let payload = try? JSONDecoder().decode(Payload.self, from: json)
    else {
      return
    }

    switch payload.kind {
    case .braveRequestAdsEnabled:
      handleBraveRequestAdsEnabled(tab: tab, replyHandler)
    case .launchNativeBraveTalk(let url):
      guard let components = URLComponents(string: url),
        case let room = String(components.path.dropFirst(1)),
        let jwt = components.queryItems?.first(where: { $0.name == "jwt" })?.value
      else {
        return
      }
      launchNativeBraveTalk(tab, room, jwt)
      replyHandler(nil, nil)
    }
  }

  private func handleBraveRequestAdsEnabled(
    tab: Tab,
    _ replyHandler: @escaping (Any?, String?) -> Void
  ) {
    guard let rewards = rewards, tab.isPrivate != true else {
      replyHandler(false, nil)
      return
    }

    if rewards.isEnabled {
      replyHandler(true, nil)
      return
    }

    // If rewards are disabled we show a Rewards panel,
    // The `rewardsEnabledReplyHandler` will be called from other place.
    rewardsEnabledReplyHandler = replyHandler
    tab.rewardsEnabledCallback = { [weak self] success in
      self?.rewardsEnabledReplyHandler?(success, nil)
    }
    tab.tabDelegate?.showRequestRewardsPanel(tab)
  }
}

#endif
