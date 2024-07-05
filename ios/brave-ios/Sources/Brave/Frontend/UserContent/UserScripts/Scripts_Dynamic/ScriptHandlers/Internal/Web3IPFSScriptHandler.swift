// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit
import os.log

protocol Web3IPFSScriptHandlerDelegate: AnyObject {
  func web3IPFSDecisionHandler(_ proceed: Bool, originalURL: URL)
}

class Web3IPFSScriptHandler: TabContentScript {
  weak var delegate: Web3IPFSScriptHandlerDelegate?
  var originalURL: URL?
  fileprivate weak var tab: Tab?

  required init(tab: Tab) {
    self.tab = tab
  }

  static let scriptName = "Web3IPFSScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .page
  static let userScript: WKUserScript? = nil

  func userContentController(
    _ userContentController: WKUserContentController,
    didReceive message: WKScriptMessage
  ) async -> (Any?, String?) {
    guard let params = message.body as? [String: String], let originalURL = originalURL else {
      return (nil, nil)
    }

    if params["type"] == "IPFSDisable" {
      delegate?.web3IPFSDecisionHandler(false, originalURL: originalURL)
    } else if params["type"] == "IPFSProceed" {
      delegate?.web3IPFSDecisionHandler(true, originalURL: originalURL)
    } else {
      assertionFailure("Invalid message: \(message.body)")
    }

    return (nil, nil)
  }
}
