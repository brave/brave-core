// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import Preferences
import Shared
import WebKit

class YoutubeQualityScriptHandler: NSObject, TabContentScript {
  private var url: URL?
  private var urlObserver: NSObjectProtocol?

  init(tab: Tab) {
    self.url = tab.url
    super.init()

    urlObserver = tab.webView?.observe(
      \.visibleURL,
      options: [.new],
      changeHandler: { [weak self] object, change in
        guard let self = self, let url = change.newValue else { return }
        if self.url?.withoutFragment != url?.withoutFragment {
          self.url = url

          object.evaluateSafeJavaScript(
            functionName: "window.__firefox__.\(Self.refreshQuality)",
            contentWorld: Self.scriptSandbox,
            asFunction: true
          )
        }
      }
    )
  }

  private static let refreshQuality = "refresh_youtube_quality_\(uniqueID)"
  private static let setQuality = "set_youtube_quality_\(uniqueID)"
  private static let highestQuality = "'hd2160p'"

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

  static func setEnabled(option: Preferences.Option<String>, for tab: Tab) {
    let enabled = canEnableHighQuality(option: option)

    tab.webView?.evaluateSafeJavaScript(
      functionName: "window.__firefox__.\(Self.setQuality)",
      args: [enabled ? Self.highestQuality : "''"],
      contentWorld: Self.scriptSandbox,
      escapeArgs: false,
      asFunction: true
    )
  }

  func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    if !verifyMessage(message: message) {
      assertionFailure("Missing required security token.")
      return
    }

    replyHandler(
      Self.canEnableHighQuality(option: Preferences.General.youtubeHighQuality)
        ? Self.highestQuality : "",
      nil
    )
  }

  private static func canEnableHighQuality(option: Preferences.Option<String>) -> Bool {
    guard let qualityPreference = YoutubeHighQualityPreference(rawValue: option.value) else {
      return false
    }

    switch Reach().connectionStatus() {
    case .offline, .unknown: return false
    case .online(let type):
      if type == .wiFi {
        return qualityPreference == .wifi || qualityPreference == .on
      }

      return qualityPreference == .on
    }
  }
}
