// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation
import Web

extension TabDataValues {
  private struct WidgetSearchTabHelperKey: TabDataKey {
    static var defaultValue: WidgetSearchTabHelper? { nil }
  }

  var widgetSearchTabHelper: WidgetSearchTabHelper? {
    get { self[WidgetSearchTabHelperKey.self] }
    set { self[WidgetSearchTabHelperKey.self] = newValue }
  }
}

/// Marks a tab as having a pending widget-initiated search whose next Brave Search request
/// should use `source=ios-widget`.
///
/// Install the widget tab helper when a widget search is initiated;
/// It tears itself down once the attribution is applied or the tab navigates elsewhere
final class WidgetSearchTabHelper: TabObserver {
  private enum BraveSearchSource {
    static let host = "search.brave.com"
    static let queryKey = "source"
    static let appValue = "ios"
    static let widgetValue = "ios-widget"
  }

  private weak var tab: (any TabState)?

  init(tab: some TabState) {
    self.tab = tab
    tab.addObserver(self)
  }

  deinit {
    tab?.removeObserver(self)
  }

  // MARK: - TabObserver

  func tabDidCommitNavigation(_ tab: some TabState) {
    guard self.tab === tab else { return }
    // Don't tear down on the NTP commit that precedes the actual widget search navigation.
    guard let url = tab.lastCommittedURL, !url.isNewTabURL else { return }
    detach()
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }

  // MARK: - Attribution

  /// Returns `url` rewritten with widget `source` attribution if the engine is Brave Search,
  /// and tears the helper down. Returns `url` unchanged for other engines (still tearing down,
  /// since the pending widget search has been resolved).
  func finalize(_ url: URL, forEngine engineName: String?) -> URL {
    defer { detach() }
    guard engineName == OpenSearchEngine.EngineNames.brave else {
      return url
    }
    return Self.applyWidgetAttribution(url)
  }

  /// Removes this helper from its tab. After this call the helper is no longer reachable
  /// via `TabDataValues` and will be deallocated once the current call stack unwinds.
  private func detach() {
    tab?.widgetSearchTabHelper = nil
  }

  // MARK: - URL rewriting

  /// Rewrites `source=ios` to `source=ios-widget` on `search.brave.com`.
  static func applyWidgetAttribution(_ url: URL) -> URL {
    guard url.host?.lowercased() == BraveSearchSource.host,
      var components = URLComponents(url: url, resolvingAgainstBaseURL: false),
      let items = components.queryItems,
      items.contains(where: {
        $0.name == BraveSearchSource.queryKey && $0.value == BraveSearchSource.appValue
      })
    else { return url }

    components.queryItems = items.map { item in
      (item.name == BraveSearchSource.queryKey && item.value == BraveSearchSource.appValue)
        ? URLQueryItem(name: BraveSearchSource.queryKey, value: BraveSearchSource.widgetValue)
        : item
    }
    return components.url ?? url
  }
}
