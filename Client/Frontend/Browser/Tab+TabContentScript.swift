/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import WebKit
import BraveShared
import Shared
import os.log

extension TabContentScriptLoader {
  static var uniqueID: String {
    UUID().uuidString.replacingOccurrences(of: "-", with: "")
  }
  
  static var messageUUID: String {
    UUID().uuidString.replacingOccurrences(of: "-", with: "")
  }
  
  static func loadUserScript(named: String) -> String? {
    guard let path = Bundle.module.path(forResource: named, ofType: "js"),
          let source: String = try? String(contentsOfFile: path) else {
      Logger.module.error("Failed to load script: \(named).js")
      assertionFailure("Failed to Load Script: \(named).js")
      return nil
    }
    return source
  }
  
  static func secureScript(handlerName: String, securityToken: String, script: String) -> String {
    secureScript(handlerNamesMap: ["$<message_handler>": handlerName], securityToken: securityToken, script: script)
  }
  
  static func secureScript(handlerNamesMap: [String: String], securityToken: String, script: String) -> String {
    guard !script.isEmpty else {
      return script
    }
    
    var script = script
    for (obfuscatedHandlerName, actualHandlerName) in handlerNamesMap {
      script = script.replacingOccurrences(of: obfuscatedHandlerName, with: actualHandlerName)
    }
    
    let messageHandlers: String = {
      if !handlerNamesMap.isEmpty {
        let handlers = "[\(handlerNamesMap.map({"'\($0.value)'"}).joined(separator: ", "))]"
        return """
        \(handlers).forEach(e => {
            if (e && e.length > 0 && webkit.messageHandlers[e]) {
              Object.freeze(webkit.messageHandlers[e]);
              Object.freeze(webkit.messageHandlers[e].postMessage);
            }
          });
        """
      }
      return ""
    }()
    
    return """
    (function() {
      const SECURITY_TOKEN = '\(securityToken)';
    
      \(messageHandlers)
    
      \(script)
    })();
    """
  }
}

extension TabContentScript {
  func verifyMessage(message: WKScriptMessage) -> Bool {
    verifyMessage(message: message, securityToken: Self.scriptId)
  }
  
  func verifyMessage(message: WKScriptMessage, securityToken: String) -> Bool {
    (message.body as? [String: Any])?["securityToken"] as? String == securityToken
  }
}
