// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit
import os.log

struct ReadyState: Codable {
  let state: State

  enum State: String, Codable {
    // Page State
    case loading
    case interactive
    case complete
    case loaded

    // History State
    case pushstate
    case replacestate
    case popstate
  }

  public static func from(message: WKScriptMessage) -> ReadyState? {
    if !JSONSerialization.isValidJSONObject(message.body) {
      return nil
    }

    do {
      let data = try JSONSerialization.data(withJSONObject: message.body, options: [])
      return try JSONDecoder().decode(ReadyState.self, from: data)
    } catch {
      Logger.module.error("Error Decoding ReadyState: \(error.localizedDescription)")
    }

    return nil
  }

  private enum CodingKeys: String, CodingKey {
    case state
  }
}

class ReadyStateScriptHandler: TabContentScript {
  private var debounceTimer: Timer?

  static let scriptName = "ReadyStateScript"
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
      forMainFrameOnly: true,
      in: scriptSandbox
    )
  }()

  func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    defer { replyHandler(nil, nil) }

    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      return
    }

    guard let readyState = ReadyState.from(message: message) else {
      Logger.module.error("Invalid Ready State")
      return
    }

    tab.onPageReadyStateChanged?(readyState.state)
  }
}
