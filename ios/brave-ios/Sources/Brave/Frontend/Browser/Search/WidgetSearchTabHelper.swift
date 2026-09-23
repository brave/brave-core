// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
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
/// It tears itself down once the search is committed or the tab navigates elsewhere.
final class WidgetSearchTabHelper: TabObserver {
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
    // detach this helper from its tab. After this call the helper is no longer reachable
    // via `TabDataValues` and will be deallocated once the current call stack unwinds.
    tab.widgetSearchTabHelper = nil
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }
}
