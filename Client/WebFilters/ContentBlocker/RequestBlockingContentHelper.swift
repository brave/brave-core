// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit
import BraveCore

private let log = Logger.braveCoreLogger

class RequestBlockingContentHelper: TabContentScript {
  private struct RequestBlockingDTO: Decodable {
    struct RequestBlockingDTOData: Decodable, Hashable {
      let resourceType: AdblockEngine.ResourceType
      let resourceURL: URL
      let sourceURL: URL
    }
    
    let securityToken: String
    let data: RequestBlockingDTOData
  }
  
  static func name() -> String {
    return "RequestBlockingContentHelper"
  }
  
  static func scriptMessageHandlerName() -> String {
    return ["requestBlockingContentHelper", UserScriptManager.messageHandlerTokenString].joined(separator: "_")
  }
  
  func scriptMessageHandlerName() -> String? {
    return Self.scriptMessageHandlerName()
  }
  
  func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage, replyHandler: @escaping (Any?, String?) -> Void) {
    do {
      let data = try JSONSerialization.data(withJSONObject: message.body)
      let dto = try JSONDecoder().decode(RequestBlockingDTO.self, from: data)
      
      guard dto.securityToken == UserScriptManager.securityTokenString else {
        assertionFailure("Invalid security token. Fix the `RequestBlocking.js` script")
        replyHandler(false, nil)
        return
      }
      
      AdBlockStats.shared.shouldBlock(
        requestURL: dto.data.resourceURL,
        sourceURL: dto.data.sourceURL,
        resourceType: dto.data.resourceType
      ) { shouldBlock in
        assertIsMainThread("Result should happen on the main thread")
        replyHandler(shouldBlock, nil)
      }
    } catch {
      assertionFailure("Invalid type of message. Fix the `RequestBlocking.js` script")
      replyHandler(false, nil)
    }
  }
}
