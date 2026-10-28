// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
@_spi(ChromiumWebViewAccess) import Web
import WebKit

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
    callPlaybackFunction(VideoBackgroundPlaybackScript.markPlayingVideos) {
      $0.markPlayingVideos()
    }
  }

  /// Resumes videos recorded by `markPlayingVideos`.
  func resumePlayingVideos() {
    callPlaybackFunction(VideoBackgroundPlaybackScript.resumePlayingVideos) {
      $0.resumePlayingVideos()
    }
  }

  /// The Chromium feature is registered only while
  /// `kUseProfileWebViewConfiguration` is on. Otherwise the page script under
  /// `window.__firefox__` is used.
  private func callPlaybackFunction(
    _ functionName: String,
    chromiumCall: (BraveWebView) -> Void
  ) {
    guard let tab, tab.isWebViewCreated else { return }
    if FeatureList.kUseProfileWebViewConfiguration.enabled {
      if let webView = BraveWebView.from(tab: tab) {
        chromiumCall(webView)
      }
      return
    }
    tab.evaluateJavaScript(
      functionName: "window.__firefox__.\(functionName)",
      contentWorld: VideoBackgroundPlaybackScript.scriptSandbox,
      asFunction: true
    )
  }
}
