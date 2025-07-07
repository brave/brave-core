// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Combine
import Foundation
import Preferences
import Shared
import Web
import WebKit

class YoutubeQualityScriptHandler: NSObject, TabContentScript {

  static let refreshQuality = "refresh_youtube_quality_\(uniqueID)"
  static let setQuality = "set_youtube_quality_\(uniqueID)"
  static let highestQuality = "'hd2160p'"

  static let scriptName = "YoutubeQualityScript"
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
          "$<refresh_youtube_quality>": refreshQuality,
          "$<set_youtube_quality>": setQuality,
        ],
        securityToken: scriptId,
        script: script
      ),
      injectionTime: .atDocumentStart,
      forMainFrameOnly: true,
      in: scriptSandbox
    )
  }()

  static func setEnabled(for tab: some TabState) {
    tab.youtubeQualityTabHelper?.handleConnectionStatusChanged(status: Reachability.shared.status)
  }

  func tab(
    _ tab: some TabState,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      return
    }

    replyHandler(
      YoutubeQualityTabHelper.canEnableHighQuality(connectionStatus: Reachability.shared.status)
        ? Self.highestQuality : "",
      nil
    )
  }
}
