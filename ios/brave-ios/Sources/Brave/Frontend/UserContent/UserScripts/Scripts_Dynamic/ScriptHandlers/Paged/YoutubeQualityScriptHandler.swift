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

class YoutubeQualityScriptHandler: NSObject, TabContentScript, TabObserver {
  private var url: URL?
  private var reachableObserver: AnyCancellable?

  init(tab: some TabState) {
    self.url = tab.visibleURL
    super.init()

    tab.addObserver(self)

    reachableObserver = Reachability.shared.publisher.sink { [weak tab] status in
      guard let tab = tab else { return }
      Self.handleConnectionStatusChanged(status, tab: tab)
    }
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

  static func setEnabled(for tab: some TabState) {
    handleConnectionStatusChanged(Reachability.shared.status, tab: tab)
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
      Self.canEnableHighQuality(connectionStatus: Reachability.shared.status)
        ? Self.highestQuality : "",
      nil
    )
  }

  private static func canEnableHighQuality(connectionStatus: Reachability.Status) -> Bool {
    guard
      let qualityPreference = YoutubeHighQualityPreference(
        rawValue: Preferences.General.youtubeHighQuality.value
      )
    else {
      return false
    }

    switch connectionStatus.connectionType {
    case .wifi, .ethernet:
      if connectionStatus.isLowDataMode || connectionStatus.isExpensive {
        return qualityPreference == .on
      }

      return qualityPreference == .wifi || qualityPreference == .on

    case .cellular, .other:
      return qualityPreference == .on && !connectionStatus.isLowDataMode
        && !connectionStatus.isExpensive

    case .offline:
      return false
    }
  }

  // MARK: - TabObserver

  func tabDidUpdateURL(_ tab: some TabState) {
    if url?.withoutFragment == tab.visibleURL?.withoutFragment {
      return
    }

    url = tab.visibleURL
    tab.evaluateJavaScript(
      functionName: "window.__firefox__.\(Self.refreshQuality)",
      contentWorld: Self.scriptSandbox,
      asFunction: true
    )
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }

  private static func handleConnectionStatusChanged(
    _ status: Reachability.Status,
    tab: some TabState
  ) {
    let enabled = canEnableHighQuality(connectionStatus: status)
    tab.evaluateJavaScript(
      functionName: "window.__firefox__.\(Self.setQuality)",
      args: [enabled ? Self.highestQuality : "'auto'"],
      contentWorld: Self.scriptSandbox,
      escapeArgs: false,
      asFunction: true
    )
  }
}
