// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences

extension Preferences {
  struct UserScript {
    public static let blockAllScripts = Option<Bool>(
      key: "userscript.preferences.blockAllScripts",
      default: false
    )

    public static let cookieBlocking =
      Option<Bool>(
        key: "userscript.preferences.\(UserScriptManager.ScriptType.cookieBlocking.rawValue)",
        default: true
      )

    public static let rewardsReporting =
      Option<Bool>(
        key: "userscript.preferences.\(UserScriptManager.ScriptType.rewardsReporting.rawValue)",
        default: true
      )

    public static let mediaBackgroundPlay =
      Option<Bool>(
        key: "userscript.preferences.\(UserScriptManager.ScriptType.mediaBackgroundPlay.rawValue)",
        default: true
      )

    public static let mediaSource =
      Option<Bool>(
        key: "userscript.preferences.\(UserScriptManager.ScriptType.playlistMediaSource.rawValue)",
        default: true
      )

    public static let playlist =
      Option<Bool>(
        key: "userscript.preferences.\(UserScriptManager.ScriptType.playlist.rawValue)",
        default: true
      )

    public static let deAmp =
      Option<Bool>(
        key: "userscript.preferences.\(UserScriptManager.ScriptType.deAmp.rawValue)",
        default: true
      )

    public static let requestBlocking =
      Option<Bool>(
        key: "userscript.preferences.\(UserScriptManager.ScriptType.requestBlocking.rawValue)",
        default: true
      )

    public static let trackingProtectionStats =
      Option<Bool>(
        key:
          "userscript.preferences.\(UserScriptManager.ScriptType.trackerProtectionStats.rawValue)",
        default: true
      )

    public static let readyState =
      Option<Bool>(
        key: "userscript.preferences.\(UserScriptManager.ScriptType.readyStateHelper.rawValue)",
        default: true
      )

    public static let ethereumProvider =
      Option<Bool>(
        key: "userscript.preferences.\(UserScriptManager.ScriptType.ethereumProvider.rawValue)",
        default: true
      )

    public static let solanaProvider =
      Option<Bool>(
        key: "userscript.preferences.\(UserScriptManager.ScriptType.solanaProvider.rawValue)",
        default: true
      )

    public static let youtubeQuality =
      Option<Bool>(
        key: "userscript.preferences.\(UserScriptManager.ScriptType.youtubeQuality.rawValue)",
        default: true
      )

    public static let leo =
      Option<Bool>(
        key: "userscript.preferences.\(UserScriptManager.ScriptType.braveLeoAIChat.rawValue)",
        default: true
      )

    public static let translate =
      Option<Bool>(
        key: "userscript.preferences.\(UserScriptManager.ScriptType.braveTranslate.rawValue)",
        default: true
      )
  }
}
