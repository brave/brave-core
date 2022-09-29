// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Shared

private let log = Logger.braveCoreLogger

class SiteStateListenerScriptHandler: TabContentScript {
  private struct MessageDTO: Decodable {
    struct MessageDTOData: Decodable, Hashable {
      let windowURL: String
    }
    
    let data: MessageDTOData
  }
  
  private weak var tab: Tab?
  
  init(tab: Tab) {
    self.tab = tab
  }
  
  static let scriptName = "SiteStateListenerScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .defaultClient
  private static let downloadName = "\(scriptName)_\(uniqueID)"
  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }
    return WKUserScript.create(source: secureScript(handlerName: messageHandlerName,
                                                    securityToken: scriptId,
                                                    script: script),
                               injectionTime: .atDocumentStart,
                               forMainFrameOnly: false,
                               in: scriptSandbox)
  }()
  
  func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage, replyHandler: @escaping (Any?, String?) -> Void) {
    defer { replyHandler(nil, nil) }
    
    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      return
    }
    
    guard let tab = tab, let webView = tab.webView else {
      assertionFailure("Should have a tab set")
      return
    }
    
    do {
      let data = try JSONSerialization.data(withJSONObject: message.body)
      let dto = try JSONDecoder().decode(MessageDTO.self, from: data)
      
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
