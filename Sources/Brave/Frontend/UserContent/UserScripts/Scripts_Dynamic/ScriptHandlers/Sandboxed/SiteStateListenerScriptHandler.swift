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
          if domain.areAllShieldsOff { return }
          
          let models = await AdBlockStats.shared.cosmeticFilterModels(forFrameURL: frameURL, domain: domain)
          let args = try self.makeArgs(from: models, frameURL: frameURL, isAggressive: ShieldPreferences.blockAdsAndTrackingLevel.isAggressive)
          let source = try ScriptFactory.shared.makeScriptSource(of: .selectorsPoller).replacingOccurrences(of: "$<args>", with: args)
          
          let secureSource = CosmeticFiltersScriptHandler.secureScript(
            handlerNamesMap: [
              "$<message_handler>": CosmeticFiltersScriptHandler.messageHandlerName,
              "$<partiness_message_handler>": URLPartinessScriptHandler.messageHandlerName
            ],
            securityToken: CosmeticFiltersScriptHandler.scriptId,
            script: source
          )
          
          webView.evaluateSafeJavaScript(
            functionName: secureSource,
            frame: message.frameInfo,
            contentWorld: .defaultClient,
            asFunction: false,
            completion: { _, error in
              guard let error = error else { return }
              Logger.module.error("\(error.localizedDescription)")
            }
          )
        }
      }
    } catch {
      assertionFailure("Invalid type of message. Fix the `Site.js` script")
      Logger.module.error("\(error.localizedDescription)")
    }
  }
  
  @MainActor private func makeArgs(from modelTuples: [CachedAdBlockEngine.CosmeticFilterModelTuple], frameURL: URL, isAggressive: Bool) throws -> String {
    var standardSelectors: Set<String> = []
    var agressiveSelectors: Set<String> = []
    var styleSelectors: [String: Set<String>] = [:]
    
    for modelTuple in modelTuples {
      for (key, values) in modelTuple.model.styleSelectors {
        styleSelectors[key] = styleSelectors[key]?.union(Set(values)) ?? Set(values)
      }
      
      if modelTuple.source.isAlwaysAgressive(given: FilterListStorage.shared.filterLists) {
        agressiveSelectors = agressiveSelectors.union(modelTuple.model.hideSelectors)
      } else {
        standardSelectors = standardSelectors.union(modelTuple.model.hideSelectors)
      }
    }
    
    let styleSelectorObjects = styleSelectors.map { selector, rules -> UserScriptType.SelectorsPollerSetup.StyleSelectorEntry in
      UserScriptType.SelectorsPollerSetup.StyleSelectorEntry(
        selector: selector, rules: rules
      )
    }
    
    // TODO: @JS #7352 Add UI and enable standard mode
    // Standard mode is controlled by the `hideFirstPartyContent` boolean
    // True means standard, false means aggresive
    // (i.e. we don't hide first party content on standard mode)
    let setup = UserScriptType.SelectorsPollerSetup(
      frameURL: frameURL,
      hideFirstPartyContent: isAggressive,
      genericHide: modelTuples.contains { $0.model.genericHide },
      firstSelectorsPollingDelayMs: nil,
      switchToSelectorsPollingThreshold: 1000,
      fetchNewClassIdRulesThrottlingMs: 100,
      agressiveSelectors: agressiveSelectors,
      standardSelectors: standardSelectors,
      styleSelectors: Set(styleSelectorObjects)
    )
    
    let encoder = JSONEncoder()
    let data = try encoder.encode(setup)
    return String(data: data, encoding: .utf8)!
  }
}
