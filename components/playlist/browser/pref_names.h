// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PREF_NAMES_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PREF_NAMES_H_

namespace playlist {

// A boolean preference indicates whether playlist feature is enabled or not by
// the user.
inline constexpr char kPlaylistEnabledPref[] = "brave.playlist.enabled";

// Set of playlists. Each playlist has ids of its items
// so that playlists can share same item efficiently
// Currently, List type preference always has to be updated entirely but there
// are many cases where we only need update small part of it.
// Thus, in order to update playlists efficiently, this pref is in Dictionary
//
// e.g. {
//        "list1": {name: "playlist1", items: [id1, id2, id3]},
//        "list2": {name: "playlist2", items: [id1, id4, id5] }
//      }                                       ^ same item
inline constexpr char kPlaylistsPref[] = "brave.playlist.lists";

// Stores order of playlists.
// ["id1", "id2" ... ]
inline constexpr char kPlaylistOrderPref[] = "brave.playlist.lists_order";

// Stores playlist item key-value pairs in a dict. Each item has unique key and
// it's metadata(such as, title, media file path and etc..).
inline constexpr char kPlaylistItemsPref[] = "brave.playlist.items";

// Boolean pref indicates that we should cache media file when adding items.
inline constexpr char kPlaylistCacheByDefault[] = "brave.playlist.cache";

// A string indicates to which playlist items should be added by default.
inline constexpr char kPlaylistDefaultSaveTargetListID[] =
    "brave.playlist.default_save_target_list_id";

// Timestamp indicating first usage of playlist
inline constexpr char kPlaylistFirstUsageTime[] =
    "brave.playlist.first_usage_time";

// Timestamp indicating last usage of playlist
inline constexpr char kPlaylistLastUsageTime[] =
    "brave.playlist.last_usage_time";

// Boolean indicating if playlist was used on second day
// on first week of usage
inline constexpr char kPlaylistUsedSecondDay[] =
    "brave.playlist.used_second_day";

// Weekly storage event list counting usages of playlist
inline constexpr char kPlaylistUsageWeeklyStorage[] =
    "brave.playlist.usage_weekly_storage";

// Pref to save hls content for android
inline constexpr char kHlsContentsPref[] = "brave.playlist.hls_contents";

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PREF_NAMES_H_
