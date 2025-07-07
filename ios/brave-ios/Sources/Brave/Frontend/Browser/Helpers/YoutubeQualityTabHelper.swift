// Copyright 2025 The Brave Authors. All rights reserved.
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

class YoutubeQualityTabHelper: NSObject, TabObserver {
  private var url: URL?
  private weak var tab: (any TabState)?
  private var reachableObserver: AnyCancellable?

  public init(tab: (any TabState)?) {
    self.tab = tab
    self.url = tab?.visibleURL
    super.init()

    tab?.addObserver(self)

    reachableObserver = Reachability.shared.publisher.sink { [weak tab] status in
      tab?.youtubeQualityTabHelper?.handleConnectionStatusChanged(status: status)
    }
  }

  func handleConnectionStatusChanged(status: Reachability.Status) {
    let enabled = YoutubeQualityTabHelper.canEnableHighQuality(connectionStatus: status)
    tab?.evaluateJavaScript(
      functionName: "window.__firefox__.\(YoutubeQualityScriptHandler.setQuality)",
      args: [enabled ? YoutubeQualityScriptHandler.highestQuality : "'auto'"],
      contentWorld: YoutubeQualityScriptHandler.scriptSandbox,
      escapeArgs: false,
      asFunction: true
    )
  }

  static func canEnableHighQuality(connectionStatus: Reachability.Status) -> Bool {
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
      functionName: "window.__firefox__.\(YoutubeQualityScriptHandler.refreshQuality)",
      contentWorld: YoutubeQualityScriptHandler.scriptSandbox,
      asFunction: true
    )
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }
}
