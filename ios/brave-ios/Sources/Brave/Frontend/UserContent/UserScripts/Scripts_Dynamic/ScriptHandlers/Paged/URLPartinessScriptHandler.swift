// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Foundation
import Shared
import WebKit
import os.log

/// This handler receives a list of urls for a given frame that need to determine its partiness (i.e. 1st part vs 3rd party)
///
/// The urls are collected in the `SelectorsPollerScript.js` file.
class URLPartinessScriptHandler: TabContentScript {
  struct PartinessDTO: Decodable {
    struct PartinessDTOData: Decodable, Hashable {
      let windowOrigin: String
      let urls: [String]
    }

    let securityToken: String
    let data: PartinessDTOData
  }

  static let scriptName = "URLPartinessScript"
  static let scriptId = CosmeticFiltersScriptHandler.scriptId
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .defaultClient
  static let userScript: WKUserScript? = nil

  private weak var tab: Tab?

  init(tab: Tab) {
    self.tab = tab
  }

  func userContentController(
    _ userContentController: WKUserContentController,
    didReceiveScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    if !verifyMessage(message: message) {
      assertionFailure("Invalid security token. Fix the `SelectorsPollerScript.js` script")
      replyHandler(nil, nil)
      return
    }

    do {
      let data = try JSONSerialization.data(withJSONObject: message.body)
      let dto = try JSONDecoder().decode(PartinessDTO.self, from: data)
      var results: [String: Bool] = [:]

      guard let frameURL = NSURL(idnString: dto.data.windowOrigin) as URL? else {
        // Since we can't create a url from the source,
        // we will assume they are all 3rd party
        for urlString in dto.data.urls {
          results[urlString] = false
        }

        replyHandler(results, nil)
        return
      }

      let frameETLD1 = frameURL.baseDomain

      for urlString in dto.data.urls {
        guard let etld1 = (NSURL(idnString: urlString) as? URL)?.baseDomain else {
          // We can't determine a url.
          // Let's assume it's 3rd party
          results[urlString] = false
          continue
        }

        results[urlString] = frameETLD1 == etld1
      }

      replyHandler(results, nil)
    } catch {
      assertionFailure("Invalid type of message. Fix the `RequestBlocking.js` script")
      replyHandler(nil, nil)
    }
  }
}
