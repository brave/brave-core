// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import WebKit

class ContextMenuHandler: NSObject {
  var touchPoint = CGPoint()

  struct Elements {
    let image: URL?
    let title: String?
    let alt: String?
  }

  fileprivate weak var tab: Tab?

  fileprivate(set) var elements: Elements?

  required init(tab: Tab) {
    super.init()
    self.tab = tab
  }
}

extension ContextMenuHandler: TabContentScript {
  static var scriptName = "ContextMenuHandler"
  static var scriptId = UUID().uuidString
  static var messageHandlerName = "contextMenuMessageHandler"
  static var scriptSandbox: WKContentWorld = .page

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
      injectionTime: .atDocumentEnd,
      forMainFrameOnly: false,
      in: scriptSandbox
    )
  }()

  func userContentController(
    _ userContentController: WKUserContentController,
    didReceiveScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    guard let data = message.body as? [String: AnyObject] else { return }

    if let x = data["touchX"] as? Double, let y = data["touchY"] as? Double {
      touchPoint = CGPoint(x: x, y: y)
    }

    var imageURL: URL?
    if let urlString = data["image"] as? String {
      imageURL = URL(string: urlString)
    }

    guard let imageURL else {
      elements = nil
      return
    }

    let title = data["title"] as? String
    let alt = data["alt"] as? String
    elements = Elements(image: imageURL, title: title, alt: alt)
  }
}
