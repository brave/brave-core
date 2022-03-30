// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit

private let log = Logger.browserLogger

struct ReadyState: Codable {
  let securityToken: String
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
      log.error("Error Decoding ReadyState: \(error)")
    }

    return nil
  }
  
  private enum CodingKeys: String, CodingKey {
    case securityToken = "securitytoken"
    case state
  }
}

class ReadyStateScriptHelper: TabContentScript {
  private weak var tab: Tab?
  private var debounceTimer: Timer?

  required init(tab: Tab) {
    self.tab = tab
  }
  
  class func name() -> String {
    return "ReadyStateScriptHelper"
  }

  func scriptMessageHandlerName() -> String? {
    return "ReadyState_\(UserScriptManager.messageHandlerTokenString)"
  }

  func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage, replyHandler: (Any?, String?) -> Void) {
    
    defer { replyHandler(nil, nil) }

    guard let readyState = ReadyState.from(message: message) else {
      log.error("Invalid Ready State")
      return
    }
    
    guard readyState.securityToken == UserScriptManager.securityTokenString else {
      log.error("Invalid or Missing security token")
      return
    }
    
    tab?.onPageReadyStateChanged?(readyState.state)
  }
}
