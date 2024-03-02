// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import os.log
import AIChat

class BraveLeoScriptHandler: NSObject, TabContentScript {
  fileprivate weak var tab: Tab?

  init(tab: Tab) {
    self.tab = tab
    super.init()
  }
  
  static let getMainArticle = "getMainArticle\(uniqueID)"
  static let getPDFDocument = "getPDFDocument\(uniqueID)"

  static let scriptName = "BraveLeoScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .defaultClient
  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }
    
    return WKUserScript(source: secureScript(handlerNamesMap: ["$<message_handler>": messageHandlerName,
                                                               "$<getMainArticle>": getMainArticle,
                                                               "$<getPDFDocument>": getPDFDocument],
                                             securityToken: scriptId,
                                             script: script),
                        injectionTime: .atDocumentEnd,
                        forMainFrameOnly: true,
                        in: scriptSandbox)
  }()

  func userContentController(_ userContentController: WKUserContentController, didReceiveScriptMessage message: WKScriptMessage, replyHandler: (Any?, String?) -> Void) {
    defer { replyHandler(nil, nil) }
    
    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      return
    }
  }
}

extension BraveLeoScriptHandler: AIChatJavascript {
  
  @MainActor
  static func getPageContentType(webView: WKWebView) async -> String? {
    return try? await webView.evaluateSafeJavaScriptThrowing(functionName: "document.contentType", contentWorld: Self.scriptSandbox, escapeArgs: false, asFunction: false) as? String
  }
  
  @MainActor
  static func getMainArticle(webView: WKWebView) async -> String? {
    do {
      let articleText = try await webView.evaluateSafeJavaScriptThrowing(functionName: "window.__firefox__.\(getMainArticle)",
                                                                         args: [Self.scriptId],
                                                                         contentWorld: Self.scriptSandbox,
                                                                         asFunction: true) as? String
      return articleText
    } catch {
      Logger.module.error("Error Retrieving Main Article From Page: \(error.localizedDescription)")
      return nil
    }
  }
  
  @MainActor
  static func getPDFDocument(webView: WKWebView) async -> String? {
    // Pages containing PDF cannot contain injected Javascript
    // So we must execute an inline script
    return try? await webView.callAsyncJavaScript(
    """
    const buffer = await window.fetch(window.location.href, {
      method: 'GET',
      priority: 'high'
    });

    var array = new Uint8Array(await buffer.arrayBuffer());
    var binaryString = new Array(array.length);

    for(var i = 0; i < array.length; i++) {
      binaryString[i] = String.fromCharCode(array[i]);
    }

    return window.btoa(binaryString.join(''));
    """, contentWorld: Self.scriptSandbox) as? String
  }
}
