// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit

public class DeAmpHelper: TabContentScript {
  private struct DeAmpDTO: Decodable {
    let securityToken: String
    let destURL: URL
  }
  
  private weak var tab: Tab?
  
  init(tab: Tab) {
    self.tab = tab
  }
  
  static func name() -> String {
    return "DeAmpHelper"
  }
  
  static func scriptMessageHandlerName() -> String {
    return ["deAmpHelper", UserScriptManager.messageHandlerTokenString].joined(separator: "_")
  }
  
  func scriptMessageHandlerName() -> String? {
    return Self.scriptMessageHandlerName()
  }
  
  func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage, replyHandler: @escaping (Any?, String?) -> Void) {
    do {
      let data = try JSONSerialization.data(withJSONObject: message.body)
      let dto = try JSONDecoder().decode(DeAmpDTO.self, from: data)
      
      guard dto.securityToken == UserScriptManager.securityTokenString else {
        assertionFailure("Invalid security token. Fix the `RequestBlocking.js` script")
        replyHandler(false, nil)
        return
      }
      
      // Check that the destination is not the same as the previousURL
      // or that previousURL is nil which indicates circular loop cause by a client side redirect
      // Also check that our window url does not match the previously committed url
      // or that previousURL is nil which indicates as circular loop caused by a server side redirect
      let shouldRedirect = dto.destURL != tab?.previousComittedURL && tab?.committedURL != tab?.previousComittedURL
      replyHandler(shouldRedirect, nil)
    } catch {
      assertionFailure("Invalid type of message. Fix the `RequestBlocking.js` script")
      replyHandler(false, nil)
    }
  }
}
