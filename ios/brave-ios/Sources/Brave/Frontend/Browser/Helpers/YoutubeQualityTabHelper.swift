// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import Combine
import Foundation
import Observation
import Preferences
import Shared
import Web
import WebKit

extension TabDataValues {
  private struct YoutubeQualityTabHelperKey: TabDataKey {
    static var defaultValue: YoutubeQualityTabHelper?
  }

  var youtubeQualityTabHelper: YoutubeQualityTabHelper? {
    get { self[YoutubeQualityTabHelperKey.self] }
    set { self[YoutubeQualityTabHelperKey.self] = newValue }
  }
}

@MainActor
class YoutubeQualityTabHelper: NSObject, TabObserver {
  private var url: URL?
  private weak var tab: (any TabState)?
  private let prefChangeRegistrar: PrefChangeRegistrar

  public init(tab: some TabState) {
    self.tab = tab
    self.url = tab.visibleURL
    prefChangeRegistrar = .init(prefService: tab.profile.prefs)

    super.init()

    tab.addObserver(self)
    self.observeReachability()

    prefChangeRegistrar.addObserver(forPath: kYouTubeAutoQualityMode) { [weak self] _ in
      self?.setHighQuality(networkStatus: Reachability.shared.status)
    }
  }

  private func observeReachability() {
    _ = withObservationTracking { [weak self] in
      self?.tab?.youtubeQualityTabHelper?.setHighQuality(networkStatus: Reachability.shared.status)
    } onChange: { [weak self] in
      DispatchQueue.main.async {
        self?.observeReachability()
      }
    }
  }

  private func setHighQuality(networkStatus: Reachability.Status) {
    guard let tab else { return }
    let enabled = YoutubeQualityTabHelper.canEnableHighQuality(
      tab: tab,
      connectionStatus: networkStatus
    )
    tab.evaluateJavaScript(
      functionName: "window.__firefox__.\(YoutubeQualityScriptHandler.setQuality)",
      args: [enabled ? YoutubeQualityScriptHandler.highestQuality : "'auto'"],
      contentWorld: YoutubeQualityScriptHandler.scriptSandbox,
      escapeArgs: false,
      asFunction: true
    )
  }

  static func canEnableHighQuality(
    tab: some TabState,
    connectionStatus: Reachability.Status
  ) -> Bool {
    guard
      let qualityPreference = YoutubeHighQualityPreference(
        rawValue: tab.profile.prefs.integer(forPath: kYouTubeAutoQualityMode)
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
