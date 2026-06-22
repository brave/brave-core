// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShields
import Data
import Preferences
import Shared
import Web
import WebKit
import os.log

extension ContentBlockerHelper: TabContentScript {
  private struct ContentBlockerDTO: Decodable {
    struct ContentblockerDTOData: Decodable {
      let resourceType: AdblockEngine.ResourceType
      let resourceURL: String
    }

    let securityToken: String
    let data: [ContentblockerDTOData]
  }

  static let scriptName = "TrackingProtectionStats"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .page

  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }

    return WKUserScript(
      source: secureScript(
        handlerName: messageHandlerName,
        securityToken: UserScriptManager.securityToken,
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

    if !verifyMessage(message: message, securityToken: UserScriptManager.securityToken) {
      assertionFailure("Missing required security token.")
      return
    }

    // The frame's security origin is the source URL used to determine whether
    // each reported resource should be blocked.
    guard let sourceURL = URLOrigin(wkSecurityOrigin: message.frameInfo.securityOrigin).url else {
      return
    }

    do {
      let data = try JSONSerialization.data(withJSONObject: message.body)
      let dtos = try JSONDecoder().decode(ContentBlockerDTO.self, from: data).data
      let resources = dtos.map {
        ProtectionStatsTabHelper.BlockedResource(
          resourceURL: $0.resourceURL,
          resourceType: $0.resourceType
        )
      }
      tab.protectionStats?.handleBlockedResources(resources, sourceURL: sourceURL)
    } catch {
      Logger.module.error("\(error.localizedDescription)")
    }
  }
}
