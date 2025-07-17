// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Foundation
import Shared
import Web
import WebKit
import os.log

/// This handler receives a list of ids and selectors for a given frame for which it is then able to inject scripts and css rules in order to hide certain elements
///
/// The ids and classes are collected in the `content_cosmetic_ios.js` file.
class CosmeticFiltersScriptHandler: TabContentScript {
  struct CosmeticFiltersDTO: Decodable {
    struct CosmeticFiltersDTOData: Decodable, Hashable {
      let windowOrigin: String
      let ids: [String]
      let classes: [String]
    }

    let securityToken: String
    let data: CosmeticFiltersDTOData
  }

  static let scriptName = "content_cosmetic_ios"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .defaultClient
  static let userScript: WKUserScript? = nil

  func tab(
    _ tab: some TabState,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    if !verifyMessage(message: message) {
      assertionFailure("Invalid security token. Fix the `content_cosmetic_ios.js` script")
      replyHandler(nil, nil)
      return
    }

    do {
      let data = try JSONSerialization.data(withJSONObject: message.body)
      let dto = try JSONDecoder().decode(CosmeticFiltersDTO.self, from: data)

      guard let frameURL = URL(string: dto.data.windowOrigin) else {
        replyHandler(nil, nil)
        return
      }

      Task { @MainActor in
        let cachedEngines = AdBlockGroupsManager.shared.cachedEngines(
          isAdBlockEnabled: tab.braveShieldsHelper?.shieldLevel(
            for: frameURL,
            isPrivate: tab.isPrivate,
            considerAllShieldsOption: true
          ).isEnabled ?? true
        )

        let selectorArrays = await cachedEngines.asyncCompactMap {
          cachedEngine -> (selectors: Set<String>, isAlwaysAggressive: Bool)? in
          do {
            guard
              let selectors = try await cachedEngine.selectorsForCosmeticRules(
                frameURL: frameURL,
                ids: dto.data.ids,
                classes: dto.data.classes
              )
            else {
              return nil
            }

            return await (selectors, cachedEngine.type.isAlwaysAggressive)
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

        // cache blocked selectors
        if let url = tab.visibleURL {
          tab.contentBlocker?.cacheSelectors(
            for: url,
            standardSelectors: standardSelectors,
            aggressiveSelectors: aggressiveSelectors
          )
        }

        replyHandler(
          [
            "aggressiveSelectors": Array(aggressiveSelectors),
            "standardSelectors": Array(standardSelectors),
          ],
          nil
        )
      }
    } catch {
      assertionFailure("Invalid type of message. Fix the `content_cosmetic_ios.js` script")
      replyHandler(nil, nil)
    }
  }
}
