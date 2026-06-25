// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Foundation
import Shared
import Web
import WebKit
import os.log

class SiteStateListenerScriptHandler: TabContentScript {

  static let scriptName = "SiteStateListenerScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .defaultClient
  private static let downloadName = "\(scriptName)_\(uniqueID)"
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
      forMainFrameOnly: false,
      in: scriptSandbox
    )
  }()

  func tab(
    _ tab: some TabState,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    defer { replyHandler(nil, nil) }

    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      return
    }

    guard let frameURL = URLOrigin(wkSecurityOrigin: message.frameInfo.securityOrigin).url else {
      return
    }

    Task { @MainActor in
      guard
        let (setup, proceduralActions) = await tab.cosmeticFilteringTabHelper?
          .cosmeticFilteringSetup(for: frameURL)
      else {
        return
      }
      let script = try ScriptFactory.shared.makeScript(
        for: .contentCosmetic(setup, proceduralActions: proceduralActions)
      )
      try await tab.evaluateJavaScript(
        functionName: script.source,
        frame: message.frameInfo,
        contentWorld: CosmeticFiltersScriptHandler.scriptSandbox,
        asFunction: false
      )
    }
  }
}
