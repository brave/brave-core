// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Shared
import Web

extension TabDataValues {
  private struct HistoryTabHelperKey: TabDataKey {
    static var defaultValue: HistoryTabHelper?
  }
  public var historyTabHelper: HistoryTabHelper? {
    get { self[HistoryTabHelperKey.self] }
    set { self[HistoryTabHelperKey.self] = newValue }
  }
}

public class HistoryTabHelper: TabObserver {

  private weak var tab: (any TabState)?
  private let historyAPI: BraveHistoryAPI
  private var lastRecordedTitle: String?

  public init(
    tab: some TabState,
    historyAPI: BraveHistoryAPI
  ) {
    self.tab = tab
    self.historyAPI = historyAPI
    tab.addObserver(self)
  }

  private func recordHistoryIfNeeded(for tab: some TabState) {
    guard let url = tab.visibleURL else { return }
    guard !url.isNewTabURL,
      !InternalURL.isValid(url: url) || url.isInternalURL(for: .readermode),
      !url.isFileURL,
      !url.isInternalURL(for: .readermode)
    else { return }
    guard !tab.isPrivate else { return }
    historyAPI.add(url: url, title: tab.title ?? "", dateAdded: Date())
  }

  // MARK: - TabObserver

  public func tabDidFinishNavigation(_ tab: some TabState) {
    recordHistoryIfNeeded(for: tab)
  }

  public func tabDidUpdateURL(_ tab: some TabState) {
    if tab.visibleURL?.origin == tab.previousCommittedURL?.origin {
      recordHistoryIfNeeded(for: tab)
    }
  }

  public func tabDidChangeTitle(_ tab: some TabState) {
    guard let title = (tab.title?.isEmpty == true ? tab.visibleURL?.absoluteString : tab.title)
    else { return }
    if !title.isEmpty && title != lastRecordedTitle {
      lastRecordedTitle = title
      recordHistoryIfNeeded(for: tab)
    }
  }

  public func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }
}
