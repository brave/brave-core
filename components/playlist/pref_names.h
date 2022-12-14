// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PLAYLIST_PREF_NAMES_H_
#define BRAVE_COMPONENTS_PLAYLIST_PREF_NAMES_H_

namespace playlist {

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
constexpr char kPlaylistsPref[] = "brave.playlist.lists";

// Stores playlist item key-value pairs in a dict. Each item has unique key and
// it's metadata(such as, title, media file path and etc..).
constexpr char kPlaylistItemsPref[] = "brave.playlist.items";

// Boolean pref indicates that we should cache media file when adding items.
constexpr char kPlaylistCacheByDefault[] = "brave.playlist.cache";

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PREF_NAMES_H_
