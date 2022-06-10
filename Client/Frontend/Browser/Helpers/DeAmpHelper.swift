// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit

public class DeAmpHelper: TabContentScript {
  private struct DeAmpDTO: Decodable {
    enum CodingKeys: String, CodingKey {
      case destURL, securityToken
    }
    
    let securityToken: String
    let destURL: URL
    
    init(from decoder: Decoder) throws {
      let container = try decoder.container(keyedBy: CodingKeys.self)
      self.securityToken = try container.decode(String.self, forKey: .securityToken)
      let destURLString = try container.decode(String.self, forKey: .destURL)
      
      guard let destURL = URL(string: destURLString) else {
        throw DecodingError.dataCorrupted(DecodingError.Context(codingPath: decoder.codingPath, debugDescription: "`resourceURL` is not a valid URL. Fix the `RequestBlocking.js` script"))
      }
      
      self.destURL = destURL
    }
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
      // or that previousURL is nil
      let shouldRedirect = dto.destURL != tab?.previousComittedURL
      replyHandler(shouldRedirect, nil)
    } catch {
      assertionFailure("Invalid type of message. Fix the `RequestBlocking.js` script")
      replyHandler(false, nil)
    }
  }
}
