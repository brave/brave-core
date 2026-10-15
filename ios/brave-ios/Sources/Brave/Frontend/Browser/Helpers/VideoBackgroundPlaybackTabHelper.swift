// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
@_spi(ChromiumWebViewAccess) import Web

extension TabDataValues {
  private struct VideoBackgroundPlaybackTabHelperKey: TabDataKey {
    static var defaultValue: VideoBackgroundPlaybackTabHelper?
  }
  var videoBackgroundPlayback: VideoBackgroundPlaybackTabHelper? {
    get { self[VideoBackgroundPlaybackTabHelperKey.self] }
    set { self[VideoBackgroundPlaybackTabHelperKey.self] = newValue }
  }
}

class VideoBackgroundPlaybackTabHelper {
  private weak var tab: (any TabState)?

  init?(tab: some TabState) {
    if !tab.isChromiumTab {
      return nil
    }
    self.tab = tab
  }

  /// Records which videos are still playing before WebKit pauses them.
  func markPlayingVideos() {
    guard let tab, tab.isWebViewCreated, let webView = BraveWebView.from(tab: tab) else { return }
    webView.markPlayingVideos()
  }

  /// Resumes videos recorded by `markPlayingVideos`.
  func resumePlayingVideos() {
    guard let tab, tab.isWebViewCreated, let webView = BraveWebView.from(tab: tab) else { return }
    webView.resumePlayingVideos()
  }
}
