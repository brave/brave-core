// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Shared
@_spi(ChromiumWebViewAccess) import Web

extension TabDataValues {
  private struct ReaderModeTabHelperKey: TabDataKey {
    static var defaultValue: ReaderModeTabHelper?
  }
  var readerMode: ReaderModeTabHelper? {
    get { self[ReaderModeTabHelperKey.self] }
    set { self[ReaderModeTabHelperKey.self] = newValue }
  }
}

class ReaderModeTabHelper: TabObserver {
  private let tab: any TabState
  private(set) var readabilityResult: ReadabilityResult?

  init?(tab: some TabState) {
    if !tab.isChromiumTab {
      return nil
    }
    self.tab = tab
    self.tab.addObserver(self)
  }

  deinit {
    tab.removeObserver(self)
  }

  var state: ReaderModeState {
    if let url = tab.visibleURL.flatMap(InternalURL.init), url.isReaderModePage {
      return .active
    }
    return readabilityResult != nil ? .available : .unavailable
  }

  /// Checks the web views readability then assigns `readabilityResult`
  @MainActor
  func checkReadability() async {
    guard let url = tab.lastCommittedURL, url.isWebPage(includeDataURIs: false),
      let webView = BraveWebView.from(tab: tab)
    else {
      readabilityResult = nil
      return
    }
    guard let result = await webView.checkReadability() else {
      // No Reader Mode
      readabilityResult = nil
      return
    }
    do {
      let jsonObject = try JSONSerialization.jsonObject(with: Data(result.utf8)) as AnyObject
      readabilityResult = ReadabilityResult(object: jsonObject)
    } catch {
      readabilityResult = nil
    }
  }

  @MainActor func setStyle(_ style: ReaderModeStyle) {
    guard state == .active, let webView = BraveWebView.from(tab: tab) else { return }
    webView.setReaderModeTheme(
      style.theme.rawValue,
      fontType: style.fontType.rawValue,
      fontSize: style.fontSize.rawValue
    )
  }

  func tabDidStartNavigation(_ tab: some TabState) {
    readabilityResult = nil
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }
}
