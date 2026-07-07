// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI
import UIKit
import Web

enum QuickViewActionButton {
  case shield
  case playlist
  case readerMode
  case translate
  case refresh
  case close
  case back
  case forward
  case share
  case openTab
  case sslStatus
}

@Observable
class QuickViewToolbarModel {
  var url: URL
  var secureContentState: SecureContentState = .unknown
  var secondaryTopButton: QuickViewActionButton? {
    if readerModeState != .unavailable { return .readerMode }
    return nil
  }
  let browserColors: any BrowserColors
  var canGoBack: Bool = false
  var canGoForward: Bool = false
  var isShieldEnabled: Bool = true
  var readerModeState: ReaderModeState = .unavailable
  var isPlaylistEnabled: Bool = false
  var isTranslateEnabled: Bool = false
  var isLoading: Bool = true
  var loadingProgress: Double = 0.0
  var onActionButton: ((QuickViewActionButton) -> Void)?

  init(
    url: URL,
    browserColors: any BrowserColors,
    onActionButton: ((QuickViewActionButton) -> Void)? = nil
  ) {
    self.url = url
    self.browserColors = browserColors
    self.onActionButton = onActionButton
  }
}

extension QuickViewToolbarModel: TabObserver {
  func tabDidUpdateURL(_ tab: some TabState) {
    self.url = tab.visibleURL?.displayURL ?? tab.visibleURL ?? url
    secureContentState = tab.visibleSecureContentState
  }

  func tabDidStartLoading(_ tab: some TabState) {
    isLoading = true
  }

  func tabDidStopLoading(_ tab: some TabState) {
    isLoading = false
  }

  func tabDidChangeLoadProgress(_ tab: some TabState) {
    loadingProgress = tab.estimatedProgress
  }

  func tabDidChangeBackForwardState(_ tab: some TabState) {
    let forwardIsReaderMode =
      tab.backForwardList?.forwardList.first?.url.isInternalURL(for: .readermode) == true
    canGoForward = tab.canGoForward && !forwardIsReaderMode
    canGoBack = tab.canGoBack
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }

  func tabDidChangeVisibleSecurityState(_ tab: some TabState) {
    secureContentState = tab.visibleSecureContentState
  }
}
