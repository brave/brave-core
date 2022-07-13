// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PLAYLIST_PREF_NAMES_H_
#define BRAVE_COMPONENTS_PLAYLIST_PREF_NAMES_H_

namespace playlist {

// Lists of playlist items. Each list has ids of it's items
// so that multiple lists can share same item efficiently
// e.g. list [{ name: "playlist1", items: [id1, id2, id3] },
//            { name: "playlist2", items: [id1, id4, id5] }]
//                                          ^ same item
constexpr char kPlaylistListsPref[] = "brave.playlist.lists";

// Stores playlist item key-value pairs in a dict. Each item has unique key and
// it's metadata(such as, title, media file path and etc..).
constexpr char kPlaylistItemsPref[] = "brave.playlist.items";

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PREF_NAMES_H_
