// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import WebKit
import Shared
import BraveShields
import os.log

class SiteStateListenerScriptHandler: TabContentScript {
  struct MessageDTO: Decodable {
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
    return WKUserScript(source: secureScript(handlerName: messageHandlerName,
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
      
      if let pageData = tab.currentPageData {
        Task { @MainActor in
          let domain = pageData.domain(persistent: !tab.isPrivate)
          guard domain.isShieldExpected(.AdblockAndTp, considerAllShieldsOption: true) else { return }
          
          let models = await AdBlockStats.shared.cosmeticFilterModels(forFrameURL: frameURL, domain: domain)
          let setup = try self.makeSetup(from: models, isAggressive: domain.blockAdsAndTrackingLevel.isAggressive)
          let script = try ScriptFactory.shared.makeScript(for: .selectorsPoller(setup))
          
          try await webView.evaluateSafeJavaScriptThrowing(
            functionName: script.source,
            frame: message.frameInfo,
            contentWorld: CosmeticFiltersScriptHandler.scriptSandbox,
            asFunction: false
          )
        }
      }
    } catch {
      assertionFailure("Invalid type of message. Fix the `SiteStateListenerScript.js` script")
      Logger.module.error("\(error.localizedDescription)")
    }
  }
  
  @MainActor private func makeSetup(from modelTuples: [AdBlockStats.CosmeticFilterModelTuple], isAggressive: Bool) throws -> UserScriptType.SelectorsPollerSetup {
    var standardSelectors: Set<String> = []
    var aggressiveSelectors: Set<String> = []
    var styleSelectors: [String: Set<String>] = [:]
    
    for modelTuple in modelTuples {
      for (key, values) in modelTuple.model.styleSelectors {
        styleSelectors[key] = styleSelectors[key]?.union(Set(values)) ?? Set(values)
      }
      
      if modelTuple.isAlwaysAggressive {
        aggressiveSelectors = aggressiveSelectors.union(modelTuple.model.hideSelectors)
      } else {
        standardSelectors = standardSelectors.union(modelTuple.model.hideSelectors)
      }
    }
    
    let styleSelectorObjects = styleSelectors.map { selector, rules -> UserScriptType.SelectorsPollerSetup.StyleSelectorEntry in
      UserScriptType.SelectorsPollerSetup.StyleSelectorEntry(
        selector: selector, rules: rules
      )
    }
    
    return UserScriptType.SelectorsPollerSetup(
      hideFirstPartyContent: isAggressive,
      genericHide: modelTuples.contains { $0.model.genericHide },
      firstSelectorsPollingDelayMs: nil,
      switchToSelectorsPollingThreshold: 1000,
      fetchNewClassIdRulesThrottlingMs: 100,
      aggressiveSelectors: aggressiveSelectors,
      standardSelectors: standardSelectors,
      styleSelectors: Set(styleSelectorObjects)
    )
  }
}
