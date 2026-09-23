// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Foundation
import Web
import WebKit

/// Injects the legacy `PlaylistScript` and forwards detected media to the
/// tab's `PlaylistTabHelper`. All processing of the payload lives on the tab
/// helper so that it can be shared with `PlaylistJavaScriptFeature`; this
/// handler only parses the legacy `WKScriptMessage`.
class PlaylistScriptHandler: NSObject, TabContentScript {
  static let playlistLongPressed = "playlistLongPressed_\(uniqueID)"
  static let playlistProcessDocumentLoad = "playlistProcessDocumentLoad_\(uniqueID)"
  static let mediaCurrentTimeFromTag = "mediaCurrentTimeFromTag_\(uniqueID)"

  static let scriptName = "PlaylistScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .page
  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }

    return WKUserScript(
      source: secureScript(
        handlerNamesMap: [
          "$<message_handler>": messageHandlerName,
          "$<tagUUID>": "tagId_\(uniqueID)",
          "$<sendMessageTimeout>": "smt_\(uniqueID)",
          "$<playlistLongPressed>": playlistLongPressed,
          "$<playlistProcessDocumentLoad>": playlistProcessDocumentLoad,
          "$<mediaCurrentTimeFromTag>": mediaCurrentTimeFromTag,
        ],
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

    // Ready state messages are informational only and require no processing.
    if ReadyState.from(message: message) != nil {
      return
    }

    let info = (message.body as? [String: Any]).flatMap(PlaylistInfo.from(dictionary:))
    tab.playlist?.processPlaylistInfo(item: info)
  }
}

extension PlaylistScriptHandler {
  struct ReadyState: Codable {
    let state: String

    static func from(message: WKScriptMessage) -> ReadyState? {
      if !JSONSerialization.isValidJSONObject(message.body) {
        return nil
      }

      guard
        let data = try? JSONSerialization.data(
          withJSONObject: message.body,
          options: [.fragmentsAllowed]
        )
      else {
        return nil
      }

      return try? JSONDecoder().decode(ReadyState.self, from: data)
    }
  }
}
