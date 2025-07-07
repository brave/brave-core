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

  static func handleConnectionStatusChanged(
    _ status: Reachability.Status,
    tab: some TabState
  ) {
    YoutubeQualityScriptHandler.setQuality(for: tab, status: status)
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
    YoutubeQualityScriptHandler.refreshQuality(for: tab)
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }
}
