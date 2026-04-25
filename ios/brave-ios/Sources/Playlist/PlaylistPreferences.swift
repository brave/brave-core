// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Preferences
import Shared
import UIKit

// MARK: - PlayListDownloadType

public enum PlayListDownloadType: String, CaseIterable {
  case on
  case off
  case wifi
}

extension PrefService {
  /// Whether or not the Playlist feature in general is available to use and the UI should display
  /// buttons/settings for it.
  public var isPlaylistAvailable: Bool {
    // Right now this feature is always available unless its managed/forced by policy
    let isDisabledByPolicy =
      isManagedPreference(forPath: kPlaylistEnabledPrefName)
      && !boolean(forPath: kPlaylistEnabledPrefName)
    return !isDisabledByPolicy
  }
}

extension Preferences {
  final public class Playlist {
    /// The count of how many times  Add to Playlist URL-Bar onboarding has been shown
    public static let addToPlaylistURLBarOnboardingCount = Option<Int>(
      key: "playlist.addToPlaylistURLBarOnboardingCount",
      default: 0
    )
    /// The last played item url
    public static let lastPlayedItemUrl = Option<String?>(
      key: "playlist.last.played.item.url",
      default: nil
    )
    /// Whether to play the video when controller loaded
    public static let firstLoadAutoPlay = Option<Bool>(
      key: "playlist.firstLoadAutoPlay",
      default: false
    )
    /// The Option to download video yes / no / only wi-fi
    public static let autoDownloadVideo = Option<String>(
      key: "playlist.autoDownload",
      default: PlayListDownloadType.on.rawValue
    )
    /// The option to start the playback where user left-off
    public static let playbackLeftOff = Option<Bool>(key: "playlist.playbackLeftOff", default: true)
    /// The option to disable long-press-to-add-to-playlist gesture.
    public static let enableLongPressAddToPlaylist =
      Option<Bool>(key: "playlist.longPressAddToPlaylist", default: true)
    /// The option to enable or disable the URL-Bar button for playlist
    public static let enablePlaylistURLBarButton =
      Option<Bool>(key: "playlist.enablePlaylistURLBarButton", default: true)
    /// The option to enable or disable the continue where left-off playback in CarPlay
    public static let enableCarPlayRestartPlayback =
      Option<Bool>(key: "playlist.enableCarPlayRestartPlayback", default: false)
    /// The last time all playlist folders were synced
    public static let lastPlaylistFoldersSyncTime =
      Option<Date?>(key: "playlist.lastPlaylistFoldersSyncTime", default: nil)
    /// The date of the last cached data cleanup for dangling playlist items
    public static let lastCacheDataCleanupDate =
      Option<Date?>(key: "playlist.lastCacheDataCleanupDate", default: nil)
    /// Whether or not shuffle mode is enabled
    public static let isShuffleEnabled =
      Option<Bool>(key: "playlist.isShuffleEnabled", default: false)
    /// The last used repeat mode the user had set
    public static let repeatMode = Option<Int>(key: "playlist.repeatMode", default: 0)
  }
}
