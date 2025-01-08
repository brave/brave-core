// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShields
import Foundation
import Shared
import WebKit
import os.log

class SiteStateListenerScriptHandler: TabContentScript {
  struct MessageDTO: Decodable {
    struct MessageDTOData: Decodable, Hashable {
      let windowURL: String
      let windowOriginURL: String
    }

    let data: MessageDTOData
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
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    defer { replyHandler(nil, nil) }

    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      return
    }

    guard let webView = tab.webView else {
      assertionFailure("Should have a tab set")
      return
    }

    do {
      let data = try JSONSerialization.data(withJSONObject: message.body)
      let dto = try JSONDecoder().decode(MessageDTO.self, from: data)

      guard let frameURL = URL(string: dto.data.windowOriginURL) else {
        return
      }

      Task { @MainActor in
        if let pageData = tab.currentPageData {
          let domain = pageData.domain(persistent: !tab.isPrivate)
          guard domain.globalBlockAdsAndTrackingLevel.isEnabled else {
            return
          }

          let models = await AdBlockGroupsManager.shared.cosmeticFilterModels(
            forFrameURL: frameURL,
            domain: domain
          )
          let setup = try self.makeSetup(
            from: models,
            isAggressive: domain.globalBlockAdsAndTrackingLevel.isAggressive
          )

          // Join the procedural actions
          // Note: they can't be part of `UserScriptType.SelectorsPollerSetup`
          // As this is encoded and therefore the JSON will be escaped
          var proceduralActions: Set<String> = []
          for modelTuple in models {
            proceduralActions = proceduralActions.union(modelTuple.model.proceduralActions)
          }
          let script = try ScriptFactory.shared.makeScript(
            for: .selectorsPoller(setup, proceduralActions: proceduralActions)
          )

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

  @MainActor private func makeSetup(
    from modelTuples: [AdBlockGroupsManager.CosmeticFilterModelTuple],
    isAggressive: Bool
  ) throws -> UserScriptType.SelectorsPollerSetup {
    var standardSelectors: Set<String> = []
    var aggressiveSelectors: Set<String> = []

    for modelTuple in modelTuples {
      if modelTuple.isAlwaysAggressive {
        aggressiveSelectors = aggressiveSelectors.union(modelTuple.model.hideSelectors)
      } else {
        standardSelectors = standardSelectors.union(modelTuple.model.hideSelectors)
      }
    }

    return UserScriptType.SelectorsPollerSetup(
      hideFirstPartyContent: isAggressive,
      genericHide: modelTuples.contains { $0.model.genericHide },
      firstSelectorsPollingDelayMs: nil,
      switchToSelectorsPollingThreshold: 1000,
      fetchNewClassIdRulesThrottlingMs: 100,
      aggressiveSelectors: aggressiveSelectors,
      standardSelectors: standardSelectors
    )
  }
}
