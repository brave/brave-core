// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Foundation
import Shared
import WebKit
import os.log

private struct CosmeticFiltersResource: Decodable {
  struct Info: Decodable, Hashable {
    let sourceURL: String
    let ids: [String]
    let classes: [String]
  }

  let securityToken: String
  let data: Info
}

/// This handler receives a list of ids and selectors for a given frame for which it is then able to inject scripts and css rules in order to hide certain elements
///
/// The ids and classes are collected in the `SelectorsPollerScript.js` file.
class CosmeticFiltersScriptHandler: TabContentScript {

  static let scriptName = "SelectorsPollerScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .defaultClient
  static let userScript: WKUserScript? = nil

  private weak var tab: Tab?

  init(tab: Tab) {
    self.tab = tab
  }

  func userContentController(
    _ userContentController: WKUserContentController,
    didReceive message: WKScriptMessage
  ) async -> (Any?, String?) {

    if !verifyMessage(message: message) {
      assertionFailure("Invalid security token. Fix the `RequestBlocking.js` script")
      return (nil, nil)
    }

    do {
      let data = try JSONSerialization.data(withJSONObject: message.body)
      let resource = try JSONDecoder().decode(CosmeticFiltersResource.self, from: data)

      guard let frameURL = URL(string: resource.data.sourceURL) else {
        return (nil, nil)
      }

      return await processMessage(frameURL: frameURL, resource: resource)
    } catch {
      assertionFailure("Invalid type of message. Fix the `RequestBlocking.js` script")
    }

    return (nil, nil)
  }

  @MainActor
  private func processMessage(
    frameURL: URL,
    resource: CosmeticFiltersResource
  ) async -> (Any?, String?) {
    let domain = Domain.getOrCreate(
      forUrl: frameURL,
      persistent: self.tab?.isPrivate == true ? false : true
    )
    let cachedEngines = AdBlockGroupsManager.shared.cachedEngines(for: domain)

    let selectorArrays = await cachedEngines.asyncCompactMap {
      cachedEngine -> (selectors: Set<String>, isAlwaysAggressive: Bool)? in
      do {
        guard
          let selectors = try await cachedEngine.selectorsForCosmeticRules(
            frameURL: frameURL,
            ids: resource.data.ids,
            classes: resource.data.classes
          )
        else {
          return nil
        }

        return (selectors, cachedEngine.type.isAlwaysAggressive)
      } catch {
        Logger.module.error("\(error.localizedDescription)")
        return nil
      }
    }

    var standardSelectors: Set<String> = []
    var aggressiveSelectors: Set<String> = []
    for tuple in selectorArrays {
      if tuple.isAlwaysAggressive {
        aggressiveSelectors = aggressiveSelectors.union(tuple.selectors)
      } else {
        standardSelectors = standardSelectors.union(tuple.selectors)
      }
    }

    return (
      [
        "aggressiveSelectors": Array(aggressiveSelectors),
        "standardSelectors": Array(standardSelectors),
      ],
      nil
    )
  }
}
