// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import WebKit

enum VideoBackgroundPlaybackScript: TabContentScriptLoader {
  static let markPlayingVideos = "mark_playing_videos_\(uniqueID)"
  static let resumePlayingVideos = "resume_playing_videos_\(uniqueID)"
  static let scriptSandbox: WKContentWorld = .page

  static let userScript: WKUserScript? = {
    guard let source = loadUserScript(named: "VideoBackgroundPlaybackScript") else {
      return nil
    }
    return WKUserScript(
      source: secureScript(
        handlerNamesMap: [
          "$<mark_playing_videos>": markPlayingVideos,
          "$<resume_playing_videos>": resumePlayingVideos,
        ],
        securityToken: "",
        script: source
      ),
      injectionTime: .atDocumentStart,
      forMainFrameOnly: true,
      in: scriptSandbox
    )
  }()
}
