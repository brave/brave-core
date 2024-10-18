// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit

public class DeAmpScriptHandler: TabContentScript {
  private struct DeAmpDTO: Decodable {
    let securityToken: String
    let destURL: URL
  }

  static let scriptName = "DeAmpScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .defaultClient
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
    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      replyHandler(false, nil)
      return
    }

    do {
      let data = try JSONSerialization.data(withJSONObject: message.body)
      let dto = try JSONDecoder().decode(DeAmpDTO.self, from: data)

      // Check that the destination is not the same as the previousURL
      // or that previousURL is nil which indicates circular loop cause by a client side redirect
      // Also check that our window url does not match the previously committed url
      // or that previousURL is nil which indicates as circular loop caused by a server side redirect
      let shouldRedirect =
        dto.destURL != tab.previousComittedURL && tab.committedURL != tab.previousComittedURL
      replyHandler(shouldRedirect, nil)
    } catch {
      assertionFailure("Invalid type of message. Fix the `RequestBlocking.js` script")
      replyHandler(false, nil)
    }
  }
}
