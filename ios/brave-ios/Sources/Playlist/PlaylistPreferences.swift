// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import Shared
import UIKit

// MARK: - PlayListSide

public enum PlayListSide: String, CaseIterable {
  case left
  case right
}

// MARK: - PlayListDownloadType

public enum PlayListDownloadType: String, CaseIterable {
  case on
  case off
  case wifi
}

extension Preferences {
  final public class Playlist {
    /// The Option to show video list left or right side
    public static let listViewSide = Option<String>(
      key: "playlist.listViewSide",
      default: PlayListSide.left.rawValue
    )
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
    /// The option to enable or disable the 3-dot menu badge for playlist
    public static let enablePlaylistMenuBadge =
      Option<Bool>(key: "playlist.enablePlaylistMenuBadge", default: true)
    /// The option to enable or disable the URL-Bar button for playlist
    public static let enablePlaylistURLBarButton =
      Option<Bool>(key: "playlist.enablePlaylistURLBarButton", default: true)
    /// The option to enable or disable the continue where left-off playback in CarPlay
    public static let enableCarPlayRestartPlayback =
      Option<Bool>(key: "playlist.enableCarPlayRestartPlayback", default: false)
    /// The last time all playlist folders were synced
    public static let lastPlaylistFoldersSyncTime =
      Option<Date?>(key: "playlist.lastPlaylistFoldersSyncTime", default: nil)
    /// Sync shared folders automatically preference
    public static let syncSharedFoldersAutomatically =
      Option<Bool>(key: "playlist.syncSharedFoldersAutomatically", default: true)
    /// The date of the last cached data cleanup for dangling playlist items
    public static let lastCacheDataCleanupDate =
      Option<Date?>(key: "playlist.lastCacheDataCleanupDate", default: nil)
  }
}
