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
  private weak var tab: Tab?
  private weak var rewards: BraveRewards?
  private var rewardsEnabledReplyHandler: CheckedContinuation<(Any?, String?), Never>?
  private let launchNativeBraveTalk: (_ tab: Tab?, _ room: String, _ token: String) -> Void

  required init(
    tab: Tab,
    rewards: BraveRewards,
    launchNativeBraveTalk: @escaping (_ tab: Tab?, _ room: String, _ token: String) -> Void
  ) {
    self.tab = tab
    self.rewards = rewards
    self.launchNativeBraveTalk = launchNativeBraveTalk

    tab.rewardsEnabledCallback = { [weak self] success in
      self?.rewardsEnabledReplyHandler?.resume(returning: (success, nil))
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

  func userContentController(
    _ userContentController: WKUserContentController,
    didReceive message: WKScriptMessage
  ) async -> (Any?, String?) {
    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      return (nil, nil)
    }

    let allowedHosts = DomainUserScript.braveTalkHelper.associatedDomains

    guard let requestHost = message.frameInfo.request.url?.host,
      allowedHosts.contains(requestHost),
      message.frameInfo.isMainFrame
    else {
      Logger.module.error("Backup search request called from disallowed host")
      return (nil, nil)
    }

    guard let json = try? JSONSerialization.data(withJSONObject: message.body, options: []),
      let payload = try? JSONDecoder().decode(Payload.self, from: json)
    else {
      return (nil, nil)
    }

    switch payload.kind {
    case .braveRequestAdsEnabled:
      return await withCheckedContinuation { [weak self] continuation in
        guard let self = self else {
          continuation.resume(returning: (nil, nil))
          return
        }

        handleBraveRequestAdsEnabled(continuation)
      }
    case .launchNativeBraveTalk(let url):
      guard let components = URLComponents(string: url),
        case let room = String(components.path.dropFirst(1)),
        let jwt = components.queryItems?.first(where: { $0.name == "jwt" })?.value
      else {
        return (nil, nil)
      }
      launchNativeBraveTalk(tab, room, jwt)
      return (nil, nil)
    }
  }

  private func handleBraveRequestAdsEnabled(
    _ continuation: CheckedContinuation<(Any?, String?), Never>
  ) {
    guard let rewards = rewards, tab?.isPrivate != true else {
      continuation.resume(returning: (false, nil))
      return
    }

    if rewards.isEnabled {
      continuation.resume(returning: (true, nil))
      return
    }

    // If rewards are disabled we show a Rewards panel,
    // The `rewardsEnabledReplyHandler` will be called from other place.
    if let tab = tab {
      rewardsEnabledReplyHandler = continuation
      tab.tabDelegate?.showRequestRewardsPanel(tab)
    }
  }
}

#endif
