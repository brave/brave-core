// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Shared

private let log = Logger.braveCoreLogger

class SiteStateListenerContentHelper: TabContentScript {
  private struct MessageDTO: Decodable {
    struct MessageDTOData: Decodable, Hashable {
      let windowURL: String
    }
    
    let securityToken: String
    let data: MessageDTOData
  }
  
  static func name() -> String {
    return "SiteStateListenerContentHelper"
  }
  
  static func scriptMessageHandlerName() -> String {
    return ["siteStateListenerContentHelper", UserScriptManager.messageHandlerTokenString].joined(separator: "_")
  }
  
  private weak var tab: Tab?
  
  init(tab: Tab) {
    self.tab = tab
  }
  
  func scriptMessageHandlerName() -> String? {
    return Self.scriptMessageHandlerName()
  }
  
  func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage, replyHandler: @escaping (Any?, String?) -> Void) {
    defer { replyHandler(nil, nil) }
    guard let tab = tab, let webView = tab.webView else {
      assertionFailure("Should have a tab set")
      return
    }
    
    do {
      let data = try JSONSerialization.data(withJSONObject: message.body)
      let dto = try JSONDecoder().decode(MessageDTO.self, from: data)
      
      guard dto.securityToken == UserScriptManager.securityTokenString else {
        assertionFailure("Invalid security token. Fix the `RequestBlocking.js` script")
        return
      }
      
      guard let frameURL = URL(string: dto.data.windowURL) else {
        return
      }
      
      if let frameEvaluations = tab.frameEvaluations[frameURL] {
        for frameEvaluation in frameEvaluations {
          webView.evaluateSafeJavaScript(
            functionName: frameEvaluation.source,
            frame: frameEvaluation.frameInfo,
            contentWorld: .cosmeticFiltersSandbox,
            asFunction: false,
            completion: { _, error in
              guard let error = error else { return }
              log.error(error)
            }
          )
        }
        
        tab.frameEvaluations.removeValue(forKey: frameURL)
      }
    } catch {
      assertionFailure("Invalid type of message. Fix the `Site.js` script")
      log.error(error)
    }
  }
}
