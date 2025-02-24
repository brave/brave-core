// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AIChat
import Foundation
import WebKit
import os.log

class BraveLeoScriptHandler: NSObject, TabContentScript {
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

    return WKUserScript(
      source: secureScript(
        handlerNamesMap: [
          "$<message_handler>": messageHandlerName,
          "$<getMainArticle>": getMainArticle,
          "$<getPDFDocument>": getPDFDocument,
        ],
        securityToken: scriptId,
        script: script
      ),
      injectionTime: .atDocumentEnd,
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
  }
}

class BraveLeoScriptTabHelper: AIChatWebDelegate {
  weak var tab: Tab?

  init(tab: Tab) {
    self.tab = tab
  }

  var isLoading: Bool {
    tab?.loading == true
  }

  var title: String? {
    tab?.title
  }

  var url: URL? {
    tab?.url
  }

  func getPageContentType() async -> String? {
    guard let webView = tab?.webView else { return nil }
    return try? await webView.evaluateSafeJavaScriptThrowing(
      functionName: "document.contentType",
      contentWorld: BraveLeoScriptHandler.scriptSandbox,
      escapeArgs: false,
      asFunction: false
    ) as? String
  }

  @MainActor
  func getMainArticle() async -> String? {
    guard let webView = tab?.webView else { return nil }
    do {
      let articleText =
        try await webView.evaluateSafeJavaScriptThrowing(
          functionName: "window.__firefox__.\(BraveLeoScriptHandler.getMainArticle)",
          args: [BraveLeoScriptHandler.scriptId],
          contentWorld: BraveLeoScriptHandler.scriptSandbox,
          asFunction: true
        ) as? String
      return articleText
    } catch {
      Logger.module.error("Error Retrieving Main Article From Page: \(error.localizedDescription)")
      return nil
    }
  }

  @MainActor
  func getPDFDocument() async -> String? {
    guard let webView = tab?.webView else { return nil }
    // po webView.perform(Selector("_methodDescription"))
    if webView.responds(to: Selector(("_dataForDisplayedPDF"))),
      let pdfData = webView.perform(Selector(("_dataForDisplayedPDF"))).takeUnretainedValue()
        as? Data
    {
      return pdfData.base64EncodedString()
    }

    // The below will only get called if WebKit changes their Private-API
    // This hasn't been the case for over 5 years
    // So it's highly unlikely the below code gets called

    // We failed to grab the PDF from WebKit
    // We can only ever handle `POST` PDFs from WKNavigationDelegate
    // In such a case, just don't bother FOR NOW
    // Later on we can hook up WKNavigationDelegate to this class, and steal the request and fetch the PDF that way
    // Unfortunately though, that WILL cache miss for sure so we may take a performance hit due to IPC
    // For that reason, just attempt to `GET` the PDF below from JS cache

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
      """,
      contentWorld: BraveLeoScriptHandler.scriptSandbox
    ) as? String
  }

  @MainActor
  func getPrintViewPDF() async -> Data? {
    guard let viewPrintFormatter = tab?.webView?.viewPrintFormatter() else { return nil }
    // No article text. Attempt to parse the page as a PDF/Image
    let render = UIPrintPageRenderer()
    render.addPrintFormatter(viewPrintFormatter, startingAtPageAt: 0)

    let page = CGRect(x: 0, y: 0, width: 595.2, height: 841.8)  // A4, 72 dpi
    let printable = page.insetBy(dx: 0, dy: 0)

    render.setValue(NSValue(cgRect: page), forKey: "paperRect")
    render.setValue(NSValue(cgRect: printable), forKey: "printableRect")

    // 4. Create PDF context and draw
    let pdfData = NSMutableData()
    UIGraphicsBeginPDFContextToData(pdfData, CGRect.zero, nil)
    for i in 0..<render.numberOfPages {
      UIGraphicsBeginPDFPage()
      let bounds = UIGraphicsGetPDFContextBounds()
      render.drawPage(at: i, in: bounds)
    }

    UIGraphicsEndPDFContext()
    return pdfData as Data
  }
}
